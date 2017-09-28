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
 * \brief Internal header file of class CBackChannelDiag.
 *
 * \cond UCS_INTERNAL_DOC
 */

#ifndef UCS_BC_DIAG_H
#define UCS_BC_DIAG_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure of class CBackChannelDiag. */
typedef struct CBackChannelDiag_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    CSingleObserver bcd_inic_bcd_start;  /*!< \brief Observes the INIC.BCDiag result */
    CSingleObserver bcd_inic_bcd_end;   /*!< \brief Observes the INIC.BCDiagEnd result*/
    CSingleObserver bcd_enabletx;       /*!< \brief Observes the EXC.BCEnableTx result */
    CSingleObserver bcd_diagnosis;      /*!< \brief Observes the EXC.BCDiag result */
    CSingleObserver bcd_switchback;     /*!< \brief Observes the EXC.Switchback result. */


    CMaskedObserver bcd_terminate;       /*!< \brief Observes events leading to termination */

    CObserver       bcd_nwstatus;        /*!< \brief Observes the MOST Network status */

    CFsm     fsm;                       /*!< \brief Node Discovery state machine  */
    CService service;                   /*!< \brief Service instance for the scheduler */
    CTimer   timer;                     /*!< \brief timer for monitoring messages */

    Exc_WelcomeResult_t   welcome_result;       /*!< \brief buffer for welcome result */
    bool neton;                         /*!< \brief indicates Network availability */

    Ucs_Bcd_ReportCb_t report_fptr;      /*!< \brief Report callback function */

    uint8_t current_segment;            /*!< \brief segment which is currently tested, starts with 0. */
    Exc_BCDiagResult bcd_result;        /*!< \brief Result of current tested segment. */

}CBackChannelDiag;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Bcd_Ctor(CBackChannelDiag *self,
              CInic *inic,
              CBase *base,
              CExc *exc);

extern void Bcd_Start(CBackChannelDiag *self, Ucs_Bcd_ReportCb_t report_fptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_BC_DIAG_H */
/*!
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

