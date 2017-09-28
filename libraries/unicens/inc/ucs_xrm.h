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
 * \brief      Include file of the Extended Resource Manager.
 *
 * \defgroup   G_UCS_XRM_STREAM Additional Streaming Port Functions
 * \brief      Additional Streaming Port functions of the Extended Resource Manager.
 * \ingroup    G_UCS_IRM
 *
 */

#ifndef UCS_XRM_H
#define UCS_XRM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_net.h"
#include "ucs_rsm.h"
#include "ucs_inic.h"
#include "ucs_ret_pb.h"
#include "ucs_obs.h"
#include "ucs_xrmpool.h"
#include "ucs_class_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_XRM_INT
 * @{
 */

/*! \def XRM_NUM_RES_HDL_PER_ICM
 *  \brief Maximum number of resource handles per ICM. Depends on the maximum payload of ICMs.
 */
#define XRM_NUM_RES_HDL_PER_ICM     22U

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Stores data required by XRM during initialization. */
typedef struct Xrm_InitData_
{                    
    CBase *base_ptr;                                    /*!< \brief Reference to base instance */
    CInic *inic_ptr;                                    /*!< \brief Reference to INIC instance */
    CNetworkManagement *net_ptr;                        /*!< \brief Reference to Network instance */
    CRemoteSyncManagement *rsm_ptr;                     /*!< \brief Reference to a RSM instance */
    CXrmPool *xrmp_ptr;                                 /*!< \brief Reference to a xrm pool instance */
    Ucs_Xrm_ResourceDebugCb_t res_debugging_fptr;       /*!< \brief Callback function pointer to monitor XRM resources from application */
    Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr;          /*!< \brief Callback function pointer to signal unmute of devices */

} Xrm_InitData_t;

/*! \brief Structure that defines a XRM StreamPort configuration  */
typedef struct Xrm_StreamPort_Config_
{
    uint8_t                         index;              /*!< \brief Streaming Port instance */
    Ucs_Stream_PortOpMode_t         op_mode;            /*!< \brief Streaming Port Operation mode */
    Ucs_Stream_PortOption_t         port_option;        /*!< \brief Streaming Port Options */
    Ucs_Stream_PortClockMode_t      clock_mode;         /*!< \brief Stream Port Clock Mode */
    Ucs_Stream_PortClockDataDelay_t clock_data_delay;   /*!< \brief Stream Port Clock Data Delay */
    Ucs_Xrm_Stream_PortCfgResCb_t   result_fptr;        /*!< \brief Result callback */

} Xrm_StreamPort_Config_t;

/*! \brief Result observers used by FBlock INIC Resource Management functions */
typedef struct Xrm_Observers_
{
    /*! \brief Observer used to monitor internal errors (e.g., INIC BIST Error) */
    CMaskedObserver internal_error_obs;
    /*! \brief Observer used to monitor network status infos */
    CMaskedObserver nw_status_obs;
    /*! \brief Observer used to monitor ICM tx message object availability */
    CObserver tx_msg_obj_obs;
    /*! \brief Observer used for the INIC resource monitor */
    CObserver resource_monitor_obs;
    /*! \brief Result observer used for destruction of INIC resources */
    CSingleObserver resource_destroy_obs;
    /*! \brief Result observer used for requests of invalid resource handles */
    CSingleObserver resource_invalid_list_obs;
    /*! \brief Result observer used for sockets, ports and connections */
    CSingleObserver std_result_obs;
    /*! \brief Application callback to signal unmute of devices */
    Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr;
    /*! \brief Callback function pointer used for streaming port configurations */
    Ucs_Xrm_Stream_PortCfgResCb_t stream_port_config_fptr;
    /*! \brief Observer to proxy callback stream_port_config_fptr() */
    CSingleObserver stream_port_config_obs;
    /*! \brief Callback function pointer used by operation that enables a MOST Port */
    Ucs_StdResultCb_t most_port_enable_fptr;
    /*! \brief Observer to proxy callback most_port_enable_port_fptr() */
    CSingleObserver most_port_enable_obs;
    /*! \brief Callback function pointer by operation that enables full streaming for a MOST Port*/
    Ucs_StdResultCb_t most_port_en_full_str_fptr;
    /*! \brief Observer to proxy callback most_port_en_full_str_fptr() */
    CSingleObserver most_port_en_full_str_obs;
    /*! \brief Observer to the SyncLost event in RSM */
    CObserver rsm_sync_lost_obs;

} Xrm_Observers_t;

/*! \brief Structure of the Extended Resource Manager class. */
typedef struct CExtendedResourceManager_
{
    /*!< \brief  Jobs list queue */
    CDlList  job_list;
    /*! \brief List to temporarily store invalid resource handles */
    uint16_t inv_resource_handle_list[XRM_NUM_RES_HDL_PER_ICM];
    /*! \brief Reference to an INIC instance */
    CInic *inic_ptr;
    /*! \brief Reference to a network instance */
    CNetworkManagement *net_ptr;
    /*! \brief Reference to a rsm instance */
    CRemoteSyncManagement *rsm_ptr;
    /*!< \brief Reference to base instance */
    CBase *base_ptr;
    /*!< \brief Reference to the xrm pool instance */
    CXrmPool *xrmp_ptr;
    /*! \brief Reference to the currently processed job */
    Xrm_Job_t *current_job_ptr;
    /*! \brief Reference to the currently processed resource object */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t **current_obj_pptr;
    /*! \brief Number of invalid handles in list inv_resource_handle_list[] */
    uint8_t inv_resource_handle_list_size;
    /*! \brief Current number of destroyed handles in list inv_resource_handle_list[] */
    uint8_t curr_dest_resource_handle_size;
    /*! \brief Start index for the current invalid handles index in list inv_resource_handle_list[] */
    uint8_t inv_resource_handle_index;
    /*! \brief Service instance to add the Extended Resource Manager to the MNS scheduler */
    CService xrm_srv;
    /*! \brief Report result of the Extended Resource Manager. Used to reported status and error 
     *         information to the application.
     */
    Ucs_Xrm_Result_t report_result;
    /*! \brief Required result observers */
    Xrm_Observers_t obs;
    /*! \brief Mask that stores queued event */
    Srv_Event_t queued_event_mask;
    /*! \brief stores the currently stream port configuration (in process) */
    Xrm_StreamPort_Config_t current_streamport_config;
    /*!< \brief Callback function pointer to monitor XRM resources */
    Ucs_Xrm_ResourceDebugCb_t res_debugging_fptr;
    /*! \brief Flag to lock the API */
    bool lock_api;
    /*!< \brief Signal whether this instance is in Remote Control Mode */ 
    bool IsInRemoteControlMode;

} CExtendedResourceManager;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CExtendedResourceManager                                                   */
/*------------------------------------------------------------------------------------------------*/
extern void Xrm_Ctor(CExtendedResourceManager *self, Xrm_InitData_t * data_ptr);
extern Ucs_Return_t Xrm_Process(CExtendedResourceManager *self,
                                UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[],
                                uint16_t most_network_connection_label,
                                void * user_arg,
                                Ucs_Xrm_ReportCb_t report_fptr);
extern Ucs_Return_t Xrm_Destroy(CExtendedResourceManager *self, 
                                UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[]);
extern Ucs_Return_t Xrm_Stream_SetPortConfig(CExtendedResourceManager *self, 
                                             uint8_t index,
                                             Ucs_Stream_PortOpMode_t op_mode,
                                             Ucs_Stream_PortOption_t port_option,
                                             Ucs_Stream_PortClockMode_t clock_mode,
                                             Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                             Ucs_Xrm_Stream_PortCfgResCb_t result_fptr);
extern Ucs_Return_t Xrm_Stream_GetPortConfig(CExtendedResourceManager *self, 
                                             uint8_t index,
                                             Ucs_Xrm_Stream_PortCfgResCb_t result_fptr);
extern Ucs_Return_t Xrm_Most_EnablePort(CExtendedResourceManager *self,
                                        uint16_t most_port_handle, 
                                        bool enabled, 
                                        Ucs_StdResultCb_t result_fptr);
extern Ucs_Return_t Xrm_Most_PortEnFullStr(CExtendedResourceManager *self,
                                           uint16_t most_port_handle, 
                                           bool enabled, 
                                           Ucs_StdResultCb_t result_fptr);
extern void Xrm_SetResourceDebugCbFn(CExtendedResourceManager *self, Ucs_Xrm_ResourceDebugCb_t dbg_cb_fn);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_XRM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

