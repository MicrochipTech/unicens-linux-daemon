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
 * \brief Internal header file of the trace interface
 */

#ifndef UCS_TRACE_H
#define UCS_TRACE_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_trace_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Unit and entry ids                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define TR_UCS_ASSERT             "ASSERT failed in line %d"
#define TR_UCS_INIC_RESULT_ID_1   "INIC error data:"
#define TR_UCS_INIC_RESULT_ID_2   "--> Data[%u]: 0x%02X"

/*------------------------------------------------------------------------------------------------*/
/* Internal macros                                                                                */
/*------------------------------------------------------------------------------------------------*/

/*! \def     UCS_TR_INFO
 *  \brief   Trace macro to capture trace info events
 *  \details This macro is used to enable the capturing of trace info events. The macro must be 
 *           mapped onto a user-defined function. To disable the trace info events, the macro must
 *           not be defined. The mapped user-defined function must adhere to the following function 
 *           signature.
 * 
 *           void (*Ucs_TraceCb_t)(void * ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...);
 *           - <b>ucs_user_ptr</b><br>Reference to the User argument
 *           - <b>module_str</b><br>The name of the software module that has posted the trace
 *           - <b>entry_str</b><br>The trace entry as formatted string
 *           - <b>vargs_cnt</b><br>Number of trace arguments which will be passed within the variable 
 *                                 argument list
 *           - <b>[...]</b><br>Variable argument list to pass trace arguments
 *
 *  \warning Do not assign UCS_TR_INFO in a production system. This has major effects on the CPU load and runtime.
 *           UCS_TR_INFO is intended for debugging software during development phase. Microchip Support might
 *           request you to assign of this macro to spy on internal events. Disable this macro definition after
 *           your support case is closed.
 *
 *           <b>Example:</b>
 *           \code
 *           extern void App_UcsTraceInfo(void * ucs_user_ptr,
 *                                        const char module_str[],
 *                                        const char entry_str[],
 *                                        uint16_t vargs_cnt,
 *                                        ...);
 *
 *           #define UCS_TR_INFO   App_UcsTraceInfo
 *           \endcode
 *
 *  \ingroup G_UCS_TRACE
 */

/*! \def     UCS_TR_ERROR
 *  \brief   Trace macro to capture trace error events
 *  \details This macro is used to enable the capturing of trace error events. The macro must be 
 *           mapped onto a user-defined function. To disable the trace error events, the macro must
 *           not be defined. The mapped user-defined function must adhere to the following function 
 *           signature.
 * 
 *           void (*Ucs_TraceCb_t)(void * ucs_user_ptr, const char module_str[], const char entry_str[], uint16_t vargs_cnt, ...);
 *           - <b>ucs_user_ptr</b><br>Reference to the User argument
 *           - <b>module_str</b><br>The name of the software module that has posted the trace
 *           - <b>entry_str</b><br>The trace entry as formatted string
 *           - <b>vargs_cnt</b><br>Number of trace arguments which will be passed within the variable 
 *                                 argument list
 *           - <b>[...]</b><br>Variable argument list to pass trace arguments
 *
 *  \note    The captured error events can be used for logging and as a first step for debugging
 *           unexpected behavior. However, the application must not derive any action when an error
 *           is indicated by the trace interface. An application must handle rely on result callback 
 *           functions and handle "general.error_fptr()".
 *
 *           <b>Example:</b>
 *           \code
 *           extern void App_UcsTraceError(void * ucs_user_ptr,
 *                                         const char module_str[],
 *                                         const char entry_str[],
 *                                         uint16_t vargs_cnt,
 *                                         ...);
 *
 *           #define UCS_TR_ERROR   App_UcsTraceError
 *           \endcode
 *
 *  \ingroup G_UCS_TRACE
 */


/*! \addtogroup G_UCS_TRACE
 *  \details    The UCS Trace Interface is intended for debugging and logging purpose.
 *              There are 2 different trace options:
 *              - The definition of trace macros to print out internal states, messages
 *                and errors. This option provides two trace classes: \c info and \c error. Each trace 
 *                class can be activated by defining the respective macro UCS_TR_INFO
 *                UCS_TR_ERROR in the configuration header file \c ucs_cfg.h.
 *                While the \c info class is intended only for debugging purpose during
 *                development, the \c error class can also be active for logging purpose
 *                in a production system.
 *              - Implementation of the callback function Ucs_DebugErrorMsgCb_t which is assigned
 *                during initialization. The callback function is fired on every received Error
 *                message from a local or remote INIC.
 */

/*!
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_TRACE
 * @{
 */

/*! \def     TR_INFO
 *  \brief   Trace macro to capture trace info events
 *  \details The macro is referenced to a public trace macro which must be defined in ucs_cfg.h. The
 *           public macros refers to a trace function which must be implemented by the application. 
 *           The given arguments can be stored or immediately converted into a trace output by 
 *           invoking the function Ucs_Tr_DecodeTrace().
 *  \param   args    Container of arguments. The following arguments are of the container.
 *                   - ucs_user_ptr Reference to the User argument
 *                   - unit         Id of the UNICENS unit that has posted the trace
 *                   - entry        Id of the trace entry
 *                   - vargs_cnt    Number of trace arguments which will be passed within the variable 
 *                                  argument list
 *                   - [...]        Variable argument list to pass trace arguments
 */

/*! \def     TR_ERROR
 *  \brief   Trace macro to capture trace error events
 *  \details The macro is referenced to a public trace macro which must be defined in ucs_cfg.h. The
 *           public macros refers to a trace function which must be implemented by the application. 
 *           The given arguments can be stored or immediately converted into a trace output by 
 *           invoking the function Ucs_Tr_DecodeTrace().
 *  \param   args    Container of arguments. The following arguments are of the container.
 *                   - ucs_user_ptr Reference to the User argument
 *                   - unit         Id of the UNICENS unit that has posted the trace
 *                   - entry        Id of the trace entry
 *                   - vargs_cnt    Number of trace arguments which will be passed within the variable 
 *                                  argument list
 *                   - [...]        Variable argument list to pass trace arguments
 */

/*! \def    TR_FAILED_ASSERT
 *  \brief  Failed Assert statement which will add error entry into the trace output.
 *  \param  ucs_user_ptr Reference to the User argument
 *  \param  unit        Identifier for the respective software unit.
 */

/*! \def    TR_ASSERT
 *  \brief  Assert statement which evaluates an expression to true. If the expression
 *          evaluates to false a failed assert will be printed into the trace output.
 *  \param  ucs_user_ptr Reference to the User argument
 *  \param  unit        Identifier for the respective software unit.
 *  \param  expr        Expression which shall evaluate to \c true (expectation applies)
 */

/*! \def    TR_ERROR_INIC_RESULT
 *  \brief  Trace macro to capture INIC error data
 *  \param  ucs_user_ptr Reference to the User argument
 *  \param  unit        Identifier for the respective software unit.
 *  \param  info_ptr    Byte stream which contains the raw INIC error data
 *  \param  info_size   Size of the INIC error data in bytes
 */

/*!
 * @}
 * \endcond
 */

/* parasoft suppress MISRA2004-19_7 MISRA2004-19_4 reason "function-like macros are allowed for tracing" */
#ifdef UCS_TR_ERROR
#   define TR_ERROR(args) UCS_TR_ERROR args;
#   define TR_FAILED_ASSERT(ucs_user_ptr, unit) TR_ERROR(((ucs_user_ptr), (unit), TR_UCS_ASSERT, 1U, __LINE__))
#   define TR_ASSERT(ucs_user_ptr, unit, expr)  if (!(expr)) {TR_FAILED_ASSERT((ucs_user_ptr), (unit));}
#   define TR_ERROR_INIC_RESULT(ucs_user_ptr, unit, info_ptr, info_size)                             \
            {                                                                                       \
                uint8_t i;                                                                          \
                TR_ERROR(((ucs_user_ptr), (unit), TR_UCS_INIC_RESULT_ID_1, 0U));                     \
                for(i=0U; i<info_size; i++)                                                         \
                {                                                                                   \
                    TR_ERROR(((ucs_user_ptr), (unit), TR_UCS_INIC_RESULT_ID_2, 2U, i, info_ptr[i]))  \
                }                                                                                   \
            }
#else
#   define UCS_TR_ERROR
#   define TR_ERROR(args)
#   define TR_FAILED_ASSERT(ucs_user_ptr, unit)
#   define TR_ASSERT(ucs_user_ptr, unit, expr)
#   define TR_ERROR_INIC_RESULT(ucs_user_ptr, unit, info_ptr, info_size)
#endif

#ifdef UCS_TR_INFO
#   define TR_INFO(args) UCS_TR_INFO args;
#else
#   define UCS_TR_INFO
#   define TR_INFO(args)
#endif
/* parasoft unsuppress item MISRA2004-19_7 item MISRA2004-19_4 reason "function-like macros are allowed for tracing" */

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif  /* #ifndef UCS_TRACE_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

