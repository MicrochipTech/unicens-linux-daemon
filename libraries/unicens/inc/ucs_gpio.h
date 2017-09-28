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
 * \brief Internal header file of the GPIO module.
 *
 * \defgroup   G_UCS_GPIO_TYPES GPIO Referred Types
 * \brief      Referred types used by the Extended Resource Manager.
 * \ingroup    G_UCS_GPIO
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_GPIO
 * @{
 */


#ifndef UCS_GPIO_H
#define UCS_GPIO_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nsm.h"
#include "ucs_ret_pb.h"
#include "ucs_obs.h"
#include "ucs_gpio_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for GPIO results in case error.
 *  \param   self      Reference to CGpio instance
 *  \param   msg_ptr   Pointer to received message
 */
typedef void (*Gpio_ErrResultCb_t)(void *self, void *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Stores data required by GPIO during initialization. */
typedef struct Gpio_InitData_
{
    /*!< \brief Reference to INIC instance */
    CInic *inic_ptr;
    /*!< \brief Reference to NSM instance */
    CNodeScriptManagement *nsm_ptr;
    /*!< \brief GPIO Trigger event status function pointer */
    Ucs_Gpio_TriggerEventResultCb_t trigger_event_status_fptr;

} Gpio_InitData_t;

/*! \brief  Stores data required by GPIO during initialization. */
typedef struct Gpio_UserData_
{
    /*!< \brief PinState Result callback */
    Ucs_Gpio_PinStateResCb_t pinstate_res_cb;
    /*!< \brief PinMode Result callback */
    Ucs_Gpio_ConfigPinModeResCb_t pinmode_res_cb;
    /*!< \brief PortCreate Result callback */
    Ucs_Gpio_CreatePortResCb_t portcreate_res_cb;
    /*!< \brief GPIO Trigger event status function pointer */
    Ucs_Gpio_TriggerEventResultCb_t trigger_event_status_fptr;

} Gpio_UserData_t;

/*! \brief  Script structure of the GPIO module */
typedef struct Gpio_Script_
{
    uint8_t cfg_data[40];
    /*! \brief script used for transmitting commands */
    Ucs_Ns_Script_t script;
    /*! \brief config messages used for transmitting commands */
    Ucs_Ns_ConfigMsg_t cfg_msg;
} Gpio_Script_t;

/*! \brief  Class structure of the GPIO module */
typedef struct CGpio_
{
    /*! \brief Reference to an INIC instance */
    CInic *inic_ptr;
    /*!< \brief Reference to NSM instance */
    CNodeScriptManagement *nsm_ptr;
    /*! \brief Current user data */
    Gpio_UserData_t curr_user_data;
    /*! \brief Indicates the address of the target device */
    uint16_t device_address;
    /*! \brief Observer used for GPIO TriggerEvents  */
    CObserver triggerevent_observer;
    /*!< \brief Current script to be looked for */
    Gpio_Script_t curr_script;
    /*!< \brief Current reference to the result callback function */
    Gpio_ErrResultCb_t curr_res_cb;

} CGpio;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CGpio                                                                      */
/*------------------------------------------------------------------------------------------------*/
extern void Gpio_Ctor(CGpio * self, Gpio_InitData_t * init_ptr);
extern Ucs_Return_t Gpio_CreatePort(CGpio * self, uint8_t index, uint16_t debounce_time, Ucs_Gpio_CreatePortResCb_t res_fptr);
extern Ucs_Return_t Gpio_SetPinModeConfig(CGpio * self, uint16_t gpio_port_handle, uint8_t pin, Ucs_Gpio_PinMode_t mode, Ucs_Gpio_ConfigPinModeResCb_t res_fptr);
extern Ucs_Return_t Gpio_GetPinModeConfig(CGpio * self, uint16_t gpio_port_handle, Ucs_Gpio_ConfigPinModeResCb_t res_fptr);
extern Ucs_Return_t Gpio_SetPinStateConfig(CGpio * self, uint16_t gpio_port_handle, uint16_t mask, uint16_t data, Ucs_Gpio_PinStateResCb_t res_fptr);
extern Ucs_Return_t Gpio_GetPinStateConfig(CGpio * self, uint16_t gpio_port_handle, Ucs_Gpio_PinStateResCb_t res_fptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_GPIO_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

