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
 * \brief Declaration of CAttachService class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_ATS
 * @{
 */

#ifndef UCS_ATTACH_H
#define UCS_ATTACH_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_obs.h"
#include "ucs_fsm.h"
#include "ucs_base.h"
#include "ucs_pmfifos.h"
#include "ucs_pmevent.h"
#include "ucs_inic.h"
#include "ucs_ret_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Initialization structure of the attach service. */
typedef struct Ats_InitData_
{
    CBase *base_ptr;                    /*!< \brief Reference to UCS base instance */
    CPmFifos *fifos_ptr;                /*!< \brief Reference to PMS FIFOs */
    CPmEventHandler *pme_ptr;           /*!< \brief Reference to PMS Event Handler */
    CInic *inic_ptr;                    /*!< \brief Reference to INIC Management instance */

} Ats_InitData_t;

/*! \brief   Class structure of the attach service. */
typedef struct CAttachService_
{
    Ats_InitData_t init_data;               /*!< \brief Objects required for attach process */
    CFsm fsm;                               /*!< \brief Attach state machine */
    CService ats_srv;                       /*!< \brief Service instance for the scheduler */
    CObserver obs;                          /*!< \brief Observer used for asynchronous events */
    CObserver obs2;                         /*!< \brief Observer used for asynchronous events */
    CSingleObserver sobs;                   /*!< \brief Single-observer used for asynchronous
                                                        results */
    CMaskedObserver internal_error_obs;     /*!< \brief Error observer to handle INIC errors
                                                        during the attach process */
    CTimer timer;                           /*!< \brief Timer to check for init timeout */ 
    CSingleSubject ats_result_subject;      /*!< \brief Subject to report the result of the
                                                        attach process */
    Ucs_InitResult_t report_result;         /*!< \brief Internal result/error code memory */

} CAttachService;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CAttach                                                                    */
/*------------------------------------------------------------------------------------------------*/
extern void Ats_Ctor(CAttachService *self, Ats_InitData_t *init_ptr);
extern void Ats_Start(void *self, CSingleObserver *obs_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_ATTACH_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

