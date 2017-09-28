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
 * \brief Implementation of Port Message Event Handler
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMEH
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pmevent.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Pmev_OnFifosEvent(void *self, void *data_ptr);
static void Pmev_OnSystemEvent(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Constructor of class CPmEventHandler
 *  \param  self        The instance
 *  \param  base_ptr    Reference to base object
 *  \param  fifos_ptr   Reference to CPmFifos object
 */
void Pmev_Ctor(CPmEventHandler *self, CBase *base_ptr, CPmFifos *fifos_ptr)
{
    self->base_ptr = base_ptr;
    self->fifos_ptr = fifos_ptr;

    Obs_Ctor(&self->observer, self, &Pmev_OnFifosEvent);

    Mobs_Ctor(&self->sys_observer, self, (EH_E_BIST_FAILED | EH_E_INIT_FAILED), &Pmev_OnSystemEvent);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->sys_observer);
}

/*! \brief  Start reporting events to EH
 *  \param  self    The instance
 */
void Pmev_Start(CPmEventHandler *self)
{
    Fifos_AddEventObserver(self->fifos_ptr, &self->observer);
}

/*! \brief  Stops reporting events to EH
 *  \param  self    The instance
 */
void Pmev_Stop(CPmEventHandler *self)
{
    Fifos_RemoveEventObserver(self->fifos_ptr, &self->observer);
}

/*! \brief  Callback function to handle a PMS event
 *  \param  self        The instance
*   \param  data_ptr    Reference to the PMS event
 */
static void Pmev_OnFifosEvent(void *self, void *data_ptr)
{
    CPmEventHandler *self_ = (CPmEventHandler*)self;
    Fifos_Event_t *event_ptr = (Fifos_Event_t*)data_ptr;

    switch (*event_ptr)
    {
        case FIFOS_EV_SYNC_LOST:
            Eh_ReportEvent(&self_->base_ptr->eh, EH_E_SYNC_LOST);
            break;
        case FIFOS_EV_SYNC_ESTABLISHED:
            /* not relevant */
            break;
        case FIFOS_EV_SYNC_FAILED:
            /* not relevant */
            break;
        case FIFOS_EV_UNSYNC_COMPLETE:
            Eh_ReportEvent(&self_->base_ptr->eh, EH_E_UNSYNC_COMPLETE);
            break;
        case FIFOS_EV_UNSYNC_FAILED:
            Eh_ReportEvent(&self_->base_ptr->eh, EH_E_UNSYNC_FAILED);
            break;
        default:
            /* not relevant */
            break;
    }
}

/*! \brief  Callback function to handle an UCS system events
 *  \param  self        The instance
*   \param  data_ptr    Reference to the system event event
 */
static void Pmev_OnSystemEvent(void *self, void *data_ptr)
{
    CPmEventHandler *self_ = (CPmEventHandler*)self;
    Fifos_ForceTermination(self_->fifos_ptr);
    MISC_UNUSED(data_ptr);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

