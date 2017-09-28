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
 * \brief Header file of the Command Interpreter.
 *
 */


#ifndef UCS_CMD_PB_H
#define UCS_CMD_PB_H

#include "ucs_ams_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*------------------------------------------------------------------------------------------------*/
/* Constants                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Denotes the end of an MessageId Table
 *  \ingroup G_UCS_CMD_TYPES
 */
#define UCS_CMD_MSGID_TERMINATION   0xFFFFU



/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes used for Command Interpreter API functions
 *  \ingroup G_UCS_CMD_TYPES
 */
typedef enum Ucs_Cmd_Return_
{
    UCS_CMD_RET_SUCCESS             = 0x00,     /*!< Operation successfully completed */
    UCS_CMD_RET_ERR_MSGID_NOTAVAIL  = 0x01,     /*!< MessageId not found */
    UCS_CMD_RET_ERR_TX_BUSY         = 0x02,     /*!< No free Tx buffer available */
    UCS_CMD_RET_ERR_APPL            = 0x03,     /*!< Application handler function reports custom error */
    UCS_CMD_RET_ERR_ALREADY_ENTERED = 0x04,     /*!< MessageId Table already connected */
    UCS_CMD_RET_ERR_NULL_PTR        = 0x05      /*!< NULL pointer used as argument */
} Ucs_Cmd_Return_t;

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Type definition of user handler functions
 *  \param   msg_rx_ptr     Reference to the received message
 *  \param   user_ptr      User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \return  Possible return values are shown in the table below.
 *  Value                           | Description 
 *  ------------------------------- | ------------------------------------
 *  UCS_CMD_RET_SUCCESS             | The handler function succeeded.
 *  UCS_CMD_RET_ERR_TX_BUSY         | The handler function could not send an answer because no free Tx Buffer was available.
 *  UCS_CMD_RET_ERR_APPL            | An error happened in handler function. 
 *  \note    The application must not return other values than the ones listed above.
 *  \ingroup G_UCS_CMD_TYPES
 */
typedef Ucs_Cmd_Return_t (*Ucs_Cmd_Handler_Function_t)(Ucs_AmsRx_Msg_t *msg_rx_ptr, void *user_ptr);



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure of a single element of the MessageId Table
 *  \details The application provides a MessageId Table which contains all supported MessageIds
 *           with their belonging handler functions. The MessageId Table is an array of several 
 *           Ucs_Cmd_MsgId_t elements. It has to end with a termination entry with the 
 *           value {\ref UCS_CMD_MSGID_TERMINATION, NULL}.
 *  \ingroup G_UCS_CMD_TYPES
 */
typedef struct Ucs_Cmd_MsgId_
{
    /*! \brief MessageId */
    uint16_t msg_id;
    /*! \brief Pointer to the belonging handler function */
    Ucs_Cmd_Handler_Function_t handler_function_ptr;

} Ucs_Cmd_MsgId_t;




/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/



#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_CMD_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

