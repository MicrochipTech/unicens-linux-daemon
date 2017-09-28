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
 * \brief Public header file of the I2c module.
 * \addtogroup G_UCS_I2C_TYPES
 * @{
 */

#ifndef UCS_I2C_PB_H
#define UCS_I2C_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes of the I2C Module. */
typedef enum Ucs_I2c_ResultCode_
{
    UCS_I2C_RES_SUCCESS             = 0x00U,   /*!< \brief I2C command succeeded */
    UCS_I2C_RES_ERR_CMD             = 0x01U,   /*!< \brief I2C command failed due to an INIC function-specific error or a transmission error on the MOST network.
                                                *  \details The \em result_type section in Ucs_I2c_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                */
    UCS_I2C_RES_ERR_SYNC            = 0x02U    /*!< \brief Remote synchronization of target device failed.
                                                *  \details The \em inic_result section in Ucs_I2c_ResultDetails_t will provide you with more detailed information concerning the error code.
                                                */
} Ucs_I2c_ResultCode_t;

/*! \brief This enumerator specifies the kind of result - Target or Transmission. */
typedef enum Ucs_I2c_ResultType_
{
    UCS_I2C_RESULT_TYPE_TGT        = 0x00U,     /*!< \brief Specifies the target results, typically INIC function-specific error from target device. */
    UCS_I2C_RESULT_TYPE_TX         = 0x01U      /*!< \brief Specifies the transmission error information that occurred on the MOST network. */

} Ucs_I2c_ResultType_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Detailed information used for I2C results. */
typedef struct Ucs_I2c_ResultDetails_
{
    /*! \brief Specifies the type of the current asynchronous result.
     *  \details The following briefly describes the different types of results available:
     *              - \b UCS_I2C_RESULT_TYPE_TGT: target results, typically INIC function-specific error found on target device. \n Refer to \em inic_result to get the detailed information.
     *              - \b UCS_I2C_RESULT_TYPE_TX:  transmission results, typically transmission error on the MOST network. \n Refer to \em tx_result to get the transmission information.
     */
    Ucs_I2c_ResultType_t result_type;
    /*! \brief Holds the status of the transmission. */
    Ucs_MsgTxStatus_t tx_result;
    /*! \brief Holds the results of the target device. */
    Ucs_StdResult_t inic_result;

} Ucs_I2c_ResultDetails_t;

/*! \brief Result structure of the I2C Module. */
typedef struct Ucs_I2c_Result_
{
    /*! \brief Result code. */
    Ucs_I2c_ResultCode_t code;
    /*! \brief Detailed information on the returned result. */
    Ucs_I2c_ResultDetails_t details;

} Ucs_I2c_Result_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function type to retrieve the result of the I2c_CreatePort function
 *  \param  node_address     The node address of the device from where the results come
 *  \param  i2c_port_handle  The port resource handle.
 *  \param  result           The operation result
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_I2c_CreatePortResCb_t)(uint16_t node_address, uint16_t i2c_port_handle, Ucs_I2c_Result_t result, void *user_ptr);

/*! \brief  Callback function type to retrieve the result of the Gpio_ConfigPinMode function
 *  \param  node_address        The node address of the device from where the results come
 *  \param  i2c_port_handle     The port resource handle.
 *  \param  i2c_slave_address   The 7-bit I2C Port slave address of the peripheral to be read.
 *  \param  data_len            The number of bytes wrote to the I2C address.
 *  \param  result              The operation result
 *  \param  user_ptr            User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_I2c_WritePortResCb_t)(uint16_t node_address, uint16_t i2c_port_handle, uint8_t i2c_slave_address, uint8_t data_len, Ucs_I2c_Result_t result, void *user_ptr);

/*! \brief  Callback function type to retrieve the result of the Gpio_ConfigPinMode function
 *  \param  node_address        The node address of the device from where the results come
 *  \param  i2c_port_handle     The port resource handle.
 *  \param  i2c_slave_address   The 7-bit I2C Port slave address of the peripheral from which the data have been read.
 *  \param  data_len            The number of bytes read from the address.
 *  \param  data_ptr            The reference to the data list.
 *  \param  result              The operation result
 *  \param  user_ptr            User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_I2c_ReadPortResCb_t)(uint16_t node_address, uint16_t i2c_port_handle, uint8_t i2c_slave_address, uint8_t data_len, uint8_t data_ptr[], Ucs_I2c_Result_t result, void *user_ptr);

/*! \brief  Callback function type to report the I2C interrupt event
 *  \param  node_address     The node address of the device from where the interrupt comes
 *  \param  user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_I2c_IntEventReportCb_t)(uint16_t node_address, void *user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_I2C_PB_H */

/*! @} */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

