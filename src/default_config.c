/*------------------------------------------------------------------------------------------------*/
/* UNICENS Generated Network Configuration                                                        */
/* Generator: xml2struct for Linux V4.3.0                                                         */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_api.h"

uint16_t PacketBandwidth = 20;
uint16_t RoutesSize = 6;
uint16_t NodeSize = 7;

/* Route 1 from source-node=0x200 to sink-node=0x270 */
Ucs_Xrm_DefaultCreatedPort_t SrcOfRoute1_DcPort = { 
    .resource_type = UCS_XRM_RC_TYPE_DC_PORT,
    .port_type = UCS_XRM_PORT_TYPE_USB,
    .index = 0 };
Ucs_Xrm_UsbSocket_t SrcOfRoute1_UsbSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SrcOfRoute1_DcPort,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x01,
    .frames_per_transfer = 42 };
Ucs_Xrm_Splitter_t SrcOfRoute1_Splitter = { 
    .resource_type = UCS_XRM_RC_TYPE_SPLITTER,
    .socket_in_obj_ptr = &SrcOfRoute1_UsbSocket,
    .nw_port_handle = 0x0D00,
    .bytes_per_frame = 12 };
Ucs_Xrm_NetworkSocket_t SrcOfRoute1_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute1_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute1_Splitter,
    .socket_out_obj_ptr = &SrcOfRoute1_NetworkSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SrcOfRoute1_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute1_NetworkSocket,
    &SrcOfRoute1_SyncCon,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute1_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute1_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute1_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute1_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute1_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute1_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute1_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute1_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute1_JobList[] = {
    &SnkOfRoute1_NetworkSocket,
    &SnkOfRoute1_StrmPort0,
    &SnkOfRoute1_StrmPort1,
    &SnkOfRoute1_StrmSocket,
    &SnkOfRoute1_SyncCon,
    NULL };
/* Route 2 from source-node=0x200 to sink-node=0x240 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute2_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute2_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute2_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute2_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute2_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute2_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute2_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute2_JobList[] = {
    &SnkOfRoute2_NetworkSocket,
    &SnkOfRoute2_StrmPort0,
    &SnkOfRoute2_StrmPort1,
    &SnkOfRoute2_StrmSocket,
    &SnkOfRoute2_SyncCon,
    NULL };
/* Route 3 from source-node=0x200 to sink-node=0x271 */
Ucs_Xrm_NetworkSocket_t SrcOfRoute3_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute3_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute1_Splitter,
    .socket_out_obj_ptr = &SrcOfRoute3_NetworkSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 4 };
Ucs_Xrm_ResObject_t *SrcOfRoute3_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute3_NetworkSocket,
    &SrcOfRoute3_SyncCon,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute3_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute3_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute3_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute3_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute3_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute3_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute3_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute3_JobList[] = {
    &SnkOfRoute3_NetworkSocket,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute3_StrmSocket,
    &SnkOfRoute3_SyncCon,
    NULL };
/* Route 4 from source-node=0x200 to sink-node=0x241 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute4_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute4_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute4_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute4_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute4_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute4_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute4_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute4_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute4_JobList[] = {
    &SnkOfRoute4_NetworkSocket,
    &SnkOfRoute4_StrmPort0,
    &SnkOfRoute4_StrmPort1,
    &SnkOfRoute4_StrmSocket,
    &SnkOfRoute4_SyncCon,
    NULL };
/* Route 5 from source-node=0x200 to sink-node=0x272 */
Ucs_Xrm_NetworkSocket_t SrcOfRoute5_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute5_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute1_Splitter,
    .socket_out_obj_ptr = &SrcOfRoute5_NetworkSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 8 };
Ucs_Xrm_ResObject_t *SrcOfRoute5_JobList[] = {
    &SrcOfRoute1_DcPort,
    &SrcOfRoute1_UsbSocket,
    &SrcOfRoute1_Splitter,
    &SrcOfRoute5_NetworkSocket,
    &SrcOfRoute5_SyncCon,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute5_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute5_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute5_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute5_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute5_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute5_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute5_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute5_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute5_JobList[] = {
    &SnkOfRoute5_NetworkSocket,
    &SnkOfRoute5_StrmPort0,
    &SnkOfRoute5_StrmPort1,
    &SnkOfRoute5_StrmSocket,
    &SnkOfRoute5_SyncCon,
    NULL };
/* Route 6 from source-node=0x200 to sink-node=0x242 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute6_NetworkSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmPort_t SnkOfRoute6_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SnkOfRoute6_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SnkOfRoute6_StrmSocket = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute6_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute6_SyncCon = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute6_NetworkSocket,
    .socket_out_obj_ptr = &SnkOfRoute6_StrmSocket,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute6_JobList[] = {
    &SnkOfRoute6_NetworkSocket,
    &SnkOfRoute6_StrmPort0,
    &SnkOfRoute6_StrmPort1,
    &SnkOfRoute6_StrmSocket,
    &SnkOfRoute6_SyncCon,
    NULL };
UCS_NS_CONST uint8_t PayloadRequest1ForNode270[] = {
    0x00, 0x00, 0x01, 0x01 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request1ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C1,
    .op_type = 0x02,
    .data_size = 0x04,
    .data_ptr = PayloadRequest1ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response1ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C1,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest2ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x1B, 0x80 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request2ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest2ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response2ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest3ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x11, 0xB8 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request3ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest3ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response3ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest4ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x12, 0x60 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request4ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest4ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response4ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest5ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x13, 0xA0 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request5ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest5ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response5ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest6ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x14, 0x48 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request6ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest6ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response6ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest7ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x05, 0x00, 0x64, 0x20, 0x00, 0x89, 0x77, 0x72 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request7ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0D,
    .data_ptr = PayloadRequest7ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response7ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest8ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x06, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request8ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest8ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response8ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest9ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x02, 0x00, 0x64, 0x05, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request9ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0A,
    .data_ptr = PayloadRequest9ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response9ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest10ForNode270[] = {
    0x0F, 0x00, 0x00, 0x00, 0x2A, 0x03, 0x00, 0x64, 0x07, 0x01, 0x50 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request10ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x0B,
    .data_ptr = PayloadRequest10ForNode270 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response10ForNode270 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST Ucs_Ns_Script_t ScriptsForNode270[] = {
    {
        .pause = 0,
        .send_cmd = &Request1ForNode270,
        .exp_result = &Response1ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request2ForNode270,
        .exp_result = &Response2ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request3ForNode270,
        .exp_result = &Response3ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request4ForNode270,
        .exp_result = &Response4ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request5ForNode270,
        .exp_result = &Response5ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request6ForNode270,
        .exp_result = &Response6ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request7ForNode270,
        .exp_result = &Response7ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request8ForNode270,
        .exp_result = &Response8ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request9ForNode270,
        .exp_result = &Response9ForNode270
    }, {
        .pause = 0,
        .send_cmd = &Request10ForNode270,
        .exp_result = &Response10ForNode270
    } };
UCS_NS_CONST uint8_t PayloadRequest1ForNode240[] = {
    0x00, 0x00, 0x01, 0x01 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request1ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C1,
    .op_type = 0x02,
    .data_size = 0x04,
    .data_ptr = PayloadRequest1ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response1ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C1,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest2ForNode240[] = {
    0x0F, 0x00, 0x02, 0x0A, 0x18, 0x03, 0x00, 0x64, 0x00, 0x0F, 0x02, 0x01, 0x00, 0x00, 0x02, 0xA5, 0xDF, 0x03, 0x3F, 0x3F, 0x04, 0x02, 0x02, 0x10, 0x30, 0x30, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00, 0x13, 0x00, 0x00, 0x14, 0x00, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request2ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x26,
    .data_ptr = PayloadRequest2ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response2ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest3ForNode240[] = {
    0x0F, 0x00, 0x02, 0x04, 0x18, 0x03, 0x00, 0x64, 0x20, 0x00, 0x00, 0x21, 0x00, 0x00, 0x22, 0x00, 0x00, 0x23, 0x00, 0x00 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request3ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x02,
    .data_size = 0x14,
    .data_ptr = PayloadRequest3ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response3ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x06C4,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST Ucs_Ns_Script_t ScriptsForNode240[] = {
    {
        .pause = 0,
        .send_cmd = &Request1ForNode240,
        .exp_result = &Response1ForNode240
    }, {
        .pause = 0,
        .send_cmd = &Request2ForNode240,
        .exp_result = &Response2ForNode240
    }, {
        .pause = 0,
        .send_cmd = &Request3ForNode240,
        .exp_result = &Response3ForNode240
    } };
Ucs_Signature_t SignatureForNode200 = { .node_address = 0x200 };
Ucs_Signature_t SignatureForNode270 = { .node_address = 0x270 };
Ucs_Signature_t SignatureForNode271 = { .node_address = 0x271 };
Ucs_Signature_t SignatureForNode272 = { .node_address = 0x272 };
Ucs_Signature_t SignatureForNode240 = { .node_address = 0x240 };
Ucs_Signature_t SignatureForNode241 = { .node_address = 0x241 };
Ucs_Signature_t SignatureForNode242 = { .node_address = 0x242 };
Ucs_Rm_Node_t AllNodes[] = {
    {
        .signature_ptr = &SignatureForNode200,
        .init_script_list_ptr = NULL,
        .init_script_list_size = 0
    }, {
        .signature_ptr = &SignatureForNode270,
        .init_script_list_ptr = ScriptsForNode270,
        .init_script_list_size = 10
    }, {
        .signature_ptr = &SignatureForNode271,
        .init_script_list_ptr = ScriptsForNode270,
        .init_script_list_size = 10
    }, {
        .signature_ptr = &SignatureForNode272,
        .init_script_list_ptr = ScriptsForNode270,
        .init_script_list_size = 10
    }, {
        .signature_ptr = &SignatureForNode240,
        .init_script_list_ptr = ScriptsForNode240,
        .init_script_list_size = 3
    }, {
        .signature_ptr = &SignatureForNode241,
        .init_script_list_ptr = ScriptsForNode240,
        .init_script_list_size = 3
    }, {
        .signature_ptr = &SignatureForNode242,
        .init_script_list_ptr = ScriptsForNode240,
        .init_script_list_size = 3
    } };
Ucs_Rm_EndPoint_t SourceEndpointForRoute1 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute1_JobList,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute1 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute1_JobList,
    .node_obj_ptr = &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute2 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute2_JobList,
    .node_obj_ptr = &AllNodes[4] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute3 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute3_JobList,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute3 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute3_JobList,
    .node_obj_ptr = &AllNodes[2] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute4 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute4_JobList,
    .node_obj_ptr = &AllNodes[5] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute5 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute5_JobList,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute5 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute5_JobList,
    .node_obj_ptr = &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute6 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute6_JobList,
    .node_obj_ptr = &AllNodes[6] };
Ucs_Rm_Route_t AllRoutes[] = { {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute1,
        .active = 1,
        .route_id = 0x8000
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute2,
        .active = 1,
        .route_id = 0x8001
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute3,
        .sink_endpoint_ptr = &SinkEndpointForRoute3,
        .active = 1,
        .route_id = 0x8002
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute3,
        .sink_endpoint_ptr = &SinkEndpointForRoute4,
        .active = 1,
        .route_id = 0x8003
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute5,
        .active = 1,
        .route_id = 0x8004
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute6,
        .active = 1,
        .route_id = 0x8005
    } };
