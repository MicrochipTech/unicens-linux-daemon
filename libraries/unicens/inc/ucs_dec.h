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
 * \brief Internal header file of the Command Decoder Module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DEC_INT
 * @{
 */

#ifndef UCS_DEC_H
#define UCS_DEC_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_dl.h"
#include "ucs_message.h"
/*#include "ucs_ams.h"*/

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Constants                                                                                      */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Stores FktID and OPType in one 16 bit value */
#define DEC_FKTOP(a,b) ((uint16_t)((((uint16_t)((a)<<4)) & (uint16_t)0xFFF0)) | ((uint16_t)(((uint16_t)(b)) & (uint16_t)0x000F))) /* parasoft-suppress  MISRA2004-19_7 "Is used in arrays and therefore cannot be converted to inline function." */

/*! \brief Denotes the end of an FktOp table */
#define DEC_FKTOP_TERMINATION   0xFFFFU

/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Return codes used for decoding functions */
/* Attention: these values are used as error descriptor in error messages and
              must therefore not changed!
*/
typedef enum Dec_Return_
{
    DEC_RET_SUCCESS          = 0x00,        /*!< \brief Operation successfully completed */
    DEC_RET_FKTID_NOT_FOUND  = 0x03,        /*!< \brief FunctionID not found */
    DEC_RET_OPTYPE_NOT_FOUND = 0x04         /*!< \brief Operation Type not found */

} Dec_Return_t;


/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
typedef void (*Dec_IcmCb_t)(void *self, Msg_MostTel_t *msg_ptr);
typedef void (*Dec_RcmCb_t)(void *self, Msg_MostTel_t *msg_ptr);




/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Structure of an FktID_OpType element for FBlock INIC */
typedef struct Dec_FktOpIcm_
{
    /*!< \brief FktID and OPType (combined to a 16-bit value) */
    uint16_t fkt_op;
    /*!< \brief pointer to the belonging handler function */
    Dec_IcmCb_t handler_function_ptr;

} Dec_FktOpIcm_t;

/*! \brief   Structure of an FktID_OpType element for internal INIC Shadow FBlocks e.g. FBlock EXC*/
typedef struct Dec_FktOpIsh_
{
    /*! \brief FktID and OPType (combined to a 16-bit value) */
    uint16_t fkt_op;
    /*! \brief pointer to the belonging handler function */
    Dec_RcmCb_t handler_function_ptr;

} Dec_FktOpIsh_t;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern Dec_Return_t Dec_SearchFktOpIcm(Dec_FktOpIcm_t const list[], uint16_t *index_ptr,
                                       uint16_t function_id, Ucs_OpType_t op_type);
extern Dec_Return_t Dec_SearchFktOpIsh(Dec_FktOpIsh_t const list[], uint16_t *index_ptr,
                                       uint16_t function_id, Ucs_OpType_t op_type);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_DEC_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

