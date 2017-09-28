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
 * \brief Implementation of AMS Segmentation Class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMSSEGM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_segmentation.h"
#include "ucs_ams.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
 /*!\brief Timeout for garbage collector */
static const uint16_t SEGM_GC_TIMEOUT = 5000U;  /* parasoft-suppress  MISRA2004-8_7 "intended usage as configuration parameter" */

/*------------------------------------------------------------------------------------------------*/
/* Internal typedefs                                                                              */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static Ucs_AmsRx_Msg_t *Segm_RxRetrieveProcessingHandle(CSegmentation *self, Msg_MostTel_t *tel_ptr);
static void Segm_RxStoreProcessingHandle(CSegmentation *self, Ucs_AmsRx_Msg_t *msg_ptr);
static bool Segm_RxSearchProcessingHandle(void *current_data, void *search_data);
static bool Segm_RxGcSetLabel(void *current_data, void *search_data);
static Ucs_AmsRx_Msg_t* Segm_RxProcessTelId0(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr);
static void             Segm_RxProcessTelId1(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr);
static void             Segm_RxProcessTelId2(CSegmentation *self, Msg_MostTel_t *tel_ptr);
static Ucs_AmsRx_Msg_t* Segm_RxProcessTelId3(CSegmentation *self, Msg_MostTel_t *tel_ptr);
static void             Segm_RxProcessTelId4(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Initialization methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of class CSegmentation
 *  \param self              The instance
 *  \param base_ptr          Reference to base services
 *  \param pool_ptr          Reference to the (Rx) message pool
 *  \param rx_def_payload_sz Default memory size that is allocated when receiving segmented messages 
 *                           without size prefix
 */
void Segm_Ctor(CSegmentation *self, CBase *base_ptr, CAmsMsgPool *pool_ptr, uint16_t rx_def_payload_sz)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->base_ptr = base_ptr;                  /* initialize members */
    self->pool_ptr = pool_ptr;

    self->rx_default_payload_sz = rx_def_payload_sz;

    Dl_Ctor(&self->processing_list, self->base_ptr->ucs_user_ptr);
    T_Ctor(&self->gc_timer);
    Tm_SetTimer(&self->base_ptr->tm,            /* start garbage collector timer */
                &self->gc_timer, 
                &Segm_RxGcScanProcessingHandles, 
                self, 
                SEGM_GC_TIMEOUT, 
                SEGM_GC_TIMEOUT
                );
}

/*! \brief Constructor of class CSegmentation
 *  \param self           The instance
 *  \param error_fptr     Reference to segmentation error callback function
 *  \param error_inst     Reference to segmentation error instance
 */
void Segm_AssignRxErrorHandler(CSegmentation *self, Segm_OnError_t error_fptr, void *error_inst)
{
    self->error_fptr = error_fptr;
    self->error_inst = error_inst;
}

/*! \brief Performs cleanup of pending messages
 *  \param  self    The instance
 */
void Segm_Cleanup(CSegmentation *self)
{
    CDlNode *node_ptr = NULL;
                                                                    /* cleanup Tx queue */
    for (node_ptr = Dl_PopHead(&self->processing_list); node_ptr != NULL; node_ptr = Dl_PopHead(&self->processing_list))
    {
        Ucs_AmsRx_Msg_t *rx_ptr = (Ucs_AmsRx_Msg_t*)Dln_GetData(node_ptr);

        Amsp_FreeRxPayload(self->pool_ptr, rx_ptr);
        Amsp_FreeRxObj(self->pool_ptr, rx_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Tx segmentation                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Builds next MOST telegram according to given Application Messages
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the Application Message Tx handle
 *  \param  tel_ptr Reference to the MOST Telegram handle
 *  \return Returns \c True if segmentation was completed for the Application Message. Otherwise \c false.
 */
bool Segm_TxBuildSegment(CSegmentation *self, Ucs_AmsTx_Msg_t* msg_ptr, Msg_MostTel_t *tel_ptr)
{
    bool finished = false;
    MISC_UNUSED(self);

    tel_ptr->destination_addr = msg_ptr->destination_address;
    Msg_SetAltMsgId((CMessage*)(void*)tel_ptr, msg_ptr->msg_id);
    tel_ptr->opts.llrbc       = msg_ptr->llrbc;
    tel_ptr->info_ptr         = msg_ptr;                            /* info_ptr must carry the reference to AMS Tx message object */
    tel_ptr->opts.cancel_id   = Amsg_TxGetFollowerId(msg_ptr);

    if (msg_ptr->data_size <= SEGM_MAX_SIZE_TEL)                      /* is single transfer? */
    {
        Msg_SetExtPayload((CMessage*)(void*)tel_ptr, msg_ptr->data_ptr, (uint8_t)msg_ptr->data_size, NULL);
        finished = true;
    }
    else                                                            /* is segmented transfer? */
    {
        uint16_t next_segm_cnt = Amsg_TxGetNextSegmCnt(msg_ptr);

        if (next_segm_cnt == 0xFFFFU)                               /* first segment: size prefixed segmented message TelId = "4" */
        {
            tel_ptr->tel.tel_id = 4U;
            tel_ptr->tel.tel_data_ptr[0] = MISC_HB(msg_ptr->data_size);
            tel_ptr->tel.tel_data_ptr[1] = MISC_LB(msg_ptr->data_size);
            tel_ptr->tel.tel_len = 2U;
        }
        else                                                        /* further segments: TelId = "1,2,3" */
        {
            uint16_t index = next_segm_cnt * ((uint16_t)SEGM_MAX_SIZE_TEL - 1U);
            uint16_t remaining_sz = msg_ptr->data_size - index;
            uint8_t tel_sz = SEGM_MAX_SIZE_TEL - 1U;

            if (remaining_sz < SEGM_MAX_SIZE_TEL)
            {
                tel_ptr->tel.tel_id = 3U;                           /* is last segment */
                tel_sz = (uint8_t)remaining_sz;
                finished = true;
            }
            else
            {
                if (index == 0U)
                {
                    tel_ptr->tel.tel_id = 1U;                       /* is first segment */
                }
                else
                {
                    tel_ptr->tel.tel_id = 2U;                       /* is subsequent segment */
                }
            }

            tel_ptr->tel.tel_cnt = (uint8_t)next_segm_cnt;
            Msg_SetExtPayload((CMessage*)(void*)tel_ptr, &msg_ptr->data_ptr[index], tel_sz, NULL);
        }

        Amsg_TxIncrementNextSegmCnt(msg_ptr);
    }

    return finished;
}

/*------------------------------------------------------------------------------------------------*/
/* Rx pools                                                                                       */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Retrieves a processing Rx Application message object to a corresponding MOST telegram
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the MOST telegram
 *  \return The reference to the corresponding Rx Application Message or \c NULL if no appropriate 
 *          Rx Application Message is available.
 */
static Ucs_AmsRx_Msg_t* Segm_RxRetrieveProcessingHandle(CSegmentation *self, Msg_MostTel_t *tel_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;
    CDlNode *result_node_ptr = Dl_Foreach(&self->processing_list, &Segm_RxSearchProcessingHandle, tel_ptr);

    if (result_node_ptr != NULL)
    {
        Dl_Ret_t ret = Dl_Remove(&self->processing_list, result_node_ptr);
        TR_ASSERT(self->base_ptr->ucs_user_ptr, "[SEGM]", (ret == DL_OK));
        msg_ptr = (Ucs_AmsRx_Msg_t*)Dln_GetData(result_node_ptr);
        MISC_UNUSED(ret);
    }

    return msg_ptr;
}

/*! \brief  Stores a processing Rx Application message object into a dedicated list
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the Rx Application Message
 */
static void Segm_RxStoreProcessingHandle(CSegmentation *self, Ucs_AmsRx_Msg_t *msg_ptr)
{
    Amsg_RxSetGcMarker(msg_ptr, false);
    Amsg_RxEnqueue(msg_ptr, &self->processing_list);        /* insert at tail, since garbage collector starts at head */ 
}

/*! \brief  Performs garbage collection of outdated message objects
 *  \param  self    The instance
 */
void Segm_RxGcScanProcessingHandles(void *self)
{
    CSegmentation *self_ = (CSegmentation*)self;
                                                                            /* first remove outdated messages */
    CDlNode *node_ptr = Dl_PeekHead(&self_->processing_list);               /* get first candidate from head */

    while (node_ptr != NULL) 
    {
        Ucs_AmsRx_Msg_t *msg_ptr = (Ucs_AmsRx_Msg_t*)Dln_GetData(node_ptr);

        if (Amsg_RxGetGcMarker(msg_ptr) != false)
        {
            Msg_MostTel_t tel;

            Amsg_RxCopySignatureToTel(msg_ptr, &tel);
            self_->error_fptr(self_->error_inst, &tel, SEGM_ERR_5);

            (void)Dl_Remove(&self_->processing_list, node_ptr);

            Amsp_FreeRxPayload(self_->pool_ptr, msg_ptr);
            Amsp_FreeRxObj(self_->pool_ptr, msg_ptr);

            node_ptr = Dl_PeekHead(&self_->processing_list);                /* get next candidate from head */
        }
        else
        {
            break;
        }
    }

    (void)Dl_Foreach(&self_->processing_list, &Segm_RxGcSetLabel, NULL);    /* set label of all remaining messages */
}

/*! \brief  Sets garbage collector flags for all list members
 *  \param  current_data    The Application message object present in list
 *  \param  search_data     unused (\c NULL)
 *  \return Returns always false in order to handle all list members */
static bool Segm_RxGcSetLabel(void *current_data, void *search_data)
{
    Ucs_AmsRx_Msg_t *msg_ptr = (Ucs_AmsRx_Msg_t*)current_data;
    Amsg_RxSetGcMarker(msg_ptr, true);
    MISC_UNUSED(search_data);
    return false;
}

/*! \brief  Search routine to identify message objects with the same signature
 *          than a given MOST telegram
 *  \param  current_data    The Application message object present in list
 *  \param  search_data     The MOST Telegram object
 *  \return Returns \c true if both handles have the same functional signature,
 *          otherwise \c false. */
static bool Segm_RxSearchProcessingHandle(void *current_data, void *search_data)
{
    Ucs_AmsRx_Msg_t *msg_ptr = (Ucs_AmsRx_Msg_t*)current_data;
    Msg_MostTel_t *tel_ptr = (Msg_MostTel_t*)search_data;

    return Amsg_RxHandleIsIdentical(msg_ptr, tel_ptr);
}

/*------------------------------------------------------------------------------------------------*/
/* Rx segmentation                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Processes segmentation for a received MOST telegram
 *  \param  self        The instance
 *  \param  tel_ptr     The received MOST telegram
 *  \param  result_ptr  The result of the segmentation process
 *  \return The completed Rx Application Message or \c NULL if segmentation process is still
 *          ongoing.
 */
Ucs_AmsRx_Msg_t* Segm_RxExecuteSegmentation(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = NULL;
    *result_ptr = SEGM_RES_OK;

    switch (tel_ptr->tel.tel_id) /* parasoft-suppress  MISRA2004-15_3 "ignore unexpected TelIds" */
    {
        case 0U:
            msg_ptr = Segm_RxProcessTelId0(self, tel_ptr, result_ptr);
            break;
        case 1U:
            Segm_RxProcessTelId1(self, tel_ptr, result_ptr);
            break;
        case 2U:
            Segm_RxProcessTelId2(self, tel_ptr);
            break;
        case 3U:
            msg_ptr = Segm_RxProcessTelId3(self, tel_ptr);
            break;
        case 4U:
            Segm_RxProcessTelId4(self, tel_ptr, result_ptr);
            break;
        default:
            break;
    }

    return msg_ptr;    /* return completed message */
}

/*! \brief  Processes segmentation for a received MOST telegram with \c TelId="0"
 *  \param  self        The instance
 *  \param  tel_ptr     The received MOST telegram
 *  \param  result_ptr  Result of segmentation process
 *  \return The completed Rx Application Message or \c NULL if segmentation process
 *          does not finish successfully.
 */
static Ucs_AmsRx_Msg_t* Segm_RxProcessTelId0(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Segm_RxRetrieveProcessingHandle(self, tel_ptr);
    *result_ptr = SEGM_RES_OK;

    if (msg_ptr != NULL)                            /* treat error: segmentation process is ongoing */ 
    {
        self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_7);
        Amsp_FreeRxPayload(self->pool_ptr, msg_ptr);/* free assigned user payload and throw segmentation error */
        Amsp_FreeRxObj(self->pool_ptr, msg_ptr);
        msg_ptr = NULL;
    }
                                                    /* try to allocate handle, memory is NetServices provided (payload <= 45 bytes) */
    msg_ptr = Amsp_AllocRxObj(self->pool_ptr, (uint16_t)tel_ptr->tel.tel_len);

    if (msg_ptr == NULL)
    {
        msg_ptr = Amsp_AllocRxRsvd(self->pool_ptr);
    }

    if (msg_ptr != NULL)                            /* handle available: setup Rx Application Message */
    {
        Amsg_RxCopySignatureFromTel(msg_ptr, tel_ptr);

        if (tel_ptr->tel.tel_len > 0U)
        {                                           /* copy payload to message */
            Amsg_RxCopyToPayload(msg_ptr, tel_ptr->tel.tel_data_ptr, tel_ptr->tel.tel_len);
        }
        else
        {                                           /* set payload length to zero */
            msg_ptr->data_ptr = NULL;
            msg_ptr->data_size = 0U;
        }
    }
    else
    {
        *result_ptr = SEGM_RES_RETRY;               /* retry when next Rx object is released */
    }

    return msg_ptr;
}

/*! \brief  Processes segmentation for a received MOST telegram with \c TelId="1"
 *  \param  self        The instance
 *  \param  tel_ptr     The received MOST telegram
 *  \param  result_ptr  Result of segmentation process
 */
static void Segm_RxProcessTelId1(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr)
{
    *result_ptr = SEGM_RES_OK;

    if (tel_ptr->tel.tel_cnt != 0U)                             /* handle incorrect tel_cnt */
    {
        self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_3);
    }
    else                                                        /* tel_cnt is correct -> continue segmentation */
    {
        Ucs_AmsRx_Msg_t *msg_ptr = Segm_RxRetrieveProcessingHandle(self, tel_ptr);
        bool is_size_prefixed = false;

        if (msg_ptr != NULL)                                    /* has previous message */
        {
            if ((Amsg_RxGetExpTelCnt(msg_ptr) != 0U) || (msg_ptr->data_size > 0U)) 
            {                                                   /* error: previous message already contains segments */
                self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_7);
                Amsp_FreeRxPayload(self->pool_ptr, msg_ptr);
                Amsg_RxHandleSetup(msg_ptr);                    /* initialize message for re-use */
            }
            else                                                /* message and payload had been allocated by TelId '4' */
            {
                is_size_prefixed = true;
            }
        }
        else                                                    /* allocate message object if pre-allocation was not initiated by TelId "4" */
        {
            msg_ptr = Amsp_AllocRxObj(self->pool_ptr, 0U);
        }

        if (msg_ptr != NULL)                                    /* now allocate payload */
        {
            if (is_size_prefixed == false)
            {
                Amsg_RxCopySignatureFromTel(msg_ptr, tel_ptr);  /* save signature and try to allocate */
                (void)Amsp_AllocRxPayload(self->pool_ptr, self->rx_default_payload_sz, msg_ptr);
            }

            if (!Amsg_RxHasExternalPayload(msg_ptr))            /* allocation of payload failed */
            {
                self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_2);
                Amsp_FreeRxObj(self->pool_ptr, msg_ptr);
                msg_ptr = NULL;
            }
            else                                                /* allocation of payload succeeded */
            {
                (void)Amsg_RxAppendPayload(msg_ptr, tel_ptr);
                Segm_RxStoreProcessingHandle(self, msg_ptr);
                msg_ptr = NULL;
            }
        }
        else                                                    /* no message object allocated */
        {                                                       /* send segmentation error */
            self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_4);
        }
    }
}

/*! \brief  Processes segmentation for a received MOST telegram with \c TelId="2"
 *  \param  self    The instance
 *  \param  tel_ptr The received MOST telegram
 */
static void Segm_RxProcessTelId2(CSegmentation *self, Msg_MostTel_t *tel_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Segm_RxProcessTelId3(self, tel_ptr); /* pretend having TelId '2' but store the */
                                                                    /* assembled message again */
    if (msg_ptr != NULL)
    {
        Segm_RxStoreProcessingHandle(self, msg_ptr);
    }
}

/*! \brief  Processes segmentation for a received MOST telegram with \c TelId="3"
 *  \param  self    The instance
 *  \param  tel_ptr The received MOST telegram
 *  \return The assembled Rx Application Message or \c NULL if segmentation process
 *          did not process successfully.
 */
static Ucs_AmsRx_Msg_t* Segm_RxProcessTelId3(CSegmentation *self, Msg_MostTel_t *tel_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Segm_RxRetrieveProcessingHandle(self, tel_ptr);

    if (msg_ptr == NULL)                                    /* is first segment missing */
    {
        self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_1);
    }
    else
    {
        uint8_t exp_tel_cnt = Amsg_RxGetExpTelCnt(msg_ptr);

        if ((exp_tel_cnt == 0U) && (msg_ptr->data_size == 0U)) 
        {                                                   /* error: did not receive first segment */
            self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_1);
            Segm_RxStoreProcessingHandle(self, msg_ptr);
            msg_ptr = NULL;
        }
        else if (exp_tel_cnt != tel_ptr->tel.tel_cnt) 
        {                                                   /* has wrong TelCnt */
            self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_3);
            Segm_RxStoreProcessingHandle(self, msg_ptr);
            msg_ptr = NULL;
        }

        if (msg_ptr != NULL)
        {
            bool succ = Amsg_RxAppendPayload(msg_ptr, tel_ptr);

            if (succ == false)
            {
                self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_2);
                Amsp_FreeRxPayload(self->pool_ptr, msg_ptr);
                Amsp_FreeRxObj(self->pool_ptr, msg_ptr);
                msg_ptr = NULL;
            }
        }
    }

    return msg_ptr;
}

/*! \brief  Processes segmentation for a received MOST telegram with \c TelId="4"
 *  \param  self        The instance
 *  \param  tel_ptr     The received MOST telegram
 *  \param  result_ptr  Result of segmentation process
 */
static void Segm_RxProcessTelId4(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr)
{
    *result_ptr = SEGM_RES_OK;

    if (tel_ptr->tel.tel_len >= 2U)                         /* telegrams has necessary length */
    {
        uint16_t msg_size;
        MISC_DECODE_WORD(&msg_size, tel_ptr->tel.tel_data_ptr);

        if (msg_size > SEGM_MAX_SIZE_TEL)                     /* application message has correct size */
        {
            Ucs_AmsRx_Msg_t *msg_ptr = Segm_RxRetrieveProcessingHandle(self, tel_ptr);

            if (msg_ptr != NULL)                            /* treat error: segmentation process is ongoing */
            {
                Amsp_FreeRxPayload(self->pool_ptr, msg_ptr);
                self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_7);
                Amsg_RxHandleSetup(msg_ptr);                /* initialize message for re-use */
            }
            else
            {                                               /* try to allocate handle, memory is NetServices provided (payload <= 45 bytes) */
                msg_ptr = Amsp_AllocRxObj(self->pool_ptr, 0U);
            }

            if (msg_ptr != NULL)                            /* allocation succeeded: decode length and allocate payload */
            {
                Amsg_RxCopySignatureFromTel(msg_ptr, tel_ptr);
                (void)Amsp_AllocRxPayload(self->pool_ptr, msg_size, msg_ptr);
                Segm_RxStoreProcessingHandle(self, msg_ptr);/* store handle and don't care if payload was allocated or not */
                msg_ptr = NULL;                             /* segmentation error 2 is treated by TelId 1 */
            }
            else
            {
                self->error_fptr(self->error_inst, tel_ptr, SEGM_ERR_4);
            }
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

