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
 * \brief Internal header file of the Network Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NET
 * @{
 */

#ifndef UCS_NET_H
#define UCS_NET_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_inic.h"
#include "ucs_ret_pb.h"
#include "ucs_obs.h"
#include "ucs_fsm.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Result codes of function Net_IsOwnAddress() */
typedef enum Net_IsOwnAddrResult_
{
    NET_IS_OWN_ADDR_NODE,   /*!< \brief Is own node position address or own logical node address */
    NET_IS_OWN_ADDR_GROUP,  /*!< \brief Is own group address */
    NET_IS_OWN_ADDR_NONE    /*!< \brief Is foreign address */

} Net_IsOwnAddrResult_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Initialization structure of the attach service. */
typedef struct Net_InitData_
{
    CBase *base_ptr;                /*!< \brief Reference to base instance */
    CInic *inic_ptr;                /*!< \brief Reference to INIC instance */

} Net_InitData_t;

/*! \brief Parameter structure for MOST Network Status */
typedef struct Net_NetworkStatusParam_
{
    /*! \brief Indicates if parameters have been changed since last update */
    uint16_t change_mask;
    /*! \brief Indicates if the MOST network is available and ready for control/packet
               data transmission */
    Ucs_Network_Availability_t availability;
    /*! \brief Indicates the sub state to parameter Availability */
    Ucs_Network_AvailInfo_t avail_info;
    /*! \brief Indicates the transition cause of the MOST network going from Available to 
               NotAvailable or vice versa */
    Ucs_Network_AvailTransCause_t avail_trans_cause;
    /*! \brief Contains events relating to the functionality for the MOST Network Interface */
    uint16_t events;
    /*! \brief Current size of packet bandwidth */
    uint16_t packet_bw;
    /*! \brief Current node address of the device */
    uint16_t node_address;
    /*! \brief Node position of the device */
    uint8_t node_position;
    /*! \brief Node position of last device in the ring */
    uint8_t max_position;

} Net_NetworkStatusParam_t;

/*! \brief Structure holds the parameters of property INIC.MOSTNetworkStatus */
typedef struct Net_NetworkStatus_
{
    /*! \brief Parameters of MOST Network Status */
    Net_NetworkStatusParam_t param;
    /*! \brief Observe MOST Network status in INIC module */
    CObserver observer;
    /*! \brief Subject to notify MOST Network Status the first time a observer has been added */
    CSubject pre_subject;
    /*! \brief Subject to notify MOST Network Status */
    CSubject subject;

} Net_NetworkStatus_t;

/*! \brief Parameter structure for MOST Network Configuration */
typedef struct Net_NetworkConfigParam_
{
    /*! \brief Indicates if parameters have been changed since last update */
    uint16_t change_mask;
    /*! \brief Current node address of the device */
    uint16_t node_address;
    /*! \brief Current group address of the device */
    uint16_t group_address;
    /*! \brief Low-level retry block count */
    uint8_t llrbc;

} Net_NetworkConfigParam_t;

/*! \brief Structure holds the parameters of property INIC.MOSTNetworkConfiguration */
typedef struct Net_NetworkConfiguration_
{
    /*! \brief Parameters of MOST Network Configuration */
    Net_NetworkConfigParam_t param;
    /*! \brief Observe MOST Network Configuration in INIC module */
    CObserver observer;
    /*! \brief Subject to notify MOST Network Configuration the first time a observer has been added */
    CSubject pre_subject;
    /*! \brief Subject to notify MOST Network Configuration */
    CSubject subject;

} Net_NetworkConfiguration_t;

/*! \brief  Class structure of the Network Management. */
typedef struct CNetworkManagement_
{
    /*! \brief Parameters of property INIC.MOSTNetworkStatus */
    Net_NetworkStatus_t network_status;
    /*! \brief Parameters of property INIC.MOSTNetworkConfig. */
    Net_NetworkConfiguration_t network_configuration;
    /*! \brief Reference to INIC instance */
    CInic *inic_ptr;
    /*! \brief Reference to base instance */
    CBase *base_ptr;
    /*! \brief Service instance for the scheduler */
    CService net_srv;

} CNetworkManagement;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CNetworkManagement                                                         */
/*------------------------------------------------------------------------------------------------*/
extern void Net_Ctor(CNetworkManagement *self, Net_InitData_t *init_ptr);
extern void Net_AddObserverNetworkStatus(CNetworkManagement *self, CMaskedObserver *obs_ptr);
extern void Net_DelObserverNetworkStatus(CNetworkManagement *self, CMaskedObserver *obs_ptr);
extern void Net_AddObserverNetworkConfig(CNetworkManagement *self, CMaskedObserver *obs_ptr);
extern void Net_DelObserverNetworkConfig(CNetworkManagement *self, CMaskedObserver *obs_ptr);
extern Net_IsOwnAddrResult_t Net_IsOwnAddress(CNetworkManagement *self, uint16_t address);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_NET_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

