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
 * \brief Implementation of class CPmFifos
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMFIFOS
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pmfifos.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The initialization value of sync_count. It is incremented for each sync or un-sync attempt. */
static const uint8_t     FIFOS_SYNC_CNT_INITIAL = 0xFFU;

/*------------------------------------------------------------------------------------------------*/
/* Internal typedefs                                                                              */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Fifos_Cleanup(CPmFifos *self);
static void Fifos_OnSyncTimeout(void *self);
static void Fifos_OnUnsyncTimeout(void *self);
static void Fifos_OnFifoEvent(void *self, void *fifo_id_ptr);

static void Fifos_HandleFifoStateChange(CPmFifos *self, Pmp_FifoId_t fifo_id);
static bool Fifos_AreAllFifosInState(CPmFifos *self, Fifo_SyncState_t target_state);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of class CPmFifos
 *  \param  self         The instance
 *  \param  base_ptr     Reference to basic services
 *  \param  channel_ptr  Reference to the port message channel
 *  \param  icm_fifo_ptr Reference to ICM FIFO, or NULL.
 *  \param  mcm_fifo_ptr Reference to MCM FIFO, or NULL.
 *  \param  rcm_fifo_ptr Reference to RCM FIFO, or NULL.
 *  \details At least one FIFO (MCM or ICM) must be provided.
 */
void Fifos_Ctor(CPmFifos *self, CBase *base_ptr, CPmChannel *channel_ptr, CPmFifo *icm_fifo_ptr, CPmFifo *mcm_fifo_ptr, CPmFifo *rcm_fifo_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->base_ptr      = base_ptr;
    self->channel_ptr   = channel_ptr;
    self->state         = FIFOS_S_UNSYNCED;

    self->unsync_initial = false;
    Fifos_ConfigureSyncParams(self, FIFOS_SYNC_RETRIES, FIFOS_SYNC_TIMEOUT);

    self->fifos[PMP_FIFO_ID_ICM] = icm_fifo_ptr;
    self->fifos[PMP_FIFO_ID_RCM] = rcm_fifo_ptr;
    self->fifos[PMP_FIFO_ID_MCM] = mcm_fifo_ptr;

    T_Ctor(&self->init_timer); 
    Sub_Ctor(&self->event_subject, self->base_ptr->ucs_user_ptr);
    Obs_Ctor(&self->obs_icm, self, &Fifos_OnFifoEvent);
    Obs_Ctor(&self->obs_rcm, self, &Fifos_OnFifoEvent);
    Obs_Ctor(&self->obs_mcm, self, &Fifos_OnFifoEvent);

    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[FIFOS]", (!((icm_fifo_ptr == NULL) && (mcm_fifo_ptr == NULL))));

    if (icm_fifo_ptr != NULL)
    {
        Fifo_AddStateObserver(icm_fifo_ptr, &self->obs_icm);
    }

    if (rcm_fifo_ptr != NULL)
    {
        Fifo_AddStateObserver(rcm_fifo_ptr, &self->obs_rcm);
    }

    if (mcm_fifo_ptr != NULL)
    {
        Fifo_AddStateObserver(mcm_fifo_ptr, &self->obs_mcm);
    }

    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_Ctor(): FIFOS created, state %d", 1U, self->state));
}

/*! \brief          Adds an observer of synchronization events
 *  \param self     The instance
 *  \param obs_ptr  The observer. The notification result type is Fifos_Event_t.
 */
void Fifos_AddEventObserver(CPmFifos *self, CObserver *obs_ptr)
{
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[FIFOS]", (obs_ptr != 0));
    (void)Sub_AddObserver(&self->event_subject, obs_ptr);
}

/*! \brief          Removes an observer of synchronization events
 *  \param self     The instance
 *  \param obs_ptr  The observer.
 */
void Fifos_RemoveEventObserver(CPmFifos *self, CObserver *obs_ptr)
{
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[FIFOS]", (obs_ptr != 0));
    (void)Sub_RemoveObserver(&self->event_subject, obs_ptr);
}

/*! \brief          Forces all FIFOs to state UNSYNCED without waiting for INIC responses and
 *                  without throwing events
 *  \details        Stops the LLD interface and releases all pending message resources.
 *                  This function shall be called if the UCS requires a un-normal termination
 *                  which is not detected by port message protocol.
 *  \param self     The instance
 */
void Fifos_ForceTermination(CPmFifos *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_ForceTermination(): Termination started, state: %d", 1U, self->state));
    Fifos_Cleanup(self);
    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_ForceTermination(): Termination done, state: %d", 1U, self->state));
}

/*! \brief          Configures retries and timeout for synchronize or un-synchronize
 *                  operation
 *  \details        This method shall be called before starting a synchronization or un-synchronization
 *                  or after it has finished. The current counter of synchronization attempts is reset.
 *  \param self     The instance
 *  \param retries  The number of retries until event FIFOS_EV_SYNC_FAILED or 
 *                  FIFOS_EV_UNSYNC_FAILED will be notified
 *  \param timeout  The timeout in milliseconds when the retry is performed
 */
void Fifos_ConfigureSyncParams(CPmFifos *self, uint8_t retries, uint16_t timeout)
{
    self->cmd_retries = retries;
    self->cmd_timeout = timeout;
    self->sync_cnt = FIFOS_SYNC_CNT_INITIAL;
}

/*------------------------------------------------------------------------------------------------*/
/* Synchronization                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief    Initializes all port message FIFOs
 *  \details  Possible results of the operation are the following events which are fired
 *            asynchronously. Refer also Fifos_AddEventObserver() and \ref Fifos_Event_t.
 *            - \ref FIFOS_EV_SYNC_ESTABLISHED
 *            - \ref FIFOS_EV_SYNC_FAILED
 *  \param    self        The instance
 *  \param    reset_cnt   If \c true resets the synchronization counter. In this case an automatic
 *                        retries will be done after the first synchronization timeout.
 *  \param    force_sync  If \c true the method will also trigger the synchronization of already 
 *                        synced \ref CPmFifo objects.
 */
void Fifos_Synchronize(CPmFifos *self, bool reset_cnt, bool force_sync)
{
    uint8_t cnt;
    self->state = FIFOS_S_SYNCING;
    self->unsync_initial = false;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_Synchronize(): Synchronization started, state: %d", 1U, self->state));

    if (reset_cnt)
    {
        self->sync_cnt = FIFOS_SYNC_CNT_INITIAL;
    }

    self->sync_cnt++;
    Pmch_Initialize(self->channel_ptr);                     /* Start LLD if not already done */

    for (cnt = 0U; cnt < PMP_MAX_NUM_FIFOS; cnt++)
    {
        if (self->fifos[cnt] != NULL)
        {
            if (force_sync || (Fifo_GetState(self->fifos[cnt]) != FIFO_S_SYNCED))
            {
                Fifo_Synchronize(self->fifos[cnt]);
            }
        }
    }

    Tm_SetTimer(&self->base_ptr->tm, &self->init_timer, 
                &Fifos_OnSyncTimeout, self, 
                self->cmd_timeout, 0U);
}

/*! \brief      Un-initializes all port message FIFOs
 *  \details    Possible results of the operation are the following events which are fired
 *              asynchronously. Refer also Fifos_AddEventObserver() and \ref Fifos_Event_t.
 *              - \ref FIFOS_EV_UNSYNC_COMPLETE
 *              - \ref FIFOS_EV_UNSYNC_FAILED
 *  \param      self       The instance
 *  \param      reset_cnt  If \c true resets the synchronization counter. In this case an automatic
 *                         retries will be done after the first synchronization timeout.
 *  \param      initial    If the un-synchronization shall be executed prior to a initial synchronization
 *                         it is recommended to set the argument to \c true. After notifying the event 
 *                         FIFOS_EV_UNSYNC_COMPLETE the LLD interface will not be stopped. The subsequent 
 *                         call of Fifos_Synchronize() will not start the LLD interface un-necessarily.
 *                         To trigger a final un-synchronization \c initial shall be set to \c false.
 *                         I.e., FIFOS_EV_UNSYNC_COMPLETE stops the LLD interface.
 */
void Fifos_Unsynchronize(CPmFifos *self, bool reset_cnt, bool initial)
{
    uint8_t cnt;
    self->state = FIFOS_S_UNSYNCING;
    self->unsync_initial = initial;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_Unsynchronize(): Un-synchronization started, state: %d", 1U, self->state));

    if (reset_cnt)
    {
        self->sync_cnt = FIFOS_SYNC_CNT_INITIAL;
    }

    self->sync_cnt++;
    Pmch_Initialize(self->channel_ptr);                     /* Start LLD if not already done */

    for (cnt = 0U; cnt < PMP_MAX_NUM_FIFOS; cnt++)
    {
        if (self->fifos[cnt] != NULL)
        {
            if (initial || (Fifo_GetState(self->fifos[cnt]) != FIFO_S_UNSYNCED_READY))
            {
                Fifo_Unsynchronize(self->fifos[cnt]);
            }
        }
    }

    Tm_SetTimer(&self->base_ptr->tm, &self->init_timer, 
                &Fifos_OnUnsyncTimeout, self, 
                self->cmd_timeout, 0U);
}

/*! \brief  Handles the synchronization timeout
 *  \param  self    The instance
 */
static void Fifos_OnSyncTimeout(void *self)
{
    CPmFifos *self_ = (CPmFifos*)self;
    Fifos_Event_t the_event = FIFOS_EV_SYNC_FAILED;

    self_->state = FIFOS_S_UNSYNCED;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_OnSyncTimeout(): state: %d", 1U, self_->state));

    if (self_->sync_cnt < self_->cmd_retries)
    {
        Fifos_Synchronize(self_, false, false);             /* retry synchronization after first timeout */
    }
    else
    {
        Fifos_Cleanup(self_);
        Sub_Notify(&self_->event_subject, &the_event);
    }
}

/*! \brief  Handles the un-synchronization timeout
 *  \param  self    The instance
 */
static void Fifos_OnUnsyncTimeout(void *self)
{
    CPmFifos *self_ = (CPmFifos*)self;
    Fifos_Event_t the_event = FIFOS_EV_UNSYNC_FAILED;

    self_->state = FIFOS_S_UNSYNCED;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_OnUnsyncTimeout(): state: %d", 1U, self_->state));

    if (self_->sync_cnt < self_->cmd_retries)
    {
        Fifos_Unsynchronize(self_, false, self_->unsync_initial);   /* retry synchronization after first timeout */
    }
    else
    {
        self_->unsync_initial = false;                              /* un-sync timeout will lead to termination - stop LLD */
        Fifos_Cleanup(self_);
        Sub_Notify(&self_->event_subject, &the_event);
    }
}

/*! \brief      Performs a cleanup of the Port Message Channel and the dedicated FIFOs
 *  \details    Releases all message objects which are currently in use.
 *  \param      self            The instance
 */
static void Fifos_Cleanup(CPmFifos *self)
{
    uint8_t count;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_Cleanup(): Channel cleanup started", 0U));

    if (self->unsync_initial == false)
    {
        Pmch_Uninitialize(self->channel_ptr);
    }

    for (count = 0U; count < PMP_MAX_NUM_FIFOS; count++)    /* stop & cleanup all FIFOs */
    {
        if (self->fifos[count] != NULL)
        {                                                   /* stop and avoid recursion */
            Fifo_Stop(self->fifos[count], FIFO_S_UNSYNCED_INIT, false);
            Fifo_Cleanup(self->fifos[count]);
        }
    }

    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_Cleanup(): Channel cleanup completed", 0U));

    /* notify external event after message objects were released */
    self->state = FIFOS_S_UNSYNCED;
}

/*------------------------------------------------------------------------------------------------*/
/* FIFO observation                                                                               */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Notifies an event to the host class
 *  \param  self        The instance
 *  \param  fifo_id_ptr Specific event identifier, pointer to "fifo_id"
 */
static void Fifos_OnFifoEvent(void *self, void *fifo_id_ptr)
{
    CPmFifos *self_ = (CPmFifos*)self;
    Fifos_HandleFifoStateChange(self_, *((Pmp_FifoId_t*)fifo_id_ptr));
}

/*! \brief  Executes transition to new synchronization states
 *  \param  self        The instance
 *  \param  fifo_id     The FIFO identifier
 */
static void Fifos_HandleFifoStateChange(CPmFifos *self, Pmp_FifoId_t fifo_id)
{
    Fifos_Event_t the_event;

    TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): FIFOs state: %d, FIFO: %d, FIFO State: %d", 3U, 
             self->state, fifo_id, Fifo_GetState(self->fifos[fifo_id])));

    switch (self->state)
    {
        case FIFOS_S_SYNCING:
            if (Fifos_AreAllFifosInState(self, FIFO_S_SYNCED))
            {
                self->state = FIFOS_S_SYNCED;                           /* now the complete channel is synced */
                Tm_ClearTimer(&self->base_ptr->tm, &self->init_timer);
                Fifos_ConfigureSyncParams(self, FIFOS_UNSYNC_RETRIES, FIFOS_UNSYNC_TIMEOUT);
                the_event = FIFOS_EV_SYNC_ESTABLISHED;
                Sub_Notify(&self->event_subject, &the_event);
                TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Synchronization of Port Message channel completed", 0U));
            }
            break;

        case FIFOS_S_UNSYNCING:
            if (Fifos_AreAllFifosInState(self, FIFO_S_UNSYNCED_READY))
            {
                Fifos_Cleanup(self);
                self->state = FIFOS_S_UNSYNCED;                         /* now the complete channel is un-synced */
                Tm_ClearTimer(&self->base_ptr->tm, &self->init_timer);
                the_event = FIFOS_EV_UNSYNC_COMPLETE;
                Sub_Notify(&self->event_subject, &the_event);
                TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Un-synchronization of Port Message channel completed", 0U));
            }
            break;

        case FIFOS_S_SYNCED:
            if (!Fifos_AreAllFifosInState(self, FIFO_S_SYNCED))
            {
                self->state = FIFOS_S_UNSYNCING;                        /* set state to 'unsyncing' and wait until all FIFOs are unsynced */
                self->sync_cnt = 0U;                                    /* pretend having triggered an un-sync which starts the timer */
                Tm_SetTimer(&self->base_ptr->tm, &self->init_timer, 
                            &Fifos_OnUnsyncTimeout, self, 
                            FIFOS_UNSYNC_TIMEOUT, 0U);
                the_event = FIFOS_EV_SYNC_LOST;
                Sub_Notify(&self->event_subject, &the_event);
                TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Lost synchronization of Port Message channel", 0U));
            }
            if (Fifos_AreAllFifosInState(self, FIFO_S_UNSYNCED_READY))
            {
                Fifos_Cleanup(self);
                self->state = FIFOS_S_UNSYNCED;                         /* the complete channel suddenly goes unsynced_complete */
                Tm_ClearTimer(&self->base_ptr->tm, &self->init_timer);
                the_event = FIFOS_EV_UNSYNC_COMPLETE;
                Sub_Notify(&self->event_subject, &the_event);
                TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Sudden un-synchronization of Port Message channel completed", 0U));
            }
            break;

        case FIFOS_S_UNSYNCED:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Unexpected FIFO event in state unsynced", 0U));
            break;

        default:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[FIFOS]", "Fifos_HandleFifoStateChange(): Unexpected FIFOs state", 0U));
            break;
    }

    MISC_UNUSED(fifo_id);
}

/*! \brief  Helper function that evaluates if all configured FIFOs are in a given state 
 *  \param  self            The instance
 *  \param  target_state    The required state that is evaluated for all FIFOs
 *  \return \c true if all FIFOs are in the given \c target_state, otherwise \c false.
 */
static bool Fifos_AreAllFifosInState(CPmFifos *self, Fifo_SyncState_t target_state)
{
    bool ret = true;
    uint8_t cnt;

    for (cnt = 0U; cnt < PMP_MAX_NUM_FIFOS; cnt++)
    {
        if (self->fifos[cnt] != NULL)
        {
            Fifo_SyncState_t state = Fifo_GetState(self->fifos[cnt]);

            if (state != target_state)
            {
                ret = false;
            }
        }
    }

    return ret;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

