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
 * \brief Implementation of class message
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_MESSAGE
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_message.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of common MOST message class
 *  \param      self      The instance
 */
void Msg_Ctor(CMessage *self)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    Dln_Ctor(&self->node, self);

    self->rsvd_memory.allocator_ptr = NULL;
    self->rsvd_memory.mem_info_ptr  = NULL;
    self->rsvd_memory.public_buffer.next_buffer_ptr = NULL;
    self->rsvd_memory.public_buffer.data_ptr = &self->rsvd_buffer[0];
    self->rsvd_memory.public_buffer.data_size  = MSG_SIZE_RSVD_BUFFER;
    self->rsvd_memory.public_buffer.total_size = MSG_SIZE_RSVD_BUFFER;

    self->start_ptr                = &self->rsvd_buffer[0];
    self->pb_msg.tel.tel_data_ptr  = &self->rsvd_buffer[0];
/*  self->pb_msg.tel.tel_id        = 0U;
    self->pb_msg.tel.tel_cnt       = 0U;
    self->pb_msg.tel.tel_len       = 0U; */

    self->pb_msg.opts.llrbc     = MSG_LLRBC_DEFAULT;

/*  self->header_rsvd_sz           = 0U;
    self->header_curr_idx          = 0U;
    self->header_curr_sz           = 0U;
    self->ref_ptr                  = NULL; */
}

/*! \brief      Prepares the message for re-usage 
 *  \details    In future this function has to take care that external memory
 *              has to be reinitialize properly.
 *  \param      self    The instance
 */
void Msg_Cleanup(CMessage *self)
{
    void *handle = self->lld_handle_ptr;       /* restore associated LLD message object */
    void *pool_ptr = self->pool_ptr;           /* restore associated pool reference */

    Msg_Ctor(self);                            /* simply call constructor now */

    self->lld_handle_ptr = handle;
    self->pool_ptr = pool_ptr;
}

/*! \brief      Adds external message payload to the message
 *  \details    The internally reserved message payload is no longer in in use.
 *  \param      self            The instance
 *  \param      payload_ptr     Pointer to externally allocated payload
 *  \param      payload_sz      Size of externally allocated payload
 *  \param      mem_info_ptr    Reference to additional memory information
 */
void Msg_SetExtPayload(CMessage *self, uint8_t *payload_ptr, uint8_t payload_sz, void* mem_info_ptr)
{
    self->pb_msg.tel.tel_data_ptr = payload_ptr;
    self->pb_msg.tel.tel_len = payload_sz;

    self->ext_memory.allocator_ptr = NULL;
    self->ext_memory.mem_info_ptr  = mem_info_ptr;
    self->ext_memory.public_buffer.data_ptr = payload_ptr;
    self->ext_memory.public_buffer.data_size  = payload_sz;
    self->ext_memory.public_buffer.total_size = payload_sz;
    self->ext_memory.public_buffer.next_buffer_ptr = NULL;
}

/*! \brief      Initially defines a header space in front of the data body
 *  \details    Ensure that \c start_ptr is assigned correctly before calling 
 *              this functions.
 *  \param      self        The instance
 *  \param      header_sz   Size of the header
 */
void Msg_ReserveHeader(CMessage *self, uint8_t header_sz)
{
 /* self->start_ptr stays */
    self->header_rsvd_sz  = header_sz;
    self->header_curr_idx = header_sz;
    self->header_curr_sz  = 0U;

    self->pb_msg.tel.tel_data_ptr = &self->start_ptr[header_sz];
}

/*! \brief      Adds a defined header space in front of the current header
 *  \param      self        The instance
 *  \param      header_sz   Size of the header
 */
void Msg_PullHeader(CMessage *self, uint8_t header_sz)
{
/*  UCS_ASSERT(header_sz <= self->curr_header_sz); */

/*  self->pb_msg.tel.tel_data_ptr  = &self->rsvd_buffer[MSG_SIZE_RSVD_HEADER];*/
    self->header_curr_idx -= header_sz;
    self->header_curr_sz  += header_sz;
}

/*! \brief      Undoes a message header of a defined size
 *  \param      self        The instance
 *  \param      header_sz   Size of the header
 */
void Msg_PushHeader(CMessage *self, uint8_t header_sz)
{
    self->header_curr_idx += header_sz;
    self->header_curr_sz  -= header_sz;
}

/*------------------------------------------------------------------------------------------------*/
/* Class Properties (get/set)                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Retrieves the reference to the containing MOST Telegrams structure 
 *  \param  self    The instance
 *  \return Pointer to the internal MOST Telegram structure
 */
Msg_MostTel_t* Msg_GetMostTel(CMessage *self)
{
    return &self->pb_msg;
}

/*! \brief  Retrieves the start of the current message header
 *  \param  self    The instance
 *  \return Pointer to the current header start 
 */
uint8_t* Msg_GetHeader(CMessage *self)
{
    return &(self->rsvd_buffer[self->header_curr_idx]);
}

/*! \brief  Retrieves the size of the current message header
 *  \param  self    The instance
 *  \return Size of the current header in bytes 
 */
uint8_t Msg_GetHeaderSize(CMessage * self)
{
    return (self->header_curr_sz);
}

/*! \brief  Retrieves the message buffer as memory structure
 *  \param  self    The instance
 *  \return Reference to the message memory structure
 */
Ucs_Mem_Buffer_t* Msg_GetMemTx(CMessage *self)
{
    self->rsvd_memory.public_buffer.data_ptr = &(self->rsvd_buffer[self->header_curr_idx]);

    if (self->ext_memory.public_buffer.data_size == 0U)
    {
        self->rsvd_memory.public_buffer.next_buffer_ptr = NULL;
        self->rsvd_memory.public_buffer.data_size = (uint16_t)self->header_curr_sz + (uint16_t)self->pb_msg.tel.tel_len;
        self->rsvd_memory.public_buffer.total_size = (uint16_t)self->header_curr_sz + (uint16_t)self->pb_msg.tel.tel_len;
    }
    else
    {
        self->rsvd_memory.public_buffer.next_buffer_ptr = &self->ext_memory.public_buffer;
        self->rsvd_memory.public_buffer.data_size = (uint16_t)self->header_curr_sz;             /* only header is enclosed */
        self->rsvd_memory.public_buffer.total_size = self->rsvd_memory.public_buffer.data_size 
                                                    + self->ext_memory.public_buffer.data_size;
    }

    return &self->rsvd_memory.public_buffer;
}

/*! \brief  Assigns a message status handler which is called as soon as the message is processed
 *  \param  self            The instance
 *  \param  callback_fptr   Reference to the status callback function
 *  \param  inst_ptr        The instance which implements the status callback
 */
void Msg_SetTxStatusHandler(CMessage *self, Msg_TxStatusCb_t callback_fptr, void *inst_ptr)
{
    self->tx_status_inst = inst_ptr;
    self->tx_status_fptr = callback_fptr;
}

/*! \brief  Marks the message as occupied by the LLD
 *  \param  self       The instance
 *  \param  active     Set to \c true if the message is occupied by the LLD, otherwise \c false.
 */
void Msg_SetTxActive(CMessage *self, bool active)
{
    self->tx_active = active;
}

/*! \brief  Checks if the message as occupied by the LLD
 *  \param  self       The instance
 *  \return Returns \c true if the message is occupied by the LLD, otherwise \c false.
 */
bool Msg_IsTxActive(CMessage *self)
{
    return self->tx_active;
}

/*! \brief  Marks the message as bypass message
 *  \param  self       The instance
 *  \param  bypass     Set to \c true if the message is supposed to be a bypass message, otherwise \c false.
 */
void Msg_SetTxBypass(CMessage *self, bool bypass)
{
    self->tx_bypass = bypass;
}

/*! \brief  Checks if the message is marked as bypass message
 *  \param  self       The instance
 *  \return Returns \c true if the message is marked as bypass message, otherwise \c false.
 */
bool Msg_IsTxBypass(CMessage *self)
{
    return self->tx_bypass;
}

/*! \brief  Fires a status notification for the message object
 *  \param  self    The instance
 *  \param  status  The transmission status
 */
void Msg_NotifyTxStatus(CMessage *self, Ucs_MsgTxStatus_t status)
{
    if (self->tx_status_fptr != NULL)
    {
        self->tx_status_fptr(self->tx_status_inst, &self->pb_msg, status); 
    }
}

/*! \brief  Assigns a low-level driver message
 *  \param  self    The instance
 *  \param  handle  The reference to a low-level driver message object (Tx or Rx)
 */
void Msg_SetLldHandle(CMessage *self, void *handle)
{
    self->lld_handle_ptr = handle;
}

/*! \brief  Retrieves the reference to a low-level driver message
 *  \param  self    The instance
 *  \return The reference to a low-level driver message object or \c NULL
 *          if no message is assigned.
 */
void *Msg_GetLldHandle(CMessage *self)
{
    return self->lld_handle_ptr;
}

/*! \brief  Assigns a reference for the owning pool
 *  \param  self      The instance
 *  \param  pool_ptr  The reference to the owning pool
 */
void Msg_SetPoolReference(CMessage *self, void *pool_ptr)
{
    self->pool_ptr = pool_ptr;
}

/*! \brief  Retrieves a reference for the owning pool
 *  \param  self      The instance
 *  \return The reference to the owning pool or \c NULL
 *          if no pool is assigned.
 */
void *Msg_GetPoolReference(CMessage *self)
{
    return self->pool_ptr;
}

/*! \brief  Retrieves the reference to the internal node member
 *  \param  self    The instance
 *  \return The reference the internal list node 
 */
CDlNode *Msg_GetNode(CMessage *self)
{
    return &self->node;
}

/*! \brief  Performs checks on length payload length 
 *  \param  self    The instance
 *  \return Returns \c true if the verification succeeded. Otherwise \c false.
 */
bool Msg_VerifyContent(CMessage *self)
{
    bool success = (self->pb_msg.tel.tel_len <= MSG_MAX_SIZE_PAYLOAD) ? true : false;

    return success;
}

/*! \brief  Merges the alternate message id into a the most message id
 *  \param  self     The instance
 *  \return The alternate message id
 */
uint16_t Msg_GetAltMsgId(CMessage *self)
{
    uint16_t msg_id;
    msg_id = (uint16_t)(self->pb_msg.id.function_id >> 4);
    msg_id = (uint16_t)((uint16_t)self->pb_msg.id.instance_id << 8) | msg_id;
    return msg_id;
}

/*! \brief  Extracts the alternate message id from a the most message id
 *  \param  self     The instance
 *  \param  alt_id   The alternate message id
 */
void Msg_SetAltMsgId(CMessage *self, uint16_t alt_id)
{
    self->pb_msg.id.fblock_id = MSG_DEF_FBLOCK_ID;
    self->pb_msg.id.instance_id = MISC_HB(alt_id);
    self->pb_msg.id.function_id = (uint16_t)((((alt_id) & (uint16_t)0xFF)) << 4) | (uint16_t)MSG_DEF_FUNC_ID_LSN;
    self->pb_msg.id.op_type = MSG_DEF_OP_TYPE;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

