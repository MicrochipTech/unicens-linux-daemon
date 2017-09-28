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
 * \brief This header file contains standard return values used by UNICENS functions 
 *        and methods.
 * \addtogroup G_UCS_INIT_AND_SRV_TYPES
 * @{
 */

#ifndef UCS_RET_H
#define UCS_RET_H

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Standard return codes used for synchronous response */
typedef enum Ucs_Return_
{
    UCS_RET_SUCCESS             = 0x00,     /*!< \brief Operation successfully completed */
    UCS_RET_ERR_PARAM           = 0x01,     /*!< \brief At least one parameter exceeds its 
                                                        admissible range */
    UCS_RET_ERR_BUFFER_OVERFLOW = 0x02,     /*!< \brief Buffer overflow or service busy */
    UCS_RET_ERR_NOT_AVAILABLE   = 0x03,     /*!< \brief Functionality not available */
    UCS_RET_ERR_NOT_SUPPORTED   = 0x04,     /*!< \brief This function is not supported by this 
                                                        derivative of INIC / physical layer / MOST 
                                                        speed */
    UCS_RET_ERR_INVALID_SHADOW  = 0x05,     /*!< \brief The requested information is not yet 
                                                        available */
    UCS_RET_ERR_ALREADY_SET     = 0x06,     /*!< \brief The value to be set is already set. The 
                                                        application can therefore be aware that no 
                                                        message will be send to INIC and no 
                                                        callback will be called */
    UCS_RET_ERR_API_LOCKED      = 0x07,     /*!< \brief INIC performs already requested function. */
    UCS_RET_ERR_NOT_INITIALIZED = 0x08      /*!< \brief UNICENS is not initialized */

} Ucs_Return_t;

/*! \brief Result codes used for asynchronous response */
typedef enum Ucs_Result_
{
    UCS_RES_SUCCESS           = 0x00,       /*!< \brief Operation successfully completed */
    UCS_RES_ERR_MOST_STANDARD = 0x01,       /*!< \brief MOST standard error occurred */
    UCS_RES_ERR_BUSY          = 0x02,       /*!< \brief Function currently busy */
    UCS_RES_ERR_PROCESSING    = 0x03,       /*!< \brief Processing error occurred */
    UCS_RES_ERR_CONFIGURATION = 0x04,       /*!< \brief Configuration error occurred */
    UCS_RES_ERR_SYSTEM        = 0x05,       /*!< \brief System error occurred */
    UCS_RES_ERR_TIMEOUT       = 0x06,       /*!< \brief Timeout occurred */
    UCS_RES_ERR_TRANSMISSION  = 0x07        /*!< \brief Transmission error occurred */

} Ucs_Result_t;

/*! \brief Result values of initialization result callback function */
typedef enum Ucs_InitResult_
{
    UCS_INIT_RES_SUCCESS          = 0x00U,      /*!< \brief Initialization succeeded */
    UCS_INIT_RES_ERR_BUF_OVERFLOW = 0x01U,      /*!< \brief No message buffer available */
    UCS_INIT_RES_ERR_INIC_SYNC    = 0x02U,      /*!< \brief INIC synchronization failed */
    UCS_INIT_RES_ERR_INIC_VERSION = 0x03U,      /*!< \brief INIC device version check failed */
    UCS_INIT_RES_ERR_INIC_SYSTEM  = 0x04U,      /*!< \brief Device attach failed due to an INIC 
                                                 *          system error
                                                 */
    UCS_INIT_RES_ERR_DEV_ATT_CFG  = 0x05U,      /*!< \brief INIC device attach failed due to an 
                                                 *          configuration error
                                                 */
    UCS_INIT_RES_ERR_DEV_ATT_PROC = 0x06U,      /*!< \brief Device attach failed due to a 
                                                 *          processing error
                                                 */
    UCS_INIT_RES_ERR_NET_CFG      = 0x07U,      /*!< \brief Network Configuration request failed */
    UCS_INIT_RES_ERR_INTERNAL     = 0x0AU,      /*!< \brief Internal error occurred */
    UCS_INIT_RES_ERR_TIMEOUT      = 0x0BU       /*!< \brief Initialization timeout occurred */

} Ucs_InitResult_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Standard result structure which provides fields for detailed status and 
 *          error information
 */
typedef struct Ucs_StdResult_
{
    Ucs_Result_t code;  /*!< \brief Result/Error code */
    uint8_t *info_ptr;  /*!< \brief INIC error data */
    uint8_t info_size;  /*!< \brief Size of the INIC error data in bytes */

} Ucs_StdResult_t;

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for UNICENS standard result callbacks
 *  \param  result      Result of the callback
 *  \param  user_ptr    User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_StdResultCb_t)(Ucs_StdResult_t result, void *user_ptr);

/*! \brief  Function signature used for UNICENS standard result callbacks
 *  \param  result      Result of the callback
 *  \param  user_ptr    User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_InitResultCb_t)(Ucs_InitResult_t result, void *user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_RET_H */

/*!
 * @}
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

