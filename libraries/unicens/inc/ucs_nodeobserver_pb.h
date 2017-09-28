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
 * \brief Public header file of the CNodeObserver class
 */
/*!
 * \cond SEC_UCS_LIB
 * \addtogroup G_UCS_MGR
 * @{
 */

#ifndef UCS_NODEOBSERVER_PB_H
#define UCS_NODEOBSERVER_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_rm_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief Manager report codes */
typedef enum Ucs_MgrReport_
{
    UCS_MGR_REP_IGNORED_UNKNOWN   = 0, /*!< \brief A discovered node is ignored due to a wrong signature,
                                        *          a missing entry in the \ref Ucs_Mgr_InitData_t "nodes_list_ptr",
                                        *          or since the desired node address is not within the following range:
                                        *          0x200..0x2FF, 0x500..0xEFF.
                                        */
    UCS_MGR_REP_IGNORED_DUPLICATE = 1, /*!< \brief A discovered node is ignored due since it is a duplicate
                                        *          of an alredy welcomed node.
                                        */
    UCS_MGR_REP_AVAILABLE         = 2, /*!< \brief A discovered node was successfully "welcomed" 
                                        *          in the network.
                                        */
    UCS_MGR_REP_NOT_AVAILABLE     = 3  /*!< \brief A previously welcomed node became invalid and is 
                                        *          no longer accessible in the network.
                                        */
} Ucs_MgrReport_t;

/*! \brief Optional callback function that reports events on ignored, welcomed and lost nodes.
 *  \param code             Report code
 *  \param node_address     The desired node_address of the node which is defined in it's signature.
 *  \param node_ptr         Reference to the node object which is part of the \ref Ucs_Mgr_InitData_t "nodes_list_ptr".
 *                          The reference is \c NULL if \c code is \c UCS_MGR_REP_IGNORED_UNKNOWN or 
 *                          \c UCS_MGR_REP_IGNORED_DUPLICATE.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_MgrReportCb_t)(Ucs_MgrReport_t code, uint16_t node_address, Ucs_Rm_Node_t *node_ptr, void *user_ptr);

/*! \brief   The initialization data of the Manager */
typedef struct Ucs_Mgr_InitData_
{
    bool enabled;                       /*!< \brief If set to \c false the application must
                                         *          handle network startup, node discovery and
                                         *          rooting by hand.
                                         */
    uint16_t packet_bw;                 /*!< \brief The desired packet bandwidth.\mns_name_inic{PacketBW} */
    
    Ucs_Rm_Route_t *routes_list_ptr;    /*!< \brief Reference to a list of routes */
    uint16_t routes_list_size;          /*!< \brief Number of routes in the list */
    
    Ucs_Rm_Node_t *nodes_list_ptr;      /*!< \brief Reference to the list of nodes */
    uint16_t nodes_list_size;           /*!< \brief Number of nodes in the list */

    Ucs_MgrReportCb_t report_fptr;      /*!< \brief Optional callback function notifying node events */

} Ucs_Mgr_InitData_t;


#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif  /* ifndef UCS_NODEOBSERVER_PB_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

