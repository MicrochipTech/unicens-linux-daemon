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
 * \brief Public header file of Application Message Service
 */
/*!
 * \addtogroup G_UCS_AMS_TYPES
 * @{
 */

#ifndef UCS_AMS_PB_H
#define UCS_AMS_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_message_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief   Defines which address type was used by the transmitter of a message. */
typedef enum Ucs_AmsRx_ReceiveType_
{
    UCS_AMSRX_RCT_SINGLECAST    = 0U,           /*!< \brief Message was transmitted as singlecast */
    UCS_AMSRX_RCT_GROUPCAST     = 1U,           /*!< \brief Message was transmitted as groupcast */
    UCS_AMSRX_RCT_BROADCAST     = 2U            /*!< \brief Message was transmitted as broadcast */

} Ucs_AmsRx_ReceiveType_t;

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Application message Tx type */
typedef struct Ucs_AmsTx_Msg_
{
    uint16_t        destination_address;        /*!< \brief   Destination address. */
    uint16_t        msg_id;                     /*!< \brief   16bit message descriptor */
    uint8_t         llrbc;                      /*!< \brief   Specifies the "Low-Level Retry Block Count" (LLRBC)
                                                 *   \details Valid values: 0..100. Default value: configurable via \ref Ucs_AmsTx_InitData_t "default_llrbc"
                                                 *            of the initialization structure \ref Ucs_AmsTx_InitData_t.
                                                 *            \mns_ic_inic{ See also <i>INIC API User's Guide</i>, section \ref SEC_IIC_18. }
                                                 */
    uint8_t        *data_ptr;                   /*!< \brief   Payload data */
    uint16_t        data_size;                  /*!< \brief   The size of payload data in bytes */
    void           *custom_info_ptr;            /*!< \brief   Customer specific reference 
                                                 *   \details The application is allowed to use this attribute to assign an
                                                 *            own reference to the message object. The reference is initialized 
                                                 *            by the UNICENS library with \c NULL and will not alter until the 
                                                 *            transmission has finished.
                                                 */
} Ucs_AmsTx_Msg_t;

/*! \brief Application message Rx type */
typedef struct Ucs_AmsRx_Msg_
{
    uint16_t        source_address;             /*!< \brief Source address */
    uint16_t        msg_id;                     /*!< \brief 16bit message descriptor */
    uint8_t        *data_ptr;                   /*!< \brief Reference to payload */
    uint16_t        data_size;                  /*!< \brief Payload size in bytes */
    void           *custom_info_ptr;            /*!< \brief Customer specific reference */
    Ucs_AmsRx_ReceiveType_t receive_type;       /*!< \brief Defines which address type was used by the transmitter of this message */

} Ucs_AmsRx_Msg_t;

/*! \brief Transmission result of an application message */
typedef enum Ucs_AmsTx_Result_
{
    UCS_AMSTX_RES_SUCCESS             = 0x00U,/*!< \brief   The transmission succeeded. */

    UCS_AMSTX_RES_ERR_RETRIES_EXP     = 0x01U,/*!< \brief   The transmission including all retries have failed.
                                               *   \details The following issues may have caused the failure:
                                               *            - message corruption
                                               *            - transmission timeouts
                                               *            - full receive buffers of the destination device
                                               *            - full receive buffers of the local device if the
                                               *              destination was the own address, own group or broadcast
                                               *              address
                                               *            .
                                               */
    UCS_AMSTX_RES_ERR_INVALID_TGT     = 0x02U,/*!< \brief   The transmission failed because the specified destination 
                                               *            address is not found or not valid.
                                               *   \details The following issues may have caused the failure:
                                               *            - device with the given destination address is not found 
                                               *            - destination address is reserved (for future use) 
                                               *            - destination address is 0xFFFF (un-initialized logical 
                                               *              node address is not supported)
                                               *            .
                                               */
    UCS_AMSTX_RES_ERR_NOT_AVAILABLE   = 0x03U,/*!< \brief   The transmission failed since the network or the INIC
                                               *            is not available.
                                               */
    UCS_AMSTX_RES_ERR_UNEXPECTED      = 0xFFU /*!< \brief   The transmission failed due to an unexpected error.
                                               *            The cause of this failure may be an invalid INIC configuration,
                                               *            or an INIC to UNICENS incompatibility issue.
                                               */
} Ucs_AmsTx_Result_t;


/*! \brief  Detailed INIC transmission information which might be useful for debugging purposes. */
typedef enum Ucs_AmsTx_Info_
{
    UCS_AMSTX_I_SUCCESS             = 0x00U, /*!< \brief   The transmission succeeded.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_SUCCESS. 
                                              */
    UCS_AMSTX_I_ERR_CFG_NORECEIVER  = 0x01U, /*!< \brief   The transmission failed because the MOST network is not accessible for
                                              *            MCM in the current attach state or for ICM in general.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_UNEXPECTED. 
                                              */
    UCS_AMSTX_I_ERR_BF              = 0x08U, /*!< \brief   The transmission failed because the receivers buffer is full.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_RETRIES_EXP. 
                                              */ 
    UCS_AMSTX_I_ERR_CRC             = 0x09U, /*!< \brief   The transmission failed because of a failed CRC.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_RETRIES_EXP. 
                                              */ 
    UCS_AMSTX_I_ERR_ID              = 0x0AU, /*!< \brief   The transmission failed because of corrupted identifiers.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_RETRIES_EXP. 
                                              */ 
    UCS_AMSTX_I_ERR_ACK             = 0x0BU, /*!< \brief   The transmission failed because of corrupted PACK or CACK.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_RETRIES_EXP. 
                                              */ 
    UCS_AMSTX_I_ERR_TIMEOUT         = 0x0CU, /*!< \brief   The transmission failed because of a transmission timeout.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_RETRIES_EXP. 
                                              */
    UCS_AMSTX_I_ERR_FATAL_WT        = 0x10U, /*!< \brief   The transmission failed because of destination is not available.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_INVALID_TGT. 
                                              */ 
    UCS_AMSTX_I_ERR_FATAL_OA        = 0x11U, /*!< \brief   The transmission failed because of the destination is the own node address. 
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_INVALID_TGT. 
                                              */
    UCS_AMSTX_I_ERR_UNAVAIL_TRANS   = 0x18U, /*!< \brief   The transmission canceled during the transition from network interface state
                                              *            "available" to "not available".
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_NOT_AVAILABLE. 
                                              */ 
    UCS_AMSTX_I_ERR_UNAVAIL_OFF     = 0x19U, /*!< \brief   The transmission failed because the network interface state is "not available".
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_NOT_AVAILABLE. 
                                              */
    UCS_AMSTX_I_ERR_UNKNOWN         = 0xFEU, /*!< \brief   The transmission failed because of an unknown INIC error code.
                                              *   \details The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_UNEXPECTED.
                                              *            Please check if the MNS version is compatible with the applied INIC firmware version.
                                              */
    UCS_AMSTX_I_ERR_UNSYNCED        = 0xFFU  /*!< \brief   The transmission failed because the communication between the EHC
                                              *            and the INIC is lost. 
                                              *   \details The reason can be a communication error between the EHC and the INIC or that 
                                              *            the application has called Ucs_Stop().\n
                                              *            The corresponding transmission result is \ref UCS_AMSTX_RES_ERR_NOT_AVAILABLE. 
                                              */
} Ucs_AmsTx_Info_t;

/*! \brief   Type of a callback function that is invoked as soon as a 
 *           message transmission was finished
 *  \details The callback function notifies the result of a completed transmission. If
 *           the message has external payload, the application must decide whether
 *           to re-use or to free the external payload.
 *  \param   msg_ptr  Reference to the related Tx message object. When the callback function returns
 *                    the reference is no longer valid.
 *  \param   result   The transmission result.
 *  \param   info     Detailed INIC transmission result, which might be helpful for debug purposes.
 *  \param   user_ptr User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_AmsTx_CompleteCb_t)(Ucs_AmsTx_Msg_t* msg_ptr, Ucs_AmsTx_Result_t result, Ucs_AmsTx_Info_t info, void *user_ptr);

/*!
 * @}
 * \addtogroup G_UCS_AMS
 * @{
 */

/*! \brief  Type of a callback function that is invoked to notify that
 *          a Tx application message object is available again while a previous
 *          allocation using Ucs_AmsTx_AllocMsg() has failed.
 *  \param  user_ptr    User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_AmsTx_MsgFreedCb_t)(void *user_ptr);

/*! \brief  Callback function type that is invoked if UNICENS has received a message 
 *          completely and appended to the Rx message queue.
 *  \param  user_ptr    User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_AmsRx_MsgReceivedCb_t)(void *user_ptr);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif  /* ifndef UCS_AMS_PB_H */

/*! @} */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

