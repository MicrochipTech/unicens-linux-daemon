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
 * \brief Implementation of Application Message Service
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_AMSC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_ams.h"
#include "ucs_amsmessage.h"
#include "ucs_dl.h"
#include "ucs_misc.h"
#include "ucs_pmp.h"
#include "ucs_encoder.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Priority of the Application Message Service */
static const uint8_t     AMS_SRV_PRIO       = 253U; /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief Event which triggers the Rx service */
static const Srv_Event_t AMS_EV_RX_SERVICE  = 1U;
/*! \brief Event which triggers the Tx service */
static const Srv_Event_t AMS_EV_TX_SERVICE  = 2U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Ams_Cleanup(CAms *self);
static void Ams_Service(void *self);
static void Ams_OnEhEvent(void *self, void *error_code_ptr);

static void Ams_TxService(CAms *self);
static void Ams_TxOnStatus(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status);
static uint8_t Ams_TxGetNextFollowerId(CAms *self);

static void Ams_RxOnTelComplete(CAms *self, Msg_MostTel_t *tel_ptr);
static void Ams_RxReleaseTel(CAms *self, Msg_MostTel_t *tel_ptr);
static void Ams_RxProcessWaitingQ(CAms *self);
static void Ams_RxOnSegError(void *self, Msg_MostTel_t *tel_ptr, Segm_Error_t error);
static void Ams_RxOnFreedMsg(void *self, void *data_ptr);
static void Ams_RxFlush(CAms *self);

/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of class CAms
 *  \param self              The instance
 *  \param base_ptr          Reference to base services
 *  \param mcm_trcv_ptr      Reference to the MCM transceiver
 *  \param rcm_trcv_ptr      Reference to the RCM transceiver
 *  \param pool_ptr          Reference to the pool for application message handles
 *  \param rx_def_payload_sz Default memory size that is allocated when receiving segmented messages 
 *                           without size prefix
 */
void Ams_Ctor(CAms *self, CBase *base_ptr, CTransceiver *mcm_trcv_ptr, CTransceiver *rcm_trcv_ptr,
              CAmsMsgPool *pool_ptr, uint16_t rx_def_payload_sz)
{
    MISC_UNUSED(rcm_trcv_ptr);
    MISC_MEM_SET((void *)self, 0, sizeof(*self));                           /* reset members to "0" */
    self->trcv_mcm_ptr = mcm_trcv_ptr;
    self->trcv_rcm_ptr = rcm_trcv_ptr;
    self->base_ptr = base_ptr;
    self->pool_ptr = pool_ptr;

    self->tx.default_llrbc = AMS_LLRBC_DEFAULT;                             /* set initial retries */
    self->tx.next_follower_id = 1U;                                         /* set initial follower id */
                                                                            /* init pools */
    Obs_Ctor(&self->rx.message_freed_observer, self, &Ams_RxOnFreedMsg);
    Amsp_AssignRxFreedObs(self->pool_ptr, &self->rx.message_freed_observer);
    Telq_Ctor(&self->rx.waiting_queue, self->base_ptr->ucs_user_ptr);        /* init Rx waiting queue */

    Dl_Ctor(&self->tx.queue, self->base_ptr->ucs_user_ptr);

    Srv_Ctor(&self->service, AMS_SRV_PRIO, self, &Ams_Service);             /* register service */
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);

    Segm_Ctor(&self->segmentation, self->base_ptr,  self->pool_ptr, rx_def_payload_sz);
    Segm_AssignRxErrorHandler(&self->segmentation, &Ams_RxOnSegError, self);

    if (self->trcv_mcm_ptr != NULL)
    {
        Trcv_RxAssignReceiver(self->trcv_mcm_ptr, &Ams_RxOnMcmTelComplete, self);
    }
    if (self->trcv_rcm_ptr != NULL)
    {
        Trcv_RxAssignReceiver(self->trcv_rcm_ptr, &Ams_RxOnRcmTelComplete, self);
    }    

    Mobs_Ctor(&self->unsync_result_observer, self, EH_M_TERMINATION_EVENTS, &Ams_OnEhEvent);     /* register error observer */
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->unsync_result_observer);
}

/*! \brief Sets the default retry values used for Application Messages
 *  \param self         The instance
 *  \param llrbc        The default low level retry block count
 */
void Ams_TxSetDefaultRetries(CAms* self, uint8_t llrbc)
{
    self->tx.default_llrbc = llrbc;
}

/*! \brief  Assigns a function of another class to receive application messages 
 *  \param  self            The instance
 *  \param  cb_fptr         Callback function
 *  \param  inst_ptr        The instance of the receiver class
 */
void Ams_RxAssignReceiver(CAms *self, Amsg_RxCompleteCb_t cb_fptr, void *inst_ptr)
{
    self->rx.complete_fptr = cb_fptr;
    self->rx.complete_inst_ptr = inst_ptr;
}

/*! \brief   Assigns an observer which is invoked if a Tx application message is freed.
 *  \details The observer is only notified a previous allocation of a Tx object has failed.
 *           The data_ptr of the update callback function is not used (always \c NULL).
 *           See \ref Obs_UpdateCb_t.
 *  \param   self            The instance
 *  \param   observer_ptr    The observer
 */
void Ams_TxAssignMsgFreedObs(CAms *self, CObserver *observer_ptr)
{
    Amsp_AssignTxFreedObs(self->pool_ptr, observer_ptr);
}

/*! \brief   Assigns a callback function that selects FIFO routing for a Tx message.
 *  \details If no callback function is assigned, then all Tx messages are routed to RCM FIFO.
 *  \param   self       The instance
 *  \param   cb_fptr    The callback function
 */
void Ams_TxAssignTrcvSelector(CAms *self, Ams_TxIsRcmMsgCb_t cb_fptr)
{
    self->tx.is_rcm_fptr = cb_fptr;
}

/*! \brief  Performs a cleanup of the Tx message queue and notifies 
 *          the transmission error UCS_AMSTX_RES_NOT_AVAILABLE.
 *  \param  self    The instance
 */
static void Ams_Cleanup(CAms *self)
{
    Ucs_AmsTx_Msg_t *tx_ptr = NULL;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Starting AMS Cleanup", 0U));
                                                    /* cleanup Tx queue */
    for (tx_ptr = Amsg_TxDequeue(&self->tx.queue); tx_ptr != NULL; tx_ptr = Amsg_TxDequeue(&self->tx.queue))
    {
        /* just just notify completion, the object is automatically freed to the pool */
        Amsg_TxNotifyComplete(tx_ptr, UCS_AMSTX_RES_ERR_NOT_AVAILABLE, UCS_AMSTX_I_ERR_UNSYNCED);
    }

    Segm_Cleanup(&self->segmentation);              /* cleanup Rx */
    Ams_RxFlush(self);
    Amsp_Cleanup(self->pool_ptr);                   /* final cleanup of pool */
    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Finished AMS Cleanup", 0U));
}

/*! \brief  Callback function which is invoked by the event handler
 *          on any termination event.
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Ams_OnEhEvent(void *self, void *error_code_ptr)
{
    CAms *self_ = (CAms*)self;
    MISC_UNUSED(error_code_ptr);
    Ams_Cleanup(self_);
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The AMS service function 
 *  \param self The instance
 */
static void Ams_Service(void *self)
{
    CAms *self_ = (CAms*)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);

    if ((event_mask & AMS_EV_TX_SERVICE) == AMS_EV_TX_SERVICE)     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->service, AMS_EV_TX_SERVICE);
        Ams_TxService(self_);
    }

    if ((event_mask & AMS_EV_RX_SERVICE) == AMS_EV_RX_SERVICE)     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->service, AMS_EV_RX_SERVICE);
        Ams_RxProcessWaitingQ(self_);
    }
}

/*! \brief Allocates and transmits MCMs for the dedicated Application Messages 
 *  \param self The instance
 */
static void Ams_TxService(CAms *self)
{
    CDlNode *node1_ptr;
                                                                    /* run as long as messages are available in Tx queue */
    for (node1_ptr = Dl_PeekHead(&self->tx.queue); node1_ptr != NULL; node1_ptr = Dl_PeekHead(&self->tx.queue))
    {
        Msg_MostTel_t *tel_ptr = NULL;
        CTransceiver *trcv_ptr = self->trcv_mcm_ptr;
        Ucs_AmsTx_Msg_t *tx_ptr = (Ucs_AmsTx_Msg_t*)Dln_GetData(node1_ptr);

        if (self->tx.is_rcm_fptr != NULL)
        {
            if (self->tx.is_rcm_fptr(tx_ptr) != false)
            {
                trcv_ptr = self->trcv_rcm_ptr;
            }
        }
                                                                    /* allocate telegram object with 2 bytes for TelId 4 */
        tel_ptr = Trcv_TxAllocateMsg(trcv_ptr, 2U);                 /* remaining message payload is attached as external memory */

        if (tel_ptr != NULL)                                        /* transmit message if telegram object is available */
        {
            CDlNode *node2_ptr = Dl_PopHead(&self->tx.queue);       /* now retrieve application message from queue, previously checked by peek operation */

            if (node2_ptr != NULL)
            {
                bool done;
                TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (node1_ptr == node2_ptr));
                tx_ptr = (Ucs_AmsTx_Msg_t*)Dln_GetData(node2_ptr);
                done = Segm_TxBuildSegment(&self->segmentation, tx_ptr, tel_ptr);   /* run segmentation */
                Trcv_TxSendMsgExt(trcv_ptr, tel_ptr, &Ams_TxOnStatus, self);        /* transmit telegram */
                TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Ams_TxService(tel_ptr=0x%p)", 1U, tel_ptr));

                if (done == false)
                {
                    Dl_InsertHead(&self->tx.queue, node2_ptr);
                }
            }
            else
            {
                TR_FAILED_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]");             /* inconsistency between peek and pop operation */
            }
        }
        else
        {
            break;
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* AMS Tx handles                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Retrieves a message Tx handle
 *  \details The payload provided is limited the supported size of the memory management.
 *           The application may also attach own payload to a message object. Therefore, 
 *           Ams_TxGetMsg() shall be called with size "0".
 *  \param   self   The instance
 *  \param   size   Payload size in bytes or "0" to use application provided payload.
 *                  The payload provided by MNS is limited to a size of 45 bytes.
 *                  Valid values: 0..45.
 *  \return  A Tx message object or \c NULL if no message object is available.
 */
Ucs_AmsTx_Msg_t * Ams_TxGetMsg(CAms *self, uint16_t size)
{
    Ucs_AmsTx_Msg_t * msg_ptr = Amsp_AllocTxObj(self->pool_ptr, size);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_address = AMS_ADDR_RSVD_RANGE; /* set invalid address to prevent internal transmission*/
        msg_ptr->llrbc = self->tx.default_llrbc;
    }

    return msg_ptr;
}

/*! \brief  Frees an unused or completed Tx message to the pool
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the related message object
 */
void Ams_TxFreeUnusedMsg(CAms *self, Ucs_AmsTx_Msg_t *msg_ptr)
{
    MISC_UNUSED(self);
    Amsg_TxFreeUnused(msg_ptr);     /* the object is automatically freed to the pool */
}

/*------------------------------------------------------------------------------------------------*/
/* AMS Transmission                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Transmits a MOST Application Message 
 *  \details    After the transmission completed the function will call one callback function. Therefore
 *              the caller is able to assign one of two different callback function. The difference between
 *              the callback function is that tx_complete_sia_fptr does no provide a self pointer whether 
 *              tx_complete_fptr and tx_complete_inst_ptr allow to invoke a class method.
 *  \param  self                    The instance
 *  \param  msg_ptr                 Reference to the related message object
 *  \param  tx_complete_sia_fptr    Single instance API callback function which is invoked as soon as 
 *                                  the transmission was finished.
 *  \param  tx_complete_fptr        Multi instance callback function which is invoked as soon as 
 *                                  the transmission was finished.
 *  \param  tx_complete_inst_ptr    Instance pointer which is referred when tx_complete_fptr is invoked. 
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_PARAM if the transmission was refused due to an invalid parameter
 */
Ucs_Return_t Ams_TxSendMsg(CAms *self, Ucs_AmsTx_Msg_t *msg_ptr, Amsg_TxCompleteSiaCb_t tx_complete_sia_fptr, 
                           Amsg_TxCompleteCb_t tx_complete_fptr, void* tx_complete_inst_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Called Ams_TxSendMsg(0x%p)", 1U, msg_ptr));

    if (Ams_TxIsValidMessage(msg_ptr))                      /* prevent application messages to loc. INIC */
    {                                                       /* do not set both callback pointers */
        TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (((tx_complete_sia_fptr != NULL) && (tx_complete_fptr != NULL)) == false))
        Amsg_TxSetCompleteCallback(msg_ptr, tx_complete_sia_fptr, tx_complete_fptr, tx_complete_inst_ptr);
        Ams_TxSendMsgDirect(self, msg_ptr);
        ret_val = UCS_RET_SUCCESS;
    }

    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (ret_val == UCS_RET_SUCCESS));

    return ret_val;
}

/*! \brief          Transmits a MOST Application Message without attributes check
 *  \details        This method shall be only be used by AMD and AMS internally
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the related message object
 */
void Ams_TxSendMsgDirect(CAms *self, Ucs_AmsTx_Msg_t *msg_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Called Ams_TxSendMsg(0x%p)", 1U, msg_ptr));

    if (msg_ptr->data_size > SEGM_MAX_SIZE_TEL)                         /* set follower id to be used for all segments */
    {
        Amsg_TxSetFollowerId(msg_ptr, Ams_TxGetNextFollowerId(self));
    }

    Amsg_TxEnqueue(msg_ptr, &self->tx.queue);                           /* schedule transmission */
    Srv_SetEvent(&self->service, AMS_EV_TX_SERVICE);
}

/*! \brief  Callback function which is invoked as soon as MCM transmission
 *          was finished in PMS. 
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the telegram
 *  \param  status  Transmission status
 */
static void Ams_TxOnStatus(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status)
{ 
    CAms *self_ = (CAms*)self;
    Ucs_AmsTx_Msg_t* msg_ptr = (Ucs_AmsTx_Msg_t*)tel_ptr->info_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[AMS]", "Ams_TxOnStatus(tel_ptr=0x%p, %d)", 2U, tel_ptr, status));

    if (msg_ptr != NULL)                                                                                      /* MOST Telegram has AMS parent? */
    {
        Amsg_TxUpdateResult(msg_ptr, status);

        if ((tel_ptr->tel.tel_id == 0U) || (tel_ptr->tel.tel_id == 3U))                                       /* is finished? */
        {                                                                                                     /* just just notify completion, the object is */
            Amsg_TxNotifyComplete(msg_ptr, Amsg_TxGetResultCode(msg_ptr), Amsg_TxGetResultInfo(msg_ptr));     /* automatically freed to the pool */
        }
        else if (status != UCS_MSG_STAT_OK)                                                                   /* check transmission needs termination before transmission end */
        {
            TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[AMS]", (Amsg_TxGetFollowerId(msg_ptr) != 0U));

            if (((uint8_t)Amsg_TxGetNextSegmCnt(msg_ptr) == (uint8_t)(tel_ptr->tel.tel_cnt + 1U))             /* is last transmitted segment */
                || ((Amsg_TxGetNextSegmCnt(msg_ptr) == 0U) && (tel_ptr->tel.tel_id == 4U)))                   /* or TelId 4 and the first segment is pending */
            {
                Amsg_TxRemoveFromQueue(msg_ptr, &self_->tx.queue);
                Amsg_TxNotifyComplete(msg_ptr, Amsg_TxGetResultCode(msg_ptr), Amsg_TxGetResultInfo(msg_ptr)); /* just just notify completion, the object is */
            }                                                                                                 /* automatically freed to the pool */
        }
    }
    Trcv_TxReleaseMsg(tel_ptr);                                                                               /* release message object to pool */

    if ((Dl_GetSize(&self_->tx.queue) > 0U) && (status != UCS_MSG_STAT_ERROR_SYNC))                           /* Application Messages are available for Tx */
    {
        Srv_SetEvent(&self_->service, AMS_EV_TX_SERVICE);
    }
}

/*! \brief   Checks if the destination address of the Tx message is valid and payload is consistent
 *  \param   msg_ptr    Reference to the Tx message object
 *  \return  Returns \c true if the destination is correct, otherwise \c false.
 */
bool Ams_TxIsValidMessage(Ucs_AmsTx_Msg_t *msg_ptr)
{
    bool ret = false;

    if (msg_ptr != NULL)
    {
        if (msg_ptr->destination_address > AMS_ADDR_RSVD_RANGE)    /* is not reserved address? */
        {
            if (((msg_ptr->destination_address & 0xFF00U) !=  0x0300U)/* is single-cast? */
               || (msg_ptr->data_size <= SEGM_MAX_SIZE_TEL))          /* or not segmented */ 
            {
                if (!((msg_ptr->data_size > 0U) && (msg_ptr->data_ptr == NULL)))
                {
                    ret = true;
                }
            }
        }
    }

    return ret;
}

/*! \brief   Retrieves the next follower id to use for segmented transfer 
 *  \param   self   The instance
 *  \return  The follower id
 */
static uint8_t Ams_TxGetNextFollowerId(CAms *self)
{
    uint8_t ret;
    ret = self->tx.next_follower_id;
    self->tx.next_follower_id++;

    if (self->tx.next_follower_id == 0U)    /* skip zero since it means */
    {                                       /* "no follower" */
        self->tx.next_follower_id = 1U;
    }

    return ret;
}

/*! \brief   Retrieves the number of messages that are queued for transmission 
 *  \param   self   The instance
 *  \return  The number of messages in the Tx queue
 */
uint16_t Ams_TxGetMsgCnt(CAms *self)
{
    return Dl_GetSize(&self->tx.queue);
}

/*------------------------------------------------------------------------------------------------*/
/* AMS Reception                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Rx callback function that can be assigned to the MCM transceiver
 *  \details The associated transceiver reference will be stored in the telegrams \c info_ptr.
 *           Later on the telegram must be released via Ams_RxReleaseTel().
 *  \param   self    The instance
 *  \param   tel_ptr Reference to the Rx telegram object
 */
void Ams_RxOnMcmTelComplete(void *self, Msg_MostTel_t *tel_ptr)
{
    CAms *self_ = (CAms*)self;
    TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[AMS]", (tel_ptr->info_ptr == NULL));
    tel_ptr->info_ptr = self_->trcv_mcm_ptr;
    Ams_RxOnTelComplete(self_, tel_ptr);
}

/*! \brief   Rx callback function that can be assigned to the RCM transceiver
 *  \details The associated transceiver reference will be stored in the telegrams \c info_ptr.
 *           Later on the telegram must be released via Ams_RxReleaseTel().
 *  \param   self    The instance
 *  \param   tel_ptr Reference to the Rx telegram object
 */
void Ams_RxOnRcmTelComplete(void *self, Msg_MostTel_t *tel_ptr)
{
    CAms *self_ = (CAms*)self;
    TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[AMS]", (tel_ptr->info_ptr == NULL));
    tel_ptr->info_ptr = self_->trcv_rcm_ptr;
    Ams_RxOnTelComplete(self_, tel_ptr);
}

/*! \brief   Releases an Rx telegram to the associated transceiver
 *  \details The associated transceiver reference is stored in the telegrams \c info_ptr
 *  \param   self    The instance
 *  \param   tel_ptr Reference to the Rx telegram object
 */
static void Ams_RxReleaseTel(CAms *self, Msg_MostTel_t *tel_ptr)
{
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", ((tel_ptr != NULL) && (tel_ptr->info_ptr != NULL)));
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", ((tel_ptr->info_ptr == self->trcv_mcm_ptr)||(tel_ptr->info_ptr == self->trcv_rcm_ptr)));

    if (tel_ptr->info_ptr != NULL)
    {
        Trcv_RxReleaseMsg((CTransceiver*)tel_ptr->info_ptr, tel_ptr);
    }

    MISC_UNUSED(self);
}

/*! \brief  Internal callback function which is invoked as soon as the transceiver
 *          reference is stored to the telegrams info_ptr.
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the Rx telegram object
 */
static void Ams_RxOnTelComplete(CAms *self, Msg_MostTel_t *tel_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;

    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Ams_RxOnComplete(0x%p)", 1U, tel_ptr));

    if (self->rx.complete_fptr == NULL)
    {
        /* no processing required, tel_ptr shall be freed */
        msg_ptr = NULL;
    }
    else if (Telq_GetSize(&self->rx.waiting_queue) > 0U)    /* asynchronous Rx is running */
    {                                                       /* queue Rx telegram for later processing */
        Telq_Enqueue(&self->rx.waiting_queue, tel_ptr);
        tel_ptr = NULL;                                     /* do not free Rx telegram */
        msg_ptr = NULL;
    }
    else
    {
        Segm_Result_t result;                               /* synchronous processing is possible now */
        msg_ptr = Segm_RxExecuteSegmentation(&self->segmentation, tel_ptr, &result);

        if (result == SEGM_RES_RETRY)
        {
            TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (msg_ptr == NULL));
            Telq_Enqueue(&self->rx.waiting_queue, tel_ptr);
            tel_ptr = NULL;                                 /* do not free Rx telegram */
        }
    }

    if (msg_ptr != NULL)
    {
        self->rx.complete_fptr(self->rx.complete_inst_ptr, (Ucs_AmsRx_Msg_t*)(void*)msg_ptr);
    }

    if (tel_ptr != NULL)
    {
        Ams_RxReleaseTel(self, tel_ptr);                    /* free Rx telegram */
    }
}

/*! \brief   Processes all telegrams in waiting queue
 *  \details Stops if allocation of Rx messages fails
 *  \param   self    The instance
 */
static void Ams_RxProcessWaitingQ(CAms *self)
{
    Msg_MostTel_t *tel_ptr;
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;

    for (tel_ptr = Telq_Peek(&self->rx.waiting_queue); tel_ptr != NULL; tel_ptr = Telq_Peek(&self->rx.waiting_queue))
    {
        Segm_Result_t result;
        msg_ptr = Segm_RxExecuteSegmentation(&self->segmentation, tel_ptr, &result);

        if (result == SEGM_RES_OK)                                              /* segmentation process succeeded */
        {
            (void)Telq_Dequeue(&self->rx.waiting_queue);                        /* remove telegram from waitingQ */
            Ams_RxReleaseTel(self, tel_ptr);                                    /* free telegram */
            tel_ptr = NULL;                                                     /* parasoft-suppress  MISRA2004-13_6 "variable is not used as a counter" */

            if (msg_ptr != NULL)
            {
                self->rx.complete_fptr(self->rx.complete_inst_ptr, (Ucs_AmsRx_Msg_t*)(void*)msg_ptr);
            }
        }
        else
        {
            TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (msg_ptr == NULL));
            break;                                                              /* wait until further Rx messages can be allocated - abort loop */
        }
    }
}

/*! \brief  Callback function which is invoked by segmentation process to notify a segmentation error 
 *  \param  self    The instance
 *  \param  tel_ptr The related Rx telegram which caused the segmentation error
 *  \param  error   The segmentation error number
 */
static void Ams_RxOnSegError(void *self, Msg_MostTel_t *tel_ptr, Segm_Error_t error)
{
    const uint8_t ERR_SZ = 2U;
    CAms *self_ = (CAms*)self; 
    Msg_MostTel_t* error_tel_ptr = NULL;

    TR_ERROR((self_->base_ptr->ucs_user_ptr, "[AMS]", "Ams_RxOnComplete(0x%p, %d)", 2U, tel_ptr, error));

    if (tel_ptr->source_addr != MSG_ADDR_INIC)
    {                                                                       /* only generate segmentation errors */
        error_tel_ptr = Trcv_TxAllocateMsg(self_->trcv_mcm_ptr, ERR_SZ);    /* for messages which are NOT locally routed by the INIC */
    }

    if (error_tel_ptr != NULL)
    {
        error_tel_ptr->destination_addr     = tel_ptr->source_addr;
        error_tel_ptr->id                   = tel_ptr->id;
        error_tel_ptr->id.op_type           = UCS_OP_ERROR;
        error_tel_ptr->tel.tel_data_ptr[0]  = 0x0CU;
        error_tel_ptr->tel.tel_data_ptr[1]  = (uint8_t)error;
        error_tel_ptr->opts.llrbc           = 0U;

        Trcv_TxSendMsg(self_->trcv_mcm_ptr, error_tel_ptr);      /* just fire the message */
    }
}

/*! \brief Callback function that is invoked if application Rx messages are available again
 *  \param self     The instance
 *  \param data_ptr Unused parameter of observer callback
 */
static void Ams_RxOnFreedMsg(void *self, void *data_ptr)
{
    CAms *self_ = (CAms*) self;
    Srv_SetEvent(&self_->service, AMS_EV_RX_SERVICE);
    MISC_UNUSED(data_ptr);
}

/*! \brief   Removes and frees a message from the Rx queue
 *  \details The application must not access the passed 
 *           message any more.
 *  \param   self       The instance
 *  \param   msg_ptr    Reference to the message, or \c NULL for the front-most
 *                      message in the Rx queue. 
 */
void Ams_RxFreeMsg(CAms *self, Ucs_AmsRx_Msg_t *msg_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[AMS]", "Ams_RxFreeMsg(msg_ptr=0x%p)", 1U, msg_ptr));
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[AMS]", (msg_ptr != NULL));

    if (msg_ptr != NULL)
    {
        Amsp_FreeRxPayload(self->pool_ptr, msg_ptr);            /* free external payload */
        Amsp_FreeRxObj(self->pool_ptr, msg_ptr);                /* return message to Rx pool */
    }
}

/*! \brief   Removes all messages located in Rx queues
 *  \param   self   The instance
 */
static void Ams_RxFlush(CAms *self)
{
    Msg_MostTel_t *tel_ptr;

    for (tel_ptr = Telq_Dequeue(&self->rx.waiting_queue); tel_ptr != NULL; tel_ptr = Telq_Dequeue(&self->rx.waiting_queue))
    {
        Ams_RxReleaseTel(self, tel_ptr);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

