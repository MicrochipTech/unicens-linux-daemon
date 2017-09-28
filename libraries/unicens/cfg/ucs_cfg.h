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

#ifndef UCS_CFG_H
#define UCS_CFG_H

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"

/*------------------------------------------------------------------------------------------------*/
/* Multi Instance API                                                                             */
/*------------------------------------------------------------------------------------------------*/
/* Number of API instances which can be created by function Ucs_CreateInstance().
 * One API instance is used to communicate with one local INIC. In this case the application
 * is connected to one network.
 * It is possible access multiple networks by having multiple API instances. Each API instance
 * requires communication with an exclusive INIC.
 * Valid range: 1..10. Default value: 1.
 */
#define UCS_NUM_INSTANCES                2

/*------------------------------------------------------------------------------------------------*/
/* Resources Management                                                                           */
/*------------------------------------------------------------------------------------------------*/
/* Maximum number of remote devices used by Resources Management modules.
 * Valid range: 0..63. Default value: 0.
 */
#define UCS_NUM_REMOTE_DEVICES            63

/*------------------------------------------------------------------------------------------------*/
/* Application Messages                                                                           */
/*------------------------------------------------------------------------------------------------*/
/* Defines the number of reserved Rx message objects. 
 * Valid values: 5..255. Default value: 20.
 */
#define UCS_AMS_NUM_RX_MSGS              20

/* Defines the payload size in bytes which is available for every Rx message object.
 * Valid values: 45..65535. Default value: 45.
 */
#define UCS_AMS_SIZE_RX_MSG              1024

/* Defines the number of reserved Tx message objects.
 * Valid values: 5..255. Default value: 20.
 */
#define UCS_AMS_NUM_TX_MSGS              20

/* Defines the payload size in bytes which is available for every Tx message object.
 * Valid values: 45..65535. Default value: 45.
 */
#define UCS_AMS_SIZE_TX_MSG              UCS_AMS_SIZE_RX_MSG

/*------------------------------------------------------------------------------------------------*/
/* Memory Optimization                                                                            */
/*------------------------------------------------------------------------------------------------*/
/* Define the following macros to reduces the RAM and ROM size of the UNICENS software by disabling 
 * certain features. If this macro is defined the following changes apply:
 * - Reduction of low-level buffers
 * - AMS does not support segmentation (payload > 45 bytes)
 */
/* #define UCS_FOOTPRINT_TINY */

/*------------------------------------------------------------------------------------------------*/
/* Tracing & Debugging                                                                            */
/*------------------------------------------------------------------------------------------------*/
/* Define the following macros to map info and error trace output to user defined functions. 
 * The purpose of these functions is debugging. It is not recommended to define these functions 
 * in a production system.
 */
 #define UCS_TR_ERROR     App_TraceError
 /*#define UCS_TR_INFO      App_TraceInfo*/

extern void App_TraceError(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...);
extern void App_TraceInfo(void *ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...);

#ifdef __cplusplus
}
#endif

#endif /* UCS_CFG_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

