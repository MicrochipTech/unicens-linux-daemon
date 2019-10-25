/*------------------------------------------------------------------------------------------------*/
/* UNICENS Generated Network Configuration                                                        */
/* Generator: xml2struct for Linux V5.1.0                                                         */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_api.h"

uint16_t PacketBandwidth = 80;
uint16_t ProxyBandwidth = 0;
uint16_t RoutesSize = 12;
uint16_t NodeSize = 5;

/* Route 1 from source-node=0x200 to sink-node=0x200 */
Ucs_Xrm_DefaultCreatedPort_t SrcOfRoute1_DcPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_DC_PORT,
    .port_type = UCS_XRM_PORT_TYPE_USB,
    .index = 0 };
Ucs_Xrm_UsbSocket_t SrcOfRoute1_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SrcOfRoute1_DcPort0,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x01,
    .frames_per_transfer = 128 };
Ucs_Xrm_NetworkSocket_t SrcOfRoute1_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute1_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute1_UsbSocket0,
    .socket_out_obj_ptr = &SrcOfRoute1_NetworkSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SrcOfRoute1_JobList0[] = {
    &SrcOfRoute1_DcPort0,
    &SrcOfRoute1_UsbSocket0,
    &SrcOfRoute1_NetworkSocket0,
    &SrcOfRoute1_SyncCon0,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute1_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_UsbSocket_t SnkOfRoute1_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SrcOfRoute1_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x81,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute1_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute1_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute1_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute1_JobList0[] = {
    &SnkOfRoute1_NetworkSocket0,
    &SrcOfRoute1_DcPort0,
    &SnkOfRoute1_UsbSocket0,
    &SnkOfRoute1_SyncCon0,
    NULL };
/* Route 2 from source-node=0x200 to sink-node=0x2B0 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute2_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_DefaultCreatedPort_t SnkOfRoute2_DcPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_DC_PORT,
    .port_type = UCS_XRM_PORT_TYPE_USB,
    .index = 0 };
Ucs_Xrm_UsbSocket_t SnkOfRoute2_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SnkOfRoute2_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x81,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute2_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute2_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute2_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute2_JobList0[] = {
    &SnkOfRoute2_NetworkSocket0,
    &SnkOfRoute2_DcPort0,
    &SnkOfRoute2_UsbSocket0,
    &SnkOfRoute2_SyncCon0,
    NULL };
/* Route 3 from source-node=0x200 to sink-node=0x240 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute3_NetworkSocket0 = { 
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
Ucs_Xrm_StrmSocket_t SnkOfRoute3_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute3_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute3_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute3_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute3_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute3_JobList0[] = {
    &SnkOfRoute3_NetworkSocket0,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute3_StrmSocket0,
    &SnkOfRoute3_SyncCon0,
    NULL };
/* Route 4 from source-node=0x200 to sink-node=0x270 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute4_NetworkSocket0 = { 
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
Ucs_Xrm_StrmSocket_t SnkOfRoute4_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute4_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute4_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute4_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute4_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute4_JobList0[] = {
    &SnkOfRoute4_NetworkSocket0,
    &SnkOfRoute4_StrmPort0,
    &SnkOfRoute4_StrmPort1,
    &SnkOfRoute4_StrmSocket0,
    &SnkOfRoute4_SyncCon0,
    NULL };
/* Route 5 from source-node=0x210 to sink-node=0x200 */
Ucs_Xrm_StrmPort_t SrcOfRoute5_StrmPort0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 0,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_64FS,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmPort_t SrcOfRoute5_StrmPort1 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_PORT,
    .index = 1,
    .clock_config = UCS_STREAM_PORT_CLK_CFG_WILD,
    .data_alignment = UCS_STREAM_PORT_ALGN_LEFT16BIT };
Ucs_Xrm_StrmSocket_t SrcOfRoute5_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SrcOfRoute5_StrmPort0,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_NetworkSocket_t SrcOfRoute5_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute5_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute5_StrmSocket0,
    .socket_out_obj_ptr = &SrcOfRoute5_NetworkSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SrcOfRoute5_JobList0[] = {
    &SrcOfRoute5_StrmPort0,
    &SrcOfRoute5_StrmPort1,
    &SrcOfRoute5_StrmSocket0,
    &SrcOfRoute5_NetworkSocket0,
    &SrcOfRoute5_SyncCon0,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute5_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_UsbSocket_t SnkOfRoute5_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SrcOfRoute1_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x82,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute5_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute5_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute5_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute5_JobList0[] = {
    &SnkOfRoute5_NetworkSocket0,
    &SrcOfRoute1_DcPort0,
    &SnkOfRoute5_UsbSocket0,
    &SnkOfRoute5_SyncCon0,
    NULL };
/* Route 6 from source-node=0x210 to sink-node=0x2B0 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute6_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_UsbSocket_t SnkOfRoute6_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SnkOfRoute2_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x82,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute6_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute6_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute6_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute6_JobList0[] = {
    &SnkOfRoute6_NetworkSocket0,
    &SnkOfRoute2_DcPort0,
    &SnkOfRoute6_UsbSocket0,
    &SnkOfRoute6_SyncCon0,
    NULL };
/* Route 7 from source-node=0x210 to sink-node=0x240 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute7_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmSocket_t SnkOfRoute7_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute3_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute7_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute7_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute7_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute7_JobList0[] = {
    &SnkOfRoute7_NetworkSocket0,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute7_StrmSocket0,
    &SnkOfRoute7_SyncCon0,
    NULL };
/* Route 8 from source-node=0x210 to sink-node=0x270 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute8_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmSocket_t SnkOfRoute8_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute4_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute8_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute8_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute8_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute8_JobList0[] = {
    &SnkOfRoute8_NetworkSocket0,
    &SnkOfRoute4_StrmPort0,
    &SnkOfRoute4_StrmPort1,
    &SnkOfRoute8_StrmSocket0,
    &SnkOfRoute8_SyncCon0,
    NULL };
/* Route 9 from source-node=0x240 to sink-node=0x200 */
Ucs_Xrm_StrmSocket_t SrcOfRoute9_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute3_StrmPort0,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_NetworkSocket_t SrcOfRoute9_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_SyncCon_t SrcOfRoute9_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SrcOfRoute9_StrmSocket0,
    .socket_out_obj_ptr = &SrcOfRoute9_NetworkSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SrcOfRoute9_JobList0[] = {
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SrcOfRoute9_StrmSocket0,
    &SrcOfRoute9_NetworkSocket0,
    &SrcOfRoute9_SyncCon0,
    NULL };
Ucs_Xrm_NetworkSocket_t SnkOfRoute9_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_UsbSocket_t SnkOfRoute9_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SrcOfRoute1_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x83,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute9_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute9_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute9_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute9_JobList0[] = {
    &SnkOfRoute9_NetworkSocket0,
    &SrcOfRoute1_DcPort0,
    &SnkOfRoute9_UsbSocket0,
    &SnkOfRoute9_SyncCon0,
    NULL };
/* Route 10 from source-node=0x240 to sink-node=0x2B0 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute10_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_UsbSocket_t SnkOfRoute10_UsbSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_USB_SOCKET,
    .usb_port_obj_ptr = &SnkOfRoute2_DcPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_USB_SCKT_SYNC_DATA,
    .end_point_addr = 0x83,
    .frames_per_transfer = 128 };
Ucs_Xrm_SyncCon_t SnkOfRoute10_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute10_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute10_UsbSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute10_JobList0[] = {
    &SnkOfRoute10_NetworkSocket0,
    &SnkOfRoute2_DcPort0,
    &SnkOfRoute10_UsbSocket0,
    &SnkOfRoute10_SyncCon0,
    NULL };
/* Route 11 from source-node=0x240 to sink-node=0x240 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute11_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmSocket_t SnkOfRoute11_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute3_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA1 };
Ucs_Xrm_SyncCon_t SnkOfRoute11_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute11_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute11_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute11_JobList0[] = {
    &SnkOfRoute11_NetworkSocket0,
    &SnkOfRoute3_StrmPort0,
    &SnkOfRoute3_StrmPort1,
    &SnkOfRoute11_StrmSocket0,
    &SnkOfRoute11_SyncCon0,
    NULL };
/* Route 12 from source-node=0x240 to sink-node=0x270 */
Ucs_Xrm_NetworkSocket_t SnkOfRoute12_NetworkSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_NW_SOCKET,
    .nw_port_handle = 0x0D00,
    .direction = UCS_SOCKET_DIR_INPUT,
    .data_type = UCS_NW_SCKT_SYNC_DATA,
    .bandwidth = 4 };
Ucs_Xrm_StrmSocket_t SnkOfRoute12_StrmSocket0 = { 
    .resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET,
    .stream_port_obj_ptr = &SnkOfRoute4_StrmPort0,
    .direction = UCS_SOCKET_DIR_OUTPUT,
    .data_type = UCS_STREAM_PORT_SCKT_SYNC_DATA,
    .bandwidth = 4,
    .stream_pin_id = UCS_STREAM_PORT_PIN_ID_SRXA0 };
Ucs_Xrm_SyncCon_t SnkOfRoute12_SyncCon0 = { 
    .resource_type = UCS_XRM_RC_TYPE_SYNC_CON,
    .socket_in_obj_ptr = &SnkOfRoute12_NetworkSocket0,
    .socket_out_obj_ptr = &SnkOfRoute12_StrmSocket0,
    .mute_mode = UCS_SYNC_MUTE_MODE_NO_MUTING,
    .offset = 0 };
Ucs_Xrm_ResObject_t *SnkOfRoute12_JobList0[] = {
    &SnkOfRoute12_NetworkSocket0,
    &SnkOfRoute4_StrmPort0,
    &SnkOfRoute4_StrmPort1,
    &SnkOfRoute12_StrmSocket0,
    &SnkOfRoute12_SyncCon0,
    NULL };
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
    0x0F, 0x00, 0x02, 0x0A, 0x18, 0x03, 0x00, 0x64, 0x00, 0x0F, 0x02, 0x01, 0x00, 0x00, 0x02, 0xA5, 0xDF, 0x03, 0x3F, 0x3F, 0x04, 0x02, 0x02, 0x10, 0x50, 0x50, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00, 0x13, 0x00, 0x00, 0x14, 0x00, 0x00 };
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
UCS_NS_CONST uint8_t PayloadRequest4ForNode240[] = {
    0x00, 0x00, 0x14 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request4ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x0701,
    .op_type = 0x02,
    .data_size = 0x03,
    .data_ptr = PayloadRequest4ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response4ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x0701,
    .op_type = 0x0C,
    .data_size = 0x00,
    .data_ptr = NULL };
UCS_NS_CONST uint8_t PayloadRequest5ForNode240[] = {
    0x1D, 0x00, 0x03, 0x35, 0x04, 0x35, 0x05, 0x35, 0x06, 0x35, 0x07, 0x41, 0x08, 0x40 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Request5ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x0703,
    .op_type = 0x02,
    .data_size = 0x0E,
    .data_ptr = PayloadRequest5ForNode240 };
UCS_NS_CONST Ucs_Ns_ConfigMsg_t Response5ForNode240 = {
    .fblock_id = 0x00,
    .inst_id = 0x01,
    .funct_id = 0x0703,
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
    }, {
        .pause = 0,
        .send_cmd = &Request4ForNode240,
        .exp_result = &Response4ForNode240
    }, {
        .pause = 0,
        .send_cmd = &Request5ForNode240,
        .exp_result = &Response5ForNode240
    } };
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
Ucs_Signature_t SignatureForNode200 = { .node_address = 0x200 };
Ucs_Signature_t SignatureForNode2B0 = { .node_address = 0x2B0 };
Ucs_Signature_t SignatureForNode210 = { .node_address = 0x210 };
Ucs_Signature_t SignatureForNode240 = { .node_address = 0x240 };
Ucs_Signature_t SignatureForNode270 = { .node_address = 0x270 };
Ucs_Rm_Node_t AllNodes[] = {
    {
        .signature_ptr = &SignatureForNode200,
        .init_script_list_ptr = NULL,
        .init_script_list_size = 0,
        .remote_attach_disabled = 0
    }, {
        .signature_ptr = &SignatureForNode2B0,
        .init_script_list_ptr = NULL,
        .init_script_list_size = 0,
        .remote_attach_disabled = 0
    }, {
        .signature_ptr = &SignatureForNode210,
        .init_script_list_ptr = NULL,
        .init_script_list_size = 0,
        .remote_attach_disabled = 0
    }, {
        .signature_ptr = &SignatureForNode240,
        .init_script_list_ptr = ScriptsForNode240,
        .init_script_list_size = 5,
        .remote_attach_disabled = 0
    }, {
        .signature_ptr = &SignatureForNode270,
        .init_script_list_ptr = ScriptsForNode270,
        .init_script_list_size = 10,
        .remote_attach_disabled = 0
    } };
Ucs_Rm_EndPoint_t SourceEndpointForRoute1 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute1_JobList0,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute1 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute1_JobList0,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute2 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute2_JobList0,
    .node_obj_ptr = &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute3 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute3_JobList0,
    .node_obj_ptr = &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute4 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute4_JobList0,
    .node_obj_ptr = &AllNodes[4] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute5 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute5_JobList0,
    .node_obj_ptr = &AllNodes[2] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute5 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute5_JobList0,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute6 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute6_JobList0,
    .node_obj_ptr = &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute7 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute7_JobList0,
    .node_obj_ptr = &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute8 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute8_JobList0,
    .node_obj_ptr = &AllNodes[4] };
Ucs_Rm_EndPoint_t SourceEndpointForRoute9 = {
    .endpoint_type = UCS_RM_EP_SOURCE,
    .jobs_list_ptr = SrcOfRoute9_JobList0,
    .node_obj_ptr = &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute9 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute9_JobList0,
    .node_obj_ptr = &AllNodes[0] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute10 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute10_JobList0,
    .node_obj_ptr = &AllNodes[1] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute11 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute11_JobList0,
    .node_obj_ptr = &AllNodes[3] };
Ucs_Rm_EndPoint_t SinkEndpointForRoute12 = {
    .endpoint_type = UCS_RM_EP_SINK,
    .jobs_list_ptr = SnkOfRoute12_JobList0,
    .node_obj_ptr = &AllNodes[4] };
Ucs_Rm_Route_t AllRoutes[] = { {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute1,
        .active = 1,
        .route_id = 0x8000,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute2,
        .active = 1,
        .route_id = 0x8001,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute3,
        .active = 1,
        .route_id = 0x1001,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute1,
        .sink_endpoint_ptr = &SinkEndpointForRoute4,
        .active = 1,
        .route_id = 0x2001,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute5,
        .active = 1,
        .route_id = 0x8002,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute6,
        .active = 1,
        .route_id = 0x8003,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute7,
        .active = 0,
        .route_id = 0x1002,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute5,
        .sink_endpoint_ptr = &SinkEndpointForRoute8,
        .active = 0,
        .route_id = 0x2002,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute9,
        .sink_endpoint_ptr = &SinkEndpointForRoute9,
        .active = 1,
        .route_id = 0x8004,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute9,
        .sink_endpoint_ptr = &SinkEndpointForRoute10,
        .active = 1,
        .route_id = 0x8005,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute9,
        .sink_endpoint_ptr = &SinkEndpointForRoute11,
        .active = 0,
        .route_id = 0x1003,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    }, {
        .source_endpoint_ptr = &SourceEndpointForRoute9,
        .sink_endpoint_ptr = &SinkEndpointForRoute12,
        .active = 0,
        .route_id = 0x2003,
        .atd = { 
            .enabled = 0,
            .clk_config = UCS_STREAM_PORT_CLK_CFG_NONE
        },
        .static_connection = { 
            .static_con_label = 0x0,
            .fallback_enabled = 0
        }    
    } };
