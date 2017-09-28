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
 * \brief Implementation of Application Message Classes
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_AMSMSG
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_amsmessage.h"
#include "ucs_dl.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

#define SELF_RX                     ((Amsg_IntMsgRx_t*)(void*)(self))
#define SELF_TX                     ((Amsg_IntMsgTx_t*)(void*)(self))

#define AMSG_TX_BACKUP_ADDR_NONE    0U

/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
static Ucs_AmsRx_ReceiveType_t Amsg_RxGetReceiveType(uint16_t destination_address);
static void Amsg_TxRestoreDestinationAddr(Ucs_AmsTx_Msg_t *self);

/*------------------------------------------------------------------------------------------------*/
/* Tx Message                                                                                      */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Initializes aggregated objects
 *  \details    Needs to be called once before first usage. Call Amsg_TxHandleSetup() before 
 *              repeated usage. 
 *  \param      self          Reference to an internal Application Message Tx handle
 *  \param      info_ptr      Memory information required to free the object
 *  \param      free_fptr     Callback function which is invoked when the object is freed
 *  \param      free_inst_ptr The instance which is passed to free_fptr 
 */
void Amsg_TxCtor(Ucs_AmsTx_Msg_t *self, void *info_ptr, Amsg_TxFreedCb_t free_fptr, void *free_inst_ptr)
{
    /* cleanup complete object */
    MISC_MEM_SET((void*)self, 0, sizeof(Amsg_IntMsgTx_t));

    /* reset default references
    SELF_TX->memory_ptr        = NULL;
    SELF_TX->memory_sz         = NULL;
    SELF_TX->memory_info_ptr   = NULL;

    SELF_TX->complete_fptr     = NULL; 
    SELF_TX->complete_inst_ptr = NULL;
    SELF_TX->complete_sia_fptr = NULL; 

    SELF_TX->backup_dest_address = AMSG_TX_BACKUP_ADDR_NONE;*/

    SELF_TX->info_ptr = info_ptr;
    SELF_TX->free_fptr = free_fptr;
    SELF_TX->free_inst_ptr = free_inst_ptr;
    SELF_TX->next_segment_cnt = 0xFFFFU;            /* start with TelId "4" */
    SELF_TX->temp_result = UCS_MSG_STAT_OK;
    SELF_TX->ignore_wrong_target = false;

    Dln_Ctor(&SELF_TX->node, self);                 /* initialize node */
}

/*! \brief  Sets payload memory provided by memory management and updates data pointer and size
 *  \param  self            The instance
 *  \param  mem_ptr         Reference to the provided memory chunk
 *  \param  mem_size        Size of the provided memory chunk
 *  \param  mem_info_ptr    Optional reference for memory management
 */
void Amsg_TxSetInternalPayload(Ucs_AmsTx_Msg_t *self, uint8_t *mem_ptr, uint16_t mem_size, void *mem_info_ptr)
{
    SELF_TX->memory_ptr        = mem_ptr;
    SELF_TX->memory_sz         = mem_size;
    SELF_TX->memory_info_ptr   = mem_info_ptr;

    SELF_TX->pb_msg.data_ptr   = mem_ptr;
    SELF_TX->pb_msg.data_size  = mem_size;
}

/*! \brief   Prepares the message object for re-usage
 *  \details The public message structure is re-initialized. The internal payload
 *           is assigned to the public data reference.
 *  \param   self            The instance
 */
void Amsg_TxReuse(Ucs_AmsTx_Msg_t *self)
{
    MISC_MEM_SET((void *)&SELF_TX->pb_msg, 0, sizeof(SELF_TX->pb_msg));  /* cleanup public object */
    /* SELF_TX->backup_dest_address = AMSG_TX_BACKUP_ADDR_NONE; */

    SELF_TX->pb_msg.data_ptr   = SELF_TX->memory_ptr;   /* reset public payload */
    SELF_TX->pb_msg.data_size  = SELF_TX->memory_sz;

    SELF_TX->next_segment_cnt  = 0xFFFFU;               /* start with TelId "4" */
    SELF_TX->temp_result       = UCS_MSG_STAT_OK;
}

/*! \brief   Assigns a Tx complete callback function
 *  \details It is not possible to assign the single and multiple instance callback
 *           at the same time. This function shall be called before message transmission. 
 *  \param   self     The instance
 *  \param   compl_sia_fptr  Reference to the single instance callback function
 *  \param   compl_fptr      Reference to a multiple instance callback function
 *  \param   compl_inst_ptr  Instance which is invoked by compl_fptr()
 */
void Amsg_TxSetCompleteCallback(Ucs_AmsTx_Msg_t *self, Amsg_TxCompleteSiaCb_t compl_sia_fptr, 
                                Amsg_TxCompleteCb_t compl_fptr, void* compl_inst_ptr)
{
    SELF_TX->complete_sia_fptr = compl_sia_fptr;
    SELF_TX->complete_fptr     = compl_fptr;
    SELF_TX->complete_inst_ptr = compl_inst_ptr;
}

/*! \brief  Invokes the correct callback function to notify the transmission result
 *          and frees the memory
 *  \param  self    Reference to the related message object
 *  \param  result  The transmission result
 *  \param  info    The INIC transmission result
 */
void Amsg_TxNotifyComplete(Ucs_AmsTx_Msg_t *self, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info)
{
    Amsg_TxRestoreDestinationAddr(self); 

    if (SELF_TX->complete_sia_fptr != NULL)                      /* invoke single instance API callback */
    {
        SELF_TX->complete_sia_fptr(self, result, info);
    }
    else if (SELF_TX->complete_fptr != NULL)
    {
        SELF_TX->complete_fptr(self, result, info, SELF_TX->complete_inst_ptr);
    }

    TR_ASSERT(NULL, "[AMSG_TX]", (SELF_TX->free_fptr != NULL));
    if (SELF_TX->free_fptr != NULL)
    {
        SELF_TX->free_fptr(SELF_TX->free_inst_ptr, self);
    }
}

/*! \brief  Frees an unused message object to the owning pool
 *  \param  self    Reference to the message object
 */
void Amsg_TxFreeUnused(Ucs_AmsTx_Msg_t *self)
{
    TR_ASSERT(NULL, "[AMSG_TX]", (SELF_TX->free_fptr != NULL));
    if (SELF_TX->free_fptr != NULL)
    {
        SELF_TX->free_fptr(SELF_TX->free_inst_ptr, self);
    }
}

/*! \brief    Updates the transmission result
 *  \param    self    Reference to the related message object
 *  \param    result  The latest MCM transmission result
 *  \details  Since the transmission result of an application message may
 *            consist of multiple telegram transmission results, it is 
 *            important to store the final transmission error. An error cannot
 *            be overwritten by a success.
 */
void Amsg_TxUpdateResult(Ucs_AmsTx_Msg_t *self, Ucs_MsgTxStatus_t result)
{
    if (result != UCS_MSG_STAT_OK)            /* store the latest error and do not overwrite with success */
    {
        SELF_TX->temp_result = result;
    }
}

/*! \brief  Returns the latest AMS transmission result code
 *  \param  self    Reference to the related message object
 *  \return Returns the transmission result which shall be notified to the application
 */
Ucs_AmsTx_Result_t Amsg_TxGetResultCode(Ucs_AmsTx_Msg_t *self)
{
    Ucs_AmsTx_Result_t res = UCS_AMSTX_RES_SUCCESS;                 /* success is the expected result */

    switch (SELF_TX->temp_result)
    {
        case UCS_MSG_STAT_OK:
            res = UCS_AMSTX_RES_SUCCESS;                            /* success is the expected result */
            break;
        case UCS_MSG_STAT_ERROR_BF: 
        case UCS_MSG_STAT_ERROR_CRC: 
        case UCS_MSG_STAT_ERROR_ID: 
        case UCS_MSG_STAT_ERROR_ACK: 
        case UCS_MSG_STAT_ERROR_TIMEOUT:
            res = UCS_AMSTX_RES_ERR_RETRIES_EXP;                    /* transmission failed, retries are possible */
            break;
        case UCS_MSG_STAT_ERROR_FATAL_WT:
        case UCS_MSG_STAT_ERROR_FATAL_OA:
            res = UCS_AMSTX_RES_ERR_INVALID_TGT;                    /* no network node found */
            break;
        case UCS_MSG_STAT_ERROR_NA_TRANS:
        case UCS_MSG_STAT_ERROR_NA_OFF:
            res = UCS_AMSTX_RES_ERR_NOT_AVAILABLE;              /* "not available" */
            break;
        case UCS_MSG_STAT_ERROR_SYNC:
            res = UCS_AMSTX_RES_ERR_NOT_AVAILABLE;
            break;
        default:
            res = UCS_AMSTX_RES_ERR_UNEXPECTED;                     /* unexpected network transmission state */
            break;
    }

    return res;
}

/*! \brief  Returns the latest MCM transmission error
 *  \param  self    Reference to the related message object
 *  \return Returns the INIC transmission result which is provided as additional info
 */
Ucs_AmsTx_Info_t Amsg_TxGetResultInfo(Ucs_AmsTx_Msg_t *self)
{
    Ucs_AmsTx_Info_t res = (Ucs_AmsTx_Info_t)SELF_TX->temp_result;

    if ((SELF_TX->temp_result == UCS_MSG_STAT_ERROR_FATAL_WT) && (SELF_TX->ignore_wrong_target != false))
    {
        res = UCS_AMSTX_I_SUCCESS;
    }

    return res;
}

/*! \brief  Queues a Tx message at the tail of a list
 *  \param  self     The instance
 *  \param  list_ptr Reference to the list
 */
void Amsg_TxEnqueue(Ucs_AmsTx_Msg_t* self, CDlList* list_ptr)
{
    Dl_InsertTail(list_ptr, &SELF_TX->node);
}

/*! \brief  Retrieves the next segment count
 *  \param  self     The instance
 *  \return The next segment count as uint16_t
 */
uint16_t Amsg_TxGetNextSegmCnt(Ucs_AmsTx_Msg_t *self)
{
    return SELF_TX->next_segment_cnt;
}

/*! \brief  Increments the next segment count
 *  \param  self     The instance
 */
void Amsg_TxIncrementNextSegmCnt(Ucs_AmsTx_Msg_t *self)
{
    SELF_TX->next_segment_cnt++;
}

/*! \brief  Retrieves the follower id which labels all telegrams of a segmented message
 *  \param  self     The instance
 *  \return The follower id
 */
uint8_t Amsg_TxGetFollowerId(Ucs_AmsTx_Msg_t *self)
{
    return SELF_TX->follower_id;
}

/*! \brief  Sets the follower id which labels all telegrams of a segmented message
 *  \param  self     The instance
 *  \param  id       The follower id
 */
void Amsg_TxSetFollowerId(Ucs_AmsTx_Msg_t *self, uint8_t id)
{
    SELF_TX->follower_id = id;
}

/*! \brief   Replaces the current destination address by a new one. 
 *  \details The current destination address can be restore by Amsg_TxRestoreDestinationAddr().
 *  \param   self               The instance
 *  \param   new_destination    The new destination address
 */
void Amsg_TxReplaceDestinationAddr(Ucs_AmsTx_Msg_t *self, uint16_t new_destination)
{
    SELF_TX->backup_dest_address = self->destination_address;    /* internal backup of current destination address */
    self->destination_address = new_destination;                 /* replace public destination address */
}

/*! \brief   Restores the destination address which was saved by calling Amsg_TxReplaceDestinationAddr().
 *  \param   self               The instance
 */
static void Amsg_TxRestoreDestinationAddr(Ucs_AmsTx_Msg_t *self)
{
    if (SELF_TX->backup_dest_address != AMSG_TX_BACKUP_ADDR_NONE)
    {
        self->destination_address = SELF_TX->backup_dest_address;/* restore public destination address */ 
    }
}

/*! \brief  Removes a message from a given queue
 *  \param  self     The instance
 *  \param  list_ptr The queue that contains the message
 */
void Amsg_TxRemoveFromQueue(Ucs_AmsTx_Msg_t *self, CDlList *list_ptr)
{
    (void)Dl_Remove(list_ptr, &SELF_TX->node);
}

/*! \brief  Peeks a Tx message from the head of a list
 *  \param  list_ptr Reference to the list
 *  \return Reference to the Tx message
 */
Ucs_AmsTx_Msg_t* Amsg_TxPeek(CDlList* list_ptr)
{
    Ucs_AmsTx_Msg_t *msg_ptr = NULL;
    CDlNode *node_ptr = Dl_PeekHead(list_ptr);

    if (node_ptr != NULL)
    {
        msg_ptr = (Ucs_AmsTx_Msg_t*)Dln_GetData(node_ptr);
    }

    return msg_ptr;
}

/*! \brief  Removes a Tx message from the head of a list
 *  \param  list_ptr Reference to the list
 *  \return Reference to the Tx message
 */
Ucs_AmsTx_Msg_t* Amsg_TxDequeue(CDlList* list_ptr)
{
    Ucs_AmsTx_Msg_t *msg_ptr = NULL;
    CDlNode *node_ptr = Dl_PopHead(list_ptr);

    if (node_ptr != NULL)
    {
        msg_ptr = (Ucs_AmsTx_Msg_t*)Dln_GetData(node_ptr);
    }

    return msg_ptr;
}

/*------------------------------------------------------------------------------------------------*/
/* Rx Message                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Initializes aggregated objects
 *  \details    Needs to be called once before first usage. Call Amsg_RxHandleSetup() before 
 *              repeated usage. 
 *  \param      self     Reference to an internal Application Message Rx handle
 *  \param      info_ptr Memory information required to free the object
 */
void Amsg_RxCtor(Ucs_AmsRx_Msg_t *self, void *info_ptr)
{
    Dln_Ctor(&SELF_RX->node, SELF_RX);
    SELF_RX->info_ptr          = info_ptr;                              /* reset memory information */
    SELF_RX->memory_sz         = 0U;
    SELF_RX->memory_ptr        = NULL;
    SELF_RX->memory_info_ptr   = NULL;
}

/*! \brief      Copies all attributes and payload from a Tx message to the Rx message
 *  \details    The caller has to ensure that the payload size of the Rx message is equal
 *              or greater than the payload size of the Tx message.
 *  \param      self           Reference to an Rx message object
 *  \param      tx_ptr         Reference to an Tx message object
 *  \param      source_address The source address that shall be set in the Rx message
 */
void Amsg_RxBuildFromTx(Ucs_AmsRx_Msg_t *self, Ucs_AmsTx_Msg_t *tx_ptr, uint16_t source_address)
{
    TR_ASSERT(NULL,"[AMSG]", (SELF_RX->memory_sz >= tx_ptr->data_size));

    self->receive_type      = Amsg_RxGetReceiveType(tx_ptr->destination_address);
    self->source_address    = source_address;
    self->msg_id            = tx_ptr->msg_id;
    self->data_size         = tx_ptr->data_size;

    Misc_MemCpy(self->data_ptr, tx_ptr->data_ptr, (size_t)self->data_size);
}

/*! \brief      Sets all attributes of an internal Rx message to valid values
 *  \param      self    Reference to an internal Rx message object
 *  \details    Assigned payload memory has to be freed before calling this function
 */
void Amsg_RxHandleSetup(Ucs_AmsRx_Msg_t *self)
{
    MISC_MEM_SET((void *)&SELF_RX->pb_msg, 0, sizeof(SELF_RX->pb_msg)); /* cleanup public message object */
    SELF_RX->pb_msg.data_ptr   = SELF_RX->memory_ptr;                   /* set data to valid memory */
    SELF_RX->gc_marker         = false;                                 /* reset garbage collector flag */
    SELF_RX->exp_tel_cnt       = 0U;                                    /* reset TelCnt */
}

/*! \brief  Evaluates if an Application Message has the same functional address
 *          as a MOST telegram
 *  \param  self    Reference to an internal Application Message Rx handle
 *  \param  tel_ptr Reference to a MOST message object
 *  \return Returns \c true if both message objects have the same functional address,
 *          otherwise \c false.
 */
bool Amsg_RxHandleIsIdentical(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t *tel_ptr)
{
    bool result = false;
    uint16_t msg_id = Msg_GetAltMsgId((CMessage*)(void*)tel_ptr);

    if ((self->source_address == tel_ptr->source_addr)
        && (self->msg_id == msg_id))
    {
        result = true;
    }

    return result;
}

/*! \brief  Copies the Rx message signature from a MOST message object to an 
 *          internal Application message object
 *  \param  self    Reference to an internal Application Message Rx handle
 *  \param  src_ptr Reference to a MOST message object
 */
void Amsg_RxCopySignatureFromTel(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* src_ptr)
{
    self->source_address   = src_ptr->source_addr;
    self->receive_type     = Amsg_RxGetReceiveType(src_ptr->destination_addr);
    self->msg_id           = Msg_GetAltMsgId((CMessage*)(void*)src_ptr);
}

/*! \brief  Copies the Rx message signature from an internal Application
 *          message object to a MOST message object
 *  \param  self     Reference to an internal Application Message Rx handle
 *  \param  target_ptr  Reference to a MOST message object
 */
void Amsg_RxCopySignatureToTel(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* target_ptr)
{
    target_ptr->source_addr        = self->source_address;
    target_ptr->destination_addr   = UCS_ADDR_DEBUG;
    Msg_SetAltMsgId((CMessage*)(void*)target_ptr, self->msg_id);
}

/*! \brief  Retrieves the addressing type related to a destination_address of an Rx message
 *  \param  destination_address  The destination address of an Rx message
 *  \return The receive type related to the destination address
 */
static Ucs_AmsRx_ReceiveType_t Amsg_RxGetReceiveType(uint16_t destination_address)
{
    Ucs_AmsRx_ReceiveType_t ret = UCS_AMSRX_RCT_SINGLECAST;

    if ((destination_address == UCS_ADDR_BROADCAST_BLOCKING) ||
        (destination_address == UCS_ADDR_BROADCAST_UNBLOCKING))
    {
        ret = UCS_AMSRX_RCT_BROADCAST;
    }
    else if ((destination_address >= 0x0300U) &&        /* 0x300..0x3FF is reserved for group cast */
             (destination_address <  0x0400U))
    {
        ret = UCS_AMSRX_RCT_GROUPCAST;
    }

    return ret;
}

/*! \brief  Appends payload of an Rx MOST message object to internal Application
 *          message object
 *  \param  self    Reference to an internal Application Message Rx handle
 *  \param  src_ptr Reference to a MOST message object
 *  \return Returns \c true if the payload was appended successfully, 
 *          otherwise \c false.
 */
bool Amsg_RxAppendPayload(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* src_ptr)
{
    uint8_t cnt;
    bool ret = false;
    const uint16_t curr_size = SELF_RX->pb_msg.data_size;                /* get current message size */

    if ((SELF_RX->memory_sz - src_ptr->tel.tel_len) >= SELF_RX->pb_msg.data_size) /* is size sufficient */
    {
        for (cnt = 0U; cnt < src_ptr->tel.tel_len; cnt++)
        {
             SELF_RX->pb_msg.data_ptr[curr_size + (uint16_t)cnt] = src_ptr->tel.tel_data_ptr[cnt];
        }

         SELF_RX->pb_msg.data_size = curr_size + src_ptr->tel.tel_len;    /* update message size */
         SELF_RX->exp_tel_cnt++;
        ret = true;
    }

    return ret;
}

/*! \brief   Copies data to allocated payload buffer 
 *  \param   self       The instance
 *  \param   data       Reference to external payload data
 *  \param   data_sz    Size of external payload data 
 */
void Amsg_RxCopyToPayload(Ucs_AmsRx_Msg_t *self, uint8_t data[], uint8_t data_sz)
{
    MISC_MEM_CPY(&self->data_ptr[0], &data[0], (size_t)data_sz); /* parasoft-suppress  MISRA2004-20_3 "data_sz is limited and checked via Msg_VerifyContent()" */
    self->data_size = data_sz;         /* remember payload size */
}

/*! \brief   Checks if the message has externally allocated payload memory
 *  \param   self       The instance
 *  \return  Returns \c true if external payload is assigned to the message, otherwise \c false.
 */
bool Amsg_RxHasExternalPayload(Ucs_AmsRx_Msg_t *self)
{
    return (SELF_RX->memory_sz > 0U);
}

/*! \brief  Sets payload memory provided by memory management and updates data pointer and size
 *  \param  self     The instance
 *  \param  mem_ptr  Reference to the provided memory chunk
 *  \param  mem_size Size of the provided memory chunk
 *  \param  info_ptr Optional reference for memory management
 */
void Amsg_RxHandleSetMemory(Ucs_AmsRx_Msg_t *self, uint8_t *mem_ptr, uint16_t mem_size, void *info_ptr)
{
    SELF_RX->memory_ptr        = mem_ptr;
    SELF_RX->memory_info_ptr   = info_ptr;
    SELF_RX->memory_sz         = mem_size;

    SELF_RX->pb_msg.data_ptr   = mem_ptr;
    SELF_RX->pb_msg.data_size  = 0U;
}

/*! \brief  Queues an Rx message at the tail of a list
 *  \param  self     The instance
 *  \param  list_ptr Reference to the list
 */
void Amsg_RxEnqueue(Ucs_AmsRx_Msg_t* self, CDlList* list_ptr)
{
    Dl_InsertTail(list_ptr, &SELF_RX->node);
}

/*! \brief  Sets or resets the garbage collector flag
 *  \param  self    The instance
 *  \param  value   New value of the flag
 */
void Amsg_RxSetGcMarker(Ucs_AmsRx_Msg_t* self, bool value)
{
    SELF_RX->gc_marker = value; 
}

/*! \brief  Retrieves the value of the garbage collector flag
 *  \param  self    The instance
 *  \return The current value of the flag
 */
bool Amsg_RxGetGcMarker(Ucs_AmsRx_Msg_t* self)
{
    return SELF_RX->gc_marker;
}

/*! \brief  Retrieves the next expected telegram count
 *  \param  self    The instance
 *  \return The next  expected telegram count as uint8_t
 */
uint8_t Amsg_RxGetExpTelCnt(Ucs_AmsRx_Msg_t* self)
{
    return SELF_RX->exp_tel_cnt;
}

/*! \brief  Peeks an Rx message from the head of a list
 *  \param  list_ptr Reference to the list
 *  \return Reference to the Rx message
 */
Ucs_AmsRx_Msg_t* Amsg_RxPeek(CDlList* list_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;
    CDlNode *node_ptr = Dl_PeekHead(list_ptr);

    if (node_ptr != NULL)
    {
        msg_ptr = (Ucs_AmsRx_Msg_t*)Dln_GetData(node_ptr);
    }

    return msg_ptr;
}

/*! \brief  Removes an Rx message from the head of a list
 *  \param  list_ptr Reference to the list
 *  \return Reference to the Rx message
 */
Ucs_AmsRx_Msg_t* Amsg_RxDequeue(CDlList* list_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;
    CDlNode *node_ptr = Dl_PopHead(list_ptr);

    if (node_ptr != NULL)
    {
        msg_ptr = (Ucs_AmsRx_Msg_t*)Dln_GetData(node_ptr);
    }

    return msg_ptr;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

