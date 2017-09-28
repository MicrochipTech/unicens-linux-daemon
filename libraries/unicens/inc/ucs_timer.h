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
 * \brief Internal header file of the timer management module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_TIMER
 * @{
 */

#ifndef UCS_TIMER_H
#define UCS_TIMER_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_dl.h"
#include "ucs_scheduler.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature used for timer handler 
 *  \param args Void pointer to optional data
 */
typedef void (*Tm_Handler_t)(void *args);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Initialization structure of the timer management module. */
typedef struct Tm_InitData_
{
    /*! \brief Observer used to request current tick count value */
    CSingleObserver *get_tick_count_obs_ptr;
    /*! \brief Observer used to start application timer for delayed TM service calls */
    CSingleObserver *set_application_timer_obs_ptr;
    /*! \brief UNICENS instance ID */
    /*uint8_t ucs_inst_id;*/

} Tm_InitData_t;

/*! \brief   Class structure of the a timer object. */
typedef struct CTimer_
{
    /*! \brief Node of the doubly linked (timer-) list */
    CDlNode node;
    /*! \brief Handler function which is invoked when the timer expires */
    Tm_Handler_t handler_fptr;
    /*! \brief Reference to optional parameter */
    void *args_ptr;
    /*! \brief The Timeout value before the timer expires for the first time, in milliseconds */
    uint16_t elapse;
    /*! \brief The period of the timer, in milliseconds */
    uint16_t period;
    /*! \brief Delta time related to next timer in list */
    uint16_t delta;
    /*! \brief Flag which signals that the timer is in use */
    bool in_use;
    /*! \brief Flag to check if timer object has changed within timer handler callback function */
    bool changed;

} CTimer;

/*! \brief   Class structure of the timer management */
typedef struct CTimerManagement_
{
    /*! \brief Doubly linked list to manage the active timers */
    CDlList timer_list;
    /*! \brief Subject to request current tick count */
    CSingleSubject get_tick_count_subject;
    /*! \brief Subject to start the application timer which triggers a UCS service call */
    CSingleSubject set_application_timer_subject;
    /*! \brief Service instance to add the timer management to the scheduler */
    CService tm_srv;
    /*! \brief Last tick count value (saved at TM service) */
    uint16_t last_tick_count;
    /*! \brief Signals that the application timer callbacks are used */
    bool delayed_tm_service_enabled;
    /*! \brief Indicates that the application timer must be started */
    bool set_service_timer;
    /*! \brief UNICENS instance ID */
    void * ucs_user_ptr;

} CTimerManagement;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CTimerManagement                                                           */
/*------------------------------------------------------------------------------------------------*/
extern void Tm_Ctor(CTimerManagement *self, CScheduler *scd, const Tm_InitData_t *init_ptr, void * ucs_user_ptr);
extern void Tm_SetTimer(CTimerManagement *self, CTimer *timer_ptr, Tm_Handler_t handler_fptr, 
                        void *args_ptr, uint16_t elapse, uint16_t period);
extern void Tm_ClearTimer(CTimerManagement *self, CTimer *timer_ptr);
extern void Tm_CheckForNextService(CTimerManagement *self);
extern void Tm_TriggerService(CTimerManagement *self);
extern void Tm_StopService(CTimerManagement *self);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CTimer                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void T_Ctor(CTimer *self);
extern bool T_IsTimerInUse(CTimer *self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_TIMER_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

