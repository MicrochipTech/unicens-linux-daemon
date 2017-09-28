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
 * \brief Implementation of the timer management module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_TIMER
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_timer.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the TM service used by scheduler */
static const uint8_t TM_SRV_PRIO = 255U;    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the TM service */
static const Srv_Event_t TM_EVENT_UPDATE_TIMERS = 1U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Tm_Service(void *self);
static void Tm_UpdateTimers(CTimerManagement *self);
static bool Tm_HandleElapsedTimer(CTimerManagement *self);
static bool Tm_UpdateTimersAdd(void *c_timer_ptr, void *n_timer_ptr);
static void Tm_SetTimerInternal(CTimerManagement *self,
                                CTimer *timer_ptr,
                                Tm_Handler_t handler_fptr,
                                void *args_ptr,
                                uint16_t elapse,
                                uint16_t period);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CTimerManagement                                                       */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the timer management class.
 *  \param self        Instance pointer
 *  \param scd         Scheduler instance
 *  \param init_ptr    Reference to the initialization data
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Tm_Ctor(CTimerManagement *self, CScheduler *scd, const Tm_InitData_t *init_ptr, void * ucs_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->ucs_user_ptr = ucs_user_ptr;
    /* Initialize subjects and add observers */
    Ssub_Ctor(&self->get_tick_count_subject, self->ucs_user_ptr);
    (void)Ssub_AddObserver(&self->get_tick_count_subject,
                           init_ptr->get_tick_count_obs_ptr);
    if(init_ptr->set_application_timer_obs_ptr != NULL)
    {
        self->delayed_tm_service_enabled = true;
        Ssub_Ctor(&self->set_application_timer_subject, self->ucs_user_ptr);
        (void)Ssub_AddObserver(&self->set_application_timer_subject,
                               init_ptr->set_application_timer_obs_ptr);
    }
    /* Initialize timer management service */
    Srv_Ctor(&self->tm_srv, TM_SRV_PRIO, self, &Tm_Service);
    /* Add timer management service to scheduler */
    (void)Scd_AddService(scd, &self->tm_srv);
}

/*! \brief Service function of the timer management.
 *  \param self    Instance pointer
 */
static void Tm_Service(void *self)
{
    CTimerManagement *self_ = (CTimerManagement *)self;
    Srv_Event_t event_mask;

    Srv_GetEvent(&self_->tm_srv, &event_mask);

    if(TM_EVENT_UPDATE_TIMERS == (event_mask & TM_EVENT_UPDATE_TIMERS))     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->tm_srv, TM_EVENT_UPDATE_TIMERS);
        Tm_UpdateTimers(self_); 
    }
}

/*! \brief If event TM_EVENT_UPDATE_TIMERS is set this function is called. Handles the update
 *         of the timer list. If a timer has expired the corresponding callback function is
 *         executed. If the expired timer is a periodic timer, the timer will be set again.
 *  \param self    Instance pointer
 */
static void Tm_UpdateTimers(CTimerManagement *self)
{
    uint16_t current_tick_count;
    Ssub_Notify(&self->get_tick_count_subject, &current_tick_count, false);

    if(self->timer_list.head != NULL)      /* At least one timer is running? */
    {
        bool continue_loop = true;
        /* Calculate time difference between the current and the last TM service run */
        uint16_t tick_count_diff = (uint16_t)(current_tick_count - self->last_tick_count);
        /* Save current tick count for next service run */
        self->last_tick_count = current_tick_count;

        /* Loop while timer list is not empty */
        while((self->timer_list.head != NULL) && (continue_loop!= false))
        {
            /* Is not first timer in list elapsed yet? */
            if(tick_count_diff <= ((CTimer *)self->timer_list.head->data_ptr)->delta)
            {
                /* Update delta of first timer in list */
                ((CTimer *)self->timer_list.head->data_ptr)->delta -= tick_count_diff;
                tick_count_diff = 0U;
            }
            else    /* At least first timer in list elapsed */
            {
                /* Update tick count difference for next timer in list */
                tick_count_diff -= ((CTimer *)self->timer_list.head->data_ptr)->delta;
                /* First timer elapsed */
                ((CTimer *)self->timer_list.head->data_ptr)->delta = 0U;
            }

            /* First timer in list elapsed? */
            if(0U == ((CTimer *)self->timer_list.head->data_ptr)->delta)
            {
                /* Handle elapsed timer */
                continue_loop = Tm_HandleElapsedTimer(self);
            }
            else    /* No elapsed timer in list. */
            {
                /* First timer in list updated! Set trigger to inform application (see 
                   Tm_CheckForNextService()) and stop TM service. */
                self->set_service_timer = true;
                continue_loop = false;
            }
        }
    }
}

/*! \brief  This function is called if the first timer in list is elapsed. The timer handler 
 *          callback function is invoked. If the timer is a periodic timer it is wound up again.
 *  \param  self    Instance pointer
 *  \return \c true if the next timer must be check.
 *  \return \c false if the wound up timer (periodic timer) is new head of timer list
 */
static bool Tm_HandleElapsedTimer(CTimerManagement *self)
{
    bool ret_val = true;

    CDlNode *node = self->timer_list.head;
    /* Reset flag to be able to check if timer object has changed within handler 
        callback function */
    ((CTimer *)node->data_ptr)->changed = false;
    /* Call timer handler callback function */
    ((CTimer *)node->data_ptr)->handler_fptr(((CTimer *)node->data_ptr)->args_ptr);

    /* Timer object hasn't changed within handler callback function? */
    if(false == ((CTimer *)node->data_ptr)->changed)
    {
        /* Remove current timer from list */
        (void)Dl_Remove(&self->timer_list, node);
        /* Mark timer as unused */
        ((CTimer *)node->data_ptr)->in_use = false;
        /* Is current timer a periodic timer? */
        if(((CTimer *)node->data_ptr)->period > 0U)
        {
            /* Reload current timer */
            Tm_SetTimerInternal(self,
                                ((CTimer *)node->data_ptr),
                                ((CTimer *)node->data_ptr)->handler_fptr,
                                ((CTimer *)node->data_ptr)->args_ptr,
                                ((CTimer *)node->data_ptr)->period,
                                ((CTimer *)node->data_ptr)->period);

            if(node == self->timer_list.head)  /* Is current timer new head of list? */
            {
                /* Set trigger to inform application (see Tm_CheckForNextService()) and
                   stop TM service. */
                self->set_service_timer = true;
                ret_val = false;
            }
        }
    }

    return ret_val;
}

/*! \brief Calls an application callback function to inform the application that the UCS must be 
 *         serviced not later than the passed time period. If the timer list is empty a possible
 *         running application timer will be stopped. This function is called at the end of
 *         Ucs_Service().
 *  \param self    Instance pointer
 */
void Tm_CheckForNextService(CTimerManagement *self)
{
    if(self->delayed_tm_service_enabled != false)
    {
        uint16_t current_tick_count;
        Ssub_Notify(&self->get_tick_count_subject, &current_tick_count, false);
        /* Has head of timer list changed? */
        if(self->set_service_timer != false)
        {
            uint16_t new_time;
            uint16_t diff = current_tick_count - self->last_tick_count;
            self->set_service_timer = false;
            if (self->timer_list.head != NULL)
            {
                /* Timer expired since last TM service? */
                if(diff >= ((CTimer *)self->timer_list.head->data_ptr)->delta)
                {
                    new_time = 1U;  /* Return minimum value */
                }
                else
                {
                    /* Calculate new timeout */
                    new_time = (uint16_t)(((CTimer *)self->timer_list.head->data_ptr)->delta - diff);
                }
                /* Inform the application that the UCS must be serviced not later than the passed
                   time period. */
                Ssub_Notify(&self->set_application_timer_subject, &new_time, false);
            }
        }
    }
    else
    {
        Tm_TriggerService(self);    /* Application timer not implemented -> Retrigger TM */
    }
}

/*! \brief Helper function to set the TM service event.
 *  \details  This function is used by the application to trigger a service call of the Timer 
 *            Management if the application timer has expired.
 *  \param self            Instance pointer
 */
void Tm_TriggerService(CTimerManagement *self)
{
    if(self->timer_list.head != NULL)      /* At least one timer is running? */
    {
        Srv_SetEvent(&self->tm_srv, TM_EVENT_UPDATE_TIMERS);
    }
}

/*! \brief Helper function to stop the TM service.
 *  \param self            Instance pointer
 */
void Tm_StopService(CTimerManagement *self)
{
    uint16_t new_time = 0U;

    /* Clear probable running application timer */
    Ssub_Notify(&self->set_application_timer_subject, &new_time, false);

    /* Reset the service timer. Not necessary ?  */
    self->set_service_timer = false;

    /* Clear the timer head queue to prevent any event to be set */
    self->timer_list.head = NULL;
}

/*! \brief Creates a new timer. The timer expires at the specified elapse time and then after 
 *         every specified period. When the timer expires the specified callback function is
 *         called.
 *  \param self            Instance pointer
 *  \param timer_ptr       Reference to the timer object
 *  \param handler_fptr    Callback function which is called when the timer expires
 *  \param args_ptr        Reference to an optional parameter which is passed to the specified
 *                         callback function
 *  \param elapse          The elapse value before the timer expires for the first time, in
 *                         milliseconds
 *  \param period          The period of the timer, in milliseconds. If this parameter is zero, the
 *                         timer is signaled once. If the parameter is greater than zero, the timer
 *                         is periodic.
 */
void Tm_SetTimer(CTimerManagement *self,
                 CTimer *timer_ptr,
                 Tm_Handler_t handler_fptr,
                 void *args_ptr,
                 uint16_t elapse,
                 uint16_t period)
{
    (void)Tm_ClearTimer(self, timer_ptr);       /* Clear timer if running */
    /* Call the internal method to set the new timer (-> does not trigger TM service!) */
    Tm_SetTimerInternal(self, timer_ptr, handler_fptr, args_ptr, elapse, period);
    Tm_TriggerService(self);                    /* New timer added -> trigger timer list update */
}

/*! \brief This function contains the internal part when adding a new timer. The function is
 *         called within Tm_SetTimer() and within Tm_UpdateTimers().
 *  \param self            Instance pointer
 *  \param timer_ptr       Reference to the timer object
 *  \param handler_fptr    Callback function which is called when the timer expires
 *  \param args_ptr        Reference to an optional parameter which is passed to the specified
 *                         callback function
 *  \param elapse          The elapse value before the timer expires for the first time, in
 *                         milliseconds
 *  \param period          The period of the timer, in milliseconds. If this parameter is zero, the
 *                         timer is signaled once. If the parameter is greater than zero, the timer
 *                         is periodic.
 */
static void Tm_SetTimerInternal(CTimerManagement *self,
                                CTimer *timer_ptr,
                                Tm_Handler_t handler_fptr,
                                void *args_ptr,
                                uint16_t elapse,
                                uint16_t period)
{
    uint16_t current_tick_count;
    Ssub_Notify(&self->get_tick_count_subject, &current_tick_count, false);

    /* Save timer specific values */
    timer_ptr->changed = true;                  /* Flag is needed by Tm_UpdateTimers() */
    timer_ptr->in_use = true;
    timer_ptr->handler_fptr = handler_fptr;
    timer_ptr->args_ptr = args_ptr;
    timer_ptr->elapse = elapse;
    timer_ptr->period = period;
    timer_ptr->delta = elapse;

    /* Create back link to be able to point from node to timer object */
    timer_ptr->node.data_ptr = (void *)timer_ptr;

    if(self->timer_list.head == NULL)            /* Is timer list empty? */
    {
        Dl_InsertHead(&self->timer_list, &timer_ptr->node);    /* Add first timer to list */
        /* Save current tick count */
        Ssub_Notify(&self->get_tick_count_subject, &self->last_tick_count, false);
    }
    else                                        /* Timer list is not empty */
    {
        CDlNode *result_ptr = NULL;

        /* Set delta value in relation to last saved tick count (last TM service) */
        timer_ptr->delta += (uint16_t)(current_tick_count - self->last_tick_count);

        /* Search slot where new timer must be inserted. Update delta of new timer
           and delta of the following timer in the list. */
        result_ptr = Dl_Foreach(&self->timer_list, &Tm_UpdateTimersAdd, (void *)timer_ptr);

        if(result_ptr != NULL)                   /* Slot found? */
        {
            /* Insert new timer at found position */
            Dl_InsertBefore(&self->timer_list, result_ptr, &timer_ptr->node);
        }
        else                                    /* No slot found -> Insert as last node */
        {
            /* Add new timer to end of list */
            Dl_InsertTail(&self->timer_list, &timer_ptr->node);
        }
    }
}

/*! \brief     Removes the specified timer from the timer list.
 *  \param     self        Instance pointer
 *  \param     timer_ptr   Reference to the timer object
 *  \attention Make sure that for a timer object Tm_SetTimer() is called before Tm_ClearTimer()
 *             is called!
 */
void Tm_ClearTimer(CTimerManagement *self, CTimer *timer_ptr)
{
    if(timer_ptr->in_use != false)          /* Is timer currently in use? */
    {
        timer_ptr->changed = true;          /* Flag is needed by Tm_UpdateTimers() */

        if(timer_ptr->node.next != NULL)     /* Has deleted timer a follower? */
        {
            /* Adjust delta of following timer */
            ((CTimer *)timer_ptr->node.next->data_ptr)->delta += timer_ptr->delta;
        }

        (void)Dl_Remove(&self->timer_list, &timer_ptr->node);
        timer_ptr->in_use = false;

        Tm_TriggerService(self);            /* Timer removed -> trigger timer list update */
    }
}

/*! \brief  Used by Tm_SetTimer() to find the slot where the new timer must be inserted.
 *  \param  c_timer_ptr Reference to current timer processed by foreach loop
 *  \param  n_timer_ptr Reference to new timer
 *  \return \c true: Slot found, stop foreach loop
 *  \return \c false: Slot not found, continue foreach loop
 */
static bool Tm_UpdateTimersAdd(void *c_timer_ptr, void *n_timer_ptr)
{
    CTimer *current_timer_ptr = (CTimer *)c_timer_ptr;
    CTimer *new_timer_ptr = (CTimer *)n_timer_ptr;
    bool ret_val;

    /* Is current timer lesser than new timer? */
    if(current_timer_ptr->delta <= new_timer_ptr->delta)
    {
        /* Update delta of new timer and continue foreach loop */
        new_timer_ptr->delta -= current_timer_ptr->delta;
        ret_val = false;
    }
    else                                                    /* Slot found! */
    {
        /* Correct delta of current timer and stop foreach loop */
        current_timer_ptr->delta -= new_timer_ptr->delta;
        ret_val = true;
    }

    return ret_val;
}


/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CTimer                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Timer class.
 *  \param self        Instance pointer
 */
void T_Ctor(CTimer *self)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
}

/*! \brief  Returns the status of the given timer.
 *  \param  self        Instance pointer
 *  \return \c true if the timer is currently in use
 *  \return \c false if the timer is not currently in use
 */
bool T_IsTimerInUse(CTimer *self)
{
    return self->in_use;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

