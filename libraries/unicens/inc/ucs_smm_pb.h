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
 * \brief      Header file of the static memory manager plug-in.
 * \addtogroup G_UCS_AMS
 * @{
 */

#ifndef UCS_SMM_PB_H
#define UCS_SMM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Default configuration                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \def     UCS_AMS_NUM_RX_MSGS
 *  \brief   Defines the number of reserved Rx message objects. 
 *           Valid values: 5..255. Default value: 20.
 */
#ifndef UCS_AMS_NUM_RX_MSGS
#   define UCS_AMS_NUM_RX_MSGS  20
#else
#  if (UCS_AMS_NUM_RX_MSGS < 5) || (UCS_AMS_NUM_RX_MSGS > 255)
#    error "UCS_AMS_NUM_RX_MSGS is not properly defined. Choose a value between: 10 and 255."
#  endif
#endif

/*! \def     UCS_AMS_SIZE_RX_MSG
 *  \brief   Defines the payload size in bytes which is available for every Rx message object.
 *           Valid values: 45..65535. Default value: 45.
 */
#ifndef UCS_AMS_SIZE_RX_MSG
#   define UCS_AMS_SIZE_RX_MSG 45
#else
#  if (UCS_AMS_SIZE_RX_MSG >= 45) && (UCS_AMS_SIZE_RX_MSG <= 65535)
#    if defined(SMM_FOOTPRINT_TINY) && (UCS_AMS_SIZE_RX_MSG != 45)
#      error Do not define UCS_AMS_SIZE_RX_MSG together with SMM_FOOTPRINT_TINY.
#    endif
#  else
#    error UCS_AMS_SIZE_RX_MSG is not properly defined. Choose a value between: 45 and 65535.
#  endif
#endif

/*! \def     UCS_AMS_NUM_TX_MSGS
 *  \brief   Defines the number of reserved Tx message objects.
 *           Valid values: 5..255. Default value: 20.
 */
#ifndef UCS_AMS_NUM_TX_MSGS
#   define UCS_AMS_NUM_TX_MSGS  20
#else
#  if (UCS_AMS_NUM_TX_MSGS < 5) || (UCS_AMS_NUM_TX_MSGS > 255)
#    error "UCS_AMS_NUM_TX_MSGS is not properly defined. Choose a value between: 10 and 255."
#  endif
#endif

/*! \def     UCS_AMS_SIZE_TX_MSG
 *  \brief   Defines the payload size in bytes which is available for every Tx message object.
 *           Valid values: 45..65535. Default value: 45.
 */
#ifndef UCS_AMS_SIZE_TX_MSG
#   define UCS_AMS_SIZE_TX_MSG 45
#else
#  if (UCS_AMS_SIZE_TX_MSG >= 45) && (UCS_AMS_SIZE_TX_MSG <= 65535)
#    if defined(SMM_FOOTPRINT_TINY) && (UCS_AMS_SIZE_TX_MSG != 45)
#      error "Do not define UCS_AMS_SIZE_TX_MSG together with SMM_FOOTPRINT_TINY."
#    endif
#  else
#    error "UCS_AMS_SIZE_TX_MSG is not properly defined. Choose a value between: 45 and 65535."
#  endif
#endif

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_SMM_PB_H */

/*!
 * @}
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

