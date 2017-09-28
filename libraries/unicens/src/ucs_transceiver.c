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
 * \brief Implementation of class CTransceiver
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_TRCV
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_transceiver.h"
#include "ucs_misc.h"
#include "ucs_pmp.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Trcv_OnTxStatusInternal(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of class CTransceiver
 *  \param  self         The instance
 *  \param  fifo_ptr     Reference to the dedicated port message FIFO
 *  \param  def_src_addr Source address that is preset in Tx message object
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 *  \param  trace_id     ID specifies FIFO in traces if multiple transceivers are running
 */
void Trcv_Ctor(CTransceiver *self, CPmFifo *fifo_ptr, uint16_t def_src_addr, void *ucs_user_ptr, uint8_t trace_id)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->fifo_ptr = fifo_ptr;
    self->tx_def_src = def_src_addr;
    self->ucs_user_ptr = ucs_user_ptr;
    self->own_id = trace_id;
    Pool_Ctor(&self->tx_msg_pool, self->tx_msgs, TRCV_SIZE_TX_POOL, ucs_user_ptr);
    TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (fifo_ptr != NULL));
}

/*! \brief   Assigns a function of another class to receive messages
 *  \details The assigned function is responsible to call Trcv_RxReleaseMsg() it has finished to process it
 *  \param   self            The instance
 *  \param   callback_fptr   Callback function
 *  \param   inst_ptr        The instance of the receiver class
 */
void Trcv_RxAssignReceiver(CTransceiver *self, Trcv_RxCompleteCb_t callback_fptr, void *inst_ptr)
{
    self->rx_complete_fptr = callback_fptr;
    self->rx_complete_inst = inst_ptr;
}

/*! \brief   Assigns a function of another class to filter Rx messages
 *  \details The assigned function is responsible to discard or pass Rx messages
 *  \param   self            The instance
 *  \param   callback_fptr   Callback function
 *  \param   inst_ptr        The instance of the filter class
 */
void Trcv_RxAssignFilter(CTransceiver *self, Trcv_RxFilterCb_t callback_fptr, void *inst_ptr)
{
    self->rx_filter_fptr = callback_fptr;
    self->rx_filter_inst = inst_ptr;
}

/*! \brief  Releases an Rx message which was received by the assigned receiver
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the received message
 */
void Trcv_RxReleaseMsg(CTransceiver *self, Msg_MostTel_t *tel_ptr)
{
    CMessage *msg_ptr = (CMessage*)(void*)tel_ptr;
    bool check_ok = !Dln_IsNodePartOfAList(Msg_GetNode(msg_ptr));   /* message object shall not be part of a list */
                                                                    /* because it was provided in an earlier step */
    TR_ASSERT(self->ucs_user_ptr, "[TRCV]", check_ok);
    if (check_ok)
    {
        Fifo_RxReleaseMsg(self->fifo_ptr, msg_ptr);
    }
}

/*! \brief  Retrieves a message object from the pool
 *  \param  self    The instance
 *  \param  size    Size of the message in bytes. Valid range: 0..45.
 *  \return Reference to the Msg_MostTel_t structure if a message is available.
 *          Otherwise \c NULL.
 */
extern Msg_MostTel_t* Trcv_TxAllocateMsg(CTransceiver *self, uint8_t size)
{
    const uint8_t TRCV_CTRL_MAX_SIZE = 45U;         /* replace by PMS constant in future */
    CMessage        *handle = NULL;
    Msg_MostTel_t   *tel_ptr = NULL;

    if (size <= TRCV_CTRL_MAX_SIZE)
    {
        handle = Pool_GetMsg(&self->tx_msg_pool);

        if (handle != NULL)
        {
            Msg_Cleanup(handle);                   /* reset headers and fields */
            Msg_ReserveHeader(handle, PMP_PM_MAX_SIZE_HEADER + ENC_MAX_SIZE_CONTENT);
            tel_ptr = Msg_GetMostTel(handle);      /* return public struct of the message object */
            tel_ptr->tel.tel_id = 0U;
            tel_ptr->tel.tel_len = size;
            tel_ptr->tel.tel_cnt = 0U;
            tel_ptr->source_addr = self->tx_def_src;
        }
    }

    return tel_ptr;
}

/*! \brief  Returns a message object to the transceiver pool a message was allocated from
 *  \param  tel_ptr Reference to the message object which needs to be returned. 
 */
void Trcv_TxReleaseMsg(Msg_MostTel_t *tel_ptr)
{
    CMessage* msg_ptr = (CMessage*)(void*)tel_ptr;                  /* avoid MISRA-C warning by converting to "void*" */
    bool check_ok = !Dln_IsNodePartOfAList(Msg_GetNode(msg_ptr));   /* message object shall not be part of a list */
    TR_ASSERT(0U, "[TRCV]", check_ok);                              /* because it was provided in an earlier step */

    if (check_ok)
    {
        Pool_ReturnMsg(msg_ptr);
    }
}

/*! \brief  Prepares a message object for re-transmission
 *  \param  tel_ptr Reference to the Tx message object which needs
 *                  to be reused. 
 */
void Trcv_TxReuseMsg(Msg_MostTel_t *tel_ptr)
{
    CMessage* msg_ptr = (CMessage*)(void*)tel_ptr; 
    TR_ASSERT(0U, "[TRCV]", (!Dln_IsNodePartOfAList(Msg_GetNode(msg_ptr)))); /* message object shall not be part of a list */
                                                                             /* because it was provided in an earlier step */
    Msg_Cleanup(msg_ptr);        /* reset headers and fields */
    Msg_ReserveHeader(msg_ptr, PMP_PM_MAX_SIZE_HEADER + ENC_MAX_SIZE_CONTENT);
}

/*! \brief   Transmits a given message object to the INIC
 *  \details After completed transmission the message object is released automatically
 *  \param   self    The instance
 *  \param   tel_ptr Reference to the message object
 */
void Trcv_TxSendMsg(CTransceiver *self, Msg_MostTel_t *tel_ptr)
{
    CMessage *msg_ptr; 

    TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (tel_ptr != NULL));
    msg_ptr = (CMessage*)(void*)tel_ptr;

    TR_INFO((self->ucs_user_ptr, "[TRCV]", "Trcv_TxSendMsg(): FIFO: %u, MSG(tgt:0x%04X, id:%02X.%01X.%04X.%01X)", 6U, self->own_id, tel_ptr->destination_addr, tel_ptr->id.fblock_id, tel_ptr->id.instance_id, tel_ptr->id.function_id, tel_ptr->id.op_type));
    Msg_SetTxStatusHandler(msg_ptr, &Trcv_OnTxStatusInternal, self);        /* just release the message */
    Fifo_Tx(self->fifo_ptr, msg_ptr, false);
}

/*! \brief  Transmits a given message object to the INIC with a dedicated result callback 
 *  \param  self          The instance
 *  \param  tel_ptr       Reference to the message object
 *  \param  callback_fptr Callback function which is invoked after message transmission has finished.
 *                        Must be \c NULL to avoid that a callback function is invoked. In this case
 *                        the message object is freed internally. Hence, the message object must 
 *                        not provide external payload.
 *  \param  inst_ptr      Reference to the instance which is invoked with callback_fptr. Has to be \c 
 *                        NULL if callback_fptr is \c NULL.
 *  \note   The provided callback function is responsible to free the message object by calling 
 *          Trcv_TxReleaseMsg() or to reuse the message object by calling Trcv_TxReuseMsg() before
 *          passing it to one of the transmit functions again.
 */
void Trcv_TxSendMsgExt(CTransceiver *self, Msg_MostTel_t *tel_ptr, Msg_TxStatusCb_t callback_fptr, void *inst_ptr)
{
    CMessage *msg_ptr; 

    TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (tel_ptr != NULL));
    msg_ptr = (CMessage*)(void*)tel_ptr;

    if (callback_fptr == NULL)
    {
        TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (inst_ptr == NULL));
        callback_fptr = &Trcv_OnTxStatusInternal;
        inst_ptr = self;
    }

    TR_INFO((self->ucs_user_ptr, "[TRCV]", "Trcv_TxSendMsgExt(): FIFO: %u, MSG(tgt:0x%04X, id:%02X.%01X.%04X.%01X)", 6U, self->own_id, tel_ptr->destination_addr, tel_ptr->id.fblock_id, tel_ptr->id.instance_id, tel_ptr->id.function_id, tel_ptr->id.op_type));
    Msg_SetTxStatusHandler(msg_ptr, callback_fptr, inst_ptr);
    Fifo_Tx(self->fifo_ptr, msg_ptr, false);
}

/*! \brief  Transmits a given message object to the INIC bypassing all other messages in the FIFO
 *  \param  self          The instance
 *  \param  tel_ptr       Reference to the message object
 *  \param  callback_fptr Callback function which is invoked after message transmission has finished.
 *                        Must be \c NULL to avoid that a callback function is invoked. In this case
 *                        the message object is freed internally. Hence, the message object must 
 *                        not provide external payload.
 *  \param  inst_ptr      Reference to the instance which is invoked
 *  \note   The provided callback function is responsible to free the message object by calling 
 *          Trcv_TxReleaseMsg() or to reuse the message object by calling Trcv_TxReuseMsg() before
 *          passing it to one of the transmit functions again.
 */
void Trcv_TxSendMsgBypass(CTransceiver *self, Msg_MostTel_t *tel_ptr, Msg_TxStatusCb_t callback_fptr, void *inst_ptr)
{
    CMessage *msg_ptr;

    TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (tel_ptr != NULL));
    msg_ptr = (CMessage*)(void*)tel_ptr;

    if (callback_fptr == NULL)
    {
        TR_ASSERT(self->ucs_user_ptr, "[TRCV]", (inst_ptr == NULL));
        callback_fptr = &Trcv_OnTxStatusInternal;
        inst_ptr = self;
    }

    Msg_SetTxStatusHandler(msg_ptr, callback_fptr, inst_ptr);
    Fifo_Tx(self->fifo_ptr, msg_ptr, true);
}

/*! \brief  Callback function which is invoked instead of an external callback 
 *          as soon as channel transmission was finished in PMS. 
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the message object
 *  \param  status  Transmission status
 */
static void Trcv_OnTxStatusInternal(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status)
{
    Trcv_TxReleaseMsg(tel_ptr);
    MISC_UNUSED(self);
    MISC_UNUSED(status);
}

/*! \brief  Internal callback function which is intended to be 
 *          invoked by the port message channel on completed reception.
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the message object
 */
void Trcv_RxOnMsgComplete(void *self, CMessage *tel_ptr)
{
    CTransceiver *self_ = (CTransceiver*)self;
    bool discard = false;

    TR_INFO((self_->ucs_user_ptr, "[TRCV]", "Trcv_RxOnMsgComplete(): FIFO: %u, MSG(src:0x%04X, id:%02X.%01X.%04X.%01X)", 6U, self_->own_id, tel_ptr->pb_msg.source_addr, tel_ptr->pb_msg.id.fblock_id, tel_ptr->pb_msg.id.instance_id, tel_ptr->pb_msg.id.function_id, tel_ptr->pb_msg.id.op_type));
    if (self_->rx_filter_fptr != NULL)
    {
        discard = self_->rx_filter_fptr(self_->rx_filter_inst, Msg_GetMostTel(tel_ptr));
    }

    if ((self_->rx_complete_fptr != NULL) && (discard == false))
    {
        /* the assigned Rx function is responsible to release the message */
        self_->rx_complete_fptr(self_->rx_complete_inst, Msg_GetMostTel(tel_ptr));
    }
    else
    {
        Fifo_RxReleaseMsg(self_->fifo_ptr, tel_ptr);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

