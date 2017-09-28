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
 * \brief Public header file of the Extended Resource Manager.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EPM
 * @{
 */

#ifndef UCS_EPM_PB_H
#define UCS_EPM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_xrm_pb.h"
#include "ucs_obs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief This enumerator specifies the state of EndPoint objects. */
typedef enum Ucs_Rm_EndPointState_
{
    UCS_RM_EP_IDLE               = 0x00U,     /*!< \brief Specifies the "Idle" state of the endpoint. This means that the endpoint has not been handled yet  */
    UCS_RM_EP_XRMPROCESSING      = 0x01U,     /*!< \brief Specifies that the EndPoint is under "XRM process". */
    UCS_RM_EP_BUILT              = 0x02U      /*!< \brief Specifies that the EndPoint is "Built". */

} Ucs_Rm_EndPointState_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/* Epm_Inst_t requires incomplete forward declaration, to hide internal data type.
 * The Epm_Inst_t object is allocated internally, the core library must access only the pointer to Epm_Inst_t. */
struct Epm_Inst_;

/*!\brief  EndpointManagement instance */
typedef struct Epm_Inst_ Epm_Inst_t;

/*! \brief Internal configuration structure of a Connection EndPoint. */
typedef struct Ucs_Rm_EndPointInt_
{
    /*! \brief Stores the current number of retries in case of error. */
    uint8_t num_retries;
    /*! \brief State of the endpoint object. */
    Ucs_Rm_EndPointState_t endpoint_state;
    /*! \brief connection label. */
    uint16_t connection_label;
    /*! \brief object counter. */
    uint8_t reference_cnt;
    /*! \brief last XRM result. */
    Ucs_Xrm_Result_t xrm_result;
    /*! \brief A subject object for this endpoint. */
    CSubject subject_obj;
    /*! \brief Reference to the EndPointManagement that handles this endpoint */
    Epm_Inst_t * epm_inst;
    /*! \brief magic number to signal that endpoint has already been initialized */
    uint32_t magic_number;

} Ucs_Rm_EndPointInt_t;

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_EPM_PB_H */

/*!
 * @}
 * \endcond
 */


/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

