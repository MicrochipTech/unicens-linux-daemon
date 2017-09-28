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
 * \brief       Declaration of the low-level driver interface
 *
 * \addtogroup  G_UCS_LLD
 * @{
 * \details     UNICENS provides a certain set of functions which are only dedicated to the low-level driver.
 *              The low-level driver \em API is a set of functions which shall be used by the low-level driver.
 *              The low-level driver \em callbacks is a set of function that shall be implemented by the low-level driver.
 *              The low-level driver \em callbacks shall be assigned to the UNICENS initialization structure.
 *              During initialization UNICENS invokes the callback \ref Ucs_Lld_Callbacks_t "start_fptr" and
 *              passes the low-level driver \em API as pointer to \ref Ucs_Lld_Api_t.
 * <!--
 *              \mns_ic_started{ See also Getting Started with \ref P_UM_STARTED_LLD. }
 *              \mns_ic_examples{ See also <i>Examples</i>, section \ref P_UM_EXAMPLE_LLD_01, \ref P_UM_EXAMPLE_LLD_02 and \ref P_UM_EXAMPLE_LLD_03. }
 * -->
 * @}
 */

#ifndef UCS_LLD_PB_H
#define UCS_LLD_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"
#include "ucs_memory_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * \addtogroup  G_UCS_LLD_TYPES
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Tx message object providing the raw port message byte stream */
typedef struct Ucs_Lld_TxMsg_
{
    struct Ucs_Lld_TxMsg_ *custom_next_msg_ptr;/*!< \brief     Shall be used by the LLD implementation to queue messages for
                                                *              asynchronous transmission
                                                *   \details   UNICENS will set this value to \c NULL since only 
                                                *              single messages are forwarded to the LLD. Within the transmit function 
                                                *              it is recommended that the LLD queues the message for asynchronous 
                                                *              transmission. Despite a driver's transmit function might signal busy for 
                                                *              a short term the UNICENS library might forward multiple messages for 
                                                *              transmission. If a driver works asynchronously (interrupt driven) it
                                                *              can easily use this pointer build a queue of waiting messages. 
                                                *              Nonetheless, it is important that \ref Ucs_Lld_Api_t::tx_release_fptr
                                                *              "tx_release_fptr" is invoked for every message separately. The Interface 
                                                *              between the UNICENS library and the LLD does only support single messages.
                                                */
    Ucs_Mem_Buffer_t *memory_ptr;              /*!< \brief     Points to the data buffer */

} Ucs_Lld_TxMsg_t;

/*! \brief  Rx message object pointing to the raw port message byte stream. */
typedef struct Ucs_Lld_RxMsg_
{
    uint8_t*        data_ptr;       /*!< \brief Points to a UNICENS allocated memory chunk. */
    uint16_t        data_size;      /*!< \brief Size of the memory chunk in bytes. Valid values: 6..72. */

} Ucs_Lld_RxMsg_t;

/*!
 * @} 
 * \addtogroup  G_UCS_LLD_API
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Low-level driver API                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Allocates an Rx message object 
 *  \param  inst_ptr    Reference to an internal UNICENS handler
 *  \param  buffer_size The size in bytes of the received Rx message. 
 *                      Valid values: 6..72.
 *  \return The Rx message object or \c NULL if no message object is available. In the latter
 *          case the low-level driver can wait until Ucs_Lld_RxMsgAvailableCb_t() is invoked.
 *          The low-level driver is allowed to pre-allocate Rx messages with the maximum size
 *          of 72 bytes. After writing received data into Ucs_Lld_RxMsg_t::data_ptr the
 *          low-level driver must set Ucs_Lld_RxMsg_t::data_size to the actual message size.
 *          \warning
 *          The function will also return \c NULL if the requested \c buffer_size exceeds the valid range.
 *          In such a case the UNICENS cannot guarantee that Ucs_Lld_RxMsgAvailableCb_t() is 
 *          called as expected. Received messages exceeding the valid range must be discarded by the LLD.
 */
typedef Ucs_Lld_RxMsg_t* (*Ucs_Lld_RxAllocateCb_t)(void *inst_ptr, uint16_t buffer_size);

/*! \brief  Frees an unused Rx message object
 *  \param  inst_ptr    Reference to internal UNICENS handler
 *  \param  msg_ptr     Reference to the unused Rx message object
 */
typedef void (*Ucs_Lld_RxFreeUnusedCb_t)(void *inst_ptr, Ucs_Lld_RxMsg_t *msg_ptr);

/*! \brief  Pass an Rx message to UNICENS
 *  \param  inst_ptr    Reference to internal UNICENS handler
 *  \param  msg_ptr     Reference to the Rx message object containing the received
 *                      message.
 */
typedef void (*Ucs_Lld_RxReceiveCb_t)(void *inst_ptr, Ucs_Lld_RxMsg_t *msg_ptr);

/*! \brief  Notifies that the LLD no longer needs to access the Tx message object
 *  \param  inst_ptr    Reference to internal UNICENS handler
 *  \param  msg_ptr     Reference to the Tx message object which is no longer accessed
 *                      by the low-level driver
 */
typedef void (*Ucs_Lld_TxReleaseCb_t)(void *inst_ptr, Ucs_Lld_TxMsg_t *msg_ptr);

/*! \brief   Initialization required for one communication channel (control or packet)
 */
typedef struct Ucs_Lld_Api_
{
    Ucs_Lld_RxAllocateCb_t   rx_allocate_fptr;     /*!< \brief  Allocates an Rx message object */
    Ucs_Lld_RxFreeUnusedCb_t rx_free_unused_fptr;  /*!< \brief  Frees an unused Rx message object */
    Ucs_Lld_RxReceiveCb_t    rx_receive_fptr;      /*!< \brief  Pass an Rx message to the UNICENS library */
    Ucs_Lld_TxReleaseCb_t    tx_release_fptr;      /*!< \brief  Notifies that the LLD no longer needs to access the Tx message object */

} Ucs_Lld_Api_t;

/*!
 * @} 
 * \addtogroup  G_UCS_LLD
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* LLD interface functions                                                                        */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      Notifies the LLD to start transmitting and receiving messages
 *  \param      api_ptr         Reference to UNICENS LLD interface
 *  \param      inst_ptr        Reference to internal UNICENS handler
 *  \param      lld_user_ptr    User defined pointer which is provided in \ref Ucs_Lld_Callbacks_t structure.
 */
typedef void (*Ucs_Lld_StartCb_t)(Ucs_Lld_Api_t* api_ptr, void *inst_ptr, void *lld_user_ptr);

/*! \brief      Notifies the LLD to stop/abort transmitting and receiving messages
 *  \details    As soon as this function is called the low-level driver is not allowed 
 *              to call any UNICENS API function.
 *  \param      lld_user_ptr    User defined pointer which is provided in \ref Ucs_Lld_Callbacks_t structure.
 */
typedef void (*Ucs_Lld_StopCb_t)(void *lld_user_ptr);

/*! \brief      Notifies the LLD to reset the INIC
 *  \details    If this function is called the low-level driver is responsible to 
 *              perform an INIC hardware reset.
 *  \param      lld_user_ptr    User defined pointer which is provided in \ref Ucs_Lld_Callbacks_t structure.
 */
typedef void (*Ucs_Lld_ResetInicCb_t)(void *lld_user_ptr);

/*! \brief      Callback function which is invoked as soon as port message objects are available again.
 *  \details    By implementing this callback function the low-level driver can avoid polling for 
 *              Rx message objects. The low-level driver should wait for the function call as soon 
 *              as Ucs_Lld_RxAllocateCb_t() returns NULL. Only then it shall call those functions again.
 *  \param      lld_user_ptr    User defined pointer which is provided in \ref Ucs_Lld_Callbacks_t structure.
 */
typedef void (*Ucs_Lld_RxMsgAvailableCb_t)(void *lld_user_ptr);

/*! \brief      Callback function which is invoked to transmit a single message to the INIC
 *  \param      msg_ptr         Reference to a single Tx message.
 *  \param      lld_user_ptr    User defined pointer which is provided in \ref Ucs_Lld_Callbacks_t structure.
 */
typedef void (*Ucs_Lld_TxTransmitCb_t)(Ucs_Lld_TxMsg_t *msg_ptr, void *lld_user_ptr);

/*!
 * @} 
 * \addtogroup  G_UCS_LLD_TYPES
 * @{
 */

/*! \brief  Set of functions implemented by the low-level driver
 */
typedef struct Ucs_Lld_Callbacks_
{
    void                      *lld_user_ptr;       /*!< \brief    Optional pointer that is passed when invoking a callback function which is assigned in Ucs_Lld_Callbacks_t. */
    Ucs_Lld_StartCb_t          start_fptr;         /*!< \brief    Callback function to initialize the low-level driver and 
                                                    *             start the transmission and reception of messages */
    Ucs_Lld_StopCb_t           stop_fptr;          /*!< \brief    Callback function to stop/abort the transmission and reception of messages */
    Ucs_Lld_RxMsgAvailableCb_t rx_available_fptr;  /*!< \brief    Callback function which is invoked as soon as Rx message objects are available again */
    Ucs_Lld_TxTransmitCb_t     tx_transmit_fptr;   /*!< \brief    Callback function to transmit one or multiple messages to the INIC */

} Ucs_Lld_Callbacks_t;

/*! @} */

#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_LLD_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

