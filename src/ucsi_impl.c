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

/************************************************************************/
/* Private Definitions and variables                                    */
/************************************************************************/
#define MAGIC (0xA144BEAF)

/************************************************************************/
/* Private Function Prototypes                                          */
/************************************************************************/
static bool EnqueueCommand(UCSI_Data_t *my, UnicensCmdEntry_t *cmd);
static void OnCommandExecuted(UCSI_Data_t *my, UnicensCmd_t cmd);
static void RB_Init(RB_t *rb, uint16_t amountOfEntries, uint32_t sizeOfEntry, uint8_t *workingBuffer);
static void *RB_GetReadPtr(RB_t *rb);
static void RB_PopReadPtr(RB_t *rb);
static void *RB_GetWritePtr(RB_t *rb);
static void RB_PopWritePtr(RB_t *rb);
static uint16_t OnUnicensGetTime(void *user_ptr);
static void OnUnicensService( void *user_ptr );
static void OnUnicensError( Ucs_Error_t error_code, void *user_ptr );
static void OnUnicensAppTimer( uint16_t timeout, void *user_ptr );
static void OnUnicensDebugErrorMsg(Msg_MostTel_t *m, void *user_ptr);
static void OnLldCtrlStart( Ucs_Lld_Api_t* api_ptr, void *inst_ptr, void *lld_user_ptr );
static void OnLldCtrlStop( void *lld_user_ptr );
static void OnLldCtrlRxMsgAvailable( void *lld_user_ptr );
static void OnLldCtrlTxTransmitC( Ucs_Lld_TxMsg_t *msg_ptr, void *lld_user_ptr );
static void OnUnicensRoutingResult(Ucs_Rm_Route_t* route_ptr, Ucs_Rm_RouteInfos_t route_infos, void *user_ptr);
static void OnUnicensNetworkStatus(uint16_t change_mask, uint16_t events, Ucs_Network_Availability_t availability,
    Ucs_Network_AvailInfo_t avail_info, Ucs_Network_AvailTransCause_t avail_trans_cause, uint16_t node_address,
    uint8_t node_position, uint8_t max_position, uint16_t packet_bw, void *user_ptr);
static void OnUnicensDebugXrmResources(Ucs_Xrm_ResourceType_t resource_type,
    Ucs_Xrm_ResObject_t *resource_ptr, Ucs_Xrm_ResourceInfos_t resource_infos,
    Ucs_Rm_EndPoint_t *endpoint_inst_ptr, void *user_ptr);
static void OnUcsInitResult(Ucs_InitResult_t result, void *user_ptr);
static void OnUcsStopResult(Ucs_StdResult_t result, void *user_ptr);
static void OnUcsGpioPortCreate(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_Result_t result, void *user_ptr);
static void OnUcsGpioPortWrite(uint16_t node_address, uint16_t gpio_port_handle, uint16_t current_state, uint16_t sticky_state, Ucs_Gpio_Result_t result, void *user_ptr);
static void OnUcsMgrReport(Ucs_MgrReport_t code, uint16_t node_address, Ucs_Rm_Node_t *node_ptr, void *user_ptr);
static void OnUcsNsRun(Ucs_Rm_Node_t * node_ptr, Ucs_Ns_ResultCode_t result, void *ucs_user_ptr);
static void OnUcsAmsRxMsgReceived(void *user_ptr);
static void OnUcsGpioTriggerEventStatus(uint16_t node_address, uint16_t gpio_port_handle,
    uint16_t rising_edges, uint16_t falling_edges, uint16_t levels, void * user_ptr);
static void OnUcsI2CWrite(uint16_t node_address, uint16_t i2c_port_handle,
    uint8_t i2c_slave_address, uint8_t data_len, Ucs_I2c_Result_t result, void *user_ptr);
static void OnUcsAmsWrite(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void *user_ptr);

/************************************************************************/
/* Public Function Implementations                                      */
/************************************************************************/

void UCSI_Init(UCSI_Data_t *my, void *pTag)
{
    Ucs_Return_t result;
    assert(NULL != my);
    memset(my, 0, sizeof(UCSI_Data_t));
    my->magic = MAGIC;
    my->tag = pTag;
    my->unicens = Ucs_CreateInstance();
    if (NULL == my->unicens)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "Can not instance a new version of UNICENS, "\
            "increase UCS_NUM_INSTANCES define", 0);
        assert(false);
        return;
    }
    result = Ucs_SetDefaultConfig(&my->uniInitData);
    if(UCS_RET_SUCCESS != result)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "Can not set default values to UNICENS config (result=0x%X)", 1, result);
        assert(false);
        return;
    }
    my->uniInitData.user_ptr = my;
    my->uniInitData.mgr.report_fptr = OnUcsMgrReport;

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
    my->uniInitData.lld.rx_available_fptr = &OnLldCtrlRxMsgAvailable;
    my->uniInitData.lld.tx_transmit_fptr = &OnLldCtrlTxTransmitC;

    my->uniInitData.rm.report_fptr = &OnUnicensRoutingResult;
    my->uniInitData.rm.debug_resource_status_fptr = &OnUnicensDebugXrmResources;

    my->uniInitData.gpio.trigger_event_status_fptr = &OnUcsGpioTriggerEventStatus;

    RB_Init(&my->rb, CMD_QUEUE_LEN, sizeof(UnicensCmdEntry_t), my->rbBuf);
}

bool UCSI_NewConfig(UCSI_Data_t *my,
    uint16_t packetBw, Ucs_Rm_Route_t *pRoutesList, uint16_t routesListSize,
    Ucs_Rm_Node_t *pNodesList, uint16_t nodesListSize)
{
    UnicensCmdEntry_t *e;
    assert(MAGIC == my->magic);
    if (my->initialized)
    {
        e = (UnicensCmdEntry_t *)RB_GetWritePtr(&my->rb);
        if (NULL == e) return false;
        e->cmd = UnicensCmd_Stop;
        RB_PopWritePtr(&my->rb);
    }
    my->uniInitData.mgr.packet_bw = packetBw;
    my->uniInitData.mgr.routes_list_ptr = pRoutesList;
    my->uniInitData.mgr.routes_list_size = routesListSize;
    my->uniInitData.mgr.nodes_list_ptr = pNodesList;
    my->uniInitData.mgr.nodes_list_size = nodesListSize;
    my->uniInitData.mgr.enabled = true;
    e = (UnicensCmdEntry_t *)RB_GetWritePtr(&my->rb);
    if (NULL == e) return false;
    e->cmd =  UnicensCmd_Init;
    e->val.Init.init_ptr = &my->uniInitData;
    RB_PopWritePtr(&my->rb);
    UCSI_CB_OnServiceRequired(my->tag);
    return true;
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
    if (NULL != my->currentCmd) return;
    my->currentCmd = e = (UnicensCmdEntry_t *)RB_GetReadPtr(&my->rb);
    if (NULL == e) return;
    switch (e->cmd) {
        case UnicensCmd_Init:
            if (UCS_RET_SUCCESS == Ucs_Init(my->unicens, e->val.Init.init_ptr, OnUcsInitResult))
                popEntry = false;
            else
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Init failed", 0);
            break;
        case UnicensCmd_Stop:
            if (UCS_RET_SUCCESS == Ucs_Stop(my->unicens, OnUcsStopResult))
                popEntry = false;
            else
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Stop failed", 0);
            break;
        case UnicensCmd_RmSetRoute:
            if (UCS_RET_SUCCESS != Ucs_Rm_SetRouteActive(my->unicens, e->val.RmSetRoute.routePtr, e->val.RmSetRoute.isActive))
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Rm_SetRouteActive failed", 0);
            break;
        case UnicensCmd_NsRun:
            if (UCS_RET_SUCCESS != Ucs_Ns_Run(my->unicens, e->val.NsRun.node_ptr, OnUcsNsRun))
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Ns_Run failed", 0);
            break;
        case UnicensCmd_GpioCreatePort:
            if (UCS_RET_SUCCESS == Ucs_Gpio_CreatePort(my->unicens, e->val.GpioCreatePort.destination, 0, e->val.GpioCreatePort.debounceTime, OnUcsGpioPortCreate))
                popEntry = false;
            else
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Gpio_CreatePort failed", 0);
            break;
        case UnicensCmd_GpioWritePort:
            if (UCS_RET_SUCCESS == Ucs_Gpio_WritePort(my->unicens, e->val.GpioWritePort.destination, 0x1D00, e->val.GpioWritePort.mask, e->val.GpioWritePort.data, OnUcsGpioPortWrite))
                popEntry = false;
            else
                UCSI_CB_OnUserMessage(my->tag, true, "UnicensCmd_GpioWritePort failed", 0);
            break;            
        case UnicensCmd_I2CWrite:
            if (UCS_RET_SUCCESS == Ucs_I2c_WritePort(my->unicens, e->val.I2CWrite.destination, 0x0F00, 
                (e->val.I2CWrite.isBurst ? UCS_I2C_BURST_MODE : UCS_I2C_DEFAULT_MODE), e->val.I2CWrite.blockCount,
                e->val.I2CWrite.slaveAddr, e->val.I2CWrite.timeout, e->val.I2CWrite.dataLen, e->val.I2CWrite.data, OnUcsI2CWrite))
                popEntry = false;
            else
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_Gpio_CreatePort failed", 0);
            break;
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
                UCSI_CB_OnUserMessage(my->tag, true, "Ucs_AmsTx_SendMsg failed", 0);
            }
            break;
        }
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
}

bool UCSI_SendAmsMessage(UCSI_Data_t *my, uint16_t msgId, uint16_t targetAddress, uint8_t *pPayload, uint32_t payloadLen)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my) return false;
    if (payloadLen > AMS_MSG_MAX_LEN)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "SendAms was called with payload length=%d, allowed is=%d", 2, payloadLen, AMS_MSG_MAX_LEN);
        return false;
    }
    entry.cmd = UnicensCmd_SendAmsMessage;
    entry.val.SendAms.msgId = msgId;
    entry.val.SendAms.targetAddress = targetAddress;
    entry.val.SendAms.payloadLen = payloadLen;
    memcpy(entry.val.SendAms.pPayload, pPayload, payloadLen);
    return EnqueueCommand(my, &entry);
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
    if (NULL == my || NULL == my->uniInitData.mgr.routes_list_ptr) return false;
    for (i = 0; i < my->uniInitData.mgr.routes_list_size; i++)
    {
        Ucs_Rm_Route_t *route = &my->uniInitData.mgr.routes_list_ptr[i];
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
    uint8_t slaveAddr, uint16_t timeout, uint8_t dataLen, uint8_t *pData)
{
    UnicensCmdEntry_t entry;
    assert(MAGIC == my->magic);
    if (NULL == my || NULL == pData || 0 == dataLen) return false;
    if (dataLen > I2C_WRITE_MAX_LEN) return false;
    entry.cmd = UnicensCmd_I2CWrite;
    entry.val.I2CWrite.destination = targetAddress;
    entry.val.I2CWrite.isBurst = isBurst;
    entry.val.I2CWrite.blockCount = blockCount;
    entry.val.I2CWrite.slaveAddr = slaveAddr;
    entry.val.I2CWrite.timeout = timeout;
    entry.val.I2CWrite.dataLen = dataLen;
    memcpy(entry.val.I2CWrite.data, pData, dataLen);
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
        UCSI_CB_OnUserMessage(my->tag, true, "Could not enqueue command. Increase CMD_QUEUE_LEN define", 0);
        return false;
    }
    memcpy(e, cmd, sizeof(UnicensCmdEntry_t));
    RB_PopWritePtr(&my->rb);
    UCSI_CB_OnServiceRequired(my->tag);
    return true;
}

static void OnCommandExecuted(UCSI_Data_t *my, UnicensCmd_t cmd)
{
    if (NULL == my)
    {
        assert(false);
        return;
    }
    if (NULL == my->currentCmd)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "OnUniCommandExecuted was called, but no "\
            "command is in queue", 0);
        assert(false);
        return;
    }
    if (my->currentCmd->cmd != cmd)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "OnUniCommandExecuted was called with "\
            "wrong command (Expected=0x%X, Got=0x%X", 2, my->currentCmd->cmd, cmd);
        assert(false);
        return;
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
    UCSI_CB_OnUserMessage(my->tag, true, "UNICENS general error, code=0x%X, restarting", 1, error_code);
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

static void OnUnicensDebugErrorMsg(Msg_MostTel_t *m, void *user_ptr)
{
    char buffer[100];
    char val[5];
    uint8_t i;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    buffer[0] = '\0';
    for (i = 0; NULL != m->tel.tel_data_ptr && i < m->tel.tel_len; i++)
    {
        snprintf(val, sizeof(val), "%02X ", m->tel.tel_data_ptr[i]);
        strcat(buffer, val);
    }
    UCSI_CB_OnUserMessage(my->tag, true, "Received error message, source=%x, %X.%X.%X.%X, [ %s ]",
        6, m->source_addr, m->id.fblock_id, m->id.instance_id,
        m->id.function_id, m->id.op_type, buffer);
}

static void OnLldCtrlStart( Ucs_Lld_Api_t* api_ptr, void *inst_ptr, void *lld_user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    my->uniLld = api_ptr;
    my->uniLldHPtr = inst_ptr;
}

static void OnLldCtrlStop( void *lld_user_ptr )
{
    UCSI_Data_t *my = (UCSI_Data_t *)lld_user_ptr;
    assert(MAGIC == my->magic);
    my->uniLld = NULL;
    my->uniLldHPtr = NULL;
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
            UCSI_CB_OnUserMessage(my->tag, true, "TX buffer is too small, increase " \
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
    uint16_t conLabel;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    conLabel = Ucs_Rm_GetConnectionLabel(my->unicens, route_ptr);
    UCSI_CB_OnRouteResult(my->tag, route_ptr->route_id, UCS_RM_ROUTE_INFOS_BUILT == route_infos, conLabel);
}

static void OnUnicensNetworkStatus(uint16_t change_mask, uint16_t events, Ucs_Network_Availability_t availability,
    Ucs_Network_AvailInfo_t avail_info, Ucs_Network_AvailTransCause_t avail_trans_cause, uint16_t node_address,
    uint8_t node_position, uint8_t max_position, uint16_t packet_bw, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnNetworkState(my->tag, UCS_NW_AVAILABLE == availability, packet_bw, max_position);
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
        break;
        case UCS_XRM_INFOS_DESTROYED:
        msg = (char *)"has been destroyed";
        break;
        case UCS_XRM_INFOS_ERR_BUILT:
        msg = (char *)"cannot be built";
        break;
        default:
        msg = (char *)"cannot be destroyed";
        break;
    }
    switch(resource_type)
    {
        case UCS_XRM_RC_TYPE_MOST_SOCKET:
        {
            Ucs_Xrm_MostSocket_t *ms = (Ucs_Xrm_MostSocket_t *)resource_ptr;
            assert(ms->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): MOST socket %s, handle=%04X, "\
                "direction=%d, type=%d, bandwidth=%d", 6, adr, msg, ms->most_port_handle,
                ms->direction, ms->data_type, ms->bandwidth);
            break;
        }
        case UCS_XRM_RC_TYPE_MLB_PORT:
        {
            Ucs_Xrm_MlbPort_t *m = (Ucs_Xrm_MlbPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): MLB port %s, index=%d, clock=%d", 4,
                adr, msg, m->index, m->clock_config);
            break;
        }
        case UCS_XRM_RC_TYPE_MLB_SOCKET:
        {
            Ucs_Xrm_MlbSocket_t *m = (Ucs_Xrm_MlbSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): MLB socket %s, direction=%d, type=%d,"\
                " bandwidth=%d, channel=%d", 6, adr, msg, m->direction, m->data_type,
                m->bandwidth, m->channel_address);
            break;
        }
        case UCS_XRM_RC_TYPE_USB_PORT:
        {
            Ucs_Xrm_UsbPort_t *m = (Ucs_Xrm_UsbPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): USB port %s, in-cnt=%d, out-cnt=%d", 4,
                adr, msg, m->streaming_if_ep_in_count, m->streaming_if_ep_out_count);
            break;
        }
        case UCS_XRM_RC_TYPE_USB_SOCKET:
        {
            Ucs_Xrm_UsbSocket_t *m = (Ucs_Xrm_UsbSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): USB socket %s, direction=%d, type=%d," \
                " ep-addr=%02X, frames=%d", 6, adr, msg, m->direction, m->data_type,
                m->end_point_addr, m->frames_per_transfer);
            break;
        }
        case UCS_XRM_RC_TYPE_STRM_PORT:
        {
            Ucs_Xrm_StrmPort_t *m = (Ucs_Xrm_StrmPort_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): I2S port %s, index=%d, clock=%d, "\
                "align=%d", 5, adr, msg, m->index, m->clock_config, m->data_alignment);
            break;
        }
        case UCS_XRM_RC_TYPE_STRM_SOCKET:
        {
            Ucs_Xrm_StrmSocket_t *m = (Ucs_Xrm_StrmSocket_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): I2S socket %s, direction=%d, type=%d"\
                ", bandwidth=%d, pin=%d", 6, adr, msg, m->direction, m->data_type,
                m->bandwidth, m->stream_pin_id);
            break;
        }
        case UCS_XRM_RC_TYPE_SYNC_CON:
        {
            Ucs_Xrm_SyncCon_t *m = (Ucs_Xrm_SyncCon_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): Sync connection %s, mute=%d, "\
                "offset=%d", 4, adr, msg, m->mute_mode, m->offset);
            break;
        }
        case UCS_XRM_RC_TYPE_COMBINER:
        {
            Ucs_Xrm_Combiner_t *m = (Ucs_Xrm_Combiner_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): Combiner %s, bytes per frame=%d",
                3, adr, msg, m->bytes_per_frame);
            break;
        }
        case UCS_XRM_RC_TYPE_SPLITTER:
        {
            Ucs_Xrm_Splitter_t *m = (Ucs_Xrm_Splitter_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): Splitter %s, bytes per frame=%d",
                3, adr, msg, m->bytes_per_frame);
            break;
        }
        case UCS_XRM_RC_TYPE_AVP_CON:
        {
            Ucs_Xrm_AvpCon_t *m = (Ucs_Xrm_AvpCon_t *)resource_ptr;
            assert(m->resource_type == resource_type);
            UCSI_CB_OnUserMessage(my->tag, false, "Xrm-Debug (0x%03X): Isoc-AVP connection %s, packetSize=%d",
                3, adr, msg, m->isoc_packet_size);
            break;
        }
        default:
        UCSI_CB_OnUserMessage(my->tag, true, "Xrm-Debug (0x%03X): Unknown type=%d %s", 3 , adr, resource_type, msg);
    }
#endif
}

static void OnUcsInitResult(Ucs_InitResult_t result, void *user_ptr)
{
    UnicensCmdEntry_t e;
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    my->initialized = (UCS_INIT_RES_SUCCESS == result);
    OnCommandExecuted(my, UnicensCmd_Init);
    if (!my->initialized)
    {
        UCSI_CB_OnUserMessage(my->tag, true, "UcsInitResult reported error (0x%X), restarting...", 1, result);
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
    OnCommandExecuted(my, UnicensCmd_Stop);
    UCSI_CB_OnStop(my->tag);
}

static void OnUcsGpioPortCreate(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_GpioCreatePort);
}

static void OnUcsGpioPortWrite(uint16_t node_address, uint16_t gpio_port_handle, uint16_t current_state, uint16_t sticky_state, Ucs_Gpio_Result_t result, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_GpioWritePort);
}

static void OnUcsMgrReport(Ucs_MgrReport_t code, uint16_t node_address, Ucs_Rm_Node_t *node_ptr, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    switch (code)
    {
    case UCS_MGR_REP_IGNORED_UNKNOWN:
        UCSI_CB_OnUserMessage(my->tag, false, "Node=%X: Ignored, because unknown", 1, node_address);
        break;
    case UCS_MGR_REP_IGNORED_DUPLICATE:
        UCSI_CB_OnUserMessage(my->tag, true, "Node=%X: Ignored, because duplicated", 1, node_address);
        break;
    case UCS_MGR_REP_AVAILABLE:
    {
        UnicensCmdEntry_t e;
        UCSI_CB_OnUserMessage(my->tag, false, "Node=%X: Available", 1, node_address);
        /* Enable usage of remote GPIO ports */
        e.cmd = UnicensCmd_GpioCreatePort;
        e.val.GpioCreatePort.destination = node_address;
        e.val.GpioCreatePort.debounceTime = 20;
        EnqueueCommand(my, &e);
        /* Execute scripts, if there are any */
        if (node_ptr && node_ptr->script_list_ptr && node_ptr->script_list_size)
        {
            e.cmd = UnicensCmd_NsRun;
            e.val.NsRun.node_ptr = node_ptr;
            EnqueueCommand(my, &e);
        }
        break;
    }
    case UCS_MGR_REP_NOT_AVAILABLE:
        UCSI_CB_OnUserMessage(my->tag, false, "Node=%X: Not available", 1, node_address);
        break;
    default:
        UCSI_CB_OnUserMessage(my->tag, true, "Node=%X: unknown code", 1, node_address);
        break;
    }
}

static void OnUcsNsRun(Ucs_Rm_Node_t * node_ptr, Ucs_Ns_ResultCode_t result, void *ucs_user_ptr)
{
    UCSI_Data_t *my;
#ifndef DEBUG_XRM
    node_ptr = node_ptr;
    result = result;
    ucs_user_ptr;
#else
    my = (UCSI_Data_t *)ucs_user_ptr;
    assert(MAGIC == my->magic);
    UCSI_CB_OnUserMessage(my->tag, false, "OnUcsNsRun (%03X): script executed %s",
        2, node_ptr->signature_ptr->node_address,
        (UCS_NS_RES_SUCCESS == result ? "succeeded" : "false"));
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
    OnCommandExecuted(my, UnicensCmd_I2CWrite);
    if (UCS_I2C_RES_SUCCESS != result.code)
        UCSI_CB_OnUserMessage(my->tag, true, "Remote I2C Write to node=0x%X failed", 1, node_address);
}

static void OnUcsAmsWrite(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void *user_ptr)
{
    UCSI_Data_t *my = (UCSI_Data_t *)user_ptr;
    assert(MAGIC == my->magic);
    OnCommandExecuted(my, UnicensCmd_SendAmsMessage);
    if (UCS_AMSTX_RES_SUCCESS != result)
        UCSI_CB_OnUserMessage(my->tag, true, "SendAms failed with result=0x%x, info=0x%X", 2, result, info);
}

/************************************************************************/
/* Debug Message output from UNICENS stack:                             */
/************************************************************************/
#if defined(UCS_TR_ERROR) || defined(UCS_TR_INFO)
#include <stdio.h>
#define TRACE_BUFFER_SZ 200
void App_TraceError(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...)
{
    va_list argptr;
    char outbuf[TRACE_BUFFER_SZ];
    void *tag = NULL;
    UCSI_Data_t *my = (UCSI_Data_t *)ucs_user_ptr;
    if (my)
    {
        assert(MAGIC == my->magic);
        tag = my->tag;
    }
    va_start(argptr, vargs_cnt);
    vsprintf(outbuf, entry_str, argptr);
    va_end(argptr);
    UCSI_CB_OnUserMessage(tag, true, "Error | %s | %s", 2, module_str, outbuf);
}

void App_TraceInfo(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...)
{
    va_list argptr;
    char outbuf[TRACE_BUFFER_SZ];
    void *tag = NULL;
    UCSI_Data_t *my = (UCSI_Data_t *)ucs_user_ptr;
    if (my)
    {
        assert(MAGIC == my->magic);
        tag = my->tag;
    }
    va_start(argptr, vargs_cnt);
    vsprintf(outbuf, entry_str, argptr);
    va_end(argptr);
    UCSI_CB_OnUserMessage(tag, false, "Info | %s | %s", 2, module_str, outbuf);
}
#endif