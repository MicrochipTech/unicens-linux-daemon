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
 * \brief Internal header file of the CManager class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_MGR
 * @{
 */

#ifndef UCS_MGR_H
#define UCS_MGR_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_fsm.h"
#include "ucs_inic.h"
#include "ucs_net.h"
#include "ucs_base.h"
#include "ucs_jobs.h"
#include "ucs_nodedis.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The default value of the desired packet bandwidth for startup command */
#define MGR_PACKET_BW_DEFAULT  52U

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      Manager Class
 *  \details    Implements the UNICENS Manager State Machine
 */
typedef struct CManager_
{
    bool            listening;                  /*!< \brief Listening is active */
    CFsm            fsm;                        /*!< \brief State machine object */
    CJobService     job_service;
    CSingleObserver job_q_obs;
    CJobQ          *current_q_ptr;
    
    CJobQ           job_q_startup;               
    CJobQ           job_q_force_startup;
 /* CJobQ           job_q_shutdown; */
    CJob            job_startup;
    CJob            job_leave_forced_na;
 /* CJob            job_shutdown; */

    CJob           *list_startup[2];
    CJob           *list_force_startup[3];
 /* CJob           *list_shutdown[2]; */

    CMaskedObserver event_observer;             /*!< \brief Observes init complete event */
    CMaskedObserver nwstatus_mobs;              /*!< \brief Observe network status */

    uint16_t             packet_bw;             /*!< \brief The desired packet bandwidth */
    CBase               *base_ptr;              /*!< \brief Reference to base services */
    CInic               *inic_ptr;              /*!< \brief Reference to class CInic */
    CNetworkManagement  *net_ptr;               /*!< \brief Reference to network management */
    CNodeDiscovery      *nd_ptr;                /*!< \brief Reference to node discovery */

    CSingleObserver startup_obs;                /*!< \brief Startup result callback */
 /* CSingleObserver shutdown_obs; */            /*!< \brief Shutdown result callback */
    CSingleObserver force_na_obs;               /*!< \brief ForceNA result callback */
    bool                 initial;               /*!< \brief Is \c true for the initial network status "available" */

} CManager;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Mgr_Ctor(CManager *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, CNodeDiscovery *nd_ptr, uint16_t packet_bw);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_MGR_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

