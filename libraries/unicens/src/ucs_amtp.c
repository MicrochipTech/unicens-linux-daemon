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
 * \brief Implementation of Application Message Tx Pool
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_AMTP
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_amtp.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Amtp_OnMsgFreed(void *self, Ucs_AmsTx_Msg_t* msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Initialization                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of class CAmtp
 *  \param self         The instance
 *  \param msg_ptr      Reference to an array of Amsg_IntMsgTx_t objects
 *  \param data_ptr     Reference to payload data which is required for the payload of all messages.
 *                      The data size must be the product of message_cnt and payload_cnt.
 *  \param msg_cnt      The number of message objects in the array
 *  \param payload_sz   The payload size for each message. The size must be a multiple of "4".
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Amtp_Ctor(CAmtp *self, Amsg_IntMsgTx_t msg_ptr[], uint8_t data_ptr[], uint8_t msg_cnt, uint16_t payload_sz, void *ucs_user_ptr)
{
    uint8_t i = 0U;
    uint32_t mem_idx = 0U;
    Ucs_AmsTx_Msg_t *tx_ptr;

    self->ucs_user_ptr = ucs_user_ptr;
    TR_ASSERT(self->ucs_user_ptr, "[AMTP]", ((payload_sz % 4U) == 0U));          /* payload_sz shall be rounded to full quadlet */
    TR_ASSERT(self->ucs_user_ptr, "[AMTP]", ((payload_sz * msg_cnt) <= 65535U)); /* total data shall be referenced by uint32_t index */

    Dl_Ctor(&self->msg_queue, self->ucs_user_ptr);

    for (i = 0U; i < msg_cnt; i++)
    {
        tx_ptr = (Ucs_AmsTx_Msg_t*)(void*)&(msg_ptr[i]);
        Amsg_TxCtor(tx_ptr, NULL, &Amtp_OnMsgFreed, self);

        if (payload_sz > 0U)
        {
            Amsg_TxSetInternalPayload(tx_ptr, &data_ptr[mem_idx], payload_sz, NULL);
            mem_idx += payload_sz;
        }

        Amsg_TxEnqueue(tx_ptr, &self->msg_queue);
    }

}

/*! \brief  Retrieves a Tx application message object
 *  \param  self     The instance
 *  \return Retrieves the reference to a Tx application message object if the allocation
 *          succeeded, otherwise \c NULL.
 */
Ucs_AmsTx_Msg_t* Amtp_AllocMsg(CAmtp *self)
{
    return Amsg_TxDequeue(&self->msg_queue);
}

/*! \brief  Callback function which is invoked if the message object is freed
 *          by the AMS
 *  \param  self     The instance
 *  \param  msg_ptr  Reference to the freed application Tx message object 
 */
static void Amtp_OnMsgFreed(void *self, Ucs_AmsTx_Msg_t* msg_ptr)
{
    CAmtp *self_ = (CAmtp*)self;

    Amsg_TxReuse(msg_ptr);
    Amsg_TxEnqueue(msg_ptr, &self_->msg_queue);
}


/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

