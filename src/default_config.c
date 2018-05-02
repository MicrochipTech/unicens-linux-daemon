/*------------------------------------------------------------------------------------------------*/
/* UNICENS Generated Network Configuration                                                        */
/* Generator: xml2struct for Linux V4.0.4                                                         */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_api.h"

uint16_t PacketBandwidth = 20;
uint16_t RoutesSize = 6;
uint16_t NodeSize = 7;

/* Route 1 from source-node=0x200 to sink-node=0x270 */
Ucs_Xrm_DefaultCreatedPort_t SrcOfRoute1_DcPort = { 
    UCS_XRM_RC_TYPE_DC_PORT,
    UCS_XRM_PORT_TYPE_USB,
    0 };
Ucs_Xrm_UsbSocket_t SrcOfRoute1_UsbSocket = { 
    UCS_XRM_RC_TYPE_USB_SOCKET,
    &SrcOfRoute1_DcPort,
    UCS_SOCKET_DIR_INPUT,
    UCS_USB_SCKT_SYNC_DATA,
    0x01,
    42 };
Ucs_Xrm_Splitter_t SrcOfRoute1_Splitter = { 
    UCS_XRM_RC_TYPE_SPLITTER,
    &SrcOfRoute1_UsbSocket,
    0x0D00,
    12 };
Ucs_Xrm_MostSocket_t SrcOfRoute1_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_SyncCon_t SrcOfRoute1_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute1_NetworkSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SrcOfRoute1_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute1_NetworkSocket,
    &SrcOfRoute1_SyncCon,
    NULL };
Ucs_Xrm_MostSocket_t SnkOfRoute1_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute1_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute1_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute1_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute1_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute1_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute1_NetworkSocket,
    &SnkOfRoute1_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute1_JobList[] = {
    &SnkOfRoute1_NetworkSocket,
    &SnkOfRoute1_StrmPort0,
    &SnkOfRoute1_StrmPort1,
    &SnkOfRoute1_StrmSocket,
    &SnkOfRoute1_SyncCon,
    NULL };
/* Route 2 from source-node=0x200 to sink-node=0x240 */
Ucs_Xrm_MostSocket_t SnkOfRoute2_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute2_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute2_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute2_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute2_NetworkSocket,
    &SnkOfRoute2_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute2_JobList[] = {
    &SnkOfRoute2_NetworkSocket,
    &SnkOfRoute2_StrmPort0,
    &SnkOfRoute2_StrmPort1,
    &SnkOfRoute2_StrmSocket,
    &SnkOfRoute2_SyncCon,
    NULL };
/* Route 3 from source-node=0x200 to sink-node=0x271 */
Ucs_Xrm_MostSocket_t SrcOfRoute3_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_SyncCon_t SrcOfRoute3_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute3_NetworkSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    4 };
Ucs_Xrm_ResObject_t *SrcOfRoute3_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute3_NetworkSocket,
    &SrcOfRoute3_SyncCon,
    NULL };
Ucs_Xrm_MostSocket_t SnkOfRoute3_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute3_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute3_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute3_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute3_NetworkSocket,
    &SnkOfRoute3_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute3_JobList[] = {
    &SnkOfRoute3_NetworkSocket,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute3_StrmSocket,
    &SnkOfRoute3_SyncCon,
    NULL };
/* Route 4 from source-node=0x200 to sink-node=0x241 */
Ucs_Xrm_MostSocket_t SnkOfRoute4_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute4_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute4_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute4_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute4_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute4_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute4_NetworkSocket,
    &SnkOfRoute4_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute4_JobList[] = {
    &SnkOfRoute4_NetworkSocket,
    &SnkOfRoute4_StrmPort0,
    &SnkOfRoute4_StrmPort1,
    &SnkOfRoute4_StrmSocket,
    &SnkOfRoute4_SyncCon,
    NULL };
/* Route 5 from source-node=0x200 to sink-node=0x272 */
Ucs_Xrm_MostSocket_t SrcOfRoute5_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_SyncCon_t SrcOfRoute5_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute5_NetworkSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    8 };
Ucs_Xrm_ResObject_t *SrcOfRoute5_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute5_NetworkSocket,
    &SrcOfRoute5_SyncCon,
    NULL };
Ucs_Xrm_MostSocket_t SnkOfRoute5_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute5_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute5_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute5_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute5_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute5_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute5_NetworkSocket,
    &SnkOfRoute5_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute5_JobList[] = {
    &SnkOfRoute5_NetworkSocket,
    &SnkOfRoute5_StrmPort0,
    &SnkOfRoute5_StrmPort1,
    &SnkOfRoute5_StrmSocket,
    &SnkOfRoute5_SyncCon,
    NULL };
/* Route 6 from source-node=0x200 to sink-node=0x242 */
Ucs_Xrm_MostSocket_t SnkOfRoute6_NetworkSocket = { 
    UCS_XRM_RC_TYPE_MOST_SOCKET,
    0x0D00,
    UCS_SOCKET_DIR_INPUT,
    UCS_MOST_SCKT_SYNC_DATA,
    4 };
Ucs_Xrm_StrmPort_t SnkOfRoute6_StrmPort0 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    0,
    UCS_STREAM_PORT_CLK_CFG_64FS,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute6_StrmPort1 = { 
    UCS_XRM_RC_TYPE_STRM_PORT,
    1,
    UCS_STREAM_PORT_CLK_CFG_WILD,
    UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute6_StrmSocket = { 
    UCS_XRM_RC_TYPE_STRM_SOCKET,
    &SnkOfRoute6_StrmPort0,
    UCS_SOCKET_DIR_OUTPUT,
    UCS_STREAM_PORT_SCKT_SYNC_DATA,
    4,
    UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute6_SyncCon = { 
    UCS_XRM_RC_TYPE_SYNC_CON,
    &SnkOfRoute6_NetworkSocket,
    &SnkOfRoute6_StrmSocket,
    UCS_SYNC_MUTE_MODE_NO_MUTING,
    0 };
Ucs_Xrm_ResObject_t *SnkOfRoute6_JobList[] = {
    &SnkOfRoute6_NetworkSocket,
    &SnkOfRoute6_StrmPort0,
    &SnkOfRoute6_StrmPort1,
    &SnkOfRoute6_StrmSocket,
    &SnkOfRoute6_SyncCon,
    NULL };
uint8_t PayloadRequest1ForNode270[] = {
    0x00, 0x00, 0x01, 0x01 };
Ucs_Ns_ConfigMsg_t Request1ForNode270 = {
    0x00,
    0x01,
    0x06C1,
    0x02,
    0x04,
    PayloadRequest1ForNode270 };
uint8_t PayloadResponse1ForNode270[] = {
    0x0F, 0x00 };
Ucs_Ns_ConfigMsg_t Response1ForNode270 = {
    0x00,
    0x01,
    0x06C1,
    0x0C,
    0x02,
    PayloadResponse1ForNode270 };
uint8_t PayloadRequest2ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x1B, 0x80 };
Ucs_Ns_ConfigMsg_t Request2ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest2ForNode270 };
uint8_t PayloadResponse2ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response2ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse2ForNode270 };
uint8_t PayloadRequest3ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x11, 0xB8 };
Ucs_Ns_ConfigMsg_t Request3ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest3ForNode270 };
uint8_t PayloadResponse3ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response3ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse3ForNode270 };
uint8_t PayloadRequest4ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x12, 0x60 };
Ucs_Ns_ConfigMsg_t Request4ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest4ForNode270 };
uint8_t PayloadResponse4ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response4ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse4ForNode270 };
uint8_t PayloadRequest5ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x13, 0xA0 };
Ucs_Ns_ConfigMsg_t Request5ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest5ForNode270 };
uint8_t PayloadResponse5ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response5ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse5ForNode270 };
uint8_t PayloadRequest6ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x14, 0x48 };
Ucs_Ns_ConfigMsg_t Request6ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest6ForNode270 };
uint8_t PayloadResponse6ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response6ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse6ForNode270 };
uint8_t PayloadRequest7ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x05, 0x00, 0x64, 0x20, 0x00, 0x89, 0x77, 0x72 };
Ucs_Ns_ConfigMsg_t Request7ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0D,
    PayloadRequest7ForNode270 };
uint8_t PayloadResponse7ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x05 };
Ucs_Ns_ConfigMsg_t Response7ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse7ForNode270 };
uint8_t PayloadRequest8ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x06, 0x00 };
Ucs_Ns_ConfigMsg_t Request8ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest8ForNode270 };
uint8_t PayloadResponse8ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response8ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse8ForNode270 };
uint8_t PayloadRequest9ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x05, 0x00 };
Ucs_Ns_ConfigMsg_t Request9ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0A,
    PayloadRequest9ForNode270 };
uint8_t PayloadResponse9ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x02 };
Ucs_Ns_ConfigMsg_t Response9ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse9ForNode270 };
uint8_t PayloadRequest10ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x03, 0x00, 0x64, 0x07, 0x01, 0x50 };
Ucs_Ns_ConfigMsg_t Request10ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x0B,
    PayloadRequest10ForNode270 };
uint8_t PayloadResponse10ForNode270[] = {
    0x0F, 0x00, 0x2A, 0x03 };
Ucs_Ns_ConfigMsg_t Response10ForNode270 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse10ForNode270 };
Ucs_Ns_Script_t ScriptsForNode270[] = {
    {
        0,
        &Request1ForNode270,
        &Response1ForNode270
    }, {
        0,
        &Request2ForNode270,
        &Response2ForNode270
    }, {
        0,
        &Request3ForNode270,
        &Response3ForNode270
    }, {
        0,
        &Request4ForNode270,
        &Response4ForNode270
    }, {
        0,
        &Request5ForNode270,
        &Response5ForNode270
    }, {
        0,
        &Request6ForNode270,
        &Response6ForNode270
    }, {
        0,
        &Request7ForNode270,
        &Response7ForNode270
    }, {
        0,
        &Request8ForNode270,
        &Response8ForNode270
    }, {
        0,
        &Request9ForNode270,
        &Response9ForNode270
    }, {
        0,
        &Request10ForNode270,
        &Response10ForNode270
    } };
uint8_t PayloadRequest1ForNode240[] = {
    0x00, 0x00, 0x01, 0x01 };
Ucs_Ns_ConfigMsg_t Request1ForNode240 = {
    0x00,
    0x01,
    0x06C1,
    0x02,
    0x04,
    PayloadRequest1ForNode240 };
uint8_t PayloadResponse1ForNode240[] = {
    0x0F, 0x00 };
Ucs_Ns_ConfigMsg_t Response1ForNode240 = {
    0x00,
    0x01,
    0x06C1,
    0x0C,
    0x02,
    PayloadResponse1ForNode240 };
uint8_t PayloadRequest2ForNode240[] = {
    0x0F, 0x00, 0x02, 0x0A, 0x18, 0x03, 0x00, 0x64, 0x00, 0x0F, 0x02, 0x01, 0x00, 0x00, 0x02, 0xA5, 0xDF, 0x03, 0x3F, 0x3F, 0x04, 0x02, 0x02, 0x10, 0x30, 0x30, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00, 0x13, 0x00, 0x00, 0x14, 0x00, 0x00 };
Ucs_Ns_ConfigMsg_t Request2ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x26,
    PayloadRequest2ForNode240 };
uint8_t PayloadResponse2ForNode240[] = {
    0x0F, 0x00, 0x18, 0x1E };
Ucs_Ns_ConfigMsg_t Response2ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse2ForNode240 };
uint8_t PayloadRequest3ForNode240[] = {
    0x0F, 0x00, 0x02, 0x04, 0x18, 0x03, 0x00, 0x64, 0x20, 0x00, 0x00, 0x21, 0x00, 0x00, 0x22, 0x00, 0x00, 0x23, 0x00, 0x00 };
Ucs_Ns_ConfigMsg_t Request3ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x02,
    0x14,
    PayloadRequest3ForNode240 };
uint8_t PayloadResponse3ForNode240[] = {
    0x0F, 0x00, 0x18, 0x0C };
Ucs_Ns_ConfigMsg_t Response3ForNode240 = {
    0x00,
    0x01,
    0x06C4,
    0x0C,
    0x04,
    PayloadResponse3ForNode240 };
uint8_t PayloadRequest4ForNode240[] = {
    0x00, 0x00, 0x64 };
Ucs_Ns_ConfigMsg_t Request4ForNode240 = {
    0x00,
    0x01,
    0x0701,
    0x02,
    0x03,
    PayloadRequest4ForNode240 };
uint8_t PayloadResponse4ForNode240[] = {
    0x1D, 0x00 };
Ucs_Ns_ConfigMsg_t Response4ForNode240 = {
    0x00,
    0x01,
    0x0701,
    0x0C,
    0x02,
    PayloadResponse4ForNode240 };
uint8_t PayloadRequest5ForNode240[] = {
    0x1D, 0x00, 0x03, 0x35, 0x04, 0x35, 0x05, 0x35, 0x06, 0x35, 0x07, 0x41, 0x08, 0x40 };
Ucs_Ns_ConfigMsg_t Request5ForNode240 = {
    0x00,
    0x01,
    0x0703,
    0x02,
    0x0E,
    PayloadRequest5ForNode240 };
uint8_t PayloadResponse5ForNode240[] = {
    0x1D, 0x00, 0x03, 0x35, 0x04, 0x35, 0x05, 0x35, 0x06, 0x35, 0x07, 0x41, 0x08, 0x40 };
Ucs_Ns_ConfigMsg_t Response5ForNode240 = {
    0x00,
    0x01,
    0x0703,
    0x0C,
    0x0E,
    PayloadResponse5ForNode240 };
Ucs_Ns_Script_t ScriptsForNode240[] = {
    {
        0,
        &Request1ForNode240,
        &Response1ForNode240
    }, {
        0,
        &Request2ForNode240,
        &Response2ForNode240
    }, {
        0,
        &Request3ForNode240,
        &Response3ForNode240
    }, {
        0,
        &Request4ForNode240,
        &Response4ForNode240
    }, {
        0,
        &Request5ForNode240,
        &Response5ForNode240
    } };
Ucs_Signature_t SignatureForNode200 = { 0x200 };
Ucs_Signature_t SignatureForNode270 = { 0x270 };
Ucs_Signature_t SignatureForNode271 = { 0x271 };
Ucs_Signature_t SignatureForNode272 = { 0x272 };
Ucs_Signature_t SignatureForNode240 = { 0x240 };
Ucs_Signature_t SignatureForNode241 = { 0x241 };
Ucs_Signature_t SignatureForNode242 = { 0x242 };
Ucs_Rm_Node_t AllNodes[] = {
    {
        &SignatureForNode200,
        NULL,
        0
    }, {
        &SignatureForNode270,
        ScriptsForNode270,
        10
    }, {
        &SignatureForNode271,
        ScriptsForNode270,
        10
    }, {
        &SignatureForNode272,
        ScriptsForNode270,
        10
    }, {
        &SignatureForNode240,
        ScriptsForNode240,
        5
    }, {
        &SignatureForNode241,
        ScriptsForNode240,
        5
    }, {
        &SignatureForNode242,
        ScriptsForNode240,
        5
    } };
Ucs_Rm_EndPoint_t SourceEndpointForRoute1 = {
    UCS_RM_EP_SOURCE,
    SrcOfRoute1_JobList,
    &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute1 = {
    UCS_RM_EP_SINK,
    SnkOfRoute1_JobList,
    &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute2 = {
    UCS_RM_EP_SINK,
    SnkOfRoute2_JobList,
    &AllNodes[4] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute3 = {
    UCS_RM_EP_SOURCE,
    SrcOfRoute3_JobList,
    &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute3 = {
    UCS_RM_EP_SINK,
    SnkOfRoute3_JobList,
    &AllNodes[2] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute4 = {
    UCS_RM_EP_SINK,
    SnkOfRoute4_JobList,
    &AllNodes[5] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute5 = {
    UCS_RM_EP_SOURCE,
    SrcOfRoute5_JobList,
    &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute5 = {
    UCS_RM_EP_SINK,
    SnkOfRoute5_JobList,
    &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute6 = {
    UCS_RM_EP_SINK,
    SnkOfRoute6_JobList,
    &AllNodes[6] };
Ucs_Rm_Route_t AllRoutes[] = { {
        &SourceEndpointForRoute1,
        &SinkEndpointForRoute1,
        1,
        0x8004
    }, {
        &SourceEndpointForRoute1,
        &SinkEndpointForRoute2,
        1,
        0x8007
    }, {
        &SourceEndpointForRoute3,
        &SinkEndpointForRoute3,
        1,
        0x8005
    }, {
        &SourceEndpointForRoute3,
        &SinkEndpointForRoute4,
        1,
        0x8008
    }, {
        &SourceEndpointForRoute5,
        &SinkEndpointForRoute5,
        1,
        0x8006
    }, {
        &SourceEndpointForRoute5,
        &SinkEndpointForRoute6,
        1,
        0x8009
    } };
