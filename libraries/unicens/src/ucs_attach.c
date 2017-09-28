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
 * \brief Implementation of CAttachService class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_ATS
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_attach.h"
#include "ucs_pmevent.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the ATS service used by scheduler */
static const uint8_t ATS_SRV_PRIO = 254U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the ATS service */
static const Srv_Event_t ATS_EVENT_SERVICE = 1U;

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Initialization timeout in milliseconds (t = 3s) */
static const uint16_t ATS_INIT_TIMEOUT = 3000U; /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */

/*------------------------------------------------------------------------------------------------*/
/* Internal definitions                                                                           */
/*------------------------------------------------------------------------------------------------*/
#define ATS_NUM_STATES     11U      /*!< \brief Number of state machine states */
#define ATS_NUM_EVENTS      5U      /*!< \brief Number of state machine events */

/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the attach state machine */
typedef enum Ats_Events_
{
    ATS_E_NIL             = 0,      /*!< \brief NIL Event */
    ATS_E_NEXT            = 1,      /*!< \brief Go to next state */
    ATS_E_RETRY           = 2,      /*!< \brief Retry current action */
    ATS_E_ERROR           = 3,      /*!< \brief An error has been occurred */
    ATS_E_TIMEOUT         = 4       /*!< \brief An timeout has been occurred */

} Ats_Events_t;

/*! \brief States of the attach state machine */
typedef enum Ats_State_
{
    ATS_S_START           =  0,     /*!< \brief Start state */
    ATS_S_PMS_UNSYNC      =  1,     /*!< \brief Initially un-synchronizes all FIFOs */
    ATS_S_PMS_INIT        =  2,     /*!< \brief PMS initialization state */
    ATS_S_VERS_CHK        =  3,     /*!< \brief Version check state */
    ATS_S_INIC_OVHL       =  4,     /*!< \brief INIC overhaul state */
    ATS_S_DEV_ATT_STAGE_1 =  5,     /*!< \brief Device attach state 1 (wait for first condition) */
    ATS_S_DEV_ATT_STAGE_2 =  6,     /*!< \brief Device attach state 2 (wait for second condition) */
    ATS_S_DEV_ATT_STAGE_3 =  7,     /*!< \brief Device attach state 3 (wait for third condition) */
    ATS_S_NW_CONFIG       =  8,     /*!< \brief Retrieve network configuration */
    ATS_S_INIT_CPL        =  9,     /*!< \brief Initialization complete state */
    ATS_S_ERROR           = 10      /*!< \brief Error state */

} Ats_State_t;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Ats_TimeoutCb(void *self);
static void Ats_Service(void *self);
static void Ats_ResetObservers(CAttachService *self);
static void Ats_StartPmsUnsync(void *self);
static void Ats_StartPmsInit(void *self);
static void Ats_StartVersChk(void *self);
static void Ats_StartInicOvhl(void *self);
static void Ats_StartDevAtt(void *self);
static void Ats_StartNwConfig(void *self);
static void Ats_InitCpl(void *self);
static void Ats_HandleInternalErrors(void *self, void *error_code_ptr);
static void Ats_HandleError(void *self);
static void Ats_HandleTimeout(void *self);
static void Ats_InvalidTransition(void *self);
static void Ats_CheckPmsUnsyncResult(void *self, void *result_ptr);
static void Ats_CheckPmsInitResult(void *self, void *result_ptr);
static void Ats_CheckVersChkResult(void *self, void *result_ptr);
static void Ats_CheckNetworkStatusReceived(void *self, void *result_ptr);
static void Ats_CheckDeviceStatusReceived(void *self, void *data_ptr);
static void Ats_CheckDevAttResult(void *self, void *result_ptr);
static void Ats_CheckNwConfigStatus(void *self, void *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t ats_trans_tab[ATS_NUM_STATES][ATS_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
/* |--------------------------------|------------------------------------------------|------------------------------------------------|--------------------------------------|----------------------------------------|
 * |   ATS_E_NIL                    | ATS_E_NEXT                                     | ATS_E_RETRY                                    | ATS_E_ERROR                          | ATS_E_TIMEOUT                          |
 * |--------------------------------|------------------------------------------------|------------------------------------------------|--------------------------------------|----------------------------------------|
 */ 
    { {NULL, ATS_S_START          }, {&Ats_StartPmsUnsync,    ATS_S_PMS_UNSYNC     }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_PMS_UNSYNC     }, {&Ats_StartPmsInit,      ATS_S_PMS_INIT       }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_PMS_INIT       }, {&Ats_StartVersChk,      ATS_S_VERS_CHK       }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_VERS_CHK       }, {&Ats_StartInicOvhl,     ATS_S_INIC_OVHL      }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_INIC_OVHL      }, {&Ats_StartDevAtt,       ATS_S_DEV_ATT_STAGE_1}, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_DEV_ATT_STAGE_1}, {NULL,                   ATS_S_DEV_ATT_STAGE_2}, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_DEV_ATT_STAGE_2}, {NULL,                   ATS_S_DEV_ATT_STAGE_3}, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_DEV_ATT_STAGE_3}, {&Ats_StartNwConfig,     ATS_S_NW_CONFIG      }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_NW_CONFIG      }, {&Ats_InitCpl,           ATS_S_INIT_CPL       }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_HandleError,       ATS_S_ERROR}, {&Ats_HandleTimeout,     ATS_S_ERROR} },
    { {NULL, ATS_S_INIT_CPL       }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR}, {&Ats_InvalidTransition, ATS_S_ERROR} },
    { {NULL, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR          }, {&Ats_InvalidTransition, ATS_S_ERROR}, {&Ats_InvalidTransition, ATS_S_ERROR} }
};

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CAttachService                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the attach service class
 *  \param self        Instance pointer
 *  \param init_ptr    Reference to the initialization data
 */
void Ats_Ctor(CAttachService *self, Ats_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    T_Ctor(&self->timer);
    self->report_result = UCS_INIT_RES_SUCCESS;
    self->init_data = *init_ptr;
    Ssub_Ctor(&self->ats_result_subject, self->init_data.base_ptr->ucs_user_ptr);
    Fsm_Ctor(&self->fsm, self, &(ats_trans_tab[0][0]), ATS_NUM_EVENTS, ATS_S_START);
    /* Initialize ATS service */
    Srv_Ctor(&self->ats_srv, ATS_SRV_PRIO, self, &Ats_Service);
    /* Add ATS service to scheduler */
    (void)Scd_AddService(&self->init_data.base_ptr->scd, &self->ats_srv);
}

/*! \brief Starts the attach process and the initialization timeout.
 *  \param self     Instance pointer
 *  \param obs_ptr  Reference to result observer
 */
void Ats_Start(void *self, CSingleObserver *obs_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    /* Observe internal errors during the attach process */
    Mobs_Ctor(&self_->internal_error_obs, self_, (EH_E_BIST_FAILED | EH_E_SYNC_LOST), &Ats_HandleInternalErrors);
    Eh_AddObsrvInternalEvent(&self_->init_data.base_ptr->eh, &self_->internal_error_obs);
    /* Set first event of attach state machine */
    Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
    Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    /* Start timeout timer used for attach process */
    Tm_SetTimer(&self_->init_data.base_ptr->tm,
                &self_->timer,
                &Ats_TimeoutCb,
                self_,
                ATS_INIT_TIMEOUT,
                0U);
    (void)Ssub_AddObserver(&self_->ats_result_subject, obs_ptr);
}

/*! \brief Timer callback used for initialization timeout.
 *  \param self    Instance pointer
 */
static void Ats_TimeoutCb(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    Fsm_SetEvent(&self_->fsm, ATS_E_TIMEOUT);
    Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
}

/*! \brief Service function of the attach service.
 *  \param self    Instance pointer
 */
static void Ats_Service(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->ats_srv, &event_mask);
    if (ATS_EVENT_SERVICE == (event_mask & ATS_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", (result != FSM_STATE_ERROR));
        MISC_UNUSED(result);
    }
}

/*! \brief Resets all module internal observers.
 *  \param self    Instance pointer
 */
static void Ats_ResetObservers(CAttachService *self)
{
    Eh_DelObsrvInternalEvent(&self->init_data.base_ptr->eh, &self->internal_error_obs);
    Sobs_Ctor(&self->sobs, NULL, NULL);
    Obs_Ctor(&self->obs, NULL, NULL);
    Obs_Ctor(&self->obs2, NULL, NULL);
}

/*------------------------------------------------------------------------------------------------*/
/* State machine actions                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief un-synchronizes PMS and observes PM events
 *  \param self    Instance pointer
 */
static void Ats_StartPmsUnsync(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    Obs_Ctor(&self_->obs, self_, &Ats_CheckPmsUnsyncResult);
    Fifos_ConfigureSyncParams(self_->init_data.fifos_ptr, FIFOS_SYNC_RETRIES, FIFOS_SYNC_TIMEOUT);
    Fifos_Unsynchronize(self_->init_data.fifos_ptr, true, true);
    Fifos_AddEventObserver(self_->init_data.fifos_ptr, &self_->obs);
}

/*! \brief Synchronizes PMS and observes PM events
 *  \param self    Instance pointer
 */
static void Ats_StartPmsInit(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    Obs_Ctor(&self_->obs, self_, &Ats_CheckPmsInitResult);
    Pmev_Start(self_->init_data.pme_ptr);                               /* enables failure reporting to all modules */
    Fifos_Synchronize(self_->init_data.fifos_ptr, false, true);         /* now synchronizes, counter is not reset to "0" */
    Fifos_AddEventObserver(self_->init_data.fifos_ptr, &self_->obs);
}

/*! \brief Starts the request of the INIC firmware and hardware revisions.
 *  \param self    Instance pointer
 */
static void Ats_StartVersChk(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    Sobs_Ctor(&self_->sobs, self_, &Ats_CheckVersChkResult);
    if (Inic_DeviceVersion_Get(self_->init_data.inic_ptr,
                              &self_->sobs) != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "INIC device version check failed!", 0U));
        self_->report_result = UCS_INIT_RES_ERR_BUF_OVERFLOW;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
    }
}

/*! \brief Starts the overhaul process of the INIC.
 *  \param self    Instance pointer
 */
static void Ats_StartInicOvhl(void *self)
{
    CAttachService *self_ = (CAttachService *)self;

    Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
}

/*! \brief Starts the attach process between EHC and INIC.
 *  \param self    Instance pointer
 */
static void Ats_StartDevAtt(void *self)
{
    CAttachService *self_ = (CAttachService *)self;

    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_StartDevAtt()  called", 0U));
    /* Assign observer to monitor the initial receipt of INIC message INIC.MOSTNetworkStatus */
    Obs_Ctor(&self_->obs, self_, &Ats_CheckNetworkStatusReceived);
    Inic_AddObsrvNwStatus(self_->init_data.inic_ptr, &self_->obs);
    /* Assign observer to monitor the initial receipt of INIC message INIC.DeviceStatus */
    Obs_Ctor(&self_->obs2, self_, &Ats_CheckDeviceStatusReceived);
    Inic_AddObsvrDeviceStatus(self_->init_data.inic_ptr, &self_->obs2);

    /* Start device attach process */
    Sobs_Ctor(&self_->sobs, self_, &Ats_CheckDevAttResult);
    if (Inic_DeviceAttach(self_->init_data.inic_ptr, &self_->sobs) != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "INIC device attach failed!", 0U));
        self_->report_result = UCS_INIT_RES_ERR_BUF_OVERFLOW;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
    }
}

/*! \brief Starts request of network configuration property required
 *         to retrieve the own group address.
 *  \param self    Instance pointer
 */
static void Ats_StartNwConfig(void *self)
{
    CAttachService *self_ = (CAttachService *)self;

    /* Assign observer to monitor the initial receipt of INIC message INIC.MOSTNetworkConfigurarion */
    Sobs_Ctor(&self_->sobs, self_, &Ats_CheckNwConfigStatus);

    if (Inic_NwConfig_Get(self_->init_data.inic_ptr, &self_->sobs) != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "INIC network configuration failed!", 0U));
        self_->report_result = UCS_INIT_RES_ERR_BUF_OVERFLOW;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
    }
}

/*! \brief This method is called when the initialization has been completed.
 *  \param self    Instance pointer
 */
static void Ats_InitCpl(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    self_->report_result = UCS_INIT_RES_SUCCESS;
    /* Attach process finished -> Reset observers and terminate state machine */
    Ats_ResetObservers(self_);
    Tm_ClearTimer(&self_->init_data.base_ptr->tm, &self_->timer);
    Fsm_End(&self_->fsm);
    Eh_ReportEvent(&self_->init_data.base_ptr->eh, EH_E_INIT_SUCCEEDED);
    Ssub_Notify(&self_->ats_result_subject, &self_->report_result, true);
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_InitCpl() called", 0U));
}

/*! \brief Handles internal errors during the attach process.
 *  \param self             Instance pointer
 *  \param error_code_ptr   Reference to reported error code
 */
static void Ats_HandleInternalErrors(void *self, void *error_code_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    uint32_t error_code = *((uint32_t *)error_code_ptr);
    switch (error_code)
    {
        case EH_E_SYNC_LOST:
            self_->report_result = UCS_INIT_RES_ERR_INIC_SYNC;
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "A control FiFo synchronization lost!", 0U));
            break;
        case EH_E_BIST_FAILED:
            self_->report_result = UCS_INIT_RES_ERR_INIC_SYSTEM;
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "INIC Build-In-Self-Test failed!", 0U));
            break;
        default:
            self_->report_result = UCS_INIT_RES_ERR_INTERNAL;
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Unknown internal error occurred! Error code: 0x%04X", 1U, error_code));
            break;
    }
    /* Error occurred -> Reset observers and terminate state machine */
    Ats_ResetObservers(self_);
    Tm_ClearTimer(&self_->init_data.base_ptr->tm, &self_->timer);
    Fsm_End(&self_->fsm);
    Eh_ReportEvent(&self_->init_data.base_ptr->eh, EH_E_INIT_FAILED);
    Ssub_Notify(&self_->ats_result_subject, &self_->report_result, true);
}

/*! \brief Handles general errors during the attach process.
 *  \param self    Instance pointer
 */
static void Ats_HandleError(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    /* Error occurred -> Reset observers and terminate state machine */
    Ats_ResetObservers(self_);
    Tm_ClearTimer(&self_->init_data.base_ptr->tm, &self_->timer);
    Fsm_End(&self_->fsm);
    Eh_ReportEvent(&self_->init_data.base_ptr->eh, EH_E_INIT_FAILED);
    Ssub_Notify(&self_->ats_result_subject, &self_->report_result, true);
    TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Error occurred during initialization!", 0U));
}

/*! \brief Handles timeouts during the attach process.
 *  \param self    Instance pointer
 */
static void Ats_HandleTimeout(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    self_->report_result = UCS_INIT_RES_ERR_TIMEOUT;
    /* Error occurred -> Reset observers and terminate state machine */
    Ats_ResetObservers(self_);
    Fsm_End(&self_->fsm);
    Eh_ReportEvent(&self_->init_data.base_ptr->eh, EH_E_INIT_FAILED);
    Ssub_Notify(&self_->ats_result_subject, &self_->report_result, true);
    TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Initialization timeout occurred!", 0U));
}

/*! \brief This method is invoked if an invalid state machine transition is executed.
 *  \param self    Instance pointer
 */
static void Ats_InvalidTransition(void *self)
{
    CAttachService *self_ = (CAttachService *)self;
    self_->report_result = UCS_INIT_RES_ERR_INTERNAL;
    /* Invalid Transition -> Reset observers and terminate state machine */
    Ats_ResetObservers(self_);
    Tm_ClearTimer(&self_->init_data.base_ptr->tm, &self_->timer);
    Fsm_End(&self_->fsm);
    Eh_ReportEvent(&self_->init_data.base_ptr->eh, EH_E_INIT_FAILED);
    Ssub_Notify(&self_->ats_result_subject, &self_->report_result, true);
    TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Invalid transition within ATS state machine!", 0U));
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of the observer results                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result callback for action "PMS Initialization". This function is part of an
 *         observer object and is invoked by Sub_Notify().
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the received PMS event. The pointer must be casted into 
 *                     data type Fifos_Event_t.
 */
static void Ats_CheckPmsUnsyncResult(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Fifos_Event_t pms_event = *((Fifos_Event_t *)result_ptr);

    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckPmsUnsyncResult() called", 0U));

    if (pms_event == FIFOS_EV_UNSYNC_COMPLETE)
    {
        Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    }
    else
    {
        self_->report_result = UCS_INIT_RES_ERR_INIC_SYNC;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckPmsUnsyncResult(): un-sync failed, event=0x%02X", 1U, pms_event));
    }
    Fifos_RemoveEventObserver(self_->init_data.fifos_ptr, &self_->obs);
}

/*! \brief Result callback for action "PMS Initialization". This function is part of an
 *         observer object and is invoked by Sub_Notify().
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the received PMS event. The pointer must be casted into 
 *                     data type Fifos_Event_t.
 */
static void Ats_CheckPmsInitResult(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Fifos_Event_t pms_event = *((Fifos_Event_t *)result_ptr);

    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckPmsInitResult() called", 0U));

    if (pms_event == FIFOS_EV_SYNC_ESTABLISHED)
    {
        Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    }
    else
    {
        self_->report_result = UCS_INIT_RES_ERR_INIC_SYNC;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckPmsInitResult(): sync failed, event=0x%02X", 1U, pms_event));
    }
    Fifos_RemoveEventObserver(self_->init_data.fifos_ptr, &self_->obs);
}

/*! \brief Result callback for action "Version Check". This function is part of a single
 *         observer object and is invoked by Ssub_Notify().
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the received version check result. The pointer must be casted
 *                     into data type Inic_StdResult_t.
 */
static void Ats_CheckVersChkResult(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    }
    else
    {
        self_->report_result = UCS_INIT_RES_ERR_INIC_VERSION;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "INIC version check failed!", 0U));
    }
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckVersChkResult() called", 0U));
}

/*! \brief Result callback which handles one of three conditions for action "Device Attach". The 
 *         function is called if INIC message INIC.MOSTNetworkStatus was received. The function is 
 *         part of an observer object and is invoked by Sub_Notify(). The property 
 *         INIC.MOSTNetworkStatus.Status() is notified. Thus, there is no error condition available.
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the MOST Network Status. The pointer must be casted into data 
 *                     type Inic_StdResult_t.
 */
static void Ats_CheckNetworkStatusReceived(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Inic_DelObsrvNwStatus(self_->init_data.inic_ptr, &self_->obs);
    Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
    Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    MISC_UNUSED(result_ptr);
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckNetworkStatusReceived() called", 0U));
}

/*! \brief Observer callback that is notified on received INIC.DeviceStatus
 *  \param self        Instance pointer
 *  \param data_ptr    The pointer to the current INIC.DeviceStatus structure
 */
static void Ats_CheckDeviceStatusReceived(void *self, void *data_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Inic_DelObsvrDeviceStatus(self_->init_data.inic_ptr, &self_->obs2);
    Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
    Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    MISC_UNUSED(data_ptr);
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckDeviceStatusReceived() called", 0U));
}

/*! \brief Result callback which handles one of two conditions for action "Device Attach". The 
 *         function handles the result of the INIC method INIC.DeviceAttach. This function is part 
 *         of a single-observer object and is invoked by Ssub_Notify().
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the received device attach result. The pointer must be casted 
 *                     into data type Inic_StdResult_t.
 */
static void Ats_CheckDevAttResult(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Inic_StdResult_t error_data = *((Inic_StdResult_t *)result_ptr);
    switch (error_data.result.code)
    {
        case UCS_RES_SUCCESS:
            /* Operation succeeded */
            Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
            Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
            break; 
        case UCS_RES_ERR_CONFIGURATION:
            /* Configuration error occurred -> attach process failed! */
            self_->report_result = UCS_INIT_RES_ERR_DEV_ATT_CFG;
            Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
            Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Device attach failed due to an configuration error!", 0U));
            TR_ERROR_INIC_RESULT(self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", error_data.result.info_ptr, error_data.result.info_size);
            break;
        case UCS_RES_ERR_SYSTEM:
            /* INIC is still attached -> attach process failed! */
            self_->report_result = UCS_INIT_RES_ERR_DEV_ATT_PROC;
            Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
            Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "EHC is already attached to the INIC!", 0U));
            break;
        default:
            /* INIC reports an unexpected error -> attach process failed! */
            self_->report_result = UCS_INIT_RES_ERR_DEV_ATT_PROC;
            Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
            Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Device attach failed! Unexpected error code = 0x%02X", 1U, error_data.result.code));
            TR_ERROR_INIC_RESULT(self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", error_data.result.info_ptr, error_data.result.info_size);
            break;
    }
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckDevAttResult() called", 0U));
}

/*! \brief Result callback for INIC network configuration
 *  \param self        Instance pointer
 *  \param result_ptr  Reference to the received network configuration status event. 
 *                     The pointer must be casted into data type Inic_StdResult_t.
 */
static void Ats_CheckNwConfigStatus(void *self, void *result_ptr)
{
    CAttachService *self_ = (CAttachService *)self;
    Inic_StdResult_t error_data = *((Inic_StdResult_t *)result_ptr);

    if (error_data.result.code == UCS_RES_SUCCESS)
    {
        /* Operation succeeded */
        Fsm_SetEvent(&self_->fsm, ATS_E_NEXT);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
    }
    else
    {
        /* INIC reports an unexpected error -> attach process failed! */
        self_->report_result = UCS_INIT_RES_ERR_NET_CFG;
        Fsm_SetEvent(&self_->fsm, ATS_E_ERROR);
        Srv_SetEvent(&self_->ats_srv, ATS_EVENT_SERVICE);
        TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Network configuration failed! Unexpected error code = 0x%02X", 1U, error_data.result.code));
        TR_ERROR_INIC_RESULT(self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", error_data.result.info_ptr, error_data.result.info_size);
    }
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[ATS]", "Ats_CheckNwConfigStatus() called", 0U));
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

