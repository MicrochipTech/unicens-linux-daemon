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
 * \brief Implementation of the API locking manager.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_ALM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_alm.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constant                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Interval for garbage collection */
static const uint16_t ALM_GARBAGE_COLLECTOR_INTERVAL = 2600U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Alm_HandleInternalErrors(void *self, void *error_code_ptr);
static void Alm_GarbageCollector(void *self);
static bool Alm_CheckRegisteredApi(void *current_alm_ptr, void *alm_inst_ptr);
static void Alm_StartTimeout(CApiLockingManager *self);
static void Alm_ClearTimeout(CApiLockingManager *self);
static bool Alm_SearchLockedApi(void *current_alm_ptr, void *alm_inst_ptr);
static void Alm_ResetRegisteredApis(CApiLockingManager *self);
static bool Alm_ResetApi(void *current_alm_ptr, void *alm_inst_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CApiLockingManager                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of the API locking manager class.
 *  \param  self        Instance pointer
 *  \param  tm_ptr      Reference to timer management instance
 *  \param  eh_ptr      Reference to event handler instance
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Alm_Ctor(CApiLockingManager *self,
              CTimerManagement *tm_ptr,
              CEventHandler *eh_ptr,
              void * ucs_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    T_Ctor(&self->garbage_collector);
    self->tm_ptr = tm_ptr;
    self->eh_ptr = eh_ptr;
    self->ucs_user_ptr = ucs_user_ptr;

    /* Observe internal errors and events */
    Mobs_Ctor(&self->internal_error_obs, self, EH_M_TERMINATION_EVENTS, &Alm_HandleInternalErrors);
    Eh_AddObsrvInternalEvent(self->eh_ptr, &self->internal_error_obs);
}

/*! \brief  Handles internal errors and events
 *  \param  self            Instance pointer
 *  \param  error_code_ptr  Reference to reported error code
 */
static void Alm_HandleInternalErrors(void *self, void *error_code_ptr)
{
    CApiLockingManager *self_ = (CApiLockingManager *)self;
    MISC_UNUSED(error_code_ptr);

    Tm_ClearTimer(self_->tm_ptr, &self_->garbage_collector);    /* Clear timeout */
    Alm_ResetRegisteredApis(self_);                             /* Reset all registered APIs */
}

/*! \brief  Checks for API locking timeouts. This method is the callback function of timer 
 *          \c garbage_collector.
 *  \param  self    Instance pointer
 */
static void Alm_GarbageCollector(void *self)
{
    CApiLockingManager *self_ = (CApiLockingManager *)self;
    (void)Dl_Foreach(&self_->api_list, &Alm_CheckRegisteredApi, self_);
}

/*! \brief  This method is used by Alm_GarbageCollector() to process each registered API.
 *  \param  current_alm_ptr     Reference to the current API
 *  \param  alm_inst_ptr        Instance of the API locking manager
 *  \return \c false to process all registered APIs
 */
static bool Alm_CheckRegisteredApi(void *current_alm_ptr, void *alm_inst_ptr)
{
    CApiLockingManager *self = (CApiLockingManager *)alm_inst_ptr;
    CApiLocking *alm_ptr_ = (CApiLocking *)current_alm_ptr;
    MISC_UNUSED(self);

    if(alm_ptr_->timeout_mask != 0U)
    {
        Alm_ModuleMask_t tmp_mask = 1U;
        while(alm_ptr_->timeout_mask != 0U)
        {
            if(tmp_mask == (tmp_mask & alm_ptr_->timeout_mask))
            {
                Ssub_Notify(&alm_ptr_->subject, &tmp_mask, false);
                alm_ptr_->method_mask &= ~tmp_mask;
                alm_ptr_->timeout_mask &= ~tmp_mask;
            }
            tmp_mask <<= 1;
        }
        Alm_ClearTimeout(self);
    }
    if(alm_ptr_->method_mask != 0U)
    {
        alm_ptr_->timeout_mask = alm_ptr_->method_mask;
    }
    return false;
}

/*! \brief  Registers a new API locking object.
 *  \param  self    Instance pointer
 *  \param  al_ptr  Reference to the API to register
 */
void Alm_RegisterApi(CApiLockingManager *self, CApiLocking *al_ptr)
{
    Dl_InsertTail(&self->api_list, &al_ptr->node);
    Dln_SetData(&al_ptr->node, al_ptr);
    al_ptr->alm_ptr = self;
}

/*! \brief  Starts the garbage collecting timer.
 *  \param  self    Instance pointer
 */
static void Alm_StartTimeout(CApiLockingManager *self)
{
    if(T_IsTimerInUse(&self->garbage_collector) == false)
    {
        Tm_SetTimer(self->tm_ptr,
                    &self->garbage_collector,
                    &Alm_GarbageCollector,
                    self,
                    ALM_GARBAGE_COLLECTOR_INTERVAL,
                    ALM_GARBAGE_COLLECTOR_INTERVAL);
    }
}

/*! \brief  Clears the garbage collecting timer. The timer is clear if no API locking flag is 
 *          currently pending.
 *  \param  self    Instance pointer
 */
static void Alm_ClearTimeout(CApiLockingManager *self)
{
    if(Dl_Foreach(&self->api_list, &Alm_SearchLockedApi, self) == NULL)
    {
        Tm_ClearTimer(self->tm_ptr, &self->garbage_collector);
    }
}

/*! \brief  Used by Alm_ClearTimeout() to check if at least one registered API is locked.
 *  \param  current_alm_ptr     Reference to the current API locking object
 *  \param  alm_inst_ptr        Instance of the API locking manager
 *  \return \c true if a locked API was found, otherwise \c false
 */
static bool Alm_SearchLockedApi(void *current_alm_ptr, void *alm_inst_ptr)
{
    CApiLocking *alm_ptr_ = (CApiLocking *)current_alm_ptr;
    bool ret_val = false;
    MISC_UNUSED(alm_inst_ptr);

    if(alm_ptr_->method_mask != 0U)
    {
        ret_val = true;
    }
    return ret_val;
}

/*! \brief  Resets all registered APIs. Called if an internal error has been occurred.
 *  \param  self    Instance pointer
 */
static void Alm_ResetRegisteredApis(CApiLockingManager *self)
{
    (void)Dl_Foreach(&self->api_list, &Alm_ResetApi, self);
}

/*! \brief  Used by Alm_ResetRegisteredApis() to reset all registered APIs.
 *  \param  current_alm_ptr     Reference to the current API locking object
 *  \param  alm_inst_ptr        Instance of the API locking manager
 *  \return \c false (process all registered APIs)
 */
static bool Alm_ResetApi(void *current_alm_ptr, void *alm_inst_ptr)
{
    CApiLocking *alm_ptr_ = (CApiLocking *)current_alm_ptr;
    MISC_UNUSED(alm_inst_ptr);

    alm_ptr_->method_mask = 0U;
    alm_ptr_->timeout_mask = 0U;

    return false;
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CApiLocking                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of the API locking class.
 *  \param  self            Instance pointer
 *  \param  obs_ptr         Observer to signal locked API methods
 *  \param  ucs_user_ptr     User reference that needs to be passed in every callback function
 */
void Al_Ctor(CApiLocking *self, CSingleObserver *obs_ptr, void * ucs_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->ucs_user_ptr = ucs_user_ptr;
    Dln_Ctor(&self->node, NULL);
    if(obs_ptr != NULL)
    {
        Ssub_Ctor(&self->subject, self->ucs_user_ptr);
        (void)Ssub_AddObserver(&self->subject, obs_ptr);
    }
}

/*! \brief  Locks the given API method.
 *  \param  self    Instance pointer
 *  \param  method  Bitmask of method to lock
 *  \return \c true if the API has been locked successfully
 *  \return \c false if the API was already locked
 */
bool Al_Lock(CApiLocking *self, Alm_ModuleMask_t method)
{
    bool ret_val = false;
    if((self->method_mask & method) == 0U)
    {
        ret_val = true;
        self->method_mask |= method;
        self->timeout_mask &= ~method;
        Alm_StartTimeout(self->alm_ptr);
    }
    return ret_val;
}

/*! \brief  Releases the lock of the given API method.
 *  \param  self    Instance pointer
 *  \param  method  Bitmask of method to lock
 */
void Al_Release(CApiLocking *self, Alm_ModuleMask_t method)
{
    self->method_mask &= ~method;
    self->timeout_mask &= ~method;
    Alm_ClearTimeout(self->alm_ptr);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

