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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "UcsXml.h"
#include "UcsXml_Private.h"

static const char* USB_PHY_STANDARD =       "Standard";
static const char* USB_PHY_HSIC =           "HSIC";

static const char* CLOCK_8FS =              "8Fs";
static const char* CLOCK_16FS =             "16Fs";
static const char* CLOCK_32FS =             "32Fs";
static const char* CLOCK_64FS =             "64Fs";
static const char* CLOCK_128FS =            "128Fs";
static const char* CLOCK_256FS =            "256Fs";
static const char* CLOCK_512FS =            "512Fs";
static const char* CLOCK_1024FS =           "1024Fs";
static const char* CLOCK_2048FS =           "2048Fs";
static const char* CLOCK_3072FS =           "3072Fs";
static const char* CLOCK_4096FS =           "4096Fs";
static const char* CLOCK_6144FS =           "6144Fs";
static const char* CLOCK_8192FS =           "8192Fs";
static const char* CLOCK_WILDCARD =         "Wildcard";

static const char* STRM_ALIGN_L16 =         "Left16Bit";
static const char* STRM_ALIGN_L24 =         "Left24Bit";
static const char* STRM_ALIGN_R16 =         "Right16Bit";
static const char* STRM_ALIGN_R24 =         "Right24Bit";
static const char* STRM_ALIGN_SEQUENTIAL =  "Seq";

static const char* I2S_PIN_SRXA0 =          "SRXA0";
static const char* I2S_PIN_SRXA1 =          "SRXA1";
static const char* I2S_PIN_SRXB0 =          "SRXB0";
static const char* I2S_PIN_SRXB1 =          "SRXB1";

static const char* MUTE_OFF =               "NoMuting";
static const char* MUTE_SIGNAL =            "MuteSignal";
/*
static const char* VAL_TRUE =               "true";
static const char* VAL_FALSE =              "false";
 */

#define ASSERT_FALSE(func, par) { UcsXml_CB_OnError("Parameter error in attribute=%s value=%s, file=%s, line=%d", 4, func, par,  __FILE__, __LINE__); return false; }
#define CHECK_POINTER(PTR) if (NULL == PTR) { ASSERT_FALSE(PTR, "NULL pointer"); }

static int32_t Str2Int(const char *val)
{
    return strtol( val, NULL, 0 );
}

void *MCalloc(struct UcsXmlObjectList *list, uint32_t nElem, uint32_t elemSize)
{
    void *obj;
    struct UcsXmlObjectList *tail = list;
    if (NULL == list || 0 == nElem || 0 == elemSize) return NULL;

    obj = calloc(nElem, elemSize);
    if (NULL == obj)
    {
        assert(false);
        return NULL;
    }
    if (NULL == list->obj)
    {
        list->obj = obj;
        return obj;
    }
    while(tail->next) tail = tail->next;
    tail->next = calloc(1, sizeof(struct UcsXmlObjectList));
    if (NULL == tail->next)
    {
        assert(false);
        free(obj);
        return NULL;
    }
    tail->next->obj = obj;
    return obj;
}

void FreeObjList(struct UcsXmlObjectList *cur)
{
    struct UcsXmlObjectList *root = cur;
    while(cur)
    {
        struct UcsXmlObjectList *next = cur->next;
        assert(NULL != cur->obj);
        if (cur->obj)
            free(cur->obj);
        if (cur != root)
            free(cur);
        cur = next;
    }
}

bool GetMostSocket(Ucs_Xrm_MostSocket_t **mostSoc, struct MostSocketParameters *param)
{
    Ucs_Xrm_MostSocket_t *soc = NULL;
    CHECK_POINTER(mostSoc);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    soc = MCalloc(param->list, 1, sizeof(Ucs_Xrm_MostSocket_t));
    CHECK_POINTER(soc);
    *mostSoc = soc;
    soc->resource_type = UCS_XRM_RC_TYPE_MOST_SOCKET;
    soc->most_port_handle = 0x0D00;
    soc->bandwidth = param->bandwidth;
    soc->direction = param->isSource ? UCS_SOCKET_DIR_INPUT : UCS_SOCKET_DIR_OUTPUT;
    switch(param->dataType)
    {
    case SYNC_DATA:
        soc->data_type = UCS_MOST_SCKT_SYNC_DATA;
        break;
    case AV_PACKETIZED:
        soc->data_type = UCS_MOST_SCKT_AV_PACKETIZED;
        break;
    case QOS_IP:
        soc->data_type = UCS_MOST_SCKT_QOS_IP;
        break;
    case DISC_FRAME_PHASE:
        soc->data_type = UCS_MOST_SCKT_DISC_FRAME_PHASE;
        break;
    default:
        ASSERT_FALSE("GetMostSocket->dataType", "");
    }
    return true;
}

bool GetUsbPort(Ucs_Xrm_UsbPort_t **usbPort, struct UsbPortParameters *param)
{
    Ucs_Xrm_UsbPort_t *port = NULL;
    CHECK_POINTER(usbPort);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->deviceInterfaces);
    CHECK_POINTER(param->streamInCount);
    CHECK_POINTER(param->streamOutCount);
    CHECK_POINTER(param->physicalLayer);
    port = MCalloc(param->list, 1, sizeof(Ucs_Xrm_UsbPort_t));
    CHECK_POINTER(port);
    *usbPort = port;
    port->resource_type = UCS_XRM_RC_TYPE_USB_PORT;
    port->index = 0;
    port->devices_interfaces = (uint16_t)Str2Int(param->deviceInterfaces);
    port->streaming_if_ep_in_count = (uint8_t)Str2Int(param->streamInCount);
    port->streaming_if_ep_out_count = (uint8_t)Str2Int(param->streamOutCount);
    if (0 == strcmp(USB_PHY_STANDARD, param->physicalLayer))
        port->physical_layer = UCS_USB_PHY_LAYER_STANDARD;
    else if (0 == strcmp(USB_PHY_HSIC, param->physicalLayer))
        port->physical_layer = UCS_USB_PHY_LAYER_HSCI;
    else ASSERT_FALSE("GetUsbPort->physical_layer", param->physicalLayer);
    return true;
}

bool GetUsbPortDefaultCreated(Ucs_Xrm_ResObject_t **usbPort, struct UcsXmlObjectList *list)
{
    Ucs_Xrm_DefaultCreatedPort_t *p;
    CHECK_POINTER(usbPort);
    CHECK_POINTER(list);
    p = MCalloc(list, 1, sizeof(Ucs_Xrm_DefaultCreatedPort_t));
    CHECK_POINTER(p);
    p->resource_type = UCS_XRM_RC_TYPE_DC_PORT;
    p->port_type = UCS_XRM_PORT_TYPE_USB;
    p->index = 0;
    *usbPort = (Ucs_Xrm_ResObject_t *)p;
    return true;
}

bool GetUsbSocket(Ucs_Xrm_UsbSocket_t **usbSoc, struct UsbSocketParameters *param)
{
    Ucs_Xrm_UsbSocket_t *soc = NULL;
    CHECK_POINTER(usbSoc);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->endpointAddress);
    CHECK_POINTER(param->framesPerTrans);
    CHECK_POINTER(param->usbPort);
    soc = MCalloc(param->list, 1, sizeof(Ucs_Xrm_UsbSocket_t));
    CHECK_POINTER(soc);
    *usbSoc = soc;
    soc->resource_type = UCS_XRM_RC_TYPE_USB_SOCKET;
    soc->direction = param->isSource ? UCS_SOCKET_DIR_INPUT : UCS_SOCKET_DIR_OUTPUT;
    switch(param->dataType)
    {
    case SYNC_DATA:
        soc->data_type = UCS_USB_SCKT_SYNC_DATA;
        break;
    case AV_PACKETIZED:
        soc->data_type = UCS_USB_SCKT_AV_PACKETIZED;
        break;
    case IPC_PACKET:
        soc->data_type = UCS_USB_SCKT_IPC_PACKET;
        break;
    default:
        ASSERT_FALSE("GetUsbSocket->dataType", "");
    }
    soc->end_point_addr = (uint8_t)Str2Int(param->endpointAddress);
    soc->frames_per_transfer = (uint16_t)Str2Int(param->framesPerTrans);
    soc->usb_port_obj_ptr = param->usbPort;
    return true;
}

bool GetMlbPort(Ucs_Xrm_MlbPort_t **mlbPort, struct MlbPortParameters *param)
{
    Ucs_Xrm_MlbPort_t *port = NULL;
    CHECK_POINTER(mlbPort);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->clockConfig);
    port = MCalloc(param->list, 1, sizeof(Ucs_Xrm_MlbPort_t));
    CHECK_POINTER(port);
    *mlbPort = port;
    port->resource_type = UCS_XRM_RC_TYPE_MLB_PORT;
    port->index = 0;
    if (0 == strcmp(param->clockConfig, CLOCK_256FS))
        port->clock_config = UCS_MLB_CLK_CFG_256_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_512FS))
        port->clock_config = UCS_MLB_CLK_CFG_512_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_1024FS))
        port->clock_config = UCS_MLB_CLK_CFG_1024_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_2048FS))
        port->clock_config = UCS_MLB_CLK_CFG_2048_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_3072FS))
        port->clock_config = UCS_MLB_CLK_CFG_3072_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_4096FS))
        port->clock_config = UCS_MLB_CLK_CFG_4096_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_6144FS))
        port->clock_config = UCS_MLB_CLK_CFG_6144_FS;
    else if (0 == strcmp(param->clockConfig, CLOCK_8192FS))
        port->clock_config = UCS_MLB_CLK_CFG_8192_FS;
    else ASSERT_FALSE("GetMlbPort->clockConfig", param->clockConfig);
    return true;
}

bool GetMlbPortDefaultCreated(Ucs_Xrm_ResObject_t **mlbPort, struct UcsXmlObjectList *list)
{
    Ucs_Xrm_DefaultCreatedPort_t *p;
    CHECK_POINTER(mlbPort);
    CHECK_POINTER(list)
    p = MCalloc(list, 1, sizeof(Ucs_Xrm_DefaultCreatedPort_t));
    CHECK_POINTER(p);
    p->resource_type = UCS_XRM_RC_TYPE_DC_PORT;
    p->port_type = UCS_XRM_PORT_TYPE_MLB;
    p->index = 0;
    *mlbPort = (Ucs_Xrm_ResObject_t *)p;
    return true;
}

bool GetMlbSocket(Ucs_Xrm_MlbSocket_t **mlbSoc, struct MlbSocketParameters *param)
{
    Ucs_Xrm_MlbSocket_t *soc = NULL;
    CHECK_POINTER(mlbSoc);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->channelAddress);
    CHECK_POINTER(param->mlbPort);
    soc = MCalloc(param->list, 1, sizeof(Ucs_Xrm_MlbSocket_t));
    CHECK_POINTER(soc);
    *mlbSoc = soc;
    soc->resource_type = UCS_XRM_RC_TYPE_MLB_SOCKET;
    soc->direction = param->isSource ? UCS_SOCKET_DIR_INPUT : UCS_SOCKET_DIR_OUTPUT;
    soc->bandwidth = param->bandwidth;
    switch(param->dataType)
    {
    case SYNC_DATA:
        soc->data_type = UCS_MLB_SCKT_SYNC_DATA;
        break;
    case AV_PACKETIZED:
        soc->data_type = UCS_MLB_SCKT_AV_PACKETIZED;
        break;
    case QOS_IP:
        soc->data_type = UCS_MLB_SCKT_QOS_IP;
        break;
    case DISC_FRAME_PHASE:
        soc->data_type = UCS_MLB_SCKT_DISC_FRAME_PHASE;
        break;
    case IPC_PACKET:
        soc->data_type = UCS_MLB_SCKT_IPC_PACKET;
        break;
    default:
        ASSERT_FALSE("GetMlbSocket->dataType", "");
    }
    soc->channel_address = (uint16_t)Str2Int(param->channelAddress);
    soc->mlb_port_obj_ptr = param->mlbPort;
    return true;
}

bool GetStrmPort(Ucs_Xrm_StrmPort_t **strmPort, struct StrmPortParameters *param)
{
    Ucs_Xrm_StrmPort_t *port = NULL;
    CHECK_POINTER(strmPort);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->clockConfig);
    port = MCalloc(param->list, 1, sizeof(Ucs_Xrm_StrmPort_t));
    CHECK_POINTER(port);
    *strmPort = port;
    port->resource_type = UCS_XRM_RC_TYPE_STRM_PORT;
    port->index = param->index;
    if (0 == port->index)
    {
        if (0 == strcmp(param->clockConfig, CLOCK_8FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_8FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_16FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_16FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_32FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_32FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_64FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_64FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_128FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_128FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_256FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_256FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_512FS))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_512FS;
        else if (0 == strcmp(param->clockConfig, CLOCK_WILDCARD))
            port->clock_config = UCS_STREAM_PORT_CLK_CFG_WILD;
        else ASSERT_FALSE("GetStrmPort->clockConfig", param->clockConfig);
    } else {
        port->clock_config = UCS_STREAM_PORT_CLK_CFG_WILD;
    }

    if (0 == strcmp(param->dataAlignment, STRM_ALIGN_L16))
        port->data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT;
    else if (0 == strcmp(param->dataAlignment, STRM_ALIGN_L24))
        port->data_alignment = UCS_STREAM_PORT_ALGN_LEFT24BIT;
    else if (0 == strcmp(param->dataAlignment, STRM_ALIGN_R16))
        port->data_alignment = UCS_STREAM_PORT_ALGN_RIGHT16BIT;
    else if (0 == strcmp(param->dataAlignment, STRM_ALIGN_R24))
        port->data_alignment = UCS_STREAM_PORT_ALGN_RIGHT24BIT;
    else if (0 == strcmp(param->dataAlignment, STRM_ALIGN_SEQUENTIAL))
        port->data_alignment = UCS_STREAM_PORT_ALGN_SEQ;
    else ASSERT_FALSE("GetStrmPort->dataAlignment", param->dataAlignment);
    return true;
}

bool GetStrmSocket(Ucs_Xrm_StrmSocket_t **strmSoc, struct StrmSocketParameters *param)
{
    Ucs_Xrm_StrmSocket_t *soc = NULL;
    CHECK_POINTER(strmSoc);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->streamPin);
    CHECK_POINTER(param->streamPortA);
    CHECK_POINTER(param->streamPortB);
    soc = MCalloc(param->list, 1, sizeof(Ucs_Xrm_StrmSocket_t));
    CHECK_POINTER(soc);
    *strmSoc = soc;
    soc->resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET;
    soc->direction = param->isSource ? UCS_SOCKET_DIR_INPUT : UCS_SOCKET_DIR_OUTPUT;
    switch(param->dataType)
    {
    case SYNC_DATA:
        soc->data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA;
        break;
    default:
        ASSERT_FALSE("GetStrmSocket->dataType", "");
    }
    soc->bandwidth = param->bandwidth;
    if (0 == strcmp(param->streamPin, I2S_PIN_SRXA0))
    {
        soc->stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0;
        soc->stream_port_obj_ptr = param->streamPortA;
        return true;
    }
    else if (0 == strcmp(param->streamPin, I2S_PIN_SRXA1))
    {
        soc->stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1;
        soc->stream_port_obj_ptr = param->streamPortA;
        return true;
    }
    else if (0 == strcmp(param->streamPin, I2S_PIN_SRXB0))
    {
        soc->stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXB0;
        soc->stream_port_obj_ptr = param->streamPortB;
        return true;
    }
    else if (0 == strcmp(param->streamPin, I2S_PIN_SRXB1))
    {
        soc->stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXB1;
        soc->stream_port_obj_ptr = param->streamPortB;
        return true;
    }
    else ASSERT_FALSE("GetStrmSocket->streamPin", param->streamPin);
    return true;
}

bool GetSplitter(Ucs_Xrm_Splitter_t **splitter, struct SplitterParameters *param)
{
    Ucs_Xrm_Splitter_t *split = NULL;
    CHECK_POINTER(splitter);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    split = MCalloc(param->list, 1, sizeof(Ucs_Xrm_Splitter_t));
    CHECK_POINTER(split);
    *splitter = split;
    split->most_port_handle = 0x0D00;
    split->resource_type = UCS_XRM_RC_TYPE_SPLITTER;
    split->bytes_per_frame = param->bytesPerFrame;
    split->socket_in_obj_ptr = param->inSoc;
    return true;
}

bool GetCombiner(Ucs_Xrm_Combiner_t **combiner, struct CombinerParameters *param)
{
    Ucs_Xrm_Combiner_t *comb = NULL;
    CHECK_POINTER(combiner);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    comb = MCalloc(param->list, 1, sizeof(Ucs_Xrm_Combiner_t));
    CHECK_POINTER(comb);
    *combiner = comb;
    comb->most_port_handle = 0x0D00;
    comb->resource_type = UCS_XRM_RC_TYPE_COMBINER;
    comb->bytes_per_frame = param->bytesPerFrame;
    comb->port_socket_obj_ptr = param->outSoc;
    return true;
}

bool GetSyncCon(Ucs_Xrm_SyncCon_t **syncCon, struct SyncConParameters *param)
{
    Ucs_Xrm_SyncCon_t *con = NULL;
    CHECK_POINTER(syncCon);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->muteMode);
    CHECK_POINTER(param->inSoc);
    CHECK_POINTER(param->outSoc);
    con = MCalloc(param->list, 1, sizeof(Ucs_Xrm_SyncCon_t));
    CHECK_POINTER(con);
    *syncCon = con;
    con->resource_type = UCS_XRM_RC_TYPE_SYNC_CON;
    if (0 == strcmp(param->muteMode, MUTE_OFF))
        con->mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING;
    else if (0 == strcmp(param->muteMode, MUTE_SIGNAL))
        con->mute_mode = UCS_SYNC_MUTE_MODE_MUTE_SIGNAL;
    else ASSERT_FALSE("GetSyncCon->mute_mode", param->muteMode);
    if (param->optional_offset)
        con->offset = (uint16_t)Str2Int(param->optional_offset);
    else
        con->offset = 0;
    con->socket_in_obj_ptr = param->inSoc;
    con->socket_out_obj_ptr = param->outSoc;
    return true;
}

bool GetAvpCon(Ucs_Xrm_AvpCon_t **avpCon, struct AvpConParameters *param)
{
    Ucs_Xrm_AvpCon_t *con = NULL;
    CHECK_POINTER(avpCon);
    CHECK_POINTER(param);
    CHECK_POINTER(param->list);
    CHECK_POINTER(param->inSoc);
    CHECK_POINTER(param->outSoc);
    con = MCalloc(param->list, 1, sizeof(Ucs_Xrm_AvpCon_t));
    CHECK_POINTER(con);
    *avpCon = con;
    con->resource_type = UCS_XRM_RC_TYPE_AVP_CON;
    con->socket_in_obj_ptr = param->inSoc;
    con->socket_out_obj_ptr = param->outSoc;
    if (param->optional_isocPacketSize)
    {
        int32_t pSize = Str2Int(param->optional_isocPacketSize);
        switch(pSize)
        {
        case 188:
            con->isoc_packet_size = UCS_ISOC_PCKT_SIZE_188;
            break;
        case 196:
            con->isoc_packet_size = UCS_ISOC_PCKT_SIZE_196;
            break;
        case 206:
            con->isoc_packet_size = UCS_ISOC_PCKT_SIZE_206;
            break;
        default:
            ASSERT_FALSE("GetAvpCon->isoc_packet_size", "");
        }
    } else {
        con->isoc_packet_size = UCS_ISOC_PCKT_SIZE_188;
    }
    return true;
}