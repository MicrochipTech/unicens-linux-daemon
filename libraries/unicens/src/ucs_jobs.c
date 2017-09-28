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
 * \brief Implementation of Job classes
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_JOBS
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_jobs.h"
#include "ucs_misc.h"
/*#include "ucs_scheduler.h"
#include "ucs_trace.h"*/

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
static const uint8_t JBS_SRV_PRIO = 246U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Jbs_Service(void *self);
static void Jbq_OnJobResult(void *self, void *data_ptr);
static bool Jbs_ForEachJbq(void *d_ptr, void *ud_ptr);
static bool Jbq_CheckState(CJobQ *self, CJob *job_ptr);

/*------------------------------------------------------------------------------------------------*/
/* CJobService Methods                                                                            */
/*------------------------------------------------------------------------------------------------*/
void Jbs_Ctor(CJobService *self, CBase *base_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->base_ptr = base_ptr;
    Dl_Ctor(&self->list, base_ptr->ucs_user_ptr);
    Srv_Ctor(&self->service, JBS_SRV_PRIO, self, &Jbs_Service);
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);
}

void Jbs_RegisterJobQ(CJobService *self, CDlNode *job_q_node)
{
    Dl_InsertTail(&self->list, job_q_node);
}

void Jbs_TriggerEvent(CJobService *self, Srv_Event_t id)
{
    Srv_SetEvent(&self->service, id);
}

static bool Jbs_ForEachJbq(void *d_ptr, void *ud_ptr)
{
    Srv_Event_t *event_ptr = (Srv_Event_t*)ud_ptr;
    CJobQ *jobQ_ptr = (CJobQ*)d_ptr;

    if ((*event_ptr & Jbq_GetEventId(jobQ_ptr)) != 0U)
    {
        Jbq_Service(jobQ_ptr);
    }

    return false;   /* continue loop for all jobQs */
}

static void Jbs_Service(void *self)
{
    CJobService *self_ = (CJobService *)self; 
    Srv_Event_t event_mask;

    Srv_GetEvent(&self_->service, &event_mask);             /* save and reset current events */
    Srv_ClearEvent(&self_->service, event_mask);

    Dl_Foreach(&self_->list, &Jbs_ForEachJbq, &event_mask); /* service jobQ with the corresponding event */
}


/*------------------------------------------------------------------------------------------------*/
/* CJobQ Methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
void Jbq_Ctor(CJobQ *self, CJobService *job_service_ptr, Srv_Event_t event_id, CJob *job_list[])
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->job_service_ptr = job_service_ptr;
    self->event_id = event_id;
    self->job_list = job_list;
    
    self->index = 0U;
    self->state = JOB_S_STOPPED;
    self->result = JOB_R_NA;
    
    Dln_Ctor(&self->node, self);
    Ssub_Ctor(&self->q_subject, 0U /*inst id*/);
    Sobs_Ctor(&self->result_obs, self, &Jbq_OnJobResult);
    Jbs_RegisterJobQ(self->job_service_ptr, &self->node);
}

Srv_Event_t Jbq_GetEventId(CJobQ *self)
{
    return self->event_id;
}

void Jbq_Start(CJobQ *self, CSingleObserver *result_obs_ptr)
{
    if (self->state != JOB_S_STARTED)
    {
        if (self->job_list[self->index] != NULL)
        {
            TR_INFO((0U, "[JBQ]", "Jbq_Start(): Starting job queue. Id: 0x%04X", 1U, self->event_id));
            self->index = 0U;
            self->state = JOB_S_STARTED;
            self->result = JOB_R_NA;
            (void)Ssub_AddObserver(&self->q_subject, result_obs_ptr);       /* register observer for finished queue */
            Job_Start(self->job_list[self->index], &self->result_obs);      /* pass own observer for finished job */
        }
        else
        {
            TR_ERROR((0U, "[JBQ]", "Jbq_Start(): Invalid job list. Id: 0x%04X", 1U, self->event_id));
        }
    }
    else
    {
        TR_ERROR((0U, "[JBQ]", "Jbq_Start(): JobQ already started. Id: 0x%04X", 1U, self->event_id));
    }
}

void Jbq_Stop(CJobQ *self)
{
    if (self->state == JOB_S_STARTED)
    {
        if (self->job_list[self->index] != NULL)
        {
            self->index = 0U;
            self->state = JOB_S_STOPPED;
            self->result = JOB_R_NA;
            (void)Ssub_RemoveObserver(&self->q_subject);
            Job_Stop(self->job_list[self->index]);
        }
    }
}


static void Jbq_OnJobResult(void *self, void *data_ptr)
{
    CJobQ *self_ = (CJobQ *)self;
    Job_Result_t *result_ptr = (Job_Result_t *)data_ptr;
    
    if (self_->state == JOB_S_STARTED)
    {
        TR_INFO((0U, "[JBQ]", "Jbq_OnJobResult(): Receiving job result. event_id=0x%04X, result=0x%02X", 2U, self_->event_id, *result_ptr));
        Jbs_TriggerEvent(self_->job_service_ptr, self_->event_id);
    }
    else
    {
        TR_INFO((0U, "[JBQ]", "Jbq_OnJobResult(): Receiving job result for stopped job. Id: 0x%04X", 1U, self_->event_id));
    }

    MISC_UNUSED(result_ptr);
}


static bool Jbq_CheckState(CJobQ *self, CJob *job_ptr)
{
    bool ret = false;

    if (self->state == JOB_S_STARTED)
    {
        if (job_ptr != NULL)
        {
            if ((Job_GetState(job_ptr) == JOB_S_FINISHED) && (Job_GetResult(job_ptr) != JOB_R_NA))
            {
                ret = true;     /* job attributes are correct -> process */
            }
        }
        else
        {
            TR_ERROR((0U, "[JBQ]", "Jbq_Service(): Invalid job list. Id: 0x%04X", 1U, self->event_id));
        }
    }
    else
    {
        TR_ERROR((0U, "[JBQ]", "Jbq_Service(): JobQ not started. Id: 0x%04X", 1U, self->event_id));
    }

    return ret;
}

void Jbq_Service(CJobQ *self)
{
    CJob *curr_job_ptr = self->job_list[self->index];
    CJob *next_job_ptr = self->job_list[self->index + 1U];

    if (Jbq_CheckState(self, curr_job_ptr))
    {
        if (curr_job_ptr != NULL)
        {
            Job_Result_t tmp_res = Job_GetResult(curr_job_ptr);

            if ((next_job_ptr != NULL) && (tmp_res == JOB_R_SUCCESS))   /* job successfully and next job available */
            {
                self->index += 1U;
                Job_Start(next_job_ptr, &self->result_obs);
            }
            else        /* current job not successful or last job */
            {
                self->result = tmp_res;                                 /* copy status from last job and finish */
                self->state = JOB_S_FINISHED;
                Ssub_Notify(&self->q_subject, &tmp_res, true/*auto-remove*/);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* CJob Methods                                                                                   */
/*------------------------------------------------------------------------------------------------*/

void Job_Ctor(CJob *self, Job_StartCb_t start_fptr, void *inst_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->start_fptr = start_fptr;
    self->inst_ptr = inst_ptr;

    self->state = JOB_S_STOPPED;
    self->result = JOB_R_NA;
    Ssub_Ctor(&self->subject, 0U /*ucs instance*/);
}

void Job_Start(CJob *self, CSingleObserver *result_obs_ptr)
{
    if (self->state != JOB_S_STARTED)
    {
        TR_ASSERT(0U, "[JOB]", (self->start_fptr != NULL));
        (void)Ssub_AddObserver(&self->subject, result_obs_ptr);
        self->state = JOB_S_STARTED;
        self->result = JOB_R_NA;

        TR_INFO((0U, "[JOB]", "Job_Start(): starting job", 0U));
        self->start_fptr(self->inst_ptr);
    }
    else
    {
        TR_INFO((0U, "[JOB]", "Job_Start(): ambiguous state during job start", 0U));
    }
}

void Job_Stop(CJob *self)
{
    self->state = JOB_S_STOPPED;
    self->result = JOB_R_NA;
    Ssub_RemoveObserver(&self->subject);
    TR_INFO((0U, "[JOB]", "Job_Stop()", 0U));
}

void Job_SetResult(CJob *self, Job_Result_t result)
{
    TR_INFO((0U, "[JOB]", "Job_SetResult(): result=%d", 1U, result));

    if (self->state == JOB_S_STARTED)
    {
        self->state = JOB_S_FINISHED;
        self->result = result;
        Ssub_Notify(&self->subject, &result, true/*auto-remove*/);
        MISC_UNUSED(self);
        MISC_UNUSED(result);
    }
    else
    {
        TR_ERROR((0U, "[JOB]", "Job_SetResult(): called in ambiguous state=%d", 1U, self->state));
    }
}

Job_State_t Job_GetState(CJob *self)
{
    return self->state;
}

Job_Result_t Job_GetResult(CJob *self)
{
    return self->result;
}

















#if 0

/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of Manager class 
 *  \param self         The instance
 *  \param base_ptr     Reference to base component
 *  \param inic_ptr     Reference to INIC component
 *  \param net_ptr      Reference to net component
 *  \param packet_bw    Desired packet bandwidth
 */
void Mgr_Ctor(CManager *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, uint16_t packet_bw)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->base_ptr = base_ptr;
    self->inic_ptr = inic_ptr;
    self->net_ptr = net_ptr;
    self->packet_bw = packet_bw;

    Srv_Ctor(&self->service, MGR_SRV_PRIO, self, &Mgr_Service);             /* register service */
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);

    Mobs_Ctor(&self->event_observer, self, EH_E_INIT_SUCCEEDED, &Mgr_OnInitComplete);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_observer);

    Sobs_Ctor(&self->startup_obs, self, &Mgr_OnNwStartupResult);
    Sobs_Ctor(&self->shutdown_obs, self, &Mgr_OnNwShutdownResult);
    Mobs_Ctor(&self->nwstatus_mobs, self, MGR_NWSTATUS_MASK, &Mgr_OnNwStatus);
    Fsm_Ctor(&self->fsm, self, &(mgr_state_tbl[0][0]), (uint8_t)MGR_EV_MAX_NUM_EVENTS, MGR_EV_NIL/*init.event*/);
}

#endif



/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

