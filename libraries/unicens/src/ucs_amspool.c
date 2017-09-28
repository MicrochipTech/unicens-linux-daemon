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
 * \brief Implementation of Application Message Pool
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_AMSPOOL
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_amspool.h"
#include "ucs_amsmessage.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal macros                                                                                */
/*------------------------------------------------------------------------------------------------*/
#define INT_RX(ptr) ((Amsg_IntMsgRx_t*)(void*)(ptr)) /* parasoft-suppress  MISRA2004-19_7 "common definition of type cast improves code" */
#define INT_TX(ptr) ((Amsg_IntMsgTx_t*)(void*)(ptr)) /* parasoft-suppress  MISRA2004-19_7 "common definition of type cast improves code" */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Amsp_FreeTxObj(void *self, Ucs_AmsTx_Msg_t* msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Initialization                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of application message pool class 
 *  \param  self                The instance
 *  \param  mem_allocator_ptr   Reference to memory allocator
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Amsp_Ctor(CAmsMsgPool *self, Ams_MemAllocator_t *mem_allocator_ptr, void *ucs_user_ptr)
{
    self->ucs_user_ptr = ucs_user_ptr;
    self->allocator_ptr = mem_allocator_ptr;
    self->rx_rsvd_msg_ptr = Amsp_AllocRxObj(self, 45U);
    self->rx_rsvd_msg_ref = self->rx_rsvd_msg_ptr;
    self->terminated = false;
    self->tx_notify_freed = false;
    self->rx_notify_freed = false;
    Sub_Ctor(&self->tx_freed_subject, self->ucs_user_ptr);
    Sub_Ctor(&self->rx_freed_subject, self->ucs_user_ptr);

    TR_ASSERT(self->ucs_user_ptr, "[AMSP]", (self->rx_rsvd_msg_ptr != NULL));
}

/*! \brief  Frees pre-allocated message memory
 *  \param  self    The instance
 */
void Amsp_Cleanup(CAmsMsgPool *self)
{
    Amsg_IntMsgRx_t *msg_ptr = INT_RX(self->rx_rsvd_msg_ptr);
    TR_INFO((self->ucs_user_ptr, "[AMSP]", "Amsp_Cleanup: rx_rsvd_msg_ptr=0x%p", 1U, self->rx_rsvd_msg_ptr));

    self->terminated = true;
    self->tx_notify_freed = false;
    self->rx_notify_freed = false;

    if (msg_ptr != NULL)
    {
        self->allocator_ptr->free_fptr(self->allocator_ptr->inst_ptr, msg_ptr->memory_ptr, AMS_MU_RX_PAYLOAD, msg_ptr->memory_info_ptr);
        self->allocator_ptr->free_fptr(self->allocator_ptr->inst_ptr, msg_ptr, AMS_MU_RX_OBJECT, msg_ptr->info_ptr);
        self->rx_rsvd_msg_ref = NULL;
        self->rx_rsvd_msg_ptr = NULL;
    }
}

/*! \brief  Assigns an observer which is invoked as soon as memory dedicated to a Tx message is 
 *          freed.The data_ptr of the update callback function is not used (always \c NULL). 
 *          See \ref Obs_UpdateCb_t. 
 *  \param  self            The instance
 *  \param  observer_ptr    The observer
 */
void Amsp_AssignTxFreedObs(CAmsMsgPool *self, CObserver *observer_ptr)
{
    (void)Sub_AddObserver(&self->tx_freed_subject, observer_ptr);
}

/*! \brief  Assigns an observer which is invoked as soon as memory dedicated to a Tx message is 
 *          freed.The data_ptr of the update callback function is not used (always \c NULL). 
 *          See \ref Obs_UpdateCb_t. 
 *  \param  self            The instance
 *  \param  observer_ptr    The observer
 */
void Amsp_AssignRxFreedObs(CAmsMsgPool *self, CObserver *observer_ptr)
{
    (void)Sub_AddObserver(&self->rx_freed_subject, observer_ptr);
}

/*------------------------------------------------------------------------------------------------*/
/* Tx allocations                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Allocates an internal Tx message object (without payload)
 *  \param  self        The instance
 *  \param  payload_sz  The required payload size in bytes 
 *  \return Reference to the Tx message object if the allocation succeeds. Otherwise \c NULL.
 */
Ucs_AmsTx_Msg_t* Amsp_AllocTxObj(CAmsMsgPool *self, uint16_t payload_sz)
{
    void *payload_info_ptr = NULL;
    void *payload_ptr = NULL;
    void *obj_info_ptr = NULL;
    Ucs_AmsTx_Msg_t *msg_ptr = (Ucs_AmsTx_Msg_t*)self->allocator_ptr->alloc_fptr(self->allocator_ptr->inst_ptr, AMSG_TX_OBJECT_SZ, AMS_MU_TX_OBJECT, &obj_info_ptr);
    TR_INFO((self->ucs_user_ptr, "[AMSP]", "Allocating TxObject: msg_ptr=0x%p, size=%d, info_ptr=0x%p", 3U, msg_ptr, AMSG_TX_OBJECT_SZ, obj_info_ptr));

    if (msg_ptr != NULL)
    {
        if (payload_sz > 0U)
        {
            payload_ptr = self->allocator_ptr->alloc_fptr(self->allocator_ptr->inst_ptr, payload_sz, AMS_MU_TX_PAYLOAD, &payload_info_ptr);
            TR_INFO((self->ucs_user_ptr, "[AMSP]", "Allocating TxPayload: msg_ptr=0x%p, mem_ptr=0x%p, size=%d, info_ptr=0x%p", 4U, msg_ptr, payload_ptr, payload_sz, payload_info_ptr));

            if (payload_ptr == NULL)
            {
                TR_INFO((self->ucs_user_ptr, "[AMSP]", "Freeing TxObject: msg_ptr=0x%p, info_ptr=0x%p", 2U, msg_ptr, obj_info_ptr));
                self->allocator_ptr->free_fptr(self->allocator_ptr->inst_ptr, msg_ptr, AMS_MU_TX_OBJECT, obj_info_ptr);
                msg_ptr = NULL;
            }
        }
    }

    if (msg_ptr != NULL)
    {
        Amsg_TxCtor(msg_ptr, obj_info_ptr, &Amsp_FreeTxObj, self);

        if (payload_ptr != NULL)
        {
            Amsg_TxSetInternalPayload(msg_ptr, (uint8_t*)payload_ptr, payload_sz, payload_info_ptr);
        }
    }
    else
    {
        self->tx_notify_freed = true;
    }

    return msg_ptr;
}

/*! \brief      Frees an internal Tx message object including its payload
 *  \param      self        The instance
 *  \param      msg_ptr     Reference to the internal Tx message object
 */
static void Amsp_FreeTxObj(void *self, Ucs_AmsTx_Msg_t* msg_ptr)
{
    CAmsMsgPool *self_ = (CAmsMsgPool*)self;
    Amsg_IntMsgTx_t *obj_ptr = INT_TX(msg_ptr);

    if (obj_ptr->memory_ptr != NULL)
    {
        TR_INFO((self_->ucs_user_ptr, "[AMSP]", "Freeing TxPayload: msg_ptr=0x%p, mem_ptr=0x%p, info_ptr=0x%p", 3U, msg_ptr, obj_ptr->memory_ptr, obj_ptr->memory_info_ptr));
        self_->allocator_ptr->free_fptr(self_->allocator_ptr->inst_ptr, obj_ptr->memory_ptr, AMS_MU_TX_PAYLOAD, obj_ptr->memory_info_ptr);
        Amsg_TxSetInternalPayload(msg_ptr, NULL, 0U, NULL);
    }

    TR_INFO((self_->ucs_user_ptr, "[AMSP]", "Freeing TxObject: msg_ptr=0x%p, info_ptr=0x%p", 2U, msg_ptr, obj_ptr->info_ptr));
    self_->allocator_ptr->free_fptr(self_->allocator_ptr->inst_ptr, msg_ptr, AMS_MU_TX_OBJECT, obj_ptr->info_ptr);

    if (self_->tx_notify_freed)
    {
        Sub_Notify(&self_->tx_freed_subject, NULL);
        self_->tx_notify_freed = false;
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Rx allocations                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Allocates an internal Rx message object (optionally with payload)
 *  \param  self        The instance
 *  \param  payload_sz  The required payload size that shall be allocated and assigned to the object.
 *                      Value "0" means that no payload memory shall be allocated in the same turn.
 *  \return Reference to the Rx message object if the allocation succeeds. Otherwise \c NULL.
 */
Ucs_AmsRx_Msg_t* Amsp_AllocRxObj(CAmsMsgPool *self, uint16_t payload_sz)
{
    void *info_ptr = NULL;
    Ucs_AmsRx_Msg_t *msg_ptr = (Ucs_AmsRx_Msg_t*)self->allocator_ptr->alloc_fptr(self->allocator_ptr->inst_ptr, AMSG_RX_OBJECT_SZ, AMS_MU_RX_OBJECT, &info_ptr);

    TR_INFO((self->ucs_user_ptr, "[AMSP]", "Allocating RxObject: msg_ptr=0x%p, size=%d, info_ptr=0x%p", 3U, msg_ptr, AMSG_RX_OBJECT_SZ, info_ptr));

    if (msg_ptr != NULL)
    {
        Amsg_RxCtor(msg_ptr, info_ptr);
        Amsg_RxHandleSetup(msg_ptr);

        if (payload_sz != 0U)
        {
            if (!Amsp_AllocRxPayload(self, payload_sz, msg_ptr))
            {
                Amsp_FreeRxObj(self, msg_ptr);  /* payload allocation has failed - release message object */
                msg_ptr = NULL;
            }
        }
    }

    return msg_ptr;
}

/*! \brief  Allocates a reserved Rx message object with payload up to 45 bytes payload
 *  \param  self        The instance
 *  \return Reference to the Rx message object if the allocation succeeds. Otherwise \c NULL.
 */
Ucs_AmsRx_Msg_t* Amsp_AllocRxRsvd(CAmsMsgPool *self)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;

    if (self->rx_rsvd_msg_ptr != NULL)
    {
        msg_ptr = self->rx_rsvd_msg_ptr;
        self->rx_rsvd_msg_ptr = NULL;
        Amsg_RxHandleSetup(msg_ptr);
        TR_INFO((self->ucs_user_ptr, "[AMSP]", "Retrieving reserved RxObject: msg_ptr=0x%p", 1U, msg_ptr));
    }
    else
    {
        self->rx_notify_freed = true;
    }

    return msg_ptr;
}

/*! \brief  Allocates payload for an internal Rx message object 
 *  \param  self        The instance
 *  \param  payload_sz  Payload size in bytes
 *  \param  msg_ptr     Reference to the internal Rx message object
 *  \return Returns \c true if the allocation succeeds. Otherwise \c NULL.
 */
bool Amsp_AllocRxPayload(CAmsMsgPool *self, uint16_t payload_sz, Ucs_AmsRx_Msg_t* msg_ptr)
{
    bool success = false;
    void *info_ptr = NULL;
    void *mem_ptr = self->allocator_ptr->alloc_fptr(self->allocator_ptr->inst_ptr, payload_sz, AMS_MU_RX_PAYLOAD, &info_ptr);

    TR_INFO((self->ucs_user_ptr, "[AMSP]", "Allocating RxPayload: msg_ptr=0x%p, mem_ptr=0x%p, size=%d, info_ptr=0x%p", 4U, msg_ptr, mem_ptr, payload_sz, info_ptr));
    TR_ASSERT(self->ucs_user_ptr, "[AMSP]", (msg_ptr != NULL));                  /* message reference is required */
    TR_ASSERT(self->ucs_user_ptr, "[AMSP]", (msg_ptr != self->rx_rsvd_msg_ref)); /* forbidden overwrite of pre-allocated message payload */

    if (mem_ptr != NULL)
    {
        Amsg_RxHandleSetMemory(msg_ptr, (uint8_t*)mem_ptr, payload_sz, info_ptr);
        success = true;
    }

    return success;
}

/*! \brief      Frees an internal Rx message object 
 *  \param      self        The instance
 *  \param      msg_ptr     Reference to the internal Rx message object
 *  \details    Payload that is assigned to the message object has to be freed 
 *              separately by using Amsp_FreeRxPayload().
 */
void Amsp_FreeRxObj(CAmsMsgPool *self, Ucs_AmsRx_Msg_t* msg_ptr)
{
    if (msg_ptr == self->rx_rsvd_msg_ref)
    {
        TR_ASSERT(self->ucs_user_ptr, "[AMSP]", (self->rx_rsvd_msg_ptr == NULL));    /* before freeing, message shall be reserved */
        TR_INFO((self->ucs_user_ptr, "[AMSP]", "Restoring reserved RxObject: msg_ptr=0x%p", 1U, msg_ptr));
        self->rx_rsvd_msg_ptr = self->rx_rsvd_msg_ref;                              /* restore reserved message */

        if (self->terminated != false)
        {                                                                           /* also free reserved message if it is freed */
            Amsp_Cleanup(self);                                                     /* from any queue after Amsp_Cleanup() */
        }
    }
    else 
    { 
        Amsg_IntMsgRx_t *obj_ptr = INT_RX(msg_ptr);
        TR_INFO((self->ucs_user_ptr, "[AMSP]", "Freeing RxObject: msg_ptr=0x%p, info_ptr=0x%p", 2U, msg_ptr, obj_ptr->info_ptr));
        self->allocator_ptr->free_fptr(self->allocator_ptr->inst_ptr, msg_ptr, AMS_MU_RX_OBJECT, obj_ptr->info_ptr);
    }

    if (self->rx_notify_freed)
    {
        Sub_Notify(&self->rx_freed_subject, NULL);
        self->rx_notify_freed = false;
    }
}

/*! \brief  Frees payload that is associated with an internal Rx message object 
 *  \param  self        The instance
 *  \param  msg_ptr     Reference to the internal Rx message object
 */
void Amsp_FreeRxPayload(CAmsMsgPool *self, Ucs_AmsRx_Msg_t* msg_ptr)
{
    Amsg_IntMsgRx_t *obj_ptr = INT_RX(msg_ptr);

    if (msg_ptr == self->rx_rsvd_msg_ref)
    {
        TR_ASSERT(self->ucs_user_ptr, "[AMSP]", (self->rx_rsvd_msg_ptr == NULL));    /* release payload before object */
        TR_INFO((self->ucs_user_ptr, "[AMSP]", "Restoring reserved RxPayload: msg_ptr=0x%p", 1U, msg_ptr));
    }
    else if (obj_ptr->memory_ptr != NULL)
    {
        TR_INFO((self->ucs_user_ptr, "[AMSP]", "Freeing RxPayload: msg_ptr=0x%p, mem_ptr=0x%p, info_ptr=0x%p", 3U, msg_ptr, obj_ptr->memory_ptr, obj_ptr->memory_info_ptr));
        self->allocator_ptr->free_fptr(self->allocator_ptr->inst_ptr, obj_ptr->memory_ptr, AMS_MU_RX_PAYLOAD, obj_ptr->memory_info_ptr);
        Amsg_RxHandleSetMemory(msg_ptr, NULL, 0U, NULL);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

