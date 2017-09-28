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
 * \brief Internal header file of the Route Manager.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_RTM
 * @{
 */

#ifndef UCS_RTM_H
#define UCS_RTM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_ret_pb.h"
#include "ucs_obs.h"
#include "ucs_epm.h"
#include "ucs_net.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Stores data required by RTM during initialization. */
typedef struct Rtm_InitData_
{
    CBase *base_ptr;                /*!< \brief Reference to base instance */
    CEndpointManagement *epm_ptr;   /*!< \brief Reference to the endpoint management instance */
    CNetworkManagement *net_ptr;    /*!< \brief Reference to Network instance */
    Ucs_Rm_ReportCb_t report_fptr;  /*!< \brief Reference to the report callback function */

} Rtm_InitData_t;

/*! \brief  Class structure of the Route Management. */
typedef struct CRouteManagement_
{
    /*! \brief Reference to a base instance */
    CBase *base_ptr;
    /*! \brief Reference to a network instance */
    CEndpointManagement * epm_ptr;
    /*!< \brief Reference to the timer management */ 
    CTimerManagement * tm_ptr;
    /*!< \brief Reference to Network instance */
    CNetworkManagement *net_ptr;
    /*!< \brief Timer for checking routes process */
    CTimer route_check;
    /*!< \brief Reference to the routes list */
    Ucs_Rm_Route_t * routes_list_ptr;
    /*! \brief Points to the current routes to be handled */
    Ucs_Rm_Route_t * curr_route_ptr;
    /*! \brief Current route index */
    uint16_t curr_route_index;
    /*! \brief Size of the current routes list */
    uint16_t routes_list_size;
    /*! \brief Service instance for the scheduler */
    CService rtm_srv;
    /*! \brief Report callback of the routes list */
    Ucs_Rm_ReportCb_t report_fptr;
    /*! \brief Observe MOST Network status in Net module */
    CMaskedObserver nwstatus_observer;
    /*! \brief Observer used to monitor UCS initialization result */
    CMaskedObserver ucsinit_observer;
    /*! \brief Observer used to monitor UCS termination event */
    CMaskedObserver ucstermination_observer;
    /*! \brief Specifies used to monitor UCS termination event */
    bool ucs_is_stopping;
    /*! \brief specifies whether the network status is available or not */
    bool nw_available;
    /*! \brief Flag to lock the API */
    bool lock_api;

} CRouteManagement;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CRouteManagement                                                           */
/*------------------------------------------------------------------------------------------------*/
extern void Rtm_Ctor(CRouteManagement * self, Rtm_InitData_t * init_ptr);
extern Ucs_Return_t Rtm_StartProcess(CRouteManagement * self,  Ucs_Rm_Route_t routes_list[], uint16_t size);
extern Ucs_Return_t Rtm_DeactivateRoute(CRouteManagement * self, Ucs_Rm_Route_t * route_ptr);
extern Ucs_Return_t Rtm_ActivateRoute(CRouteManagement * self, Ucs_Rm_Route_t * route_ptr);
extern Ucs_Return_t Rtm_SetNodeAvailable(CRouteManagement * self, Ucs_Rm_Node_t *node_ptr, bool available);
extern bool Rtm_GetNodeAvailable(CRouteManagement * self, Ucs_Rm_Node_t *node_ptr);
extern Ucs_Return_t Rtm_GetAttachedRoutes(CRouteManagement * self, Ucs_Rm_EndPoint_t * ep_inst, Ucs_Rm_Route_t * ext_routes_list[], uint16_t size_list);
extern uint16_t Rtm_GetConnectionLabel(CRouteManagement * self, Ucs_Rm_Route_t * route_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_RTM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

