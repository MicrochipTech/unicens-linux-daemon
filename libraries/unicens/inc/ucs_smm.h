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
 * \brief Header file of the class CStaticMemoryManager.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_SMM_CLASS
 * @{
 */

#ifndef UCS_SMM_H
#define UCS_SMM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_smm_pb.h"
#include "ucs_ret_pb.h"
#include "ucs_dl.h"
#include "ucs_amsmessage.h"
#include "ucs_amsallocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
#define SMM_NUM_TX_MSGS     ((uint16_t)UCS_AMS_NUM_TX_MSGS)
#define SMM_NUM_RX_MSGS     ((uint16_t)UCS_AMS_NUM_RX_MSGS)
#define SMM_SIZE_TX_MSG     ((uint16_t)UCS_AMS_SIZE_TX_MSG)
#define SMM_SIZE_RX_MSG     ((uint16_t)UCS_AMS_SIZE_RX_MSG)

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Tx object allocation info */
typedef struct Smm_TxObject_
{
    Amsg_IntMsgTx_t object;             /*!< \brief Tx object memory */
    CDlNode node;                       /*!< \brief Node that enables listing */

} Smm_TxObject_t;

/*! \brief Rx object allocation info */
typedef struct Smm_RxObject_
{
    Amsg_IntMsgRx_t object;             /*!< \brief Rx object memory */
    CDlNode node;                       /*!< \brief Node that enables listing */

} Smm_RxObject_t;

/*! \brief Tx payload allocation info */
typedef struct Smm_TxPayload_
{
    uint8_t data[SMM_SIZE_TX_MSG];      /*!< \brief Tx payload memory */
    CDlNode node;                       /*!< \brief Node that enables listing */

} Smm_TxPayload_t;

/*! \brief Rx payload allocation info */
typedef struct Smm_RxPayload_
{
    uint8_t data[SMM_SIZE_RX_MSG];      /*!< \brief Rx payload memory */
    CDlNode node;                       /*!< \brief Node that enables listing */

} Smm_RxPayload_t;

/*! \brief Static memory allocation of objects and payload */
typedef struct Smm_Resources_
{
    Smm_TxObject_t tx_objects[SMM_NUM_TX_MSGS];  /*!< \brief Statically allocated Tx objects */
    Smm_RxObject_t rx_objects[SMM_NUM_RX_MSGS];  /*!< \brief Statically allocated Rx objects */

    Smm_TxPayload_t tx_payload[SMM_NUM_TX_MSGS]; /*!< \brief Statically allocated Tx payload */
    Smm_RxPayload_t rx_payload[SMM_NUM_RX_MSGS]; /*!< \brief Statically allocated Rx payload */

} Smm_Resources_t;

/*! \brief Descriptor which simplifies access to different memory types */
typedef struct Smm_Descriptor_
{
    CDlList list;           /*!< \brief The list of available memory chunks */
    uint16_t max_mem_size;  /*!< \brief The maximum size of one memory chunk */

} Smm_Descriptor_t;


/*------------------------------------------------------------------------------------------------*/
/* Class definitions                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Structure of the class CStaticMemoryManager. */
typedef struct CStaticMemoryManager_
{
    Smm_Resources_t resources;          /*!< \brief Static allocation of memory */

    Smm_Descriptor_t null_descr;        /*!< \brief Descriptor for unknown memory type. Performance measure. */
    Smm_Descriptor_t tx_object_descr;   /*!< \brief Descriptor of Tx object type */
    Smm_Descriptor_t tx_payload_descr;  /*!< \brief Descriptor of Tx payload type */
    Smm_Descriptor_t rx_object_descr;   /*!< \brief Descriptor of Rx object type */
    Smm_Descriptor_t rx_payload_descr;  /*!< \brief Descriptor of Rx payload type */

    void *ucs_user_ptr;                /*!< \brief User reference that needs to be passed in every callback function */

} CStaticMemoryManager;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CStaticMemoryManager                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void Smm_Ctor(CStaticMemoryManager *self, void *ucs_user_ptr);
extern Ucs_Return_t Smm_LoadPlugin(CStaticMemoryManager *self, Ams_MemAllocator_t *allocator_ptr, uint16_t rx_def_payload_size);
extern Ucs_Return_t Smm_GetFreeBufferCnt(CStaticMemoryManager *self, uint16_t *rx_cnt_ptr, uint16_t *tx_cnt_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_SMM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

