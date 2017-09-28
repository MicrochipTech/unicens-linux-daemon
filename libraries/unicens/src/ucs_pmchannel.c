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
 * \brief Implementation of Port Message Channel
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pmchannel.h"
#include "ucs_pmp.h"
#include "ucs_pmcmd.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal typedefs                                                                              */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
/* LLD related interface functions */
static Ucs_Lld_RxMsg_t* Pmch_RxAllocate(void *self, uint16_t buffer_size);
static void Pmch_RxUnused(void *self, Ucs_Lld_RxMsg_t *msg_ptr);
static void Pmch_RxReceive(void *self, Ucs_Lld_RxMsg_t *msg_ptr);
static void Pmch_TxRelease(void *self, Ucs_Lld_TxMsg_t *msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Constructor of class CPmChannel
 *  \param  self        The instance
 *  \param  init_ptr    Reference to initialization data structure
 */
void Pmch_Ctor(CPmChannel *self, const Pmch_InitData_t *init_ptr)
{
    uint16_t cnt;
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->init_data    = *init_ptr;
    self->lld_active   = false;

    self->ucs_iface.rx_allocate_fptr       = &Pmch_RxAllocate;
    self->ucs_iface.rx_receive_fptr        = &Pmch_RxReceive;
    self->ucs_iface.rx_free_unused_fptr    = &Pmch_RxUnused;
    self->ucs_iface.tx_release_fptr        = &Pmch_TxRelease;

    Pool_Ctor(&self->rx_msgs_pool, self->rx_msgs,                   /* initialize Rx message pool */
              PMCH_POOL_SIZE_RX, self->init_data.ucs_user_ptr);
    for (cnt = 0U; cnt < PMCH_POOL_SIZE_RX; cnt++)                  /* and assign LLD Rx handles  */
    {
        Msg_SetLldHandle(&self->rx_msgs[cnt], &self->lld_rx_msgs[cnt]);
        self->lld_rx_msgs[cnt].msg_ptr = &self->rx_msgs[cnt];
    }
}

/*! \brief      Registers an Rx callback function dedicated to one FIFO
 *  \param      self      The instance
 *  \param      fifo_id   The FIFO identifier
 *  \param      rx_fptr   The Rx callback function
 *  \param      inst_ptr  Reference to the instance required to invoke the callback
 */
void Pmch_RegisterReceiver(CPmChannel *self, Pmp_FifoId_t fifo_id, Pmch_OnRxMsg_t rx_fptr, void *inst_ptr)
{
    TR_ASSERT(self->init_data.ucs_user_ptr, "[PMCH]", (((uint8_t)fifo_id == (uint8_t)PMP_FIFO_ID_ICM)||((uint8_t)fifo_id == (uint8_t)PMP_FIFO_ID_MCM)||((uint8_t)fifo_id == (uint8_t)PMP_FIFO_ID_RCM)));

    self->receivers[fifo_id].rx_fptr = rx_fptr;
    self->receivers[fifo_id].inst_ptr = inst_ptr;
}

/*! \brief      Un-initializes the LLD interface of the channel
 *  \param      self    The instance
 */
void Pmch_Initialize(CPmChannel *self)
{
    if (self->lld_active == false)
    {
        self->lld_active   = true;
        TR_INFO((self->init_data.ucs_user_ptr, "[PMCH]", "Pmch_Initialize(): LLD_START()", 0U));
        self->init_data.lld_iface.start_fptr(&self->ucs_iface, self, self->init_data.lld_iface.lld_user_ptr);
    }
}

/*! \brief      Un-initializes the LLD interface of the channel
 *  \param      self    The instance
 */
extern void Pmch_Uninitialize(CPmChannel *self)
{
    TR_INFO((self->init_data.ucs_user_ptr, "[PMCH]", "Pmch_Uninitialize(): Channel un-synchronization started", 0U));

    if (self->lld_active != false)
    {
        self->lld_active = false;
        TR_INFO((self->init_data.ucs_user_ptr, "[PMCH]", "Pmch_Uninitialize(): LLD_STOP()", 0U));
        self->init_data.lld_iface.stop_fptr(self->init_data.lld_iface.lld_user_ptr);
    }
}

/*! \brief      Wrapper for LLD transmit
 *  \details    This function which shall be used by all internal classes. No class shall
 *              invoke the LLD transmit function directly. Thus, it might be possible 
 *              in future to handle transmission failures and retries.
 *  \param      self    The instance
 *  \param      msg_ptr Reference to the public LLD message structure
 */
void Pmch_Transmit(CPmChannel *self, Ucs_Lld_TxMsg_t *msg_ptr)
{
    if (self->lld_active != false)
    {
        self->init_data.lld_iface.tx_transmit_fptr(msg_ptr, self->init_data.lld_iface.lld_user_ptr);
    }
    else
    {
        Pmch_TxRelease(self, msg_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* The exposed low-level driver interface                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Allocates an Rx message object 
 *  \param  self        The instance
 *  \param  buffer_size Size of the memory chunk in bytes which is needed to
 *                      copy the Rx message.
 *  \return Reference to an allocated Rx message object or \c NULL if no message object is available.
 */
static Ucs_Lld_RxMsg_t* Pmch_RxAllocate(void *self, uint16_t buffer_size)
{
    CMessage *msg_ptr = NULL;
    Ucs_Lld_RxMsg_t *handle = NULL;
    CPmChannel *self_ = (CPmChannel*)self;

    if (buffer_size <= MSG_SIZE_RSVD_BUFFER)
    {
        msg_ptr = Pool_GetMsg(&self_->rx_msgs_pool);

        if (msg_ptr != NULL)
        {
            Msg_Cleanup(msg_ptr);
            handle = &((Lld_IntRxMsg_t*)Msg_GetLldHandle(msg_ptr))->lld_msg;

            TR_ASSERT(self_->init_data.ucs_user_ptr, "[PMCH]", (handle != NULL));

            handle->data_size       = buffer_size;
            handle->data_ptr        = Msg_GetHeader(msg_ptr);
        }
        else
        {
            self_->rx_trigger_available = true;
            TR_INFO((self_->init_data.ucs_user_ptr, "[PMCH]", "Pmch_RxAllocate(): Allocation failed, size=%u", 1U, buffer_size));
        }
    }
    else
    {
        self_->rx_trigger_available = true;         
        TR_FAILED_ASSERT(self_->init_data.ucs_user_ptr, "[PMCH]");
    }

    return handle;
}

/*! \brief  Frees an unused Rx message object
 *  \param  self        The instance
 *  \param  msg_ptr     Reference to the unused Rx message object
 */
static void Pmch_RxUnused(void *self, Ucs_Lld_RxMsg_t *msg_ptr)
{
    CPmChannel *self_ = (CPmChannel*)self;
    CMessage *pb_handle = ((Lld_IntRxMsg_t*)(void*)msg_ptr)->msg_ptr;

    TR_ASSERT(self_->init_data.ucs_user_ptr, "[PMCH]", (pb_handle != NULL));
    Pmch_ReturnRxToPool(self_, pb_handle);
}

/*! \brief  Pass an Rx message to UNICENS 
 *  \param  self        The instance
 *  \param  msg_ptr     Reference to the Rx message object containing the received
 *                      message.
 */
static void Pmch_RxReceive(void *self, Ucs_Lld_RxMsg_t *msg_ptr)
{
    bool found = false;
    CPmChannel *self_ = (CPmChannel*)self;

    if (msg_ptr->data_ptr != NULL)
    {
        if (msg_ptr->data_size >= PMP_PM_MIN_SIZE_HEADER)                   /* ignore incomplete messages */
        {
            uint8_t fifo_no = (uint8_t)Pmp_GetFifoId(msg_ptr->data_ptr);    /* get channel id (FIFO number) */

            if ((fifo_no < PMP_MAX_NUM_FIFOS) && (self_->receivers[fifo_no].inst_ptr != NULL))
            {
                CMessage *handle = ((Lld_IntRxMsg_t*)(void*)msg_ptr)->msg_ptr;
                                                                            /* forward message to the respective FIFO/channel */
                self_->receivers[fifo_no].rx_fptr(self_->receivers[fifo_no].inst_ptr, handle); 
                found = true;
            }
            else
            {
                TR_ERROR((self_->init_data.ucs_user_ptr, "[PMCH]", "Pmch_RxReceive(): received message for unregistered FIFO no=%u", 1U, fifo_no));
            }
        }
        else
        {
            TR_ERROR((self_->init_data.ucs_user_ptr, "[PMCH]", "Pmch_RxReceive(): received incomplete message of size=%u", 1U, msg_ptr->data_size));
        }
    }
    else
    {
        TR_ERROR((self_->init_data.ucs_user_ptr, "[PMCH]", "Pmch_RxReceive(): message data is not valid", 0U));
    }

    if (found == false) 
    {
        Pmch_RxUnused(self_, msg_ptr);                                      /* Just return message to pool until PMC is implemented */
    }
}

/*! \brief  Notifies that the LLD no longer needs to access the Tx message object
 *  \param  self        The instance
 *  \param  msg_ptr     Reference to the Tx message object which is no longer accessed
 *                      by the low-level driver
 */
static void Pmch_TxRelease(void *self, Ucs_Lld_TxMsg_t *msg_ptr)
{
    CPmChannel *self_ = (CPmChannel*)self;
    Lld_IntTxMsg_t *tx_ptr = (Lld_IntTxMsg_t*)(void*)msg_ptr;

    if ((tx_ptr->owner_ptr == NULL) && (tx_ptr->msg_ptr == NULL))           /* tx_ptr is command */
    {
        Pmcmd_Release((CPmCommand*)(void*)tx_ptr);
    }
    else if (tx_ptr->owner_ptr != NULL)                                     /* release message to FIFO */
    {
        self_->init_data.tx_release_fptr(tx_ptr->owner_ptr, msg_ptr);
    }
    else
    {
        TR_FAILED_ASSERT(self_->init_data.ucs_user_ptr, "[PMCH]"); /* unknown FIFO - invalid message object */
    }

    TR_ASSERT(self_->init_data.ucs_user_ptr, "[PMCH]", (msg_ptr->custom_next_msg_ptr == NULL) );  /* concatenation destroyed by the LLD */

}

/*------------------------------------------------------------------------------------------------*/
/* FIFO Related Callback Functions                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Returns an unused Rx message object back to the pool
 *  \param  self    The instance
 *  \param  msg_ptr The unused Rx message object 
 */
void Pmch_ReturnRxToPool(void *self, CMessage *msg_ptr)
{
    CPmChannel *self_ = (CPmChannel*)self;

    Pool_ReturnMsg(msg_ptr);

    if (self_->rx_trigger_available == true)
    {
        self_->rx_trigger_available = false;

        if (self_->init_data.lld_iface.rx_available_fptr != NULL)
        {
            self_->init_data.lld_iface.rx_available_fptr(self_->init_data.lld_iface.lld_user_ptr);
        }
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

