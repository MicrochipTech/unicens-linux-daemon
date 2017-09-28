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
 * \brief Implementation of the UNICENS API.
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_class.h"
#include "ucs_misc.h"
#include "ucs_trace.h"
#include "ucs_ams.h"
#include "ucs_cmd.h"

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \def     UCS_NUM_INSTANCES
 *  \brief   Number of API instances which can be created by function Ucs_CreateInstance().
 *  \details One API instance is used to communicate with one local INIC. In this case the application
 *           is connected to one network.
 *           It is possible access multiple networks by having multiple API instances. Each API instance
 *           requires communication with an exclusive INIC.
 *           Valid values: 1..10. Default Value: 1.
 *  \ingroup G_UCS_INIT_AND_SRV
 */ 
#ifndef UCS_NUM_INSTANCES
# define UCS_NUM_INSTANCES 1
# define UCS_API_INSTANCES 1U                   /* default value */
#elif (UCS_NUM_INSTANCES > 10)
# define UCS_API_INSTANCES 10U
#elif (UCS_NUM_INSTANCES < 1)
# define UCS_API_INSTANCES 1U
#else
# define UCS_API_INSTANCES ((uint8_t)UCS_NUM_INSTANCES)
#endif

/*! \cond UCS_INTERNAL_DOC
 *  \addtogroup G_UCS_CLASS
 *  @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static bool Ucs_CheckInitData(const Ucs_InitData_t *init_ptr);
static void Ucs_Ctor(CUcs* self, uint8_t ucs_inst_id, void *api_user_ptr);
static void Ucs_InitComponents(CUcs* self);
static void Ucs_InitFactoryComponent(CUcs *self);
static void Ucs_InitBaseComponent(CUcs *self);
static void Ucs_InitPmsComponentConfig(CUcs *self);
static void Ucs_InitNetComponent(CUcs *self);
static void Ucs_InitLocalInicComponent(CUcs *self);
static void Ucs_InitRoutingComponent(CUcs *self);
static void Ucs_InitAtsClass(CUcs *self);
static void Ucs_InitExcComponent(CUcs *self);
static void Ucs_InitSysDiagComponent(CUcs *self);
static void Ucs_InitNodeDiscovery(CUcs *self);
static void Ucs_InitBackChannelDiagnosis(CUcs *self);
static void Ucs_InitProgramming(CUcs *self);
static void Ucs_InitManager(CUcs *self);
static void Ucs_InitResultCb(void *self, void *result_ptr);
static void Ucs_UninitResultCb(void *self, void *error_code_ptr);
static void Ucs_OnRxRcm(void *self, Msg_MostTel_t *tel_ptr);
static bool Ucs_OnRxMsgFilter(void *self, Msg_MostTel_t *tel_ptr);
static void Ucs_OnGetTickCount(void *self, void *tick_count_value_ptr);
static void Ucs_OnSetApplicationTimer(void *self, void *new_time_value_ptr);
static void Ucs_OnServiceRequest(void *self, void *result_ptr);
static void Ucs_OnGeneralError(void *self, void *result_ptr);
static void Ucs_Most_PortStatusCb(void *self, void *result_ptr);
static void Ucs_StartAppNotification(CUcs *self);
static void Ucs_StopAppNotification(CUcs *self);
static void Ucs_Inic_OnDeviceStatus(void *self, void *data_ptr);
static void Ucs_NetworkStartupResult(void *self, void *result_ptr);
static void Ucs_NetworkShutdownResult(void *self, void *result_ptr);
static void Ucs_NetworkForceNAResult(void *self, void *result_ptr);
static void Ucs_NetworkFrameCounterResult(void *self, void *result_ptr);
static void Ucs_NetworkStatus(void *self, void *result_ptr);
static void Ucs_InitPmsComponent(CUcs *self);
static void Ucs_InitPmsComponentApp(CUcs *self);
static void Ucs_InitAmsComponent(CUcs *self);
static void Ucs_AmsRx_Callback(void *self);
static void Ucs_AmsTx_FreedCallback(void *self, void *data_ptr);
static bool Ucs_McmRx_FilterCallback(void *self, Msg_MostTel_t *tel_ptr);
static Ucs_Nd_CheckResult_t Ucs_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr);
static void Ucs_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Public Methods                                                                                 */
/*------------------------------------------------------------------------------------------------*/
extern Ucs_Inst_t* Ucs_CreateInstance(void)
{
    static CUcs api_instances[UCS_API_INSTANCES];
    static uint8_t next_index = 0U;
    Ucs_Inst_t *inst_ptr = NULL;

    if (next_index < UCS_API_INSTANCES)
    {
        CUcs *ucs_ptr = &api_instances[next_index];
        ucs_ptr->ucs_inst_id = next_index + 1U;                         /* start with instance id "1" */
        TR_INFO((ucs_ptr->ucs_user_ptr, "[API]", "Ucs_CreateInstance(): returns 0x%p", 1U, ucs_ptr));
        inst_ptr = (Ucs_Inst_t*)(void*)ucs_ptr;                         /* convert API pointer to abstract data type */
        next_index++;
    }
    else
    {
        TR_INFO((0U, "[API]", "Ucs_CreateInstance(): failed!", 0U));
    }

    return inst_ptr;
}


/*------------------------------------------------------------------------------------------------*/
/* Initialization structure                                                                       */
/*------------------------------------------------------------------------------------------------*/
extern Ucs_Return_t Ucs_SetDefaultConfig(Ucs_InitData_t *init_ptr)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;

    if (init_ptr != NULL)
    {
        MISC_MEM_SET(init_ptr, 0, sizeof(*init_ptr));
        /* -- add default values here -- */
        init_ptr->general.inic_watchdog_enabled = true;
        init_ptr->ams.enabled = true;
        init_ptr->network.status.notification_mask = 0xFFFFU;           /* Initialize notification masks for NET callbacks */
        init_ptr->mgr.packet_bw = MGR_PACKET_BW_DEFAULT;
        init_ptr->mgr.enabled = false;
        ret = UCS_RET_SUCCESS;
    }

    TR_INFO((0U, "[API]", "Ucs_SetDefaultConfig(init_ptr: 0x%p): called", 1U, init_ptr));
    return ret;
}

/*! \brief  Checks if the given initialization data is valid
 *  \param  init_ptr    Reference to initialization data
 *  \return Returns \c true if the given initialization data is valid, otherwise \c false.
 */
static bool Ucs_CheckInitData(const Ucs_InitData_t *init_ptr)
{
    bool ret_val = true;

    if ((init_ptr == NULL) ||                                                   /* General NULL pointer checks */
        (init_ptr->general.get_tick_count_fptr == NULL) ||
        (init_ptr->lld.start_fptr == NULL) ||
        (init_ptr->lld.stop_fptr == NULL) ||
        (init_ptr->lld.tx_transmit_fptr == NULL)
      )
    {
        TR_ERROR((0U, "[API]", "Initialization failed. Required initialization data contains a NULL pointer.", 0U));
        ret_val = false;
    }
    else if (((init_ptr->general.set_application_timer_fptr == NULL) && (init_ptr->general.request_service_fptr != NULL)) ||
             ((init_ptr->general.set_application_timer_fptr != NULL) && (init_ptr->general.request_service_fptr == NULL)))
    {
        TR_ERROR((0U, "[API]", "Initialization failed. To run UCS in event driven service mode, both callback functions must be assigned.", 0U));
        ret_val = false;
    }
    else if ((init_ptr->mgr.enabled != false) && ((init_ptr->nd.eval_fptr != NULL) || (init_ptr->nd.report_fptr != NULL)))
    {
        TR_INFO((0U, "[API]", "Ambiguous initialization structure. NodeDiscovery callback functions are not effective if 'mgr.enabled' is 'true'.", 0U));
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Class initialization                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of API. Values are reset, initialization must be triggered via Ucs_Init().
 *  \param self         The instance
 *  \param ucs_inst_id  The ID of the instance
 *  \param api_user_ptr The user reference for API callback functions
 */
static void Ucs_Ctor(CUcs* self, uint8_t ucs_inst_id, void *api_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));                       /* reset memory and backup/restore instance id */
    self->ucs_inst_id = ucs_inst_id;
    self->ucs_user_ptr = api_user_ptr;
}

extern Ucs_Return_t Ucs_Init(Ucs_Inst_t* self, const Ucs_InitData_t *init_ptr, Ucs_InitResultCb_t init_result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;

    /* Note: "self_->ucs_inst_id" is already set to the correct value in Ucs_CreateInstance(), do not overwrite it */
    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_Init(init_ptr: 0x%p): called", 1U, init_ptr));

    if (Ucs_CheckInitData(init_ptr))
    {
        Ucs_Ctor(self_, self_->ucs_inst_id, init_ptr->user_ptr);/* initialize object */
        self_->init_result_fptr = init_result_fptr;             /* backup result callback function */

        self_->init_data = *init_ptr;                           /* backup init data */
        Ucs_InitComponents(self_);                              /* call constructors and link all components */
                                                                /* create init-complete observer */
        Sobs_Ctor(&self_->init_result_obs, self, &Ucs_InitResultCb);
        Ats_Start(&self_->inic.attach, &self_->init_result_obs);/* Start attach process */
        ret = UCS_RET_SUCCESS;
    }
                                                                /* register observer related to Ucs_Stop() */
    Mobs_Ctor(&self_->uninit_result_obs, self, (EH_E_UNSYNC_COMPLETE | EH_E_UNSYNC_FAILED), &Ucs_UninitResultCb);
    return ret;
}

extern void Ucs_Service(Ucs_Inst_t* self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    bool pending_events = false;

    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_Service(): called", 0U));
    Scd_Service(&self_->general.base.scd);                              /* Run the scheduler */
    pending_events = Scd_AreEventsPending(&self_->general.base.scd);    /* Check if events are still pending? */

    if (pending_events != false)                                        /* At least one event is pending? */
    {
        if (self_->general.request_service_fptr != NULL)
        {
            self_->general.request_service_fptr(self_->ucs_user_ptr);   /* Trigger UCS service call immediately */
        }
    }

    Tm_CheckForNextService(&self_->general.base.tm);                    /* If UCS timers are running: What is the next time that  
                                                                         * the timer management must be serviced again? */   
}

extern void Ucs_ReportTimeout(Ucs_Inst_t* self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_ReportTimeout(): called", 0U));
    Tm_TriggerService(&self_->general.base.tm);                         /* Trigger TM service call */
}

extern Ucs_Return_t Ucs_Stop(Ucs_Inst_t* self, Ucs_StdResultCb_t stopped_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_Stop() called", 0U));

    if ((self_->uninit_result_fptr == NULL) && (self_->init_complete != false))
    {
        if (stopped_fptr !=  NULL)
        {
            self_->uninit_result_fptr = stopped_fptr;
            Eh_DelObsrvPublicError(&self_->general.base.eh);
            Eh_AddObsrvInternalEvent(&self_->general.base.eh, &self_->uninit_result_obs);
            ret_val = UCS_RET_SUCCESS;
            Fifos_ConfigureSyncParams(&self_->fifos, FIFOS_UNSYNC_RETRIES, FIFOS_UNSYNC_TIMEOUT);
            Fifos_Unsynchronize(&self_->fifos, true, false);
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;         /* termination is already running */
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Connection Routing Management                                                                  */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Rm_Start(Ucs_Inst_t *self, Ucs_Rm_Route_t *routes_list, uint16_t list_size)
{
    CUcs *self_ = (CUcs*)(void*)self;

    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (self_->init_complete != false)
    {
        ret_val = Rtm_StartProcess (&self_->rtm, routes_list, list_size);
    }

    return ret_val;
}

Ucs_Return_t Ucs_Rm_SetRouteActive (Ucs_Inst_t *self, Ucs_Rm_Route_t *route_ptr, bool active)
{
    CUcs *self_ = (CUcs*)(void*)self;

    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM; 

    if ((self_ != NULL) && (route_ptr != NULL))
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            if (!active)
            {
                ret_val = Rtm_DeactivateRoute(&self_->rtm, route_ptr);
            }
            else
            {
                ret_val = Rtm_ActivateRoute(&self_->rtm, route_ptr);
            }
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Xrm_Stream_SetPortConfig(Ucs_Inst_t *self, 
                                          uint16_t destination_address,
                                          uint8_t index,
                                          Ucs_Stream_PortOpMode_t op_mode,
                                          Ucs_Stream_PortOption_t port_option,
                                          Ucs_Stream_PortClockMode_t clock_mode,
                                          Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                          Ucs_Xrm_Stream_PortCfgResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Xrm_Stream_SetPortConfig(Fac_GetXrmLegacy(&self_->factory, destination_address, self_->init_data.rm.xrm.check_unmute_fptr),
                                               index,
                                               op_mode,
                                               port_option,
                                               clock_mode,
                                               clock_data_delay,
                                               result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Xrm_Stream_GetPortConfig(Ucs_Inst_t *self, uint16_t destination_address, uint8_t index,
                                          Ucs_Xrm_Stream_PortCfgResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Xrm_Stream_GetPortConfig(Fac_GetXrmLegacy(&self_->factory, destination_address, self_->init_data.rm.xrm.check_unmute_fptr),
                                               index, result_fptr);
        }
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Node Management                                                                                */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Rm_SetNodeAvailable(Ucs_Inst_t *self, Ucs_Rm_Node_t *node_ptr, bool available)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if ((self_ != NULL) && (node_ptr != NULL))
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Rtm_SetNodeAvailable(&self_->rtm, node_ptr, available);
        }
    }

    return ret_val;
}

bool Ucs_Rm_GetNodeAvailable (Ucs_Inst_t *self, Ucs_Rm_Node_t *node_ptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    bool ret_val = false;

    if ((self_ != NULL) && (node_ptr != NULL))
    {
        ret_val = Rtm_GetNodeAvailable(&self_->rtm, node_ptr);
    }

    return ret_val;
}

Ucs_Return_t Ucs_Rm_GetAttachedRoutes (Ucs_Inst_t *self, Ucs_Rm_EndPoint_t * ep_inst, 
                                         Ucs_Rm_Route_t * ls_found_routes[], uint16_t ls_size)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Rtm_GetAttachedRoutes(&self_->rtm, ep_inst, ls_found_routes, ls_size);
        }
    }

    return ret_val;
}

uint16_t Ucs_Rm_GetConnectionLabel(Ucs_Inst_t *self, Ucs_Rm_Route_t *route_ptr)
{
    uint16_t ret_value = 0U;
    CUcs *self_ = (CUcs*)(void*)self;

    if ((self_ != NULL) && (self_->init_complete != false) && (route_ptr != NULL))
    {
        ret_value = Rtm_GetConnectionLabel(&self_->rtm, route_ptr);
    }

    return ret_value;
}

/*------------------------------------------------------------------------------------------------*/
/* Node Scripting Management                                                                      */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Ns_Run (Ucs_Inst_t *self, Ucs_Rm_Node_t * node_ptr, Ucs_Ns_ResultCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if ((self_ != NULL) && (node_ptr != NULL) && (node_ptr->signature_ptr != NULL))
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            CNodeScriptManagement * nsm_inst = Fac_GetNsm(&self_->factory, node_ptr->signature_ptr->node_address);

            ret_val = UCS_RET_ERR_NOT_AVAILABLE;
            if (nsm_inst != NULL)
            {
                ret_val = Nsm_Run_Pb(nsm_inst, node_ptr, result_fptr);
            }
        }
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* GPIO and I2C Peripheral Bus Interfaces                                                         */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Gpio_CreatePort(Ucs_Inst_t *self, uint16_t destination_address, uint8_t index, uint16_t debounce_time, Ucs_Gpio_CreatePortResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Gpio_CreatePort(Fac_GetGpio(&self_->factory, destination_address, self_->init_data.gpio.trigger_event_status_fptr), 
                                      index, 
                                      debounce_time, 
                                      result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Gpio_SetPinMode(Ucs_Inst_t *self, uint16_t destination_address, uint16_t gpio_port_handle, 
                                 uint8_t pin, Ucs_Gpio_PinMode_t mode, Ucs_Gpio_ConfigPinModeResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Gpio_SetPinModeConfig(Fac_GetGpio(&self_->factory, destination_address, self_->init_data.gpio.trigger_event_status_fptr),
                                            gpio_port_handle, 
                                            pin,
                                            mode,
                                            result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Gpio_GetPinMode(Ucs_Inst_t *self, uint16_t destination_address, uint16_t gpio_port_handle, Ucs_Gpio_ConfigPinModeResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Gpio_GetPinModeConfig(Fac_GetGpio(&self_->factory, destination_address, self_->init_data.gpio.trigger_event_status_fptr),
                                            gpio_port_handle, 
                                            result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Gpio_WritePort(Ucs_Inst_t *self, uint16_t destination_address, uint16_t gpio_port_handle, 
                                uint16_t mask, uint16_t data, Ucs_Gpio_PinStateResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Gpio_SetPinStateConfig(Fac_GetGpio(&self_->factory, destination_address, self_->init_data.gpio.trigger_event_status_fptr),
                                             gpio_port_handle,
                                             mask, 
                                             data,
                                             result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_Gpio_ReadPort(Ucs_Inst_t *self, uint16_t destination_address, uint16_t gpio_port_handle, Ucs_Gpio_PinStateResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = Gpio_GetPinStateConfig(Fac_GetGpio(&self_->factory, destination_address, self_->init_data.gpio.trigger_event_status_fptr),
                                             gpio_port_handle,
                                             result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_I2c_CreatePort(Ucs_Inst_t *self, uint16_t destination_address, uint8_t index, Ucs_I2c_Speed_t speed, 
                                uint8_t i2c_int_mask, Ucs_I2c_CreatePortResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = I2c_CreatePort(Fac_GetI2c(&self_->factory, destination_address, self_->init_data.i2c.interrupt_status_fptr),
                                     index,
                                     speed,
                                     i2c_int_mask,
                                     result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_I2c_WritePort(Ucs_Inst_t *self, uint16_t destination_address, uint16_t port_handle, Ucs_I2c_TrMode_t mode, uint8_t block_count, 
                               uint8_t slave_address, uint16_t timeout, uint8_t data_len, uint8_t * data_ptr, 
                               Ucs_I2c_WritePortResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = I2c_WritePort(Fac_GetI2c(&self_->factory, destination_address, self_->init_data.i2c.interrupt_status_fptr),
                                    port_handle,
                                    mode,
                                    block_count,
                                    slave_address,
                                    timeout,
                                    data_len,
                                    data_ptr,
                                    result_fptr);
        }
    }

    return ret_val;
}

Ucs_Return_t Ucs_I2c_ReadPort(Ucs_Inst_t *self, uint16_t destination_address, uint16_t port_handle, uint8_t slave_address, uint8_t data_len, 
                              uint16_t timeout, Ucs_I2c_ReadPortResCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if (self_ != NULL)
    {
        ret_val = UCS_RET_ERR_NOT_INITIALIZED;
        if (self_->init_complete != false)
        {
            ret_val = I2c_ReadPort(Fac_GetI2c(&self_->factory, destination_address, self_->init_data.i2c.interrupt_status_fptr),
                                   port_handle,
                                   slave_address,
                                   data_len,
                                   timeout,
                                   result_fptr);
        }
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Components                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Initializes all UCS core components
 *  \param self     The instance
 */
static void Ucs_InitComponents(CUcs* self)
{
    Ucs_InitBaseComponent(self);
    Ucs_InitFactoryComponent(self);
    Ucs_InitLocalInicComponent(self);
    Ucs_InitNetComponent(self);
    Ucs_InitPmsComponent(self);
    Ucs_InitAmsComponent(self);
    Ucs_InitRoutingComponent(self);
    Ucs_InitAtsClass(self);

    Ucs_InitExcComponent(self);
    Ucs_InitSysDiagComponent(self);
    Ucs_InitNodeDiscovery(self);
    Ucs_InitBackChannelDiagnosis(self);
    Ucs_InitProgramming(self);
    Ucs_InitManager(self);      /* shall be called as last one due to re-configuration work */
}

/*! \brief Initializes the factory component
 *  \param self     The instance
 */
static void Ucs_InitFactoryComponent(CUcs *self)
{
    Fac_InitData_t fac_init_data;
    fac_init_data.base_ptr = &self->general.base;
    fac_init_data.net_ptr  = &self->net.inst;
    fac_init_data.xrmp_ptr = &self->xrmp;
    fac_init_data.icm_transceiver = &self->icm_transceiver;
    fac_init_data.rcm_transceiver = &self->rcm_transceiver;
    Fac_Ctor(&self->factory, &fac_init_data);
}

/*! \brief Initializes the the base component
 *  \param self     The instance
 */
static void Ucs_InitBaseComponent(CUcs *self)
{
    Base_InitData_t base_init_data;

    if (self->init_data.general.request_service_fptr != NULL)   /* pointer may be NULL for termination */
    {
        self->general.request_service_fptr = self->init_data.general.request_service_fptr;
        Sobs_Ctor(&self->general.service_request_obs, self, &Ucs_OnServiceRequest);
        base_init_data.scd.service_request_obs_ptr = &self->general.service_request_obs;
    }
    else
    {
        base_init_data.scd.service_request_obs_ptr = NULL;
    }

    self->general.get_tick_count_fptr = self->init_data.general.get_tick_count_fptr;
    Sobs_Ctor(&self->general.get_tick_count_obs, self, &Ucs_OnGetTickCount);
    base_init_data.tm.get_tick_count_obs_ptr = &self->general.get_tick_count_obs;
    if (self->init_data.general.set_application_timer_fptr != NULL)
    {
        self->general.set_application_timer_fptr = self->init_data.general.set_application_timer_fptr;
        Sobs_Ctor(&self->general.set_application_timer_obs, self, &Ucs_OnSetApplicationTimer);
        base_init_data.tm.set_application_timer_obs_ptr = &self->general.set_application_timer_obs;
    }
    else
    {
        base_init_data.tm.set_application_timer_obs_ptr = NULL;
    }
    base_init_data.ucs_inst_id = self->ucs_inst_id;
    base_init_data.ucs_user_ptr = self->ucs_user_ptr;
    Base_Ctor(&self->general.base, &base_init_data);
}

/*! \brief Initializes the port message service
 *  \param self     The instance
 */
static void Ucs_InitPmsComponent(CUcs *self)
{
    CPmFifo * mcm_fifo_ptr = NULL;
        
    if (self->init_data.ams.enabled == true)
    {
        mcm_fifo_ptr = &self->msg.mcm_fifo;
    }
    
    Ucs_InitPmsComponentConfig(self);
    Ucs_InitPmsComponentApp(self);

    Fifos_Ctor(&self->fifos, &self->general.base, &self->pmch, &self->icm_fifo, mcm_fifo_ptr, &self->rcm_fifo);
    Pmev_Ctor(&self->pme, &self->general.base, &self->fifos);       /* initialize event handler */
}

/*! \brief Initializes the port message service
 *  \param self     The instance
 */
static void Ucs_InitPmsComponentConfig(CUcs *self)
{
    Pmch_InitData_t pmch_init_data;
    Fifo_InitData_t icm_init;
    Fifo_Config_t icm_config;
    Fifo_InitData_t rcm_init;
    Fifo_Config_t rcm_config;

    /* Initialize port message service */
    pmch_init_data.ucs_user_ptr = self->ucs_user_ptr;
    pmch_init_data.tx_release_fptr = &Fifo_TxOnRelease;
    pmch_init_data.lld_iface = self->init_data.lld;
    Pmch_Ctor(&self->pmch, &pmch_init_data);

    /* Initialize the ICM channel */
    icm_init.base_ptr = &self->general.base;
    icm_init.channel_ptr = &self->pmch;
    icm_init.rx_cb_fptr = &Trcv_RxOnMsgComplete;
    icm_init.rx_cb_inst = &self->icm_transceiver;
    icm_init.tx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);
    icm_init.rx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);
    icm_config.fifo_id = PMP_FIFO_ID_ICM;
    icm_config.tx_wd_timeout = 0U;
    icm_config.tx_wd_timer_value = 0U;
    icm_config.rx_ack_timeout = 10U;
    icm_config.rx_busy_allowed = 0xFU;
    icm_config.rx_credits = PMCH_FIFO_CREDITS;
    icm_config.rx_threshold = PMCH_FIFO_THRESHOLD;
    if (self->init_data.general.inic_watchdog_enabled == false)
    {
        icm_config.rx_ack_timeout = 0U;
    }
    Fifo_Ctor(&self->icm_fifo, &icm_init, &icm_config);

    /* Initialize the RCM channel */
    rcm_init.base_ptr = &self->general.base;
    rcm_init.channel_ptr = &self->pmch;
    rcm_init.rx_cb_fptr = &Trcv_RxOnMsgComplete;
    rcm_init.rx_cb_inst = &self->rcm_transceiver;
    rcm_init.tx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);
    rcm_init.rx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);
    rcm_config.fifo_id = PMP_FIFO_ID_RCM;
    rcm_config.tx_wd_timeout = 10U;             /* Watchdog timeout: 1s */
    rcm_config.tx_wd_timer_value = 600U;        /* Watchdog trigger every 600 ms */
    rcm_config.rx_ack_timeout = 10U;            /* Acknowledge timeout: 10 ms */
    rcm_config.rx_busy_allowed = 0xFU;
    rcm_config.rx_credits = PMCH_FIFO_CREDITS;
    rcm_config.rx_threshold = PMCH_FIFO_THRESHOLD;
    if (self->init_data.general.inic_watchdog_enabled == false)
    {
        /* Disable INIC watchdog */
        rcm_config.tx_wd_timeout = 0U;          /* Watchdog timeout:    0 -> infinite */
        rcm_config.tx_wd_timer_value = 0U;      /* Watchdog timer:      0 -> no timer */
        rcm_config.rx_ack_timeout = 0U;         /* Acknowledge timeout: 0 -> infinite */
    }
    Fifo_Ctor(&self->rcm_fifo, &rcm_init, &rcm_config);
#if 0
    Fifos_Ctor(&self->fifos, &self->general.base, &self->pmch, &self->icm_fifo, NULL/*MCM*/, &self->rcm_fifo);
    Pmev_Ctor(&self->pme, &self->general.base, &self->fifos);       /* initialize event handler */
#endif

    /* initialize transceivers and set reference to FIFO instance */
    Trcv_Ctor(&self->icm_transceiver, &self->icm_fifo, MSG_ADDR_EHC_CFG, self->ucs_user_ptr, PMP_FIFO_ID_ICM);
    Trcv_RxAssignFilter(&self->icm_transceiver, &Ucs_OnRxMsgFilter, self);
    Trcv_RxAssignReceiver(&self->icm_transceiver, &Inic_OnIcmRx, self->inic.local_inic);
    Trcv_Ctor(&self->rcm_transceiver, &self->rcm_fifo, MSG_ADDR_EHC_CFG, self->ucs_user_ptr, PMP_FIFO_ID_RCM);
    /* Assign RX filter and receiver function to the RCM transceiver */
    Trcv_RxAssignFilter(&self->rcm_transceiver, &Ucs_OnRxMsgFilter, self);
    Trcv_RxAssignReceiver(&self->rcm_transceiver, &Ucs_OnRxRcm, self);
}

/*! \brief Initializes the network management component
 *  \param self     The instance
 */
static void Ucs_InitNetComponent(CUcs *self)
{
    Net_InitData_t net_init_data;

    Sobs_Ctor(&self->net.startup_obs, self, &Ucs_NetworkStartupResult);
    Sobs_Ctor(&self->net.shutdown_obs, self, &Ucs_NetworkShutdownResult);
    Sobs_Ctor(&self->net.force_na_obs, self, &Ucs_NetworkForceNAResult);
    Sobs_Ctor(&self->net.frame_counter_obs, self, &Ucs_NetworkFrameCounterResult);
    net_init_data.base_ptr = &self->general.base;
    net_init_data.inic_ptr = self->inic.local_inic;
    Net_Ctor(&self->net.inst, &net_init_data);
}

/*! \brief Initializes the FBlock INIC
 *  \param self     The instance
 */
static void Ucs_InitLocalInicComponent(CUcs *self)
{
    self->inic.local_inic = Fac_GetInic(&self->factory, UCS_ADDR_LOCAL_INIC);
    Obs_Ctor(&self->inic.device_status_obs, self, &Ucs_Inic_OnDeviceStatus);
}

/*! \brief Initializes the Routing components
 *  \param self     The instance
 */
static void Ucs_InitRoutingComponent(CUcs *self)
{
    Epm_InitData_t epm_init;
    Rtm_InitData_t rtm_init;

    /* Initialize the unique XRM Pool Instance */
    Xrmp_Ctor(&self->xrmp);

    /* Initialize the EndPoint Management Instance */
    epm_init.base_ptr = &self->general.base;
    epm_init.fac_ptr  = &self->factory;
    epm_init.res_debugging_fptr = self->init_data.rm.debug_resource_status_fptr;
    epm_init.check_unmute_fptr  = self->init_data.rm.xrm.check_unmute_fptr;
    Epm_Ctor (&self->epm, &epm_init);

    /* Initialize the Routes Management Instance */
    rtm_init.base_ptr = &self->general.base;
    rtm_init.epm_ptr  = &self->epm;
    rtm_init.net_ptr  = &self->net.inst;
    rtm_init.report_fptr = self->init_data.rm.report_fptr;
    Rtm_Ctor(&self->rtm, &rtm_init);
}

/*! \brief Initializes the attach service
 *  \param self     The instance
 */
static void Ucs_InitAtsClass(CUcs *self)
{
    Ats_InitData_t ats_init_data;
    ats_init_data.base_ptr = &self->general.base;
    ats_init_data.fifos_ptr = &self->fifos;
    ats_init_data.inic_ptr = self->inic.local_inic;
    ats_init_data.pme_ptr = &self->pme;
    Ats_Ctor(&self->inic.attach, &ats_init_data);
}

/*! \brief Initializes the FBlock ExtendedNetworkControl API
 *  \param self The instance
 */
static void Ucs_InitExcComponent(CUcs *self)
{
    /* Create the FBlock ExtendedNetworkControl instance */
    Exc_Ctor(&self->exc, &self->general.base, &self->rcm_transceiver);
}

/*! \brief Initializes the SystemDiagnosis component
 *  \param self The instance
 */
static void Ucs_InitSysDiagComponent(CUcs *self)
{
    /* Create the System Diagnosis instance */
    SysDiag_Ctor(&self->sys_diag, self->inic.local_inic, &self->general.base, &self->exc);
}


static void Ucs_InitNodeDiscovery(CUcs *self)
{
    Nd_InitData_t nd_init_data;

    if (self->init_data.mgr.enabled == false)
    {
        nd_init_data.inst_ptr = self;
        nd_init_data.report_fptr = &Ucs_OnNdReport;
        nd_init_data.eval_fptr = &Ucs_OnNdEvaluate;
    }
    else
    {
        nd_init_data.inst_ptr = &self->nobs;
        nd_init_data.report_fptr = &Nobs_OnNdReport;
        nd_init_data.eval_fptr = &Nobs_OnNdEvaluate;
    }

    Nd_Ctor(&self->nd, self->inic.local_inic, &self->general.base, &self->exc, &nd_init_data);

}

static void Ucs_InitBackChannelDiagnosis(CUcs *self)
{
    Bcd_Ctor(&self->bcd, self->inic.local_inic, &self->general.base, &self->exc);
}

static void Ucs_InitProgramming(CUcs *self)
{
    Prg_Ctor(&self->prg, self->inic.local_inic, &self->general.base, &self->exc);
}


/*! \brief      Initializes the Manager class
 *  \details    This function shall be called as the latest initialization function since
 *              it may disable some of the conventional API.
 *  \param self The instance
 */
static void Ucs_InitManager(CUcs *self)
{
    if (self->init_data.mgr.enabled == true)
    {
        Mgr_Ctor(&self->mgr, &self->general.base, self->inic.local_inic, &self->net.inst, &self->nd, self->init_data.mgr.packet_bw);
        Nobs_Ctor(&self->nobs, &self->general.base, &self->nd, &self->rtm, &self->init_data.mgr);
    }
}


/*! \brief Callback function which announces the result of the attach process
 *  \param self         The instance
 *  \param result_ptr   Result of the initialization process. Result must be casted into data type 
 *                      Ucs_InitResult_t. Possible return values are shown in the table below.
 *         Result Code                   | Description 
 *         ----------------------------- | ----------------------------------------------------
 *         UCS_INIT_RES_SUCCESS          | Initialization succeeded
 *         UCS_INIT_RES_ERR_BUF_OVERFLOW | No message buffer available
 *         UCS_INIT_RES_ERR_PMS_INIT     | PMS Initialization failed
 *         UCS_INIT_RES_ERR_INIC_VERSION | INIC device version check failed
 *         UCS_INIT_RES_ERR_DEV_ATT_CFG  | Device attach failed due to an configuration error
 *         UCS_INIT_RES_ERR_DEV_ATT_PROC | Device attach failed due to a system error
 *         UCS_INIT_RES_ERR_NET_CFG      | Network configuration failed
 *         UCS_INIT_RES_ERR_TIMEOUT      | Initialization timeout occurred
 */
static void Ucs_InitResultCb(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Ucs_InitResult_t *result_ptr_ = (Ucs_InitResult_t *)result_ptr;

    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_InitResultCb(): Ucs_Init() completed, internal event code: %u", 1U, *result_ptr_));
    if (*result_ptr_ != UCS_INIT_RES_SUCCESS)
    {
        Ucs_StopAppNotification(self_);
    }

    if (self_->init_result_fptr != NULL)
    {
        self_->init_result_fptr(*result_ptr_, self_->ucs_user_ptr);
    }

    /* Start notification if initialization succeeded */
    if (*result_ptr_ == UCS_INIT_RES_SUCCESS)
    {
        self_->init_complete = true;
        Ucs_StartAppNotification(self_);
    }
}

/*! \brief Callback function which announces the result of Ucs_Stop()
 *  \param self             The instance
 *  \param error_code_ptr   Reference to the error code
 */
static void Ucs_UninitResultCb(void *self, void *error_code_ptr)
{
    CUcs *self_ = (CUcs*)self;
    uint32_t error_code = *((uint32_t *)error_code_ptr);
    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_UninitResultCb(): Ucs_Stop() completed, internal event code: %u", 1U, error_code));

    self_->init_complete = false;
    Eh_DelObsrvInternalEvent(&self_->general.base.eh, &self_->uninit_result_obs);

    Ucs_StopAppNotification(self_);

    if (self_->uninit_result_fptr != NULL)
    {
        Ucs_StdResult_t result;

        result.code      = UCS_RES_SUCCESS;
        result.info_ptr  = NULL;
        result.info_size = 0U;

        if (error_code != EH_E_UNSYNC_COMPLETE)
        {
            result.code     = UCS_RES_ERR_TIMEOUT;
        }

        self_->uninit_result_fptr(result, self_->ucs_user_ptr);
        self_->uninit_result_fptr = NULL;
    }
}

/*! \brief Starts the notification after the initialization has succeeded
 *  \param self     The instance
 */
static void Ucs_StartAppNotification(CUcs *self)
{
    self->general.general_error_fptr = self->init_data.general.error_fptr;          /* assign general error notification */
    Sobs_Ctor(&self->general.general_error_obs, self, &Ucs_OnGeneralError);
    Eh_AddObsrvPublicError(&self->general.base.eh, &self->general.general_error_obs);

    if (self->init_data.network.status.cb_fptr != NULL)                             /* Start notification of Network Status */
    {
        self->net.status_fptr = self->init_data.network.status.cb_fptr;
        Mobs_Ctor(&self->net.status_obs,
                  self,
                  (uint32_t)self->init_data.network.status.notification_mask,
                  &Ucs_NetworkStatus);
        Net_AddObserverNetworkStatus(&self->net.inst, &self->net.status_obs);
    }

    if ((self->init_data.ams.tx.message_freed_fptr != NULL) && (self->msg.ams_tx_alloc_failed != false))
    {
        self->msg.ams_tx_alloc_failed = false;
        self->init_data.ams.tx.message_freed_fptr(self->ucs_user_ptr);
    }

    if (self->init_data.inic.power_state_fptr != NULL)
    {
        self->inic.power_state = Inic_GetDevicePowerState(self->inic.local_inic);   /* remember the current value */
        self->init_data.inic.power_state_fptr(self->inic.power_state, self->ucs_user_ptr);
        Inic_AddObsvrDeviceStatus(self->inic.local_inic, &self->inic.device_status_obs);
    }
    
    if(self->init_data.rm.xrm.most_port_status_fptr != NULL)                        /* Initialize callback pointer for MOST port status callback */
    {
        self->xrm.most_port_status_fptr = self->init_data.rm.xrm.most_port_status_fptr;
        Obs_Ctor(&self->xrm.most_port_status_obs, self, &Ucs_Most_PortStatusCb);
        Inic_AddObsrvMostPortStatus(self->inic.local_inic, &self->xrm.most_port_status_obs);
    }
}

/*! \brief Stops application events for timer management and event service
 *  \param self     The instance
 */
static void Ucs_StopAppNotification(CUcs *self)
{
    self->general.request_service_fptr = NULL;      /* clear service request to avoid any pending events to be called again */
    Tm_StopService(&self->general.base.tm);         /* stop timer service */
}

/*------------------------------------------------------------------------------------------------*/
/* Message Routing                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function to receive RCM Rx messages
 *  \param  self     The UCS instance
 *  \param  tel_ptr  The received RCM Rx message object 
 *  \return Returns \c true to discard the message and free it to the pool (no-pass).
 *          Otherwise, returns \c false (pass).
 */
static void Ucs_OnRxRcm(void *self, Msg_MostTel_t *tel_ptr)
{
    CUcs *self_ = (CUcs*)self;

    if (tel_ptr->id.fblock_id == FB_EXC)
    {
        Exc_OnRcmRxFilter(&(self_->exc), tel_ptr);
    }
    else if (tel_ptr->id.fblock_id == FB_INIC)
    {
        if (!Nsm_OnRcmRxFilter(Fac_FindNsm(&self_->factory, tel_ptr->source_addr), tel_ptr))
        {
            CInic * inic_ptr = Fac_FindInic(&self_->factory, tel_ptr->source_addr);
            if (inic_ptr != NULL)
            {
                Inic_OnRcmRxFilter(inic_ptr, tel_ptr);
            }
        }
    }

    Trcv_RxReleaseMsg(&self_->rcm_transceiver, tel_ptr); /* free Rx telegram */
}

/*! \brief  Callback function which filters Control Rx messages
 *  \param  self     The UCS instance
 *  \param  tel_ptr  The received Rx message object 
 *  \return Returns \c true to discard the message and free it to the pool (no-pass).
 *          Otherwise, returns \c false (pass).
 */
static bool Ucs_OnRxMsgFilter(void *self, Msg_MostTel_t *tel_ptr)
{
    CUcs *self_ = (CUcs*)self;
    bool ret = false;                   /* just pass - do not discard message */

    if (self_->rx_filter_fptr != NULL)
    {
        ret = self_->rx_filter_fptr(tel_ptr, self_->ucs_user_ptr);
    }

    if (ret == false)
    {
        if ((tel_ptr->id.op_type == UCS_OP_ERROR) || (tel_ptr->id.op_type == UCS_OP_ERRORACK))
        {
            if (self_->init_data.general.debug_error_msg_fptr != NULL)
            {
                self_->init_data.general.debug_error_msg_fptr(tel_ptr, self_->ucs_user_ptr);
            }
        }
    }
    else
    {
        TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_OnRxMsgFilter(): message discarded by unit test", 0U));
    }

    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* Internal Observers / Basic API                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Callback function which is invoked to request the current tick count value
 *  \param self                     The instance
 *  \param tick_count_value_ptr     Reference to the requested tick count value. The pointer must 
 *                                  be casted into data type uint16_t.
 */
static void Ucs_OnGetTickCount(void *self, void *tick_count_value_ptr)
{
    CUcs *self_ = (CUcs*)self;
    *((uint16_t *)tick_count_value_ptr) = self_->general.get_tick_count_fptr(self_->ucs_user_ptr);
}

/*! \brief  Callback function which is invoked to start the application timer when the UNICENS service
 *          is implemented event driven         
 *  \param  self                The instance
 *  \param  new_time_value_ptr  Reference to the new timer value. The pointer must be casted into 
 *                              data type uint16_t.
 */
static void Ucs_OnSetApplicationTimer(void *self, void *new_time_value_ptr)
{
    CUcs *self_ = (CUcs*)self;
    TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_OnSetApplicationTimer(%d)", 1U, *((uint16_t *)new_time_value_ptr)));
    self_->general.set_application_timer_fptr(*((uint16_t *)new_time_value_ptr), self_->ucs_user_ptr);
}

/*! \brief Callback function which is invoked to announce a request for service
 *  \param self         The instance
 *  \param result_ptr   Result pointer (not used)
 */
static void Ucs_OnServiceRequest(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;

    TR_ASSERT(self_->ucs_user_ptr, "[API]", self_->init_data.general.request_service_fptr != NULL);
    self_->general.request_service_fptr(self_->ucs_user_ptr);   /* Call application callback */
    MISC_UNUSED(result_ptr);
}

/*! \brief Callback function which announces a general error
 *  \param self         The instance
 *  \param result_ptr   Reference to the result. Must be casted into Eh_PublicErrorData_t.
 */
static void Ucs_OnGeneralError(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Ucs_Error_t error_code = *((Ucs_Error_t *)result_ptr);

    self_->init_complete = false;                               /* General error occurred -> Lock UCS API */
    Ucs_StopAppNotification(self_);

    if (self_->general.general_error_fptr != NULL)              /* callback is not assigned during initialization  */
    {
        self_->general.general_error_fptr(error_code, self_->ucs_user_ptr); 
    }
}

/*! \brief Observer callback for Inic_MostPortStatus_Status/Error(). Casts the result and 
 *         invokes the application result callback.
 *  \param self         Instance pointer
 *  \param result_ptr   Reference to result
 */
static void Ucs_Most_PortStatusCb(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    if(self_->xrm.most_port_status_fptr != NULL)
    {
        Inic_MostPortStatus_t status = *((Inic_MostPortStatus_t *)result_ptr);
        self_->xrm.most_port_status_fptr(status.most_port_handle,
                                         status.availability,
                                         status.avail_info,
                                         status.freestreaming_bw,
                                         self_->ucs_user_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* INIC                                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Observer callback function for INICs device status
 *  \param   self       The instance
 *  \param   data_ptr   Pointer to structure Inic_DeviceStatus_t
 */
static void Ucs_Inic_OnDeviceStatus(void *self, void *data_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Ucs_Inic_PowerState_t pws = ((Inic_DeviceStatus_t *)data_ptr)->power_state;

    if ((self_->init_data.inic.power_state_fptr != NULL) && (pws != self_->inic.power_state))
    {
        self_->init_data.inic.power_state_fptr(pws, self_->ucs_user_ptr);
    }

    self_->inic.power_state = pws;
}

/*------------------------------------------------------------------------------------------------*/
/* Network Management                                                                             */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Network_Startup(Ucs_Inst_t* self, uint16_t packet_bw, uint16_t forced_na_timeout,
                                 Ucs_StdResultCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;

    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;
    if (self_->init_complete != false)
    {
        ret_val = Inic_NwStartup(self_->inic.local_inic, forced_na_timeout,
                                 packet_bw, &self_->net.startup_obs);
        if (ret_val == UCS_RET_SUCCESS)
        {
            self_->net.startup_fptr = result_fptr;
        }
    }
    return ret_val;
}

/*! \brief  Callback function which announces the result of Ucs_Network_Startup()
 *  \param  self         The instance
 *  \param  result_ptr   Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Ucs_NetworkStartupResult(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (self_->net.startup_fptr != NULL)
    {
        self_->net.startup_fptr(result_ptr_->result, self_->ucs_user_ptr);
    }
}

Ucs_Return_t Ucs_Network_Shutdown(Ucs_Inst_t *self, Ucs_StdResultCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;
    if (self_->init_complete != false)
    {
        ret_val = Inic_NwShutdown(self_->inic.local_inic, &self_->net.shutdown_obs);
        if (ret_val == UCS_RET_SUCCESS)
        {
            self_->net.shutdown_fptr = result_fptr;
        }
    }
    return ret_val;
}

/*! \brief  Callback function which announces the result of Ucs_Network_Shutdown()
 *  \param  self        The instance
 *  \param  result_ptr  Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Ucs_NetworkShutdownResult(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (self_->net.shutdown_fptr != NULL)
    {
        self_->net.shutdown_fptr(result_ptr_->result, self_->ucs_user_ptr);
    }
}

Ucs_Return_t Ucs_Network_ForceNotAvailable(Ucs_Inst_t *self, bool force, Ucs_StdResultCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;
    if (self_->init_complete != false)
    {
        ret_val = Inic_NwForceNotAvailable(self_->inic.local_inic, force, &self_->net.force_na_obs);
        if (ret_val == UCS_RET_SUCCESS)
        {
            self_->net.force_na_fptr = result_fptr;
        }
    }
    return ret_val;
}

/*! \brief  Callback function which announces the result of Network_ForceNotAvailable()
 *  \param  self        The instance
 *  \param  result_ptr  Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Ucs_NetworkForceNAResult(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (self_->net.force_na_fptr != NULL)
    {
        self_->net.force_na_fptr(result_ptr_->result, self_->ucs_user_ptr);
    }
}

Ucs_Return_t Ucs_Network_GetFrameCounter(Ucs_Inst_t *self, uint32_t reference, Ucs_Network_FrameCounterCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;
    if (self_->init_complete != false)
    {
        ret_val = Inic_NwFrameCounter_Get(self_->inic.local_inic, reference, &self_->net.frame_counter_obs);
        if (ret_val == UCS_RET_SUCCESS)
        {
            self_->net.frame_counter_fptr = result_fptr;
        }
    }
    return ret_val;
}


/*! \brief Callback function which announces the result of Ucs_Network_GetFrameCounter()
 *  \param self         The instance
 *  \param result_ptr   Reference to result. Must be casted into Inic_StdResult_t and data_info 
 *                      must be casted into Inic_FrameCounterStatus_t.
 */
static void Ucs_NetworkFrameCounterResult(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;

    if (self_->net.frame_counter_fptr != NULL)
    {
        Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
        uint32_t reference;
        uint32_t frame_counter;
        uint8_t  lock;

        if (result_ptr_->data_info != NULL)
        {
            Inic_FrameCounterStatus_t *frame_counter_result_data_ptr = (Inic_FrameCounterStatus_t *)result_ptr_->data_info;
            reference     = frame_counter_result_data_ptr->reference;
            frame_counter = frame_counter_result_data_ptr->frame_counter;
            lock          = frame_counter_result_data_ptr->lock;
        }
        else
        {
            reference     = 0U;
            frame_counter = 0U;
            lock          = 0U;  
        }

        self_->net.frame_counter_fptr(reference, frame_counter, lock, result_ptr_->result, self_->ucs_user_ptr);
    }
}

/*! \brief  Observer callback which monitors the network status
 *  \param  self        The instance
 *  \param  result_ptr  Reference to result. Must be casted into Net_NetworkStatusParam_t.
 */
static void Ucs_NetworkStatus(void *self, void *result_ptr)
{
    CUcs *self_ = (CUcs*)self;
    Net_NetworkStatusParam_t *result_ptr_ = (Net_NetworkStatusParam_t *)result_ptr;

    if (self_->net.status_fptr != NULL)
    {
        self_->net.status_fptr( result_ptr_->change_mask,
                                result_ptr_->events,
                                result_ptr_->availability,
                                result_ptr_->avail_info,
                                result_ptr_->avail_trans_cause,
                                result_ptr_->node_address,
                                result_ptr_->node_position,
                                result_ptr_->max_position,
                                result_ptr_->packet_bw,
                                self_->ucs_user_ptr);
    }
}

uint8_t Ucs_Network_GetNodesCount(Ucs_Inst_t *self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    return Inic_GetNumberOfNodes(self_->inic.local_inic);
}


/*------------------------------------------------------------------------------------------------*/
/* Node Discovery                                                                                 */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Nd_Start(Ucs_Inst_t* self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (self_->init_complete != false)
    {
        ret_val = Nd_Start(&self_->nd);
    }
    return ret_val;
}


Ucs_Return_t Ucs_Nd_Stop(Ucs_Inst_t* self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (self_->init_complete != false)
    {
        ret_val = Nd_Stop(&self_->nd);
    }
    return ret_val;
}


Ucs_Return_t Ucs_Nd_InitAll(Ucs_Inst_t* self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (self_->init_complete != false)
    {
        Nd_InitAll(&self_->nd);
        ret_val = UCS_RET_SUCCESS;
    }
    return ret_val;

}

/*! \brief  Callback function to proxy the user callback for node evaluation
 *  \param  self            The instance
 *  \param  signature_ptr   Reference to the node signature
 *  \return The evaluation return value which defines how to proceed with the node.
 */
static Ucs_Nd_CheckResult_t Ucs_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Nd_CheckResult_t ret_val = UCS_ND_CHK_UNKNOWN;

    if (self_->init_data.nd.eval_fptr != NULL)
    {
        ret_val = self_->init_data.nd.eval_fptr(signature_ptr, self_->ucs_user_ptr);
    }

    return ret_val;
}

/*! \brief  Callback function to proxy the user callback for node evaluation
 *  \param  self            The instance
 *  \param  code            The report code 
 *  \param  signature_ptr   Reference to the node signature or NULL if no signature applies.
 */
static void Ucs_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr)
{
    CUcs *self_ = (CUcs*)(void*)self;

    if (self_->init_data.nd.report_fptr != NULL)
    {
        self_->init_data.nd.report_fptr(code, signature_ptr, self_->ucs_user_ptr);
    }
}


/*------------------------------------------------------------------------------------------------*/
/* BackChannel Diagnosis                                                                          */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Bcd_Start(Ucs_Inst_t* self, Ucs_Bcd_ReportCb_t report_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (report_fptr == NULL)
    {
        ret_val = UCS_RET_ERR_PARAM;
    }
    else if (self_->init_complete != false)
    {
        Bcd_Start(&self_->bcd, report_fptr);
        ret_val = UCS_RET_SUCCESS;
    }
    return ret_val;
}


/*------------------------------------------------------------------------------------------------*/
/*  Programming service                                                                              */
/*------------------------------------------------------------------------------------------------*/
Ucs_Return_t Ucs_Prog_Start(Ucs_Inst_t *self, 
                            uint16_t node_id, 
                            Ucs_Signature_t *signature, 
                            Ucs_Prg_SessionType_t session_type, 
                            Ucs_Prg_Command_t* command_list, 
                            Ucs_Prg_ReportCb_t result_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (result_fptr == NULL)
    {
        ret_val = UCS_RET_ERR_PARAM;
    }
    else if (self_->init_complete != false)
    {
        Prg_Start(&self_->prg, node_id, signature, session_type, command_list, result_fptr);
        ret_val = UCS_RET_SUCCESS;
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Message Handling                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Initializes the port message service for application interface (MCM)
 *  \param self     The instance
 */
static void Ucs_InitPmsComponentApp(CUcs *self)
{
    Fifo_InitData_t mcm_init;
    Fifo_Config_t mcm_config;

    /* Initialize the MCM channel */
    mcm_init.base_ptr = &self->general.base;
    mcm_init.channel_ptr = &self->pmch;
    mcm_init.rx_cb_fptr = &Trcv_RxOnMsgComplete;
    mcm_init.rx_cb_inst = &self->msg.mcm_transceiver;
    mcm_init.tx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);
    mcm_init.rx_encoder_ptr = Enc_GetEncoder(ENC_CONTENT_00);

    /* Enable INIC watchdog */
    mcm_config.fifo_id = PMP_FIFO_ID_MCM;
    mcm_config.tx_wd_timeout = 10U;             /* Watchdog timeout: 1s */
    mcm_config.tx_wd_timer_value = 600U;        /* Watchdog trigger every 600 ms */
    mcm_config.rx_ack_timeout = 10U;            /* Acknowledge timeout: 10 ms */
    mcm_config.rx_busy_allowed = 0xFU;
    mcm_config.rx_credits = PMCH_MCM_CREDITS;
    mcm_config.rx_threshold = PMCH_MCM_THRESHOLD;
    if (self->init_data.general.inic_watchdog_enabled == false)
    {
        /* Disable INIC watchdog */
        mcm_config.tx_wd_timeout = 0U;          /* Watchdog timeout:    0 -> infinite */
        mcm_config.tx_wd_timer_value = 0U;      /* Watchdog timer:      0 -> no timer */
        mcm_config.rx_ack_timeout = 0U;         /* Acknowledge timeout: 0 -> infinite */
    }
    Fifo_Ctor(&self->msg.mcm_fifo,&mcm_init, &mcm_config);
#if 0
    Fifos_Ctor(&self->fifos, &self->general.base, &self->pmch, NULL, &self->msg.mcm_fifo, NULL);
    Pmev_Ctor(&self->pme, &self->general.base, &self->fifos);       /* initialize event handler */
#endif

    /* initialize transceivers and set reference to FIFO instance */
    Trcv_Ctor(&self->msg.mcm_transceiver, &self->msg.mcm_fifo, MSG_ADDR_EHC_APP, self->ucs_user_ptr, PMP_FIFO_ID_MCM);
    Trcv_RxAssignFilter(&self->msg.mcm_transceiver, &Ucs_McmRx_FilterCallback, self);
}

static void Ucs_InitAmsComponent(CUcs *self)
{
    Smm_Ctor(&self->msg.smm, self->ucs_user_ptr);
    (void)Smm_LoadPlugin(&self->msg.smm, &self->msg.ams_allocator, SMM_SIZE_RX_MSG);

    TR_ASSERT(self->ucs_user_ptr, "[API]", (self->msg.ams_allocator.alloc_fptr != NULL));
    TR_ASSERT(self->ucs_user_ptr, "[API]", (self->msg.ams_allocator.free_fptr != NULL));

    Amsp_Ctor(&self->msg.ams_pool, &self->msg.ams_allocator, self->ucs_user_ptr);
    Ams_Ctor(&self->msg.ams, &self->general.base, &self->msg.mcm_transceiver, NULL, &self->msg.ams_pool,
             SMM_SIZE_RX_MSG);
    Ams_TxSetDefaultRetries(&self->msg.ams, self->init_data.ams.tx.default_llrbc);

    Amd_Ctor(&self->msg.amd, &self->general.base, &self->msg.ams);
    Amd_AssignReceiver(&self->msg.amd, &Ucs_AmsRx_Callback, self);
    /* Amd_RxAssignModificator(&self->amd, &Mnsa_AmdRx_Modificator, self); */

    self->msg.ams_tx_alloc_failed = false;
    Obs_Ctor(&self->msg.ams_tx_freed_obs, self, &Ucs_AmsTx_FreedCallback);
    if (self->init_data.ams.tx.message_freed_fptr != NULL)
    {
        Ams_TxAssignMsgFreedObs(&self->msg.ams, &self->msg.ams_tx_freed_obs);
    }

    Cmd_Ctor(&self->msg.cmd, &self->general.base);
}

extern Ucs_AmsTx_Msg_t* Ucs_AmsTx_AllocMsg(Ucs_Inst_t *self, uint16_t data_size)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_AmsTx_Msg_t *ret_ptr = NULL;

    if ((self_->init_complete != false) && (self_->init_data.ams.enabled == true))
    {
        ret_ptr = Ams_TxGetMsg(&self_->msg.ams, data_size);
    }

    self_->msg.ams_tx_alloc_failed = (ret_ptr == NULL) ? true : false;
    return ret_ptr;
}

extern Ucs_Return_t Ucs_AmsTx_SendMsg(Ucs_Inst_t *self, Ucs_AmsTx_Msg_t *msg_ptr, Ucs_AmsTx_CompleteCb_t tx_complete_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if ((self_->init_complete != false) && (self_->init_data.ams.enabled == true))
    {
        ret_val = Ams_TxSendMsg(&self_->msg.ams, msg_ptr, NULL, tx_complete_fptr, self_->ucs_user_ptr);
    }

    return ret_val;
}

extern void Ucs_AmsTx_FreeUnusedMsg(Ucs_Inst_t *self, Ucs_AmsTx_Msg_t *msg_ptr)
{
    CUcs *self_ = (CUcs*)(void*)self;

    if (msg_ptr != NULL)
    {
        Ams_TxFreeUnusedMsg(&self_->msg.ams, msg_ptr);
    }
}

extern Ucs_AmsRx_Msg_t* Ucs_AmsRx_PeekMsg(Ucs_Inst_t *self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    Ucs_AmsRx_Msg_t *ret = NULL;

    if ((self_->init_complete != false) && (self_->init_data.ams.enabled == true))
    {
        ret = Amd_RxPeekMsg(&self_->msg.amd);
    }

    return ret;
}

extern void Ucs_AmsRx_ReleaseMsg(Ucs_Inst_t *self)
{
    CUcs *self_ = (CUcs*)(void*)self;

    if ((self_->init_complete != false) && (self_->init_data.ams.enabled == true))
    {
        Amd_RxReleaseMsg(&self_->msg.amd);
    }
}

extern uint16_t Ucs_AmsRx_GetMsgCnt(Ucs_Inst_t *self)
{
    CUcs *self_ = (CUcs*)(void*)self;
    uint16_t ret = 0U;

    if ((self_->init_complete != false) && (self_->init_data.ams.enabled == true))
    {
        ret = Amd_RxGetMsgCnt(&self_->msg.amd);
    }
    return ret;
}

/*! \brief  Callback function which announces that a new application message
 *          is added to the Rx queue
 *  \param  self     The instance
 */
static void Ucs_AmsRx_Callback(void *self)
{
    CUcs *self_ = (CUcs*)self;

    if (self_->init_data.ams.rx.message_received_fptr != NULL)
    {
        self_->init_data.ams.rx.message_received_fptr(self_->ucs_user_ptr);
    }
}

/*! \brief  Callback function which announces that the AMS Tx Pool provides again a Tx message object
 *          after a prior allocation has failed.
 *  \param  self     The instance
 *  \param  data_ptr Not used (always \c NULL)
 */
static void Ucs_AmsTx_FreedCallback(void *self, void *data_ptr)
{
    CUcs *self_ = (CUcs*)self;
    MISC_UNUSED(data_ptr);

    if ((self_->msg.ams_tx_alloc_failed != false) && (self_->init_complete != false))
    {
        self_->msg.ams_tx_alloc_failed = false;
        self_->init_data.ams.tx.message_freed_fptr(self_->ucs_user_ptr); 
    }
}

/*! \brief  Callback function which filters MCM Rx messages
 *  \param  self     The instance
 *  \param  tel_ptr  The received Rx message object 
 *  \return Returns \c true to discard the message and free it to the pool (no-pass).
 *          Otherwise, returns \c false (pass).
 */
static bool Ucs_McmRx_FilterCallback(void *self, Msg_MostTel_t *tel_ptr)
{
    CUcs *self_ = (CUcs*)self;
    bool ret = false;                           /* default: pass the message */

    if ((tel_ptr->id.fblock_id != MSG_DEF_FBLOCK_ID) || (tel_ptr->id.op_type != MSG_DEF_OP_TYPE) ||
        ((tel_ptr->id.function_id & (uint16_t)0x000FU) != MSG_DEF_FUNC_ID_LSN))
    {
        TR_INFO((self_->ucs_user_ptr, "[API]", "Ucs_McmRx_FilterCallback(): discarding Rx message with signature %02X.%02X.%03X.%X ", 4U, tel_ptr->id.fblock_id, tel_ptr->id.instance_id, tel_ptr->id.function_id, tel_ptr->id.op_type));
        ret = true;
    }

    MISC_UNUSED(self_);

    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* Message decoding                                                                               */
/*------------------------------------------------------------------------------------------------*/
Ucs_Cmd_Return_t Ucs_Cmd_AddMsgIdTable(Ucs_Inst_t *self, Ucs_Cmd_MsgId_t *msg_id_tab_ptr)
{
    Ucs_Cmd_Return_t ret_val;
    CUcs *self_ = (CUcs*)(void*)self;

    if (msg_id_tab_ptr != NULL)
    {
        ret_val = Cmd_AddMsgIdTable(&(self_->msg.cmd), msg_id_tab_ptr);
    }
    else
    {
        ret_val = UCS_CMD_RET_ERR_NULL_PTR;
    }

    return ret_val;
}


Ucs_Cmd_Return_t Ucs_Cmd_RemoveMsgIdTable(Ucs_Inst_t *self)
{
    Ucs_Cmd_Return_t ret_val;
    CUcs *self_ = (CUcs*)(void*)self;

    ret_val = Cmd_RemoveMsgIdTable(&(self_->msg.cmd));

    return ret_val;
}


Ucs_Cmd_Return_t Ucs_Cmd_DecodeMsg(Ucs_Inst_t *self, Ucs_AmsRx_Msg_t *msg_rx_ptr)
{
    Ucs_Cmd_Return_t ret_val;
    CUcs *self_ = (CUcs*)(void*)self;

    if(msg_rx_ptr != NULL)
    {
        ret_val = Cmd_DecodeMsg(&(self_->msg.cmd), msg_rx_ptr);
    }
    else
    {
        ret_val = UCS_CMD_RET_ERR_NULL_PTR;
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Unit tests only                                                                                */
/*------------------------------------------------------------------------------------------------*/
extern void Ucs_AssignRxFilter(Ucs_Inst_t *self, Ucs_RxFilterCb_t callback_fptr)
{
    CUcs *self_ = (CUcs*)(void*)self;
    self_->rx_filter_fptr = callback_fptr;
}


/*!
 * @}
 * \endcond
 */
/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

