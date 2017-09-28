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
 * \brief Internal header file of Application Message Pools
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMSPOOL
 * @{
 */

#ifndef UCS_AMSPOOL_H
#define UCS_AMSPOOL_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_ams_pb.h"
#include "ucs_obs.h"
#include "ucs_amsallocator.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Classes                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Class members of AMS Pool */
typedef struct CAmsMsgPool_
{
    Ams_MemAllocator_t *allocator_ptr;              /*!< \brief  Interface to memory allocator */
    Ucs_AmsRx_Msg_t    *rx_rsvd_msg_ptr;            /*!< \brief  Pre-allocated Rx message or NULL if no 
                                                     *           reserved message is available */
    Ucs_AmsRx_Msg_t    *rx_rsvd_msg_ref;            /*!< \brief  Stores the reference of the reserved message
                                                     *           to identify it and restore the 
                                                     *           \c rx_rsvd_msg_ptr. */
    CSubject            tx_freed_subject;           /*!< \brief  Allows to observe freed Tx message event */
    CSubject            rx_freed_subject;           /*!< \brief  Allows to observe freed Rx message event */
    bool                tx_notify_freed;            /*!< \brief  Is \c true when to notify the next Tx freed object */
    bool                rx_notify_freed;            /*!< \brief  Is \c true when to notify the next Rx freed object */
    bool                terminated;                 /*!< \brief  Is \c true if a cleanup was done. Helps to release the 
                                                     *           pre-allocated message after the first cleanup attempt. */
    void               *ucs_user_ptr;               /*!< \brief User reference that needs to be passed in every callback function */

} CAmsMsgPool;

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
extern void Amsp_Ctor(CAmsMsgPool *self, Ams_MemAllocator_t *mem_allocator_ptr, void *ucs_user_ptr);
extern void Amsp_Cleanup(CAmsMsgPool *self);
/* Tx */
extern void Amsp_AssignTxFreedObs(CAmsMsgPool *self, CObserver *observer_ptr);
extern Ucs_AmsTx_Msg_t* Amsp_AllocTxObj(CAmsMsgPool *self, uint16_t payload_sz);
/* Rx */
extern void Amsp_AssignRxFreedObs(CAmsMsgPool *self, CObserver *observer_ptr);
extern Ucs_AmsRx_Msg_t* Amsp_AllocRxObj(CAmsMsgPool *self, uint16_t payload_sz);
extern Ucs_AmsRx_Msg_t* Amsp_AllocRxRsvd(CAmsMsgPool *self);
extern bool Amsp_AllocRxPayload(CAmsMsgPool *self, uint16_t payload_sz, Ucs_AmsRx_Msg_t* msg_ptr);
extern void Amsp_FreeRxObj(CAmsMsgPool *self, Ucs_AmsRx_Msg_t* msg_ptr);
extern void Amsp_FreeRxPayload(CAmsMsgPool *self, Ucs_AmsRx_Msg_t* msg_ptr);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_AMSPOOL_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

