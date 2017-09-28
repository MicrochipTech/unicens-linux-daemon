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
 * \brief Implementation of the Sync Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_RSM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rsm.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the RSM service used by scheduler */
static const uint8_t RSM_SRV_PRIO = 250U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! \brief Event for processing one of the below tasks */
static const Srv_Event_t RSM_EVENT_PROCESS_DEV           = 0x01U;
/*! \brief Event for signaling the SyncLost event */
static const Srv_Event_t RSM_EVENT_SIG_SYNCLOST          = 0x02U;

/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Mask for the Network NodeAddress Info */
static const uint32_t RSM_MASK_NETWORK_NODE_ADDRESS = 0x0010U;

/*! \brief Invalid device node address */
static const uint16_t RSM_INVALID_NODE_ADDRESS = 0x0000U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Rsm_Service (void *self);
static void Rsm_ProcessJob (CRemoteSyncManagement *self);
static Ucs_Return_t Rsm_SendSync (CRemoteSyncManagement *self);
static Ucs_Return_t Rsm_RequestNtfDevId (CRemoteSyncManagement *self);
static Ucs_Return_t Rsm_ClearLastNtfDevId (CRemoteSyncManagement *self);
static Ucs_Return_t Rsm_SetNotificationAll (CRemoteSyncManagement * self);
static Ucs_Return_t Rsm_SetNotificationGpio (CRemoteSyncManagement * self);
static Ucs_Return_t Rsm_ProcessDeviceJob (CRemoteSyncManagement *self);
static bool Rsm_IsLocal (CRemoteSyncManagement * self);
static void Rsm_MnsInitSucceededCb(void *self, void *event_ptr);
static void Rsm_MnsNwStatusInfosCb(void *self, void *event_ptr);
static void Rsm_SyncResultCb(void *self, void *result_ptr);
static void Rsm_MsgObjAvailCb(void *self, void *result_ptr);
static void Rsm_SignalSyncCompleted (CRemoteSyncManagement * self);
static void Rsm_SignalSyncError (CRemoteSyncManagement * self);
static void Rsm_SignalSyncLost (CRemoteSyncManagement * self);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CRemoteSyncManager                                                     */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Remote Sync Manager class.
 *  \param self        Instance pointer
 *  \param init_ptr    init data_ptr
 */
void Rsm_Ctor(CRemoteSyncManagement *self, Rsm_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CRemoteSyncManagement));

    /* Init all instances */
    self->base_ptr = init_ptr->base_ptr;
    self->inic_ptr = init_ptr->inic_ptr;
    self->net_ptr  = init_ptr->net_ptr;

    /* Init observer */
    Mobs_Ctor(&self->event_param.ucsinit_observer, self, EH_E_INIT_SUCCEEDED, &Rsm_MnsInitSucceededCb);
    Sobs_Ctor(&self->event_param.stdresult_observer, self, &Rsm_SyncResultCb);
    Obs_Ctor(&self->event_param.txavailability_observer, self, &Rsm_MsgObjAvailCb);

    /* Init event_param variables */
    self->event_param.own_device_address = RSM_INVALID_NODE_ADDRESS;

    /* Initialize Sync Management service */
    Srv_Ctor(&self->rsm_srv, RSM_SRV_PRIO, self, &Rsm_Service);
    /* Add RSM service to scheduler */
    (void)Scd_AddService(&self->base_ptr->scd, &self->rsm_srv);
    /* Add Observer for MNS initialization Result */
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_param.ucsinit_observer);
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Add an observer to the SyncLost Event subject
 *  \param self   Instance pointer
 *  \param obs    init data_ptr
 */
void Rsm_AddObserver(CRemoteSyncManagement * self, CObserver * obs)
{
    (void)Sub_AddObserver(&self->event_param.subject, obs);   
}

/*! \brief Removes an observer registered by Rsm_AddObserver
 *  \param self       Instance pointer
 *  \param obs_ptr    observer to be removed
 */
void Rsm_DelObserver(CRemoteSyncManagement * self, CObserver * obs_ptr)
{
    (void)Sub_RemoveObserver(&self->event_param.subject, obs_ptr);  
}

/*! \brief Synchronizes to the given device
 *  \param self                  Instance pointer
 *  \param user_data             reference to the user data that'll be attached in the sync_complete_fptr callback function 
 *  \param sync_complete_fptr    result callback function to the device to be synchronized
 *  \return Possible return values are
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked. 
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_PARAM parameter exceeds its admissible range
 */
Ucs_Return_t Rsm_SyncDev(CRemoteSyncManagement * self, void* user_data, Rsm_ResultCb_t sync_complete_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_API_LOCKED;

    if (self->dev_infos.sync_state != RSM_DEV_SYNCING)
    {
        self->dev_infos.curr_user_data   = user_data;
        self->dev_infos.curr_res_cb_fptr = sync_complete_fptr;
        if (self->dev_infos.sync_state == RSM_DEV_SYNCED)
        {
            self->dev_infos.next_st = RSM_ST_SYNC_SUCC;
            Srv_SetEvent(&self->rsm_srv, RSM_EVENT_PROCESS_DEV);
            result = UCS_RET_SUCCESS;
        }
        else
        {
            if (Rsm_IsLocal(self))
            {
                self->dev_infos.next_st = RSM_ST_NTF_GPIO;
            }
            else
            {
                self->dev_infos.next_st = RSM_ST_SYNC_REQ;
            }
            result = Rsm_ProcessDeviceJob (self);
        }
    }

    return result;
}

/*! \brief Returns the state (ready or busy) of the given device.
 *  \param self   Instance pointer.
 *  \return state of the given device.
 */
Rsm_DevSyncState_t Rsm_GetDevState(CRemoteSyncManagement * self)
{
    return self->dev_infos.sync_state;
}

/*! \brief Reports SyncLost for the given RSM instance.
 *  \param self     Reference to the instance ptr
 */
void Rsm_ReportSyncLost (CRemoteSyncManagement * self)
{
    self->last_synclost_cause = RSM_SLC_CFGNOTOK;
    Srv_SetEvent(&self->rsm_srv, RSM_EVENT_SIG_SYNCLOST);
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Service function of the Sync management.
 *  \param self    Instance pointer
 */
static void Rsm_Service (void *self)
{
    CRemoteSyncManagement *self_ = (CRemoteSyncManagement *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->rsm_srv, &event_mask);

    /* Handle event to process jobs within devices */
    if((event_mask & RSM_EVENT_PROCESS_DEV) == RSM_EVENT_PROCESS_DEV)
    {
        Srv_ClearEvent(&self_->rsm_srv, RSM_EVENT_PROCESS_DEV);
        Rsm_ProcessJob(self_);
    }

    /* Handle event to signal "SyncLost" */
    if((event_mask & RSM_EVENT_SIG_SYNCLOST) == RSM_EVENT_SIG_SYNCLOST)
    {
        Srv_ClearEvent(&self_->rsm_srv, RSM_EVENT_SIG_SYNCLOST);
        Rsm_SignalSyncLost(self_);
    }
}

/*! \brief  Processes the next job, if available, in a device.
 *  \param  self        Instance pointer
 */
static void Rsm_ProcessJob (CRemoteSyncManagement *self)
{
    if (self->dev_infos.next_st != RSM_ST_IDLE)
    {
        (void)Rsm_ProcessDeviceJob(self);
    }
}

/*! \brief  Processes the next job for the given device.
 *  \param  self       Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_API_LOCKED The INIC API is locked
 *          - \c UCS_RET_ERR_PARAM parameter exceeds its admissible range
 */
static Ucs_Return_t Rsm_ProcessDeviceJob (CRemoteSyncManagement *self)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    switch (self->dev_infos.next_st)
    {
        case RSM_ST_SYNC_REQ:
            result = Rsm_SendSync (self);
            break;

        case RSM_ST_NTF_REQ:
            result = Rsm_RequestNtfDevId (self);
            break;

        case RSM_ST_NTF_CLEAR:
            result = Rsm_ClearLastNtfDevId (self);
            break;

        case RSM_ST_NTF_ALL:
            result = Rsm_SetNotificationAll (self);
            break;

        case RSM_ST_NTF_GPIO:
            result = Rsm_SetNotificationGpio (self);
            break;

        case RSM_ST_SYNC_SUCC:
            Rsm_SignalSyncCompleted (self);
            break;

        case RSM_ST_SYNC_ERR:
            Rsm_SignalSyncError (self);
            break;

        default:
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[RSM]", "Unexpected State Transition: 0x%02X", 1U, (Rsm_StateTransition_t)(self->dev_infos.next_st)));
            break;
    }

    return result;
}

/*! \brief  Sends a Sync command to the given device.
 *  \param  self    Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 */
static Ucs_Return_t Rsm_SendSync (CRemoteSyncManagement *self)
{
    Ucs_Return_t result;

    result = Inic_DeviceSync (self->inic_ptr,
                              &self->event_param.stdresult_observer);

    if(result == UCS_RET_SUCCESS)
    {
        self->dev_infos.sync_state = RSM_DEV_SYNCING;
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RSM]", "Start synchronization to remote device", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->event_param.txavailability_observer);
    }

    return result;
}

/*! \brief  Retrieves the ID of the device that has notified to the INIC.DeviceStatus() FktIDs on the given remote device.
 *  \param  self    Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_API_LOCKED The INIC API is locked
 */
static Ucs_Return_t Rsm_RequestNtfDevId (CRemoteSyncManagement *self)
{
    Ucs_Return_t result;
    uint16_t funcid_devstatus = (uint16_t)0x0220;

    result = Inic_Notification_Get(self->inic_ptr, funcid_devstatus, &self->event_param.stdresult_observer);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RSM]", "DeviceId Request for INIC.DeviceStatus() succeeded", 0U));
    }
    else if (result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->event_param.txavailability_observer);
    }
    else
    {
        Srv_SetEvent(&self->rsm_srv, RSM_EVENT_PROCESS_DEV);
    }

    return result;
}

/*! \brief  Clears the current DevId of all remote functions on the given remote device.
 *  \param  self    Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_PARAM parameter exceeds its admissible range
 */
static Ucs_Return_t Rsm_ClearLastNtfDevId (CRemoteSyncManagement *self)
{
    Ucs_Return_t result;
    Ucs_Inic_NotificationCtrl_t control = UCS_INIC_NTF_CLEAR_ALL;
    Inic_FktIdList_t rm_fktid_list;
    rm_fktid_list.fktids_ptr = NULL;
    rm_fktid_list.num_fktids = 0U;

    result = Inic_Notification_Set(self->inic_ptr, control, self->event_param.own_device_address, rm_fktid_list);
    if(result == UCS_RET_SUCCESS)
    {
        self->dev_infos.next_st = RSM_ST_NTF_ALL;
        Srv_SetEvent(&self->rsm_srv, RSM_EVENT_PROCESS_DEV);
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RSM]", "Clear DevId for all Remote functions succeeded", 0U));
    }
    else if (result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->event_param.txavailability_observer);
    }

    return result;
}

/*! \brief  Sets all notification for the given remote device.
 *  \param  self          Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_PARAM parameter exceeds its admissible range
 */
static Ucs_Return_t Rsm_SetNotificationAll (CRemoteSyncManagement * self)
{
    Ucs_Return_t result;
    Ucs_Inic_NotificationCtrl_t control = UCS_INIC_NTF_SET_FUNC;

    uint16_t funcid_list[2] = {0x0705U, 0x0802U};
    Inic_FktIdList_t rm_fktid_list;
    rm_fktid_list.fktids_ptr = &funcid_list[0];
    rm_fktid_list.num_fktids = 2U;

    result = Inic_Notification_Set(self->inic_ptr, control, self->event_param.own_device_address, rm_fktid_list);
    if(result == UCS_RET_SUCCESS)
    {
        self->dev_infos.next_st = RSM_ST_SYNC_SUCC;
        Srv_SetEvent(&self->rsm_srv, RSM_EVENT_PROCESS_DEV);
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RSM]", "Set Notification for All Remote functions succeeded", 0U));
    }
    else if (result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->event_param.txavailability_observer);
    }

    return result;
}

/*! \brief  Sets Gpio notification for the local device.
 *  \param  self          Instance pointer
 *  \return Possible return values are
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 */
static Ucs_Return_t Rsm_SetNotificationGpio (CRemoteSyncManagement * self)
{
    Ucs_Return_t result;
    /*! \brief GPIO TriggerEvent function id */
    uint16_t RSM_GPIOTREVENT_FUNCID = 0x0705U;
    /*! \brief Local EHC address */
    uint16_t RSM_EHC_ADDRESS = 0x0002U;

    Ucs_Inic_NotificationCtrl_t control = UCS_INIC_NTF_SET_FUNC;
    uint16_t funcid_resmonitor = RSM_GPIOTREVENT_FUNCID;
    Inic_FktIdList_t rm_fktid_list;
    rm_fktid_list.fktids_ptr = &funcid_resmonitor;
    rm_fktid_list.num_fktids = 1U;

    result = Inic_Notification_Set(self->inic_ptr, control, RSM_EHC_ADDRESS, rm_fktid_list);
    if(result == UCS_RET_SUCCESS)
    {
        self->dev_infos.next_st = RSM_ST_SYNC_SUCC;
        Srv_SetEvent(&self->rsm_srv, RSM_EVENT_PROCESS_DEV);
    }
    else if (result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Inic_AddObsrvOnTxMsgObjAvail(self->inic_ptr, &self->event_param.txavailability_observer);
    }

    return result;
}

/*! \brief  Check whether the given device is local or remote.
 *  \param  self          Instance pointer
 *  \return Returns \c true if the device is local, otherwise \c false.
 */
static bool Rsm_IsLocal (CRemoteSyncManagement *self)
{
    return (Inic_GetTargetAddress(self->inic_ptr) == UCS_ADDR_LOCAL_DEV);
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Called if MNS initialization has been succeeded.
 *  \param  self        Instance pointer
 *  \param  event_ptr  Reference to reported event
 */
static void Rsm_MnsInitSucceededCb(void *self, void *event_ptr)
{
    CRemoteSyncManagement *self_ = (CRemoteSyncManagement *)self;
    MISC_UNUSED(event_ptr);

    /* Remove ucsinit_observer */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->event_param.ucsinit_observer);

    /* Add network status observer */
    Mobs_Ctor(&self_->event_param.nwstatus_observer, self, RSM_MASK_NETWORK_NODE_ADDRESS, &Rsm_MnsNwStatusInfosCb);
    Net_AddObserverNetworkStatus(self_->net_ptr, &self_->event_param.nwstatus_observer);
}

/*! \brief  Result callback for the "Sync Request".
 *  \param  self               Instance pointer
 *  \param  result_ptr         Reference to the result.
 */
static void Rsm_SyncResultCb(void *self, void *result_ptr)
{
    CRemoteSyncManagement *self_ = (CRemoteSyncManagement *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
    Inic_NotificationResult_t * res_inf = NULL;

    if(result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        if (self_->dev_infos.sync_state == RSM_DEV_SYNCING)
        {
            switch (self_->dev_infos.next_st)
            {
                case RSM_ST_SYNC_REQ:
                    self_->dev_infos.next_st = RSM_ST_NTF_REQ;
                    TR_INFO((self_->base_ptr->ucs_user_ptr, "[RSM]", "Remote DeviceSync has been successfully created.", 0U));
                    break;

                case RSM_ST_NTF_REQ:
                    res_inf = (Inic_NotificationResult_t *)result_ptr_->data_info;
                    if (res_inf != NULL)
                    {
                        if (res_inf->device_id == RSM_INVALID_NODE_ADDRESS)
                        {
                            self_->dev_infos.next_st = RSM_ST_NTF_ALL;
                        }
                        else
                        {
                            self_->dev_infos.next_st = RSM_ST_NTF_CLEAR;
                        }
                    }
                    break;

                default:
                    break;
            }
            Srv_SetEvent(&self_->rsm_srv, RSM_EVENT_PROCESS_DEV);
        }
    }
    else
    {
        self_->dev_infos.next_st = RSM_ST_SYNC_ERR;
        self_->dev_infos.curr_result.code = RSM_RES_ERR_SYNC;
        self_->dev_infos.curr_result.details.inic_result = result_ptr_->result;
        if (result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
        {
            self_->dev_infos.curr_result.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
        }
        else
        {
            self_->dev_infos.curr_result.details.tx_result = UCS_MSG_STAT_OK;
        }

        Srv_SetEvent(&self_->rsm_srv, RSM_EVENT_PROCESS_DEV);
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[RSM]", "Synchronization to the remote device failed. Result code: 0x%02X", 1U, result_ptr_->result.code));
        if (result_ptr_->result.info_ptr != NULL)
        {
            TR_ERROR_INIC_RESULT(self_->base_ptr->ucs_user_ptr, "[RSM]", result_ptr_->result.info_ptr, result_ptr_->result.info_size);
        }
    }
}

/*! \brief  Event Callback function for the network status.
 *  \param  self          Instance pointer
 *  \param  event_ptr     Reference to the events
 */
static void Rsm_MnsNwStatusInfosCb(void *self, void *event_ptr)
{
    CRemoteSyncManagement *self_ = (CRemoteSyncManagement *)self;
    Net_NetworkStatusParam_t *result_ptr_ = (Net_NetworkStatusParam_t *)event_ptr;
    bool signal_synclost = false;

    if ((RSM_MASK_NETWORK_NODE_ADDRESS & result_ptr_->change_mask) == RSM_MASK_NETWORK_NODE_ADDRESS)
    {
        if (result_ptr_->node_address != 0xFFFFU)
        {
            if ((self_->event_param.own_device_address != RSM_INVALID_NODE_ADDRESS)  && 
                (result_ptr_->node_address != self_->event_param.own_device_address))
            {
                self_->last_synclost_cause = RSM_SLC_SYSMODIF;
                signal_synclost = true;
            }

            self_->event_param.own_device_address = result_ptr_->node_address;
        }
    }

    if (signal_synclost)
    {
        Srv_SetEvent(&self_->rsm_srv, RSM_EVENT_SIG_SYNCLOST);
    }
}

/*! \brief  Event Callback function that signals that a TxMsgObj is now available.
 *  \param  self          Instance pointer
 *  \param  result_ptr    Reference to the results
 */
static void Rsm_MsgObjAvailCb(void *self, void *result_ptr)
{
    CRemoteSyncManagement *self_ = (CRemoteSyncManagement *)self;
    MISC_UNUSED(result_ptr);

    Srv_SetEvent(&self_->rsm_srv, RSM_EVENT_PROCESS_DEV);

    /* delete observer */
    Inic_DelObsrvOnTxMsgObjAvail(self_->inic_ptr, &self_->event_param.txavailability_observer);
}

/*------------------------------------------------------------------------------------------------*/
/* Notification Functions                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Signals the successful synchronization to a remote device.
 *  \param  self        Instance pointer
 */
static void Rsm_SignalSyncCompleted (CRemoteSyncManagement * self)
{
    Rsm_ResultCb_t tmp_res_cb = self->dev_infos.curr_res_cb_fptr;
    void * data = self->dev_infos.curr_user_data;
    Rsm_Result_t res = {RSM_RES_SUCCESS, {UCS_MSG_STAT_OK, {UCS_RES_SUCCESS, NULL, 0}}};

    self->dev_infos.sync_state = RSM_DEV_SYNCED;
    self->dev_infos.next_st    = RSM_ST_IDLE;
    self->dev_infos.curr_res_cb_fptr = NULL;
    self->dev_infos.curr_user_data   = NULL;

    if (tmp_res_cb != NULL)
    {
        tmp_res_cb(data, res);
    }
}

/*! \brief  Signals that the synchronization to a remote device failed.
 *  \param  self        Instance pointer
 */
static void Rsm_SignalSyncError (CRemoteSyncManagement * self)
{
    Rsm_ResultCb_t tmp_res_cb = self->dev_infos.curr_res_cb_fptr;
    void * data = self->dev_infos.curr_user_data;

    self->dev_infos.sync_state = RSM_DEV_UNSYNCED;
    self->dev_infos.next_st    = RSM_ST_IDLE;
    self->dev_infos.curr_res_cb_fptr = NULL;
    self->dev_infos.curr_user_data   = NULL;

    if (tmp_res_cb != NULL)
    {
        tmp_res_cb(data, self->dev_infos.curr_result);
    }
}

/*! \brief  Signals that the synchronization to a remote device has been lost.
 *  \param  self        Instance pointer
 */
static void Rsm_SignalSyncLost (CRemoteSyncManagement * self)
{
    if ((self->dev_infos.sync_state == RSM_DEV_SYNCED) &&
        (!Rsm_IsLocal(self)))
    {
        self->dev_infos.sync_state = RSM_DEV_UNSYNCED;
        if(Sub_GetNumObservers(&self->event_param.subject) > 0U)
        {
            Sub_Notify(&self->event_param.subject, &self->last_synclost_cause);
        }
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

