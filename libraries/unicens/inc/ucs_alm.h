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
 * \brief Internal header file of the API locking module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_ALM
 * @{
 */

#ifndef UCS_ALM_H
#define UCS_ALM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_eh.h"
#include "ucs_timer.h"
#include "ucs_obs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Bitmask used to store locked API methods */
typedef uint32_t Alm_ModuleMask_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Class structure of the API locking manager */
typedef struct CApiLockingManager_
{
    CTimerManagement *tm_ptr;           /*!< \brief Reference to timer management instance */
    CEventHandler *eh_ptr;              /*!< \brief Reference to event handler instance */
    CTimer garbage_collector;           /*!< \brief Timer for garbage collection */
    CDlList api_list;                   /*!< \brief List of registered APIs */
    void *ucs_user_ptr;         /*!< \brief User reference that needs to be passed in every callback function */
    CMaskedObserver internal_error_obs; /*!< \brief Error observer to handle internal errors and 
                                                    events */

} CApiLockingManager;

/*! \brief  Class structure of the API locking */
typedef struct CApiLocking_
{
    CDlNode node;                       /*!< \brief Node of the doubly linked (API-) list */
    CApiLockingManager *alm_ptr;        /*!< \brief Reference to CApiLockingManager instance */
    Alm_ModuleMask_t method_mask;       /*!< \brief Bitmask which holds locked API methods */
    Alm_ModuleMask_t timeout_mask;      /*!< \brief Bitmask to report timeouts */
    CSingleSubject subject;             /*!< \brief Subject to update registered observer */
    void * ucs_user_ptr;                /*!< \brief UNICENS instance ID */

} CApiLocking;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CApiLockingManager                                                         */
/*------------------------------------------------------------------------------------------------*/
extern void Alm_Ctor(CApiLockingManager *self,
                     CTimerManagement *tm_ptr,
                     CEventHandler *eh_ptr,
                     void * ucs_user_ptr);
extern void Alm_RegisterApi(CApiLockingManager *self, CApiLocking *al_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CApiLocking                                                                */
/*------------------------------------------------------------------------------------------------*/
extern void Al_Ctor(CApiLocking *self, CSingleObserver *obs_ptr, void * ucs_user_ptr);
extern bool Al_Lock(CApiLocking *self, Alm_ModuleMask_t method);
extern void Al_Release(CApiLocking *self, Alm_ModuleMask_t method);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_ALM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

