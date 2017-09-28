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
 * \brief Implementation of the I2C Module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_I2C
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_i2c.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void I2c_PortCreateResCb(void *self, void *result_ptr);
static void I2c_PortWriteResCb(void *self, void *result_ptr);
static void I2c_PortReadResCb(void *self, void *result_ptr);
static void I2c_TriggerEventStatusCb(void *self, void *result_ptr);
static bool I2c_RxFilter4NsmCb(Msg_MostTel_t *tel_ptr, void *self);
static void I2c_PortCreate_Result(void *self, Msg_MostTel_t *msg_ptr);
static void I2c_PortRead_Result(void *self, Msg_MostTel_t *msg_ptr);
static void I2c_PortWrite_Result(void *self, Msg_MostTel_t *msg_ptr);
static void I2c_RxError(void *self, Msg_MostTel_t *msg_ptr, I2c_ErrResultCb_t res_cb_fptr);
static void I2c_NsmResultCb(void * self, Nsm_Result_t result);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class I2C                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the I2C class.
 *  \param self        Instance pointer
 *  \param init_ptr    init data_ptr
 */
void I2c_Ctor(CI2c * self, I2c_InitData_t * init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CI2c));

    /* Set class instances */
    self->inic_ptr = init_ptr->inic_ptr;
    self->base_ptr = self->inic_ptr->base_ptr;
    self->nsm_ptr  = init_ptr->nsm_ptr; 

    self->curr_user_data.i2c_interrupt_report_fptr = init_ptr->i2c_interrupt_report_fptr;

    /* Init GPIOTriggerEvent observer */
    Obs_Ctor(&self->triggerevent_observer, self, &I2c_TriggerEventStatusCb);

    /* Subscribe Observers */
    Inic_AddObsrvGpioTriggerEvent(self->inic_ptr, &self->triggerevent_observer);

    /* Set device id */
    self->device_address = Inic_GetTargetAddress(self->inic_ptr);
}

/*------------------------------------------------------------------------------------------------*/
/* Service Functions                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Creates an I2C Port with its associated parameter. 
 *  \param  self                Reference to CI2c instance
 *  \param  index               I2C Port instance
 *  \param  speed               The speed grade of the I2C Port
 *  \param  i2c_int_mask        The I2C interrupt pin mask on the GPIO Port.
 *  \param  res_fptr            Required result callback function pointer.    
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t I2c_CreatePort(CI2c * self, uint8_t index, Ucs_I2c_Speed_t speed, uint8_t i2c_int_mask, Ucs_I2c_CreatePortResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;
    uint8_t address     = 0x00U; /* Address will be ignored */
    uint8_t mode        = 0x01U; /* Master Mode */

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            I2c_Script_t * tmp_script = &self->curr_script;

            /* Set Data */
            tmp_script->cfg_data[0] = index;
            tmp_script->cfg_data[1] = address;
            tmp_script->cfg_data[2] = mode;
            tmp_script->cfg_data[3] = (uint8_t)speed;

            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_I2C_PORT_CREATE;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_STARTRESULT;
            tmp_script->cfg_msg.DataLen  = 4U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];
          
            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;

            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &I2c_RxFilter4NsmCb, &I2c_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.int_pin_mask = i2c_int_mask;
                self->curr_user_data.portcreate_res_cb = res_fptr;
                self->curr_res_cb = &I2c_PortCreateResCb;
            }
        }
    }

    return result;
}                                                

/*! \brief  Writes a block of bytes to an I2C device at a specified I2C address.
 *  \param  self            Reference to CI2c instance
 *  \param  port_handle     Port resource handle
 *  \param  mode            The write transfer mode
 *  \param  block_count     The number of blocks to be written to the I2C address.
 *  \param  slave_address   The 7-bit I2C slave address of the peripheral to be read
 *  \param  timeout         The timeout for the I2C Port write
 *  \param  data_len        Number of bytes to be written to the addressed I2C peripheral
 *  \param  data_ptr        Reference to the data to be written
 *  \param  res_fptr        Required result callback function pointer.   
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t I2c_WritePort(CI2c * self, uint16_t port_handle, Ucs_I2c_TrMode_t mode, uint8_t block_count, uint8_t slave_address, uint16_t timeout, uint8_t data_len, uint8_t data_ptr[], Ucs_I2c_WritePortResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        if ((0U < data_len) && (NULL != data_ptr))
        {
            result = UCS_RET_ERR_API_LOCKED;
            if (!Nsm_IsLocked(self->nsm_ptr))
            {
                bool is_ok = true;

                result = UCS_RET_ERR_PARAM;
                if ((UCS_I2C_BURST_MODE == mode) && (0U == block_count))
                {
                    is_ok = false;
                }

                if (is_ok)
                {
                    uint8_t i;
                    I2c_Script_t * tmp_script = &self->curr_script;

                    for (i = 0U; i < data_len; i++)
                    {
                        tmp_script->cfg_data[8U + i] = data_ptr[i];
                    }

                    /* Set Data */
                    tmp_script->cfg_data[0] = MISC_HB(port_handle);
                    tmp_script->cfg_data[1] = MISC_LB(port_handle);
                    tmp_script->cfg_data[2] = (uint8_t)mode;
                    tmp_script->cfg_data[3] = block_count;
                    tmp_script->cfg_data[4] = slave_address;
                    tmp_script->cfg_data[5] = (mode == UCS_I2C_BURST_MODE)  ? (data_len/block_count):data_len;
                    tmp_script->cfg_data[6] = MISC_HB(timeout);
                    tmp_script->cfg_data[7] = MISC_LB(timeout);

                    /* Set message id */
                    tmp_script->cfg_msg.FBlockId = FB_INIC;
                    tmp_script->cfg_msg.InstId   = 0U;
                    tmp_script->cfg_msg.FunktId  = INIC_FID_I2C_PORT_WRITE;
                    tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_STARTRESULT;
                    tmp_script->cfg_msg.DataLen  = data_len + 8U;
                    tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];
          
                    /* Set script */
                    tmp_script->script.send_cmd = &tmp_script->cfg_msg;
                    tmp_script->script.pause    = 0U;

                    /* Transmit script */
                    result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &I2c_RxFilter4NsmCb, &I2c_NsmResultCb);
                    if(result == UCS_RET_SUCCESS)
                    {
                        self->curr_user_data.portwrite_res_cb = res_fptr;
                        self->curr_res_cb = &I2c_PortWriteResCb;
                    }
                }
            }
        }
    }

    return result;
}

/*! \brief  Reads a block of bytes from an I2C device at a specified I2C address. 
 *  \param  self            Reference to CI2c instance
 *  \param  port_handle     Port resource handle
 *  \param  slave_address   The 7-bit I2C slave address of the peripheral to be read
 *  \param  data_len        Number of bytes to be read from the address
 *  \param  timeout         The timeout for the I2C Port read
 *  \param  res_fptr        Required result callback function pointer.   
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t I2c_ReadPort(CI2c * self, uint16_t port_handle, uint8_t slave_address, uint8_t data_len, uint16_t timeout, Ucs_I2c_ReadPortResCb_t res_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((NULL != self) && (NULL != res_fptr))
    {
        result = UCS_RET_ERR_API_LOCKED;
        if (!Nsm_IsLocked(self->nsm_ptr))
        {
            I2c_Script_t * tmp_script = &self->curr_script;
        
            /* Set Data */
            tmp_script->cfg_data[0] = MISC_HB(port_handle);
            tmp_script->cfg_data[1] = MISC_LB(port_handle);
            tmp_script->cfg_data[2] = slave_address;
            tmp_script->cfg_data[3] = data_len;
            tmp_script->cfg_data[4] = MISC_HB(timeout);
            tmp_script->cfg_data[5] = MISC_LB(timeout); 
        
            /* Set message id */
            tmp_script->cfg_msg.FBlockId = FB_INIC;
            tmp_script->cfg_msg.InstId   = 0U;
            tmp_script->cfg_msg.FunktId  = INIC_FID_I2C_PORT_READ;
            tmp_script->cfg_msg.OpCode   = (uint8_t)UCS_OP_STARTRESULT;
            tmp_script->cfg_msg.DataLen  = 6U;
            tmp_script->cfg_msg.DataPtr  = &tmp_script->cfg_data[0];
        
            /* Set script */
            tmp_script->script.send_cmd = &tmp_script->cfg_msg;
            tmp_script->script.pause    = 0U;
        
            /* Transmit script */
            result = Nsm_Run_Pv(self->nsm_ptr, &tmp_script->script, 1U, self, &I2c_RxFilter4NsmCb, &I2c_NsmResultCb);
            if(result == UCS_RET_SUCCESS)
            {
                self->curr_user_data.portread_res_cb = res_fptr;
                self->curr_res_cb = &I2c_PortReadResCb;
            }
        }
    }

    return result;
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Handles the result of the I2CPortCreate.StartResultAck
 *  \param  self            Instance pointer
 *  \param  result_ptr      result pointer
 */
static void I2c_PortCreateResCb(void *self, void *result_ptr)
{
    CI2c *self_ = (CI2c *)self;
    uint16_t i2c_port_handle;
    Ucs_I2c_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_I2c_Result_t));

    if (NULL != result_ptr_)
    {
        i2c_port_handle = 0U;
        res.code = UCS_I2C_RES_ERR_CMD;
        res.details.result_type = UCS_I2C_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code        = UCS_I2C_RES_SUCCESS;
                i2c_port_handle = *(uint16_t *)result_ptr_->data_info;
            }
            else if(result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
            {
                res.details.result_type = UCS_I2C_RESULT_TYPE_TX;
                res.details.tx_result   = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_I2C_RES_ERR_SYNC;
            }
        }

        if (NULL != self_->curr_user_data.portcreate_res_cb)
        {
            self_->curr_user_data.portcreate_res_cb(self_->device_address, i2c_port_handle, res, self_->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the I2CPortWrite.StartResultAck
 *  \param  self            Instance pointer
 *  \param  result_ptr      result pointer
 */
static void I2c_PortWriteResCb(void *self, void *result_ptr)
{
    CI2c *self_ = (CI2c *)self;
    Inic_I2cWriteResStatus_t wr_res;
    Ucs_I2c_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_I2c_Result_t));

    if (NULL != result_ptr_)
    {
        wr_res.data_len = 0U;
        wr_res.port_handle = 0U;
        wr_res.slave_address = 0U;
        res.code = UCS_I2C_RES_ERR_CMD;
        res.details.result_type = UCS_I2C_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code = UCS_I2C_RES_SUCCESS;
                wr_res = *(Inic_I2cWriteResStatus_t *)result_ptr_->data_info;
            }
            else if(result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
            {
                res.details.result_type = UCS_I2C_RESULT_TYPE_TX;
                res.details.tx_result   = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_I2C_RES_ERR_SYNC;
            }
        }

        if (NULL != self_->curr_user_data.portwrite_res_cb)
        {
            self_->curr_user_data.portwrite_res_cb(self_->device_address, wr_res.port_handle, wr_res.slave_address, wr_res.data_len, res, self_->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the I2CPortRead.StartResultAck
 *  \param  self            Instance pointer
 *  \param  result_ptr      result pointer
 */
static void I2c_PortReadResCb(void *self, void *result_ptr)
{
    CI2c *self_ = (CI2c *)self;
    Inic_I2cReadResStatus_t read_res;
    Ucs_I2c_Result_t res;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    /* Init result */
    MISC_MEM_SET(&res, 0, sizeof(Ucs_I2c_Result_t));

    if (NULL != result_ptr_)
    {
        read_res.data_len = 0U;
        read_res.data_ptr = NULL;
        read_res.port_handle = 0U;
        read_res.slave_address = 0U;
        res.code = UCS_I2C_RES_ERR_CMD;
        res.details.result_type = UCS_I2C_RESULT_TYPE_TGT;
        res.details.inic_result = result_ptr_->result;
        if (result_ptr_->data_info != NULL)
        {
            if(result_ptr_->result.code == UCS_RES_SUCCESS)
            {
                res.code = UCS_I2C_RES_SUCCESS;
                read_res = *(Inic_I2cReadResStatus_t *)result_ptr_->data_info;
            }
            else if(result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
            {
                res.details.result_type = UCS_I2C_RESULT_TYPE_TX;
                res.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            }
            else if (result_ptr_->result.code == UCS_RES_ERR_CONFIGURATION)
            {
                res.code = UCS_I2C_RES_ERR_SYNC;
            }
        }

        if (NULL != self_->curr_user_data.portread_res_cb)
        {
            self_->curr_user_data.portread_res_cb(self_->device_address, read_res.port_handle, read_res.slave_address, read_res.data_len, read_res.data_ptr, res, self_->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Handles the result of the GPIOPortTriggerEvent.Status
 *  \param  self            Instance pointer
 *  \param  result_ptr      result pointer
 */
static void I2c_TriggerEventStatusCb(void *self, void *result_ptr)
{
    CI2c *self_ = (CI2c *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if ((NULL != result_ptr_) && 
        (NULL != self_->curr_user_data.i2c_interrupt_report_fptr))
    {
        Inic_GpioTriggerEventStatus_t status;
        uint16_t int_mask = self_->curr_user_data.int_pin_mask;
        status = *(Inic_GpioTriggerEventStatus_t *)result_ptr_->data_info;

        if ((!status.is_first_report) && 
            ((int_mask == (status.rising_edges & int_mask)) ||
            (int_mask == (status.levels & int_mask)) ||
            (int_mask == (status.falling_edges & int_mask))))
        {
            self_->curr_user_data.i2c_interrupt_report_fptr(self_->device_address, self_->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Checks whether the incoming is our message and handles It if it's.
 *  \param  tel_ptr          Reference to the message object.
 *  \param  self             Reference to the user argument.
 *  \return  Returns \c true to discard the message and free it to the pool if it's  our message. Otherwise, returns 
 *           \c false.
 */
static bool I2c_RxFilter4NsmCb(Msg_MostTel_t *tel_ptr, void *self)
{
    CI2c *self_ = (CI2c *)self;
    bool ret_val = true;

    if ((tel_ptr != NULL) && (tel_ptr->id.function_id == self_->curr_script.script.send_cmd->FunktId))
    {
        if (tel_ptr->id.op_type == UCS_OP_RESULT)
        {
            switch(tel_ptr->id.function_id)
            {
            case INIC_FID_I2C_PORT_CREATE:
                I2c_PortCreate_Result(self_, tel_ptr);
                break;
            case INIC_FID_I2C_PORT_READ:
                I2c_PortRead_Result(self_, tel_ptr);
                break;
            case INIC_FID_I2C_PORT_WRITE:
                I2c_PortWrite_Result(self_, tel_ptr);
                break;
            default:
                ret_val = false;
                break;
            }
        }
        else if (tel_ptr->id.op_type == UCS_OP_ERROR)
        {
            I2c_ErrResultCb_t res_cb_fptr = self_->curr_res_cb;
            I2c_RxError(self_, tel_ptr, res_cb_fptr);
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
static void I2c_NsmResultCb(void * self, Nsm_Result_t result)
{
    CI2c *self_ = (CI2c *)self;

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

            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[I2C]", "TIMEOUT ERROR occurred for currently I2C command. No response received from target device with address 0x%X.", 1U, self_->device_address));
        }

        if ((self_->curr_res_cb != NULL) && (allow_report))
        {
            self_->curr_res_cb(self_, &res_data);
        }
    }
}

/*---------------------------------- GW Functions ----------------------------------*/ 

/*! \brief   Error Handler function for all I2C methods
 *  \param   self      Reference to CI2c instance
 *  \param   msg_ptr   Pointer to received message
 *  \param   res_cb_fptr   Pointer to a specified error handler function
 */
static void I2c_RxError(void *self, Msg_MostTel_t *msg_ptr, I2c_ErrResultCb_t res_cb_fptr)
{
    CI2c *self_ = (CI2c *)self;
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

/*! \brief   Handler function for I2CPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable i2c_port_handle which holds the
 *           I2C Port resource handle.
 *  \param   self      Reference to CI2c instance
 *  \param   msg_ptr   Pointer to received message
 */
static void I2c_PortCreate_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CI2c *self_ = (CI2c *)self;
    uint16_t i2c_port_handle;
    Inic_StdResult_t res_data;

    MISC_DECODE_WORD(&i2c_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &i2c_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    I2c_PortCreateResCb(self_, &res_data);
}

/*! \brief   Handler function for I2CPortRead.ResultAck
 *  \details Element res_data.data_info points to a variable of type Inic_I2cReadResStatus_t which holds the
 *          the results of the I2CPortRead.StartResultAck command.
 *  \param   self      Reference to CI2c instance
 *  \param   msg_ptr   Pointer to received message
 */
static void I2c_PortRead_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CI2c *self_ = (CI2c *)self;
    Inic_I2cReadResStatus_t i2c_read_res;
    Inic_StdResult_t res_data;

    res_data.data_info       = &i2c_read_res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&i2c_read_res.port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    i2c_read_res.slave_address = msg_ptr->tel.tel_data_ptr[2];
    i2c_read_res.data_len      = msg_ptr->tel.tel_data_ptr[3];
    i2c_read_res.data_ptr      = &msg_ptr->tel.tel_data_ptr[4];

    I2c_PortReadResCb(self_, &res_data);
}

/*! \brief   Handler function for I2CPortWrite.ResultAck
 *  \details Element res_data.data_info points to a variable of type Inic_I2cWriteResStatus_t which holds the
 *          the results of the I2CPortWrite.StartResultAck command.
 *  \param   self      Reference to CI2c instance
 *  \param   msg_ptr   Pointer to received message
 */
static void I2c_PortWrite_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CI2c *self_ = (CI2c *)self;
    Inic_I2cWriteResStatus_t i2c_write_res;
    Inic_StdResult_t res_data;

    res_data.data_info       = &i2c_write_res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&i2c_write_res.port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    i2c_write_res.slave_address = msg_ptr->tel.tel_data_ptr[2];
    i2c_write_res.data_len      = msg_ptr->tel.tel_data_ptr[3];

    I2c_PortWriteResCb(self_, &res_data);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

