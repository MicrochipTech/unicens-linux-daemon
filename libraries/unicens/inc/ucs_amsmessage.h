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
 * \brief Internal header file of Application Message Classes
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMSMSG
 * @{
 */

#ifndef UCS_AMSMESSAGE_H
#define UCS_AMSMESSAGE_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_ams_pb.h"
#include "ucs_message.h"
#include "ucs_dl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define AMSG_TX_OBJECT_SZ   (sizeof(Amsg_IntMsgTx_t))
#define AMSG_RX_OBJECT_SZ   (sizeof(Amsg_IntMsgRx_t))

/*------------------------------------------------------------------------------------------------*/
/* Internal types                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Internal transmission result of an application message */
typedef enum Amsg_TxIntStatus_
{
    AMSG_TX_INTRES_NONE     = 0x00U,  /*!< \brief   The internal transmission is not applicable. */
    AMSG_TX_INTRES_SUCCESS  = 0x01U,  /*!< \brief   The internal transmission succeeded. */
    AMSG_TX_INTRES_ERRBUF   = 0x02U   /*!< \brief   The internal transmission failed. */

} Amsg_TxIntStatus_t;

/*! \brief  Assignable function which is invoked as soon as an application message is received 
 *          completely and available in the Rx message queue
 *  \param  self            The instance (optional)
 *  \param  msg_ptr         Reference to the received message
 */
typedef void (*Amsg_RxCompleteCb_t)(void* self, Ucs_AmsRx_Msg_t* msg_ptr);

/*! \brief  Callback function type which is fired as soon as an AMS transmission was finished 
 *  \param  msg_ptr         Reference to the related message object
 *  \param  result          Transmission result
 *  \param  info            Detailed INIC transmission result
 *  \param  self            The instance (optional)
 */
typedef void (*Amsg_TxCompleteCb_t)(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void* self);

/*! \brief  Single instance API callback function type which is fired as soon as an AMS transmission was finished 
 *  \param  msg_ptr         Reference to the related message object
 *  \param  result          Transmission result
 *  \param  info            Detailed INIC transmission result
 */
typedef void (*Amsg_TxCompleteSiaCb_t)(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info);

/*! \brief  Callback function which is invoked to free a Tx message object to the owning pool 
 *  \param  owner_ptr       The owning pool of the message object
 *  \param  msg_ptr         Reference to the related message object
 */
typedef void (*Amsg_TxFreedCb_t)(void *owner_ptr, Ucs_AmsTx_Msg_t* msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Internal Tx message structure */
typedef struct Amsg_IntMsgTx_
{
    Ucs_AmsTx_Msg_t     pb_msg;                 /*!< \brief Public message struct must be the first member */
    void               *info_ptr;               /*!< \brief Custom object information required by memory management */

    void               *free_inst_ptr;          /*!< \brief Reference which is passed to free_ptr */
    Amsg_TxFreedCb_t    free_fptr;              /*!< \brief Callback function which is called to free the object */

    uint8_t            *memory_ptr;             /*!< \brief Reference to payload provided by memory management */
    void               *memory_info_ptr;        /*!< \brief Custom payload information required by memory management */
    uint16_t            memory_sz;              /*!< \brief Size of the payload that is provided by memory management */

    uint16_t            next_segment_cnt;       /*!< \brief Specifies the next segment count. '0xFF' means size prefixed */
    uint8_t             follower_id;            /*!< \brief Identifier of segmented messages and corresponding telegrams  */
    Ucs_MsgTxStatus_t   temp_result;            /*!< \brief Stores the temporary result that is notified when then transmission
                                                 *          has completed 
                                                 */
    uint16_t            backup_dest_address;    /*!< \brief Backup of replaced target address. */
    bool                ignore_wrong_target;    /*!< \brief Forces the message to report transmission result "success", although 
                                                 *          the INIC has reported transmission error "wrong target"
                                                 */
    CDlNode             node;                   /*!< \brief Node required for message pool */

    Amsg_TxCompleteSiaCb_t complete_sia_fptr;   /*!< \brief Single instance API Callback function which is invoked
                                                 *          after transmission completed
                                                 */
    Amsg_TxCompleteCb_t    complete_fptr;       /*!< \brief Callback function which is invoked after transmission
                                                 *          completed 
                                                 */
    void                  *complete_inst_ptr;   /*!< \brief Instance pointer which is required to invoke complete_fptr */

} Amsg_IntMsgTx_t;

/*! \brief Internal Rx message structure */
typedef struct Amsg_IntMsgRx_
{
    Ucs_AmsRx_Msg_t     pb_msg;                 /*!< \brief Public message structure must be the first member */
    void               *info_ptr;               /*!< \brief Custom object information required by memory management */

    uint8_t            *memory_ptr;             /*!< \brief Reference to payload provided by memory management */
    void               *memory_info_ptr;        /*!< \brief Custom payload information required by memory management */
    uint16_t            memory_sz;              /*!< \brief The size of the allocated user payload in bytes */

    CDlNode             node;                   /*!< \brief Node required for message pool */

    uint8_t             exp_tel_cnt;            /*!< \brief The expected TelCnt used for segmented transfer */
    bool                gc_marker;              /*!< \brief Identifies message objects that were already
                                                 *          marked by the garbage collector. 
                                                 */
} Amsg_IntMsgRx_t;

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/* Tx */
extern void Amsg_TxCtor(Ucs_AmsTx_Msg_t *self, void *info_ptr, Amsg_TxFreedCb_t free_fptr, void *free_inst_ptr);
extern void Amsg_TxSetInternalPayload(Ucs_AmsTx_Msg_t *self, uint8_t *mem_ptr, uint16_t mem_size, void *mem_info_ptr);
extern void Amsg_TxReuse(Ucs_AmsTx_Msg_t *self);
extern void Amsg_TxSetCompleteCallback(Ucs_AmsTx_Msg_t *self, Amsg_TxCompleteSiaCb_t compl_sia_fptr, 
                                Amsg_TxCompleteCb_t compl_fptr, void* compl_inst_ptr);
extern void Amsg_TxNotifyComplete(Ucs_AmsTx_Msg_t *self, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info);
extern void Amsg_TxFreeUnused(Ucs_AmsTx_Msg_t *self);
extern void Amsg_TxUpdateResult(Ucs_AmsTx_Msg_t *self, Ucs_MsgTxStatus_t result);
extern Ucs_AmsTx_Result_t Amsg_TxGetResultCode(Ucs_AmsTx_Msg_t *self);
extern Ucs_AmsTx_Info_t Amsg_TxGetResultInfo(Ucs_AmsTx_Msg_t *self);
extern uint16_t Amsg_TxGetNextSegmCnt(Ucs_AmsTx_Msg_t *self);
extern void Amsg_TxIncrementNextSegmCnt(Ucs_AmsTx_Msg_t *self);
extern uint8_t Amsg_TxGetFollowerId(Ucs_AmsTx_Msg_t *self);
extern void Amsg_TxSetFollowerId(Ucs_AmsTx_Msg_t *self, uint8_t id);
extern void Amsg_TxReplaceDestinationAddr(Ucs_AmsTx_Msg_t *self, uint16_t new_destination);
extern void Amsg_TxRemoveFromQueue(Ucs_AmsTx_Msg_t *self, CDlList *list_ptr);
extern void Amsg_TxEnqueue(Ucs_AmsTx_Msg_t* self, CDlList* list_ptr);
extern Ucs_AmsTx_Msg_t* Amsg_TxPeek(CDlList* list_ptr);
extern Ucs_AmsTx_Msg_t* Amsg_TxDequeue(CDlList* list_ptr);

/* Rx */
extern void Amsg_RxCtor(Ucs_AmsRx_Msg_t *self, void *info_ptr);
extern void Amsg_RxBuildFromTx(Ucs_AmsRx_Msg_t *self, Ucs_AmsTx_Msg_t *tx_ptr, uint16_t source_address);
extern void Amsg_RxHandleSetup(Ucs_AmsRx_Msg_t *self);
extern void Amsg_RxHandleSetMemory(Ucs_AmsRx_Msg_t *self, uint8_t *mem_ptr, uint16_t mem_size, void *info_ptr);
extern bool Amsg_RxHandleIsIdentical(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t *tel_ptr);
extern void Amsg_RxCopySignatureFromTel(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* src_ptr);
extern void Amsg_RxCopySignatureToTel(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* target_ptr);
extern void Amsg_RxCopyToPayload(Ucs_AmsRx_Msg_t *self, uint8_t data[], uint8_t data_sz);
extern bool Amsg_RxAppendPayload(Ucs_AmsRx_Msg_t *self, Msg_MostTel_t* src_ptr);
extern bool Amsg_RxHasExternalPayload(Ucs_AmsRx_Msg_t *self);
extern void Amsg_RxEnqueue(Ucs_AmsRx_Msg_t* self, CDlList* list_ptr);
extern void Amsg_RxSetGcMarker(Ucs_AmsRx_Msg_t* self, bool value);
extern bool Amsg_RxGetGcMarker(Ucs_AmsRx_Msg_t* self);
extern uint8_t Amsg_RxGetExpTelCnt(Ucs_AmsRx_Msg_t* self);
/* Rx helpers */
extern Ucs_AmsRx_Msg_t* Amsg_RxPeek(CDlList* list_ptr);
extern Ucs_AmsRx_Msg_t* Amsg_RxDequeue(CDlList* list_ptr);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_AMSMESSAGE_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

