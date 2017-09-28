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
 * \brief Implementation of the Finite State Machine.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_FSM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_fsm.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal definitions                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief NIL-event, indicates that no event is pending at the moment */
#define FSM_E_NILEVENT      0

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static Fsm_Act_t Fsm_StateEval(CFsm *self);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Finite State Machine class.
 *  \param self             Instance pointer
 *  \param inst_ptr         Instance pointer used for state machine actions
 *  \param trans_table_ptr  Pointer to transition table
 *  \param num_events       Maximum number of events
 *  \param init_state       Initialization state to start with
 */
void Fsm_Ctor(CFsm *self, void *inst_ptr, const Fsm_StateElem_t *trans_table_ptr,
              uint8_t num_events, int8_t init_state)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->inst_ptr             = inst_ptr;
    self->event_occured        = FSM_E_NILEVENT;       /* Clear event variable */
    self->current_state        = init_state;           /* Set initialization state */
    self->transition_table_ptr = trans_table_ptr;      /* Set pointer to given transition table */
    self->num_events           = num_events;           /* Store maximum number of events */
    self->internal_state       = FSM_STATE_IDLE;       /* Clear internal state */
}

/*! \brief   Determine required action
 *  \details This function determines the required action in dependency of the current state 
 *           and the triggered event. The current state will be transitioned to the next state. 
 *           The internal event variable will be cleared and the determined action will be 
 *           returned.
 *  \param   self    Instance pointer
 *  \return  Determined required action
 *  \return  \c NULL if no action is required
 */
static Fsm_Act_t Fsm_StateEval(CFsm *self)
{
    Fsm_Act_t retval = NULL;                                    /* Set default return value */

    if(self->event_occured != FSM_E_NILEVENT)                   /* Event occurred ? */
    {
        if((uint8_t)self->event_occured <= self->num_events)    /* Check if event is valid */
        {
            /* Get state-matrix-element in dependency of current state and triggered event */
            uint8_t i = ((uint8_t)self->current_state * self->num_events) + (uint8_t)self->event_occured;
            Fsm_StateElem_t stateEvaluation = self->transition_table_ptr[i];
            self->current_state = stateEvaluation.next_state;   /* Set new state */
            self->internal_state = FSM_STATE_IDLE;              /* Set internal state to \c IDLE */
            retval = stateEvaluation.action_fptr;               /* Return required action */
        }
        else
        {
            self->internal_state = FSM_STATE_ERROR;             /* Error occurred: Unknown event */
        }

        self->event_occured = FSM_E_NILEVENT;                   /* Clear event variable */
    }

    return retval;
}

/*! \brief   Service function for Finite State Machines
 *  \details The state machine will be serviced until it will be stopped by the user or no 
 *           further event is triggered. If a state transition occurred the associated action 
 *           will be executed.
 *  \param   self    Instance pointer
 *  \return  Internal state of the state machine (see \ref Fsm_State_t).
 */
Fsm_State_t Fsm_Service(CFsm *self)
{
    /* Internal state is set to \c FSM_STATE_SERVICE and any event is triggered? */
    while((self->internal_state == FSM_STATE_SERVICE)  && (self->event_occured != FSM_E_NILEVENT))
    {
        Fsm_Act_t action_fptr = Fsm_StateEval(self);    /* Execute state transition */
        if(action_fptr != NULL)                          /* Action required ? */
        {
            (*action_fptr)(self->inst_ptr);             /* Execute action */
        }
    }

    return self->internal_state;                        /* Return internal state machine state */
}

/*! \brief   Set an event
 *  \details This function sets the given event and triggers the service for the given 
 *           state machine.
 *  \param   self    Instance pointer
 *  \param   e       New event
 */
void Fsm_SetEvent(CFsm *self, int8_t e)
{
    if(self->internal_state != FSM_STATE_END)
    {
        self->event_occured  = e;                  /* Set new event */
        self->internal_state = FSM_STATE_SERVICE;  /* Set internal state to \c FSM_STATE_SERVICE */
    }
}

/*! \brief   Sets the wait state
 *  \details This function sets the given state state machine into the wait state. The state
 *           machine stops and must be re-triggered.
 *  \param   self    Instance pointer
 */
void Fsm_Wait(CFsm *self)
{
    if(self->internal_state != FSM_STATE_END)
    {
        self->internal_state = FSM_STATE_WAIT;     /* Set internal state to \c WAIT */
    }
}

/*! \brief   End processing of the state machine
 *  \details If this function is called the given state machine will be stopped immediately.
 *  \param   self    Instance pointer
 */
void Fsm_End(CFsm *self)
{
    self->internal_state = FSM_STATE_END;      /* Set internal state to \c END */
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

