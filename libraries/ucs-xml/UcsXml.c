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
#include "mxml.h"
#include "UcsXml_Private.h"
#include "UcsXml.h"

/************************************************************************/
/* PRIVATE DECLARATIONS                                                 */
/************************************************************************/

#define COMPILETIME_CHECK(cond)  (void)sizeof(int[2 * !!(cond) - 1])
#define RETURN_ASSERT(result) { UcsXml_CB_OnError("Assertion in file=%s, line=%d", 2, __FILE__, __LINE__); return result; }
#define MISC_HB(value)      ((uint8_t)((uint16_t)(value) >> 8))
#define MISC_LB(value)      ((uint8_t)((uint16_t)(value) & (uint16_t)0xFF))

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
    bool inUse;
    char scriptName[32];
    Ucs_Rm_Node_t *node;
    struct UcsXmlScript *next;
};

struct UcsXmlJobList
{
    Ucs_Xrm_ResObject_t *job;
    struct UcsXmlJobList *next;
};

typedef enum
{
    MSocket_MOST = 20,
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
    mxml_node_t *pendingCombinerMostSockets;
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
    ScriptData_t scriptData;
} PrivateData_t;

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

#define MOST_SOCKET                         "MOSTSocket"
#define USB_SOCKET                          "USBSocket"
#define MLB_SOCKET                          "MediaLBSocket"
#define STREAM_SOCKET                       "StreamSocket"
#define SPLITTER                            "Splitter"
#define COMBINER                            "Combiner"
static const char* ALL_SOCKETS[] = { MOST_SOCKET, USB_SOCKET, MLB_SOCKET,
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
static const char* PAYLOAD_RES_HEX =        "PayloadResponse";
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

static const char* VALUE_TRUE =             "true";
static const char* VALUE_FALSE =            "false";
static const char* VALUE_1 =                "1";
static const char* VALUE_0 =                "0";

/************************************************************************/
/* Private Function Prototypes                                          */
/************************************************************************/

static void FreeVal(UcsXmlVal_t *ucs);
static bool GetElement(mxml_node_t *element, const char *name, bool goDeep, mxml_node_t **out, bool mandatory);
static bool GetElementArray(mxml_node_t *element, const char *array[], const char **foundName, mxml_node_t **out);
static bool GetCount(mxml_node_t *element, const char *name, uint32_t *out, bool mandatory);
static bool GetCountArray(mxml_node_t *element, const char *array[], uint32_t *out, bool mandatory);
static bool GetString(mxml_node_t *element, const char *key, const char **out, bool mandatory);
static bool CheckInteger(const char *val, bool forceHex);
static bool GetUInt16(mxml_node_t *element, const char *key, uint16_t *out, bool mandatory);
static bool GetUInt8(mxml_node_t *element, const char *key, uint8_t *out, bool mandatory);
static bool GetSocketType(const char *txt, MSocketType_t *out);
static bool GetPayload(mxml_node_t *element, const char *name, uint8_t **pPayload, uint8_t *len, uint8_t offset,
            struct UcsXmlObjectList *obj, bool mandatory);
static bool AddJob(struct UcsXmlJobList **joblist, Ucs_Xrm_ResObject_t *job, struct UcsXmlObjectList *objList);
static Ucs_Xrm_ResObject_t **GetJobList(struct UcsXmlJobList *joblist, struct UcsXmlObjectList *objList);
static struct UcsXmlJobList *DeepCopyJobList(struct UcsXmlJobList *jobsIn, struct UcsXmlObjectList *objList);
static void AddRoute(struct UcsXmlRoute **pRtLst, struct UcsXmlRoute *route);
static void AddScript(struct UcsXmlScript **pScrLst, struct UcsXmlScript *script);
static ParseResult_t ParseAll(mxml_node_t *tree, UcsXmlVal_t *ucs, PrivateData_t *priv);
static ParseResult_t ParseNode(mxml_node_t * node, PrivateData_t *priv);
static ParseResult_t ParseConnection(mxml_node_t * node, const char *conType, PrivateData_t *priv);
static ParseResult_t ParseSocket(mxml_node_t *soc, bool isSource, MSocketType_t socketType, struct UcsXmlJobList **jobList, PrivateData_t *priv);
static ParseResult_t ParseScript(mxml_node_t *scr, PrivateData_t *priv);
static bool FillScriptInitialValues(Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptMsgSend(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptGpioPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptGpioPinMode(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptGpioPinState(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptPortWrite(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptPortRead(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseScriptPause(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv);
static ParseResult_t ParseRoutes(UcsXmlVal_t *ucs, PrivateData_t *priv);

/************************************************************************/
/* Public Functions                                                     */
/************************************************************************/

UcsXmlVal_t *UcsXml_Parse(const char *xmlString)
{
    UcsXmlVal_t *val = NULL;
    ParseResult_t result = Parse_MemoryError;
    mxml_node_t *tree;
    if (!(tree = mxmlLoadString(NULL, xmlString, MXML_NO_CALLBACK))) goto ERROR;
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
    assert(false);
    if (!tree)
        mxmlDelete(tree);
    if (val)
        FreeVal(val);
    return NULL;
}

void UcsXml_FreeVal(UcsXmlVal_t *val)
{
    FreeVal(val);
}

/************************************************************************/
/* Private Function Implementations                                     */
/************************************************************************/

void FreeVal(UcsXmlVal_t *ucs)
{
    PrivateData_t *priv;
    if (NULL == ucs || NULL == ucs->pInternal)
        return;
    priv = ucs->pInternal;
    FreeObjList(&priv->objList);
    free(ucs->pInternal);
    free(ucs);
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

static bool GetElementArray(mxml_node_t *element, const char *array[], const char **foundName, mxml_node_t **out)
{
    mxml_node_t *n = element;
    if (NULL == n || NULL == array || NULL == foundName || NULL == out) return false;
    while ((n = n->next))
    {
        uint32_t i;
        if (MXML_ELEMENT != n->type)
            continue;
        for (i = 0; NULL != array[i]; i++)
        {
            if (0 == strcmp(array[i], n->value.opaque))
            {
                *foundName = array[i];
                *out = n;
                return true;
            }
        }
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
        if(!GetElementArray(n, array, &tmp, &n))
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
    if (0 == strcmp(txt, MOST_SOCKET)) {
            *out = MSocket_MOST;
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
    tempLen = strlen(txt) + 1;
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
            assert(false);
            return 0;
        }
        if (!CheckInteger(token, true))
        {
            UcsXml_CB_OnError("Script payload contains non valid hex number='%s'", 1, token);
            free(txtCopy);
            assert(false);
            return 0;
        }
        p[offset + len++] = strtol( token, NULL, 16 );
        token = strtok_r( NULL, " ,.-", &tkPtr );
    }
    *outLen = len;
    return true;
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
    if (NULL == outJob)
        return false;
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
    if (NULL == jobsOut) { assert(false); return NULL; }
    while(jobsIn)
    {
        tail->job = jobsIn->job;
        if (jobsIn->next)
        {
            tail->next = MCalloc(objList, 1, sizeof(struct UcsXmlJobList));
            if (NULL == tail->next) { assert(false); return NULL; }
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
    priv->autoRouteId = 0x8000;
    if (!GetCount(tree, NODE, &nodeCount, true))
        RETURN_ASSERT(Parse_XmlError);

    ucs->pNod = MCalloc(&priv->objList, nodeCount, sizeof(Ucs_Rm_Node_t));
    if (NULL == ucs->pNod) RETURN_ASSERT(Parse_MemoryError);

    if (!GetUInt16(tree, PACKET_BW, &ucs->packetBw, true))
        RETURN_ASSERT(Parse_XmlError);

    /*Iterate all nodes*/
    if (!GetElement(tree, NODE, true, &sub, true))
        RETURN_ASSERT(Parse_XmlError);
    while(sub)
    {
        const char *conType;
        mxml_node_t *con;
        memset(&priv->nodeData, 0, sizeof(NodeData_t));
        priv->nodeData.nod = &ucs->pNod[ucs->nodSize];
        if (Parse_Success != (result = ParseNode(sub, priv)))
            return result;
        /*/Iterate all connections. Node without any connection is also valid.*/
        if (GetElementArray(sub->child, ALL_CONNECTIONS, &conType, &con))
        {
            while(con)
            {
                const char *socTypeStr;
                MSocketType_t socType;
                mxml_node_t *soc;
                memset(&priv->conData, 0, sizeof(ConnectionData_t));
                if (Parse_Success != (result = ParseConnection(con, conType, priv)))
                    return result;
                /*Iterate all sockets*/
                if(!GetElementArray(con->child, ALL_SOCKETS, &socTypeStr, &soc)) RETURN_ASSERT(Parse_XmlError);
                while(soc)
                {
                    if (!GetSocketType(socTypeStr, &socType)) RETURN_ASSERT(Parse_XmlError);
                    if (Parse_Success != (result = ParseSocket(soc, (0 == priv->conData.sockCnt), socType, &priv->conData.jobList, priv)))
                        return result;
                    ++priv->conData.sockCnt;
                    if(!GetElementArray(soc, ALL_SOCKETS, &socTypeStr, &soc))
                        break;
                }
                if(!GetElementArray(con, ALL_CONNECTIONS, &conType, &con))
                    break;
            }
        }
        ++ucs->nodSize;
        if (!GetElement(sub, NODE, false, &sub, false))
            break;
    }

    /*Fill route structures*/
    result = ParseRoutes(ucs, priv);
    if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError)
    else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError);

    /*Iterate all scripts. No scripts at all is allowed*/
    if(GetElement(tree, SCRIPT, true, &sub, false))
    {
        bool found = true;
        struct UcsXmlScript *scrlist = priv->pScrLst;
        while(sub)
        {
            result = ParseScript(sub, priv);
            if (Parse_MemoryError == result) RETURN_ASSERT(Parse_MemoryError)
            else if (Parse_XmlError == result) RETURN_ASSERT(Parse_XmlError);
            if(!GetElement(sub, SCRIPT, false, &sub, false))
                break;
        }
        /* Check if all scripts where referenced */
        while(NULL != scrlist)
        {
            if (!scrlist->inUse)
            {
                UcsXml_CB_OnError("Script not defined:'%s', used by node=0x%X", 1, scrlist->scriptName, scrlist->node->signature_ptr->node_address);
                found = false;
            }
            scrlist = scrlist->next;
        }
        if (!found)
            RETURN_ASSERT(Parse_XmlError);
    }
    return result;
}

static ParseResult_t ParseNode(mxml_node_t *node, PrivateData_t *priv)
{
    const char *txt;
    mxml_node_t *port;
    Ucs_Signature_t *signature;
    assert(NULL != node && NULL != priv);
    priv->nodeData.nod->signature_ptr = MCalloc(&priv->objList, 1, sizeof(Ucs_Signature_t));
    signature = priv->nodeData.nod->signature_ptr;
    if(NULL == signature) RETURN_ASSERT(Parse_MemoryError);
    if (!GetUInt16(node, ADDRESS, &signature->node_address, true))
        RETURN_ASSERT(Parse_XmlError);
    if (GetString(node, SCRIPT, &txt, false))
    {
        struct UcsXmlScript *scr = MCalloc(&priv->objList, 1, sizeof(struct UcsXmlScript));
        if (NULL == scr) RETURN_ASSERT(Parse_MemoryError);
        scr->node = priv->nodeData.nod;
        strncpy(scr->scriptName, txt, sizeof(scr->scriptName));
        AddScript(&priv->pScrLst, scr);
    }
    /*Iterate all ports*/
    if(GetElementArray(node->child, ALL_PORTS, &txt, &port))
    {
        while(port)
        {
            if (0 == (strcmp(txt, MLB_PORT)))
            {
                struct MlbPortParameters p;
                p.list = &priv->objList;
                if (!GetString(port, CLOCK_CONFIG, &p.clockConfig, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetMlbPort(&priv->nodeData.mlbPort, &p)) RETURN_ASSERT(Parse_XmlError);
            }
            else if (0 == (strcmp(txt, USB_PORT)))
            {
                struct UsbPortParameters p;
                p.list = &priv->objList;
                if (!GetString(port, PHYSICAL_LAYER, &p.physicalLayer, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetString(port, DEVICE_INTERFACES, &p.deviceInterfaces, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetString(port, STRM_IN_COUNT, &p.streamInCount, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetString(port, STRM_OUT_COUNT, &p.streamOutCount, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetUsbPort(&priv->nodeData.usbPort, &p)) RETURN_ASSERT(Parse_XmlError);
            }
            else if (0 == (strcmp(txt, STRM_PORT)))
            {
                struct StrmPortParameters p;
                p.list = &priv->objList;
                p.index = 0;
                if (!GetString(port, CLOCK_CONFIG, &p.clockConfig, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetString(port, STRM_ALIGN, &p.dataAlignment, true)) RETURN_ASSERT(Parse_XmlError);
                if (!GetStrmPort(&priv->nodeData.strmPortA, &p)) RETURN_ASSERT(Parse_XmlError);
                p.index = 1;
                if (!GetStrmPort(&priv->nodeData.strmPortB, &p)) RETURN_ASSERT(Parse_XmlError);
            }
            else
            {
                UcsXml_CB_OnError("Unknown Port:'%s'", 1, txt);
                RETURN_ASSERT(Parse_XmlError);
            }
            if(!GetElementArray(port, ALL_PORTS, &txt, &port))
                break;
        }
    }
    return Parse_Success;;
}

static ParseResult_t ParseConnection(mxml_node_t * node, const char *conType, PrivateData_t *priv)
{
    assert(NULL != node && NULL != priv);
    if (NULL == conType) RETURN_ASSERT(Parse_XmlError);
    if (!GetDataType(conType, &priv->conData.dataType)) RETURN_ASSERT(Parse_XmlError);
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
                RETURN_ASSERT(Parse_XmlError);
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
                RETURN_ASSERT(Parse_XmlError);
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
        RETURN_ASSERT(Parse_XmlError);
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
    case MSocket_MOST:
    {
        const char* txt;
        struct MostSocketParameters p;
        /* If there is an combiner stored, add it now into job list (right before MOST socket) */
        if (priv->conData.combiner)
            if (!AddJob(jobList, priv->conData.combiner, &priv->objList)) RETURN_ASSERT(Parse_XmlError);

        p.list = &priv->objList;
        p.isSource = isSource;
        p.dataType = priv->conData.dataType;
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetString(soc, ROUTE, &priv->conData.routeName, true)) RETURN_ASSERT(Parse_XmlError);
        if (GetString(soc, ROUTE_IS_ACTIVE, &txt, false))
        {
            if (0 == strcmp(txt, VALUE_TRUE) || 0 == strcmp(txt, VALUE_1))
                priv->conData.isDeactivated = false;
            else if (0 == strcmp(txt, VALUE_FALSE) || 0 == strcmp(txt, VALUE_0))
                priv->conData.isDeactivated = true;
            else RETURN_ASSERT(Parse_XmlError);
        } else {
            priv->conData.isDeactivated = false;
        }
        if (!GetUInt16(soc, ROUTE_ID, &priv->conData.routeId, false))
            priv->conData.routeId = ++priv->autoRouteId;
        if (priv->conData.syncOffsetNeeded)
        {
            if (!GetUInt16(soc, OFFSET, &priv->conData.syncOffset, true)) RETURN_ASSERT(Parse_XmlError);
        }
        if (!GetMostSocket((Ucs_Xrm_MostSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
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
                RETURN_ASSERT(Parse_XmlError);
            priv->nodeData.usbPort = (Ucs_Xrm_UsbPort_t *)p.usbPort;
        }
        if(!AddJob(jobList, p.usbPort, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        if (!GetString(soc, ENDPOINT_ADDRESS, &p.endpointAddress, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetString(soc, FRAMES_PER_TRANSACTION, &p.framesPerTrans, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetUsbSocket((Ucs_Xrm_UsbSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
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
                RETURN_ASSERT(Parse_XmlError);
            priv->nodeData.mlbPort = (Ucs_Xrm_MlbPort_t *)p.mlbPort;
        }
        if (!AddJob(jobList, p.mlbPort, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetString(soc, CHANNEL_ADDRESS, &p.channelAddress, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetMlbSocket((Ucs_Xrm_MlbSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
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
        if (!AddJob(jobList, p.streamPortA, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, p.streamPortB, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        if (!GetUInt16(soc, BANDWIDTH, &p.bandwidth, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetString(soc, STRM_PIN, &p.streamPin, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetStrmSocket((Ucs_Xrm_StrmSocket_t **)targetSock, &p)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, *targetSock, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        break;
    }
    case MSocket_SPLITTER:
    {
        mxml_node_t *mostSoc;
        struct SplitterParameters p;
        if (isSource)
        {
            UcsXml_CB_OnError("Splitter can not be used as input socket", 0);
            RETURN_ASSERT(Parse_XmlError);
        }
        p.list = &priv->objList;
        if (!GetUInt16(soc, BYTES_PER_FRAME, &p.bytesPerFrame, true)) RETURN_ASSERT(Parse_XmlError);
        /* Current input socket will be stored inside splitter
         * and splitter will become the new input socket */
        if (!(p.inSoc = priv->conData.inSocket)) RETURN_ASSERT(Parse_XmlError);
        if (!GetSplitter((Ucs_Xrm_Splitter_t **)&priv->conData.inSocket, &p)) RETURN_ASSERT(Parse_XmlError);
        if (!AddJob(jobList, priv->conData.inSocket, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
        if (!GetElement(soc->child, MOST_SOCKET, false, &mostSoc, true))
            RETURN_ASSERT(Parse_XmlError);
        priv->conData.syncOffsetNeeded = true;

        while(mostSoc)
        {
            struct UcsXmlJobList *jobListCopy = DeepCopyJobList(*jobList, &priv->objList);
            if (!ParseSocket(mostSoc, false, MSocket_MOST, &jobListCopy, priv)) RETURN_ASSERT(Parse_XmlError);
            if (!GetElement(mostSoc, MOST_SOCKET, false, &mostSoc, false))
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
            RETURN_ASSERT(Parse_XmlError);
        }
        p.list = &priv->objList;
        if (!GetUInt16(soc, BYTES_PER_FRAME, &p.bytesPerFrame, true)) RETURN_ASSERT(Parse_XmlError);
        if (!GetCombiner(&priv->conData.combiner, &p)) RETURN_ASSERT(Parse_XmlError);
        priv->conData.syncOffsetNeeded = true;
        if (!GetElement(soc->child, MOST_SOCKET, false, &priv->conData.pendingCombinerMostSockets, true))
            RETURN_ASSERT(Parse_XmlError);
        break;
    }
    default:
        RETURN_ASSERT(Parse_XmlError);
    }
    /*Handle Pending Combiner Tasks*/
    if (NULL != priv->conData.outSocket && NULL != priv->conData.combiner &&
        NULL != priv->conData.pendingCombinerMostSockets)
    {
        mxml_node_t *tmp = priv->conData.pendingCombinerMostSockets;
        priv->conData.pendingCombinerMostSockets = NULL;
        /* Current output socket will be stored inside combiner
         * and combiner will become the new output socket */
        priv->conData.combiner->port_socket_obj_ptr = priv->conData.outSocket;
        priv->conData.outSocket = priv->conData.combiner;
        while(tmp)
        {
            struct UcsXmlJobList *jobListCopy = DeepCopyJobList(*jobList, &priv->objList);
            if (!ParseSocket(tmp, true, MSocket_MOST, &jobListCopy, priv)) RETURN_ASSERT(Parse_XmlError);
            if (!GetElement(tmp, MOST_SOCKET, false, &tmp, false))
                return Parse_Success; /* Do not break here, otherwise an additional invalid route will be created */
        }
    }
    /*Connect in and out socket once they are created*/
    if (priv->conData.inSocket && priv->conData.outSocket)
    {
        bool mostIsInput;
        bool mostIsOutput;
        Ucs_Rm_EndPoint_t *ep;
        struct UcsXmlRoute *route;
        switch(priv->conData.dataType)
        {
        case SYNC_DATA:
        {
            Ucs_Xrm_SyncCon_t *con = MCalloc(&priv->objList, 1, sizeof(Ucs_Xrm_SyncCon_t));
            if (NULL == con) RETURN_ASSERT(Parse_MemoryError);
            if (!AddJob(jobList, con, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
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
            if (NULL == con) RETURN_ASSERT(Parse_MemoryError);
            if (!AddJob(jobList, con, &priv->objList)) RETURN_ASSERT(Parse_XmlError);
            con->resource_type = UCS_XRM_RC_TYPE_AVP_CON;
            con->socket_in_obj_ptr = priv->conData.inSocket;
            con->socket_out_obj_ptr = priv->conData.outSocket;
            con->isoc_packet_size = priv->conData.isocPacketSize;
            break;
        }
        default:
            UcsXml_CB_OnError("Could not connect sockets, data type not implemented: %d", 1, priv->conData.dataType);
            RETURN_ASSERT(Parse_XmlError);
            break;
        }
        ep = MCalloc(&priv->objList, 1, sizeof(Ucs_Rm_EndPoint_t));
        if (NULL == ep) RETURN_ASSERT(Parse_MemoryError);

        mostIsInput = (UCS_XRM_RC_TYPE_MOST_SOCKET == *((Ucs_Xrm_ResourceType_t *)priv->conData.inSocket));
        mostIsOutput = (UCS_XRM_RC_TYPE_MOST_SOCKET == *((Ucs_Xrm_ResourceType_t *)priv->conData.outSocket));
        if (!mostIsInput && !mostIsOutput)
        {
            UcsXml_CB_OnError("At least one MOST socket required per connection", 0);
            RETURN_ASSERT(Parse_XmlError);
        }
        ep->endpoint_type = mostIsOutput ? UCS_RM_EP_SOURCE : UCS_RM_EP_SINK;
        ep->jobs_list_ptr = GetJobList(*jobList, &priv->objList);
        if(NULL == ep->jobs_list_ptr) RETURN_ASSERT(Parse_MemoryError);
        ep->node_obj_ptr = priv->nodeData.nod;
        route = MCalloc(&priv->objList, 1, sizeof(struct UcsXmlRoute));
        if (NULL == route) RETURN_ASSERT(Parse_MemoryError);
        route->isSource = mostIsOutput;
        route->isActive = !priv->conData.isDeactivated;
        route->routeId = priv->conData.routeId;
        route->ep = ep;
        assert(NULL != priv->conData.routeName);
        strncpy(route->routeName, priv->conData.routeName, sizeof(route->routeName));
        AddRoute(&priv->pRtLst, route);
    }
    return Parse_Success;
}

static ParseResult_t ParseScript(mxml_node_t *scr, PrivateData_t *priv)
{
    bool found = false;
    mxml_node_t *act;
    uint32_t actCnt;
    uint32_t i = 0;
    const char *txt;
    struct UcsXmlScript *scrlist;
    Ucs_Ns_Script_t *script;
    assert(NULL != scr && NULL != priv);
    priv->scriptData.pause = 0;
    scrlist = priv->pScrLst;
    if (!GetCountArray(scr->child, ALL_SCRIPTS, &actCnt, false)) RETURN_ASSERT(Parse_XmlError);
    if (NULL == (script = MCalloc(&priv->objList, actCnt, sizeof(Ucs_Ns_Script_t))))
        RETURN_ASSERT(Parse_MemoryError);
    actCnt = 0;
    /*Iterate all actions*/
    if (!GetElementArray(scr->child, ALL_SCRIPTS, &txt, &act)) RETURN_ASSERT(Parse_XmlError);
    while(act)
    {
        if (0 == strcmp(txt, SCRIPT_MSG_SEND)) {
            ParseResult_t result = ParseScriptMsgSend(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PORT_CREATE)) {
            ParseResult_t result = ParseScriptGpioPortCreate(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PORT_PIN_MODE)) {
            ParseResult_t result = ParseScriptGpioPinMode(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_GPIO_PIN_STATE)) {
            ParseResult_t result = ParseScriptGpioPinState(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_CREATE)) {
            ParseResult_t result = ParseScriptPortCreate(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_WRITE)) {
            ParseResult_t result = ParseScriptPortWrite(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_I2C_PORT_READ)) {
            ParseResult_t result = ParseScriptPortRead(act, &script[i], priv);
            if (Parse_Success != result) return result;
            ++actCnt;
        } else if (0 == strcmp(txt, SCRIPT_PAUSE)) {
            ParseResult_t result = ParseScriptPause(act, &script[i], priv);
            if (Parse_Success != result) return result;
        } else {
            UcsXml_CB_OnError("Unknown script action:'%s'", 1, txt);
            RETURN_ASSERT(Parse_XmlError);
        }
        if (!GetElementArray(act, ALL_SCRIPTS, &txt, &act))
            break;
        ++i;
    }
    if (!GetString(scr, NAME, &txt, true))
        RETURN_ASSERT(Parse_XmlError);
    while(NULL != scrlist)
    {
        if (0 == strcmp(txt, scrlist->scriptName))
        {
            Ucs_Rm_Node_t *node = scrlist->node;
            node->script_list_ptr = script;
            node->script_list_size = actCnt;
            scrlist->inUse = true;
            found = true;
        }
        scrlist = scrlist->next;
    }
    if(!found)
    {
        UcsXml_CB_OnError("Script defined:'%s', which was never referenced", 1, txt);
        RETURN_ASSERT(Parse_XmlError);
    }
    return Parse_Success;
}

static bool FillScriptInitialValues(Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    assert(NULL != scr && NULL != priv);
    scr->send_cmd = MCalloc(&priv->objList, 1, sizeof(Ucs_Ns_ConfigMsg_t));
    scr->exp_result = MCalloc(&priv->objList, 1, sizeof(Ucs_Ns_ConfigMsg_t));
    assert(scr->send_cmd && scr->exp_result);
    if (NULL == scr->send_cmd || NULL == scr->exp_result) return false;
    scr->pause = priv->scriptData.pause;
    priv->scriptData.pause = 0;
    return true;
}

static ParseResult_t ParseScriptMsgSend(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!FillScriptInitialValues(scr, priv)) return Parse_MemoryError;
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    if (!GetUInt8(act, FBLOCK_ID, &req->FBlockId, true))
        RETURN_ASSERT(Parse_XmlError);

    if (!GetUInt16(act, FUNCTION_ID, &req->FunktId, true))
        RETURN_ASSERT(Parse_XmlError);

    if (!GetUInt8(act, OP_TYPE_REQUEST, &req->OpCode, true))
        RETURN_ASSERT(Parse_XmlError);

    res->FBlockId = req->FBlockId;
    res->FunktId = req->FunktId;

    if (GetUInt8(act, OP_TYPE_RESPONSE, &res->OpCode, false))
        GetPayload(act, PAYLOAD_RES_HEX, &res->DataPtr, &res->DataLen, 0, &priv->objList, false);

    if (!GetPayload(act, PAYLOAD_REQ_HEX, &req->DataPtr, &req->DataLen, 0, &priv->objList, true))
        RETURN_ASSERT(Parse_XmlError);
    if (0 == req->DataLen || NULL == req->DataPtr)
        RETURN_ASSERT(Parse_XmlError);
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    uint16_t debounce;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    if (!GetUInt16(act, DEBOUNCE_TIME, &debounce, true))
        RETURN_ASSERT(Parse_XmlError);
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x701;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    req->DataLen = 3;
    res->DataLen = 2;
    req->DataPtr = MCalloc(&priv->objList, req->DataLen, 1);
    if (NULL == req->DataPtr) return Parse_MemoryError;
    res->DataPtr = MCalloc(&priv->objList, res->DataLen, 1);
    if (NULL == res->DataPtr) return Parse_MemoryError;
    req->DataPtr[0] = 0; /*GPIO Port instance, always 0*/
    req->DataPtr[1] = MISC_HB(debounce);
    req->DataPtr[2] = MISC_LB(debounce);

    res->DataPtr[0] = 0x1D;
    res->DataPtr[1] = 0x00;
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPinMode(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
#define PORT_HANDLE_OFFSET (2)
    uint8_t *payload;
    uint8_t payloadLen = 0;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x703;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    if (!GetPayload(act, PIN_CONFIG, &payload, &payloadLen,
        PORT_HANDLE_OFFSET, /* First two bytes are reserved for port handle */
        &priv->objList, true)) RETURN_ASSERT(Parse_XmlError);
    payload[0] = 0x1D;
    payload[1] = 0x00;
    req->DataPtr = payload;
    res->DataPtr = payload;
    req->DataLen = payloadLen + PORT_HANDLE_OFFSET;
    res->DataLen = payloadLen + PORT_HANDLE_OFFSET;
    return Parse_Success;
}

static ParseResult_t ParseScriptGpioPinState(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    uint16_t mask, data;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    if (!GetUInt16(act, PIN_MASK, &mask, true))
        RETURN_ASSERT(Parse_XmlError);
    if (!GetUInt16(act, PIN_DATA, &data, true))
        RETURN_ASSERT(Parse_XmlError);
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x704;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    req->DataLen = 6;
    res->DataLen = 8;
    req->DataPtr = MCalloc(&priv->objList, req->DataLen, 1);
    if (NULL == req->DataPtr) return Parse_MemoryError;
    res->DataPtr = MCalloc(&priv->objList, res->DataLen, 1);
    if (NULL == res->DataPtr) return Parse_MemoryError;
    req->DataPtr[0] = 0x1D;
    req->DataPtr[1] = 0x00;
    req->DataPtr[2] = MISC_HB(mask);
    req->DataPtr[3] = MISC_LB(mask);
    req->DataPtr[4] = MISC_HB(data);
    req->DataPtr[5] = MISC_LB(data);
    memcpy(res->DataPtr, req->DataPtr, req->DataLen);
    res->DataPtr[6] = 0x00;
    res->DataPtr[7] = 0x00;
    return Parse_Success;
}

static ParseResult_t ParseScriptPortCreate(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    const char *txt;
    uint8_t speed;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    if (!GetString(act, I2C_SPEED, &txt, true))
        RETURN_ASSERT(Parse_XmlError);
    if (0 == strcmp(txt, I2C_SPEED_SLOW))
        speed = 0;
    else if (0 == strcmp(txt, I2C_SPEED_FAST))
        speed = 1;
    else
    {
        UcsXml_CB_OnError("Invalid I2C speed:'%s'", 1, txt);
        RETURN_ASSERT(Parse_XmlError);
    }
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x6C1;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    req->DataLen = 4;
    res->DataLen = 2;
    req->DataPtr = MCalloc(&priv->objList, req->DataLen, 1);
    if (NULL == req->DataPtr) return Parse_MemoryError;
    res->DataPtr = MCalloc(&priv->objList, res->DataLen, 1);
    if (NULL == res->DataPtr) return Parse_MemoryError;
    req->DataPtr[0] = 0x00; /* I2C Port Instance always 0 */
    req->DataPtr[1] = 0x00; /* I2C slave address, always 0, because we are Master */
    req->DataPtr[2] = 0x01; /* We are Master */
    req->DataPtr[3] = speed;

    res->DataPtr[0] = 0x0F;
    res->DataPtr[1] = 0x00;
    return Parse_Success;
}

static ParseResult_t ParseScriptPortWrite(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
#define HEADER_OFFSET 8
    const char *txt;
    uint8_t mode, blockCount, address, length, payloadLength;
    uint16_t timeout;
    uint8_t *payload;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
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
            RETURN_ASSERT(Parse_XmlError);
        }
    } else {
        mode = 0;
    }
    if (!GetUInt8(act, I2C_WRITE_BLOCK_COUNT, &blockCount, false))
        blockCount = 0;
    if (!GetUInt8(act, I2C_SLAVE_ADDRESS, &address, true))
        RETURN_ASSERT(Parse_XmlError);
    if (!GetUInt8(act, I2C_PAYLOAD_LENGTH, &length, false))
        length = 0;
    if (!GetUInt16(act, I2C_TIMEOUT, &timeout, false))
        timeout = 100;
    if (!GetPayload(act, I2C_PAYLOAD, &payload, &payloadLength, HEADER_OFFSET, &priv->objList, true))
        RETURN_ASSERT(Parse_XmlError);
    if (0 == length)
        length = payloadLength;
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x6C4;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    req->DataLen = payloadLength + HEADER_OFFSET;
    res->DataLen = 4;
    req->DataPtr = payload;
    res->DataPtr = MCalloc(&priv->objList, res->DataLen, 1);
    if (NULL == res->DataPtr) return Parse_MemoryError;

    req->DataPtr[0] = 0x0F;
    req->DataPtr[1] = 0x00;
    req->DataPtr[2] = mode;
    req->DataPtr[3] = blockCount;
    req->DataPtr[4] = address;
    req->DataPtr[5] = length;
    req->DataPtr[6] = MISC_HB(timeout);
    req->DataPtr[7] = MISC_LB(timeout);

    res->DataPtr[0] = 0x0F;
    res->DataPtr[1] = 0x00;
    res->DataPtr[2] = address;
    if (2 == mode)
        res->DataPtr[3] = blockCount * length;
    else
        res->DataPtr[3] = length;
    return Parse_Success;
}

static ParseResult_t ParseScriptPortRead(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    uint8_t address, length;
    uint16_t timeout;
    Ucs_Ns_ConfigMsg_t *req, *res;
    assert(NULL != act && NULL != scr && NULL != priv);
    if (!GetUInt8(act, I2C_SLAVE_ADDRESS, &address, true))
        RETURN_ASSERT(Parse_XmlError);
    if (!GetUInt8(act, I2C_PAYLOAD_LENGTH, &length, true))
        RETURN_ASSERT(Parse_XmlError);
    if (!GetUInt16(act, I2C_TIMEOUT, &timeout, false))
        timeout = 100;
    if (!FillScriptInitialValues(scr, priv))
        RETURN_ASSERT(Parse_MemoryError);
    req = scr->send_cmd;
    res = scr->exp_result;
    req->InstId = res->InstId = 1;
    req->FunktId = res->FunktId = 0x6C3;
    req->OpCode = 0x2;
    res->OpCode = 0xC;
    req->DataLen = 6;
    res->DataLen = 4;
    req->DataPtr = MCalloc(&priv->objList, req->DataLen, 1);
    if (NULL == req->DataPtr) return Parse_MemoryError;
    res->DataPtr = MCalloc(&priv->objList, res->DataLen, 1);
    if (NULL == res->DataPtr) return Parse_MemoryError;

    req->DataPtr[0] = 0x0F;
    req->DataPtr[1] = 0x00;
    req->DataPtr[2] = address;
    req->DataPtr[3] = length;
    req->DataPtr[4] = MISC_HB(timeout);
    req->DataPtr[5] = MISC_LB(timeout);

    res->DataPtr[0] = 0x0F;
    res->DataPtr[1] = 0x00;
    res->DataPtr[2] = address;
    res->DataPtr[3] = length;
    return Parse_Success;
}

static ParseResult_t ParseScriptPause(mxml_node_t *act, Ucs_Ns_Script_t *scr, PrivateData_t *priv)
{
    assert(NULL != act && NULL != priv);
    if (!GetUInt16(act, PAUSE_MS, &priv->scriptData.pause, true))
            RETURN_ASSERT(Parse_XmlError);
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
        if (!sourceRoute->isSource) /*There can be more sinks than sources, so count them*/
        {
            ++routeAmount;
        }
        sourceRoute = sourceRoute->next;
    }
    if (0 == routeAmount)
        return Parse_Success; /*Its okay to have no routes at all (e.g. MEP traffic only)*/
    ucs->pRoutes = MCalloc(&priv->objList, routeAmount, sizeof(Ucs_Rm_Route_t));
    if (NULL == ucs->pRoutes) RETURN_ASSERT(Parse_MemoryError);

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
                    Ucs_Rm_Route_t *route = &ucs->pRoutes[ucs->routesSize++];
                    route->source_endpoint_ptr = sourceRoute->ep;
                    route->sink_endpoint_ptr = sinkRoute->ep;
                    route->active = sinkRoute->isActive;
                    route->route_id = sinkRoute->routeId;
                }
                sinkRoute = sinkRoute->next;
            }
        }
        sourceRoute = sourceRoute->next;
    }
    if (routeAmount != ucs->routesSize)
    {
        UcsXml_CB_OnError("At least one sink (num=%d) is not connected, because of wrong Route name!", 2, (routeAmount - ucs->routesSize));
        RETURN_ASSERT(Parse_XmlError);
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
