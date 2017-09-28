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
 * \brief Internal header file of LLD Message Pool
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_PMF
 * @{
 */

#ifndef UCS_LLDPOOL_H
#define UCS_LLDPOOL_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_base.h"
#include "ucs_lld_pb.h"
#include "ucs_message.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Number of LLD Tx handles dedicated to each FIFO */
#define LLDP_NUM_HANDLES              5U

/*------------------------------------------------------------------------------------------------*/
/* Internal types                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Internal LLD Tx message */
typedef struct Lld_IntTxMsg_
{
    Ucs_Lld_TxMsg_t lld_msg;    /*!< \brief     Contains the public LLD Tx message
                                 *   \details   This attribute needs to be the first one in this structure
                                 */
    CDlNode     node;           /*!< \brief     Node required for queuing */
    CMessage   *msg_ptr;        /*!< \brief     Reference to the associated common message object, or
                                 *              \c NULL if the object is a command */
    void       *owner_ptr;      /*!< \brief     Points to the FIFO which owns the message object 
                                 *              or NULL if the object is a command */

} Lld_IntTxMsg_t;

/*! \brief  Internal LLD Rx message */
typedef struct Lld_IntRxMsg_
{
    Ucs_Lld_RxMsg_t  lld_msg;   /*!< \brief     Contains the public LLD Rx message 
                                 *   \details   This attribute needs to be the first one in this structure
                                 */
    CMessage        *msg_ptr;   /*!< \brief     Reference to the associated common message object*/

} Lld_IntRxMsg_t;

/*! \brief The class CLldPool*/
typedef struct CLldPool_
{ 
    CDlList list;                             /*!< \brief Points to the first available message in Tx pool */
    Lld_IntTxMsg_t messages[LLDP_NUM_HANDLES];/*!< \brief Available messages in Tx pool */

} CLldPool;

/*------------------------------------------------------------------------------------------------*/
/* Function prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
extern void Lldp_Ctor(CLldPool *self, void *owner_ptr, void *ucs_user_ptr);
extern void Lldp_ReturnTxToPool(CLldPool *self, Lld_IntTxMsg_t *msg_ptr);
extern Lld_IntTxMsg_t* Lldp_GetTxFromPool(CLldPool *self);


#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif                                              /* UCS_LLDPOOL_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

