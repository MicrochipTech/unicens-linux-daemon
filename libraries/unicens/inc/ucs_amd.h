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
 * \brief Internal header file of Application Message Distributor
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMD
 * @{
 */
#ifndef UCS_AMD_H
#define UCS_AMD_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_ams.h"
#include "ucs_base.h"
#include "ucs_amsmessage.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Assignable function which is invoked as soon as an application message is received 
 *          completely and available in the Rx message queue
 *  \param  self    The instance
 */
typedef void (*Amd_RxMsgCompleteCb_t)(void *self);

/*! \brief  Assignable callback function which is able to read and modify the Rx message
 *  \param  self    The instance
 *  \param  msg_ptr Reference to the Rx message object
 */
typedef void (*Amd_RxModificationCb_t)(void *self, Ucs_AmsRx_Msg_t *msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Application Message Service Class
 *  \details    Allows transmission and reception of MOST Application Messages
 */
typedef struct CAmd_
{
    CBase                  *base_ptr;                   /*!< \brief  Reference to basic services */
    CAms                   *ams_ptr;                    /*!< \brief  Reference to AMS */
    CService                service;                    /*!< \brief  Service object */
    bool                    started;                    /*!< \brief  Is \c true if the AMD is allowed
                                                         *           to distribute messages
                                                         */
    CMaskedObserver         event_observer;             /*!< \brief  Observes init complete event */
    CMaskedObserver         terminate_observer;         /*!< \brief  Observes events leading to termination */

    void                   *preprocess_inst_ptr;        /*!< \brief Reference to the message preprocessor */
    Amd_RxMsgCompleteCb_t   preprocess_fptr;            /*!< \brief Callback function which is invoked first 
                                                         *          on completed message reception 
                                                         */

    void                   *receive_inst_ptr;           /*!< \brief Reference to the message receiver */
    Amd_RxMsgCompleteCb_t   receive_fptr;               /*!< \brief Callback function which is invoked after 
                                                         *          the preprocessor has finished
                                                         */
    CDlList                 pre_queue;                  /*!< \brief Queue of messages for the preprocessor */
    CDlList                 rx_queue;                   /*!< \brief Queue of messages for the receiver */

    CDlList                *first_q_ptr;                /*!< \brief Reference where to queue the messages first */
    Amd_RxMsgCompleteCb_t   first_receive_fptr;         /*!< \brief Reference of the callback to be fired first */
    void                   *first_receive_inst_ptr;     /*!< \brief Reference to the first receiver */

    Amd_RxModificationCb_t  rx_modification_fptr;       /*!< \brief Callback function for message modification */
    void                   *rx_modification_inst_ptr;   /*!< \brief Callback reference for message modification */

} CAmd;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Amd_Ctor(CAmd *self, CBase *base_ptr, CAms *ams_ptr);
extern void Amd_AssignPreprocessor(CAmd *self, Amd_RxMsgCompleteCb_t callback_fptr, void *inst_ptr);
extern void Amd_AssignReceiver(CAmd *self, Amd_RxMsgCompleteCb_t callback_fptr, void *inst_ptr);
extern void Amd_RxAssignModificator(CAmd *self, Amd_RxModificationCb_t callback_fptr, void *inst_ptr);

extern Ucs_AmsRx_Msg_t* Amd_PrePeekMsg(CAmd *self);
extern void Amd_PreReleaseMsg(CAmd *self);
extern void Amd_PreForwardMsg(CAmd *self);

extern Ucs_AmsRx_Msg_t* Amd_RxPeekMsg(CAmd *self);
extern void Amd_RxReleaseMsg(CAmd *self);
extern uint16_t Amd_RxGetMsgCnt(CAmd *self);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_AMD_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

