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
 * \brief Implementation of CManager class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_MGR
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_mgr.h"
#include "ucs_misc.h"
#include "ucs_scheduler.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Network status mask */
static const uint32_t    MGR_NWSTATUS_MASK = 0x0FU;         /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief The time in milliseconds the INIC will go to AutoForcedNA after sync lost. */
static const uint16_t    MGR_AUTOFORCED_NA_TIME = 5000U;    /* parasoft-suppress  MISRA2004-8_7 "configuration property" */


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Mgr_OnInitComplete(void *self, void *error_code_ptr);
static void Mgr_OnNwStatus(void *self, void *data_ptr);
static void Mgr_OnJobQResult(void *self, void *result_ptr);
static void Mgr_Startup(void *self);
static void Mgr_OnNwStartupResult(void *self, void *result_ptr);
static void Mgr_LeaveForcedNA(void *self);
static void Mgr_OnLeaveForcedNAResult(void *self, void *result_ptr);
#if 0
static void Mgr_Shutdown(void *self);
static void Mgr_OnNwShutdownResult(void *self, void *result_ptr);
#endif

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of Manager class 
 *  \param self         The instance
 *  \param base_ptr     Reference to base component
 *  \param inic_ptr     Reference to INIC component
 *  \param net_ptr      Reference to net component
 *  \param nd_ptr       Reference to NodeDiscovery component
 *  \param packet_bw    Desired packet bandwidth
 */
void Mgr_Ctor(CManager *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, CNodeDiscovery *nd_ptr, uint16_t packet_bw)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->initial = true;
    self->base_ptr = base_ptr;
    self->inic_ptr = inic_ptr;
    self->net_ptr = net_ptr;
    self->nd_ptr = nd_ptr;
    self->packet_bw = packet_bw;

    Jbs_Ctor(&self->job_service, base_ptr);
    Job_Ctor(&self->job_leave_forced_na, &Mgr_LeaveForcedNA, self);
    Job_Ctor(&self->job_startup, &Mgr_Startup, self);
#if 0
    Job_Ctor(&self->job_shutdown, &Mgr_Shutdown, self);
#endif

    self->list_startup[0] = &self->job_startup;
    self->list_startup[1] = NULL;
    self->list_force_startup[0] = &self->job_leave_forced_na;
    self->list_force_startup[1] = &self->job_startup;
    self->list_force_startup[2] = NULL;
#if 0
    self->list_shutdown[0] = &self->job_shutdown;
    self->list_shutdown[1] = NULL;
#endif

    Jbq_Ctor(&self->job_q_startup, &self->job_service, 1U, self->list_startup);
    Jbq_Ctor(&self->job_q_force_startup, &self->job_service, 2U, self->list_force_startup);
#if 0
    Jbq_Ctor(&self->job_q_shutdown, &self->job_service, 4U, self->list_shutdown);
#endif

    Sobs_Ctor(&self->startup_obs, self, &Mgr_OnNwStartupResult);
    Sobs_Ctor(&self->force_na_obs, self, &Mgr_OnLeaveForcedNAResult);
#if 0
    Sobs_Ctor(&self->shutdown_obs, self, &Mgr_OnNwShutdownResult);
#endif
    Sobs_Ctor(&self->job_q_obs, self, &Mgr_OnJobQResult);

    Mobs_Ctor(&self->event_observer, self, EH_E_INIT_SUCCEEDED, &Mgr_OnInitComplete);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_observer);
    Mobs_Ctor(&self->nwstatus_mobs, self, MGR_NWSTATUS_MASK, &Mgr_OnNwStatus);
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Methods                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked if the initialization is complete
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Mgr_OnInitComplete(void *self, void *error_code_ptr)
{
    CManager *self_ = (CManager*)self;
    MISC_UNUSED(error_code_ptr);

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Received init complete event", 0U));
    Net_AddObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_mobs);    /* register observer */
    (void)Nd_Start(self_->nd_ptr);
}

/*! \brief      NetworkStatus callback function
 *  \details    The function is only active if \c listening flag is \c true.
 *              This avoids to re-register und un-register the observer for several times.
 *  \param      self        The instance
 *  \param      data_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Mgr_OnNwStatus(void *self, void *data_ptr)
{
    CManager *self_ = (CManager*)self;
    Net_NetworkStatusParam_t *param_ptr = (Net_NetworkStatusParam_t *)data_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStatus()", 0U));

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStatus(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStatus(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStatus(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if ((param_ptr->change_mask & ((uint16_t)UCS_NW_M_AVAIL | (uint16_t)UCS_NW_M_PACKET_BW)) != 0U)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStatus(): trigger event", 0U));

        if (self_->current_q_ptr != NULL)
        {
            Jbq_Stop(self_->current_q_ptr);
        }

        if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FORCED_NA)
        {
            self_->current_q_ptr = &self_->job_q_force_startup;         /* stop forcing NA, then startup */
            Jbq_Start(&self_->job_q_force_startup, &self_->job_q_obs);
        }
        else if (param_ptr->availability == UCS_NW_NOT_AVAILABLE)
        {
            self_->current_q_ptr = &self_->job_q_startup;               /* just startup */
            Jbq_Start(&self_->job_q_startup, &self_->job_q_obs);
        }
#if 0
        else if ((param_ptr->node_position != 0U) || (param_ptr->packet_bw != self_->packet_bw))
        {
            self_->current_q_ptr = &self_->job_q_shutdown;              /* just shutdown - startup is triggered automatically */
            Jbq_Start(&self_->job_q_shutdown, &self_->job_q_obs);
        }
#endif       
        if (self_->initial != false)
        {
            self_->initial = false;
            if (self_->current_q_ptr == NULL)                           /* trigger InitAll() if no job is required for the */
            {                                                           /* initial network status notification */
                Nd_InitAll(self_->nd_ptr);
            }
        }
    }
}

/*! \brief      Callback function that is triggered after finished a job.
 *  \details    Failed jobs will be restarted here.
 *  \param      self        The instance
 *  \param      result_ptr  Reference to the job result \ref Job_Result_t.
 */
static void Mgr_OnJobQResult(void *self, void *result_ptr)
{
    CManager *self_ = (CManager*)self;
    Job_Result_t *res = (Job_Result_t *)result_ptr;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnJobQResult(): result=%d", 1U, *res));

    if ((*res != JOB_R_SUCCESS) && (self_->current_q_ptr != NULL))
    {
        Jbq_Start(self_->current_q_ptr, &self_->job_q_obs);
    }
    else
    {
        self_->current_q_ptr = NULL;
    }
}


/*------------------------------------------------------------------------------------------------*/
/* Job: LeaveForcedNA                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that sets the INIC from "Forced-NotAvailable" to "NotAvailable"
 *  \param      self    The instance
 */
static void Mgr_LeaveForcedNA(void *self)
{
    CManager *self_ = (CManager*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_LeaveForcedNA()", 0U));
    ret = Inic_NwForceNotAvailable(self_->inic_ptr, false /*no longer force NA*/, &self_->force_na_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_LeaveForcedNA(), function returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Inic_NwForceNotAvailable()
 *  \param  self         The instance
 *  \param  result_ptr   Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Mgr_OnLeaveForcedNAResult(void *self, void *result_ptr)
{
    CManager *self_ = (CManager*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnLeaveForcedNAResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_FAILED);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: Startup                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that starts the network with the given parameters
 *  \param      self    The instance
 */
static void Mgr_Startup(void *self)
{
    CManager *self_ = (CManager*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_Startup()", 0U));
    ret = Inic_NwStartup(self_->inic_ptr, MGR_AUTOFORCED_NA_TIME, self_->packet_bw, &self_->startup_obs);    /* Startup without ForcedNA */

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_Startup(), startup returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_startup, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Net_NetworkStartup()
 *  \param  self         The instance
 *  \param  result_ptr   Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Mgr_OnNwStartupResult(void *self, void *result_ptr)
{
    CManager *self_ = (CManager*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwStartupResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_startup, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_startup, JOB_R_FAILED);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: Shutdown                                                                                  */
/*------------------------------------------------------------------------------------------------*/
#if 0
/*! \brief      Action that performs a network shutdown.
 *  \param      self    The instance
 */
static void Mgr_Shutdown(void *self)
{
    CManager *self_ = (CManager*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_Shutdown()", 0U));
    ret = Inic_NwShutdown(self_->inic_ptr, &self_->shutdown_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_Shutdown(), shutdown returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_shutdown, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Net_NetworkShutdown()
 *  \param  self        The instance
 *  \param  result_ptr  Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Mgr_OnNwShutdownResult(void *self, void *result_ptr)
{
    CManager *self_ = (CManager*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Mgr_OnNwShutdownResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_FAILED);
    }
}
#endif

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

