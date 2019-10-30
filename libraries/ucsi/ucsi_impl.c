/*------------------------------------------------------------------------------------------------*/
/* UNICENS Integration Helper Component                                                           */
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
#include <stdio.h>
#include "ucsi_api.h"
#include "ucsi_print.h"

/************************************************************************/
/* Private Definitions and variables                                    */
/************************************************************************/

#define TRACE_BUFFER_SZ     (100)
#define MAGIC               (0xA144BEAF)
#define LOCAL_NODE_ADDR     (0x1)
#define UNKNOWN_NODE_ADDR   (0xFFFF)

#define LIB_VERSION_MAJOR   (2)
#define LIB_VERSION_MINOR   (3)
#define LIB_VERSION_RELEASE (0)
#define LIB_VERSION_BUILD   (4567)

#define NODE_START_ADDR     (0x401)
#define NODE_END_ADDR       (NODE_START_ADDR + MAX_NODES - 1)
#define SONOMA_IS_VERSION   (0x41U)
#define MISC_HB(value)      ((uint8_t)((uint16_t)(value) >> 8))
#define MISC_LB(value)      ((uint8_t)((uint16_t)(value) & (uint16_t)0xFF))

static char m_traceBuffer[TRACE_BUFFER_SZ];

/************************************************************************/
/* Throw error if UNICENS Library is not existent or wrong version      */
/************************************************************************/

#if !defined(UCS_VERSION_MAJOR) || !defined(UCS_VERSION_MINOR) || !defined(UCS_VERSION_RELEASE) || !defined(UCS_VERSION_BUILD)
#error UNICENS library is missing. Perform command 'git submodule update --init --recursive'
#endif
#if (UCS_VERSION_MAJOR != LIB_VERSION_MAJOR) ||  (UCS_VERSION_MINOR != LIB_VERSION_MINOR) || (UCS_VERSION_RELEASE != LIB_VERSION_RELEASE) || (UCS_VERSION_BUILD != LIB_VERSION_BUILD)
#error UNICENS library is outdated. Perform command 'git submodule update --init --recursive'
#endif

/************************************************************************/
/* Private Function Prototypes                                          */
/************************************************************************/
static bool EnqueueCommand(UCSI_Data_t *my, UnicensCmdEntry_t *cmd);
static void OnCommandExecuted(UCSI_Data_t *my, UnicensCmd_t cmd, bool success);
static void RB_Init(RB_t *rb, uint16_t amountOfEntries, uint32_t sizeOfEntry, uint8_t *workingBuffer);
static void *RB_GetReadPtr(RB_t *rb);
static void RB_PopReadPtr(RB_t *rb);
static void *RB_GetWritePtr(RB_t *rb);
static void RB_PopWritePtr(RB_t *rb);
static uint16_t OnUnicensGetTime(void *user_ptr);
static void OnUnicensService( void *user_ptr );
static void OnUnicensError( Ucs_Error_t error_code, void *user_ptr );
static void OnUnicensAppTimer( uint16_t timeout, void *user_ptr );
static void OnUnicensDebugErrorMsg(Ucs_Message_t *m, void *user_ptr);
static void OnLldCtrlStart( Ucs_Lld_Api_t* api_ptr, void *inst_ptr, void *lld_user_ptr );
static void OnLldCtrlStop( void *lld_user_ptr );
static void OnLldResetInic(void *lld_user_ptr);
static void OnLldCtrlRxMsgAvailable( void *lld_user_ptr );
static void OnLldCtrlTxTransmitC( Ucs_Lld_TxMsg_t *msg_ptr, void *lld_user_ptr );
static void OnUnicensRoutingResult(Ucs_Rm_Route_t* route_ptr, Ucs_Rm_RouteInfos_t route_infos, void *user_ptr);
static void OnUnicensNetworkStatus(uint16_t change_mask, uint16_t events, Ucs_Network_Availability_t availability,
    Ucs_Network_AvailInfo_t avail_info,Ucs_Network_AvailTransCause_t avail_trans_cause, uint16_t node_address,
    uint8_t max_position, uint16_t packet_bw, void *user_ptr);
static void OnUnicensDebugXrmResources(Ucs_Xrm_ResourceType_t resource_type,
    Ucs_Xrm_ResObject_t *resource_ptr, Ucs_Xrm_ResourceInfos_t resource_infos,
    Ucs_Rm_EndPoint_t *endpoint_inst_ptr, void *user_ptr);
static void OnUcsInitResult(Ucs_InitResult_t result, void *user_ptr);
static void OnUcsStopResult(Ucs_StdResult_t result, void *user_ptr);
static void OnUcsGpioPortCreate(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_Result_t result, void *user_ptr);
static void OnUcsGpioPortWrite(uint16_t node_address, uint16_t gpio_port_handle, uint16_t current_state, uint16_t sticky_state, Ucs_Gpio_Result_t result, void *user_ptr);
static void OnUcsSupvReport(Ucs_Supv_Report_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr, void *user_ptr);
static void OnUcsNsRun(uint16_t node_address, Ucs_Ns_ResultCode_t result, Ucs_Ns_ErrorInfo_t error_info, void *ucs_user_ptr);
static void OnUcsAmsRxMsgReceived(void *user_ptr);
static void OnUcsGpioTriggerEventStatus(uint16_t node_address, uint16_t gpio_port_handle,
    uint16_t rising_edges, uint16_t falling_edges, uint16_t levels, void * user_ptr);
static void OnUcsI2CWrite(uint16_t node_address, uint16_t i2c_port_handle,
    uint8_t i2c_slave_address, uint8_t data_len, Ucs_I2c_Result_t result, void *user_ptr);
static void OnUcsI2CRead(uint16_t node_address, uint16_t i2c_port_handle,
            uint8_t i2c_slave_address, uint8_t data_len, uint8_t data_ptr[], Ucs_I2c_Result_t result, void *user_ptr);
static void OnUcsPacketFilterMode(uint16_t node_address, Ucs_StdResult_t result, void *user_ptr);
static void OnSupvModeReport(Ucs_Supv_Mode_t mode, Ucs_Supv_State_t state, void *user_ptr);
static void OnUcsProgramLocalNode(Ucs_Signature_t *signature_ptr, Ucs_Prg_Command_t **program_pptr, Ucs_Prg_ReportCb_t *result_fptr, void *user_ptr);
static void OnProgramSignature(Ucs_Signature_t *signature_ptr, void *user_ptr);
static void OnProgramEvent(Ucs_Supv_ProgramEvent_t code, void *user_ptr);
static void OnProgramResult(Ucs_Prg_Report_t *result_ptr, void *user_ptr);
#if ENABLE_AMS_LIB
static void OnUcsAmsWrite(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void *user_ptr);
#endif
static const char *GetSupervisorModeString(Ucs_Supv_Mode_t mode);
static void OnHdxReport(Ucs_Hdx_Report_t *result, void *user_ptr);
static void ProgrammingSetFoundNodeCount(UCSI_Data_t *my, uint8_t nodeCount);
static void ProgrammingStoreSignature(UCSI_Data_t *my, const Ucs_Signature_t *signature);
static bool ProgrammingExit(UCSI_Data_t *my);
static uint8_t ProgrammingGetNodeCount(UCSI_Data_t *my);
static uint16_t BuildIdentString(Ucs_IdentString_t *ident_string, uint8_t data[]);
static uint16_t CalcCCITT16(uint8_t data[], uint16_t length, uint16_t start_value);
static uint16_t CalcCCITT16Step(uint16_t crc, uint8_t value);

/************************************************************************/
/* Public Function Implementations                                      */
/************************************************************************/

void UCSI_Init(UCSI_Data_t *my, void *pTag, bool debugLocalNode)
{
    Ucs_Return_t result;
    assert(NULL != my);
    memset(my, 0, sizeof(UCSI_Data_t));
    my->magic = MAGIC;
    my->tag = pTag;
    my->unicens = Ucs_CreateInstance();
    if (NULL == my->unicens)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Can not instance a new version of UNICENS, "\
            "increase UCS_NUM_INSTANCES define", 0);
        assert(false);
        return;
    }
    result = Ucs_SetDefaultConfig(&my->uniInitData);
    if(UCS_RET_SUCCESS != result)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Can not set default values to UNICENS config (result=0x%X)", 1, result);
        assert(false);
        return;
    }
    my->uniInitData.user_ptr = my;
    my->uniInitData.supv.report_fptr = OnUcsSupvReport;
    my->uniInitData.supv.report_mode_fptr = OnSupvModeReport;
    my->uniInitData.supv.diag_hdx_fptr = OnHdxReport;
    my->uniInitData.supv.diag_type = UCS_SUPV_DT_HDX;
    my->uniInitData.supv.prog_local_fptr = OnUcsProgramLocalNode;
    my->uniInitData.supv.prog_signature_fptr = OnProgramSignature;
    my->uniInitData.supv.prog_event_fptr = OnProgramEvent;

    my->uniInitData.general.inic_watchdog_enabled = ENABLE_INIC_WATCHDOG;
    my->uniInitData.general.get_tick_count_fptr = &OnUnicensGetTime;
    my->uniInitData.general.request_service_fptr = &OnUnicensService;
    my->uniInitData.general.error_fptr = &OnUnicensError;
    my->uniInitData.general.set_application_timer_fptr = &OnUnicensAppTimer;
    my->uniInitData.general.debug_error_msg_fptr = &OnUnicensDebugErrorMsg;
    my->uniInitData.ams.enabled = ENABLE_AMS_LIB;
    my->uniInitData.ams.rx.message_received_fptr = &OnUcsAmsRxMsgReceived;
    my->uniInitData.network.status.notification_mask = 0xC2;
    my->uniInitData.network.status.cb_fptr = &OnUnicensNetworkStatus;

    my->uniInitData.lld.lld_user_ptr = my;
    my->uniInitData.lld.start_fptr =  &OnLldCtrlStart;
    my->uniInitData.lld.stop_fptr = &OnLldCtrlStop;
    my->uniInitData.lld.reset_fptr = &OnLldResetInic;
    my->uniInitData.lld.rx_available_fptr = &OnLldCtrlRxMsgAvailable;
    my->uniInitData.lld.tx_transmit_fptr = &OnLldCtrlTxTransmitC;

    my->uniInitData.rm.report_fptr = &OnUnicensRoutingResult;
    my->uniInitData.rm.debug_resource_status_fptr = &OnUnicensDebugXrmResources;
    my->uniInitData.rm.debug_message_enable = debugLocalNode;

    my->uniInitData.gpio.trigger_event_status_fptr = &OnUcsGpioTriggerEventStatus;

    RB_Init(&my->rb, CMD_QUEUE_LEN, sizeof(UnicensCmdEntry_t), my->rbBuf);
}

bool UCSI_RunCableDiagnosis(UCSI_Data_t *my)
{
    UnicensCmdEntry_t *e;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    e = (UnicensCmdEntry_t *)RB_GetWritePtr(&my->rb);
    if (NULL == e) return false;
    my->supvShallMode = UCS_SUPV_MODE_DIAGNOSIS;
    e->cmd = UnicensCmd_SupvSetMode;
    e->val.SupvMode.supvMode = UCS_SUPV_MODE_INACTIVE;
    return EnqueueCommand(my, e);
}

bool UCSI_NewConfig(UCSI_Data_t *my,
    uint16_t packetBw, uint16_t proxyBw, Ucs_Rm_Route_t *pRoutesList, uint16_t routesListSize,
    Ucs_Rm_Node_t *pNodesList, uint16_t nodesListSize,
    uint8_t programAmountOfNodes, bool programPersistent)
{
    UnicensCmdEntry_t *e;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    if (my->initialized)
    {
        e = (UnicensCmdEntry_t *)RB_GetWritePtr(&my->rb);
        if (NULL == e) return false;
        e->cmd = UnicensCmd_Stop;
        RB_PopWritePtr(&my->rb);
    }
    my->uniInitData.supv.packet_bw = packetBw;
    my->uniInitData.supv.proxy_channel_bw = proxyBw;
    my->uniInitData.supv.routes_list_ptr = pRoutesList;
    my->uniInitData.supv.routes_list_size = routesListSize;
    my->uniInitData.supv.nodes_list_ptr = pNodesList;
    my->uniInitData.supv.nodes_list_size = nodesListSize;
    if (programAmountOfNodes >= 1)
        my->program.triggerNodeCount = programAmountOfNodes - 1; /* Root node does not count */
    my->program.persistent = programPersistent;
    if (0 == my->program.triggerNodeCount) {
        my->supvShallMode = UCS_SUPV_MODE_NORMAL;
        my->uniInitData.supv.mode = UCS_SUPV_MODE_NORMAL;
    } else {
        my->supvShallMode = UCS_SUPV_MODE_PROGRAMMING;
        my->uniInitData.supv.mode = UCS_SUPV_MODE_INACTIVE;
    }
    e = (UnicensCmdEntry_t *)RB_GetWritePtr(&my->rb);
    if (NULL == e) return false;
    e->cmd =  UnicensCmd_Init;
    e->val.Init.init_ptr = &my->uniInitData;
    RB_PopWritePtr(&my->rb);
    UCSI_CB_OnServiceRequired(my->tag);
    UCSIPrint_Init(pRoutesList, routesListSize, my);
    return true;
}

bool UCSI_ExecuteScript(UCSI_Data_t *my, uint16_t targetAddress, Ucs_Ns_Script_t *pScriptList, uint8_t scriptListLength)
{
    UnicensCmdEntry_t e;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    if (!my->initialized || (UCS_SUPV_MODE_NORMAL != my->uniInitData.supv.mode)
        || NULL == my->uniInitData.supv.nodes_list_ptr || 0 == my->uniInitData.supv.nodes_list_size
        || NULL == pScriptList || 0 == scriptListLength)
    {
        return false;
    }
    e.cmd = UnicensCmd_NsRun;
    e.val.NsRun.nodeAddress = targetAddress;
    e.val.NsRun.scriptPtr = pScriptList;
    e.val.NsRun.scriptSize = scriptListLength;
    return EnqueueCommand(my, &e);
}

bool UCSI_ProcessRxData(UCSI_Data_t *my,
    const uint8_t *pBuffer, uint32_t len)
{
    Ucs_Lld_RxMsg_t *msg = NULL;
    assert(MAGIC == my->magic);
    if (NULL == my->uniLld || NULL == my->uniLldHPtr) return false;
    msg = my->uniLld->rx_allocate_fptr(my->uniLldHPtr, len);
    if (NULL == msg)
    {
        /*This may happen by definition, OnLldCtrlRxMsgAvailable()
          will be called, once buffers are available again*/
        return false;
    }
    msg->data_size = len;
    memcpy(msg->data_ptr, pBuffer, len);
    my->uniLld->rx_receive_fptr(my->uniLldHPtr, msg);
    return true;
}

void UCSI_Service(UCSI_Data_t *my)
{
    UnicensCmdEntry_t *e;
    bool popEntry = true; /*Set to false in specific case, where function will callback asynchrony.*/
    assert(MAGIC == my->magic);
    if (NULL != my->unicens && my->triggerService) {
        my->triggerService = false;
        Ucs_Service(my->unicens);
    }
    if (my->printTrigger)
    {
        my->printTrigger = false;
        UCSIPrint_Service(UCSI_CB_OnGetTime(my->tag));
    }
    if (NULL != my->currentCmd) return;
    my->currentCmd = e = (UnicensCmdEntry_t *)RB_GetReadPtr(&my->rb);
    if (NULL == e) return;
    switch (e->cmd) {
        case UnicensCmd_Init:
            if (UCS_RET_SUCCESS == Ucs_Init(my->unicens, e->val.Init.init_ptr, OnUcsInitResult))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Init failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_Init, false, LOCAL_NODE_ADDR);
            }
            break;
        case UnicensCmd_Stop:
            if (UCS_RET_SUCCESS == Ucs_Stop(my->unicens, OnUcsStopResult))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Stop failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_Stop, false, LOCAL_NODE_ADDR);
            }
            break;
        case UnicensCmd_RmSetRoute:
            if (UCS_RET_SUCCESS == Ucs_Rm_SetRouteActive(my->unicens, e->val.RmSetRoute.routePtr, e->val.RmSetRoute.isActive))
            {
                my->pendingRoutePtr = e->val.RmSetRoute.routePtr;
                popEntry = false;
            } else  {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Rm_SetRouteActive failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_RmSetRoute, false, e->val.RmSetRoute.routePtr->sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address);
            }
            break;
        case UnicensCmd_NsRun:
            if (UCS_RET_SUCCESS != Ucs_Ns_Run(my->unicens, e->val.NsRun.nodeAddress, e->val.NsRun.scriptPtr, e->val.NsRun.scriptSize, OnUcsNsRun))
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Ns_Run failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_NsRun, false, e->val.NsRun.nodeAddress);
            }
            break;
        case UnicensCmd_GpioCreatePort:
            if (UCS_RET_SUCCESS == Ucs_Gpio_CreatePort(my->unicens, e->val.GpioCreatePort.destination, 0, e->val.GpioCreatePort.debounceTime, OnUcsGpioPortCreate))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Gpio_CreatePort failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_GpioCreatePort, false, e->val.GpioCreatePort.destination);
            }
            break;
        case UnicensCmd_GpioWritePort:
            if (UCS_RET_SUCCESS == Ucs_Gpio_WritePort(my->unicens, e->val.GpioWritePort.destination, 0x1D00, e->val.GpioWritePort.mask, e->val.GpioWritePort.data, OnUcsGpioPortWrite))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Gpio_WritePort failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_GpioWritePort, false, e->val.GpioWritePort.destination);
            }
            break;
        case UnicensCmd_I2CWrite:
            if (UCS_RET_SUCCESS == Ucs_I2c_WritePort(my->unicens, e->val.I2CWrite.destination, 0x0F00,
                (e->val.I2CWrite.isBurst ? UCS_I2C_BURST_MODE : UCS_I2C_DEFAULT_MODE), e->val.I2CWrite.blockCount,
                e->val.I2CWrite.slaveAddr, e->val.I2CWrite.timeout, e->val.I2CWrite.dataLen, e->val.I2CWrite.data, OnUcsI2CWrite))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_I2c_WritePort failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_I2CWrite, false, e->val.I2CWrite.destination);
                if (e->val.I2CWrite.result_fptr) {
                    e->val.I2CWrite.result_fptr(NULL /*processing error*/, e->val.I2CWrite.request_ptr);
                }
            }
            break;
        case UnicensCmd_I2CRead:
            if (UCS_RET_SUCCESS == Ucs_I2c_ReadPort(my->unicens, e->val.I2CRead.destination, 0x0F00,
                e->val.I2CRead.slaveAddr, e->val.I2CRead.dataLen, e->val.I2CRead.timeout, OnUcsI2CRead))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_I2c_ReadPort failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_I2CRead, false, e->val.I2CRead.destination);
            }
            break;
#if ENABLE_AMS_LIB
        case UnicensCmd_SendAmsMessage:
        {
            Ucs_AmsTx_Msg_t *msg;
            msg = Ucs_AmsTx_AllocMsg(my->unicens, e->val.SendAms.payloadLen);
            if (NULL == msg)
            {
                /* Try again later */
                popEntry = false;
                break;
            }
            if (0 != e->val.SendAms.payloadLen)
            {
                assert(NULL != msg->data_ptr);
                memcpy(msg->data_ptr, e->val.SendAms.pPayload, e->val.SendAms.payloadLen);
            }
            msg->custom_info_ptr = NULL;
            msg->data_size = e->val.SendAms.payloadLen;
            msg->destination_address = e->val.SendAms.targetAddress;
            msg->llrbc = 10;
            msg->msg_id = e->val.SendAms.msgId;
            if (UCS_RET_SUCCESS == Ucs_AmsTx_SendMsg(my->unicens, msg, OnUcsAmsWrite))
            {
                popEntry = false;
            }
            else
            {
                Ucs_AmsTx_FreeUnusedMsg(my->unicens, msg);
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_AmsTx_SendMsg failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_SendAmsMessage, false, e->val.SendAms.targetAddress);
            }
            break;
        }
#endif
        case UnicensCmd_PacketFilterMode:
            if (UCS_RET_SUCCESS == Ucs_Network_SetPacketFilterMode(my->unicens, e->val.PacketFilterMode.destination_address, e->val.PacketFilterMode.mode, OnUcsPacketFilterMode))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Ucs_Network_SetPacketFilterMode failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_PacketFilterMode, false, e->val.PacketFilterMode.destination_address);
            }
            break;
        case UnicensCmd_ProgramNode:
            if (UCS_RET_SUCCESS == Ucs_Supv_ProgramNode(my->unicens, e->val.ProgramNode.nodePosAddr, &e->val.ProgramNode.signature, &e->val.ProgramNode.commands, OnProgramResult))
                popEntry = false;
            else
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "UnicensCmd_ProgramNode failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_ProgramNode, false, e->val.ProgramNode.nodePosAddr);
            }
            break;
        case UnicensCmd_ProgramExit:
            if (UCS_RET_SUCCESS != Ucs_Supv_ProgramExit(my->unicens))
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "UnicensCmd_ProgramExit failed", 0);
            }
            break;
        case UnicensCmd_SupvSetMode:
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Setting supervisor mode to '%s'", 1, GetSupervisorModeString(e->val.SupvMode.supvMode));
            if (UCS_RET_SUCCESS == Ucs_Supv_SetMode(my->unicens, e->val.SupvMode.supvMode)) {
                popEntry = false;
            } else {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "UnicensCmd_SupvSetMode failed", 0);
                UCSI_CB_OnCommandResult(my->tag, UnicensCmd_SupvSetMode, false, UNKNOWN_NODE_ADDR);
            }
            break;
        default:
            assert(false);
            break;
    }
    if (popEntry)
    {
        my->currentCmd = NULL;
        RB_PopReadPtr(&my->rb);
    }
}

void UCSI_Timeout(UCSI_Data_t *my)
{
    assert(MAGIC == my->magic);
    if (NULL == my->unicens) return;
    Ucs_ReportTimeout(my->unicens);
    if (my->printTrigger)
    {
        my->printTrigger = false;
        UCSIPrint_Service(UCSI_CB_OnGetTime(my->tag));
    }
}

bool UCSI_SendAmsMessage(UCSI_Data_t *my, uint16_t msgId, uint16_t targetAddress, uint8_t *pPayload, uint32_t payloadLen)
{
#if ENABLE_AMS_LIB
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    if (payloadLen > AMS_MSG_MAX_LEN)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "SendAms was called with payload length=%d, allowed is=%d", 2, payloadLen, AMS_MSG_MAX_LEN);
        return false;
    }
    entry.cmd = UnicensCmd_SendAmsMessage;
    entry.val.SendAms.msgId = msgId;
    entry.val.SendAms.targetAddress = targetAddress;
    entry.val.SendAms.payloadLen = payloadLen;
    memcpy(entry.val.SendAms.pPayload, pPayload, payloadLen);
    return EnqueueCommand(my, &entry);
#else
    return false;
#endif
}

bool UCSI_GetAmsMessage(UCSI_Data_t *my, uint16_t *pMsgId, uint16_t *pSourceAddress, uint8_t **pPayload, uint32_t *pPayloadLen)
{
    Ucs_AmsRx_Msg_t *msg;
    assert(MAGIC == my->magic);
    if (NULL == my->unicens || NULL == pPayload || NULL == pPayloadLen) return false;
    msg = Ucs_AmsRx_PeekMsg(my->unicens);
    if (NULL == msg) return false;
    *pMsgId = msg->msg_id;
    *pSourceAddress = msg->source_address;
    *pPayload = msg->data_ptr;
    *pPayloadLen = msg->data_size;
    return true;
}

void UCSI_ReleaseAmsMessage(UCSI_Data_t *my)
{
    assert(MAGIC == my->magic);
    if (NULL == my->unicens) return;
    Ucs_AmsRx_ReleaseMsg(my->unicens);
}

bool UCSI_SetRouteActive(UCSI_Data_t *my, uint16_t routeId, bool isActive)
{
    uint16_t i;
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my || NULL == my->uniInitData.supv.routes_list_ptr) return false;
    for (i = 0; i < my->uniInitData.supv.routes_list_size; i++)
    {
        Ucs_Rm_Route_t *route = &my->uniInitData.supv.routes_list_ptr[i];
        if (route->route_id != routeId)
            continue;
        entry.cmd = UnicensCmd_RmSetRoute;
        entry.val.RmSetRoute.routePtr = route;
        entry.val.RmSetRoute.isActive = isActive;
        return EnqueueCommand(my, &entry);
    }
    return false;
}

bool UCSI_I2CWrite(UCSI_Data_t *my, uint16_t targetAddress, bool isBurst, uint8_t blockCount,
    uint8_t slaveAddr, uint16_t timeout, uint8_t dataLen, const uint8_t *pData,
    Ucsi_ResultCb_t result_fptr, void *request_ptr)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my || NULL == pData || 0 == dataLen) return false;
    if (dataLen > I2C_WRITE_MAX_LEN)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "I2CWrite was called with payload length=%d, allowed is=%d", 2, dataLen, I2C_WRITE_MAX_LEN);
        return false;
    }
    entry.cmd = UnicensCmd_I2CWrite;
    entry.val.I2CWrite.destination = targetAddress;
    entry.val.I2CWrite.isBurst = isBurst;
    entry.val.I2CWrite.blockCount = blockCount;
    entry.val.I2CWrite.slaveAddr = slaveAddr;
    entry.val.I2CWrite.timeout = timeout;
    entry.val.I2CWrite.dataLen = dataLen;
    entry.val.I2CWrite.result_fptr = result_fptr;
    entry.val.I2CWrite.request_ptr = request_ptr;
    memcpy(entry.val.I2CWrite.data, pData, dataLen);
    return EnqueueCommand(my, &entry);
}

bool UCSI_I2CRead(UCSI_Data_t *my, uint16_t targetAddress, uint8_t slaveAddr, uint16_t timeout, uint8_t dataLen)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my || 0 == dataLen) return false;
    entry.cmd = UnicensCmd_I2CRead;
    entry.val.I2CRead.destination = targetAddress;
    entry.val.I2CRead.slaveAddr = slaveAddr;
    entry.val.I2CRead.timeout = timeout;
    entry.val.I2CRead.dataLen = dataLen;
    return EnqueueCommand(my, &entry);
}

bool UCSI_SetGpioState(UCSI_Data_t *my, uint16_t targetAddress, uint8_t gpioPinId, bool isHighState)
{
    uint16_t mask;
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    mask = 1 << gpioPinId;
    entry.cmd = UnicensCmd_GpioWritePort;
    entry.val.GpioWritePort.destination = targetAddress;
    entry.val.GpioWritePort.mask = mask;
    entry.val.GpioWritePort.data = isHighState ? mask : 0;
    return EnqueueCommand(my, &entry);
}

bool UCSI_EnablePromiscuousMode(UCSI_Data_t *my, uint16_t targetAddress, bool enablePromiscuous)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    entry.cmd = UnicensCmd_PacketFilterMode;
    entry.val.PacketFilterMode.destination_address = targetAddress;
    entry.val.PacketFilterMode.mode = enablePromiscuous ? 0xA : 0x0;
    return EnqueueCommand(my, &entry);
}

/************************************************************************/
/* Private Functions                                                    */
/************************************************************************/

static bool EnqueueCommand(UCSI_Data_t *my, UnicensCmdEntry_t *cmd)
{
    UnicensCmdEntry_t *e;
    if (NULL == my || NULL == cmd)
    {
        assert(false);
        return false;
    }
    e = RB_GetWritePtr(&my->rb);
    if (NULL == e)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Could not enqueue command. Increase CMD_QUEUE_LEN define", 0);
        return false;
    }
    memcpy(e, cmd, sizeof(UnicensCmdEntry_t));
    RB_PopWritePtr(&my->rb);
    UCSI_CB_OnServiceRequired(my->tag);
    UCSIPrint_UnicensActivity();
    return true;
}

static void OnCommandExecuted(UCSI_Data_t *my, UnicensCmd_t cmd, bool success)
{
    UnicensCmdEntry_t *e;
    if (NULL == my)
    {
        assert(false);
        return;
    }
    e = my->currentCmd;
    if (NULL == e)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "OnUniCommandExecuted was called, but no "\
            "command is in queue", 0);
        assert(false);
        return;
    }
    if (e->cmd != cmd)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "OnUniCommandExecuted was called with "\
            "wrong command (Expected=0x%X, Got=0x%X", 2, e->cmd, cmd);
        assert(false);
        return;
    }
    UCSIPrint_UnicensActivity();
    switch (e->cmd) {
        case UnicensCmd_Init:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, LOCAL_NODE_ADDR);
            break;
        case UnicensCmd_Stop:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, LOCAL_NODE_ADDR);
            break;
        case UnicensCmd_GpioCreatePort:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.GpioCreatePort.destination);
            break;
        case UnicensCmd_GpioWritePort:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.GpioWritePort.destination);
            break;
        case UnicensCmd_I2CWrite:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.I2CWrite.destination);
            break;
        case UnicensCmd_I2CRead:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.I2CRead.destination);
            break;
#if ENABLE_AMS_LIB
        case UnicensCmd_SendAmsMessage:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.SendAms.targetAddress);
            break;
#endif
        case UnicensCmd_PacketFilterMode:
                UCSI_CB_OnCommandResult(my->tag, cmd, success, e->val.PacketFilterMode.destination_address);
            break;
        default:
            UCSI_CB_OnCommandResult(my->tag, cmd, success, UNKNOWN_NODE_ADDR);
            break;
    }
    my->currentCmd = NULL;
    RB_PopReadPtr(&my->rb);
}

static void RB_Init(RB_t *rb, uint16_t amountOfEntries, uint32_t sizeOfEntry, uint8_t *workingBuffer)
{
    assert(NULL != rb);
    assert(NULL != workingBuffer);
    rb->dataQueue = workingBuffer;
    rb->pRx = rb->dataQueue;
    rb->pTx = rb->dataQueue;
    rb->amountOfEntries = amountOfEntries;
    rb->sizeOfEntry = sizeOfEntry;
    rb->rxPos = 0;
    rb->txPos = 0;
}

static void *RB_GetReadPtr(RB_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    if (rb->txPos - rb->rxPos > 0)
    return (void *)rb->pRx;
    return NULL;
}

static void RB_PopReadPtr(RB_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);

    rb->pRx += rb->sizeOfEntry;
    if (rb->pRx >= rb->dataQueue + ( rb->amountOfEntries * rb->sizeOfEntry))
    rb->pRx = rb->dataQueue;
    ++rb->rxPos;
    assert(rb->txPos >= rb->rxPos);
}

static void *RB_GetWritePtr(RB_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    if (rb->txPos - rb->rxPos < rb->amountOfEntries)
    return (void *)rb->pTx;
    return NULL;
}

static void RB_PopWritePtr(RB_t *rb)
{
    assert(NULL != rb);
    assert(0 != rb->dataQueue);
    rb->pTx += rb->sizeOfEntry;
    if (rb->pTx >= rb->dataQueue + ( rb->amountOfEntries * rb->sizeOfEntry))
    rb->pTx = rb->dataQueue;
    ++rb->txPos;
    assert(rb->txPos >= rb->rxPos);
}

static uint16_t OnUnicensGetTime(void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    return UCSI_CB_OnGetTime(my->tag);
}

static void OnUnicensService( void *user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    my->triggerService = true;
    UCSI_CB_OnServiceRequired(my->tag);
}

static void OnUnicensError( Ucs_Error_t error_code, void *user_ptr )
{
    UnicensCmdEntry_t e;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    error_code = error_code;
    assert(MAGIC == my->magic);
    UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "UNICENS general error, code=0x%X, restarting", 1, error_code);
    e.cmd = UnicensCmd_Init;
    e.val.Init.init_ptr = &my->uniInitData;
    EnqueueCommand(my, &e);
}

static void OnUnicensAppTimer( uint16_t timeout, void *user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnSetServiceTimer(my->tag, timeout);
}

static void OnUnicensDebugErrorMsg(Ucs_Message_t *m, void *user_ptr)
{
    char val[5];
    uint8_t i;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    m_traceBuffer[0] = '\0';
    for (i = 0; NULL != m->tel.tel_data_ptr && i < m->tel.tel_len; i++)
    {
        snprintf(val, sizeof(val), "%02X ", m->tel.tel_data_ptr[i]);
        strcat(m_traceBuffer, val);
    }
    UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Received error message, source=%x, %X.%X.%X.%X, [ %s ]",
        6, m->source_addr, m->id.fblock_id, m->id.instance_id,
        m->id.function_id, m->id.op_type, m_traceBuffer);
}

static void OnLldCtrlStart( Ucs_Lld_Api_t* api_ptr, void *inst_ptr, void *lld_user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    my->uniLld = api_ptr;
    my->uniLldHPtr = inst_ptr;
    UCSI_CB_OnStart(my->tag);
}

static void OnLldCtrlStop( void *lld_user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    my->uniLld = NULL;
    my->uniLldHPtr = NULL;
    UCSIPrint_SetNetworkAvailable(false, 0);
    UCSI_CB_OnStop(my->tag);
}

static void OnLldResetInic(void *lld_user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnResetInic(my->tag);
}

static void OnLldCtrlRxMsgAvailable( void *lld_user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnServiceRequired(my->tag);
}

static void OnLldCtrlTxTransmitC( Ucs_Lld_TxMsg_t *msg_ptr, void *lld_user_ptr )
{
    UCSI_Data_t *my;
    Ucs_Mem_Buffer_t * buf_ptr;
    uint8_t buffer[BOARD_PMS_TX_SIZE];
    uint32_t bufferPos = 0;
    my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    if (NULL == msg_ptr || NULL == my || NULL == my->uniLld || NULL == my->uniLldHPtr)
    {
        assert(false);
        return;
    }
    for (buf_ptr = msg_ptr->memory_ptr; buf_ptr != NULL; buf_ptr = buf_ptr->next_buffer_ptr)
    {
        if (buf_ptr->data_size + bufferPos > sizeof(buffer))
        {
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "TX buffer is too small, increase " \
                "BOARD_PMS_TX_SIZE define (%lu > %lu)", 2, buf_ptr->data_size + bufferPos, sizeof(buffer));
            my->uniLld->tx_release_fptr(my->uniLldHPtr, msg_ptr);
            return;
        }
        memcpy(&buffer[bufferPos], buf_ptr->data_ptr, buf_ptr->data_size);
        bufferPos += buf_ptr->data_size;
    }
    assert(bufferPos == msg_ptr->memory_ptr->total_size);
    my->uniLld->tx_release_fptr(my->uniLldHPtr, msg_ptr);
    UCSI_CB_OnTxRequest(my->tag, buffer, bufferPos);
}

static void OnUnicensRoutingResult(Ucs_Rm_Route_t* route_ptr, Ucs_Rm_RouteInfos_t route_infos, void *user_ptr)
{
    bool available;
    uint16_t conLabel;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    if (route_ptr == my->pendingRoutePtr)
    {
        if (my->currentCmd->val.RmSetRoute.isActive) {
            OnCommandExecuted(my, UnicensCmd_RmSetRoute, (UCS_RM_ROUTE_INFOS_BUILT == route_infos));
        } else {
            OnCommandExecuted(my, UnicensCmd_RmSetRoute, (UCS_RM_ROUTE_INFOS_DESTROYED == route_infos));
        }
        my->pendingRoutePtr = NULL;
    }
    if (NULL == route_ptr ||
        UCS_RM_ROUTE_INFOS_ATD_UPDATE == route_infos ||
        UCS_RM_ROUTE_INFOS_ATD_ERROR == route_infos)
        return;
    available = UCS_RM_ROUTE_INFOS_BUILT == route_infos;
    conLabel = Ucs_Rm_GetConnectionLabel(my->unicens, route_ptr);
    UCSIPrint_SetRouteState(route_ptr->route_id, available, conLabel);
    UCSI_CB_OnRouteResult(my->tag, route_ptr->route_id, available, conLabel);
}

static void OnUnicensNetworkStatus(uint16_t change_mask, uint16_t events, Ucs_Network_Availability_t availability,
    Ucs_Network_AvailInfo_t avail_info,Ucs_Network_AvailTransCause_t avail_trans_cause, uint16_t node_address,
    uint8_t max_position, uint16_t packet_bw, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    bool available = UCS_NW_AVAILABLE == availability;
    assert(MAGIC == my->magic);
    ProgrammingSetFoundNodeCount(my, available ? max_position : 0);
    UCSIPrint_SetNetworkAvailable(available, max_position);
    UCSI_CB_OnNetworkState(my->tag, available, packet_bw, max_position);
}

static void OnUnicensDebugXrmResources(Ucs_Xrm_ResourceType_t resource_type,
    Ucs_Xrm_ResObject_t *resource_ptr, Ucs_Xrm_ResourceInfos_t resource_infos,
    Ucs_Rm_EndPoint_t *endpoint_inst_ptr, void *user_ptr)
{
    char *msg = NULL;
    UCSI_Data_t *my;
    uint16_t adr = 0xFFFF;
#ifndef DEBUG_XRM
    resource_type = resource_type;
    resource_ptr = resource_ptr;
    resource_infos = resource_infos;
    endpoint_inst_ptr = endpoint_inst_ptr;
    user_ptr = user_ptr;
#else
    endpoint_inst_ptr = endpoint_inst_ptr;
    my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    if (NULL == resource_ptr) return;
    if (endpoint_inst_ptr && endpoint_inst_ptr->node_obj_ptr &&
            endpoint_inst_ptr->node_obj_ptr->signature_ptr)
        adr = endpoint_inst_ptr->node_obj_ptr->signature_ptr->node_address;
    switch (resource_infos)
    {
    case UCS_XRM_INFOS_BUILT:
        msg = (char *)"has been built";
        UCSIPrint_SetObjectState(resource_ptr, ObjState_Build);
        break;
    case UCS_XRM_INFOS_DESTROYED:
        msg = (char *)"has been destroyed";
        UCSIPrint_SetObjectState(resource_ptr, ObjState_Unused);
        break;
    case UCS_XRM_INFOS_ERR_BUILT:
        msg = (char *)"cannot be built";
        UCSIPrint_SetObjectState(resource_ptr, ObjState_Failed);
        break;
    case UCS_XRM_INFOS_ERR_DESTROYED:
        msg = (char *)"cannot be destroyed";
        UCSIPrint_SetObjectState(resource_ptr, ObjState_Failed);
        break;
    default:
        msg = (char *)"has unknown state";
        break;
    }
    switch(resource_type)
    {
        case UCS_XRM_RC_TYPE_NW_SOCKET:
        {
            Ucs_Xrm_NetworkSocket_t *ms = (Ucs_Xrm_NetworkSocket_t *)resource_ptr;
            assert(ms->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): NW socket %s, handle=%04X, "\
                "direction=%d, type=%d, bandwidth=%d", 6, adr, msg, ms->nw_port_handle,
                ms->direction, ms->data_type, ms->bandwidth);
            break;
        }
        case UCS_XRM_RC_TYPE_MLB_PORT:
        {
            Ucs_Xrm_MlbPort_t *m = (Ucs_Xrm_MlbPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): MLB port %s, index=%d, clock=%d", 4,
                adr, msg, m->index, m->clock_config);
            break;
        }
        case UCS_XRM_RC_TYPE_MLB_SOCKET:
        {
            Ucs_Xrm_MlbSocket_t *m = (Ucs_Xrm_MlbSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): MLB socket %s, direction=%d, type=%d,"\
                " bandwidth=%d, channel=%d", 6, adr, msg, m->direction, m->data_type,
                m->bandwidth, m->channel_address);
            break;
        }
        case UCS_XRM_RC_TYPE_USB_PORT:
        {
            Ucs_Xrm_UsbPort_t *m = (Ucs_Xrm_UsbPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): USB port %s, in-cnt=%d, out-cnt=%d", 4,
                adr, msg, m->streaming_if_ep_in_count, m->streaming_if_ep_out_count);
            break;
        }
        case UCS_XRM_RC_TYPE_USB_SOCKET:
        {
            Ucs_Xrm_UsbSocket_t *m = (Ucs_Xrm_UsbSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): USB socket %s, direction=%d, type=%d," \
                " ep-addr=%02X, frames=%d", 6, adr, msg, m->direction, m->data_type,
                m->end_point_addr, m->frames_per_transfer);
            break;
        }
        case UCS_XRM_RC_TYPE_STRM_PORT:
        {
            Ucs_Xrm_StrmPort_t *m = (Ucs_Xrm_StrmPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): I2S port %s, index=%d, clock=%d, "\
                "align=%d", 5, adr, msg, m->index, m->clock_config, m->data_alignment);
            break;
        }
        case UCS_XRM_RC_TYPE_STRM_SOCKET:
        {
            Ucs_Xrm_StrmSocket_t *m = (Ucs_Xrm_StrmSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): I2S socket %s, direction=%d, type=%d"\
                ", bandwidth=%d, pin=%d", 6, adr, msg, m->direction, m->data_type,
                m->bandwidth, m->stream_pin_id);
            break;
        }
        case UCS_XRM_RC_TYPE_SYNC_CON:
        {
            Ucs_Xrm_SyncCon_t *m = (Ucs_Xrm_SyncCon_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): Sync connection %s, mute=%d, "\
                "offset=%d", 4, adr, msg, m->mute_mode, m->offset);
            break;
        }
        case UCS_XRM_RC_TYPE_COMBINER:
        {
            Ucs_Xrm_Combiner_t *m = (Ucs_Xrm_Combiner_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): Combiner %s, bytes per frame=%d",
                3, adr, msg, m->bytes_per_frame);
            break;
        }
        case UCS_XRM_RC_TYPE_SPLITTER:
        {
            Ucs_Xrm_Splitter_t *m = (Ucs_Xrm_Splitter_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): Splitter %s, bytes per frame=%d",
                3, adr, msg, m->bytes_per_frame);
            break;
        }
        case UCS_XRM_RC_TYPE_AVP_CON:
        {
            Ucs_Xrm_AvpCon_t *m = (Ucs_Xrm_AvpCon_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Xrm-Debug (0x%03X): Isoc-AVP connection %s, packetSize=%d",
                3, adr, msg, m->isoc_packet_size);
            break;
        }
        default:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Xrm-Debug (0x%03X): Unknown type=%d %s", 3 , adr, resource_type, msg);
    }
#endif
}

static void OnUcsInitResult(Ucs_InitResult_t result, void *user_ptr)
{
    UnicensCmdEntry_t e;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    my->initialized = (UCS_INIT_RES_SUCCESS == result);
    OnCommandExecuted(my, UnicensCmd_Init, (UCS_INIT_RES_SUCCESS == result));
    if (!my->initialized)
    {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "UcsInitResult reported error (0x%X), restarting...", 1, result);
        e.cmd = UnicensCmd_Init;
        e.val.Init.init_ptr = &my->uniInitData;
        EnqueueCommand(my, &e);
    }
}

static void OnUcsStopResult(Ucs_StdResult_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    result = result; /*TODO: check error case*/
    assert(MAGIC == my->magic);
    my->initialized = false;
    OnCommandExecuted(my, UnicensCmd_Stop, (UCS_RES_SUCCESS == result.code));
    UCSI_CB_OnStop(my->tag);
}

static void OnUcsGpioPortCreate(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_GpioCreatePort, (UCS_GPIO_RES_SUCCESS == result.code));
}

static void OnUcsGpioPortWrite(uint16_t node_address, uint16_t gpio_port_handle, uint16_t current_state, uint16_t sticky_state, Ucs_Gpio_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_GpioWritePort, (UCS_GPIO_RES_SUCCESS == result.code));
}

static void OnUcsSupvReport(Ucs_Supv_Report_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr, void *user_ptr)
{
    uint16_t node_address;
    uint16_t node_pos_addr;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    assert(NULL != signature_ptr);
    node_address = signature_ptr->node_address;
    node_pos_addr = signature_ptr->node_pos_addr;
    switch (code)
    {
    case UCS_SUPV_REP_IGNORED_UNKNOWN:
        UCSIPrint_SetNodeAvailable(node_address, node_pos_addr, NodeState_Ignored);
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Node=%X(%X): Ignored, because unknown", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_IGNORED_DUPLICATE:
        UCSIPrint_SetNodeAvailable(node_address, node_pos_addr, NodeState_Ignored);
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Node=%X(%X): Ignored, because duplicated", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_NOT_AVAILABLE:
        UCSIPrint_SetNodeAvailable(node_address, node_pos_addr, NodeState_NotAvailable);
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Node=%X(%X): Not available", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_WELCOMED:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Node=%X(%X): Welcomed", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_SCRIPT_FAILURE:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Node=%X(%X): Script failure", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_IRRECOVERABLE:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Node=%X(%X): IRRECOVERABLE ERROR!!", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_SCRIPT_SUCCESS:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Node=%X(%X): Script ok", 2, node_address, node_pos_addr);
        break;
    case UCS_SUPV_REP_AVAILABLE:
        UCSIPrint_SetNodeAvailable(node_address, node_pos_addr, NodeState_Available);
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Node=%X(%X): Available", 2, node_address, node_pos_addr);
        break;
    default:
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Node=%X(%X): unknown code", 2, node_address, node_pos_addr);
        break;
    }
    UCSI_CB_OnMgrReport(my->tag, code, signature_ptr, node_ptr);
}

static void OnUcsNsRun(uint16_t node_address, Ucs_Ns_ResultCode_t result, Ucs_Ns_ErrorInfo_t error_info, void *ucs_user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)ucs_user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnCommandResult(my->tag, UnicensCmd_NsRun, (UCS_NS_RES_SUCCESS == result), node_address);
#ifdef DEBUG_XRM
    UCSI_CB_OnUserMessage(my->tag, (UCS_NS_RES_SUCCESS != result), "OnUcsNsRun (%03X): script executed %s",
        2, node_address, (UCS_NS_RES_SUCCESS == result ? "succeeded" : "false"));
#endif
}

static void OnUcsAmsRxMsgReceived(void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnAmsMessageReceived(my->tag);
}

static void OnUcsGpioTriggerEventStatus(uint16_t node_address, uint16_t gpio_port_handle,
    uint16_t rising_edges, uint16_t falling_edges, uint16_t levels, void * user_ptr)
{
    uint8_t i;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    for (i = 0; i < 16; i++)
    {
        if (0 != ((rising_edges >> i) & 0x1))
            UCSI_CB_OnGpioStateChange(my->tag, node_address, i, true);
        if (0 != ((falling_edges >> i) & 0x1))
            UCSI_CB_OnGpioStateChange(my->tag, node_address, i, false);
    }
}

static void OnUcsI2CWrite(uint16_t node_address, uint16_t i2c_port_handle,
    uint8_t i2c_slave_address, uint8_t data_len, Ucs_I2c_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_I2CWrite, (UCS_I2C_RES_SUCCESS == result.code));
    if ((my->currentCmd->cmd == UnicensCmd_I2CWrite) && (my->currentCmd->val.I2CWrite.result_fptr)) {
        my->currentCmd->val.I2CWrite.result_fptr(&result.code, my->currentCmd->val.I2CWrite.request_ptr);
    } else {
        if (UCS_I2C_RES_SUCCESS != result.code)
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Remote I2C Write to node=0x%X failed", 1, node_address);
    }
}

static void OnUcsI2CRead(uint16_t node_address, uint16_t i2c_port_handle,
            uint8_t i2c_slave_address, uint8_t data_len, uint8_t data_ptr[], Ucs_I2c_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_I2CRead, (UCS_I2C_RES_SUCCESS == result.code));
    UCSI_CB_OnI2CRead(my->tag, (UCS_I2C_RES_SUCCESS == result.code), node_address, i2c_slave_address, data_ptr, data_len);
}

#if ENABLE_AMS_LIB
static void OnUcsAmsWrite(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_SendAmsMessage, (UCS_AMSTX_RES_SUCCESS == result));
    if (UCS_AMSTX_RES_SUCCESS != result)
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "SendAms failed with result=0x%x, info=0x%X", 2, result, info);
}
#endif

static void OnUcsPacketFilterMode(uint16_t node_address, Ucs_StdResult_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_PacketFilterMode, (UCS_RES_SUCCESS == result.code));
    if (UCS_RES_SUCCESS != result.code)
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Set promiscuous mode failed with error code %d", 1, result.code);
}

static void OnSupvModeReport(Ucs_Supv_Mode_t mode, Ucs_Supv_State_t state, void *user_ptr)
{
    const char *pModeString = "Unknown mode";
    const char *pStateString = "Unknown state";
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    pModeString = GetSupervisorModeString(mode);
    switch(state)
    {
        case UCS_SUPV_STATE_BUSY:
            pStateString = "UCS_SUPV_STATE_BUSY";
            break;
        case UCS_SUPV_STATE_READY:
            pStateString = "UCS_SUPV_STATE_READY";
            break;
        default:
            assert(false);
    }
    if (UCS_SUPV_STATE_BUSY == state && UCS_SUPV_MODE_DIAGNOSIS == mode) {
        memset(my->cableResult, 0, sizeof(my->cableResult));
    }
    UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, "Supervisor mode='%s', state='%s'", 2, pModeString, pStateString);
    if (UCS_SUPV_STATE_READY == state)
    {
        bool check;
        if (NULL != my->currentCmd && UnicensCmd_SupvSetMode == my->currentCmd->cmd)
        {
            OnCommandExecuted(my, UnicensCmd_SupvSetMode, true);
        }
        check = !my->switchOnlyInInactive;
        check |= my->switchOnlyInInactive && UCS_SUPV_MODE_INACTIVE == mode;
        if (check && my->supvShallMode != mode)
        {
            UnicensCmdEntry_t *entry;
            my->switchOnlyInInactive = false;
            entry = RB_GetWritePtr(&my->rb);
            if (NULL == entry)
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Could not enqueue SupvMode command. Increase CMD_QUEUE_LEN define", 0);
                return;
            }
            entry->cmd = UnicensCmd_SupvSetMode;
            entry->val.SupvMode.supvMode = my->supvShallMode;
            RB_PopWritePtr(&my->rb);
            UCSI_CB_OnServiceRequired(my->tag);
            UCSIPrint_UnicensActivity();
        }
    }
}

static const char *GetSupervisorModeString(Ucs_Supv_Mode_t mode)
{
    const char *pModeString = "Unknown mode";
    switch(mode)
    {
        case UCS_SUPV_MODE_NORMAL:
            pModeString = "UCS_SUPV_MODE_NORMAL";
            break;
        case UCS_SUPV_MODE_INACTIVE:
            pModeString = "UCS_SUPV_MODE_INACTIVE";
            break;
        case UCS_SUPV_MODE_FALLBACK:
            pModeString = "UCS_SUPV_MODE_FALLBACK";
            break;
        case UCS_SUPV_MODE_DIAGNOSIS:
            pModeString = "UCS_SUPV_MODE_DIAGNOSIS";
            break;
        case UCS_SUPV_MODE_PROGRAMMING:
            pModeString = "UCS_SUPV_MODE_PROGRAMMING";
            break;
        default:
            assert(false);
            break;
    }
    return pModeString;
}

static void OnHdxReport(Ucs_Hdx_Report_t *result, void *user_ptr)
{
    const char *pCodeString = "unknown";
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(result);
    assert(MAGIC == my->magic);
    switch(result->code)
    {
        case UCS_HDX_RES_SUCCESS:
            pCodeString = "UCS_HDX_RES_SUCCESS";
            break;
        case UCS_HDX_RES_SLAVE_WRONG_POS:
            pCodeString = "UCS_HDX_RES_SLAVE_WRONG_POS";
            break;
        case UCS_HDX_RES_RING_BREAK:
            pCodeString = "UCS_HDX_RES_RING_BREAK";
            break;
        case UCS_HDX_RES_NO_RING_BREAK:
            pCodeString = "UCS_HDX_RES_NO_RING_BREAK";
            break;
        case UCS_HDX_RES_NO_RESULT:
            pCodeString = "UCS_HDX_RES_NO_RESULT";
            break;
        case UCS_HDX_RES_TIMEOUT:
            pCodeString = "UCS_HDX_RES_TIMEOUT";
            break;
        case UCS_HDX_RES_ERROR:
            pCodeString = "UCS_HDX_RES_ERROR";
            break;
        case UCS_HDX_RES_END:
            pCodeString = "UCS_HDX_RES_END";
            break;
        default:
            assert(false);
            break;
    }
    
    if (result->signature_ptr) {
        uint16_t nodeAddr = result->signature_ptr->node_address;
        uint16_t posAddr = result->signature_ptr->node_pos_addr;
        snprintf(m_traceBuffer, sizeof(m_traceBuffer), "HalfDuplex Report code='%s', result=0x%X pos=0x%X nodeAddr=0x%X posAddr=0x%X", 
             pCodeString, result->cable_diag_result, result->position, nodeAddr, posAddr);
        if (result->position <= MAX_NODES) {
            my->cableResult[result->position - 1] = nodeAddr;
        }
    } else {
        uint8_t i = 0;
        uint8_t len = 0;
        snprintf(m_traceBuffer, sizeof(m_traceBuffer), "HalfDuplex Report code='%s', result=0x%X pos=0x%X", 
             pCodeString, result->cable_diag_result, result->position);
        for (i = 0; i < MAX_NODES; i++) {
            if (my->cableResult[i]) {
                len++;
            } else {
                break;
            }
        }
        UCSI_CB_OnCableDiagnosisResult(my->tag, my->cableResult, len);
    }
    UCSI_CB_OnUserMessage(my->tag, UCSI_MsgDebug, m_traceBuffer, 0);
    my->switchOnlyInInactive = true;
    my->supvShallMode = UCS_SUPV_MODE_NORMAL;
}

static void OnUcsProgramLocalNode(Ucs_Signature_t *signature_ptr, Ucs_Prg_Command_t **program_pptr, Ucs_Prg_ReportCb_t *result_fptr, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    signature_ptr = signature_ptr;

    /* Currently not implemented */
    program_pptr = NULL;
    result_fptr = NULL;
    user_ptr = NULL;
}

static void OnProgramSignature(Ucs_Signature_t *signature_ptr, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    ProgrammingStoreSignature(my, signature_ptr);
}

static void OnProgramEvent(Ucs_Supv_ProgramEvent_t code, void *user_ptr)
{
    bool error = false;
    const char * code_str = "UNKNOWN";
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    switch (code)
    {
        case UCS_SUPV_PROG_INFO_EXIT:
            code_str = "UCS_SUPV_PROG_INFO_EXIT";
            break;
        case UCS_SUPV_PROG_INFO_SCAN_NEW:
            code_str = "UCS_SUPV_PROG_INFO_SCAN_NEW";
            memset(my->program.signatureValid, 0, sizeof(my->program.signatureValid));
            break;
        case UCS_SUPV_PROG_ERROR_INIT_NWS:
            code_str = "UCS_SUPV_PROG_ERROR_INIT_NWS";
            break;
        case UCS_SUPV_PROG_ERROR_LOCAL_CFG:
            code_str = "UCS_SUPV_PROG_ERROR_LOCAL_CFG";
            break;
        case UCS_SUPV_PROG_ERROR_STARTUP:
            code_str = "UCS_SUPV_PROG_ERROR_STARTUP";
            break;
        case UCS_SUPV_PROG_ERROR_STARTUP_TO:
            code_str = "UCS_SUPV_PROG_ERROR_STARTUP_TO";
            break;
        case UCS_SUPV_PROG_ERROR_UNSTABLE:
            code_str = "UCS_SUPV_PROG_ERROR_UNSTABLE";
            break;
        case UCS_SUPV_PROG_ERROR_PROGRAM:
            code_str = "UCS_SUPV_PROG_ERROR_PROGRAM";
            break;
        default:
            code_str = "UCS_SUPV_UNKNOWN";
            break;
    }
    UCSI_CB_OnUserMessage(my->tag, error, "Programming Event='%s'", 1, code_str);
}

static void OnProgramResult(Ucs_Prg_Report_t *result_ptr, void *user_ptr)
{
    bool success;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    assert(result_ptr);
    success = (UCS_PRG_RES_SUCCESS == result_ptr->code);
    OnCommandExecuted(my, UnicensCmd_ProgramNode, success);
    UCSI_CB_OnUserMessage(my->tag, !success, "Programming Result=0x%02X", 1, result_ptr->code);
}

static void ProgrammingSetFoundNodeCount(UCSI_Data_t *my, uint8_t nodeCount)
{
    if (NULL == my) return;
    if (0 == nodeCount || nodeCount > 64) return;
    if (UCS_SUPV_MODE_PROGRAMMING == my->supvShallMode) {
        my->program.allowProgramming = (nodeCount - 1) == my->program.triggerNodeCount;
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "Programming is %s, due to the node count of %d", 2, my->program.allowProgramming ? "enabled" : "disabled", nodeCount);
    }
}

static void ProgrammingStoreSignature(UCSI_Data_t *my, const Ucs_Signature_t *signature)
{
    uint8_t pos;
    uint8_t count;
    if (NULL == my) return;
    if (NULL == signature) return;
    if (signature->node_pos_addr < NODE_START_ADDR || signature->node_pos_addr > NODE_END_ADDR)
    {
        assert(false);
        return;
    }
    pos = signature->node_pos_addr - NODE_START_ADDR;
    memcpy(&my->program.nodes[pos], signature, sizeof(Ucs_Signature_t));
    my->program.signatureValid[pos] = true;
    count = ProgrammingGetNodeCount(my);
    if ( my->program.allowProgramming && 0 != my->program.triggerNodeCount && count == my->program.triggerNodeCount)
    {
        bool leaveProgrammingMode = true;
        Ucs_IdentString_t newIdentString = { 0 };
        const Ucs_Signature_t *nodeToBeFlashed;
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "Check if programming needed, node count %d/%d", 2, count, my->program.triggerNodeCount);
        nodeToBeFlashed = UCSI_CB_OnProgrammingModeDeviceDiscovery(my->tag, my->program.nodes, my->program.triggerNodeCount, &newIdentString);
        if (nodeToBeFlashed) {
            /* Program node */
            UnicensCmdEntry_t *entry;
            entry = RB_GetWritePtr(&my->rb);
            if (NULL == entry)
            {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Could not enqueue program command. Increase CMD_QUEUE_LEN define", 0);
                leaveProgrammingMode = true;
                return;
            }
            leaveProgrammingMode = false;
            UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "Programming nodePos=0x%X, node address 0x%X change to 0x%X, mac %04X%04X%04X change to %04X%04X%04X", 9, 
                nodeToBeFlashed->node_pos_addr,
                nodeToBeFlashed->node_address,
                newIdentString.node_address,
                nodeToBeFlashed->mac_47_32,
                nodeToBeFlashed->mac_31_16,
                nodeToBeFlashed->mac_15_0,
                newIdentString.mac_47_32,
                newIdentString.mac_31_16,
                newIdentString.mac_15_0);
            entry->cmd = UnicensCmd_ProgramNode;
            entry->val.ProgramNode.nodePosAddr = nodeToBeFlashed->node_pos_addr;
            memcpy(&entry->val.ProgramNode.signature, nodeToBeFlashed, sizeof(Ucs_Signature_t));
            entry->val.ProgramNode.commands.mem_id = my->program.persistent ? UCS_PRG_MID_IS : UCS_PRG_MID_ISTEST;
            entry->val.ProgramNode.commands.session_type = UCS_PRG_ST_IS;
            entry->val.ProgramNode.commands.address = 0;
            entry->val.ProgramNode.commands.unit_size = 1;
            entry->val.ProgramNode.commands.data_size = BuildIdentString(&newIdentString, entry->val.ProgramNode.data);
            entry->val.ProgramNode.commands.data_ptr = entry->val.ProgramNode.data;
            RB_PopWritePtr(&my->rb);
            UCSI_CB_OnServiceRequired(my->tag);
            UCSIPrint_UnicensActivity();
        }
        if (leaveProgrammingMode) {
            if (ProgrammingExit(my)) {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "Leaving programming mode, no programming needed", 0);
            } else {
                UCSI_CB_OnUserMessage(my->tag, UCSI_MsgError, "Can not leave program mode. Increase CMD_QUEUE_LEN define", 0);
            }
        }
    } else {
        UCSI_CB_OnUserMessage(my->tag, UCSI_MsgUrgent, "Wait for programming node count %d/%d", 2, count, my->program.triggerNodeCount);
    }
}

static bool ProgrammingExit(UCSI_Data_t *my)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    my->supvShallMode = UCS_SUPV_MODE_NORMAL;
    entry.cmd = UnicensCmd_ProgramExit;
    return EnqueueCommand(my, &entry);
}

static uint8_t ProgrammingGetNodeCount(UCSI_Data_t *my)
{
    uint8_t i = 0;
    if (NULL == my) return 0;
    for (; i < MAX_NODES; i++)
    {
        if (!my->program.signatureValid[i])
            break;
    }
    return i;
}

static uint16_t BuildIdentString(Ucs_IdentString_t *ident_string, uint8_t data[])
{
    uint16_t i = 0;
    uint16_t crc16;
    data[i++] = SONOMA_IS_VERSION;
    data[i++] = 0xFFU;
    data[i++] = MISC_HB(ident_string->node_address);
    data[i++] = MISC_LB(ident_string->node_address);
    data[i++] = (MISC_HB(ident_string->group_address)) | 0xFCU;
    data[i++] = MISC_LB(ident_string->group_address);
    data[i++] = MISC_HB(ident_string->mac_15_0);
    data[i++] = MISC_LB(ident_string->mac_15_0);
    data[i++] = MISC_HB(ident_string->mac_31_16);
    data[i++] = MISC_LB(ident_string->mac_31_16);
    data[i++] = MISC_HB(ident_string->mac_47_32);
    data[i++] = MISC_LB(ident_string->mac_47_32);

    crc16 = CalcCCITT16(&data[0], 12U, 0U);

    data[i++] = MISC_LB(crc16);          /* Cougar needs Little Endian here. */
    data[i++] = MISC_HB(crc16);
    return i;
}

static uint16_t CalcCCITT16(uint8_t data[], uint16_t length, uint16_t start_value)
{
    uint8_t i = 0U;
    while( i < length )
    {
        start_value = CalcCCITT16Step(start_value, data[i]);
        i++;
    }
    return ( start_value );
}

static uint16_t CalcCCITT16Step(uint16_t crc, uint8_t value)
{
   uint8_t crc_hi = MISC_HB(crc);
   uint8_t crc_lo = MISC_LB(crc);

   value = (value ^ crc_lo) & 0xFFU;
   value = (value ^ ((uint8_t)(value << 4) & 0xF0U)) & 0xFFU;
   crc_lo = (crc_hi ^ ((uint8_t)(value << 3) & 0xFCU) ^ ((value >> 4) & 0xFU)) & 0xFFU;
   crc_hi = (value ^ ((value >> 5) & 0x7U)) & 0xFFU;

   return (uint16_t)(((uint16_t)((uint16_t)crc_hi << 8) & (uint16_t)0xFF00U) | (uint16_t)((uint16_t)crc_lo & (uint16_t)0xFFU));
}

/************************************************************************/
/* Callback from UCSI Print component:                                  */
/************************************************************************/

void UCSIPrint_CB_NeedService(void *tag)
{
    UCSI_Data_t *my = (UCSI_Data_t *)tag;
    assert(MAGIC == my->magic);
    my->printTrigger = true;
}

void UCSIPrint_CB_OnUserMessage(void *usr, const char pMsg[])
{
    void *tag = NULL;
    UCSI_Data_t *my = (UCSI_Data_t *)usr;
    if (my)
    {
        assert(MAGIC == my->magic);
        tag = my->tag;
    }
    UCSI_CB_OnPrintRouteTable(tag, pMsg);
}

/************************************************************************/
/* Debug Message output from UNICENS stack:                             */
/************************************************************************/
#if defined(UCS_TR_ERROR) || defined(UCS_TR_INFO)
#include <stdio.h>
void App_TraceError(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...)
{
    va_list argptr;
    void *tag = NULL;
    UCSI_Data_t *my = (UCSI_Data_t *)ucs_user_ptr;
    if (my)
    {
        assert(MAGIC == my->magic);
        tag = my->tag;
    }
    va_start(argptr, vargs_cnt);
    vsnprintf(m_traceBuffer, sizeof(m_traceBuffer), entry_str, argptr);
    va_end(argptr);
    UCSIPrint_UnicensActivity();
    UCSI_CB_OnUserMessage(tag, true, "Error | %s | %s", 2, module_str, m_traceBuffer);
}

void App_TraceInfo(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...)
{
    va_list argptr;
    void *tag = NULL;
    UCSI_Data_t *my = (UCSI_Data_t *)ucs_user_ptr;
    if (my)
    {
        assert(MAGIC == my->magic);
        tag = my->tag;
    }
    va_start(argptr, vargs_cnt);
    vsnprintf(m_traceBuffer, sizeof(m_traceBuffer), entry_str, argptr);
    va_end(argptr);
    UCSI_CB_OnUserMessage(tag, false, "Info | %s | %s", 2, module_str, m_traceBuffer);
}
#endif

