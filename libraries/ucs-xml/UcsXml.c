/*------------------------------------------------------------------------------------------------*/
/* UNICENS XML Parser                                                                             */
/* Copyright 2017, Microchip Technology Inc. and its subsidiaries.                                */
/*                                                                                                */
/* Redistribution and use in source and binary forms, with or without                             */
/* modification, are permitted provided that the following conditions are met:                    */
/*                                                                                                */
/* 1. Redistributions of source code must retain the above copyright notice, this                 */
/*    list of conditions and the following disclaimer.                                            */
/*                                                                                                */
/* 2. Redistributions in binary form must reproduce the above copyright notice,                   */
/*    this list of conditions and the following disclaimer in the documentation                   */
/*    and/or other materials provided with the distribution.                                      */
/*                                                                                                */
/* 3. Neither the name of the copyright holder nor the names of its                               */
/*    contributors may be used to endorse or promote products derived from                        */
/*    this software without specific prior written permission.                                    */
/*                                                                                                */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                      */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE                   */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                     */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                     */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                  */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                  */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*------------------------------------------------------------------------------------------------*/
#include <assert.h>
#include <string.h>
#include <assert.h>
#include "mxml.h"
#include "UcsXml_Private.h"
#include "UcsXml.h"

#ifdef XML_FILE_SUPPORTED
#include <stdio.h>
#endif

/************************************************************************/
/* PRIVATE DECLARATIONS                                                 */
/************************************************************************/

#define COMPILETIME_CHECK(cond)  (void)sizeof(int[2 * !!(cond) - 1])
#define RETURN_ASSERT(result, reason) { UcsXml_CB_OnError("Assertion in file=%s, line=%d reason='%s'", 3, __FILE__, __LINE__, reason); assert(false); return result; }
#define MISC_HB(value)      ((uint8_t)((uint16_t)(value) >> 8))
#define MISC_LB(value)      ((uint8_t)((uint16_t)(value) & (uint16_t)0xFF))
#define ROUTE_AUTO_ID_START (0x8000)
#define ROUTE_INVALID_ID    (0xFFFF)

struct UcsXmlRoute
{
    bool isSource;
    bool isActive;
    uint16_t routeId;
    char routeName[32];
    Ucs_Rm_EndPoint_t *ep;
    struct UcsXmlRoute *next;
};

struct UcsXmlScript
{
    bool singleShot;
    bool inUse;
    char scriptName[32];
    Ucs_Rm_Node_t *node;
    struct UcsXmlScript *next;
};

struct UcsXmlDriverInfoList
{
    DriverInformation_t *driverInfo;
    struct UcsXmlDriverInfoList *next;
};

struct UcsXmlJobList
{
    Ucs_Xrm_ResObject_t *job;
    struct UcsXmlJobList *next;
};

typedef enum
{
    MSocket_NETWORK = 20,
    MSocket_USB,
    MSocket_MLB,
    MSocket_STRM,
    MSocket_SPLITTER,
    MSocket_COMBINER
} MSocketType_t;

typedef enum
{
    Parse_Success = 10,
    Parse_MemoryError,
    Parse_XmlError
} ParseResult_t;

typedef struct
{
    Ucs_Rm_Node_t *nod;
    Ucs_Xrm_UsbPort_t *usbPort;
    Ucs_Xrm_MlbPort_t *mlbPort;
    Ucs_Xrm_StrmPort_t *strmPortA;
    Ucs_Xrm_StrmPort_t *strmPortB;
} NodeData_t;

typedef struct
{
    MDataType_t dataType;
    uint8_t sockCnt;
    bool syncOffsetNeeded;
    bool isDeactivated;
    uint16_t routeId;
    uint16_t syncOffset;
    const char *routeName;
    Ucs_Xrm_ResObject_t *inSocket;
    Ucs_Xrm_ResObject_t *outSocket;
    struct UcsXmlJobList *jobList;
    Ucs_Xrm_Combiner_t *combiner;
    mxml_node_t *pendingCombinerSockets;
    Ucs_Sync_MuteMode_t muteMode;
    Ucs_Avp_IsocPacketSize_t isocPacketSize;
} ConnectionData_t;

typedef struct
{
    uint16_t pause;
} ScriptData_t;

typedef struct {
    uint16_t autoRouteId;
    struct UcsXmlObjectList objList;
    struct UcsXmlRoute *pRtLst;
    struct UcsXmlScript *pScrLst;
    NodeData_t nodeData;
    ConnectionData_t conData;
    struct UcsXmlDriverInfoList *drvInfLst;
} PrivateData_t;

typedef struct {
    struct UcsXmlObjectList objList;
    struct UcsXmlScript *pScrLst;
    ScriptData_t scriptData;
} PrivateDataScript_t;

/************************************************************************/
/* Constants                                                            */
/************************************************************************/

/*Key section*/
static const char* UNICENS =                "Unicens";
static const char* PACKET_BW =              "AsyncBandwidth";
static const char* NAME =                   "Name";
static const char* ROUTE =                  "Route";
static const char* ROUTE_ID =               "RouteId";
static const char* ROUTE_IS_ACTIVE =        "IsActive";
static const char* ENDPOINT_ADDRESS =       "EndpointAddress";
static const char* CHANNEL_ADDRESS =        "ChannelAddress";
static const char* BANDWIDTH =              "Bandwidth";
static const char* BYTES_PER_FRAME =        "BytesPerFrame";
static const char* OFFSET =                 "Offset";
static const char* NODE =                   "Node";
static const char* CLOCK_CONFIG =           "ClockConfig";
static const char* ADDRESS =                "Address";
static const char* FRAMES_PER_TRANSACTION = "FramesPerTransaction";
static const char* MUTE_MODE =              "MuteMode";
static const char* MUTE_MODE_NO_MUTING =    "NoMuting";
static const char* MUTE_MODE_MUTE_SIGNAL =  "MuteSignal";
static const char* AVP_PACKET_SIZE =        "IsocPacketSize";
#define SYNC_CONNECTION                     "SyncConnection"
#define AVP_CONNECTION                      "AVPConnection"
#define DFP_CONNECTION                      "DFPhaseConnection"
#define QOS_CONNECTION                      "QoSConnection"
#define IPC_CONNECTION                      "IPCConnection"

static const char* ALL_CONNECTIONS[] = { SYNC_CONNECTION, AVP_CONNECTION,
                        DFP_CONNECTION, QOS_CONNECTION, IPC_CONNECTION, NULL };

#define NETWORK_SOCKET                      "NetworkSocket"
#define USB_SOCKET                          "USBSocket"
#define MLB_SOCKET                          "MediaLBSocket"
#define STREAM_SOCKET                       "StreamSocket"
#define SPLITTER                            "Splitter"
#define COMBINER                            "Combiner"
static const char* ALL_SOCKETS[] = { NETWORK_SOCKET, USB_SOCKET, MLB_SOCKET,
                        STREAM_SOCKET, SPLITTER, COMBINER, NULL };

#define MLB_PORT                            "MediaLBPort"
#define USB_PORT                            "USBPort"
#define STRM_PORT                           "StreamPort"
static const char* ALL_PORTS[] = { MLB_PORT, USB_PORT, STRM_PORT, NULL };

static const char* PHYSICAL_LAYER =         "PhysicalLayer";
static const char* DEVICE_INTERFACES =      "DeviceInterfaces";
static const char* STRM_IN_COUNT =          "StreamingIfEpInCount";
static const char* STRM_OUT_COUNT =         "StreamingIfEpOutCount";

static const char* STRM_PIN =                "StreamPinID";
static const char* STRM_ALIGN =              "DataAlignment";

static const char* SCRIPT =                 "Script";
static const char* FBLOCK_ID =              "FBlockId";
static const char* FUNCTION_ID =            "FunctionId";
static const char* OP_TYPE_REQUEST =        "OpTypeRequest";
static const char* OP_TYPE_RESPONSE =       "OpTypeResponse";
static const char* PAYLOAD_REQ_HEX =        "PayloadRequest";
static const char* PAUSE_MS =               "WaitTime";
static const char* DEBOUNCE_TIME =          "DebounceTime";
static const char* PIN_CONFIG =             "PinConfiguration";
static const char* PIN_MASK =               "Mask";
static const char* PIN_DATA =               "Data";
static const char* I2C_SPEED =              "Speed";
static const char* I2C_SPEED_SLOW =         "SlowMode";
static const char* I2C_SPEED_FAST =         "FastMode";
static const char* I2C_WRITE_MODE =         "Mode";
static const char* I2C_WRITE_MODE_DEFAULT = "DefaultMode";
static const char* I2C_WRITE_MODE_REPEAT =  "RepeatedStartMode";
static const char* I2C_WRITE_MODE_BURST =   "BurstMode";
static const char* I2C_WRITE_BLOCK_COUNT =  "BlockCount";
static const char* I2C_SLAVE_ADDRESS =      "Address";
static const char* I2C_PAYLOAD_LENGTH =     "Length";
static const char* I2C_PAYLOAD =            "Payload";
static const char* I2C_TIMEOUT =            "Timeout";

#define SCRIPT_MSG_SEND                     "MsgSend"
#define SCRIPT_PAUSE                        "Pause"
#define SCRIPT_GPIO_PORT_CREATE             "GPIOPortCreate"
#define SCRIPT_GPIO_PORT_PIN_MODE           "GPIOPortPinMode"
#define SCRIPT_GPIO_PIN_STATE               "GPIOPinState"
#define SCRIPT_I2C_PORT_CREATE              "I2CPortCreate"
#define SCRIPT_I2C_PORT_WRITE               "I2CPortWrite"
#define SCRIPT_I2C_PORT_READ                "I2CPortRead"
static const char* ALL_SCRIPTS[] = { SCRIPT_MSG_SEND, SCRIPT_PAUSE,
    SCRIPT_GPIO_PORT_CREATE, SCRIPT_GPIO_PORT_PIN_MODE, SCRIPT_GPIO_PIN_STATE,
    SCRIPT_I2C_PORT_CREATE, SCRIPT_I2C_PORT_WRITE, SCRIPT_I2C_PORT_READ, NULL };
static const char* ALL_SCRIPTS_NO_PAUSE[] = { SCRIPT_MSG_SEND,
    SCRIPT_GPIO_PORT_CREATE, SCRIPT_GPIO_PORT_PIN_MODE, SCRIPT_GPIO_PIN_STATE,
    SCRIPT_I2C_PORT_CREATE, SCRIPT_I2C_PORT_WRITE, SCRIPT_I2C_PORT_READ, NULL };

static const char* L_DRIVER_ATTR =          "Driver";
static const char* L_DRIVER_TAG =           "Driver";
static const char* L_DRIVER_NAME =          "Name";
static const char* L_BUFFERSIZE =           "BufferSize";
static const char* L_BUFFFERCOUNT =         "BufferCount";
static const char* L_ALSA_CH_COUNT=         "AudioChannelCount";
static const char* L_ALSA_CH_RES =          "AudioChannelResolution";
static const char* L_DRIVER_ALSARES_8BIT =  "8bit";
static const char* L_DRIVER_ALSARES_16BIT = "16bit";
static const char* L_DRIVER_ALSARES_24BIT = "24bit";
static const char* L_DRIVER_ALSARES_32BIT = "32bit";

#define L_DRIVER_ALSA                       "Alsa"
#define L_DRIVER_CDEV                       "Cdev"
#define L_DRIVER_V4L2                       "V4l2"
static const char * ALL_DRIVERS[] = { L_DRIVER_ALSA, L_DRIVER_CDEV, L_DRIVER_V4L2, NULL };

static const char* VALUE_TRUE =             "true";
static const char* VALUE_FALSE =            "false";
static const char* VALUE_1 =                "1";
static const char* VALUE_0 =                "0";

/************************************************************************/
/* Private Function Prototypes                                          */
/************************************************************************/

#ifdef XML_FILE_SUPPORTED
static char *ReadFile(const char *fileName);
#endif
static void FreeVal(UcsXmlVal_t *ucs);
static void FreeValScript(UcsXmlScript_t *script);
static bool GetElement(mxml_node_t *element, const char *name, bool goDeep, mxml_node_t **out, bool mandatory);
static bool GetElementArray(mxml_node_t *element, const char *array[], const char **foundName, mxml_node_t **out, bool skipFirstElement);
static bool GetCount(mxml_node_t *element, const char *name, uint32_t *out, bool mandatory);
static bool GetCountArray(mxml_node_t *element, const char *array[], uint32_t *out, bool mandatory);
static bool GetString(mxml_node_t *element, const char *key, const char **out, bool mandatory);
static bool CheckInteger(const char *val, bool forceHex);
static bool GetUInt16(mxml_node_t *element, const char *key, uint16_t *out, bool mandatory);
static bool GetUInt8(mxml_node_t *element, const char *key, uint8_t *out, bool mandatory);
static bool GetSocketType(const char *txt, MSocketType_t *out);
static bool GetPayload(mxml_node_t *element, const char *name, uint8_t **pPayload, uint8_t *len, uint8_t offset,
            struct UcsXmlObjectList *obj, bool mandatory);
static Ucs_Xrm_ResourceType_t GetResourceType(Ucs_Xrm_ResObject_t *element);
static bool AddJob(struct UcsXmlJobList **joblist, Ucs_Xrm_ResObject_t *job, struct UcsXmlObjectList *objList);
static Ucs_Xrm_ResObject_t **GetJobList(struct UcsXmlJobList *joblist, struct UcsXmlObjectList *objList);
static struct UcsXmlJobList *DeepCopyJobList(struct UcsXmlJobList *jobsIn, struct UcsXmlObjectList *objList);
static void AddRoute(struct UcsXmlRoute **pRtLst, struct UcsXmlRoute *route);
static void AddScript(struct UcsXmlScript **pScrLst, struct UcsXmlScript *script);
static ParseResult_t ParseAll(mxml_node_t *tree, UcsXmlVal_t *ucs, PrivateData_t *priv);
static ParseResult_t ParseNode(mxml_node_t * node, PrivateData_t *priv);
static ParseResult_t ParseConnection(mxml_node_t * node, const char *conType, PrivateData_t *priv);
static ParseResult_t ParseSocket(mxml_node_t *soc, bool isSource, MSocketType_t socketType, struct UcsXmlJobList **jobList, PrivateData_t *priv);
static ParseResult_t ParseAllScripts(mxml_node_t *tree, struct UcsXmlScript *scrlist, struct UcsXmlObjectList *objList, bool checkReference);
static ParseResult_t ParseScript(mxml_node_t *scr, struct UcsXmlScript *scrlist, struct UcsXmlObjectList *objList);
static bool FillScriptInitialValues(Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptMsgSend(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptGpioPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptGpioPinMode(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptGpioPinState(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptPortWrite(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptPortRead(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData);
static ParseResult_t ParseScriptPause(mxml_node_t *act, ScriptData_t *scriptData);
static ParseResult_t ParseRoutes(UcsXmlVal_t *ucs, PrivateData_t *priv);
static bool AddDriverInfo(struct UcsXmlDriverInfoList **drvList, DriverInformation_t *drvInfo, struct UcsXmlObjectList *objList);
static ParseResult_t ParseDriver(mxml_node_t *soc, UcsXmlVal_t *ucs, PrivateData_t *priv);
static ParseResult_t StoreDriverInfo(PrivateData_t *priv, const char *driverLink);
static uint16_t GetDrvInfCount(struct UcsXmlDriverInfoList *drvInfLst);
static void FillDriverArray(struct UcsXmlDriverInfoList *drvInfLst, DriverInformation_t **ppDriveInfo);

/************************************************************************/
/* Public Functions                                                     */
/************************************************************************/

UcsXmlVal_t *UcsXml_Parse(const char *xmlString)
{
    UcsXmlVal_t *val = NULL;
    ParseResult_t result = Parse_MemoryError;
    mxml_node_t *tree;
    if (!(tree = mxmlLoadString(NULL, xmlString, MXML_NO_CALLBACK)))
    {
        UcsXml_CB_OnError("XML is not parsable, double check that is well formed!", 0);
        goto ERROR;
    }
    if (!GetElement(tree, UNICENS, true, &tree, true)) goto ERROR;
    /*Do not use MCalloc for the root element*/
    val = calloc(1, sizeof(UcsXmlVal_t));
    if (!val) goto ERROR;
    val->pInternal = calloc(1, sizeof(PrivateData_t));
    if (!val->pInternal) goto ERROR;
    result = ParseAll(tree, val, val->pInternal);
    if (Parse_Success == result)
        return val;
ERROR:
    if (Parse_MemoryError == result)
        UcsXml_CB_OnError("XML memory error, aborting..", 0);
    else
        UcsXml_CB_OnError("XML parsing error, aborting..", 0);
    if (tree)
        mxmlDelete(tree);
    if (val)
        FreeVal(val);
    return NULL;
}

#ifdef XML_FILE_SUPPORTED
UcsXmlVal_t *UcsXml_ParseFile(const char *fileName)
{
    UcsXmlVal_t *val;
    char *content = ReadFile(fileName);
    if (NULL == content)
    {
        UcsXml_CB_OnError("UcsXml_ParseFile:Could not read file:'%s'", 1, fileName);
        return NULL;
    }
    val = UcsXml_Parse(content);
    free(content);
    return val;
}
#endif

void UcsXml_FreeVal(UcsXmlVal_t *val)
{
    FreeVal(val);
}

UcsXmlScript_t *UcsXml_ParseScript(const char *xmlString)
{
    ParseResult_t result = Parse_MemoryError;
    UcsXmlScript_t *script = NULL;
    mxml_node_t *tree = NULL;
    PrivateDataScript_t *priv = NULL;
    struct UcsXmlScript scriptLst = { 0 };
    Ucs_Rm_Node_t node = { 0 };
    scriptLst.node = &node;
    scriptLst.singleShot = true;
    if (!(tree = mxmlLoadString(NULL, xmlString, MXML_NO_CALLBACK))) goto ERROR;
    if (!GetElement(tree, UNICENS, true, &tree, true)) goto ERROR;
    if (!GetElement(tree, SCRIPT, true, &tree, true)) goto ERROR;
    /*Do not use MCalloc for the root element*/
    script = calloc(1, sizeof(UcsXmlScript_t));
    if (!script) goto ERROR;
    priv = script->pInternal = calloc(1, sizeof(PrivateDataScript_t));
    if (!priv) goto ERROR;
    result = ParseScript(tree, &scriptLst, &priv->objList);
    if (Parse_Success != result) goto ERROR;
    script->pScriptList = (Ucs_Ns_Script_t *)node.init_script_list_ptr;
    script->scriptListLength = node.init_script_list_size;
    return script;
ERROR:
    if (Parse_MemoryError == result)
        UcsXml_CB_OnError("XML memory error, aborting..", 0);
    else
        UcsXml_CB_OnError("XML parsing error, aborting..", 0);
    if (tree)
        mxmlDelete(tree);
    if (script)
        FreeValScript(script);
    if (priv)
        free(priv);
    return NULL;
}

#ifdef XML_FILE_SUPPORTED
UcsXmlScript_t *UcsXml_ParseScriptFile(const char *fileName)
{
    UcsXmlScript_t *script;
    char *content = ReadFile(fileName);
    if (NULL == content)
    {
        UcsXml_CB_OnError("UcsXml_ParseScriptFile:Could not read file:'%s'", 1, fileName);
        return NULL;
    }
    script = UcsXml_ParseScript(content);
    free(content);
    return script;
}
#endif

void UcsXml_FreeScript(UcsXmlScript_t *script)
{
    FreeValScript(script);
}

/************************************************************************/
/* Private Function Implementations                                     */
/************************************************************************/

#ifdef XML_FILE_SUPPORTED
static char *ReadFile(const char *fileName)
{
    char *buffer;
    int stringSize, readSize;
    FILE *fh = fopen(fileName, "r");
    if (!fh) return NULL;
    fseek(fh, 0, SEEK_END);
    stringSize = ftell(fh);
    rewind(fh);
    buffer = (char *)malloc(stringSize + 1);
    readSize = fread(buffer, sizeof(char), stringSize, fh);
    buffer[stringSize] = '\0'; /*In any case, terminate it.*/
    if (stringSize != readSize)
    {
        free(buffer);
        buffer = NULL;
    }
    fclose(fh);
    return buffer;
}
#endif

static void FreeVal(UcsXmlVal_t *ucs)
{
    PrivateData_t *priv;
    if (NULL == ucs || NULL == ucs->pInternal)
        return;
    priv = ucs->pInternal;
    FreeObjList(&priv->objList);
    free(ucs->pInternal);
    free(ucs);
}

static void FreeValScript(UcsXmlScript_t *script)
{
    PrivateDataScript_t *priv;
    if (NULL == script || NULL == script->pInternal)
        return;
    priv = script->pInternal;
    FreeObjList(&priv->objList);
    free(script->pInternal);
    free(script);
}

static bool GetElement(mxml_node_t *element, const char *name, bool goDeep, mxml_node_t **out, bool mandatory)
{
    mxml_node_t *n = element;
    if (NULL == n || NULL == name || NULL == out) return false;
    if (goDeep)
    {
        *out = mxmlFindElement(n, n, name, NULL, NULL, MXML_DESCEND);
        return (NULL != *out);
    }
    while ((n = n->next))
    {
        if (MXML_ELEMENT != n->type)
            continue;
        if (0 == strcmp(name, n->value.opaque))
        {
            *out = n;
            return true;
        }
    }
    if (mandatory)
        UcsXml_CB_OnError("Can not find tag <%s>", 1, name);
    return false;
}

static bool GetElementArray(mxml_node_t *element, const char *array[], const char **foundName, mxml_node_t **out, bool skipFirstElement)
{
    mxml_node_t *n = element;
    if (NULL == n || NULL == array || NULL == foundName || NULL == out) return false;
    if (skipFirstElement)
        n = n->next;
    while (n)
    {
        uint32_t i;
        if (MXML_ELEMENT != n->type)
        {
            n = n->next;
            continue;
        }
        for (i = 0; NULL != array[i]; i++)
        {
            if (0 == strcmp(array[i], n->value.opaque))
            {
                *foundName = array[i];
                *out = n;
                return true;
            }
        }
        n = n->next;
    }
    return false;
}

static bool GetCount(mxml_node_t *element, const char *name, uint32_t *out, bool mandatory)
{
    uint32_t cnt = 0;
    mxml_node_t *n;
    if (NULL == element || NULL == name) return false;
    if(!GetElement(element, name, true, &n, false))
        return false;
    while(NULL != n)
    {
        ++cnt;
        if(!GetElement(n, name, false, &n, false))
            break;
    }
    if (mandatory && 0 == cnt)
    {
        UcsXml_CB_OnError("element count of <%s> is zero", 1, name);
        return false;
    }
    *out = cnt;
    return true;
}

static bool GetCountArray(mxml_node_t *element, const char *array[], uint32_t *out, bool mandatory)
{
    const char *tmp;
    uint32_t cnt = 0;
    mxml_node_t *n;
    if (NULL == element || NULL == array) return false;
    n = element;
    while(NULL != n)
    {
        if(!GetElementArray(n, array, &tmp, &n, true))
            break;
        ++cnt;
    }
    if (mandatory && 0 == cnt)
    {
        UcsXml_CB_OnError("element count is zero, searched with string array", 0);
        return false;
    }
    *out = cnt;
    return true;
}

static bool GetString(mxml_node_t *element, const char *key, const char **out, bool mandatory)
{
    int32_t i;
    if (NULL == element || NULL == key) return false;
    for (i = 0; i < element->value.element.num_attrs; i++)
    {
        mxml_attr_t *attr = &element->value.element.attrs[i];
        if (0 == strcmp(key, attr->name))
        {
            *out = attr->value;
            return true;
        }
    }
    if (mandatory)
        UcsXml_CB_OnError("Can not find attribute='%s' from element <%s>",
            2, key, element->value.element.name);
    return false;
}

static bool CheckInteger(const char *value, bool forceHex)
{
    bool hex = forceHex;
    int32_t len;
    if (!value) return false;
    len = strlen(value);
    if (len >= 3 && '0' == value[0] && 'x' == value[1])
    {
        hex = true;
        value += 2;
    }
    while(value[0])
    {
        bool valid = false;
        uint8_t v = value[0];
        if (v >= '0' && v <= '9') valid = true;
        if (hex)
        {
            if (v >= 'a' && v <= 'f') valid = true;
            if (v >= 'A' && v <= 'F') valid = true;
        }
        if (!valid) return false;
        ++value;
    }
    return true;
}

static bool GetUInt16(mxml_node_t *element, const char *key, uint16_t *out, bool mandatory)
{
    long int value;
    const char* txt;
    if (!GetString(element, key, &txt, mandatory)) return false;
    if (!CheckInteger(txt, false))
    {
        UcsXml_CB_OnError("key='%s' contained invalid integer='%s'", 2, key, txt);
        return false;
    }
    value = strtol( txt, NULL, 0 );
    if (value > 0xFFFF)
    {
        UcsXml_CB_OnError("key='%s' is out of range='%d'", 2, key, value);
        return false;
    }
    *out = value;
    return true;
}

static bool GetUInt8(mxml_node_t *element, const char *key, uint8_t *out, bool mandatory)
{
    long int value;
    const char* txt;
    if (!GetString(element, key, &txt, mandatory)) return false;
    if (!CheckInteger(txt, false))
    {
        UcsXml_CB_OnError("key='%s' contained invalid integer='%s'", 2, key, txt);
        return false;
    }
    value = strtol( txt, NULL, 0 );
    if (value > 0xFF)
    {
        UcsXml_CB_OnError("key='%s' is out of range='%d'", 2, key, value);
        return false;
    }
    *out = value;
    return true;
}

static bool GetDataType(const char *txt, MDataType_t *out)
{
    if (NULL == txt || NULL == out) return false;
    if (0 == strcmp(SYNC_CONNECTION, txt)) {
        *out = SYNC_DATA;
    } else if (0 == strcmp(AVP_CONNECTION, txt)) {
        *out = AV_PACKETIZED;
    } else if (0 == strcmp(QOS_CONNECTION, txt)) {
        *out = QOS_IP;
    } else if (0 == strcmp(DFP_CONNECTION, txt)) {
        *out = DISC_FRAME_PHASE;
    } else if (0 == strcmp(IPC_CONNECTION, txt)) {
        *out = IPC_PACKET;
    } else {
        UcsXml_CB_OnError("Unknown data type : '%s'", 1, txt);
        return false;
    }
    return true;
}

static bool GetSocketType(const char *txt, MSocketType_t *out)
{
    if (0 == strcmp(txt, NETWORK_SOCKET)) {
            *out = MSocket_NETWORK;
    } else if (0 == strcmp(txt, USB_SOCKET)) {
        *out = MSocket_USB;
    } else if (0 == strcmp(txt, MLB_SOCKET)) {
        *out = MSocket_MLB;
    } else if (0 == strcmp(txt, STREAM_SOCKET)) {
        *out = MSocket_STRM;
    } else if (0 == strcmp(txt, SPLITTER)) {
        *out = MSocket_SPLITTER;
    } else if (0 == strcmp(txt, COMBINER)) {
        *out = MSocket_COMBINER;
    } else {
        UcsXml_CB_OnError("Unknown port : '%s'", 1, txt);
        return false;
    }
    return true;
}

static bool GetPayload(mxml_node_t *element, const char *name, uint8_t **pPayload, uint8_t *outLen, uint8_t offset, struct UcsXmlObjectList *obj, bool mandatory)
{
    uint32_t tempLen, len = 0;
    uint8_t *p;
    const char *txt;
    char *txtCopy;
    char *tkPtr;
    char *token;
    if (!GetString(element, name, &txt, mandatory))
        return false;
    tempLen = strlen(txt);
    if (0 == tempLen)
    {
        /* Empty payload is OK*/
        *pPayload = NULL;
        *outLen = 0;
        return true;
    }
    tempLen += 1; /* Add space for zero termination */
    txtCopy = malloc(tempLen);
    if (NULL == txtCopy)
        return false;
    strncpy(txtCopy, txt, tempLen);
    tempLen = tempLen / 3; /* 2 chars hex value plus space (AA )  */
    p = MCalloc(obj, offset + tempLen, 1);
    if (NULL == p)
    {
        free(txtCopy);
        return false;
    }
    *pPayload = p;
    token = strtok_r( txtCopy, " ,.-", &tkPtr );
    while( NULL != token )
    {
        if( len >= tempLen )
        {
            UcsXml_CB_OnError("Script payload values must be stuffed to two characters", 0);
            free(txtCopy);
            return false;
        }
        if (!CheckInteger(token, true))
        {
            UcsXml_CB_OnError("Script payload contains non valid hex number='%s'", 1, token);
            free(txtCopy);
            return false;
        }
        p[offset + len++] = strtol( token, NULL, 16 );
        token = strtok_r( NULL, " ,.-", &tkPtr );
    }
    *outLen = len;
    return true;
}

static Ucs_Xrm_ResourceType_t GetResourceType(Ucs_Xrm_ResObject_t *element)
{
    Ucs_Xrm_ResourceType_t typ = *((Ucs_Xrm_ResourceType_t *)element);
    assert(UCS_XRM_RC_TYPE_QOS_CON >= typ);
    return typ;
}

static bool AddJob(struct UcsXmlJobList **joblist, Ucs_Xrm_ResObject_t *job, struct UcsXmlObjectList *objList)
{
    struct UcsXmlJobList *tail;
    if (NULL == joblist || NULL == job)
        return false;
    assert(UCS_XRM_RC_TYPE_QOS_CON >= *((Ucs_Xrm_ResourceType_t *)job));
    if (NULL == joblist[0])
    {
        joblist[0] = MCalloc(objList, 1, sizeof(struct UcsXmlJobList));
        if (NULL == joblist[0]) return false;;
        joblist[0]->job = job;
        return true;
    }
    tail = joblist[0];
    while(tail->next) tail = tail->next;
    tail->next = MCalloc(objList, 1, sizeof(struct UcsXmlJobList));
    if (NULL == tail->next) return false;
    tail->next->job = job;
    return true;
}

static Ucs_Xrm_ResObject_t **GetJobList(struct UcsXmlJobList *joblist, struct UcsXmlObjectList *objList)
{
    Ucs_Xrm_ResObject_t **outJob;
    uint32_t count = 0;
    struct UcsXmlJobList *tail;
    if (NULL == joblist)
        return false;
    /*First: Get amount of stored jobs by enumerate all*/
    tail = joblist;
    while(tail)
    {
        ++count;
        tail = tail->next;
    }
    if (0 == count)
        return false;
    /*Second: Allocate count+1 elements (NULL terminated) and copy pointers*/
    outJob = MCalloc(objList, (count + 1), sizeof(Ucs_Xrm_ResObject_t *));
    if (NULL == outJob) RETURN_ASSERT(NULL, "calloc returned NULL");
    tail = joblist;
    count = 0;
    while(tail)
    {
        outJob[count++] = tail->job;
        tail = tail->next;
    }
    return outJob;
}

static struct UcsXmlJobList *DeepCopyJobList(struct UcsXmlJobList *jobsIn, struct UcsXmlObjectList *objList)
{
    struct UcsXmlJobList *jobsOut, *tail;
    if (NULL == jobsIn || NULL == objList)
        return NULL;
    jobsOut = tail = MCalloc(objList, 1, sizeof(struct UcsXmlJobList));
    if (NULL == jobsOut) RETURN_ASSERT(NULL, "calloc returned NULL");
    while(jobsIn)
    {
        tail->job = jobsIn->job;
        if (jobsIn->next)
        {
            tail->next = MCalloc(objList, 1, sizeof(struct UcsXmlJobList));
            if (NULL == tail->next) RETURN_ASSERT(NULL, "calloc returned NULL");
            tail = tail->next;
        }
        jobsIn = jobsIn->next;
    }
    return jobsOut;
}

static void AddRoute(struct UcsXmlRoute **pRtLst, struct UcsXmlRoute *route)
{
    struct UcsXmlRoute *tail;
    if (NULL == pRtLst || NULL == route)
    {
        assert(false);
        return;
    }
    if (NULL == pRtLst[0])
    {
        pRtLst[0] = route;
        return;
    }
    tail = pRtLst[0];
    while(tail->next) tail = tail->next;
    tail->next = route;
}

static void AddScript(struct UcsXmlScript **pScrLst, struct UcsXmlScript *script)
{
    struct UcsXmlScript *tail;
    if (NULL == pScrLst || NULL == script)
    {
        assert(false);
        return;
    }
    if (NULL == pScrLst[0])
    {
        pScrLst[0] = script;
        return;
    }
    tail = pScrLst[0];
    while(tail->next) tail = tail->next;
    tail->next = script;
}

static ParseResult_t ParseAll(mxml_node_t *tree, UcsXmlVal_t *ucs, PrivateData_t *priv)
{
    uint32_t nodeCount;
    mxml_node_t *sub;
    ParseResult_t result;
    priv->autoRouteId = ROUTE_AUTO_ID_START;
    if (!GetCount(tree, NODE, &nodeCount, true))
        RETURN_ASSERT(Parse_XmlError, "No node defined");

    ucs->pNod = MCalloc(&priv->objList, nodeCount, sizeof(Ucs_Rm_Node_t));
    if (NULL == ucs->pNod) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");

    if (!GetUInt16(tree, PACKET_BW, &ucs->packetBw, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");

    /*Iterate all nodes*/
    if (!GetElement(tree, NODE, true, &sub, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    while(sub)
    {
        const char *conType;
        mxml_node_t *con;
        memset(&priv->nodeData, 0, sizeof(NodeData_t));
        priv->nodeData.nod = &ucs->pNod[ucs->nodSize];
        if (Parse_Success != (result = ParseNode(sub, priv)))
            return result;
        /*/Iterate all connections. Node without any connection is also valid.*/
        if (GetElementArray(sub->child, ALL_CONNECTIONS, &conType, &con, false))
        {
            while(con)
            {
                const char *driverLink;
                const char *socTypeStr;
                MSocketType_t socType;
                mxml_node_t *soc;
                memset(&priv->conData, 0, sizeof(ConnectionData_t));
                if (Parse_Success != (result = ParseConnection(con, conType, priv)))
                    return result;
                /*Iterate all sockets*/
                if(!GetElementArray(con->child, ALL_SOCKETS, &socTypeStr, &soc, false)) RETURN_ASSERT(Parse_XmlError, "No Sockets defined");
                while(soc)
                {
                    if (!GetSocketType(socTypeStr, &socType)) RETURN_ASSERT(Parse_XmlError, "Could not get socket type");
                    if (Parse_Success != (result = ParseSocket(soc, (0 == priv->conData.sockCnt), socType, &priv->conData.jobList, priv)))
                        return result;
                    ++priv->conData.sockCnt;
                    if(!GetElementArray(soc, ALL_SOCKETS, &socTypeStr, &soc, true))
                        break;
                }
                if (GetString(con, L_DRIVER_ATTR, &driverLink, false)) {
                    result = StoreDriverInfo(priv, driverLink);
                    if (Parse_Success != result) return result;
                }
                if(!GetElementArray(con, ALL_CONNECTIONS, &conType, &con, true))
                    break;
            }
        }
        ++ucs->nodSize;
        if (!GetElement(sub, NODE, false, &sub, false))
            break;
    }

    /*Fill route structures*/
    result = ParseRoutes(ucs, priv);
    if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError, "Aborting further parsing, because nodes failed")
    else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError, "Aborting further parsing, because nodes failed");


    /* Parse all scripts*/
    result = ParseAllScripts(tree, priv->pScrLst, &priv->objList, true);
    if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError, "Aborting further parsing, because scripts failed")
    else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError, "Aborting further parsing, because scripts failed");

    /*Iterate all drivers. No driver at all is allowed*/
    if(GetElement(tree, L_DRIVER_TAG, true, &sub, false))
    {
        while(sub)
        {
            result = ParseDriver(sub, ucs, priv);
            if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError, "Aborting further parsing, because drivers failed")
            else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError, "Aborting further parsing, because drivers failed");
            if(!GetElement(sub, L_DRIVER_TAG, false, &sub, false))
                break;
        }
    }

    /*Fill driver informations*/
    ucs->driverSize = GetDrvInfCount(priv->drvInfLst);
    if (0 != ucs->driverSize)
    {
        ucs->ppDriver = MCalloc(&priv->objList, ucs->driverSize, sizeof(DriverInformation_t *));
        FillDriverArray(priv->drvInfLst, ucs->ppDriver);
    }
    return Parse_Success;
}

static ParseResult_t ParseNode(mxml_node_t *node, PrivateData_t *priv)
{
    const char *txt;
    mxml_node_t *port;
    Ucs_Signature_t *signature;
    assert(NULL != node && NULL != priv);
    priv->nodeData.nod->signature_ptr = MCalloc(&priv->objList, 1, sizeof(Ucs_Signature_t));
    signature = priv->nodeData.nod->signature_ptr;
    if(NULL == signature) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    if (!GetUInt16(node, ADDRESS, &signature->node_address, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (GetString(node, SCRIPT, &txt, false))
    {
        struct UcsXmlScript *scr = MCalloc(&priv->objList, 1, sizeof(struct UcsXmlScript));
        if (NULL == scr) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
        scr->node = priv->nodeData.nod;
        strncpy(scr->scriptName, txt, sizeof(scr->scriptName));
        AddScript(&priv->pScrLst, scr);
    }
    /*Iterate all ports*/
    if(GetElementArray(node->child, ALL_PORTS, &txt, &port, false))
    {
        while(port)
        {
            if (0 == (strcmp(txt, MLB_PORT)))
            {
                struct MlbPortParameters p;
                p.list = &priv->objList;
                if (!GetString(port, CLOCK_CONFIG, &p.clockConfig, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetMlbPort(&priv->nodeData.mlbPort, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get MLB Port properties");
            }
            else if (0 == (strcmp(txt, USB_PORT)))
            {
                struct UsbPortParameters p;
                p.list = &priv->objList;
                if (!GetString(port, PHYSICAL_LAYER, &p.physicalLayer, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetString(port, DEVICE_INTERFACES, &p.deviceInterfaces, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetString(port, STRM_IN_COUNT, &p.streamInCount, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetString(port, STRM_OUT_COUNT, &p.streamOutCount, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetUsbPort(&priv->nodeData.usbPort, &p)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
            }
            else if (0 == (strcmp(txt, STRM_PORT)))
            {
                struct StrmPortParameters p;
                p.list = &priv->objList;
                p.index = 0;
                if (!GetString(port, CLOCK_CONFIG, &p.clockConfig, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetString(port, STRM_ALIGN, &p.dataAlignment, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                if (!GetStrmPort(&priv->nodeData.strmPortA, &p)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                p.index = 1;
                if (!GetStrmPort(&priv->nodeData.strmPortB, &p)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
            }
            else
            {
                UcsXml_CB_OnError("Unknown Port:'%s'", 1, txt);
                RETURN_ASSERT(Parse_XmlError, "Internal error");
            }
            if(!GetElementArray(port, ALL_PORTS, &txt, &port, true))
                break;
        }
    }
    return Parse_Success;;
}

static ParseResult_t ParseConnection(mxml_node_t * node, const char *conType, PrivateData_t *priv)
{
    assert(NULL != node && NULL != priv);
    if (NULL == conType) RETURN_ASSERT(Parse_XmlError, "Internal error");
    if (!GetDataType(conType, &priv->conData.dataType)) RETURN_ASSERT(Parse_XmlError, "Can not get data type");
    switch (priv->conData.dataType)
    {
    case SYNC_DATA:
    {
        const char *txt;
        if (GetString(node, MUTE_MODE, &txt, false))
        {
            if (0 == strcmp(txt, MUTE_MODE_NO_MUTING))
                priv->conData.muteMode = UCS_SYNC_MUTE_MODE_NO_MUTING;
            else if (0 == strcmp(txt, MUTE_MODE_MUTE_SIGNAL))
                priv->conData.muteMode = UCS_SYNC_MUTE_MODE_MUTE_SIGNAL;
            else
            {
                UcsXml_CB_OnError("ParseConnection: MuteMode='%s' not implemented", 1, txt);
                RETURN_ASSERT(Parse_XmlError, "Wrong enum");
            }
        }
        else
        {
            /*Be tolerant, this is an optional feature*/
            priv->conData.muteMode = UCS_SYNC_MUTE_MODE_NO_MUTING;
        }
        break;
    }
    case AV_PACKETIZED:
    {
        uint16_t size;
        if (GetUInt16(node, AVP_PACKET_SIZE, &size, false))
        {
            switch(size)
            {
            case 188:
                priv->conData.isocPacketSize = UCS_ISOC_PCKT_SIZE_188;
                break;
            case 196:
                priv->conData.isocPacketSize = UCS_ISOC_PCKT_SIZE_196;
                break;
            case 206:
                priv->conData.isocPacketSize = UCS_ISOC_PCKT_SIZE_206;
                break;
            default:
                UcsXml_CB_OnError("ParseConnection: %s='%d' not implemented", 2, AVP_PACKET_SIZE, size);
                RETURN_ASSERT(Parse_XmlError, "Wrong enum");
            }
        }
        else
        {
            /*Be tolerant, this is an optional feature*/
            priv->conData.isocPacketSize = UCS_ISOC_PCKT_SIZE_188;
        }
        break;
    }
    default:
        UcsXml_CB_OnError("ParseConnection: Datatype='%s' not implemented", 1, conType);
        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        break;
    }
    return Parse_Success;
}

static ParseResult_t ParseSocket(mxml_node_t *soc, bool isSource, MSocketType_t socketType, struct UcsXmlJobList **jobList, PrivateData_t *priv)
{
    Ucs_Xrm_ResObject_t **targetSock;
    assert(NULL != soc && NULL != priv);
    targetSock = isSource ? &priv->conData.inSocket : &priv->conData.outSocket;
    switch(socketType)
    {
    case MSocket_NETWORK:
    {
        const char* txt;
        struct NetworkSocketParameters p;
        /* If there is an combiner stored, add it now into job list (right before Network socket) */
        if (priv->conData.combiner)
            if (!AddJob(jobList, priv->conData.combiner, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");

        p.list = &priv->objList;
        p.isSource = isSource;
        p.dataType = priv->conData.dataType;
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetString(soc, ROUTE, &priv->conData.routeName, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (GetString(soc, ROUTE_IS_ACTIVE, &txt, false))
        {
            if (0 == strcmp(txt, VALUE_TRUE) || 0 == strcmp(txt, VALUE_1))
                priv->conData.isDeactivated = false;
            else if (0 == strcmp(txt, VALUE_FALSE) || 0 == strcmp(txt, VALUE_0))
                priv->conData.isDeactivated = true;
            else RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        } else {
            priv->conData.isDeactivated = false;
        }
        if (!GetUInt16(soc, ROUTE_ID, &priv->conData.routeId, false))
            priv->conData.routeId = ROUTE_INVALID_ID;
        if (priv->conData.syncOffsetNeeded)
        {
            if (!GetUInt16(soc, OFFSET, &priv->conData.syncOffset, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        }
        if (!GetNetworkSocket((Ucs_Xrm_NetworkSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get Network Socket");
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        break;
    }
    case MSocket_USB:
    {
        struct UsbSocketParameters p;
        p.list = &priv->objList;
        p.isSource = isSource;
        p.dataType = priv->conData.dataType;
        if (priv->nodeData.usbPort)
        {
            p.usbPort = priv->nodeData.usbPort;
        } else {
            if (!GetUsbPortDefaultCreated(&p.usbPort, &priv->objList))
                RETURN_ASSERT(Parse_XmlError, "Can not get default created USB port");
            priv->nodeData.usbPort = (Ucs_Xrm_UsbPort_t *)p.usbPort;
        }
        if(!AddJob(jobList, p.usbPort, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        if (!GetString(soc, ENDPOINT_ADDRESS, &p.endpointAddress, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetString(soc, FRAMES_PER_TRANSACTION, &p.framesPerTrans, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetUsbSocket((Ucs_Xrm_UsbSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get USB socket");
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        break;
    }
    case MSocket_MLB:
    {
        struct MlbSocketParameters p;
        p.list = &priv->objList;
        p.isSource = isSource;
        p.dataType = priv->conData.dataType;
        if (priv->nodeData.mlbPort)
        {
            p.mlbPort = priv->nodeData.mlbPort;
        } else {
            if (!GetMlbPortDefaultCreated(&p.mlbPort, &priv->objList))
                RETURN_ASSERT(Parse_XmlError, "Can not get default created MLB port");
            priv->nodeData.mlbPort = (Ucs_Xrm_MlbPort_t *)p.mlbPort;
        }
        if (!AddJob(jobList, p.mlbPort, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetString(soc, CHANNEL_ADDRESS, &p.channelAddress, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetMlbSocket((Ucs_Xrm_MlbSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        break;
    }
    case MSocket_STRM:
    {
        struct StrmSocketParameters p;
        p.list = &priv->objList;
        p.isSource = isSource;
        p.dataType = priv->conData.dataType;
        p.streamPortA = priv->nodeData.strmPortA;
        p.streamPortB = priv->nodeData.strmPortB;
        if (NULL == p.streamPortA || NULL == p.streamPortB)
        {
            UcsXml_CB_OnError("Node 0x%X uses StreamSocket without creating a StreamPort", 1, priv->nodeData.nod->signature_ptr->node_address);
            RETURN_ASSERT(Parse_XmlError, "No StreamPort");
        }
        if (!AddJob(jobList, p.streamPortA, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        if (!AddJob(jobList, p.streamPortB, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetString(soc, STRM_PIN, &p.streamPin, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetStrmSocket((Ucs_Xrm_StrmSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get stream socket");
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        break;
    }
    case MSocket_SPLITTER:
    {
        mxml_node_t *networkSoc;
        struct SplitterParameters p;
        if (isSource)
        {
            UcsXml_CB_OnError("Splitter can not be used as input socket", 0);
            RETURN_ASSERT(Parse_XmlError, "Wrong usage of Splitter");
        }
        p.list = &priv->objList;
        if (!GetUInt16(soc, BYTES_PER_FRAME, &p.bytesPerFrame, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        /* Current input socket will be stored inside splitter
         * and splitter will become the new input socket */
        if (!(p.inSoc = priv->conData.inSocket)) RETURN_ASSERT(Parse_XmlError, "Wrong usage of Splitter");
        if (!GetSplitter((Ucs_Xrm_Splitter_t **)&priv->conData.inSocket, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get Splitter");
        if (!AddJob(jobList, priv->conData.inSocket, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
        if (!GetElement(soc->child, NETWORK_SOCKET, false, &networkSoc, true))
            RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        priv->conData.syncOffsetNeeded = true;

        while(networkSoc)
        {
            struct UcsXmlJobList *jobListCopy = DeepCopyJobList(*jobList, &priv->objList);
            if (!ParseSocket(networkSoc, false, MSocket_NETWORK, &jobListCopy, priv)) RETURN_ASSERT(Parse_XmlError, "Failed to parse Network Socket");
            if (!GetElement(networkSoc, NETWORK_SOCKET, false, &networkSoc, false))
                return Parse_Success; /* Do not break here, otherwise an additional invalid route will be created */
        }
        break;
    }
    case MSocket_COMBINER:
    {
        struct CombinerParameters p;
        if (!isSource)
        {
            UcsXml_CB_OnError("Combiner can not be used as output socket", 0);
            RETURN_ASSERT(Parse_XmlError, "Wrong usage of Combiner");
        }
        p.list = &priv->objList;
        if (!GetUInt16(soc, BYTES_PER_FRAME, &p.bytesPerFrame, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
        if (!GetCombiner(&priv->conData.combiner, &p)) RETURN_ASSERT(Parse_XmlError, "Can not get combiner");
        priv->conData.syncOffsetNeeded = true;
        if (!GetElement(soc->child, NETWORK_SOCKET, false, &priv->conData.pendingCombinerSockets, true))
            RETURN_ASSERT(Parse_XmlError, "No Network Socket inside Combiner");
        break;
    }
    default:
        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
    }
    /*Handle Pending Combiner Tasks*/
    if (NULL != priv->conData.outSocket && NULL != priv->conData.combiner &&
        NULL != priv->conData.pendingCombinerSockets)
    {
        mxml_node_t *tmp = priv->conData.pendingCombinerSockets;
        priv->conData.pendingCombinerSockets = NULL;
        /* Current output socket will be stored inside combiner
         * and combiner will become the new output socket */
        priv->conData.combiner->port_socket_obj_ptr = priv->conData.outSocket;
        priv->conData.outSocket = priv->conData.combiner;
        while(tmp)
        {
            struct UcsXmlJobList *jobListCopy = DeepCopyJobList(*jobList, &priv->objList);
            if (!ParseSocket(tmp, true, MSocket_NETWORK, &jobListCopy, priv)) RETURN_ASSERT(Parse_XmlError, "Failed to parse Network Socket in Combiner");
            if (!GetElement(tmp, NETWORK_SOCKET, false, &tmp, false))
                return Parse_Success; /* Do not break here, otherwise an additional invalid route will be created */
        }
    }
    /*Connect in and out socket once they are created*/
    if (priv->conData.inSocket && priv->conData.outSocket)
    {
        bool networkIsInput;
        bool networkIsOutput;
        Ucs_Rm_EndPoint_t *ep;
        struct UcsXmlRoute *route;
        switch(priv->conData.dataType)
        {
        case SYNC_DATA:
        {
            Ucs_Xrm_SyncCon_t *con = MCalloc(&priv->objList, 1, sizeof(Ucs_Xrm_SyncCon_t));
            if (NULL == con) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
            if (!AddJob(jobList, con, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
            con->resource_type = UCS_XRM_RC_TYPE_SYNC_CON;
            con->socket_in_obj_ptr = priv->conData.inSocket;
            con->socket_out_obj_ptr = priv->conData.outSocket;
            con->mute_mode = priv->conData.muteMode;
            con->offset = priv->conData.syncOffset;
            break;
        }
        case AV_PACKETIZED:
        {
            Ucs_Xrm_AvpCon_t *con = MCalloc(&priv->objList, 1, sizeof(Ucs_Xrm_AvpCon_t));
            if (NULL == con) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
            if (!AddJob(jobList, con, &priv->objList)) RETURN_ASSERT(Parse_XmlError, "Failed to add job");
            con->resource_type = UCS_XRM_RC_TYPE_AVP_CON;
            con->socket_in_obj_ptr = priv->conData.inSocket;
            con->socket_out_obj_ptr = priv->conData.outSocket;
            con->isoc_packet_size = priv->conData.isocPacketSize;
            break;
        }
        default:
            UcsXml_CB_OnError("Could not connect sockets, data type not implemented: %d", 1, priv->conData.dataType);
            RETURN_ASSERT(Parse_XmlError, "Wrong enum");
            break;
        }
        ep = MCalloc(&priv->objList, 1, sizeof(Ucs_Rm_EndPoint_t));
        if (NULL == ep) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");

        networkIsInput = (UCS_XRM_RC_TYPE_NW_SOCKET == *((Ucs_Xrm_ResourceType_t *)priv->conData.inSocket));
        networkIsOutput = (UCS_XRM_RC_TYPE_NW_SOCKET == *((Ucs_Xrm_ResourceType_t *)priv->conData.outSocket));
        if (!networkIsInput && !networkIsOutput)
        {
            UcsXml_CB_OnError("At least one Network socket required per connection", 0);
            RETURN_ASSERT(Parse_XmlError, "Wrong usage of connection");
        }
        ep->endpoint_type = networkIsOutput ? UCS_RM_EP_SOURCE : UCS_RM_EP_SINK;
        ep->jobs_list_ptr = GetJobList(*jobList, &priv->objList);
        if(NULL == ep->jobs_list_ptr) RETURN_ASSERT(Parse_MemoryError, "Got empty job list");
        ep->node_obj_ptr = priv->nodeData.nod;
        route = MCalloc(&priv->objList, 1, sizeof(struct UcsXmlRoute));
        if (NULL == route) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
        route->isSource = networkIsOutput;
        route->isActive = !priv->conData.isDeactivated;
        route->routeId = priv->conData.routeId;
        route->ep = ep;
        assert(NULL != priv->conData.routeName);
        strncpy(route->routeName, priv->conData.routeName, sizeof(route->routeName));
        AddRoute(&priv->pRtLst, route);
    }
    return Parse_Success;
}

static ParseResult_t ParseAllScripts(mxml_node_t *tree, struct UcsXmlScript *scrlist, struct UcsXmlObjectList *objList, bool checkReference)
{
    mxml_node_t *sub;
    ParseResult_t result = Parse_Success; /* It's okay to have no scripts */
    /*Iterate all scripts. No scripts at all is allowed*/
    if(GetElement(tree, SCRIPT, true, &sub, false))
    {
        bool found = true;
        while(sub)
        {
            result = ParseScript(sub, scrlist, objList);
            if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError, "Aborting further parsing, because scripts failed")
            else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError, "Aborting further parsing, because scripts failed");
            if(!GetElement(sub, SCRIPT, false, &sub, false))
                break;
        }
        /* Check if all scripts where referenced */
        while(checkReference && NULL != scrlist)
        {
            if (!scrlist->inUse)
            {
                UcsXml_CB_OnError("Script not defined:'%s', used by node=0x%X", 1, scrlist->scriptName, scrlist->node->signature_ptr->node_address);
                found = false;
            }
            scrlist = scrlist->next;
        }
        if (!found)
            RETURN_ASSERT(Parse_XmlError, "Script not found");
    }
    return result;
}

static ParseResult_t ParseScript(mxml_node_t *scr, struct UcsXmlScript *scrlist, struct UcsXmlObjectList *objList)
{
    bool found = false;
    mxml_node_t *act;
    uint32_t actCnt;
    const char *txt;
    Ucs_Ns_Script_t *script;
    ScriptData_t scriptData = { 0 };
    assert(NULL != scr && NULL != scrlist && NULL != objList);
    if (!GetCountArray(scr->child, ALL_SCRIPTS_NO_PAUSE, &actCnt, true)) RETURN_ASSERT(Parse_XmlError, "Script without any action");
    if (NULL == (script = MCalloc(objList, actCnt, sizeof(Ucs_Ns_Script_t))))
        RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    actCnt = 0;
    /*Iterate all actions*/
    if (!GetElementArray(scr->child, ALL_SCRIPTS, &txt, &act, false)) RETURN_ASSERT(Parse_XmlError, "Script without any action");
    while(act)
    {
        if (0 == strcmp(txt, SCRIPT_MSG_SEND)) {
            ParseResult_t result = ParseScriptMsgSend(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PORT_CREATE)) {
            ParseResult_t result = ParseScriptGpioPortCreate(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PORT_PIN_MODE)) {
            ParseResult_t result = ParseScriptGpioPinMode(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PIN_STATE)) {
            ParseResult_t result = ParseScriptGpioPinState(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_CREATE)) {
            ParseResult_t result = ParseScriptPortCreate(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_WRITE)) {
            ParseResult_t result = ParseScriptPortWrite(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_READ)) {
            ParseResult_t result = ParseScriptPortRead(act, &script[actCnt], objList, &scriptData);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_PAUSE)) {
            ParseResult_t result = ParseScriptPause(act, &scriptData);
            if (Parse_Success != result) return result;
        } else {
            UcsXml_CB_OnError("Unknown script action:'%s'", 1, txt);
            RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        }
        if (!GetElementArray(act, ALL_SCRIPTS, &txt, &act, true))
            break;
    }
    if (!GetString(scr, NAME, &txt, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    while(NULL != scrlist)
    {
        if (scrlist->singleShot || 0 == strcmp(txt, scrlist->scriptName))
        {
            Ucs_Rm_Node_t *node = scrlist->node;
            if (NULL == node)
                return Parse_MemoryError;
            node->init_script_list_ptr = script;
            node->init_script_list_size = actCnt;
            scrlist->inUse = true;
            found = true;
        }
        scrlist = scrlist->next;
    }
    if(!found)
    {
        UcsXml_CB_OnError("Script defined:'%s', which was never referenced", 1, txt);
        RETURN_ASSERT(Parse_XmlError, "Unused script");
    }
    return Parse_Success;
}

static bool FillScriptInitialValues(Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    assert(NULL != scr && NULL != objList && NULL != scriptData);
    scr->send_cmd = MCalloc(objList, 1, sizeof(Ucs_Ns_ConfigMsg_t));
    scr->exp_result = MCalloc(objList, 1, sizeof(Ucs_Ns_ConfigMsg_t));
    if (NULL == scr->send_cmd || NULL == scr->exp_result) RETURN_ASSERT(false, "calloc returned NULL");
    scr->pause = scriptData->pause;
    scriptData->pause = 0;
    return true;
}

static ParseResult_t ParseScriptMsgSend(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!FillScriptInitialValues(scr, objList, scriptData)) return Parse_MemoryError;
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    if (!GetUInt8(act, FBLOCK_ID, &req->fblock_id, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");

    if (!GetUInt16(act, FUNCTION_ID, &req->funct_id, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");

    if (!GetUInt8(act, OP_TYPE_REQUEST, &req->op_type, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");

    res->fblock_id = req->fblock_id;
    res->funct_id = req->funct_id;

    if (!GetUInt8(act, OP_TYPE_RESPONSE, &res->op_type, false))
        res->op_type = 0xFF;

    if (!GetPayload(act, PAYLOAD_REQ_HEX, (uint8_t **)&req->data_ptr, &req->data_size, 0, objList, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    
    res->data_size = 0x0; /* Using Wildcard */ 
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    uint16_t debounce;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    if (!GetUInt16(act, DEBOUNCE_TIME, &debounce, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x701;
    req->op_type = 0x2;
    res->op_type = 0xC;
    req->data_size = 3;
    req->data_ptr = MCalloc(objList, req->data_size, 1);
    if (NULL == req->data_ptr) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    ((uint8_t *)req->data_ptr)[0] = 0; /*GPIO Port instance, always 0*/
    ((uint8_t *)req->data_ptr)[1] = MISC_HB(debounce);
    ((uint8_t *)req->data_ptr)[2] = MISC_LB(debounce);
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPinMode(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
#define PORT_HANDLE_OFFSET (2)
    uint8_t *payload;
    uint8_t payloadLen = 0;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x703;
    req->op_type = 0x2;
    res->op_type = 0xC;
    if (!GetPayload(act, PIN_CONFIG, &payload, &payloadLen,
        PORT_HANDLE_OFFSET, /* First two bytes are reserved for port handle */
        objList, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    payload[0] = 0x1D;
    payload[1] = 0x00;
    req->data_ptr = payload;
    req->data_size = payloadLen + PORT_HANDLE_OFFSET;
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPinState(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    uint16_t mask, data;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    if (!GetUInt16(act, PIN_MASK, &mask, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (!GetUInt16(act, PIN_DATA, &data, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x704;
    req->op_type = 0x2;
    res->op_type = 0xC;
    req->data_size = 6;
    req->data_ptr = MCalloc(objList, req->data_size, 1);
    if (NULL == req->data_ptr) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    ((uint8_t *)req->data_ptr)[0] = 0x1D;
    ((uint8_t *)req->data_ptr)[1] = 0x00;
    ((uint8_t *)req->data_ptr)[2] = MISC_HB(mask);
    ((uint8_t *)req->data_ptr)[3] = MISC_LB(mask);
    ((uint8_t *)req->data_ptr)[4] = MISC_HB(data);
    ((uint8_t *)req->data_ptr)[5] = MISC_LB(data);
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    const char *txt;
    uint8_t speed;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    if (!GetString(act, I2C_SPEED, &txt, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (0 == strcmp(txt, I2C_SPEED_SLOW))
        speed = 0;
    else if (0 == strcmp(txt, I2C_SPEED_FAST))
        speed = 1;
    else
    {
        UcsXml_CB_OnError("Invalid I2C speed:'%s'", 1, txt);
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    }
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x6C1;
    req->op_type = 0x2;
    res->op_type = 0xC;
    req->data_size = 4;
    req->data_ptr = MCalloc(objList, req->data_size, 1);
    if (NULL == req->data_ptr) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    ((uint8_t *)req->data_ptr)[0] = 0x00; /* I2C Port Instance always 0 */
    ((uint8_t *)req->data_ptr)[1] = 0x00; /* I2C slave address, always 0, because we are Master */
    ((uint8_t *)req->data_ptr)[2] = 0x01; /* We are Master */
    ((uint8_t *)req->data_ptr)[3] = speed;
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptPortWrite(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
#define HEADER_OFFSET 8
    const char *txt;
    uint8_t mode, blockCount, address, length, payloadLength;
    uint16_t timeout;
    uint8_t *payload;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (GetString(act, I2C_WRITE_MODE, &txt, false))
    {
        if (0 == strcmp(txt, I2C_WRITE_MODE_DEFAULT))
            mode = 0;
        else if (0 == strcmp(txt, I2C_WRITE_MODE_REPEAT))
            mode = 1;
        else if (0 == strcmp(txt, I2C_WRITE_MODE_BURST))
            mode = 2;
        else
        {
            UcsXml_CB_OnError("Invalid I2C mode:'%s'", 1, txt);
            RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        }
    } else {
        mode = 0;
    }
    if (!GetUInt8(act, I2C_WRITE_BLOCK_COUNT, &blockCount, false))
        blockCount = 0;
    if (!GetUInt8(act, I2C_SLAVE_ADDRESS, &address, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (!GetUInt8(act, I2C_PAYLOAD_LENGTH, &length, false))
        length = 0;
    if (!GetUInt16(act, I2C_TIMEOUT, &timeout, false))
        timeout = 100;
    if (!GetPayload(act, I2C_PAYLOAD, &payload, &payloadLength, HEADER_OFFSET, objList, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (0 == length)
        length = payloadLength;
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x6C4;
    req->op_type = 0x2;
    res->op_type = 0xC;
    req->data_size = payloadLength + HEADER_OFFSET;
    req->data_ptr = payload;

    ((uint8_t *)req->data_ptr)[0] = 0x0F;
    ((uint8_t *)req->data_ptr)[1] = 0x00;
    ((uint8_t *)req->data_ptr)[2] = mode;
    ((uint8_t *)req->data_ptr)[3] = blockCount;
    ((uint8_t *)req->data_ptr)[4] = address;
    ((uint8_t *)req->data_ptr)[5] = length;
    ((uint8_t *)req->data_ptr)[6] = MISC_HB(timeout);
    ((uint8_t *)req->data_ptr)[7] = MISC_LB(timeout);
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptPortRead(mxml_node_t *act, Ucs_Ns_Script_t *scr, struct UcsXmlObjectList *objList, ScriptData_t *scriptData)
{
    uint8_t address, length;
    uint16_t timeout;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != objList && NULL != scriptData);
    if (!GetUInt8(act, I2C_SLAVE_ADDRESS, &address, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (!GetUInt8(act, I2C_PAYLOAD_LENGTH, &length, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    if (!GetUInt16(act, I2C_TIMEOUT, &timeout, false))
        timeout = 100;
    if (!FillScriptInitialValues(scr, objList, scriptData))
        RETURN_ASSERT(Parse_MemoryError, "Script initialization failed");
    req = (Ucs_Ns_ConfigMsg_t *)scr->send_cmd;
    res = (Ucs_Ns_ConfigMsg_t *)scr->exp_result;
    req->inst_id = res->inst_id = 1;
    req->funct_id = res->funct_id = 0x6C3;
    req->op_type = 0x2;
    res->op_type = 0xC;
    req->data_size = 6;
    req->data_ptr = MCalloc(objList, req->data_size, 1);
    if (NULL == req->data_ptr) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");

    ((uint8_t *)req->data_ptr)[0] = 0x0F;
    ((uint8_t *)req->data_ptr)[1] = 0x00;
    ((uint8_t *)req->data_ptr)[2] = address;
    ((uint8_t *)req->data_ptr)[3] = length;
    ((uint8_t *)req->data_ptr)[4] = MISC_HB(timeout);
    ((uint8_t *)req->data_ptr)[5] = MISC_LB(timeout);
    res->data_size = 0x0; /* Using Wildcard */
    return Parse_Success;
}

static ParseResult_t ParseScriptPause(mxml_node_t *act, ScriptData_t *scriptData)
{
    assert(NULL != act && NULL != scriptData);
    if (!GetUInt16(act, PAUSE_MS, &scriptData->pause, true))
        RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
    return Parse_Success;
}

static ParseResult_t ParseRoutes(UcsXmlVal_t *ucs, PrivateData_t *priv)
{
    uint16_t routeAmount = 0;
    struct UcsXmlRoute *sourceRoute;
    assert(NULL != ucs && NULL != priv);
    /*First: Count the amount of routes and allocate the correct amount*/
    sourceRoute = priv->pRtLst;
    while (NULL != sourceRoute)
    {
        if (sourceRoute->isSource)
        {
            struct UcsXmlRoute *sinkRoute = priv->pRtLst;
            while (NULL != sinkRoute)
            {
                if (sourceRoute != sinkRoute
                    && !sinkRoute->isSource
                    && (0 == strncmp(sourceRoute->routeName, sinkRoute->routeName, sizeof(sourceRoute->routeName))))
                {
                    routeAmount++;
                }
                sinkRoute = sinkRoute->next;
            }
        }
        sourceRoute = sourceRoute->next;
    }
    if (0 == routeAmount)
        return Parse_Success; /*Its okay to have no routes at all (e.g. MEP traffic only)*/
    ucs->pRoutes = MCalloc(&priv->objList, routeAmount, sizeof(Ucs_Rm_Route_t));
    if (NULL == ucs->pRoutes) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");

    /*Second: Fill allocated structure now*/
    sourceRoute = priv->pRtLst;
    while (NULL != sourceRoute)
    {
        if (sourceRoute->isSource)
        {
            struct UcsXmlRoute *sinkRoute = priv->pRtLst;
            while (NULL != sinkRoute)
            {
                if (sourceRoute != sinkRoute
                    && !sinkRoute->isSource
                    && (0 == strncmp(sourceRoute->routeName, sinkRoute->routeName, sizeof(sourceRoute->routeName))))
                {
                    Ucs_Rm_Route_t *route;
                    if(ucs->routesSize >= routeAmount)
                    {
                        RETURN_ASSERT(Parse_MemoryError, "Internal routing error");
                    }
                    route = &ucs->pRoutes[ucs->routesSize++];
                    route->source_endpoint_ptr = sourceRoute->ep;
                    route->sink_endpoint_ptr = sinkRoute->ep;
                    route->active = sinkRoute->isActive && sourceRoute->isActive;
                    if (ROUTE_INVALID_ID != sinkRoute->routeId)
                        route->route_id = sinkRoute->routeId;
                    else if (ROUTE_INVALID_ID != sourceRoute->routeId)
                        route->route_id = sourceRoute->routeId;
                    else
                        route->route_id = priv->autoRouteId++;
                }
                sinkRoute = sinkRoute->next;
            }
        }
        sourceRoute = sourceRoute->next;
    }

#ifdef DEBUG
    /* Third perform checks when running in debug mode*/
    {
        Ucs_Xrm_ResourceType_t *job;
        uint16_t i, j;
        for (i = 0; i < routeAmount; i++)
        {
            Ucs_Rm_Route_t *route = &ucs->pRoutes[i];
            assert(NULL != route->source_endpoint_ptr);
            assert(NULL != route->sink_endpoint_ptr);
            assert(NULL != route->source_endpoint_ptr->jobs_list_ptr);
            assert(UCS_RM_EP_SOURCE == route->source_endpoint_ptr->endpoint_type);
            assert(UCS_RM_EP_SINK == route->sink_endpoint_ptr->endpoint_type);
            assert(NULL != route->source_endpoint_ptr->node_obj_ptr);
            assert(NULL != route->sink_endpoint_ptr->node_obj_ptr);
            assert(NULL != route->source_endpoint_ptr->node_obj_ptr->signature_ptr);
            assert(NULL != route->sink_endpoint_ptr->node_obj_ptr->signature_ptr);
            j = 0;
            while((job = ((Ucs_Xrm_ResourceType_t *)route->source_endpoint_ptr->jobs_list_ptr[j])))
            {
                assert(UCS_XRM_RC_TYPE_QOS_CON >= *job);
                ++j;
            }
            j = 0;
            while((job = ((Ucs_Xrm_ResourceType_t *)route->sink_endpoint_ptr->jobs_list_ptr[j])))
            {
                assert(UCS_XRM_RC_TYPE_QOS_CON >= *job);
                ++j;
            }
        }
    }
#endif
    return Parse_Success;
}

static bool AddDriverInfo(struct UcsXmlDriverInfoList **drvList, DriverInformation_t *drvInfo, struct UcsXmlObjectList *objList)
{
    struct UcsXmlDriverInfoList *tail;
    if (NULL == drvList || NULL == drvInfo)
        return false;
    if (NULL == drvList[0])
    {
        drvList[0] = MCalloc(objList, 1, sizeof(struct UcsXmlDriverInfoList));
        if (NULL == drvList[0]) return false;;
        drvList[0]->driverInfo = drvInfo;
        return true;
    }
    tail = drvList[0];
    while(tail->next) tail = tail->next;
    tail->next = MCalloc(objList, 1, sizeof(struct UcsXmlDriverInfoList));
    if (NULL == tail->next) return false;
    tail->next->driverInfo = drvInfo;
    return true;
}

static ParseResult_t ParseDriver(mxml_node_t *drv, UcsXmlVal_t *ucs, PrivateData_t *priv)
{
    bool found = false;
    mxml_node_t *driver;
    DriverInformation_t *drvInf = NULL;
    const char *driverLink;
    const char *driverType;
    struct UcsXmlDriverInfoList *head = priv->drvInfLst;
    if (!head) return Parse_XmlError;
    if (!GetString(drv, L_DRIVER_NAME, &driverLink, true))  return Parse_XmlError;
    /*Iterate all drivers in driver list*/
    do
    {
        drvInf = head->driverInfo;
        if (NULL == drvInf) return Parse_MemoryError;
        /*Iterate all drivers in XML*/
        if(0 == strcmp(driverLink, drvInf->linkName) &&
            GetElementArray(drv->child, ALL_DRIVERS, &driverType, &driver, false))
        {
            bool firstDriverEntry = true;
            while(driver)
            {
                found = true;
                if (firstDriverEntry)
                {
                    firstDriverEntry = false;
                }
                else
                {
                    /* Duplicate whole UcsXmlDriverInfoList structure and link it direct after the original */
                    struct UcsXmlDriverInfoList *copyList = MCalloc(&priv->objList, 1, sizeof(struct UcsXmlDriverInfoList));
                    DriverInformation_t *copyInfo =  MCalloc(&priv->objList, 1, sizeof(DriverInformation_t));
                    if (NULL == copyList || NULL == copyInfo) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
                    memcpy(copyList, head, sizeof(struct UcsXmlDriverInfoList));
                    memcpy(copyInfo, head->driverInfo, sizeof(DriverInformation_t));
                    copyList->driverInfo = copyInfo;
                    head->next = copyList;
                    head = copyList;
                }
                if(0 == strcmp(driverType, L_DRIVER_CDEV))
                {
                    drvInf->driverType = Driver_LinuxCdev;
                    if (!GetString(driver, L_DRIVER_NAME, &drvInf->drv.LinuxCdev.aimName, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFERSIZE, &drvInf->drv.LinuxCdev.bufferSize, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFFERCOUNT, &drvInf->drv.LinuxCdev.numBuffers, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                } 
                else if(0 == strcmp(driverType, L_DRIVER_V4L2))
                {
                    drvInf->driverType = Driver_LinuxV4l2;
                    if (!GetString(driver, L_DRIVER_NAME, &drvInf->drv.LinuxV4l2.aimName, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFERSIZE, &drvInf->drv.LinuxV4l2.bufferSize, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFFERCOUNT, &drvInf->drv.LinuxV4l2.numBuffers, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                }
                else if(0 == strcmp(driverType, L_DRIVER_ALSA))
                {
                    const char *resolution;
                    drvInf->driverType = Driver_LinuxAlsa;
                    if (!GetString(driver, L_DRIVER_NAME, &drvInf->drv.LinuxAlsa.aimName, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFERSIZE, &drvInf->drv.LinuxAlsa.bufferSize, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt16(driver, L_BUFFFERCOUNT, &drvInf->drv.LinuxAlsa.numBuffers, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetUInt8(driver, L_ALSA_CH_COUNT, &drvInf->drv.LinuxAlsa.amountOfChannels, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (!GetString(driver, L_ALSA_CH_RES, &resolution, true)) RETURN_ASSERT(Parse_XmlError, "Missing mandatory attribute");
                    if (0 == strcmp(L_DRIVER_ALSARES_8BIT, resolution)) {
                        drvInf->drv.LinuxAlsa.resolutionInBit = 8;
                    } else if (0 == strcmp(L_DRIVER_ALSARES_16BIT, resolution)) {
                        drvInf->drv.LinuxAlsa.resolutionInBit = 16;
                    } else if (0 == strcmp(L_DRIVER_ALSARES_24BIT, resolution)) {
                        drvInf->drv.LinuxAlsa.resolutionInBit = 24;
                    } else if (0 == strcmp(L_DRIVER_ALSARES_32BIT, resolution)) {
                        drvInf->drv.LinuxAlsa.resolutionInBit = 32;
                    } else {
                        UcsXml_CB_OnError("Invalid ALSA resolution '%s'", 1, resolution);
                        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
                    }
                }
                else
                {
                    UcsXml_CB_OnError("Invalid Driver Type", 0);
                    RETURN_ASSERT(Parse_XmlError, "Wrong enum");
                }
                if(!GetElementArray(driver, ALL_DRIVERS, &driverType, &driver, true))
                    break;
            }
        }
        head = head->next;
    }
    while(NULL != head);
    if (!found)
    {
        UcsXml_CB_OnError("Did not find driver info for '%s'", 1, driverLink);
        RETURN_ASSERT(Parse_XmlError, "No driver info");
    }
    return Parse_Success;
}

static ParseResult_t StoreDriverInfo(PrivateData_t *priv, const char *driverLink)
{
#define MAX_CHANNEL_NAME_LENGTH 6
    char *channelName;
    ConnectionData_t *con = &priv->conData;
    DriverCfgDataType_t cfgDataType;
    DriverCfgDirection_t cfgDirection;
    uint16_t subBufferSize = 0;
    uint16_t packetsPerXact;

    DriverInformation_t *drvInf = MCalloc(&priv->objList, 1, sizeof(DriverInformation_t));
    if (NULL == drvInf) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");

    channelName = MCalloc(&priv->objList, MAX_CHANNEL_NAME_LENGTH, sizeof(char));
    if (NULL == channelName) RETURN_ASSERT(Parse_MemoryError, "calloc returned NULL");
    
    drvInf->linkName = driverLink;
    drvInf->phy = DriverPhyUnknown;
    AddDriverInfo(&priv->drvInfLst, drvInf, &priv->objList);

    switch(con->dataType)
    {
    case SYNC_DATA:         cfgDataType=DriverCfgDataType_Sync; break;
    case CONTROL_DATA:      cfgDataType=DriverCfgDataType_Control; break;
    case AV_PACKETIZED:     cfgDataType=DriverCfgDataType_Isoc; break;
    case QOS_IP:
    case DISC_FRAME_PHASE:
    case IPC_PACKET:
    case INVALID:
    default:
        UcsXml_CB_OnError("StoreDriverInfo: Unsupported Data Type=0x%X", 1, con->dataType);
        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
    }
    if (NULL == con->inSocket)
    {
        UcsXml_CB_OnError("Driver Configuration:In Socket was not set on node=0x%X, route=%s", 
                2, priv->nodeData.nod->signature_ptr->node_address, driverLink);
        RETURN_ASSERT(Parse_XmlError, "Not enough info to fill driver structure");
    }
    switch(GetResourceType(con->inSocket))
    {
    case UCS_XRM_RC_TYPE_NW_SOCKET:
    {
        Ucs_Xrm_NetworkSocket_t *networkSock = (Ucs_Xrm_NetworkSocket_t *)con->inSocket;
        subBufferSize = networkSock->bandwidth;
        cfgDirection = DriverCfgDirection_Rx;
        break;
    }
    case UCS_XRM_RC_TYPE_MLB_SOCKET:
    {
        Ucs_Xrm_MlbSocket_t *mlbSock = (Ucs_Xrm_MlbSocket_t *)con->inSocket;
        cfgDirection = DriverCfgDirection_Tx;
        subBufferSize = mlbSock->bandwidth;
        snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ca%d", mlbSock->channel_address);
        drvInf->phy = DriverPhyMlb;
        break;
    }
    case UCS_XRM_RC_TYPE_USB_SOCKET:
    {
        Ucs_Xrm_UsbSocket_t *usbSock = (Ucs_Xrm_UsbSocket_t *)con->inSocket;
        cfgDirection = DriverCfgDirection_Tx;
        packetsPerXact = usbSock->frames_per_transfer;
        snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ep%02X", usbSock->end_point_addr);
        drvInf->phy = DriverPhyUsb;
        break;
    }
    case UCS_XRM_RC_TYPE_STRM_SOCKET:
        cfgDirection = DriverCfgDirection_Tx;
        assert(false); /* TODO: Implement */
        break;
    case UCS_XRM_RC_TYPE_SPLITTER:
    {
        Ucs_Xrm_Splitter_t *splitter = (Ucs_Xrm_Splitter_t *)con->inSocket;
        subBufferSize = splitter->bytes_per_frame;
        cfgDirection = DriverCfgDirection_Tx;
        if (UCS_XRM_RC_TYPE_USB_SOCKET == GetResourceType(splitter->socket_in_obj_ptr))
        {
            Ucs_Xrm_UsbSocket_t *usbSock = (Ucs_Xrm_UsbSocket_t *)splitter->socket_in_obj_ptr;
            packetsPerXact = usbSock->frames_per_transfer;
            snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ep%02X", usbSock->end_point_addr);
            drvInf->phy = DriverPhyUsb;
        }
        else if (UCS_XRM_RC_TYPE_MLB_SOCKET == GetResourceType(splitter->socket_in_obj_ptr))
        {
            Ucs_Xrm_MlbSocket_t *mlbSock = (Ucs_Xrm_MlbSocket_t *)splitter->socket_in_obj_ptr;
            snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ca%d", mlbSock->channel_address);
            drvInf->phy = DriverPhyMlb;
        }
        break;
    }
    case UCS_XRM_RC_TYPE_COMBINER: /* Combiner not supported as input */
    default:
        UcsXml_CB_OnError("StoreDriverInfo: Unsupported Resource as inSocket=0x%X", 1, GetResourceType(con->inSocket));
        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        break;
    }
    if (DriverCfgDataType_Isoc == cfgDataType)
    {
        switch(con->isocPacketSize)
        {
            case UCS_ISOC_PCKT_SIZE_188: subBufferSize = 188; break;
            case UCS_ISOC_PCKT_SIZE_196: subBufferSize = 196; break;
            case UCS_ISOC_PCKT_SIZE_206: subBufferSize = 206; break;
            default:
                UcsXml_CB_OnError("StoreDriverInfo: Non supported isoc packet size=%d", 1, con->isocPacketSize);
                RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        }
    }
    if (NULL == con->outSocket)
    {
        UcsXml_CB_OnError("Driver Configuration:Out Socket was not set on node=0x%X, route=%s", 
                2, priv->nodeData.nod->signature_ptr->node_address, driverLink);
        RETURN_ASSERT(Parse_XmlError, "Not enough info to fill driver structure");
    }
    switch(GetResourceType(con->outSocket))
    {
    case UCS_XRM_RC_TYPE_NW_SOCKET:
    {
        if (0 == subBufferSize)
        {
            Ucs_Xrm_NetworkSocket_t *networkSock = (Ucs_Xrm_NetworkSocket_t *)con->outSocket;
            subBufferSize = networkSock->bandwidth;
        }
        break;
    }
    case UCS_XRM_RC_TYPE_MLB_SOCKET:
    {
        Ucs_Xrm_MlbSocket_t *mlbSock = (Ucs_Xrm_MlbSocket_t *)con->outSocket;
        subBufferSize = mlbSock->bandwidth;
        snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ca%d", mlbSock->channel_address);
        drvInf->phy = DriverPhyMlb;
        break;
    }
    case UCS_XRM_RC_TYPE_USB_SOCKET:
    {
        Ucs_Xrm_UsbSocket_t *usbSock = (Ucs_Xrm_UsbSocket_t *)con->outSocket;
        packetsPerXact = usbSock->frames_per_transfer;
        snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ep%02X", usbSock->end_point_addr);
        drvInf->phy = DriverPhyUsb;
        break;
    }
    case UCS_XRM_RC_TYPE_STRM_SOCKET:
        break;
    case UCS_XRM_RC_TYPE_COMBINER:
    {
        Ucs_Xrm_Combiner_t *combiner = (Ucs_Xrm_Combiner_t *)con->outSocket;
        subBufferSize = combiner->bytes_per_frame;
        if (UCS_XRM_RC_TYPE_USB_SOCKET == GetResourceType(combiner->port_socket_obj_ptr))
        {
            Ucs_Xrm_UsbSocket_t *usbSock = (Ucs_Xrm_UsbSocket_t *)combiner->port_socket_obj_ptr;
            packetsPerXact = usbSock->frames_per_transfer;
            snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ep%02X", usbSock->end_point_addr);
            drvInf->phy = DriverPhyUsb;
        }
        else if (UCS_XRM_RC_TYPE_MLB_SOCKET == GetResourceType(combiner->port_socket_obj_ptr))
        {
            Ucs_Xrm_MlbSocket_t *mlbSock = (Ucs_Xrm_MlbSocket_t *)combiner->port_socket_obj_ptr;
            snprintf(channelName, MAX_CHANNEL_NAME_LENGTH, "ca%d", mlbSock->channel_address);
            drvInf->phy = DriverPhyMlb;
        }
        break;
    }
    case UCS_XRM_RC_TYPE_SPLITTER: /* Splitter not supported as output */
    default:
        UcsXml_CB_OnError("StoreDriverInfo: Unsupported Resource as outSocket=0x%X", 1, GetResourceType(con->outSocket));
        RETURN_ASSERT(Parse_XmlError, "Wrong enum");
        break;
    }
    switch(drvInf->driverType)
    {
    case Driver_LinuxCdev:
        drvInf->drv.LinuxCdev.channelName = channelName;
        drvInf->drv.LinuxCdev.dataType = cfgDataType;
        drvInf->drv.LinuxCdev.direction = cfgDirection;
        drvInf->drv.LinuxCdev.subBufferSize = subBufferSize;
        drvInf->drv.LinuxCdev.packetsPerXact = packetsPerXact;
        break;
    case Driver_LinuxAlsa:
        drvInf->drv.LinuxAlsa.channelName = channelName;
        drvInf->drv.LinuxAlsa.dataType = cfgDataType;
        drvInf->drv.LinuxAlsa.direction = cfgDirection;
        drvInf->drv.LinuxAlsa.subBufferSize = subBufferSize;
        drvInf->drv.LinuxAlsa.packetsPerXact = packetsPerXact;
        break;
    case Driver_LinuxV4l2:
        drvInf->drv.LinuxV4l2.channelName = channelName;
        drvInf->drv.LinuxV4l2.dataType = cfgDataType;
        drvInf->drv.LinuxV4l2.direction = cfgDirection;
        drvInf->drv.LinuxV4l2.subBufferSize = subBufferSize;
        drvInf->drv.LinuxV4l2.packetsPerXact = packetsPerXact;
        break;
    default:
        break;
    }
    drvInf->nodeAddress = priv->nodeData.nod->signature_ptr->node_address;
    return Parse_Success;
}

static uint16_t GetDrvInfCount(struct UcsXmlDriverInfoList *drvInfLst)
{
    uint16_t cnt = 0;
    struct UcsXmlDriverInfoList *head = drvInfLst;
    if (NULL == drvInfLst)
        return 0;
    do
    {
        ++cnt;
        head = head->next;
    }
    while(NULL != head);
    return cnt;
}

static void FillDriverArray(struct UcsXmlDriverInfoList *drvInfLst, DriverInformation_t **ppDriverInfo)
{
    uint16_t i = 0;
    struct UcsXmlDriverInfoList *head = drvInfLst;
    do
    {
        ppDriverInfo[i++] = head->driverInfo;
        head = head->next;
    }
    while(NULL != head);
}
