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
 * \brief Internal header file of the I2C module.
 *
 * \defgroup   G_UCS_I2C_TYPES I2C Referred Types
 * \brief      Referred types used by the Extended Resource Manager.
 * \ingroup    G_UCS_I2C
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_I2C
 * @{
 */


#ifndef UCS_I2C_H
#define UCS_I2C_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nsm.h"
#include "ucs_ret_pb.h"
#include "ucs_obs.h"
#include "ucs_i2c_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for I2C results in case error.
 *  \param   self      Reference to CI2c instance
 *  \param   msg_ptr   Pointer to received message
 */
typedef void (*I2c_ErrResultCb_t)(void *self, void *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Stores data required by I2C during initialization. */
typedef struct I2c_InitData_
{
    /*!< \brief Reference to INIC instance */
    CInic *inic_ptr;
    /*!< \brief Reference to NSM instance */
    CNodeScriptManagement *nsm_ptr;
    /*!< \brief Status function pointer for the I2C Interrupt event */
    Ucs_I2c_IntEventReportCb_t i2c_interrupt_report_fptr;

} I2c_InitData_t;

/*! \brief  Stores data required by I2C during initialization. */
typedef struct I2c_UserData_
{
    /*!< \brief PinState Result callback */
    Ucs_I2c_ReadPortResCb_t portread_res_cb;
    /*!< \brief PortCreate Result callback */
    Ucs_I2c_WritePortResCb_t portwrite_res_cb;
    /*!< \brief PortCreate Result callback */
    Ucs_I2c_CreatePortResCb_t portcreate_res_cb;
    /*!< \brief Status function pointer for the I2c interrupt */
    Ucs_I2c_IntEventReportCb_t i2c_interrupt_report_fptr;
    /*!< \brief I2C interrupt pin mask on GPIO Port */
    uint8_t int_pin_mask;

} I2c_UserData_t;

/*! \brief  Script structure of the I2C module */
typedef struct I2c_Script_
{
    uint8_t cfg_data[40];
    /*! \brief script used for transmitting commands */
    Ucs_Ns_Script_t script;
    /*! \brief config messages used for transmitting commands */
    Ucs_Ns_ConfigMsg_t cfg_msg;
} I2c_Script_t;

/*! \brief  Class structure of the I2C module */
typedef struct CI2c_
{
    /*! \brief Reference to an INIC instance */
    CInic *inic_ptr;
    /*! \brief Reference to a base instance */
    CBase *base_ptr;
    /*!< \brief Reference to NSM instance */
    CNodeScriptManagement *nsm_ptr;
    /*!< \brief Current user data */
    I2c_UserData_t curr_user_data;
    /*!< \brief Indicates the address of target device */
    uint16_t device_address;
    /*! \brief Observer used for I2C to check the GPIO TriggerEvents  */
    CObserver triggerevent_observer;
    /*!< \brief Current script to be looked for */
    I2c_Script_t curr_script;
    /*!< \brief Current reference to the result callback function */
    I2c_ErrResultCb_t curr_res_cb;

} CI2c;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CI2c                                                                       */
/*------------------------------------------------------------------------------------------------*/
extern void I2c_Ctor(CI2c * self, I2c_InitData_t * init_ptr);
extern Ucs_Return_t I2c_CreatePort(CI2c * self, uint8_t index, Ucs_I2c_Speed_t speed, uint8_t i2c_int_mask, Ucs_I2c_CreatePortResCb_t res_fptr);
extern Ucs_Return_t I2c_WritePort(CI2c * self, uint16_t port_handle, Ucs_I2c_TrMode_t mode, uint8_t block_count, uint8_t slave_address, uint16_t timeout, 
                                  uint8_t data_len, uint8_t data_ptr[], Ucs_I2c_WritePortResCb_t res_fptr);
extern Ucs_Return_t I2c_ReadPort(CI2c * self, uint16_t port_handle, uint8_t slave_address, uint8_t data_len, uint16_t timeout, Ucs_I2c_ReadPortResCb_t res_fptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_I2C_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

