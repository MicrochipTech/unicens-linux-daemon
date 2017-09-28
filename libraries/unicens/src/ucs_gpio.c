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
 * \brief Implementation of the GPIO module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_GPIO
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_gpio.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Gpio_PortCreateResCb(void *self, void *result_ptr);
static void Gpio_PinModeConfigResCb(void *self, void *result_ptr);
static void Gpio_PinStateConfigResCb(void *self, void *result_ptr);
static void Gpio_TriggerEventStatusCb(void *self, void *result_ptr);
static bool Gpio_RxFilter4NsmCb(Msg_MostTel_t *tel_ptr, void *self);
static void Gpio_RxError(void *self, Msg_MostTel_t *msg_ptr, Gpio_ErrResultCb_t res_cb_fptr);
static void Gpio_PortCreate_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Gpio_PortPinMode_Status(void *self, Msg_MostTel_t *msg_ptr);
static void Gpio_PortPinState_Status(void *self, Msg_MostTel_t *msg_ptr);
static void Gpio_NsmResultCb(void * self, Nsm_Result_t result);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class Gpio                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the GPIO class.
 *  \param self        Reference to CGpio instance.
 *  \param init_ptr    init data_ptr.
 */
void Gpio_Ctor(CGpio *self, Gpio_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CGpio));

    /* Set class instances */
    self->inic_ptr = init_ptr->inic_ptr;
    self->nsm_ptr  = init_ptr->nsm_ptr;

    self->curr_user_data.trigger_event_status_fptr = init_ptr->trigger_event_status_fptr;

    /* Init observers */
    Obs_Ctor(&self->triggerevent_observer, self, &Gpio_TriggerEventStatusCb);

    /* Subscribe Observers */
    Inic_AddObsrvGpioTriggerEvent(self->inic_ptr, &self->triggerevent_observer);

    /* Set device target address */
    self->device_address = Inic_GetTargetAddress(self->inic_ptr);
}

/*------------------------------------------------------------------------------------------------*/
/* Service Functions                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Creates the GPIO port
 *  \param self             Reference to CGpio instance.
 *  \param index            The index of the GPIO Port instance.
 *  \param debounce_time    The timeout for the GPIO debounce timer (in ms).
 *  \param res_fptr         Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Gpio_CreatePort(CGpio * self, uint8_t index, uint16_t debounce_time, Ucs_Gpio_CreatePortResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            Gpio_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = index;
            tmp_script->cfg_data[1] = MISC_HB(debounce_time);
            tmp_script->cfg_data[2] = MISC_LB(debounce_time);

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_GPIO_PORT_CREATE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_STARTRESULT;
            tmp_script->cfg_msg.DataLen  = 3U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];

            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
            
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &Gpio_RxFilter4NsmCb, &Gpio_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.portcreate_res_cb = res_fptr;
                self->curr_res_cb = &Gpio_PortCreateResCb;
            }
        }
    }

    return result;
}

/*! \brief Sets the pin mode configuration of the given GPIO port
 *  \param self                 Reference to CGpio instance.
 *  \param gpio_port_handle     The GPIO Port resource handle.
 *  \param pin                  The GPIO pin that is to be configured.
 *  \param mode                 The mode of the GPIO pin.
 *  \param res_fptr             Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Gpio_SetPinModeConfig(CGpio * self, uint16_t gpio_port_handle, uint8_t pin, Ucs_Gpio_PinMode_t mode, Ucs_Gpio_ConfigPinModeResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            Gpio_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = MISC_HB(gpio_port_handle);
            tmp_script->cfg_data[1] = MISC_LB(gpio_port_handle);
            tmp_script->cfg_data[2] = pin;
            tmp_script->cfg_data[3] = (uint8_t)mode;

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_GPIO_PORT_PIN_MODE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_SETGET;
            tmp_script->cfg_msg.DataLen  = 4U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];

            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
            
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &Gpio_RxFilter4NsmCb, &Gpio_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.pinmode_res_cb = res_fptr;
                self->curr_res_cb = &Gpio_PinModeConfigResCb;
            }
        }
    }

    return result;
}

/*! \brief Gets the pin mode configuration of the given GPIO port
 *  \param self                 Reference to CGpio instance.
 *  \param gpio_port_handle     The GPIO Port resource handle.
 *  \param res_fptr             Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Gpio_GetPinModeConfig(CGpio * self, uint16_t gpio_port_handle, Ucs_Gpio_ConfigPinModeResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            Gpio_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = MISC_HB(gpio_port_handle);
            tmp_script->cfg_data[1] = MISC_LB(gpio_port_handle);

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_GPIO_PORT_PIN_MODE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_GET;
            tmp_script->cfg_msg.DataLen  = 2U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];

            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
            
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &Gpio_RxFilter4NsmCb, &Gpio_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.pinmode_res_cb = res_fptr;
                self->curr_res_cb = &Gpio_PinModeConfigResCb;
            }
        }
    }

    return result;
}

/*! \brief Sets the pin state configuration of the given GPIO port
 *  \param  self                Reference to CGpio instance.
 *  \param gpio_port_handle     The GPIO Port resource handle.
 *  \param mask                 The GPIO pin to be written.
 *  \param data                 The state of the GPIO pin to be written.
 *  \param res_fptr             Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Gpio_SetPinStateConfig(CGpio * self, uint16_t gpio_port_handle, uint16_t mask, uint16_t data, Ucs_Gpio_PinStateResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            Gpio_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = MISC_HB(gpio_port_handle);
            tmp_script->cfg_data[1] = MISC_LB(gpio_port_handle);
            tmp_script->cfg_data[2] = MISC_HB(mask);
            tmp_script->cfg_data[3] = MISC_LB(mask);
            tmp_script->cfg_data[4] = MISC_HB(data);
            tmp_script->cfg_data[5] = MISC_LB(data);

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_GPIO_PORT_PIN_STATE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_SETGET;
            tmp_script->cfg_msg.DataLen  = 6U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];

            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
            
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &Gpio_RxFilter4NsmCb, &Gpio_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.pinstate_res_cb = res_fptr;
                self->curr_res_cb = &Gpio_PinStateConfigResCb;
            }
        }
    }

    return result;
}

/*! \brief Retrieves the pin state configuration of the given GPIO port
 *  \param  self                Reference to CGpio instance.
 *  \param gpio_port_handle     The GPIO Port resource handle.
 *  \param res_fptr             Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Gpio_GetPinStateConfig(CGpio * self, uint16_t gpio_port_handle, Ucs_Gpio_PinStateResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            Gpio_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = MISC_HB(gpio_port_handle);
            tmp_script->cfg_data[1] = MISC_LB(gpio_port_handle);

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_GPIO_PORT_PIN_STATE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_GET;
            tmp_script->cfg_msg.DataLen  = 2U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];

            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
            
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &Gpio_RxFilter4NsmCb, &Gpio_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.pinstate_res_cb = res_fptr;
                self->curr_res_cb = &Gpio_PinStateConfigResCb;
            }
        }
    }

    return result;
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Handles the result of the GPIOPortCreate.StartResultAck
 *  \param  self            Reference to CGpio instance
 *  \param  result_ptr      result pointer
 */
static void Gpio_PortCreateResCb(void *self, void *result_ptr)
{
    CGpio *self_ = (CGpio *)self;
    uint16_t gpio_port_handle;
    Ucs_Gpio_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_Gpio_Result_t));

    if (NULL != result_ptr_)
    {
        gpio_port_handle = 0U;
        res.code = UCS_GPIO_RES_ERR_CMD;
        res.details.result_type = UCS_GPIO_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code         = UCS_GPIO_RES_SUCCESS;
                gpio_port_handle = *(uint16_t *)result_ptr_->data_info;
            }
            else if(result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
            {
                res.details.result_type = UCS_GPIO_RESULT_TYPE_TX;
                res.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_GPIO_RES_ERR_SYNC;
            }
        }

        if (NULL != self_->curr_user_data.portcreate_res_cb)
        {
            self_->curr_user_data.portcreate_res_cb(self_->device_address, gpio_port_handle, res, self_->inic_ptr->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the GPIOPortPinMode.Status
 *  \param  self            Reference to CGpio instance
 *  \param  result_ptr      result pointer
 */
static void Gpio_PinModeConfigResCb(void *self, void *result_ptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_GpioPortPinModeStatus_t status;
    Ucs_Gpio_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_Gpio_Result_t));

    if (NULL != result_ptr_)
    {
        status.gpio_handle = 0U;
        status.cfg_list = NULL;
        status.len = 0U;
        res.code = UCS_GPIO_RES_ERR_CMD;
        res.details.result_type = UCS_GPIO_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code = UCS_GPIO_RES_SUCCESS;
                status = *(Inic_GpioPortPinModeStatus_t *)result_ptr_->data_info;
            }
            else if(result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
            {
                res.details.result_type = UCS_GPIO_RESULT_TYPE_TX;
                res.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_GPIO_RES_ERR_SYNC;
            }
        }

        if (NULL != self_->curr_user_data.pinmode_res_cb)
        {
            self_->curr_user_data.pinmode_res_cb(self_->device_address, status.gpio_handle, status.cfg_list, status.len, res, self_->inic_ptr->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the GPIOPortPinSate.Status
 *  \param  self            Reference to CGpio instance
 *  \param  result_ptr      result pointer
 */
static void Gpio_PinStateConfigResCb(void *self, void *result_ptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_GpioPortPinStateStatus_t status;
    Ucs_Gpio_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_Gpio_Result_t));

    if (NULL != result_ptr_)
    {
        status.gpio_handle   = 0U;
        status.current_state = 0U;
        status.sticky_state  = 0U;
        res.code = UCS_GPIO_RES_ERR_CMD;
        res.details.result_type = UCS_GPIO_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code = UCS_GPIO_RES_SUCCESS;
                status = *(Inic_GpioPortPinStateStatus_t *)result_ptr_->data_info;
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_GPIO_RES_ERR_SYNC;
            }
            else 
            {
                res.details.result_type = UCS_GPIO_RESULT_TYPE_TX;
                res.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
        }

        if (NULL != self_->curr_user_data.pinstate_res_cb)
        {
            self_->curr_user_data.pinstate_res_cb(self_->device_address, status.gpio_handle, status.current_state, status.sticky_state, res, self_->inic_ptr->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the GPIOPortTriggerEvent.Status
 *  \param  self            Reference to CGpio instance
 *  \param  result_ptr      result pointer
 */
static void Gpio_TriggerEventStatusCb(void *self, void *result_ptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_GpioTriggerEventStatus_t status;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (NULL != result_ptr_)
    {
        status = *(Inic_GpioTriggerEventStatus_t *)result_ptr_->data_info;

        if (NULL != self_->curr_user_data.trigger_event_status_fptr)
        {
            self_->curr_user_data.trigger_event_status_fptr(self_->device_address, status.gpio_handle, status.rising_edges, status.falling_edges, status.levels, self_->inic_ptr->base_ptr->ucs_user_ptr);
        }
    }
}


/*! \brief  Checks whether the incoming is our message and handles It if it's.
 *  \param  tel_ptr          Reference to the message object.
 *  \param  self             Reference to the user argument.
 *  \return  Returns \c true to discard the message and free it to the pool if it's  our message. Otherwise, returns 
 *           \c false.
 */
static bool Gpio_RxFilter4NsmCb(Msg_MostTel_t *tel_ptr, void *self)
{
    CGpio *self_ = (CGpio *)self;
    bool ret_val = true;

    if ((tel_ptr != NULL) && (tel_ptr->id.function_id == self_->curr_script.script.send_cmd->FunktId))
    {
        if (tel_ptr->id.op_type == UCS_OP_RESULT)
        {
            switch(tel_ptr->id.function_id)
            {
            case INIC_FID_GPIO_PORT_CREATE:
                Gpio_PortCreate_Result(self_, tel_ptr);
                break;
            case INIC_FID_GPIO_PORT_PIN_MODE:
                Gpio_PortPinMode_Status(self_, tel_ptr);
                break;
            case INIC_FID_GPIO_PORT_PIN_STATE:
                Gpio_PortPinState_Status(self_, tel_ptr);
                break;
            default:
                ret_val = false;
                break;
            }
        }
        else if (tel_ptr->id.op_type == UCS_OP_ERROR)
        {
            Gpio_ErrResultCb_t res_cb_fptr = self_->curr_res_cb;
            Gpio_RxError(self_, tel_ptr, res_cb_fptr);
        }
    }
    else
    {
        ret_val = false;
    }

    return ret_val;
}

/*! \brief  Result callback function for NSM result. Whenever this function is called the NodeScripting has finished the
 *  script's execution. This function handles transmission and sync error. Only these two kind of errors can occur.
 *  \param  self     Reference to the called user instance.
 *  \param  result   Result of the scripting operation.
 */
static void Gpio_NsmResultCb(void * self, Nsm_Result_t result)
{
    CGpio *self_ = (CGpio *)self;

    if (self_ != NULL)
    {
        Inic_StdResult_t res_data;
        bool allow_report = false;

        if ((result.code == UCS_NS_RES_ERROR) && (result.details.result_type == NS_RESULT_TYPE_TX))
        {
            res_data.data_info        = &result.details.tx_result;
            res_data.result.code      = UCS_RES_ERR_TRANSMISSION;
            res_data.result.info_ptr  = NULL;
            res_data.result.info_size = 0U;
            allow_report = true;
        }
        else if ((result.code == UCS_NS_RES_ERROR) && (result.details.result_type == NS_RESULT_TYPE_TGT_SYNC))
        {
            res_data.data_info        = &result.details.inic_result;
            res_data.result.code      = result.details.inic_result.code;
            res_data.result.info_ptr  = result.details.inic_result.info_ptr;
            res_data.result.info_size = result.details.inic_result.info_size;
            allow_report = true;
        }
        else if ((result.code == UCS_NS_RES_ERROR) && ((result.details.tx_result == UCS_MSG_STAT_OK) || 
                 (result.details.inic_result.code == UCS_RES_SUCCESS)))
        {
            res_data.data_info        = NULL;
            res_data.result.code      = UCS_RES_ERR_TIMEOUT;
            res_data.result.info_ptr  = NULL;
            res_data.result.info_size = 0U;

            TR_ERROR((self_->nsm_ptr->base_ptr->ucs_user_ptr, "[GPIO]", "TIMEOUT ERROR occurred for currently GPIO command. No response received from target device with address 0x%X.", 1U, self_->device_address));
        }

        if ((self_->curr_res_cb != NULL) && (allow_report))
        {
            self_->curr_res_cb(self_, &res_data);
        }
    }
}

/*---------------------------------- GW Functions ----------------------------------*/ 

/*! \brief   Error Handler function for all GPIO methods
 *  \param   self      Reference to CGpio instance
 *  \param   msg_ptr   Pointer to received message
 *  \param   res_cb_fptr   Pointer to a specified error handler function
 */
static void Gpio_RxError(void *self, Msg_MostTel_t *msg_ptr, Gpio_ErrResultCb_t res_cb_fptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_StdResult_t res_data;

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_->inic_ptr,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    if (res_cb_fptr != NULL)
    {
        res_cb_fptr(self_, &res_data);
    }
}

/*! \brief   Handler function for GPIOPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable gpio_port_handle which holds the
 *           GPIO Port resource handle.
 *  \param   self      Reference to CGpio instance
 *  \param   msg_ptr   Pointer to received message
 */
static void Gpio_PortCreate_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CGpio *self_ = (CGpio *)self;
    uint16_t gpio_port_handle;
    Inic_StdResult_t res_data;

    MISC_DECODE_WORD(&gpio_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &gpio_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    Gpio_PortCreateResCb(self_, &res_data);
}

/*! \brief   Handler function for GPIOPortPinMode.Status
 *  \param   self      Reference to CGpio instance
 *  \param   msg_ptr   Pointer to received message
 */
static void Gpio_PortPinMode_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_GpioPortPinModeStatus_t res;
    Inic_StdResult_t res_data;
    uint8_t i = 2U, j = 0U;
    Ucs_Gpio_PinConfiguration_t pin_ls[16U];

    res.cfg_list             = &pin_ls[0];
    res.len                  = (msg_ptr->tel.tel_len - 2U) >> 1U;
    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&res.gpio_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    for (; (i < msg_ptr->tel.tel_len) && (j < 16U); i=i+2U)
    {
        pin_ls[j].pin  = msg_ptr->tel.tel_data_ptr[i];
        pin_ls[j].mode = (Ucs_Gpio_PinMode_t)msg_ptr->tel.tel_data_ptr[i+1U];
        j++;
    }

    Gpio_PinModeConfigResCb(self_, &res_data);
}

/*! \brief   Handler function for GPIOPortPinState.Status
 *  \param   self      Reference to CGpio instance
 *  \param   msg_ptr   Pointer to received message
 */
static void Gpio_PortPinState_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CGpio *self_ = (CGpio *)self;
    Inic_GpioPortPinStateStatus_t res;
    Inic_StdResult_t res_data;

    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&res.gpio_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    MISC_DECODE_WORD(&res.current_state, &(msg_ptr->tel.tel_data_ptr[2]));
    MISC_DECODE_WORD(&res.sticky_state, &(msg_ptr->tel.tel_data_ptr[4]));

    Gpio_PinStateConfigResCb(self_, &res_data);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

