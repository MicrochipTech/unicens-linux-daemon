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
 * \brief Declaration of public message types
 */

#ifndef UCS_MESSAGE_PB_H
#define UCS_MESSAGE_PB_H

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * \addtogroup G_UCS_TRACE_TYPES
 * @{
 */
/*------------------------------------------------------------------------------------------------*/
/* Defines                                                                                        */
/*------------------------------------------------------------------------------------------------*/
#define UCS_ADDR_INTERNAL                0x0000U  /* < \brief   Internal transmission destination address
                                                   *   \details Can be used for internal message transmission
                                                   *            to avoid possible race conditions during 
                                                   *            recalculation of the own node address.
                                                   */
#define UCS_ADDR_LOCAL_INIC              0x0001U  /* < \brief Destination address of the local INIC */
#define UCS_ADDR_BROADCAST_BLOCKING      0x03C8U  /*!< \brief Blocking broadcast destination address */
#define UCS_ADDR_BROADCAST_UNBLOCKING    0x03FFU  /*!< \brief Unblocking broadcast destination address */
#define UCS_ADDR_DEBUG                   0x0FF0U  /* < \brief Optional debug destination address */

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Message transmission status for internal/debug use
 */
typedef enum Ucs_MsgTxStatus_
{
    UCS_MSG_STAT_OK                     = 0x00U, /*!< \brief Transmission succeeded */
    UCS_MSG_STAT_ERROR_CFG_NO_RCVR      = 0x01U, /*!< \brief No internal receiver exists */
    UCS_MSG_STAT_ERROR_BF               = 0x08U, /*!< \brief Buffer full */ 
    UCS_MSG_STAT_ERROR_CRC              = 0x09U, /*!< \brief CRC */ 
    UCS_MSG_STAT_ERROR_ID               = 0x0AU, /*!< \brief Corrupted identifiers */ 
    UCS_MSG_STAT_ERROR_ACK              = 0x0BU, /*!< \brief Corrupted PACK or CACK */ 
    UCS_MSG_STAT_ERROR_TIMEOUT          = 0x0CU, /*!< \brief TX timeout */
    UCS_MSG_STAT_ERROR_FATAL_WT         = 0x10U, /*!< \brief Wrong target */ 
    UCS_MSG_STAT_ERROR_FATAL_OA         = 0x11U, /*!< \brief Own node address */
    UCS_MSG_STAT_ERROR_NA_TRANS         = 0x18U, /*!< \brief Control channel was switched off and 
                                                  *          a pending transmission was canceled */ 
    UCS_MSG_STAT_ERROR_NA_OFF           = 0x19U, /*!< \brief Control channel not available */
    UCS_MSG_STAT_ERROR_UNKNOWN          = 0xFEU, /*!< \brief Unknown error status */
    UCS_MSG_STAT_ERROR_SYNC             = 0xFFU  /*!< \brief Internal error which is notified if 
                                                  *          communication link with INIC is lost
                                                  */
} Ucs_MsgTxStatus_t;

/*! \brief   Operation Types 
 */
typedef enum Ucs_OpType_
{
    UCS_OP_SET              = 0x0,  /*!< \brief Operation Set (Property) */
    UCS_OP_GET              = 0x1,  /*!< \brief Operation Get (Property) */
    UCS_OP_SETGET           = 0x2,  /*!< \brief Operation SetGet (Property) */
    UCS_OP_INC              = 0x3,  /*!< \brief Operation Increment (Property) */
    UCS_OP_DEC              = 0x4,  /*!< \brief Operation Decrement (Property) */
    UCS_OP_STATUS           = 0xC,  /*!< \brief Operation Status (Property) */

    UCS_OP_START            = 0x0,  /*!< \brief Operation Start (Method) */
    UCS_OP_ABORT            = 0x1,  /*!< \brief Operation Abort (Method) */
    UCS_OP_STARTRESULT      = 0x2,  /*!< \brief Operation StartResult (Method) */
    UCS_OP_PROCESSING       = 0xB,  /*!< \brief Operation Processing (Method) */
    UCS_OP_RESULT           = 0xC,  /*!< \brief Operation Result (Method) */

    UCS_OP_STARTACK         = 0x8,  /*!< \brief Operation StartAck (Method) */
    UCS_OP_ABORTACK         = 0x7,  /*!< \brief Operation AbortAck (Method) */
    UCS_OP_STARTRESULTACK   = 0x6,  /*!< \brief Operation StartResultAck (Method) */
    UCS_OP_PROCESSINGACK    = 0xA,  /*!< \brief Operation ProcessingAck (Method) */
    UCS_OP_RESULTACK        = 0xD,  /*!< \brief Operation ResultAck (Method) */

    UCS_OP_GETINTERFACE     = 0x5,  /*!< \brief Operation GetInterface (Property/Method) */
    UCS_OP_INTERFACE        = 0xE,  /*!< \brief Operation Interface (Property/Method) */
    UCS_OP_ERROR            = 0xF,  /*!< \brief Operation Error (Property/Method) */
    UCS_OP_ERRORACK         = 0x9   /*!< \brief Operation ErrorAck (Property/Method) */

} Ucs_OpType_t;

/*! \brief  MOST message id "FBlockID.InstID.FktID.OPType" */
typedef struct Msg_MsgId_
{
    uint8_t         fblock_id;      /*!< \brief FBlockID */
    uint8_t         instance_id;    /*!< \brief InstID */
    uint16_t        function_id;    /*!< \brief FktID */
    Ucs_OpType_t    op_type;        /*!< \brief Operation type */

} Msg_MsgId_t;

/*! \brief  Retry options */
typedef struct Msg_TxOptions_
{
    uint8_t     llrbc;          /*!< \brief   Low-level retry block count performed by the INIC. 
                                 *   \details The LLRBC are applicable for MCMs. ICMs don't care.
                                 *            Values exceeding the maximum value are be corrected 
                                 *            by the INIC silently to the maximum value.
                                 *            Valid range: 0..100
                                 */
    uint8_t     cancel_id;      /*!< \brief   Either "0" or label for a group of dependent telegrams. 
                                 *   \details The value determines the required action if the transmission
                                 *            has failed.
                                 *            Valid range:
                                 *            - 0: Only the failed telegram will is removed from the FIFO.
                                 *            - 1..255: All telegrams with the same cancel_id as a failed telegram 
                                 *              will be removed from the FIFO queue.
                                 */

} Msg_TxOptions_t;

/*! \brief  Most telegram data */
typedef struct Msg_TelData_
{
    uint8_t     tel_id;         /*!< \brief Telegram id which indicates the telegram as part of
                                 *          segmented message or as single transfer. */
    uint8_t     tel_len;        /*!< \brief The telegram length. 
                                 *          I.e. the number of telegram bytes starting at address
                                 *          which is referred in \c tel_data_ptr. The INIC will add
                                 *          \em one in case of \"tel_id = 1..3\". 
                                 */
    uint8_t     tel_cnt;        /*!< \brief The message count indexing the telegram within a segmented
                                 *          message.
                                 *          The respective tel_cnt is moved by the INIC to \"DATA[0]\"
                                 *          in case of \"tel_id = 1..3\". Otherwise it is ignored.
                                 */
    uint8_t    *tel_data_ptr;   /*!< \brief Points to telegram data. */

} Msg_TelData_t;

/*! \brief  Common MOST message */
typedef struct Msg_MostTel_
{
    uint16_t destination_addr;      /*!< \brief MOST destination address */
    uint16_t source_addr;           /*!< \brief MOST source address */

    Msg_MsgId_t         id;         /*!< \brief MOST message id "FBlockID.InstID.FktID.OPType" */
    Msg_TxOptions_t     opts;       /*!< \brief Message transmission options */
    Msg_TelData_t       tel;        /*!< \brief MOST telegram data */
    void               *info_ptr;   /*!< \brief Possible reference to additional data */

} Msg_MostTel_t;

/*! @} */

#ifdef __cplusplus
}                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_MESSAGE_PB_H */


/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

