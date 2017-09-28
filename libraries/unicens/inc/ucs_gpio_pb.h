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
 * \brief Public header file of the Gpio module.
 * \addtogroup G_UCS_GPIO_TYPES
 * @{
 */

#ifndef UCS_GPIO_PB_H
#define UCS_GPIO_PB_H

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes of the GPIO Module. */
typedef enum Ucs_Gpio_ResultCode_
{
    UCS_GPIO_RES_SUCCESS             = 0x00U,  /*!< \brief GPIO command succeeded */
    UCS_GPIO_RES_ERR_CMD             = 0x01U,  /*!< \brief GPIO command failed due to an INIC function-specific error or a transmission error on the MOST network.
                                                *  \details The \em result_type section in Ucs_Gpio_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                */
    UCS_GPIO_RES_ERR_SYNC            = 0x02U   /*!< \brief Remote synchronization of target device failed.
                                                *  \details The \em inic_result section in Ucs_Gpio_ResultDetails_t will provide you with more detailed information concerning this error code.
                                                */
} Ucs_Gpio_ResultCode_t;

/*! \brief This enumerator specifies the kind of result - Target or Transmission. */
typedef enum Ucs_Gpio_ResultType_
{
    UCS_GPIO_RESULT_TYPE_TGT        = 0x00U,     /*!< \brief Specifies the target results, typically INIC function-specific error from target device. */
    UCS_GPIO_RESULT_TYPE_TX         = 0x01U      /*!< \brief Specifies the transmission error information that occurred on the MOST network. */

} Ucs_Gpio_ResultType_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Detailed information used for GPIO results. */
typedef struct Ucs_Gpio_ResultDetails_
{
    /*! \brief Specifies the type of the current asynchronous result.
     *  \details The following briefly describes the different types of results available:
     *              - \b UCS_GPIO_RESULT_TYPE_TGT: target results, typically INIC function-specific error found on target device. \n Refer to \em inic_result to get the detailed information.
     *              - \b UCS_GPIO_RESULT_TYPE_TX:  transmission results, typically transmission error on the MOST network. \n Refer to \em tx_result to get the transmission information.
     */
    Ucs_Gpio_ResultType_t result_type;
    /*! \brief Holds the status of the transmission. */
    Ucs_MsgTxStatus_t tx_result;
    /*! \brief Holds the results of the target device. */
    Ucs_StdResult_t inic_result;

} Ucs_Gpio_ResultDetails_t;

/*! \brief Result structure of the GPIO Module. */
typedef struct Ucs_Gpio_Result_
{
    /*! \brief Result code. */
    Ucs_Gpio_ResultCode_t code;
    /*! \brief Detailed information on the returned result. */
    Ucs_Gpio_ResultDetails_t details;

} Ucs_Gpio_Result_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature of result callback used by Ucs_Gpio_CreatePort()
 *  \param  node_address     The node address of the device from where the results come
 *  \param  gpio_port_handle The port resource handle.
 *  \param  result           The operation result
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Gpio_CreatePortResCb_t)(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_Result_t result, void *user_ptr);

/*! \brief Function signature of result callback used by Ucs_Gpio_SetPinMode() and Ucs_Gpio_GetPinMode().
 *  \param  node_address     The node address of the device from where the results come
 *  \param  gpio_port_handle The port resource handle.
 *  \param  pin              The pin that has been configured.
 *  \param  mode             The mode of the GPIO pin.
 *  \param  result            The operation result
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Gpio_ConfigPinModeResCb_t)(uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_PinConfiguration_t pin_cfg_list[], uint8_t list_sz, Ucs_Gpio_Result_t result, void *user_ptr);

/*! \brief Function signature of result callback used by Ucs_Gpio_WritePort() and Ucs_Gpio_ReadPort().
 *  \param  node_address     The node address of the device from where the results come
 *  \param  gpio_port_handle The port resource handle.
 *  \param  current_state    The current state of the GPIO pin
 *  \param  sticky_state     The sticky state of all GPIO pins configured as sticky inputs.
 *  \param  result            The operation result
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Gpio_PinStateResCb_t)(uint16_t node_address, uint16_t gpio_port_handle, uint16_t current_state, uint16_t sticky_state, Ucs_Gpio_Result_t result, void *user_ptr);

/*! \brief Function signature of result callback used by Gpio_TriggerEvents()
 *  \param  node_address     The node address of the device from where the results come
 *  \param  gpio_port_handle The port resource handle.
 *  \param  rising_edges     The GPIO pins on which a rising-edge trigger condition was detected
 *  \param  falling_edges    The GPIO pins on which a falling-edge trigger condition was detected
 *  \param  levels           The GPIO pins on which a logic level condition was detected
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Gpio_TriggerEventResultCb_t)(uint16_t node_address, uint16_t gpio_port_handle, uint16_t rising_edges, uint16_t falling_edges, uint16_t levels, void *user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_GPIO_PB_H */

/*!
 * @}
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

