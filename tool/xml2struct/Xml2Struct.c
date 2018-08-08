/*------------------------------------------------------------------------------------------------*/
/* UNICENS Stucture Printing module                                                               */
/* Copyright 2018, Microchip Technology Inc. and its subsidiaries.                                */
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "Console.h"
#include "Xml2Struct.h"

static const char *VERSION_STR = "V4.2.0";

#define CASE(X) case X: { return #X; }
#define CHECK_ASSERT(X) { \
    if (!(X)) { \
        ConsolePrintf(PRIO_ERROR, RED"Statement '"#X"' is false"RESETCOLOR"\n"); \
        assert(false); \
        exit(-1); } }

#ifdef C99_STRUCTS
#define C99(X) X
#else
#define C99(X)
#endif

struct ObjectList
{
    void *obj;
    struct ObjectList *next;
};

struct NameLookupTable
{
    Ucs_Xrm_ResObject_t *element;
    char *name;
    struct NameLookupTable *next;
};

struct LocalVar
{
    struct ObjectList objList;
    struct NameLookupTable allNames;
    uint16_t currentRoute;
    bool isSourceJob;
    const char *prefix;
};

static void PrintHeader(void);
static void *Mcalloc(struct ObjectList *list, uint32_t nElem, uint32_t elemSize);
static void Mfree(struct ObjectList *cur);
static char *GetNameFromTable(Ucs_Xrm_ResObject_t *element);
static void StoreNameInTable(Ucs_Xrm_ResObject_t *element, char *name);
static char *AllocateString(const char format[], uint16_t vargsCnt, ...);
static char *GetVariableName(Ucs_Xrm_ResObject_t *element, const char *shortName);
static void PrintDcPort(Ucs_Xrm_DefaultCreatedPort_t *port);
static void PrintNetworkSocket(Ucs_Xrm_MostSocket_t *socket);
static const char*GetMlbClkString(Ucs_Mlb_ClockConfig_t clk);
static void PrintMlbPort(Ucs_Xrm_MlbPort_t *port);
static void PrintMlbSocket(Ucs_Xrm_MlbSocket_t *socket);
static const char*GetUsbPhyString(Ucs_Usb_PhysicalLayer_t phy);
static void PrintUsbPort(Ucs_Xrm_UsbPort_t *port);
static void PrintUsbSocket(Ucs_Xrm_UsbSocket_t *socket);
static const char*GetStrmClkString(Ucs_Stream_PortClockConfig_t clk);
static const char*GetStrmAlignString(Ucs_Stream_PortDataAlign_t align);
static void PrintStrmPort(Ucs_Xrm_StrmPort_t *port);
static void PrintStrmSocket(Ucs_Xrm_StrmSocket_t *socket);
static void PrintCombiner(Ucs_Xrm_Combiner_t *combiner);
static void PrintSplitter(Ucs_Xrm_Splitter_t *splitter);
static const char*GetSyncConMuteString(Ucs_Sync_MuteMode_t mode);
static void PrintSyncCon(Ucs_Xrm_SyncCon_t *con);
static const char*GetAvpConIsocString(Ucs_Avp_IsocPacketSize_t isize);
static void PrintAvpCon(Ucs_Xrm_AvpCon_t *con);
static const char*GetTypeString(Ucs_Xrm_ResObject_t *element);
static const char*GetResourceTypeString(Ucs_Xrm_ResourceType_t *element);
static const char*GetDirectionString(Ucs_SocketDirection_t dir);
static const char*GetMostDataTypeString(Ucs_Most_SocketDataType_t dtyp);
static const char*GetMlbDataTypeString(Ucs_Mlb_SocketDataType_t dtyp);
static const char*GetUsbDataTypeString(Ucs_Usb_SocketDataType_t dtyp);
static const char*GetStreamDataTypeString(Ucs_Stream_SocketDataType_t dtyp);
static const char*GetPortTypeString(Ucs_Xrm_PortType_t ptyp);
static Ucs_Xrm_ResourceType_t GetType(Ucs_Xrm_ResObject_t *element);
static void PrintUcsElement(Ucs_Xrm_ResObject_t *element);
static void PrintJobs(Ucs_Xrm_ResObject_t **jobs_list_ptr);
static void PrintScripts(Ucs_Ns_Script_t *scripts, uint8_t len, uint16_t nodeAddress);
static void PrintNodes(Ucs_Rm_Node_t *nodes, uint8_t len);
static const char*GetEndpointTypeString(Ucs_Rm_EndPointType_t eptyp);
static void PrintEndpoint(Ucs_Rm_EndPoint_t *ep, bool isSourceEp, uint8_t routePos);
static void PrintRoutes(Ucs_Rm_Route_t *routes, uint8_t len);

static struct LocalVar m;

const char *GetXml2StructVersion(void)
{
	return VERSION_STR;
}

void PrintUcsStructures(
    uint16_t packetBw,
    Ucs_Rm_Route_t *pRoutes,
    uint16_t routesSize,
    Ucs_Rm_Node_t *pNod,
    uint16_t nodSize,
    const char *variablePrefix)
{
    uint16_t i;
    memset(&m, 0, sizeof(struct LocalVar));
    CHECK_ASSERT(pNod);
    CHECK_ASSERT(nodSize);
    if (NULL == variablePrefix)
        m.prefix = "";
    else
        m.prefix = variablePrefix;
    PrintHeader();
    ConsolePrintf(PRIO_HIGH, "#include \"ucs_api.h\"\n\n");
    ConsolePrintf(PRIO_HIGH, "uint16_t %sPacketBandwidth = %u;\n", m.prefix, packetBw);
    ConsolePrintf(PRIO_HIGH, "uint16_t %sRoutesSize = %u;\n", m.prefix, routesSize);
    ConsolePrintf(PRIO_HIGH, "uint16_t %sNodeSize = %u;\n\n", m.prefix, nodSize);
    /* Iterate all routes for printing the resources*/
    for (i = 0; i < routesSize; i++)
    {
        Ucs_Rm_Route_t *route = &pRoutes[i];
        CHECK_ASSERT(route->source_endpoint_ptr);
        CHECK_ASSERT(route->sink_endpoint_ptr);
        CHECK_ASSERT(route->source_endpoint_ptr->jobs_list_ptr);
        CHECK_ASSERT(route->sink_endpoint_ptr->jobs_list_ptr);
        m.currentRoute = i + 1;
        m.isSourceJob = true;
        ConsolePrintf(PRIO_HIGH, "/* Route %d from source-node=0x%X to sink-node=0x%X */\n",
                m.currentRoute,
                route->source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address,
                route->sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address);
        
        PrintJobs(route->source_endpoint_ptr->jobs_list_ptr);
        m.isSourceJob = false;
        PrintJobs(route->sink_endpoint_ptr->jobs_list_ptr);
    }
    if (0 == routesSize)
    {
        ConsolePrintf(PRIO_HIGH, "Ucs_Rm_Route_t *%sAllRoutes = NULL;\n", m.prefix);
    }
    /* Iterate all scripts */
    for (i = 0; i < nodSize; i++)
    {
        Ucs_Rm_Node_t *node = &pNod[i];
        if (node->script_list_size)
        {
            CHECK_ASSERT(node->signature_ptr);
            CHECK_ASSERT(0 != node->signature_ptr->node_address);
            PrintScripts(node->script_list_ptr, node->script_list_size, node->signature_ptr->node_address);
        }
    }
    if (0 == nodSize)
    {
        ConsolePrintf(PRIO_HIGH, "Ucs_Rm_Node_t *%sAllNodes = NULL;\n", m.prefix);
    }
    PrintNodes(pNod, nodSize);
    PrintRoutes(pRoutes, routesSize);
    Mfree(&m.objList);
}

void PrintHeaderFile(const char *variablePrefix)
{
    const char *prefix;
    if (NULL == variablePrefix)
        prefix = "";
    else
        prefix = variablePrefix;
    PrintHeader();
    ConsolePrintfStart(PRIO_HIGH,"#ifndef %s_DEFAULT_CONFIG_H_\n", prefix);
    ConsolePrintfContinue("#define %s_DEFAULT_CONFIG_H_\n\n", prefix);
    ConsolePrintfContinue("#ifdef __cplusplus\n");
    ConsolePrintfContinue("extern \"C\" {\n");
    ConsolePrintfContinue("#endif\n\n");
    ConsolePrintfContinue("#include \"ucs_api.h\"\n\n");
    ConsolePrintfContinue("extern uint16_t %sPacketBandwidth;\n", prefix);
    ConsolePrintfContinue("extern uint16_t %sRoutesSize;\n", prefix);
    ConsolePrintfContinue("extern uint16_t %sNodeSize;\n", prefix);
    ConsolePrintfContinue("extern Ucs_Rm_Route_t %sAllRoutes[];\n", prefix);
    ConsolePrintfContinue("extern Ucs_Rm_Node_t %sAllNodes[];\n\n", prefix);
    ConsolePrintfContinue("#ifdef __cplusplus\n");
    ConsolePrintfContinue("}\n");
    ConsolePrintfContinue("#endif\n\n");
    ConsolePrintfExit("#endif /* %s_DEFAULT_CONFIG_H_ */\n", prefix);
}

static void PrintHeader(void) {
    ConsolePrintfStart(PRIO_HIGH, "/*------------------------------------------------------------------------------------------------*/\n");
    ConsolePrintfContinue("/* UNICENS Generated Network Configuration                                                        */\n");
	ConsolePrintfContinue("/* Generator: xml2struct for Linux %s                                                         */\n", VERSION_STR);
    ConsolePrintfExit("/*------------------------------------------------------------------------------------------------*/\n");
}

void *Mcalloc(struct ObjectList *list, uint32_t nElem, uint32_t elemSize)
{
    void *obj;
    struct ObjectList *tail = list;
    if (NULL == list || 0 == nElem || 0 == elemSize) return NULL;
    
    obj = calloc(nElem, elemSize);
    CHECK_ASSERT(obj);
    if (NULL == list->obj)
    {
        list->obj = obj;
        return obj;
    }
    while(tail->next) tail = tail->next;
    tail->next = calloc(1, sizeof(struct ObjectList));
    CHECK_ASSERT(tail->next);
    tail->next->obj = obj;
    return obj;
}

void Mfree(struct ObjectList *cur)
{
    struct ObjectList *root = cur;
    while(cur)
    {
        struct ObjectList *next = cur->next;
        if (cur->obj)
            free(cur->obj);
        if (cur != root)
            free(cur);
        cur = next;
    }
}

static char *GetNameFromTable(Ucs_Xrm_ResObject_t *element)
{
    struct NameLookupTable *tail = &m.allNames;
    if (!element) return NULL;
    while(tail)
    {
        if (element == tail->element)
            return tail->name;
        tail = tail->next;
    }
    return NULL;
}

static void StoreNameInTable(Ucs_Xrm_ResObject_t *element, char *name)
{
    struct NameLookupTable *tail = &m.allNames;
    CHECK_ASSERT(NULL == GetNameFromTable(element));
    while(tail)
    {
        if (tail->next)
        {
            tail = tail->next;
        }
        else {
            tail->next = Mcalloc(&m.objList, 1, sizeof(struct NameLookupTable));
            CHECK_ASSERT(tail->next);
            tail = tail->next;
            break;
        }
    }
    tail->element = element;
    tail->name = name;
}

static char *AllocateString(const char format[], uint16_t vargsCnt, ...)
{
    int32_t len;
    char *returnString;
    va_list argptr;
    char outbuf[100];
    va_start(argptr, vargsCnt);
    vsprintf(outbuf, format, argptr);
    va_end(argptr);
    len = strlen(outbuf);
    returnString = Mcalloc(&m.objList, 1 + len, 1);
    CHECK_ASSERT(returnString);
    memcpy(returnString, outbuf, len);
    return returnString;
}

static char *GetVariableName(Ucs_Xrm_ResObject_t *element, const char *shortName)
{
    char *name = GetNameFromTable(element);
    if (name)
        return name;
    if (!shortName) return NULL;
    name = AllocateString("%s%sOfRoute%d_%s", 4, 
        m.prefix,
        m.isSourceJob ? "Src" : "Snk",
        m.currentRoute,
        shortName);
    CHECK_ASSERT(name);
    StoreNameInTable(element, name);
    return name;
}

static void PrintDcPort(Ucs_Xrm_DefaultCreatedPort_t *port)
{ 
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(port), 
            GetVariableName(port, "DcPort"),
            GetResourceTypeString(&port->resource_type));
    ConsolePrintfContinue(C99(".port_type = ")"%s,\n"TAB, GetPortTypeString(port->port_type));
    ConsolePrintfContinue(C99(".index = ")"%u", port->index);
    ConsolePrintfExit(" };\n");
}

static void PrintNetworkSocket(Ucs_Xrm_MostSocket_t *socket)
{
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(socket), 
            GetVariableName(socket, "NetworkSocket"),
            GetResourceTypeString(&socket->resource_type));
    ConsolePrintfContinue(C99(".most_port_handle = ")"0x%04X,\n"TAB, socket->most_port_handle);
    ConsolePrintfContinue(C99(".direction = ")"%s,\n"TAB, GetDirectionString(socket->direction));
    ConsolePrintfContinue(C99(".data_type = ")"%s,\n"TAB, GetMostDataTypeString(socket->data_type));
    ConsolePrintfContinue(C99(".bandwidth = ")"%u", socket->bandwidth);
    ConsolePrintfExit(" };\n");
}

static const char*GetMlbClkString(Ucs_Mlb_ClockConfig_t clk)
{
    switch(clk)
    {
        CASE(UCS_MLB_CLK_CFG_256_FS);
        CASE(UCS_MLB_CLK_CFG_512_FS);
        CASE(UCS_MLB_CLK_CFG_1024_FS);
        CASE(UCS_MLB_CLK_CFG_2048_FS);
        CASE(UCS_MLB_CLK_CFG_3072_FS);
        CASE(UCS_MLB_CLK_CFG_4096_FS);
        CASE(UCS_MLB_CLK_CFG_6144_FS);
        CASE(UCS_MLB_CLK_CFG_8192_FS);
        CASE(UCS_MLB_CLK_CFG_WILDCARD);
    default:
        ConsolePrintf(PRIO_ERROR, "GetMlbClkString clk-config:%d not implemented\n", clk);
        exit(-1);
    }
}

static void PrintMlbPort(Ucs_Xrm_MlbPort_t *port)
{
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(port), 
            GetVariableName(port, "MlbPort"),
            GetResourceTypeString(&port->resource_type));
    ConsolePrintfContinue(C99(".index = ")"%d,\n"TAB, port->index);
    ConsolePrintfContinue(C99(".clock_config = ")"%s", GetMlbClkString(port->clock_config));
    ConsolePrintfExit(" };\n");
}

static void PrintMlbSocket(Ucs_Xrm_MlbSocket_t *socket)
{
    PrintUcsElement(socket->mlb_port_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(socket), 
            GetVariableName(socket, "MlbSocket"),
            GetResourceTypeString(&socket->resource_type));
    ConsolePrintfContinue(C99(".mlb_port_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(socket->mlb_port_obj_ptr));
    ConsolePrintfContinue(C99(".direction = ")"%s,\n"TAB, GetDirectionString(socket->direction));
    ConsolePrintfContinue(C99(".data_type = ")"%s,\n"TAB, GetMlbDataTypeString(socket->data_type));
    ConsolePrintfContinue(C99(".bandwidth = ")"%u,\n"TAB, socket->bandwidth);
    ConsolePrintfContinue(C99(".channel_address = ")"0x%02X", socket->channel_address);
    ConsolePrintfExit(" };\n");
}

static const char*GetUsbPhyString(Ucs_Usb_PhysicalLayer_t phy)
{
    switch(phy)
    {
        CASE(UCS_USB_PHY_LAYER_STANDARD);
        CASE(UCS_USB_PHY_LAYER_HSCI);
    default:
        ConsolePrintf(PRIO_ERROR, "GetUsbPhyString phy:%d not implemented\n", phy);
        exit(-1);
    }
}

static void PrintUsbPort(Ucs_Xrm_UsbPort_t *port)
{
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(port), 
            GetVariableName(port, "UsbPort"),
            GetResourceTypeString(&port->resource_type));
    ConsolePrintfContinue(C99(".index = ")"%d,\n"TAB, port->index);
    ConsolePrintfContinue(C99(".physical_layer = ")"%s,\n"TAB, GetUsbPhyString(port->physical_layer));
    ConsolePrintfContinue(C99(".devices_interfaces = ")"0x%04X,\n"TAB, port->devices_interfaces);
    ConsolePrintfContinue(C99(".streaming_if_ep_out_count = ")"%d,\n"TAB, port->streaming_if_ep_out_count);
    ConsolePrintfContinue(C99(".streaming_if_ep_in_count = ")"%d", port->streaming_if_ep_in_count);
    ConsolePrintfExit(" };\n");
}

static void PrintUsbSocket(Ucs_Xrm_UsbSocket_t *socket)
{
    PrintUcsElement(socket->usb_port_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(socket), 
            GetVariableName(socket, "UsbSocket"),
            GetResourceTypeString(&socket->resource_type));
    ConsolePrintfContinue(C99(".usb_port_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(socket->usb_port_obj_ptr));
    ConsolePrintfContinue(C99(".direction = ")"%s,\n"TAB, GetDirectionString(socket->direction));
    ConsolePrintfContinue(C99(".data_type = ")"%s,\n"TAB, GetUsbDataTypeString(socket->data_type));
    ConsolePrintfContinue(C99(".end_point_addr = ")"0x%02X,\n"TAB, socket->end_point_addr);
    ConsolePrintfContinue(C99(".frames_per_transfer = ")"%u", socket->frames_per_transfer);
    ConsolePrintfExit(" };\n");
}

static const char*GetStrmClkString(Ucs_Stream_PortClockConfig_t clk)
{
    switch(clk)
    {
        CASE(UCS_STREAM_PORT_CLK_CFG_8FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_16FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_32FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_64FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_128FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_256FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_512FS);
        CASE(UCS_STREAM_PORT_CLK_CFG_WILD);
    default:
        ConsolePrintf(PRIO_ERROR, "GetStrmClkString stream-config:%d not implemented\n", clk);
        exit(-1);
    }
}

static const char*GetStrmAlignString(Ucs_Stream_PortDataAlign_t align)
{
    switch(align)
    {
        CASE(UCS_STREAM_PORT_ALGN_LEFT16BIT);
        CASE(UCS_STREAM_PORT_ALGN_LEFT24BIT);
        CASE(UCS_STREAM_PORT_ALGN_RIGHT16BIT);
        CASE(UCS_STREAM_PORT_ALGN_RIGHT24BIT);
        CASE(UCS_STREAM_PORT_ALGN_SEQ);
#ifdef TDM_STREAM_FORMAT_SUPPORTED
        CASE(UCS_STREAM_PORT_ALGN_TDM16BIT);
        CASE(UCS_STREAM_PORT_ALGN_TDM24BIT);
#endif
    default:
        ConsolePrintf(PRIO_ERROR, "GetStrmAlignString align:%d not implemented\n", align);
        exit(-1);
    }
}

static void PrintStrmPort(Ucs_Xrm_StrmPort_t *port)
{
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(port), 
            GetVariableName(port, (0 == port->index ? "StrmPort0" : "StrmPort1")),
            GetResourceTypeString(&port->resource_type));
    ConsolePrintfContinue(C99(".index = ")"%u,\n"TAB, port->index);
    ConsolePrintfContinue(C99(".clock_config = ")"%s,\n"TAB, GetStrmClkString(port->clock_config));
    ConsolePrintfContinue(C99(".data_alignment = ")"%s", GetStrmAlignString(port->data_alignment));
    ConsolePrintfExit(" };\n");
}

static const char*GetStrmPinString(Ucs_Stream_PortPinId_t pin)
{
    switch(pin)
    {
        CASE(UCS_STREAM_PORT_PIN_ID_SRXA0);
        CASE(UCS_STREAM_PORT_PIN_ID_SRXA1);
        CASE(UCS_STREAM_PORT_PIN_ID_SRXB0);
        CASE(UCS_STREAM_PORT_PIN_ID_SRXB1);
    default:
        ConsolePrintf(PRIO_ERROR, "GetStrmPinString pin:%d not implemented\n", pin);
        exit(-1);
    }
}

static void PrintStrmSocket(Ucs_Xrm_StrmSocket_t *socket)
{
    PrintUcsElement(socket->stream_port_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(socket), 
            GetVariableName(socket, "StrmSocket"),
            GetResourceTypeString(&socket->resource_type));
    ConsolePrintfContinue(C99(".stream_port_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(socket->stream_port_obj_ptr));
    ConsolePrintfContinue(C99(".direction = ")"%s,\n"TAB, GetDirectionString(socket->direction));
    ConsolePrintfContinue(C99(".data_type = ")"%s,\n"TAB, GetStreamDataTypeString(socket->data_type));
    ConsolePrintfContinue(C99(".bandwidth = ")"%u,\n"TAB, socket->bandwidth);
    ConsolePrintfContinue(C99(".stream_pin_id = ")"%s", GetStrmPinString(socket->stream_pin_id));
    ConsolePrintfExit(" };\n");
}

static void PrintCombiner(Ucs_Xrm_Combiner_t *combiner)
{
    PrintUcsElement(combiner->port_socket_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(combiner), 
            GetVariableName(combiner, "Combiner"),
            GetResourceTypeString(&combiner->resource_type));
    ConsolePrintfContinue(C99(".port_socket_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(combiner->port_socket_obj_ptr));
    ConsolePrintfContinue(C99(".most_port_handle = ")"0x%04X,\n"TAB, combiner->most_port_handle);
    ConsolePrintfContinue(C99(".bytes_per_frame = ")"%u", combiner->bytes_per_frame);
    ConsolePrintfExit(" };\n");
}

static void PrintSplitter(Ucs_Xrm_Splitter_t *splitter)
{
    PrintUcsElement(splitter->socket_in_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(splitter),
            GetVariableName(splitter, "Splitter"),
            GetResourceTypeString(&splitter->resource_type));
    ConsolePrintfContinue(C99(".socket_in_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(splitter->socket_in_obj_ptr));
    ConsolePrintfContinue(C99(".most_port_handle = ")"0x%04X,\n"TAB, splitter->most_port_handle);
    ConsolePrintfContinue(C99(".bytes_per_frame = ")"%u", splitter->bytes_per_frame);
    ConsolePrintfExit(" };\n");
}

static const char*GetSyncConMuteString(Ucs_Sync_MuteMode_t mode)
{
    switch(mode)
    {
        CASE(UCS_SYNC_MUTE_MODE_NO_MUTING);
        CASE(UCS_SYNC_MUTE_MODE_MUTE_SIGNAL);
    default:
        ConsolePrintf(PRIO_ERROR, "GetSyncConMuteString mode:%d not implemented\n", mode);
        exit(-1);
    }
}

static void PrintSyncCon(Ucs_Xrm_SyncCon_t *con)
{
    PrintUcsElement(con->socket_in_obj_ptr);
    PrintUcsElement(con->socket_out_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(con), 
            GetVariableName(con, "SyncCon"),
            GetResourceTypeString(&con->resource_type));
    ConsolePrintfContinue(C99(".socket_in_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(con->socket_in_obj_ptr));
    ConsolePrintfContinue(C99(".socket_out_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(con->socket_out_obj_ptr));
    ConsolePrintfContinue(C99(".mute_mode = ")"%s,\n"TAB, GetSyncConMuteString(con->mute_mode));
    ConsolePrintfContinue(C99(".offset = ")"%u", con->offset);
    ConsolePrintfExit(" };\n");
}

static const char*GetAvpConIsocString(Ucs_Avp_IsocPacketSize_t isize)
{
    switch(isize)
    {
        CASE(UCS_ISOC_PCKT_SIZE_188);
        CASE(UCS_ISOC_PCKT_SIZE_196);
        CASE(UCS_ISOC_PCKT_SIZE_206);
    default:
        ConsolePrintf(PRIO_ERROR, "GetAvpConIsocString size:%d not implemented\n", isize);
        assert(false);
        exit(-1);
    }
}

static void PrintAvpCon(Ucs_Xrm_AvpCon_t *con)
{
    PrintUcsElement(con->socket_in_obj_ptr);
    PrintUcsElement(con->socket_out_obj_ptr);
    ConsolePrintfStart(PRIO_HIGH, "%s %s = { \n"TAB C99(".resource_type = ")"%s,\n"TAB, 
            GetTypeString(con), 
            GetVariableName(con, "AvpCon"),
            GetResourceTypeString(&con->resource_type));
    ConsolePrintfContinue(C99(".socket_in_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(con->socket_in_obj_ptr));
    ConsolePrintfContinue(C99(".socket_out_obj_ptr = ")"&%s,\n"TAB, GetNameFromTable(con->socket_out_obj_ptr));
    ConsolePrintfContinue(C99(".isoc_packet_size = ")"%s", GetAvpConIsocString(con->isoc_packet_size));
    ConsolePrintfExit(" };\n");
}

static const char*GetTypeString(Ucs_Xrm_ResObject_t *element)
{
    Ucs_Xrm_ResourceType_t typ = GetType(element);
    switch(typ)
    {
        case UCS_XRM_RC_TYPE_DC_PORT: return "Ucs_Xrm_DefaultCreatedPort_t";
        case UCS_XRM_RC_TYPE_MOST_SOCKET: return "Ucs_Xrm_MostSocket_t";
        case UCS_XRM_RC_TYPE_MLB_PORT: return "Ucs_Xrm_MlbPort_t";
        case UCS_XRM_RC_TYPE_MLB_SOCKET: return "Ucs_Xrm_MlbSocket_t";
        case UCS_XRM_RC_TYPE_USB_PORT: return "Ucs_Xrm_UsbPort_t";
        case UCS_XRM_RC_TYPE_USB_SOCKET: return "Ucs_Xrm_UsbSocket_t";
        case UCS_XRM_RC_TYPE_STRM_PORT: return "Ucs_Xrm_StrmPort_t";
        case UCS_XRM_RC_TYPE_STRM_SOCKET: return "Ucs_Xrm_StrmSocket_t";
        case UCS_XRM_RC_TYPE_SYNC_CON: return "Ucs_Xrm_SyncCon_t";
        case UCS_XRM_RC_TYPE_COMBINER: return "Ucs_Xrm_Combiner_t";
        case UCS_XRM_RC_TYPE_SPLITTER: return "Ucs_Xrm_Splitter_t";
        case UCS_XRM_RC_TYPE_AVP_CON: return "Ucs_Xrm_AvpCon_t";
    default:
        ConsolePrintf(PRIO_ERROR, "GetTypeString ResourceType:%d not implemented\n", typ);
        exit(-1);
    }
}

static const char*GetResourceTypeString(Ucs_Xrm_ResourceType_t *element)
{
    Ucs_Xrm_ResourceType_t typ = GetType(element);
    switch(typ)
    {
        CASE(UCS_XRM_RC_TYPE_DC_PORT);
        CASE(UCS_XRM_RC_TYPE_MOST_SOCKET);
        CASE(UCS_XRM_RC_TYPE_MLB_PORT);
        CASE(UCS_XRM_RC_TYPE_MLB_SOCKET);
        CASE(UCS_XRM_RC_TYPE_USB_PORT);
        CASE(UCS_XRM_RC_TYPE_USB_SOCKET);
        CASE(UCS_XRM_RC_TYPE_STRM_PORT);
        CASE(UCS_XRM_RC_TYPE_STRM_SOCKET);
        CASE(UCS_XRM_RC_TYPE_SYNC_CON);
        CASE(UCS_XRM_RC_TYPE_COMBINER);
        CASE(UCS_XRM_RC_TYPE_SPLITTER);
        CASE(UCS_XRM_RC_TYPE_AVP_CON);
        default:
            ConsolePrintf(PRIO_ERROR, "GetResourceTypeString ResourceType:%d not implemented\n", typ);
            exit(-1);
    }
}

static const char*GetDirectionString(Ucs_SocketDirection_t dir)
{
    switch(dir)
    {
        CASE(UCS_SOCKET_DIR_INPUT);
        CASE(UCS_SOCKET_DIR_OUTPUT);
        default:
            ConsolePrintf(PRIO_ERROR, "GetDirectionString Ucs_SocketDirection_t:%d not implemented\n", dir);
            exit(-1);
    }
}

static const char*GetMostDataTypeString(Ucs_Most_SocketDataType_t dtyp)
{
    switch(dtyp)
    {
        CASE(UCS_MOST_SCKT_SYNC_DATA);
        CASE(UCS_MOST_SCKT_AV_PACKETIZED);
        CASE(UCS_MOST_SCKT_QOS_IP);
        CASE(UCS_MOST_SCKT_DISC_FRAME_PHASE);
        default:
            ConsolePrintf(PRIO_ERROR, "GetMostDataTypeString Ucs_Most_SocketDataType_t:%d not implemented\n", dtyp);
            exit(-1);
    }
}

static const char*GetMlbDataTypeString(Ucs_Mlb_SocketDataType_t dtyp)
{
    switch(dtyp)
    {
        CASE(UCS_MLB_SCKT_SYNC_DATA);
        CASE(UCS_MLB_SCKT_CONTROL_DATA);
        CASE(UCS_MLB_SCKT_AV_PACKETIZED);
        CASE(UCS_MLB_SCKT_QOS_IP);
        CASE(UCS_MLB_SCKT_DISC_FRAME_PHASE);
        CASE(UCS_MLB_SCKT_IPC_PACKET);
        default:
            ConsolePrintf(PRIO_ERROR, "GetMlbDataTypeString Ucs_Mlb_SocketDataType_t:%d not implemented\n", dtyp);
            exit(-1);
    }
}

static const char*GetUsbDataTypeString(Ucs_Usb_SocketDataType_t dtyp)
{
    switch(dtyp)
    {
        CASE(UCS_USB_SCKT_SYNC_DATA);
        CASE(UCS_USB_SCKT_CONTROL_DATA);
        CASE(UCS_USB_SCKT_AV_PACKETIZED);
        CASE(UCS_USB_SCKT_IPC_PACKET);
        default:
            ConsolePrintf(PRIO_ERROR, "GetUsbDataTypeString Ucs_Usb_SocketDataType_t:%d not implemented\n", dtyp);
            exit(-1);
    }
}

static const char*GetStreamDataTypeString(Ucs_Stream_SocketDataType_t dtyp)
{
    switch(dtyp)
    {
        CASE(UCS_STREAM_PORT_SCKT_SYNC_DATA);
        default:
            ConsolePrintf(PRIO_ERROR, "GetStramDataTypeString Ucs_Stream_SocketDataType_t:%d not implemented\n", dtyp);
            exit(-1);
    }
}

static const char*GetPortTypeString(Ucs_Xrm_PortType_t ptyp)
{
    switch(ptyp)
    {
        CASE(UCS_XRM_PORT_TYPE_MLB);
        CASE(UCS_XRM_PORT_TYPE_USB);
        CASE(UCS_XRM_PORT_TYPE_STRM);
        default:
            ConsolePrintf(PRIO_ERROR, "GetPortTypeString Ucs_Xrm_PortType_t:%d not implemented\n", ptyp);
            exit(-1);
    }
}

static Ucs_Xrm_ResourceType_t GetType(Ucs_Xrm_ResObject_t *element)
{
    Ucs_Xrm_ResourceType_t typ = *((Ucs_Xrm_ResourceType_t *)element);
    CHECK_ASSERT(UCS_XRM_RC_TYPE_QOS_CON >= typ);
    return typ;
}

static void PrintUcsElement(Ucs_Xrm_ResObject_t *element)
{
    Ucs_Xrm_ResourceType_t typ;
    if (GetNameFromTable(element))
        return; /* Already defined */
    typ = GetType(element);
    switch(typ)
    {
    case UCS_XRM_RC_TYPE_DC_PORT:
        PrintDcPort((Ucs_Xrm_DefaultCreatedPort_t *)element);
        break;
    case UCS_XRM_RC_TYPE_MOST_SOCKET:
        PrintNetworkSocket((Ucs_Xrm_MostSocket_t *)element);
        break;
    case UCS_XRM_RC_TYPE_MLB_PORT:
        PrintMlbPort((Ucs_Xrm_MlbPort_t *)element);
        break;
    case UCS_XRM_RC_TYPE_MLB_SOCKET:
        PrintMlbSocket((Ucs_Xrm_MlbSocket_t *)element);
        break;
    case UCS_XRM_RC_TYPE_USB_PORT:
        PrintUsbPort((Ucs_Xrm_UsbPort_t *)element);
        break;
    case UCS_XRM_RC_TYPE_USB_SOCKET:
        PrintUsbSocket((Ucs_Xrm_UsbSocket_t *)element);
        break;
    case UCS_XRM_RC_TYPE_STRM_PORT:
        PrintStrmPort((Ucs_Xrm_StrmPort_t *)element);
        break;
    case UCS_XRM_RC_TYPE_STRM_SOCKET:
        PrintStrmSocket((Ucs_Xrm_StrmSocket_t *)element);
        break;
    case UCS_XRM_RC_TYPE_SYNC_CON:
        PrintSyncCon((Ucs_Xrm_SyncCon_t *)element);
        break;
    case UCS_XRM_RC_TYPE_COMBINER:
        PrintCombiner((Ucs_Xrm_Combiner_t *)element);
        break;
    case UCS_XRM_RC_TYPE_SPLITTER:
        PrintSplitter((Ucs_Xrm_Splitter_t *)element);
        break;
    case UCS_XRM_RC_TYPE_AVP_CON:
        PrintAvpCon((Ucs_Xrm_AvpCon_t *)element);
        break;
    default:
        ConsolePrintf(PRIO_ERROR, "PrintUcsElement ResourceType:%d not implemented\n", typ);
        exit(-1);
    }
}

static void PrintJobs(Ucs_Xrm_ResObject_t **jobs_list_ptr)
{
    uint16_t i;
    Ucs_Xrm_ResObject_t *job = NULL;
    for (i = 0; (job = jobs_list_ptr[i]); i++ )
    {
        PrintUcsElement(job);
    }
    if (GetVariableName(jobs_list_ptr, NULL))
        return; /*Already printed */
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Xrm_ResObject_t *%s[] = {\n"TAB,
        GetVariableName(jobs_list_ptr, "JobList"));
    for (i = 0; (job = jobs_list_ptr[i]); i++ )
    {
        if(i) ConsolePrintfContinue(",\n"TAB);
        ConsolePrintfContinue("&%s", GetVariableName(job, NULL));
    }
    ConsolePrintfContinue(",\n"TAB"NULL");
    ConsolePrintfExit(" };\n");
}

static void PrintScriptMessage(Ucs_Ns_ConfigMsg_t *msg, uint16_t nodeAddress, bool isRequest, uint8_t scriptNr)
{
    uint8_t i;
    char *varName;
    bool gotPayload = (0xFF != msg->DataLen && NULL != msg->DataPtr);
    if (GetNameFromTable(msg)) return;
    varName = AllocateString("%s%s%dForNode%x", 3, 
        m.prefix,
        isRequest ? "Request" : "Response", 
        scriptNr, nodeAddress);
    StoreNameInTable(msg, varName);
    if (gotPayload)
    {
        ConsolePrintfStart(PRIO_HIGH, "uint8_t Payload%s[] = {\n"TAB, varName);
        for (i = 0; i < msg->DataLen; i++)
        {
            if(i) ConsolePrintfContinue(", ");
            ConsolePrintfContinue("0x%02X", msg->DataPtr[i]);
        }
        ConsolePrintfExit(" };\n");
    }
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Ns_ConfigMsg_t %s = {\n"TAB, varName);
    ConsolePrintfContinue(C99(".FBlockId = ")"0x%02X,\n"TAB, msg->FBlockId);
    ConsolePrintfContinue(C99(".InstId = ")"0x%02X,\n"TAB, msg->InstId);
    ConsolePrintfContinue(C99(".FunktId = ")"0x%04X,\n"TAB, msg->FunktId);
    ConsolePrintfContinue(C99(".OpCode = ")"0x%02X,\n"TAB, msg->OpCode);
    ConsolePrintfContinue(C99(".DataLen = ")"0x%02X,\n"TAB, msg->DataLen);
    if (gotPayload)
        ConsolePrintfContinue(C99(".DataPtr = ")"Payload%s", varName);
    else
        ConsolePrintfContinue(C99(".DataPtr = ")"NULL");
    ConsolePrintfExit(" };\n");
}

static void PrintScripts(Ucs_Ns_Script_t *scripts, uint8_t len, uint16_t nodeAddress)
{
    uint8_t i;
    Ucs_Ns_Script_t *script = NULL;
    char *varName;
    if (NULL == scripts || 0 == len) return;
    if (GetNameFromTable(scripts)) return;
    for (i = 0; i < len; i++ )
    {
        script = &scripts[i];
        CHECK_ASSERT(script->send_cmd);
        PrintScriptMessage(script->send_cmd, nodeAddress, true, (i + 1));
        if (script->exp_result)
            PrintScriptMessage(script->exp_result, nodeAddress, false, (i + 1));
    }
    varName = AllocateString("%sScriptsForNode%X", 2, m.prefix, nodeAddress);
    StoreNameInTable(scripts, varName);
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Ns_Script_t %s[] = {\n"TAB"{\n"TAB, varName);
    for (i = 0; i < len; i++ )
    {
        script = &scripts[i];
        if(i) ConsolePrintfContinue(", {\n"TAB);
        ConsolePrintfContinue(TAB C99(".pause = ")"%u,\n"TAB, script->pause);
        ConsolePrintfContinue(TAB C99(".send_cmd = ")"&%s,\n"TAB, GetVariableName(script->send_cmd, NULL));
        if (script->exp_result)
            ConsolePrintfContinue(TAB C99(".exp_result = ")"&%s\n"TAB, GetVariableName(script->exp_result, NULL));
        else
            ConsolePrintfContinue(TAB C99(".exp_result = ")"NULL");
        ConsolePrintfContinue("}");
    }
    ConsolePrintfExit(" };\n");
}

static void PrintSignature(Ucs_Signature_t *signature)
{
    CHECK_ASSERT(signature);
    ConsolePrintf(PRIO_HIGH, "Ucs_Signature_t %sSignatureForNode%X = { "C99(".node_address = ")"0x%X };\n",
        m.prefix, signature->node_address, signature->node_address);
}

static void PrintNodes(Ucs_Rm_Node_t *nodes, uint8_t len)
{
    uint8_t i;
    CHECK_ASSERT(nodes);
    CHECK_ASSERT(len);
    /* Iterate all nodes for printing signatures and node definition */
    for (i = 0; i < len; i++)
    {
        Ucs_Rm_Node_t *node = &nodes[i];
        PrintSignature(node->signature_ptr);
    }
    /* Iterate all nodes for printing node informations*/
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Rm_Node_t %sAllNodes[] = {\n"TAB"{\n"TAB, m.prefix);
    for (i = 0; i < len; i++)
    {
        Ucs_Rm_Node_t *node = &nodes[i];
        uint16_t addr = node->signature_ptr->node_address;
        char *varName = AllocateString("%sAllNodes[%d]", 2, m.prefix, i);
        StoreNameInTable(node, varName);
        if(i) ConsolePrintfContinue(", {\n"TAB);
        ConsolePrintfContinue(TAB C99(".signature_ptr = ")"&%sSignatureForNode%X,\n"TAB, m.prefix, addr);
        if (node->script_list_ptr && node->script_list_size)
        {
            ConsolePrintfContinue(TAB C99(".script_list_ptr = ")"%s,\n"TAB, GetNameFromTable(node->script_list_ptr));
            ConsolePrintfContinue(TAB C99(".script_list_size = ")"%u\n"TAB, node->script_list_size);
        }
        else
        {
            ConsolePrintfContinue(TAB C99(".script_list_ptr = ")"NULL,\n"TAB);
            ConsolePrintfContinue(TAB C99(".script_list_size = ")"0\n"TAB);
        }
        ConsolePrintfContinue("}");
    }
    ConsolePrintfExit(" };\n");
}

static const char*GetEndpointTypeString(Ucs_Rm_EndPointType_t eptyp)
{
    switch(eptyp)
    {
        CASE(UCS_RM_EP_SOURCE);
        CASE(UCS_RM_EP_SINK);
        default:
            ConsolePrintf(PRIO_ERROR, "GetEndpointTypeString ep-type:%d not implemented\n", eptyp);
            exit(-1);
    }
}

static void PrintEndpoint(Ucs_Rm_EndPoint_t *ep, bool isSourceEp, uint8_t routePos)
{
    char *varName;
    CHECK_ASSERT(ep);
    if (GetNameFromTable(ep)) return;
    varName = AllocateString("%s%sEndpointForRoute%u", 
        3, m.prefix, (isSourceEp ? "Source" : "Sink"), (routePos + 1));
    StoreNameInTable(ep, varName);
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Rm_EndPoint_t %s = {\n"TAB, varName);
    ConsolePrintfContinue(C99(".endpoint_type = ")"%s,\n"TAB, GetEndpointTypeString(ep->endpoint_type));
    ConsolePrintfContinue(C99(".jobs_list_ptr = ")"%s,\n"TAB, GetNameFromTable(ep->jobs_list_ptr));
    ConsolePrintfContinue(C99(".node_obj_ptr = ")"&%s", GetNameFromTable(ep->node_obj_ptr));
    ConsolePrintfExit(" };\n");
}

static void PrintRoutes(Ucs_Rm_Route_t *routes, uint8_t len)
{
    uint8_t i;
    if (NULL == routes || 0 == len)
        return;
    /* Iterate all routes to create Endpoints */
    for (i = 0; i < len; i++)
    {
        Ucs_Rm_Route_t *route = &routes[i];
        PrintEndpoint(route->source_endpoint_ptr, true, i);
        PrintEndpoint(route->sink_endpoint_ptr, false, i);
    }
    /* Iterate all routes for each route */
    ConsolePrintfStart(PRIO_HIGH, "Ucs_Rm_Route_t %sAllRoutes[] = { {\n"TAB, m.prefix);
    for (i = 0; i < len; i++)
    {
        Ucs_Rm_Route_t *route = &routes[i];
        if(i) ConsolePrintfContinue(", {\n"TAB);
        ConsolePrintfContinue(TAB C99(".source_endpoint_ptr = ")"&%s,\n"TAB, GetNameFromTable(route->source_endpoint_ptr));
        ConsolePrintfContinue(TAB C99(".sink_endpoint_ptr = ")"&%s,\n"TAB, GetNameFromTable(route->sink_endpoint_ptr));
        ConsolePrintfContinue(TAB C99(".active = ")"%u,\n"TAB, route->active);
        ConsolePrintfContinue(TAB C99(".route_id = ")"0x%04X", route->route_id);
        ConsolePrintfContinue("\n"TAB"}");
    }
    ConsolePrintfExit(" };\n");
}
