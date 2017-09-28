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
 * \brief Internal header file of the event handler.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EH
 * @{
 */

#ifndef UCS_EH_H
#define UCS_EH_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_eh_pb.h"
#include "ucs_obs.h"
#include "ucs_trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief A control FIFO synchronization is lost. When this event occurs the PMS still waits 
 *         until all FIFOs are unsynchronized. So this event is no termination event.
 */
#define EH_E_SYNC_LOST              0x0001U
/*! \brief INIC Build-In-Self-Test failed
 */
#define EH_E_BIST_FAILED            0x0002U
/*! \brief Notifies completed un-synchronization of Port Message FIFOs
 */
#define EH_E_UNSYNC_COMPLETE        0x0004U
/*! \brief Notifies that the Port Message Channel was not able to un-synchronize its FIFOs 
 *         within a definite time
 */
#define EH_E_UNSYNC_FAILED          0x0008U
/*! \brief UNICENS initialization succeeded
 */
#define EH_E_INIT_SUCCEEDED         0x0010U
/*! \brief UNICENS initialization failed
 */
#define EH_E_INIT_FAILED            0x0020U

/*! \brief Mask including all events that lead to the termination of the UCS
 */
#define EH_M_TERMINATION_EVENTS     (EH_E_UNSYNC_COMPLETE | EH_E_UNSYNC_FAILED | \
                                     EH_E_BIST_FAILED | EH_E_INIT_FAILED)

/*! \brief Bitmask to identify all internal event codes
 */
#define EH_M_ALL_EVENTS             (EH_M_TERMINATION_EVENTS | EH_E_INIT_SUCCEEDED | EH_E_SYNC_LOST)

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature used for callback functions which notifies the event handler 
 *         observers.
 *  \param self             Instance pointer
 *  \param event_code       Reported event code
 */
typedef void (*Ehobs_UpdateCb_t)(void *self, uint32_t event_code);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Class structure of the event handler. */
typedef struct CEventHandler_
{
    /*! \brief Subject used for internal events */
    CSubject internal_event_subject;
    /*! \brief Single subject to report error to application */
    CSingleSubject public_error_subject;
    /*! \brief UNICENS instance ID */
    void * ucs_user_ptr;

} CEventHandler;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CEventHandler                                                              */
/*------------------------------------------------------------------------------------------------*/
extern void Eh_Ctor(CEventHandler *self, void *ucs_user_ptr);
extern void Eh_AddObsrvPublicError(CEventHandler *self, CSingleObserver *obs_ptr);
extern void Eh_DelObsrvPublicError(CEventHandler *self);
extern void Eh_ReportEvent(CEventHandler *self, uint32_t event_code);
extern void Eh_AddObsrvInternalEvent(CEventHandler *self, CMaskedObserver *obs_ptr);
extern void Eh_DelObsrvInternalEvent(CEventHandler *self, CMaskedObserver *obs_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_EH_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

