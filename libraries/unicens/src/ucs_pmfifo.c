/*------------------------------------------------------------------------------------------------*/
/* UNICENS V2.1.0-3564                                                                            */
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

/*!
 * \file
 * \brief Implementation of Port Message FIFO
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMF
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pmfifo.h"
#include "ucs_pmp.h"
#include "ucs_pmcmd.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal macros                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
static const uint8_t     FIFO_SRV_PRIO              = 252U; /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
static const Srv_Event_t FIFO_SE_RX_SERVICE         = 1U;   /*!< \brief Event which triggers the Rx service */
static const Srv_Event_t FIFO_SE_TX_SERVICE         = 2U;   /*!< \brief Event which triggers the Rx service */
static const Srv_Event_t FIFO_SE_TX_APPLY_STATUS    = 4U;   /*!< \brief Event which triggers to apply the current INIC status */
static const Srv_Event_t FIFO_SE_ALL                = 7U;   /* parasoft-suppress  MISRA2004-8_7 "configuration property" */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Fifo_InitCounters(CPmFifo *self, uint8_t tx_sid_complete, uint8_t tx_credits);
static void Fifo_Service(void *self);

static void Fifo_RxService(CPmFifo *self);
static void Fifo_RxCheckStatusTrigger(CPmFifo *self);
static void Fifo_RxGetCredit(CPmFifo *self);
static void Fifo_RxReleaseCredit(CPmFifo *self);
static bool Fifo_RxProcessData(CPmFifo *self, CMessage *msg_ptr);
static void Fifo_RxProcessStatus(CPmFifo *self, CMessage *msg_ptr);
static void Fifo_RxProcessCommand(CPmFifo *self, CMessage *msg_ptr);
static void Fifo_RxProcessSyncStatus(CPmFifo *self, uint8_t sid, uint8_t type, uint8_t code, uint8_t *header_ptr);
static uint8_t Fifo_RxCheckFailureCode(CPmFifo *self, uint8_t failure_code);
static void Fifo_OnRx(void *self, CMessage *msg_ptr);

static void Fifo_TxService(CPmFifo *self);
static void Fifo_TxProcessData(CPmFifo *self);
static void Fifo_TxProcessStatus(CPmFifo *self);
static void Fifo_TxProcessCommand(CPmFifo *self);

static void Fifo_TxEnqueueBypassMsg(CPmFifo *self, CDlList *q_ptr, CMessage *msg_ptr);
static bool Fifo_FindFirstRegularMsg(void *d_ptr, void *ud_ptr);

static void Fifo_TxExecuteCancel(CPmFifo *self, uint8_t failure_sid, uint8_t failure_code);
static void Fifo_TxExecuteCancelAll(CPmFifo *self, uint8_t failure_sid, uint8_t failure_code);
static void Fifo_TxFinishedCancelAll(CPmFifo *self);
static uint8_t Fifo_TxPendingGetFollowerId(CPmFifo *self);
static void Fifo_TxCancelFollowers(CPmFifo *self, uint8_t follower_id, Ucs_MsgTxStatus_t status);

static bool Fifo_TxHasAccessPending(CPmFifo *self);
static void Fifo_TxRestorePending(CPmFifo *self);

static void Fifo_TxOnWatchdogTimer(void *self);
static void Fifo_TxStartWatchdog(CPmFifo *self);

static uint8_t Fifo_TxGetValidAcknowledges(CPmFifo *self, uint8_t sid);
static bool Fifo_TxNotifyStatus(CPmFifo *self, uint8_t sid, Ucs_MsgTxStatus_t status);
static void Fifo_TxApplyCurrentStatus(CPmFifo *self);
static void Fifo_TxUpdateCurrentStatus(CPmFifo *self, uint8_t sid, uint8_t type, uint8_t code);
static bool Fifo_TxIsIncomingSidValid(CPmFifo *self, uint8_t sid);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of message FIFO
 *  \param  self        The instance
 *  \param  init_ptr    Reference to initialization data
 *  \param  config_ptr  Reference to configuration
 */
void Fifo_Ctor(CPmFifo *self, const Fifo_InitData_t *init_ptr, const Fifo_Config_t *config_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->init          = *init_ptr;
    self->config        = *config_ptr;

    self->sync_state    = FIFO_S_UNSYNCED_INIT;                         /* initialize members */
    Sub_Ctor(&self->sync_state_subject, self->init.base_ptr->ucs_user_ptr);

    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Ctor(): state: %u", 1U, self->sync_state));

    Srv_Ctor(&self->service, FIFO_SRV_PRIO, self, &Fifo_Service);       /* registration of service */
    (void)Scd_AddService(&self->init.base_ptr->scd, &self->service);

    T_Ctor(&self->wd.timer);                                            /* setup watchdog */
    self->wd.timer_value = self->config.tx_wd_timer_value;
    Pmcmd_Ctor(&self->wd.wd_cmd, self->config.fifo_id, PMP_MSG_TYPE_CMD);
    Pmcmd_SetContent(&self->wd.wd_cmd, 0U, PMP_CMD_TYPE_REQ_STATUS, PMP_CMD_CODE_REQ_STATUS, NULL, 0U);

    /* init Rx part */
    Dl_Ctor(&self->rx.queue, self->init.base_ptr->ucs_user_ptr);
    self->rx.encoder_ptr = self->init.rx_encoder_ptr;
    self->rx.on_complete_fptr = self->init.rx_cb_fptr;
    self->rx.on_complete_inst = self->init.rx_cb_inst;

    self->rx.ack_threshold = self->config.rx_threshold;

    if (self->config.rx_threshold > self->config.rx_credits)/* configuration error - use single acknowledge */
    {
        self->rx.ack_threshold = 1U;
        TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");
    }

    self->rx.wait_processing = false;
    Pmcmd_Ctor(&self->rx.status, self->config.fifo_id, PMP_MSG_TYPE_STATUS);
    Pmcmd_SetContent(&self->rx.status, 0U, PMP_STATUS_TYPE_FLOW, PMP_STATUS_CODE_SUCCESS, NULL, 0U);

    /* init Tx part */
    Dl_Ctor(&self->tx.waiting_queue, self->init.base_ptr->ucs_user_ptr);
    Dl_Ctor(&self->tx.pending_q, self->init.base_ptr->ucs_user_ptr);

    Pmcmd_Ctor(&self->tx.cancel_cmd, self->config.fifo_id, PMP_MSG_TYPE_CMD);
    Pmcmd_SetContent(&self->tx.cancel_cmd, 0U, PMP_CMD_TYPE_MSG_ACTION, PMP_CMD_CODE_ACTION_CANCEL, NULL, 0U);

    Fifo_InitCounters(self, 0U, 0U);                        /* values are incremented on each sync attempt */
    self->tx.encoder_ptr = init_ptr->tx_encoder_ptr;

    /* FIFO synchronization command */
    self->sync_cnt = 0xFFU;
    self->sync_params[0] = config_ptr->rx_credits;
    self->sync_params[1] = config_ptr->rx_busy_allowed; 
    self->sync_params[2] = config_ptr->rx_ack_timeout;
    self->sync_params[3] = config_ptr->tx_wd_timeout;
    Pmcmd_Ctor(&self->tx.sync_cmd, self->config.fifo_id, PMP_MSG_TYPE_CMD);
    Pmcmd_SetContent(&self->tx.sync_cmd, 0U, PMP_CMD_TYPE_SYNCHRONIZATION, PMP_CMD_CODE_SYNC, self->sync_params, 4U);

    /* default PM header for Tx */
    self->tx.pm_header.pml = 6U;
    self->tx.pm_header.pmhl  = self->tx.encoder_ptr->pm_hdr_sz - 3U;
    Pmh_SetFph(&self->tx.pm_header, self->config.fifo_id, PMP_MSG_TYPE_DATA);
    self->tx.pm_header.sid = 0U;
    self->tx.pm_header.ext_type = (uint8_t)self->tx.encoder_ptr->content_type;

    Lldp_Ctor(&self->tx.lld_pool, self, self->init.base_ptr->ucs_user_ptr);

    Pmch_RegisterReceiver(self->init.channel_ptr, self->config.fifo_id, &Fifo_OnRx, self);
}

/*! \brief Initializes flow control and related counters
 *  \param  self            The instance
 *  \param  tx_sid_complete Reference to initialization data
 *  \param  tx_credits      Number of credits for Tx
 */
static void Fifo_InitCounters(CPmFifo *self, uint8_t tx_sid_complete, uint8_t tx_credits)
{
    self->rx.busy_num = 0U;
    self->rx.expected_sid = tx_sid_complete + 1U;
    self->rx.ack_last_ok_sid = tx_sid_complete;

    self->tx.credits = tx_credits;
    self->tx.sid_next_to_use = tx_sid_complete +1U;
    self->tx.sid_last_completed = tx_sid_complete;

    self->tx.failure_status = 0U;
    self->tx.failure_sid = 0U;

    self->tx.current_sid = tx_sid_complete;
    self->tx.current_type = PMP_STATUS_TYPE_FLOW;
    self->tx.current_code = (uint8_t)PMP_STATUS_CODE_SUCCESS;
}

/*! \brief          Adds an observer of synchronization state changes
 *  \param self     The instance
 *  \param obs_ptr  The observer. The notification result type is \ref Pmp_FifoId_t.
 */
void Fifo_AddStateObserver(CPmFifo *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->sync_state_subject, obs_ptr);
}

/*! \brief          Removes an observer of synchronization state changes
 *  \param self     The instance
 *  \param obs_ptr  The observer.
 */
void Fifo_RemoveStateObserver(CPmFifo *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->sync_state_subject, obs_ptr);
}

/*! \brief  Stops execution of a FIFO and notifies sync lost if necessary
 *  \param  self        The instance
 *  \param  new_state   The new synchronization state
 *  \param  allow_notification Set to \c false in order to avoid recursion
 */
void Fifo_Stop(CPmFifo *self, Fifo_SyncState_t new_state, bool allow_notification)
{
    bool notify = false; 

    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Stop(): FIFO: %u, state: %u, new_state: %u", 3U, self->config.fifo_id, self->sync_state, new_state));

    if (self->sync_state != new_state)
    {
        notify = true;
    }

    self->sync_state = new_state;
    self->tx.credits = 0U;

    if (self->wd.timer_value != 0U)
    {
        Tm_ClearTimer(&self->init.base_ptr->tm, &self->wd.timer);
    }

    if ((notify != false) && (allow_notification != false))
    {
        Sub_Notify(&self->sync_state_subject, &self->config.fifo_id);
    }
}

/*! \brief   Releases all external references
 *  \details It is important to call Fifo_Stop() prior to this functions. The low-level driver
 *           must be stopped as well to avoid concurrent access to message objects.
 *  \param   self   The instance
 */
void Fifo_Cleanup(CPmFifo *self)
{
    CMessage *msg_ptr = NULL;
    CDlNode *node_ptr = NULL;

    TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (self->sync_state == FIFO_S_UNSYNCED_INIT));
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Cleanup(): FIFO: %u", 1U, self->config.fifo_id));

    /* cleanup pending queue */
    for (node_ptr = Dl_PopHead(&self->tx.pending_q); node_ptr != NULL; node_ptr = Dl_PopHead(&self->tx.pending_q))
    {
        msg_ptr = (CMessage*)Dln_GetData(node_ptr);

        Msg_NotifyTxStatus(msg_ptr, UCS_MSG_STAT_ERROR_SYNC);
        Lldp_ReturnTxToPool(&self->tx.lld_pool, (Lld_IntTxMsg_t*)Msg_GetLldHandle(msg_ptr));
        Msg_SetLldHandle(msg_ptr, NULL);                    /* remove link to LLD message object */
    }

    /* cleanup waiting queue */
    for (node_ptr = Dl_PopHead(&self->tx.waiting_queue); node_ptr != NULL; node_ptr = Dl_PopHead(&self->tx.waiting_queue))
    {
        msg_ptr = (CMessage*)Dln_GetData(node_ptr);

        Msg_NotifyTxStatus(msg_ptr, UCS_MSG_STAT_ERROR_SYNC);
    }

    /* cleanup Rx queue */
    for (node_ptr = Dl_PopHead(&self->rx.queue); node_ptr != NULL; node_ptr = Dl_PopHead(&self->rx.queue))
    {
        msg_ptr = (CMessage*)Dln_GetData(node_ptr);

        Pmch_ReturnRxToPool(self->init.channel_ptr, msg_ptr);
    }

    Srv_ClearEvent(&self->service, FIFO_SE_ALL);
}


/*! \brief   Service function of FIFO
 *  \details The processing order of Rx followed by Tx is important for Fifo_RxProcessCommand()
 *  \param   self   The instance
 */
static void Fifo_Service(void *self)
{
    CPmFifo *self_ = (CPmFifo*)self;
    Srv_Event_t event_mask;

    Srv_GetEvent(&self_->service, &event_mask);

    if(FIFO_SE_RX_SERVICE == (event_mask & FIFO_SE_RX_SERVICE))     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->service, FIFO_SE_RX_SERVICE);
        Fifo_RxService(self_);
    }

    if((event_mask & FIFO_SE_TX_APPLY_STATUS) == FIFO_SE_TX_APPLY_STATUS)
    {
        Srv_ClearEvent(&self_->service, FIFO_SE_TX_APPLY_STATUS);
        Fifo_TxApplyCurrentStatus(self_);
    }

    if(FIFO_SE_TX_SERVICE == (event_mask & FIFO_SE_TX_SERVICE))     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->service, FIFO_SE_TX_SERVICE);
        Fifo_TxService(self_);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Tx Implementation                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Enqueues a message for transmission
 *  \param   self    The instance
 *  \param   msg_ptr The Tx message object
 *  \param   bypass  Use \c true if the message shall bypass all other messages
 *                   in the FIFO. Otherwise \c false.
 */
void Fifo_Tx(CPmFifo *self, CMessage *msg_ptr, bool bypass)
{
    uint8_t *msg_hdr_ptr = NULL;

    TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (msg_ptr != NULL));
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Tx(): FIFO: %u, msg_ptr: 0x%p, FuncId: 0x%X, queued Tx message", 3U, self->config.fifo_id, msg_ptr, msg_ptr->pb_msg.id.function_id));

    Msg_PullHeader(msg_ptr, self->tx.encoder_ptr->msg_hdr_sz);
    msg_hdr_ptr = Msg_GetHeader(msg_ptr);
    self->tx.encoder_ptr->encode_fptr(Msg_GetMostTel(msg_ptr), msg_hdr_ptr);

    if (bypass == false)
    {
        Dl_InsertTail(&self->tx.waiting_queue, Msg_GetNode(msg_ptr));       /* enqueue message for asynchronous transmission */
    }
    else
    {
        Fifo_TxEnqueueBypassMsg(self, &self->tx.waiting_queue, msg_ptr);    /* queue before first non-bypass message */
    }

    Srv_SetEvent(&self->service, FIFO_SE_TX_SERVICE);
}

/*! \brief   Enqueues a bypass message between the last bypass and the first regular message in a queue
 *  \param   self    The instance
 *  \param   q_ptr   The message queue
 *  \param   msg_ptr The Tx message object
 */
static void Fifo_TxEnqueueBypassMsg(CPmFifo *self, CDlList *q_ptr, CMessage *msg_ptr)
{
    CDlNode *node_ptr = Dl_Foreach(q_ptr, &Fifo_FindFirstRegularMsg, NULL); /* find first "non-bypass" message */
    Msg_SetTxBypass(msg_ptr, true);                                         /* mark new message as bypass message */

    if (node_ptr == NULL)                                                   /* no message or only bypass messages found */
    {                               
        Dl_InsertTail(&self->tx.waiting_queue, Msg_GetNode(msg_ptr));       /* enqueue message to tail */   
    }
    else                                                                    /* first "non-bypass" message is found */
    {                                                                       /* insert the bypass message before the first regular message found */
        Dl_InsertBefore(&self->tx.waiting_queue, node_ptr, Msg_GetNode(msg_ptr));
    }
}

/*! \brief   Required as "for-each" function to find the first "regular message"
 *  \param   d_ptr   Points to a message object in the queue
 *  \param   ud_ptr  Unused data reference, always \c NULL
 *  \return  Returns \c true if a regular (non-bypass) message is found.
 */
static bool Fifo_FindFirstRegularMsg(void *d_ptr, void *ud_ptr)
{
    bool ret = true;
    MISC_UNUSED(ud_ptr);

    if (Msg_IsTxBypass((CMessage*)d_ptr))
    {
        ret = false;
    }

    return ret;
}

/*! \brief   Processing of data, status and command messages
 *  \param   self    The instance
 */
static void Fifo_TxService(CPmFifo *self)
{
    Fifo_TxProcessCommand(self);
    Fifo_TxProcessStatus(self);
    Fifo_TxProcessData(self);
}

/*! \brief   Processing of status messages
 *  \param   self    The instance
 */
static void Fifo_TxProcessStatus(CPmFifo *self)
{
    if (Pmcmd_IsTriggered(&self->rx.status) != false)
    {
        if (Pmcmd_Reserve(&self->rx.status) != false)
        {
            Pmcmd_SetTrigger(&self->rx.status, false);
            self->rx.ack_last_ok_sid = (self->rx.expected_sid - self->rx.busy_num) - 1U;
            self->rx.wait_processing = false;

            if (self->rx.busy_num == 0U)                /* currently no processing of data messages active */
            {                                           /* notify the latest with SUCCESS */
                Pmcmd_UpdateContent(&self->rx.status, self->rx.expected_sid - 1U, PMP_STATUS_TYPE_FLOW, PMP_STATUS_CODE_SUCCESS);
            }
            else                                        /* message processing is active */
            {                                           /* notify code busy according to remaining credits */
                Pmcmd_UpdateContent(&self->rx.status, self->rx.expected_sid - self->rx.busy_num, PMP_STATUS_TYPE_FLOW, PMP_STATUS_CODE_BUSY);
            }

            Pmch_Transmit(self->init.channel_ptr, Pmcmd_GetLldTxObject(&self->rx.status));
        }
    }
}

/*! \brief   Processing of queued data messages
 *  \param   self    The instance
 */
static void Fifo_TxProcessData(CPmFifo *self)
{
    /* process all queued messages as long as credits are available,
     * process all queued messages if FIFO is not synced 
     */
    while ((self->tx.cancel_all_running == false) && (self->tx.credits > 0U))
    {
        CMessage *msg_ptr = NULL;
        CDlNode *node_ptr = NULL;
        uint8_t *msg_hdr_ptr = NULL;
        Lld_IntTxMsg_t *lld_tx_ptr = NULL;

        node_ptr = Dl_PopHead(&self->tx.waiting_queue);             /* get message node */
        if (node_ptr == NULL)
        {
            msg_ptr = NULL;                                         /* stop processing - no further messages in queue */
            break;
        }

        msg_ptr = (CMessage*)Dln_GetData(node_ptr);                 /* get message object */

        if (self->sync_state != FIFO_S_SYNCED)
        {
            Msg_NotifyTxStatus(msg_ptr, UCS_MSG_STAT_ERROR_SYNC);     /* notify sync error while not synced */
        }
        else
        {
            lld_tx_ptr = Lldp_GetTxFromPool(&self->tx.lld_pool);
            TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (msg_ptr != NULL));
            TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (lld_tx_ptr != NULL));
            TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxProcessData(): FIFO: %u, msg_ptr: 0x%p, FuncId: 0x%X, SID: 0x%02X, queued Tx message", 4U, self->config.fifo_id, msg_ptr, msg_ptr->pb_msg.id.function_id, self->tx.sid_next_to_use));

            Msg_SetLldHandle(msg_ptr, lld_tx_ptr);                     /* link message objects */
            lld_tx_ptr->msg_ptr = msg_ptr;

            Msg_PullHeader(msg_ptr, self->tx.encoder_ptr->pm_hdr_sz);  /* get PM header pointer */
            msg_hdr_ptr = Msg_GetHeader(msg_ptr);

            {
                uint8_t tel_length = Msg_GetMostTel(msg_ptr)->tel.tel_len;
                self->tx.pm_header.pml = (Msg_GetHeaderSize(msg_ptr) + tel_length) - 2U;
            }

            self->tx.pm_header.sid = self->tx.sid_next_to_use;        /* assign SeqID */
            self->tx.sid_next_to_use++;

            Pmh_BuildHeader(&self->tx.pm_header, msg_hdr_ptr);        /* build PM header */
            lld_tx_ptr->lld_msg.memory_ptr = Msg_GetMemTx(msg_ptr);

            Msg_SetTxActive(msg_ptr, true);
            Dl_InsertTail(&self->tx.pending_q, Msg_GetNode(msg_ptr));

            Pmch_Transmit(self->init.channel_ptr, (Ucs_Lld_TxMsg_t*)(void*)lld_tx_ptr);

            self->tx.credits--;
        }
    }
}

/*! \brief   Processing of status messages
 *  \param   self    The instance
 */
static void Fifo_TxProcessCommand(CPmFifo *self)
{
    if (Pmcmd_IsTriggered(&self->tx.sync_cmd) != false)
    {
        if (Pmcmd_Reserve(&self->tx.sync_cmd) != false)
        {
            Pmcmd_SetTrigger(&self->tx.sync_cmd, false);

            if (self->sync_state == FIFO_S_SYNCING)
            {
                self->sync_cnt++;
                Pmcmd_SetContent(&self->tx.sync_cmd, self->sync_cnt, PMP_CMD_TYPE_SYNCHRONIZATION, PMP_CMD_CODE_SYNC, self->sync_params, 4U);
                Pmch_Transmit(self->init.channel_ptr, Pmcmd_GetLldTxObject(&self->tx.sync_cmd));
            }
            else if (self->sync_state == FIFO_S_UNSYNCING)
            {
                Pmcmd_SetContent(&self->tx.sync_cmd, 0U, PMP_CMD_TYPE_SYNCHRONIZATION, PMP_CMD_CODE_UNSYNC, NULL, 0U);
                Pmch_Transmit(self->init.channel_ptr, Pmcmd_GetLldTxObject(&self->tx.sync_cmd));
            }
            else
            {
                Pmcmd_Release(&self->tx.sync_cmd);
                TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");
            }
        }
        else
        {
            TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");
        }
    }
}

/*! \brief      Releases a LLD Tx message object
 *  \param      self        The instance
 *  \param      handle_ptr  The unused LLD Tx message object 
 *  \details    If Fifo_TxApplyStatus() is waiting for a message object 
 *              being released 
 */
void Fifo_TxOnRelease(void *self, Ucs_Lld_TxMsg_t *handle_ptr)
{
    CPmFifo *self_ = (CPmFifo*)self;
    Lld_IntTxMsg_t *tx_ptr = (Lld_IntTxMsg_t*)(void*)handle_ptr;

    if (tx_ptr->msg_ptr != NULL)
    {
        Msg_SetTxActive(tx_ptr->msg_ptr, false);
    }
    else
    {
        TR_FAILED_ASSERT(self_->init.base_ptr->ucs_user_ptr, "[FIFO]");
    }

    if (self_->tx.status_waiting_release != false)
    {
        self_->tx.status_waiting_release = false;
        Srv_SetEvent(&self_->service, (FIFO_SE_TX_APPLY_STATUS | FIFO_SE_TX_SERVICE));
    }
}

/*! \brief   Triggers a command CANCEL_ALL and stops further Tx processing
 *  \details CANCEL_ALL shall be called only, if the front-most pending message 
 *           has followers (is segmented, i.e. \c cancel_id > 0). Use command CANCEL
 *           if the front-most message has no followers (\c cancel_id == NULL).
 *  \param   self           The instance
 *  \param   failure_sid    The failure sid
 *  \param   failure_code   The failure code reported by the INIC
 */
static void Fifo_TxExecuteCancelAll(CPmFifo *self, uint8_t failure_sid, uint8_t failure_code)
{
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxExecuteCancelAll(): FIFO: %u, SID: %u, Code: %u", 3U, self->config.fifo_id, failure_sid, failure_code));

    if (Pmcmd_Reserve(&self->tx.cancel_cmd) != false)                   /* prepare cancel command */
    {
        Pmcmd_UpdateContent(&self->tx.cancel_cmd, self->tx.current_sid, 
                            PMP_CMD_TYPE_MSG_ACTION, PMP_CMD_CODE_ACTION_CANCEL_ALL);
        Pmch_Transmit(self->init.channel_ptr, Pmcmd_GetLldTxObject(&self->tx.cancel_cmd));
    }
    else
    {
        TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");   /* Unable to reserve cancel command */
    }

    self->tx.cancel_all_running = true;
    self->tx.failure_sid = failure_sid;
    self->tx.failure_status = failure_code;
}

/*! \brief   Shall be called if the command CANCEL_ALL was processed completely
 *  \param   self           The instance
 *  \details Since the CANCEL_ALL is used to cancel the front-most message and
 *           all of its followers (same cancel_id)

 for mid-level retries, the canceled messages
 *           are moved from the processing_q to the waiting_q again. The MLR timer is
 *           started. As soon as the timer elapses, Tx processing is continued again.
 *           If the front-most message has a follower id, all pending messages are 
 *           moved to the waiting queue and all messages with the same follower id 
 *           are notified as failed.
 */
static void Fifo_TxFinishedCancelAll(CPmFifo *self)
{
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxFinishedCancelAll(): FIFO: %u, FailureStatus: %u,", 2U, self->config.fifo_id, self->tx.failure_status));

    if (self->tx.failure_status != 0U)                          /* avoid multiple execution of the same CANCELED status */
    {                                                           /* and all of its followers */
        uint8_t follower_id = Fifo_TxPendingGetFollowerId(self);
        Fifo_TxRestorePending(self);                            /* move remaining messages to waiting_q */
        Fifo_TxCancelFollowers(self, follower_id, (Ucs_MsgTxStatus_t)self->tx.failure_status);
                                                                /* notify front-most and message and all of its followers */
        self->tx.cancel_all_running = false;                    /* continue with Tx processing */
        self->tx.failure_sid = 0U;
        self->tx.failure_status = 0U;
        Srv_SetEvent(&self->service, FIFO_SE_TX_SERVICE);
    }
}

/*! \brief   Triggers a command CANCEL while Tx processing continues
 *  \param   self           The instance
 *  \param   failure_sid    The failure sid
 *  \param   failure_code   The failure code reported by the INIC
 */
static void Fifo_TxExecuteCancel(CPmFifo *self, uint8_t failure_sid, uint8_t failure_code)
{
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxExecuteCancel(): FIFO: %u, SID: %u, Code: %u", 3U, self->config.fifo_id, failure_sid, failure_code));

    if (Pmcmd_Reserve(&self->tx.cancel_cmd) != false)
    {
        Pmcmd_UpdateContent(&self->tx.cancel_cmd, self->tx.current_sid, 
                            PMP_CMD_TYPE_MSG_ACTION, PMP_CMD_CODE_ACTION_CANCEL);
        Pmch_Transmit(self->init.channel_ptr, Pmcmd_GetLldTxObject(&self->tx.cancel_cmd));
    }
    else
    {
        TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");   /* Unable to reserve cancel command */
    }

    self->tx.cancel_all_running = false;
    self->tx.failure_sid = failure_sid;
    self->tx.failure_status = failure_code;
}

/*! \brief   Checks if the LLD has released all messages in the pending_q
 *  \param   self           The instance
 *  \return  Returns \c true if all messages are released by the LLD, otherwise \c false.
 */
static bool Fifo_TxHasAccessPending(CPmFifo *self)
{
    bool ret = true;
    CDlNode *node_ptr = Dl_PeekTail(&self->tx.pending_q);           /* if the tail is not active, then all */
                                                                    /* pending message are not active */
    if (node_ptr != NULL)
    {
        CMessage *msg_ptr = (CMessage*)Dln_GetData(node_ptr);

        if (Msg_IsTxActive(msg_ptr) != false)
        {
            TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxHasAccessPending(): FIFO: %u, msg_ptr: 0x%p, still in use", 2U, self->config.fifo_id, msg_ptr));
            self->tx.status_waiting_release = true;
            ret = false;
        }
    }

    return ret;
}

/*! \brief   Moves all pending messages to the waiting_q
 *  \details All messages from pending_q will be moved to the waiting_g and 
 *           all consumed credits are restored. The message objects are restored
 *           to the queue in the same order as they have been forwarded to the LLD.
 *           This method is typically called to restore the waiting_q in the correct
 *           order before notifying a 
 *  \param   self           The instance
 */
static void Fifo_TxRestorePending(CPmFifo *self)
{
    /* take tail from pending_q to the head of waiting_q */
    CMessage *msg_ptr = NULL;
    CDlNode *node_ptr = NULL;

    /* cleanup pending queue */
    for (node_ptr = Dl_PopTail(&self->tx.pending_q); node_ptr != NULL; node_ptr = Dl_PopTail(&self->tx.pending_q))
    {
        msg_ptr = (CMessage*)Dln_GetData(node_ptr);

        TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxRestorePending(): FIFO: %u, msg_ptr: 0x%p", 2U, self->config.fifo_id, msg_ptr));
        TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (Msg_IsTxActive(msg_ptr) == false));

        self->tx.sid_last_completed++;
        self->tx.credits++;
        Lldp_ReturnTxToPool(&self->tx.lld_pool, (Lld_IntTxMsg_t*)Msg_GetLldHandle(msg_ptr));
        Msg_SetLldHandle(msg_ptr, NULL);                            /* remove link to LLD message object */
        Msg_PushHeader(msg_ptr, self->tx.encoder_ptr->pm_hdr_sz);   /* set index to position of message header */
        Dl_InsertHead(&self->tx.waiting_queue, node_ptr);           /* enqueue message to waiting_q */
    }
}

/*! \brief   Retrieves the follower id of the front-most pending message
 *  \param   self           The instance
 *  \return  Returns the follower id of the front-most pending message.
 */
static uint8_t Fifo_TxPendingGetFollowerId(CPmFifo *self)
{
    CDlNode *node_ptr;
    CMessage *tx_ptr;
    uint8_t ret = 0U;

    node_ptr = Dl_PeekHead(&self->tx.pending_q);
    TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (node_ptr != NULL));

    if (node_ptr != NULL)
    {
        tx_ptr = (CMessage*)Dln_GetData(node_ptr);
        ret = tx_ptr->pb_msg.opts.cancel_id;
    }

    return ret;
}

/*! \brief  Aborts the transmission of all messages in the waiting_q with a given follower id
 *  \param  self          The instance
 *  \param  follower_id   The follower id a message needs to have to be canceled
 *  \param  status        The transmission status that shall be notified 
 */
static void Fifo_TxCancelFollowers(CPmFifo *self, uint8_t follower_id, Ucs_MsgTxStatus_t status)
{
    CDlNode *node_ptr;
    CDlList temp_queue;

    Dl_Ctor(&temp_queue, self->init.base_ptr->ucs_user_ptr);
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxCancelFollowers(): FIFO: %u: FollowerId: %u", 2U, self->config.fifo_id, follower_id));

    for (node_ptr = Dl_PopHead(&self->tx.waiting_queue); node_ptr != NULL; node_ptr = Dl_PopHead(&self->tx.waiting_queue))
    {
        CMessage *tx_ptr = (CMessage*)Dln_GetData(node_ptr);

        TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (Msg_GetLldHandle(tx_ptr) == NULL));

        if (tx_ptr->pb_msg.opts.cancel_id == follower_id)
        {
            Msg_NotifyTxStatus(tx_ptr, status);             /* notify failed transmission of message and all followers */
        }
        else
        {
            Dl_InsertTail(&temp_queue, node_ptr);           /* add to temporary queue and keep order of messages */
        }
    }

    if (Dl_GetSize(&temp_queue) > 0U)                       /* restore temp_queue to waiting_q */
    {
        Dl_AppendList(&self->tx.waiting_queue, &temp_queue);/* temp_queue will be empty now */
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Tx Message Processing                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Retrieves the number of (implicit) acknowledges that are related to one SID
 *  \param  self    The instance
 *  \param  sid     The sequence ID
 *  \return The number of implicit acknowledges that are related to the SID
 */
static uint8_t Fifo_TxGetValidAcknowledges(CPmFifo *self, uint8_t sid)
{
    uint8_t diff_s = (uint8_t)(sid - self->tx.sid_last_completed);                          /* number of implicit acknowledged data */
    uint8_t diff_b = (uint8_t)(self->tx.sid_next_to_use - self->tx.sid_last_completed);     /* number of "sent but un-acknowledged data" + 1 */

    if (diff_b <= diff_s)                                                                   /* check valid acknowledges */
    {
        diff_s = 0U;
    }

    return diff_s;
}


/*! \brief  Checks id an incoming SID of a status message is valid.
 *  \param  self The instance
 *  \param  sid  The sequence ID
 *  \return Returns \c true if the SID is valid, otherwise \c false.
 */
static bool Fifo_TxIsIncomingSidValid(CPmFifo *self, uint8_t sid)
{
    bool ret = false;
    uint8_t diff_s = (uint8_t)(sid - self->tx.sid_last_completed);                          /* number of implicit acknowledged data */
    uint8_t diff_b = (uint8_t)(self->tx.sid_next_to_use - self->tx.sid_last_completed);     /* number of "sent but un-acknowledged data" + 1 */
    uint8_t diff_p = (uint8_t)(self->tx.current_sid - self->tx.sid_last_completed);         /* pending/known acknowledges */

    if (diff_b > diff_s)                                                                    /* check if SID fits in valid range */
    {
        if (diff_s >= diff_p)                                                               /* avoid overwriting with smaller values */
        {
            ret = true;
        }
    }

    return ret;
}

/*! \brief  Implicitly notifies transmission status to calling classes
 *  \param  self    The instance
 *  \param  sid     The sequence ID until the status shall be notified
 *  \param  status  The status which is notified
 *  \return Returns \c true if all desired messages had been notified, 
 *          otherwise \c false.
 */
static bool Fifo_TxNotifyStatus(CPmFifo *self, uint8_t sid, Ucs_MsgTxStatus_t status)
{
    bool ret = true;
    uint8_t acks = Fifo_TxGetValidAcknowledges(self, sid);

    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxNotifyStatus(): FIFO: %u, calculated_acks: %u", 2U, self->config.fifo_id, acks));

    while (acks > 0U)
    {
        CDlNode *node_ptr = Dl_PopHead(&self->tx.pending_q);

        if (node_ptr != NULL)
        {
            CMessage *tx_ptr = (CMessage*)node_ptr->data_ptr;

            if (!Msg_IsTxActive(tx_ptr))
            {
                TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (tx_ptr != NULL));
                TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxNotifyStatus(): FIFO: %u, FuncId: 0x%X, notified status: %u", 3U, self->config.fifo_id, tx_ptr->pb_msg.id.function_id, status));
                Msg_NotifyTxStatus(tx_ptr, status);
                Lldp_ReturnTxToPool(&self->tx.lld_pool, (Lld_IntTxMsg_t*)Msg_GetLldHandle(tx_ptr));
                Msg_SetLldHandle(tx_ptr, NULL);                             /* remove link to LLD message object */

                self->tx.credits++;                                         /* increment credits */
                self->tx.sid_last_completed++;                              /* update last acknowledge SID */
            }
            else
            {
                TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxNotifyStatus(): FIFO: %u, LLD objects still occupied", 1U, self->config.fifo_id));
                Dl_InsertHead(&self->tx.pending_q, node_ptr);
                self->tx.status_waiting_release = true;
                ret = false;
                break;
            }
        }
        else
        {
            TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");   /* not yet handled */
                                                                            /* trigger sync again */
        }

        acks--;
    }

    return ret;
}

/*! \brief  Updates the current Tx status with the content of a received FIFO status
 *  \param  self    The instance
 *  \param  sid     The sequence id of the FIFO status
 *  \param  type    The type of the FIFO status. Valid types are only:
 *                  - PMP_STATUS_TYPE_FLOW
 *                  - PMP_STATUS_TYPE_FAILURE
 *  \param  code    The code of the FIFO status
 */
static void Fifo_TxUpdateCurrentStatus(CPmFifo *self, uint8_t sid, uint8_t type, uint8_t code)
{
    TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (type == (uint8_t)PMP_STATUS_TYPE_FAILURE) || (type == (uint8_t)PMP_STATUS_TYPE_FLOW));
    if (Fifo_TxIsIncomingSidValid(self, sid))               /* is new or updating status */
    {
        self->tx.current_sid = sid;                         /* update current status */
        self->tx.current_type = (Pmp_StatusType_t)type;
        self->tx.current_code = code;
    }
    else
    {
        TR_ERROR((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxUpdateCurrentStatus(): FIFO: %u, sid: %u, type: %u, code: %u, INVALID SID", 4U, self->config.fifo_id, sid, type, code));
    }
}

/*! \brief  Analyses the current Tx status, tries to notify statuses to the transmitter and triggers
 *          retry/cancel actions.
 *  \param  self    The instance
 */
static void Fifo_TxApplyCurrentStatus(CPmFifo *self)
{
    if ((self->tx.cancel_all_running == false) && (self->tx.failure_status != 0U))      /* Command(CANCEL) is pending */
    {
        if (Fifo_TxGetValidAcknowledges(self, self->tx.current_sid) > 1U)               /* ?>=1? "single cancel" is valid and implicit */
        {
            if (Fifo_TxNotifyStatus(self, self->tx.failure_sid, (Ucs_MsgTxStatus_t)self->tx.failure_status))
            {
                self->tx.failure_status = 0U;                                           /* implicit canceled stops retries */
                self->tx.failure_sid = 0U;
            }
        }
    }

    if ((self->tx.current_type == PMP_STATUS_TYPE_FAILURE) && (self->tx.status_waiting_release == false))
    {
        if (self->tx.cancel_all_running == false)
        {
            if (Fifo_TxNotifyStatus(self, self->tx.current_sid - 1U, UCS_MSG_STAT_OK) != false)
            {
                /* important: failed message now is front-most message in the tx.pending_q, */
                /*            any implicit acknowledge was done before */
                if (self->tx.failure_status == 0U)                  /* failure not yet handled - avoid multiple calls */
                {
                    if (Fifo_TxPendingGetFollowerId(self) == 0U)
                    {
                        Fifo_TxExecuteCancel(self, self->tx.current_sid, self->tx.current_code);    /* execute simple cancel */
                    }
                    else
                    {
                        Fifo_TxExecuteCancelAll(self, self->tx.current_sid, self->tx.current_code); /* execute cancel all */
                        /* self->tx.cancel_all_running now is 'true' and Tx is stopped */
                    }
                }
            }
        }
    }

    if ((self->tx.current_type == PMP_STATUS_TYPE_FLOW) && (self->tx.status_waiting_release == false))
    {
        if ((uint8_t)PMP_STATUS_CODE_SUCCESS == self->tx.current_code)                           /* acknowledge pending messages */
        {
                                                                   /* no further retries possible */
            (void)Fifo_TxNotifyStatus(self, self->tx.current_sid, UCS_MSG_STAT_OK);
        }
        else if ((uint8_t)PMP_STATUS_CODE_CANCELED == self->tx.current_code)
        {
            if (self->tx.cancel_all_running != false)
            {
                /* wait until the last SID is notified */
                if (self->tx.current_sid == (uint8_t)(self->tx.sid_next_to_use - (uint8_t)1U))
                {
                    /* cancel done if none of pending messages is active */
                    if (Fifo_TxHasAccessPending(self) != false)
                    {
                        Fifo_TxFinishedCancelAll(self);
                    }
                }
            }
            else if (Fifo_TxNotifyStatus(self, self->tx.current_sid, (Ucs_MsgTxStatus_t)self->tx.failure_status))
            {
                self->tx.failure_status = 0U;
                self->tx.failure_sid = 0U;
            }
        }
        else 
        {
            if (Fifo_TxNotifyStatus(self, self->tx.current_sid - 1U, UCS_MSG_STAT_OK)) /* just implicitly acknowledge preceding message */
            {
                if ((uint8_t)PMP_STATUS_CODE_NACK == self->tx.current_code)
                {
                    Fifo_Stop(self, FIFO_S_UNSYNCED_INIT, true);
                    TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Rx Implementation                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Receives a message on the respective FIFO 
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the Rx message
 */
static void Fifo_OnRx(void *self, CMessage *msg_ptr)
{
    CPmFifo *self_ = (CPmFifo*)self;
    Dl_InsertTail(&self_->rx.queue, Msg_GetNode(msg_ptr));      /* enqueue in rx_queue */
    Srv_SetEvent(&self_->service, (FIFO_SE_RX_SERVICE | FIFO_SE_TX_APPLY_STATUS | FIFO_SE_TX_SERVICE));
}

/*! \brief  Processes the Rx queue completely and triggers possible Tx events
 *  \param  self    The instance
 */
static void Fifo_RxService(CPmFifo *self)
{
    while (self->rx.wait_processing == false)                    /* process all Rx messages if possible */
    {
        CMessage *msg_ptr;
        uint8_t *header_ptr;
        Pmp_MsgType_t type;
        bool ok;

        bool free_msg = true;                                   /* default: free every status or command message */
        CDlNode *node_ptr = Dl_PopHead(&self->rx.queue);

        if (node_ptr == NULL)
        {
            msg_ptr = NULL;                                     /* stop processing - no further messages in queue */
            break;
        }

        msg_ptr = (CMessage*)node_ptr->data_ptr;
        header_ptr = Msg_GetHeader(msg_ptr);
        type = Pmp_GetMsgType(header_ptr);
        ok = Pmp_VerifyHeader(header_ptr, MSG_SIZE_RSVD_BUFFER);

        if (ok != false)
        {
            switch (type)
            {
                case PMP_MSG_TYPE_CMD:
                    Fifo_RxProcessCommand(self, msg_ptr);
                    break;
                case PMP_MSG_TYPE_STATUS:
                    Fifo_RxProcessStatus(self, msg_ptr);
                    break;
                case PMP_MSG_TYPE_DATA:
                    free_msg = Fifo_RxProcessData(self, msg_ptr);                   /* important: message can be freed */
                    break;                                                          /*            synchronously */
                default:
                    TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");   /* unknown FIFO message type */
                    break;
            }
        }
        else
        {
            TR_FAILED_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]");           /* invalid message header */
        }

        if (free_msg != false)
        {
            Pmch_ReturnRxToPool(self->init.channel_ptr, msg_ptr);
        }
    }
}

/*! \brief   Evaluates the trigger condition to transmit a Rx status
 *  \details Needs to be called before and after processing Rx data messages
 *  \param   self    The instance
 */
static void Fifo_RxCheckStatusTrigger(CPmFifo *self)
{
    /* calculate the number of credits the INIC has consumed */
    /* if less messages are processing, the freed can be acknowledged */
    uint8_t consumed_inic_credits = (self->rx.expected_sid - self->rx.ack_last_ok_sid) - 1U;
    uint8_t possible_acks = consumed_inic_credits - self->rx.busy_num;

    if ((consumed_inic_credits >= self->rx.ack_threshold) && (possible_acks > 0U))
    {
        if (Pmcmd_IsTriggered(&self->rx.status) == false)
        {
            Pmcmd_SetTrigger(&self->rx.status, true);       /* INIC might run out of credits */ 
            Srv_SetEvent(&self->service, FIFO_SE_TX_SERVICE);
        }
    }
}

/*! \brief  This function shall be called before processing a valid FIFO data message
 *  \param  self    The instance
 */
static void Fifo_RxGetCredit(CPmFifo *self)
{
    self->rx.busy_num++;
    Fifo_RxCheckStatusTrigger(self);
}

/*! \brief   This function shall be called after processing a valid FIFO data message
 *  \details It is important to call this function after the message object is freed,
 *           so that the flow control can be updated.
 *  \param   self    The instance
 */
static void Fifo_RxReleaseCredit(CPmFifo *self)
{
    self->rx.busy_num--;
    Fifo_RxCheckStatusTrigger(self);
}

/*! \brief   Releases a FIFO data message which was received and forwarded by the FIFO
 *  \details The function returns the message to the channel's Rx message pool and
 *           has to update the number of credits (processing handles). 
 *           A FIFO data message is initially allocated from the channel's Rx message pool.
 *           When processing the handle the determined FIFO need to calculate the amount of 
 *           credits. When freeing the message the handle needs to be returned to the channel's
 *           Rx pool again and the FIFO needs to refresh the status and credits calculation.
 *           Therefore the message has to be freed to the respective FIFO again.
 *  \param   self    The instance
  * \param  msg_ptr The Rx data message
 */
void Fifo_RxReleaseMsg(CPmFifo *self, CMessage *msg_ptr)
{
    Pmch_ReturnRxToPool(self->init.channel_ptr, msg_ptr);
    Fifo_RxReleaseCredit(self);
}

/*! \brief  Processes an Rx data message
 *  \param  self    The instance
 *  \param  msg_ptr The Rx data message
 *  \return \c true if the message object is no longer needed.
 *          Otherwise \c false.
 */
static bool Fifo_RxProcessData(CPmFifo *self, CMessage *msg_ptr)
{
    bool free_msg = true;
    uint8_t content_header_sz = 0U;
    uint8_t sid = 0U;
    uint8_t *header_ptr = Msg_GetHeader(msg_ptr);
    sid = Pmp_GetSid(header_ptr);

    if (self->sync_state != FIFO_S_SYNCED)
    {                                                       /* discard Rx messages while FIFO is not synced */
        TR_ERROR((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_RxProcessData(): FIFO: %u, state: %u, discards Rx message with SID=0x%02X while not synced (warning)", 3U, self->config.fifo_id, self->sync_state, sid));
    }
    else if (sid == self->rx.expected_sid)                       /* check if SID is ok */
    {
        uint8_t pm_header_sz = Pmp_GetPmhl(header_ptr) + 3U;
        TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (pm_header_sz == self->rx.encoder_ptr->pm_hdr_sz));

        self->rx.expected_sid++;                            /* update SID */
        content_header_sz = self->rx.encoder_ptr->msg_hdr_sz;

        /* parasoft suppress item MISRA2004-17_4 reason "necessary offset usage" */
        self->rx.encoder_ptr->decode_fptr(Msg_GetMostTel(msg_ptr), &(header_ptr[pm_header_sz]));
        /* parasoft unsuppress item MISRA2004-17_4 reason "necessary offset usage" */

        Msg_ReserveHeader(msg_ptr, content_header_sz + pm_header_sz);
        Msg_PullHeader(msg_ptr, content_header_sz + pm_header_sz);

        if (Msg_VerifyContent(msg_ptr))
        {
            if (self->rx.on_complete_fptr != NULL)
            {
                (void)Fifo_RxGetCredit(self);
                free_msg =  false;                              /* callback is responsible to free the message  */
                self->rx.on_complete_fptr(self->rx.on_complete_inst, msg_ptr);
                                                                /* Fifo_RxReleaseCredit() is called when message is freed */
            }
        }
    }
    else
    {
        TR_ERROR((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_RxProcessData(): FIFO: %u, state: %u, discards Rx message with unexpected SID=0x%02X (warning)", 3U, self->config.fifo_id, self->sync_state, sid));
    }

    return free_msg;
}

/*! \brief  Processes an Rx status message
 *  \param  self    The instance
 *  \param  msg_ptr The Rx status message
 */
static void Fifo_RxProcessStatus(CPmFifo *self, CMessage *msg_ptr)
{
    CPmh pm_header;
    uint8_t current_sid;
    uint8_t current_type;
    uint8_t current_code;
    uint8_t *header_ptr = Msg_GetHeader(msg_ptr);

    Pmh_DecodeHeader(&pm_header, header_ptr);
    current_sid = pm_header.sid;
    current_type = (uint8_t)Pmh_GetExtStatusType(&pm_header);
    current_code = (uint8_t)Pmh_GetExtStatusCode(&pm_header);

    self->wd.request_started = false;                                                   /* status finishes a wd request */

    switch ((Pmp_StatusType_t)current_type)
    {
        case PMP_STATUS_TYPE_FAILURE:
            Fifo_TxUpdateCurrentStatus(self, current_sid, current_type, Fifo_RxCheckFailureCode(self, current_code));  /* just update status type FAILURE */
            break;
        case PMP_STATUS_TYPE_FLOW:
            Fifo_TxUpdateCurrentStatus(self, current_sid, current_type, current_code);  /* just update status type FLOW (codes: BUSY, NACK, SUCCESS, CANCELED) */
            break;
        case PMP_STATUS_TYPE_SYNCED:
            Fifo_RxProcessSyncStatus(self, current_sid, current_type, current_code, header_ptr);
            break;
        case PMP_STATUS_TYPE_UNSYNCED_BSY:
            TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_RxProcessStatus(): FIFO: %u, state: %u, received UNSYNCED_BSY", 2U, self->config.fifo_id, self->sync_state));
            if (self->sync_state != FIFO_S_SYNCING)
            {
                Fifo_Stop(self, FIFO_S_UNSYNCED_BUSY, true);
            }
            break;
        case PMP_STATUS_TYPE_UNSYNCED_RDY:
            TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_RxProcessStatus(): FIFO: %u, state: %u, received UNSYNCED_RDY", 2U, self->config.fifo_id, self->sync_state));
            if (self->sync_state == FIFO_S_SYNCING)
            {
                if (current_code == (uint8_t)PMP_UNSYNC_R_COMMAND)
                {
                    Fifo_Synchronize(self);                 /* retry synchronization */
                }
            }
            else
            {
                Fifo_Stop(self, FIFO_S_UNSYNCED_READY, true);
            }
            break;
        default:
            /* ignore status */
            TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_RxProcessStatus(): FIFO: %u, state: %u, received UNKNOWN TYPE: %u", 3U, self->config.fifo_id, self->sync_state, current_type));
            break;
    }
}

/*! \brief  Checks failure_code and sets invalid code to UCS_MSG_STAT_ERROR_UNKNOWN
 *  \param  self         The instance
 *  \param  failure_code The INIC failure code
 *  \return Returns the checked failure code
 */
static uint8_t Fifo_RxCheckFailureCode(CPmFifo *self, uint8_t failure_code)
{
    uint8_t ret;
    MISC_UNUSED(self);

    switch (failure_code)
    {
        case (uint8_t)UCS_MSG_STAT_ERROR_CFG_NO_RCVR:
        case (uint8_t)UCS_MSG_STAT_ERROR_BF:
        case (uint8_t)UCS_MSG_STAT_ERROR_CRC:
        case (uint8_t)UCS_MSG_STAT_ERROR_ID:
        case (uint8_t)UCS_MSG_STAT_ERROR_ACK:
        case (uint8_t)UCS_MSG_STAT_ERROR_TIMEOUT:
        case (uint8_t)UCS_MSG_STAT_ERROR_FATAL_WT:
        case (uint8_t)UCS_MSG_STAT_ERROR_FATAL_OA:
        case (uint8_t)UCS_MSG_STAT_ERROR_NA_TRANS:
        case (uint8_t)UCS_MSG_STAT_ERROR_NA_OFF:
            ret = failure_code;
            break;
        default:
            ret = (uint8_t)UCS_MSG_STAT_ERROR_UNKNOWN;
            break;
    }

    return ret;
}

/*! \brief  Processes an Rx command message
 *  \param  self    The instance
 *  \param  msg_ptr The Rx command message
 */
static void Fifo_RxProcessCommand(CPmFifo *self, CMessage *msg_ptr)
{
    MISC_UNUSED(msg_ptr);
                                                             /* be aware that PMHL might vary */
    Pmcmd_SetTrigger(&self->rx.status, true);                /* just trigger latest Rx status now */
}

/*! \brief  Processes a status SYNCED from the INIC
 *  \param  self        The instance
 *  \param  sid         The sid of the sync status
 *  \param  type        The type of the sync status
 *  \param  code        The code of the sync status
 *  \param  header_ptr  Pointer to the raw port message
 *  \return The current synchronization state
 */
static void Fifo_RxProcessSyncStatus(CPmFifo *self, uint8_t sid, uint8_t type, uint8_t code, uint8_t *header_ptr)
{
    bool check = false;
    uint8_t tx_credits = 0U;

    TR_ASSERT(self->init.base_ptr->ucs_user_ptr, "[FIFO]", (type==(uint8_t)PMP_STATUS_TYPE_SYNCED));
    MISC_UNUSED(type);
    MISC_UNUSED(code);

    if (Pmp_GetDataSize(header_ptr) == 4U)
    {
        tx_credits = Pmp_GetData(header_ptr, 0U) & (uint8_t)PMP_CREDITS_MASK;

        if ((tx_credits >= PMP_CREDITS_MIN) && 
            (Pmp_GetData(header_ptr, 1U) == self->sync_params[1]) &&
            (Pmp_GetData(header_ptr, 2U) == self->sync_params[2]) &&
            (Pmp_GetData(header_ptr, 3U) == self->sync_params[3]) &&
            (sid == (self->sync_cnt)))
        {
            check = true;                                               /* the sync status parameters are correct */
        }
    }

    if ((check != false) && (self->sync_state == FIFO_S_SYNCING))
    {
        Fifo_InitCounters(self, sid, tx_credits);                       /* values are incremented on each sync attempt */
        self->sync_state = FIFO_S_SYNCED;                               /* sync status shall have 4 bytes message body */
        self->rx.wait_processing = false;
        Fifo_TxStartWatchdog(self);
        Sub_Notify(&self->sync_state_subject, &self->config.fifo_id);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Synchronization                                                                                */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Synchronizes the FIFO
 *  \param  self    The instance
 */
void Fifo_Synchronize(CPmFifo *self)
{
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Synchronize(): FIFO: %u, state: %u", 2U, self->config.fifo_id, self->sync_state));
    self->sync_state           = FIFO_S_SYNCING;
    Pmcmd_SetTrigger(&self->tx.sync_cmd, true);
    Srv_SetEvent(&self->service, FIFO_SE_TX_SERVICE);
}

/*! \brief  Un-synchronizes the FIFO
 *  \param  self    The instance
 */
void Fifo_Unsynchronize(CPmFifo *self)
{
    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_Unsynchronize(): FIFO: %u, state: %u", 2U, self->config.fifo_id, self->sync_state));
    if (self->sync_state != FIFO_S_UNSYNCED_READY)
    {
        self->sync_state           = FIFO_S_UNSYNCING;
        Pmcmd_SetTrigger(&self->tx.sync_cmd, true);
        Srv_SetEvent(&self->service, FIFO_SE_TX_SERVICE);
    }
}

/*! \brief  Retrieves the current synchronization state
 *  \param  self    The instance
 *  \return The current synchronization state
 */
Fifo_SyncState_t Fifo_GetState(CPmFifo *self)
{
    return self->sync_state;
}

/*------------------------------------------------------------------------------------------------*/
/* Watchdog                                                                                       */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Starts the watchdog handling 
 *  \param  self    The instance
 */
static void Fifo_TxStartWatchdog(CPmFifo *self)
{
    self->wd.request_started = false;

    TR_INFO((self->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxStartWatchdog(): fifo_id: %u, timeout: %u", 2U, self->config.fifo_id, self->wd.timer_value));

    if (self->wd.timer_value != 0U)
    {
        Tm_SetTimer(&self->init.base_ptr->tm, &self->wd.timer, &Fifo_TxOnWatchdogTimer, 
                    self, 
                    self->wd.timer_value, 
                    self->wd.timer_value
                    );
    }
}

/*! \brief   Callback function which is invoked if the watchdog timer expires 
 *  \param   self    The instance
 */
static void Fifo_TxOnWatchdogTimer(void *self)
{
    CPmFifo *self_ = (CPmFifo*)self;

    TR_INFO((self_->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxOnWatchdogTimer(): FIFO: %u, state: %u", 2U, self_->config.fifo_id, self_->sync_state));

    if (self_->wd.request_started == false)
    {
        if (Pmcmd_Reserve(&self_->wd.wd_cmd) != false)
        {
            self_->wd.request_started = true;       /* indicate that a status is expected */
            Pmcmd_UpdateContent(&self_->wd.wd_cmd, self_->tx.sid_next_to_use - 1U, PMP_CMD_TYPE_REQ_STATUS, PMP_CMD_CODE_REQ_STATUS);
            Pmch_Transmit(self_->init.channel_ptr, Pmcmd_GetLldTxObject(&self_->wd.wd_cmd));
        }
        else
        {
            TR_ERROR((self_->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxOnWatchdogTimer(): Unable to reserve watchdog command ", 0U));
            Fifo_Stop(self_, FIFO_S_UNSYNCED_INIT, true);
        }
    }
    else                                            /* status not received in time - notify communication error */
    {
        TR_ERROR((self_->init.base_ptr->ucs_user_ptr, "[FIFO]", "Fifo_TxOnWatchdogTimer(): Missing response on status request", 0U));
        Fifo_Stop(self_, FIFO_S_UNSYNCED_INIT, true);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

