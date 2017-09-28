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
 * \brief Internal header file of the Finite State Machine.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_FSM
 * @{
 */

#ifndef UCS_FSM_H
#define UCS_FSM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Internal state machine states which are also used as return values for method 
 *         Fsm_Service().
 */
typedef enum Fsm_State_
{
    FSM_STATE_IDLE,     /*!< \brief The state machine is in idle mode */
    FSM_STATE_SERVICE,  /*!< \brief An event is pending and the state machine must be serviced */
    FSM_STATE_WAIT,     /*!< \brief Waiting for asynchronous data/signal/event */
    FSM_STATE_END,      /*!< \brief The state machine is finished */
    FSM_STATE_ERROR     /*!< \brief An error occurred while processing the state machine */

} Fsm_State_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature used for state machine actions */
typedef void (*Fsm_Act_t)(void *self);

/*! \brief Structure is used to define state elements */
typedef struct Fsm_StateElem_
{
    /*! \brief Function pointer to the action that shall be executed */
    Fsm_Act_t action_fptr;
    /*! \brief Next state */
    int8_t next_state;

} Fsm_StateElem_t;

/*! \brief Class structure of the finite state machine */
typedef struct CFsm_
{
    /*! \brief Reference to transition table */
    const Fsm_StateElem_t *transition_table_ptr;
    /*! \brief Instance pointer used for actions */
    void *inst_ptr;
    /*! \brief Current event */
    int8_t event_occured;
    /*! \brief Current state */
    int8_t current_state;
    /*! \brief Maximum number of events */
    uint8_t num_events;
    /*! \brief Internal state of the state machine */
    Fsm_State_t internal_state;

} CFsm;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void Fsm_Ctor(CFsm *self, void *inst_ptr, const Fsm_StateElem_t *trans_table_ptr,
                     uint8_t num_events, int8_t init_state);
extern Fsm_State_t Fsm_Service(CFsm *self);
extern void Fsm_SetEvent(CFsm *self, int8_t e);
extern void Fsm_Wait(CFsm *self);
extern void Fsm_End(CFsm *self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_FSM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

