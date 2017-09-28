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
 * \brief Implementation of the Extended Resource Manager. This file contains the implementation of 
 *        the basic functions of the class CExtendedResourceManager.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_XRM_INT
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_xrm.h"
#include "ucs_xrm_pv.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the XRM service used by scheduler */
const uint8_t XRM_SRV_PRIO = 250U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! \brief Event to trigger Extended Resource Manager service */
const Srv_Event_t XRM_EVENT_PROCESS              = 0x01U;
/*! \brief Event to trigger error handling */
const Srv_Event_t XRM_EVENT_ERROR                = 0x02U;
/*! \brief Event to trigger request list of invalid resource handles */
const Srv_Event_t XRM_EVENT_REQ_INV_RES_LST      = 0x04U;
/*! \brief Event to trigger destruction of invalid resources */
const Srv_Event_t XRM_EVENT_DESTROY_INV_RES      = 0x08U;
/*! \brief Event to resume the destruction of resources */
const Srv_Event_t XRM_EVENT_RESUME_JOB_DESTRUCT  = 0x10U;
/*! \brief Event to reset INIC's Resource Monitor */
const Srv_Event_t XRM_EVENT_RESET_RES_MONITOR    = 0x20U;
/*! \brief Event to trigger notification for automatically destroyed resources */
const Srv_Event_t XRM_EVENT_NOTIFY_AUTO_DEST_RES = 0x40U;
/*! \brief Event to trigger notification for destroyed resources */
const Srv_Event_t XRM_EVENT_NOTIFY_DESTROYED_JOB = 0x80U;
/*! \brief Event to trigger notification for automatically destroyed resources on remote devices */
const Srv_Event_t XRM_EVENT_NOTIFY_AUTO_DEST_RESR   = 0x100U;
/*! \brief Event to trigger configuration of a stream port */
const Srv_Event_t XRM_EVENT_STREAMPORT_CONFIG_SET   = 0x200U;
/*! \brief Event to read configuration of a stream port */
const Srv_Event_t XRM_EVENT_STREAMPORT_CONFIG_GET   = 0x400U;

/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Invalid resource handle */
const uint16_t XRM_INVALID_RESOURCE_HANDLE  = 0xFFFFU;
/*! \brief Invalid MOST connection label */
const uint16_t XRM_INVALID_CONNECTION_LABEL = 0xFFFFU;  /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! \brief Default value used for INIC sender handles */
const uint16_t XRM_DEFAULT_SENDER_HANDLE    = 0x0001U;
/*! \brief Invalid device node address */
const uint16_t XRM_INVALID_NODE_ADDRESS     = 0x0000U; 
/*! \brief Mask for network availability info */
const uint16_t XRM_MASK_NETWORK_AVAILABILITY  = 0x0002U;
/*! \brief Mask for node address update info */
const uint16_t XRM_MASK_NETWORK_NODE_ADDRESS  = 0x0010U; 

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CExtendedResourceManager                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Extended Resource Manager class.
 *  \param self     Instance pointer
 *  \param data_ptr Data pointer (receive reference to MNS instance)
 */
void Xrm_Ctor(CExtendedResourceManager *self, Xrm_InitData_t *data_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CExtendedResourceManager));

    /* Retrieve the initialization data */
    self->net_ptr  = data_ptr->net_ptr;
    self->base_ptr = data_ptr->base_ptr;
    self->rsm_ptr  = data_ptr->rsm_ptr;
    self->inic_ptr = data_ptr->inic_ptr;
    self->xrmp_ptr = data_ptr->xrmp_ptr;
    self->res_debugging_fptr = data_ptr->res_debugging_fptr;

    /* Set the flag that indicates the run mode of the instance */
    self->IsInRemoteControlMode = (UCS_ADDR_LOCAL_DEV != Inic_GetTargetAddress(self->inic_ptr)) ? true:false;

    /* Initialize observers */
    Obs_Ctor(&self->obs.tx_msg_obj_obs, self, &Xrm_MsgObjAvailCb);
    Obs_Ctor(&self->obs.resource_monitor_obs, self, &Xrm_ResourceMonitorCb);
    Sobs_Ctor(&self->obs.std_result_obs, self, &Xrm_StdResultCb);
    Sobs_Ctor(&self->obs.resource_invalid_list_obs, self, &Xrm_RequestResourceListResultCb);
    Sobs_Ctor(&self->obs.resource_destroy_obs, self, &Xrm_DestroyResourcesResultCb);
    Sobs_Ctor(&self->obs.stream_port_config_obs, self, &Xrm_Stream_PortConfigResult);
    Sobs_Ctor(&self->obs.most_port_enable_obs, self, &Xrm_Most_PortEnableResult);
    Sobs_Ctor(&self->obs.most_port_en_full_str_obs, self, &Xrm_Most_PortEnFullStrResult);
    Obs_Ctor(&self->obs.rsm_sync_lost_obs, self, &Xrm_RmtDevSyncLostCb);

    /* Add observer to resource monitor subject */
    Inic_AddObsrvResMonitor(self->inic_ptr, &self->obs.resource_monitor_obs);
    /* Initialize callback pointer for unmute callback */
    self->obs.check_unmute_fptr = data_ptr->check_unmute_fptr;

    /* Add observer to the MNS termination event */
    Mobs_Ctor(&self->obs.internal_error_obs, self, EH_M_TERMINATION_EVENTS, &Xrm_UninitializeService);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->obs.internal_error_obs);
    /* Add observer to the MNS network event */
    Mobs_Ctor(&self->obs.nw_status_obs, self, (XRM_MASK_NETWORK_AVAILABILITY | XRM_MASK_NETWORK_NODE_ADDRESS), &Xrm_MnsNwStatusInfosCb);
    Net_AddObserverNetworkStatus(self->net_ptr, &self->obs.nw_status_obs);
    /* Add observer to the MNS RSM event */
    Rsm_AddObserver(self->rsm_ptr, &self->obs.rsm_sync_lost_obs);

    /* Initialize the Jobs list queue */
    Dl_Ctor(&self->job_list, self->base_ptr->ucs_user_ptr);

    /* Initialize XRM service */
    Srv_Ctor(&self->xrm_srv, XRM_SRV_PRIO, self, &Xrm_Service);
    /* Add XRM service to scheduler */
    (void)Scd_AddService(&self->base_ptr->scd, &self->xrm_srv);
}

/*! \brief Service function of the Extended Resource Manager.
 *  \param self    Instance pointer
 */
void Xrm_Service(void *self)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->xrm_srv, &event_mask);

    /* Handle event to process a XRM job */
    if((event_mask & XRM_EVENT_PROCESS) == XRM_EVENT_PROCESS)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_PROCESS);
        Xrm_ProcessJob(self_);
    }
    /* Handle event to request the list of invalid resource handles */
    if((event_mask & XRM_EVENT_REQ_INV_RES_LST) == XRM_EVENT_REQ_INV_RES_LST)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_REQ_INV_RES_LST);
        Xrm_RequestResourceList(self_);
    }
    /* Handle event to destroy invalid INIC resources */
    if((event_mask & XRM_EVENT_DESTROY_INV_RES) == XRM_EVENT_DESTROY_INV_RES)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_DESTROY_INV_RES);
        Xrm_DestroyResources(self_, &Xrm_DestroyResourcesResultCb);
    }
    /* Handle event to resume the destruction of all INIC resources of a job */
    if((event_mask & XRM_EVENT_RESUME_JOB_DESTRUCT) == XRM_EVENT_RESUME_JOB_DESTRUCT)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_RESUME_JOB_DESTRUCT);
        Xrm_ResumeJobDestruction(self_);
    }
    /* Handle event to resume the destruction of all INIC resources of a job */
    if((event_mask & XRM_EVENT_RESET_RES_MONITOR) == XRM_EVENT_RESET_RES_MONITOR)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_RESET_RES_MONITOR);
        Xrm_ResetResourceMonitor(self_);
    }
    /* Handle error event */
    if((event_mask & XRM_EVENT_ERROR) == XRM_EVENT_ERROR)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
        Xrm_HandleError(self_);
    }
    /* Handle event to notify application of automatically destroyed resources */
    if((event_mask & XRM_EVENT_NOTIFY_AUTO_DEST_RES) == XRM_EVENT_NOTIFY_AUTO_DEST_RES)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_NOTIFY_AUTO_DEST_RES);
        Xrm_ReportAutoDestructionResult(self_);
    }
    /* Handle event to report result of resource destruction of a specific XRM job */
    if((event_mask & XRM_EVENT_NOTIFY_DESTROYED_JOB) == XRM_EVENT_NOTIFY_DESTROYED_JOB)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_NOTIFY_DESTROYED_JOB);
        Xrm_ReportJobDestructionResult(self_);
    }
    /* Handle event to notify application that resources on remote devices have been automatically destroyed */
    if ((event_mask & XRM_EVENT_NOTIFY_AUTO_DEST_RESR) == XRM_EVENT_NOTIFY_AUTO_DEST_RESR)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_NOTIFY_AUTO_DEST_RESR);
        Xrm_ReleaseResrcHandles(self_);
    }
    /* Handle event to set streaming port configuration */
    if ((event_mask & XRM_EVENT_STREAMPORT_CONFIG_SET) == XRM_EVENT_STREAMPORT_CONFIG_SET)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_STREAMPORT_CONFIG_SET);
        (void)Xrm_SetStreamPortConfiguration(self_);
    }
    /* Handle event to get streaming port configuration */
    if ((event_mask & XRM_EVENT_STREAMPORT_CONFIG_GET) == XRM_EVENT_STREAMPORT_CONFIG_GET)
    {
        Srv_ClearEvent(&self_->xrm_srv, XRM_EVENT_STREAMPORT_CONFIG_GET);
        (void)Xrm_GetStreamPortConfiguration(self_);
    }
}

/*! \brief Checks if the API is locked and the MNS are initialized.
 *  \param self     Instance pointer
 *  \return \c true if the API is not locked and the MNS are initialized, otherwise \c false.
 */
bool Xrm_IsApiFree(CExtendedResourceManager *self)
{
    return (self->lock_api == false);
}

/*! \brief Locks/Unlocks the XRM API.
 *  \param self     Instance pointer
 *  \param status   Locking status. \c true = Lock, \c false = Unlock
 */
void Xrm_ApiLocking(CExtendedResourceManager *self, bool status)
{
    self->lock_api = status;
}

/*! \brief  Add observer to be notified if ICM TX message object is available. Store pending events.
 *  \param  self        Instance pointer
 *  \param  event_mask  Event to be queued
 */
void Xrm_WaitForTxMsgObj(CExtendedResourceManager *self, Srv_Event_t event_mask)
{
    Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->obs.tx_msg_obj_obs);
    self->queued_event_mask |= event_mask;
}

/*! \brief  Checks whether the given resource object list is part of the given Job
 *  \param  job_ptr    Reference to a job list
 *  \param  ud_ptr     Reference to the user data. Not used !
 *  \return \c true if it's part of my job list, otherwise \c false.
 */
bool Xrm_SetNtfForThisJob(void * job_ptr, void * ud_ptr)
{
    Xrm_Job_t * job_ptr_ = (Xrm_Job_t *)job_ptr;
    MISC_UNUSED(ud_ptr);

    if(job_ptr_->valid != false)
    {
        job_ptr_->notify = true;
    }

    return false;
}

/*! \brief  Handle internal errors and un-initialize XRM service.
 *  \param  self            Instance pointer
 *  \param  error_code_ptr  Reference to internal error code
 */
void Xrm_UninitializeService(void *self, void *error_code_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    MISC_UNUSED(error_code_ptr);

    Xrm_ApiLocking(self_, true);

    MISC_MEM_SET(&self_->report_result, 0x00, sizeof(Ucs_Xrm_Result_t));
    self_->report_result.code = UCS_XRM_RES_RC_AUTO_DESTROYED;

    (void)Dl_Foreach(&self_->job_list, &Xrm_SetNtfForThisJob, NULL);

    /* Notify destruction of current connections */
    Xrm_NotifyInvalidJobs(self_);
    /* Remove XRM service from schedulers list */
    (void)Scd_RemoveService(&self_->base_ptr->scd, &self_->xrm_srv);
    /* Remove error/event observers */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->obs.internal_error_obs);
    /* Remove rsm observers */
    Rsm_DelObserver(self_->rsm_ptr, &self_->obs.rsm_sync_lost_obs);
}


/*! \brief  Handle the network status information mask "Availability" and "NodeAddress".
 *  \param  self            Instance pointer
 *  \param  result_ptr      Reference to the results
 */
void Xrm_MnsNwStatusInfosCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Net_NetworkStatusParam_t *result_ptr_ = (Net_NetworkStatusParam_t *)result_ptr;

    if ((XRM_MASK_NETWORK_AVAILABILITY & result_ptr_->change_mask) == XRM_MASK_NETWORK_AVAILABILITY)
    {
        if ((result_ptr_->availability == UCS_NW_NOT_AVAILABLE)  &&
            (self_->IsInRemoteControlMode))
        {
            /* Release all resources */
            Xrm_ReleaseResrcHandles(self_);
        }
    }
}

/*! \brief  Whenever this function is called, a message object (ICM or MCM) is available.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Not used!
 */
void Xrm_MsgObjAvailCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    MISC_UNUSED(result_ptr);
    Srv_SetEvent(&self_->xrm_srv, self_->queued_event_mask);
    self_->queued_event_mask = 0U;
    Inic_DelObsrvOnTxMsgObjAvail(self_->inic_ptr, &self_->obs.tx_msg_obj_obs);
}

/*! \brief  Whenever this function is called, all remote devices have lost the synchronization.
 *  \param  self        instance pointer 
 *  \param  result_ptr  Not Used !
 */
void Xrm_RmtDevSyncLostCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    MISC_UNUSED(result_ptr);

    Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_NOTIFY_AUTO_DEST_RESR);
}

/*! \brief Processes the XRM job that is specified by the given resource object list.
 *  \param self                             Instance pointer
 *  \param resource_object_list[]           Reference to array of references to INIC resource objects
 *  \param most_network_connection_label    MOST network connection label
 *  \param user_arg                         User argument
 *  \param report_fptr                      Report function pointer
 *  \return  Possible return values are shown in the table below.
 *           Value                     | Description 
 *           ------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS           | No error 
 *           UCS_RET_ERR_NOT_AVAILABLE | Associated job not found
 *           UCS_RET_ERR_PARAM         | Null pointer detected
 *           UCS_RET_ERR_API_LOCKED    | API is currently locked
 */
Ucs_Return_t Xrm_Process(CExtendedResourceManager *self, 
                         UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[],
                         uint16_t most_network_connection_label,
                         void * user_arg,
                         Ucs_Xrm_ReportCb_t report_fptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self != NULL)
    {
        if(Xrm_IsApiFree(self) != false)
        {
            if((resource_object_list != NULL) && (report_fptr != NULL))
            {
                Xrm_ApiLocking(self, true);
                self->current_job_ptr = Xrm_GetJob(self, resource_object_list);
                if(self->current_job_ptr != NULL)
                {
                    bool job_is_mine = Dl_IsNodeInList(&self->job_list, &self->current_job_ptr->node);
                    if (job_is_mine)
                    {
                        if(self->current_job_ptr->valid == false)
                        {
                            self->current_job_ptr->user_arg = user_arg;
                            self->current_job_ptr->valid = true;
                            self->current_job_ptr->notify = false;
                            self->current_job_ptr->report_fptr = report_fptr;
                            self->current_job_ptr->most_network_connection_label = most_network_connection_label;
                            self->current_job_ptr->resource_object_list_ptr = resource_object_list;
                            self->current_obj_pptr = &self->current_job_ptr->resource_object_list_ptr[0];
                            Xrm_ProcessJob(self);
                        }
                        else
                        {
                            ret_val = UCS_RET_ERR_ALREADY_SET;
                            Xrm_ApiLocking(self, false);
                        }
                    }
                    else
                    {
                        ret_val = UCS_RET_ERR_PARAM;
                        Xrm_ApiLocking(self, false);
                    }
                }
                else
                {
                    Xrm_ApiLocking(self, false);
                    ret_val = UCS_RET_ERR_NOT_AVAILABLE;
                }
            }
            else
            {
                ret_val = UCS_RET_ERR_PARAM;
            }
        }
        else
        {
            ret_val = UCS_RET_ERR_API_LOCKED;
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_PARAM;
    }

    return ret_val;
}

/*! \brief   Destroys all resources that are specified by the given resource object list.
 *  \details This function triggers the destruction of all resources which are used by the given
 *           job. A resource will be destroyed only if it is not used by other valid resources.
 *  \param   self                     Instance pointer
 *  \param   resource_object_list[]   Reference to array of references to INIC resource objects
 *  \return  Possible return values are shown in the table below.
 *           Value                     | Description 
 *           ------------------------- | ------------------------------------------------------
 *           UCS_RET_SUCCESS           | No error
 *           UCS_RET_ERR_ALREADY_SET   | Connection is already destroyed
 *           UCS_RET_ERR_NOT_AVAILABLE | Associated job not found
 *           UCS_RET_ERR_PARAM         | Null pointer detected
 *           UCS_RET_ERR_API_LOCKED    | API is currently locked
 */
Ucs_Return_t Xrm_Destroy(CExtendedResourceManager *self,
                         UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[])
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self != NULL)
    {
        if(Xrm_IsApiFree(self) != false)
        {
            if(resource_object_list != NULL)
            {
                Xrm_ApiLocking(self, true);
                self->current_job_ptr = Xrm_GetJob(self, resource_object_list);
                if((self->current_job_ptr != NULL) &&
                   (self->current_job_ptr->resource_object_list_ptr != NULL))
                {
                    Xrm_PreJobDestrResult_t result;

                    result = Xrm_PrepareJobDestruction(self);
                    if(result == XRM_PRE_JOB_DEST_TASKS_EXIST)
                    {
                        Xrm_DestroyResources(self, &Xrm_DestroyJobResourcesResultCb);
                    }
                    else if(result == XRM_PRE_JOB_DEST_DONE)
                    {
                        Srv_SetEvent(&self->xrm_srv, XRM_EVENT_NOTIFY_DESTROYED_JOB);
                    }
                    else if (result == XRM_PRE_JOB_DEST_BUSY)
                    {
                        Xrm_ApiLocking(self, false);
                        ret_val = UCS_RET_ERR_API_LOCKED;
                    }
                    else
                    {
                        Xrm_ApiLocking(self, false);
                        ret_val = UCS_RET_ERR_ALREADY_SET;
                    }
                }
                else
                {
                    Xrm_ApiLocking(self, false);
                    ret_val = UCS_RET_ERR_NOT_AVAILABLE;
                }
            }
            else
            {
                ret_val = UCS_RET_ERR_PARAM;
            }
        }
        else
        {
            ret_val = UCS_RET_ERR_API_LOCKED;
        }
    }
    else
    {
        /* This means that there is no instance associated to this job,
         * what in turn means that the job is not available. 
         */
        ret_val = UCS_RET_ERR_NOT_AVAILABLE;
    }

    return ret_val;
}

/*! \brief  Prepares the destruction of INIC resources of the current job.
 *  \param  self    Instance pointer
 *  \return  Possible return values are shown in the table below.
 *           Value                           | Description 
 *           ------------------------------- | ------------------------------------
 *           XRM_PRE_JOB_DEST_TASKS_EXIST    | There are resources to destroy
 *           XRM_PRE_JOB_DEST_NO_TASKS_EXIST | All resources already destroyed
 *           XRM_PRE_JOB_DEST_DONE           | Only shared resources affected. Invoke result callback immediately
 *           XRM_PRE_JOB_DEST_BUSY           | Preparation of JobDestruction is currently not possible. Other resources are currently being destroyed
 */
Xrm_PreJobDestrResult_t Xrm_PrepareJobDestruction(CExtendedResourceManager *self)
{
    Xrm_PreJobDestrResult_t ret_val = XRM_PRE_JOB_DEST_BUSY;
    if (self->inv_resource_handle_list_size == 0U)
    {
        ret_val = Xrm_UnsafePrepareJobDestruction(self);
    }
    return ret_val;
}

/*! \brief  Prepares precariously the destruction of INIC resources of the current job (This was legacy code and is unsafe).
 *  \param  self    Instance pointer
 *  \return  Possible return values are shown in the table below.
 *           Value                           | Description 
 *           ------------------------------- | ------------------------------------
 *           XRM_PRE_JOB_DEST_TASKS_EXIST    | There are resources to destroy
 *           XRM_PRE_JOB_DEST_NO_TASKS_EXIST | All resources already destroyed
 *           XRM_PRE_JOB_DEST_DONE           | Only shared resources affected. Invoke result callback immediately
 */
Xrm_PreJobDestrResult_t Xrm_UnsafePrepareJobDestruction(CExtendedResourceManager *self)
{
    uint8_t i;
    uint16_t resource_handle;
    Xrm_PreJobDestrResult_t ret_val = XRM_PRE_JOB_DEST_NO_TASKS_EXIST;
    self->inv_resource_handle_index     = 0U;
    self->inv_resource_handle_list_size = 0U;
    for(i=Xrm_CountResourceObjects(self, self->current_job_ptr); (i>0U) && (self->inv_resource_handle_list_size < XRM_NUM_RES_HDL_PER_ICM); i--)
    {
        uint8_t count = Xrm_CountResourceHandleEntries(self, self->current_job_ptr->resource_object_list_ptr[i - 1U]);
        if(count == 1U)
        {
            resource_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, self->current_job_ptr->resource_object_list_ptr[i - 1U], NULL);
            if(resource_handle != XRM_INVALID_RESOURCE_HANDLE)
            {
                self->inv_resource_handle_list[self->inv_resource_handle_list_size] = resource_handle;
                self->inv_resource_handle_list_size++;
                ret_val = XRM_PRE_JOB_DEST_TASKS_EXIST;
            }
        }
        else if(count > 0U)
        {
            Xrm_ReleaseResourceHandle(self, self->current_job_ptr, self->current_job_ptr->resource_object_list_ptr[i - 1U]);
            ret_val = (ret_val == XRM_PRE_JOB_DEST_NO_TASKS_EXIST) ? XRM_PRE_JOB_DEST_DONE : ret_val;
        }
    }
    return ret_val;
}


/*! \brief  Resumes the destruction of all resources of the current job.
 *  \param  self    Instance pointer
 */
void Xrm_ResumeJobDestruction(CExtendedResourceManager *self)
{
    if(Xrm_UnsafePrepareJobDestruction(self) == XRM_PRE_JOB_DEST_TASKS_EXIST)
    {
        Xrm_DestroyResources(self, &Xrm_DestroyJobResourcesResultCb);
    }
    else
    {
        MISC_MEM_SET(&self->report_result, 0x00, sizeof(Ucs_Xrm_Result_t));
        self->report_result.code = UCS_XRM_RES_SUCCESS_DESTROY;
        Xrm_NotifyInvalidJobs(self);
        Xrm_ApiLocking(self, false);
    }
}

/*! \brief  Returns the number of resource objects for the job that is identified by the given job
 *          reference.
 *  \param  self        Instance pointer
 *  \param  job_ptr     Reference to job
 *  \return Number of INIC resource objects of the desired job
 */
uint8_t Xrm_CountResourceObjects(CExtendedResourceManager *self, Xrm_Job_t *job_ptr)
{
    uint8_t num_resource_objects = 0U;
    MISC_UNUSED(self);
    while(job_ptr->resource_object_list_ptr[num_resource_objects] != NULL)
    {
        num_resource_objects++;
    }

    return num_resource_objects;
}

/*! \brief  Returns the reference of the job that is identified by the given resource object list.
 *  \param  self                        Instance pointer
 *  \param  resource_object_list[]      Reference to array of references to INIC resource objects
 *  \return Reference to the desired job if the job was found, otherwise NULL.
 */
Xrm_Job_t * Xrm_GetJob(CExtendedResourceManager *self,
                       UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[])
{
    Xrm_Job_t *ret_ptr = NULL;

    ret_ptr = Xrmp_GetJob(self->xrmp_ptr, resource_object_list);
    if (ret_ptr != NULL)
    {
        if ((!Dl_IsNodeInList(&self->job_list, &ret_ptr->node)) && 
            (!Dln_IsNodePartOfAList(&ret_ptr->node)))
        {
            Dln_SetData(&ret_ptr->node, ret_ptr);
            Dl_InsertTail(&self->job_list, &ret_ptr->node);
        }
    }

    return ret_ptr;
}

/*! \brief  Checks whether the given resource object list is part of the given Job
 *  \param  job_ptr           Reference to a job list
 *  \param  resrc_obj_ptr     Reference to array of references to INIC resource objects
 *  \return \c true if it's part of my job list, otherwise \c false.
 */
bool Xrm_IsPartOfJobList (void * job_ptr, void * resrc_obj_ptr)
{
    Xrm_Job_t *job_ptr_ = (Xrm_Job_t *)job_ptr;
    bool ret_val = false;

    if(job_ptr_->resource_object_list_ptr == (UCS_XRM_CONST Ucs_Xrm_ResObject_t **)resrc_obj_ptr)
    {
        ret_val = true;
    }

    return ret_val;
}

/*! \brief  Checks whether the given resource object list is part of my Job list
 *  \param  self                        Instance pointer
 *  \param  resource_object_list[]      Reference to array of references to INIC resource objects
 *  \return \c true if it's part of my job list, otherwise \c false.
 */
bool Xrm_IsInMyJobList(CExtendedResourceManager *self, UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[])
{
    return (NULL != Dl_Foreach(&self->job_list, &Xrm_IsPartOfJobList, (void *)resource_object_list));
}

/*! \brief  Returns the table index of the given resource object.
 *  \param  self        Instance pointer
 *  \param  job_ptr     Reference to job
 *  \param  obj_pptr    Reference to array of references to INIC resource objects
 *  \return Table index of the given resource object. If entry is not found 0xFF is returned.
 */
uint8_t Xrm_GetResourceObjectIndex(CExtendedResourceManager *self,
                                   Xrm_Job_t *job_ptr,
                                   UCS_XRM_CONST Ucs_Xrm_ResObject_t **obj_pptr)
{
    return Xrmp_GetResourceHandleIdx(self->xrmp_ptr, job_ptr, obj_pptr);
}

/*! \brief  Check if the current device is already attached respectively sync'ed.
 *  \param  self    Instance pointer
 *  \return \c true if no error occurred, otherwise \c false.
 */
bool Xrm_IsCurrDeviceAlreadyAttached(CExtendedResourceManager *self)
{
    bool ret_val = true;

    if (Rsm_GetDevState(self->rsm_ptr) == RSM_DEV_UNSYNCED)
    {
        ret_val = false;
    }

    return ret_val;
}

/*! \brief  Check if the current device is already attached respectively sync'ed.
 *  \param  self    XRM Instance pointer
 *  \param  job_ptr Reference to the XRM job to be looked for
 *  \return \c true if the given job is part of my jobs_list, otherwise \c false.
 */
bool Xrm_IsInMyJobsList (void * self, void * job_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Xrm_Job_t *job_ptr_ = (Xrm_Job_t *)job_ptr;
    bool ret_val = false;

    if ((self_ != NULL) && (job_ptr_ != NULL) && 
        (Dl_IsNodeInList(&self_->job_list, &job_ptr_->node)))
    {
        ret_val = true;
    }

    return ret_val;
}

/*! \brief  Search for the next resource object to process.
 *  \param  self    Instance pointer
 *  \return \c true if no error occurred, otherwise \c false.
 */
bool Xrm_SearchNextResourceObject(CExtendedResourceManager *self)
{
    uint16_t tmp_resource_handle;
    bool ret_val = true;

    while(*self->current_obj_pptr != NULL)
    {
        if(Xrm_IsDefaultCreatedPort(self, *self->current_obj_pptr) != false)
        {
            self->current_obj_pptr++;
        }
        else
        {
            tmp_resource_handle = Xrm_GetResourceHandle(self, NULL, *self->current_obj_pptr, &Xrm_IsInMyJobsList);
            if(tmp_resource_handle == XRM_INVALID_RESOURCE_HANDLE)
            {
                break;
            }
            else
            {
                if(Xrm_GetResourceHandle(self, self->current_job_ptr, *self->current_obj_pptr, NULL) == XRM_INVALID_RESOURCE_HANDLE)
                {
                    if(Xrm_StoreResourceHandle(self, tmp_resource_handle, self->current_job_ptr, *self->current_obj_pptr) == false)
                    {
                        self->report_result.code = UCS_XRM_RES_ERR_CONFIG;
                        Xrm_HandleError(self);
                        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Misconfiguration. Resource handle list is too small.", 0U));
                        ret_val = false;
                    }
                }
                self->current_obj_pptr++;
            }
        }
    }

    return ret_val;
}

/*! \brief  Process the next INIC resource object in the resource object list of the current job.
 *  \param  self    Instance pointer
 */
void Xrm_ProcessJob(CExtendedResourceManager *self)
{
    if(Xrm_SearchNextResourceObject(self) != false)
    {
        if(*self->current_obj_pptr != NULL)
        {
            if (Xrm_IsCurrDeviceAlreadyAttached(self) == false)
            {
                (void)Xrm_RemoteDeviceAttach(self, XRM_EVENT_PROCESS);
            }
            else
            {
                switch(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr))
                {
                    case UCS_XRM_RC_TYPE_MOST_SOCKET:
                        Xrm_CreateMostSocket(self);
                        break;
                    case UCS_XRM_RC_TYPE_MLB_PORT:
                        Xrm_CreateMlbPort(self);
                        break;
                    case UCS_XRM_RC_TYPE_MLB_SOCKET:
                        Xrm_CreateMlbSocket(self);
                        break;
                    case UCS_XRM_RC_TYPE_USB_PORT:
                        Xrm_CreateUsbPort(self);
                        break;
                    case UCS_XRM_RC_TYPE_USB_SOCKET:
                        Xrm_CreateUsbSocket(self);
                        break;
                    case UCS_XRM_RC_TYPE_RMCK_PORT:
                        Xrm_CreateRmckPort(self);
                        break;
                    case UCS_XRM_RC_TYPE_STRM_PORT:
                        Xrm_CreateStreamPort(self);
                        break;
                    case UCS_XRM_RC_TYPE_STRM_SOCKET:
                        Xrm_CreateStreamSocket(self);
                        break;
                    case UCS_XRM_RC_TYPE_SYNC_CON:
                        Xrm_CreateSyncCon(self);
                        break;
                    case UCS_XRM_RC_TYPE_DFIPHASE_CON:
                        Xrm_CreateDfiPhaseCon(self);
                        break;
                    case UCS_XRM_RC_TYPE_COMBINER:
                        Xrm_CreateCombiner(self);
                        break;
                    case UCS_XRM_RC_TYPE_SPLITTER:
                        Xrm_CreateSplitter(self);
                        break;
                    case UCS_XRM_RC_TYPE_AVP_CON:
                        Xrm_CreateAvpCon(self);
                        break;
                    case UCS_XRM_RC_TYPE_QOS_CON:
                        Xrm_CreateQoSCon(self);
                        break;
                    default:
                        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Unexpected Resource Type: 0x%02X", 1U, *(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr)));
                        self->report_result.code = UCS_XRM_RES_ERR_CONFIG;
                        Xrm_HandleError(self);
                        break;
                }
            }
        }
        else
        {
            Xrm_FinishJob(self);
        }
    }
}

/*! \brief  Checks if the given resource object is from type "Default Created Port".
 *  \param  self                    Instance pointer
 *  \param  resource_object_ptr     Reference to the resource object
 *  \return Returns \c true if resource object is from type "Default Created Port", otherwise \c false.
 */
bool Xrm_IsDefaultCreatedPort(CExtendedResourceManager *self, UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr)
{
    MISC_UNUSED(self);
    return (*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(resource_object_ptr) == UCS_XRM_RC_TYPE_DC_PORT);
}

/*! \brief  Stores the given resource handle in the resource handle list.
 *  \param  self                Instance pointer
 *  \param  resource_handle     Resource handle to save
 *  \param  job_ptr             Reference to job
 *  \param  resource_object_ptr Reference to resource object
 *  \return \c true if free slot in handle list was found, otherwise \c false
 */
bool Xrm_StoreResourceHandle(CExtendedResourceManager *self,
                             uint16_t resource_handle,
                             Xrm_Job_t *job_ptr,
                             UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr)
{
    return Xrmp_StoreResourceHandle(self->xrmp_ptr, resource_handle, job_ptr, resource_object_ptr);
}

/*! \brief  Retrieves the resource handle identified by the given job reference and the given
 *          resource object reference.
 *  \param  self                    Instance pointer
 *  \param  job_ptr                 Reference to the job. Use NULL as wildcard.
 *  \param  resource_object_ptr     Reference to the resource object
 *  \param  func_ptr                Reference to a function that checks if found jobs by XRMP belongs to our own job list
 *  \return Resource handle if handle was found, otherwise XRM_INVALID_RESOURCE_HANDLE.
 */
uint16_t Xrm_GetResourceHandle(CExtendedResourceManager *self,
                               Xrm_Job_t *job_ptr,
                               UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr, Xrmp_CheckJobListFunc_t func_ptr)
{
    return Xrmp_GetResourceHandle(self->xrmp_ptr, job_ptr, resource_object_ptr, func_ptr, self);
}

/*! \brief  Checks for the resource handle in the given resource handle list and counts It if found.
 *  \param  resrc_ptr       Reference to the resource handle list to be looked for.
 *  \param  job_ptr         Reference to the job list to be looked for.
 *  \param  param_ptr       Reference to the user parameter.
 *  \param  user_arg        Reference to the user argument.
 *  \return \c false to continue the for-each-loop of the resources list queue.
 */
bool Xrm_IncrResHandleEntryCnt (void *resrc_ptr, void *job_ptr, void *param_ptr, void * user_arg)
{
    Xrm_ResourceHandleListItem_t * resrc_ptr_ = (Xrm_ResourceHandleListItem_t *)resrc_ptr;
    Xrm_Job_t * job_ptr_ = (Xrm_Job_t *)job_ptr;
    Xrm_CntEntriesResHandle_t * param_ptr_ = (Xrm_CntEntriesResHandle_t *)param_ptr;
    MISC_UNUSED(user_arg);

    if((resrc_ptr_->resource_handle != XRM_INVALID_RESOURCE_HANDLE) &&
       (resrc_ptr_->job_ptr == job_ptr_) &&
       (resrc_ptr_->resource_object_ptr == param_ptr_->resource_object_ptr))
    {
        (*param_ptr_->cnt_res)++;
    }

    return false;
}

/*! \brief  Finds the resource handle to be counted in my job list and pass it to the record callback function .
 *  \param  job_ptr                 Reference to the job to be looked for.
 *  \param  param_ptr               Reference to the user parameter.
 *  \return \c false to continue the for-each-loop of the job_list queue
 */
bool Xrm_CntResHandleEntries(void * job_ptr, void * param_ptr)
{
    Xrm_CntEntriesResHandle_t * param_ptr_ = (Xrm_CntEntriesResHandle_t *)param_ptr;

    Xrmp_Foreach(param_ptr_->xrm_inst->xrmp_ptr, &Xrm_IncrResHandleEntryCnt, job_ptr, param_ptr_, NULL);

    return false;
}

/*! \brief  Retrieves the number of list entries that uses the given resource handle.
 *  \param  self                    Instance pointer
 *  \param  resource_object_ptr     Reference to the current resource object
 *  \return Number of list entries
 */
uint8_t Xrm_CountResourceHandleEntries(CExtendedResourceManager *self,
                                       UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr)
{
    uint8_t ret_val = 0U;
    Xrm_CntEntriesResHandle_t cnt_entry_param;
    cnt_entry_param.xrm_inst = self;
    cnt_entry_param.cnt_res = &ret_val;
    cnt_entry_param.resource_object_ptr = resource_object_ptr;

    (void)Dl_Foreach(&self->job_list, &Xrm_CntResHandleEntries, &cnt_entry_param);

    return ret_val;
}

/*! \brief  Releases the given resource handle.
 *  \param  resrc_ptr       Reference to the resource handle list to be looked for.
 *  \param  job_ptr         Reference to the job list to be looked for.
 *  \param  resrc_obj_pptr  Reference to the resource object to be looked for.
 *  \param  user_arg         Reference to the user argument
 *  \return \c true to stop the foreach loop when the resource handle has been found, otherwise \c false
 */
bool Xrm_ReleaseResrcHandle(void *resrc_ptr, void *job_ptr, void *resrc_obj_pptr, void * user_arg)
{
    bool ret_val = false;
    Xrm_ResourceHandleListItem_t * resrc_ptr_ = (Xrm_ResourceHandleListItem_t *)resrc_ptr;
    Xrm_Job_t * job_ptr_ = (Xrm_Job_t *)job_ptr;
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *resrc_obj_ptr_ = *(UCS_XRM_CONST Ucs_Xrm_ResObject_t **)resrc_obj_pptr;
    MISC_UNUSED(user_arg);

    if((resrc_ptr_->job_ptr == job_ptr_) &&
        (resrc_ptr_->resource_object_ptr == resrc_obj_ptr_))
    {
        resrc_ptr_->resource_handle = XRM_INVALID_RESOURCE_HANDLE;
        resrc_ptr_->job_ptr = NULL;
        resrc_ptr_->resource_object_ptr = NULL;
        ret_val = true;
    }

    return ret_val;
}

/*! \brief  Releases the given resource handle. Frees the corresponding table row.
 *  \param  self                    Instance pointer
 *  \param  job_ptr                 Reference to the job
 *  \param  resource_object_ptr     Reference to the resource object
 */
void Xrm_ReleaseResourceHandle(CExtendedResourceManager *self,
                               Xrm_Job_t *job_ptr,
                               UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr)
{
    void * resource_object_pptr = (void *)&resource_object_ptr;
    Xrmp_Foreach(self->xrmp_ptr, &Xrm_ReleaseResrcHandle, job_ptr, resource_object_pptr, NULL);
}

/*! \brief  Releases the given resource and sets the notification to \c true.
 *  \param  resrc_ptr       Reference to the resource handle list to be looked for.
 *  \param  resrc_handle    Reference to the resource handle to be found.
 *  \param  job_ptr         Reference to the job to be looked for.
 *  \param  user_arg        Reference to a user argument.
 *  \return \c false to continue the for-each-loop of the resources list table
 */
bool Xrm_FreeResrcHandleAndNtf(void *resrc_ptr, void *resrc_handle, void *job_ptr, void * user_arg)
{
    Xrm_ResourceHandleListItem_t * resrc_ptr_ = (Xrm_ResourceHandleListItem_t *)resrc_ptr;
    uint16_t * resrc_handle_ = (uint16_t *)resrc_handle;
    Xrm_Job_t * job_ptr_ = (Xrm_Job_t *)job_ptr;
    CExtendedResourceManager *self = (CExtendedResourceManager *) user_arg;

    if((resrc_ptr_->resource_handle == *resrc_handle_) &&
       (*resrc_handle_ != XRM_INVALID_RESOURCE_HANDLE) &&
       ((resrc_ptr_->job_ptr == job_ptr_) || 
       (Dl_IsNodeInList(&self->job_list, &resrc_ptr_->job_ptr->node))))
    {
        resrc_ptr_->job_ptr->notify = true;
        resrc_ptr_->job_ptr->valid = false;
        resrc_ptr_->resource_handle = XRM_INVALID_RESOURCE_HANDLE;
        resrc_ptr_->job_ptr = NULL;

        if (self->res_debugging_fptr != NULL)
        {
            self->res_debugging_fptr(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(resrc_ptr_->resource_object_ptr),
                resrc_ptr_->resource_object_ptr, UCS_XRM_INFOS_DESTROYED, self->current_job_ptr->user_arg, self->base_ptr->ucs_user_ptr);
        }

        resrc_ptr_->resource_object_ptr = NULL;
    }

    return false;
}

/*! \brief  Releases all given resource handles. Frees the corresponding table rows. Marks the
 *          corresponding job(s) as invalid and sets the notification flag.
 *  \param  self                        Instance pointer
 *  \param  job_ptr                     Reference to the job. Use NULL as wildcard.
 *  \param  resource_handle_list        Resource handle list
 *  \param  resource_handle_list_size   Size of list resource_handle_list[]
 *  \param  failed_resource_handle      This parameter can be used to specify where the release
 *                                      process has to be stopped. All resource handles prior to
 *                                      the failed handle are released. If this feature is not
 *                                      used \c failed_resource_handle must be set to 
 *                                      \ref XRM_INVALID_RESOURCE_HANDLE.
 *  \return the index of the resource where the release process has stopped.
 */
uint8_t Xrm_ReleaseResourceHandles(CExtendedResourceManager *self,
                                   Xrm_Job_t *job_ptr,
                                   uint16_t resource_handle_list[],
                                   uint8_t resource_handle_list_size,
                                   uint16_t failed_resource_handle)
{
    uint8_t i;

    for(i=0U; i<resource_handle_list_size; i++)
    {
        if((failed_resource_handle != XRM_INVALID_RESOURCE_HANDLE) &&
           (resource_handle_list[i] == failed_resource_handle))
        {
            break;
        }

        Xrmp_Foreach(self->xrmp_ptr, &Xrm_FreeResrcHandleAndNtf, &resource_handle_list[i], job_ptr, self);
    }

    return i;
}

/*! \brief  Releases all resource handles created on remote devices. Frees the corresponding table rows. Marks the
 *          corresponding job(s) as invalid and sets the notification flag.
 *  \param  self       Instance pointer
 */
void Xrm_ReleaseResrcHandles(CExtendedResourceManager *self)
{
    if(Xrm_IsApiFree(self) != false)
    {
        Xrm_ApiLocking(self, true);

        Xrm_MarkResrcAndJobsAsInvalid(self);
        Xrm_NotifyInvalidJobs(self);
        Xrm_ApiLocking(self, false);
    }
    else
    {
        self->queued_event_mask |= XRM_EVENT_NOTIFY_AUTO_DEST_RESR;
    }
}

/*! \brief  Handles and reports Extended Resource Manager errors.
 *  \param  self    Instance pointer
 */
void Xrm_HandleError(CExtendedResourceManager *self)
{
    self->current_job_ptr->valid = false;
    self->current_job_ptr->notify = false;
    self->current_job_ptr->report_fptr(Inic_GetTargetAddress(self->inic_ptr), XRM_INVALID_CONNECTION_LABEL, self->report_result, self->current_job_ptr->user_arg);
    Xrm_ApiLocking(self, false);
    /* Execute the queued events */
    if (self->queued_event_mask > 0U)
    {
        Srv_SetEvent(&self->xrm_srv, self->queued_event_mask);
        self->queued_event_mask = 0U;
    }
}

/*! \brief  Reports result of automatically destroyed resources
 *  \param  self    Instance pointer
 */
void Xrm_ReportAutoDestructionResult(CExtendedResourceManager *self)
{
    MISC_MEM_SET(&self->report_result, 0x00, sizeof(Ucs_Xrm_Result_t));
    self->report_result.code = UCS_XRM_RES_RC_AUTO_DESTROYED;
    Xrm_NotifyInvalidJobs(self);
    Xrm_ApiLocking(self, false);
    /* Execute the queued events */
    if (self->queued_event_mask > 0U)
    {
        Srv_SetEvent(&self->xrm_srv, self->queued_event_mask);
        self->queued_event_mask = 0U;
    }
}

/*! \brief  Reports result of resource destruction for a specific XRM job
 *  \param  self    Instance pointer
 */
void Xrm_ReportJobDestructionResult(CExtendedResourceManager *self)
{
    MISC_MEM_SET(&self->report_result, 0x00, sizeof(Ucs_Xrm_Result_t));
    self->report_result.code = UCS_XRM_RES_SUCCESS_DESTROY;
    self->current_job_ptr->notify = true;
    Xrm_NotifyInvalidJobs(self);
    Xrm_ApiLocking(self, false);
    /* Execute the queued events */
    if (self->queued_event_mask > 0U)
    {
        Srv_SetEvent(&self->xrm_srv, self->queued_event_mask);
        self->queued_event_mask = 0U;
    }
}

/*! \brief  Reports the conclusion of Extended Resource Manager jobs.
 *  \param  self    Instance pointer
 */
void Xrm_FinishJob(CExtendedResourceManager *self)
{
    MISC_MEM_SET(&self->report_result, 0x00, sizeof(Ucs_Xrm_Result_t));
    self->report_result.code = UCS_XRM_RES_SUCCESS_BUILD;
    self->current_job_ptr->report_fptr(Inic_GetTargetAddress(self->inic_ptr), self->current_job_ptr->connection_label, self->report_result, self->current_job_ptr->user_arg);
    Xrm_ApiLocking(self, false);
    /* Execute the queued events */
    if (self->queued_event_mask > 0U)
    {
        Srv_SetEvent(&self->xrm_srv, self->queued_event_mask);
        self->queued_event_mask = 0U;
    }
}

/*! \brief  Marks the given resource as invalid and sets the notification.
 *  \param  resrc_ptr   Reference to the resource handle list to be looked for.
 *  \param  xrm_inst    Reference to the XRM instance to be looked for.
 *  \param  ud_ptr2     Optional reference to the user data 2. Not used !
 *  \param  ud_ptr3     Optional reference to the user data 3. Not used !
 *  \return \c false to continue the for-each-loop of the job_list queue
 */
bool Xrm_MarkThisResrcAsInvalid (void *resrc_ptr, void * xrm_inst, void *ud_ptr2, void *ud_ptr3)
{
    Xrm_ResourceHandleListItem_t * resrc_ptr_ = (Xrm_ResourceHandleListItem_t *)resrc_ptr;
    CExtendedResourceManager * xrm_inst_ = (CExtendedResourceManager *)xrm_inst;
    MISC_UNUSED(ud_ptr2);
    MISC_UNUSED(ud_ptr3);

    if (Dl_IsNodeInList(&xrm_inst_->job_list, &resrc_ptr_->job_ptr->node))
    {
        if (resrc_ptr_->job_ptr->valid == true)
        {
            resrc_ptr_->job_ptr->valid  = false;
            resrc_ptr_->job_ptr->notify = true;
        }
        
        /* Inform monitor callback function */
        if (xrm_inst_->res_debugging_fptr != NULL)
        {
            xrm_inst_->res_debugging_fptr(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(resrc_ptr_->resource_object_ptr),
                                 resrc_ptr_->resource_object_ptr, UCS_XRM_INFOS_DESTROYED, xrm_inst_->current_job_ptr->user_arg, xrm_inst_->base_ptr->ucs_user_ptr);
        }

        resrc_ptr_->resource_handle = XRM_INVALID_RESOURCE_HANDLE;
        resrc_ptr_->job_ptr = NULL;
        resrc_ptr_->resource_object_ptr = NULL;
    }

    return false;
}

/*! \brief  Marks all jobs on remote devices as "invalid".
 *  \param  self    Instance pointer
 */
void Xrm_MarkResrcAndJobsAsInvalid (CExtendedResourceManager *self)
{
    Xrmp_Foreach(self->xrmp_ptr, &Xrm_MarkThisResrcAsInvalid, self, NULL, NULL);

    self->report_result.code = UCS_XRM_RES_RC_AUTO_DESTROYED;
}

/*! \brief  Calls the result callbacks of jobs that were marked as invalid.
 *  \param  job_ptr    Reference to the job to be looked for.
 *  \param  xrm_inst   XRM Instance pointer.
 *  \return \c false to continue the for-each-loop of the job_list queue
 */
bool Xrm_SetJobAsInvalid(void * job_ptr, void * xrm_inst)
{
    Xrm_Job_t *job_ptr_ = (Xrm_Job_t *)job_ptr;
    CExtendedResourceManager * xrm_inst_ = (CExtendedResourceManager *)xrm_inst;

    if(job_ptr_->notify != false)
    {
        job_ptr_->report_fptr(Inic_GetTargetAddress(xrm_inst_->inic_ptr), job_ptr_->connection_label, xrm_inst_->report_result, job_ptr_->user_arg);
        job_ptr_->notify = false;
    }

    return false;
}

/*! \brief  Calls the result callbacks of jobs that were marked as invalid.
 *  \param  self    Instance pointer
 */
void Xrm_NotifyInvalidJobs(CExtendedResourceManager *self)
{
    (void)Dl_Foreach(&self->job_list, &Xrm_SetJobAsInvalid, self);
}

/*! \brief  Sets the monitoring callback for XRM resources.
 *  \param  self        Reference to the XRM Instance to be looked for.
 *  \param  dbg_cb_fn   Debug callback function to set.
 */
void Xrm_SetResourceDebugCbFn(CExtendedResourceManager *self, Ucs_Xrm_ResourceDebugCb_t dbg_cb_fn)
{
    if ((self != NULL) && (dbg_cb_fn != NULL))
    {
        self->res_debugging_fptr = dbg_cb_fn;
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

