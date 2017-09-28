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
 * \brief Declaration of class message
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_MESSAGE
 * @{
 */

#ifndef UCS_MESSAGE_H
#define UCS_MESSAGE_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_memory.h"
#include "ucs_dl.h"
#include "ucs_message_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Common macros                                                                                  */
/*------------------------------------------------------------------------------------------------*/
#define MSG_ADDR_INVALID    0U  /*!< \brief   The (source) address the INIC uses to declare an invalid source address.
                                 *   \details Invalid source addresses can be:
                                 *            - invalid messages from MOST: source_address = [0x0000..0x000F] 
                                 *            - invalid messages from EHC: source_address != [0x0002, 0x0003]
                                 *            .
                                 */
#define MSG_ADDR_INIC       1U  /*!< \brief The address of the local INIC */
#define MSG_ADDR_EHC_CFG    2U  /*!< \brief The address of the EHC configuration interface (ICM and RCM FIFO) */
#define MSG_ADDR_EHC_APP    3U  /*!< \brief The address of the EHC application interface (MCM FIFO) */

#define MSG_LLRBC_DEFAULT   10U /*!< \brief The default LowLevelRetry BlockCount */
#define MSG_LLRBC_MAX       100U/*!< \brief The maximum LowLevelRetry BlockCount */

#define MSG_DEF_FBLOCK_ID   0xCCU           /*! \brief Predefined FBlockID required to surround "new 16bit message id". */
#define MSG_DEF_FUNC_ID_LSN 0xCU            /*! \brief Predefined function id (least sign. nibble) required to surround "new 16bit message id". */
#define MSG_DEF_OP_TYPE     (UCS_OP_STATUS) /*! \brief Predefined OpType required to surround "new 16bit message id". */

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/* necessary forward declaration */
struct CMessage_;
/*! \brief Common message class which provides MOST style message addressing */
typedef struct CMessage_ CMessage;

/*! \brief  Assignable function which is invoked as soon as transmission
 *          of the message object is finished. 
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the message object
 *  \param  status  Transmission status
 */
typedef void (*Msg_TxStatusCb_t)(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status);

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Size in bytes of reserved message header */
#define MSG_SIZE_RSVD_HEADER  24U
/*! \brief      Size in bytes of message payload */
#define MSG_MAX_SIZE_PAYLOAD  45U
/*! \brief      Size in bytes of pre-allocated message buffer
 *  \details    Size = 24(header) + 45(payload) + 3(stuffing) = 72 */
#define MSG_SIZE_RSVD_BUFFER  72U

/*------------------------------------------------------------------------------------------------*/
/* Class CMessage                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Class CMessage 
 *  \details Common internal message class which embeds the public message attributes
 */
struct CMessage_
{
    Msg_MostTel_t       pb_msg;                 /*!< \brief  Public part which defines the MOST telegram 
                                                 *           structure. This attribute must be the first 
                                                 *           element inside the message structure.
                                                 */
    uint8_t rsvd_buffer[MSG_SIZE_RSVD_BUFFER];  /*!< \brief  Reserved memory space */
    Mem_IntBuffer_t     rsvd_memory;            /*!< \brief  Reserved memory which is needed at least for the
                                                 *           Port message header (24 bytes) */
    Mem_IntBuffer_t     ext_memory;             /*!< \brief  Possible user memory */

    uint8_t            *start_ptr;              /*!< \brief  Points to the start of the message buffer */
    uint8_t             header_curr_idx;        /*!< \brief  Index of the end of the current header */
    uint8_t             header_curr_sz;         /*!< \brief  Current size of header in bytes */
    uint8_t             header_rsvd_sz;         /*!< \brief  Reserved size of header in bytes */

    void               *pool_ptr;               /*!< \brief  Point to the pool the message is allocated from and released to */
    void               *lld_handle_ptr;         /*!< \brief  Possible reference to another message object */
    CDlNode             node;                   /*!< \brief  Node for usage in a doubly linked list */

    Msg_TxStatusCb_t    tx_status_fptr;         /*!< \brief  Pointer to Tx status callback */
    void               *tx_status_inst;         /*!< \brief  Reference to instance which needs Tx status notification */

    bool                tx_active;              /*!< \brief  Is \c true if the object is occupied by the LLD, otherwise \c false */
    bool                tx_bypass;              /*!< \brief  Is \c true if a message was queued as bypass message */

};


/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void             Msg_Ctor(CMessage *self);
extern void             Msg_Cleanup(CMessage *self);

extern void             Msg_ReserveHeader(CMessage *self, uint8_t header_sz);
extern void             Msg_PullHeader(CMessage *self, uint8_t header_sz);
extern void             Msg_PushHeader(CMessage *self, uint8_t header_sz);

extern void             Msg_NotifyTxStatus(CMessage *self, Ucs_MsgTxStatus_t status);

/*------------------------------------------------------------------------------------------------*/
/* Properties                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern Msg_MostTel_t*   Msg_GetMostTel(CMessage *self);

extern uint8_t*         Msg_GetHeader(CMessage *self);
extern uint8_t          Msg_GetHeaderSize(CMessage *self);
extern Ucs_Mem_Buffer_t* Msg_GetMemTx(CMessage *self);

extern void             Msg_SetLldHandle(CMessage *self, void *handle);
extern void            *Msg_GetLldHandle(CMessage *self);
extern void             Msg_SetPoolReference(CMessage *self, void *pool_ptr);
extern void            *Msg_GetPoolReference(CMessage *self);

extern CDlNode         *Msg_GetNode(CMessage *self);

extern void             Msg_SetTxStatusHandler(CMessage *self, Msg_TxStatusCb_t callback_fptr, void *inst_ptr);
extern void             Msg_SetExtPayload(CMessage *self, uint8_t *payload_ptr, uint8_t payload_sz, void* mem_info_ptr);
extern void             Msg_SetTxActive(CMessage *self, bool active);
extern bool             Msg_IsTxActive(CMessage *self);
extern void             Msg_SetTxBypass(CMessage *self, bool bypass);
extern bool             Msg_IsTxBypass(CMessage *self);

extern bool             Msg_VerifyContent(CMessage *self);

extern uint16_t         Msg_GetAltMsgId(CMessage *self);
extern void             Msg_SetAltMsgId(CMessage *self, uint16_t alt_id);

#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_MESSAGE_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

