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
 * \brief Private header file of the Extended Resource Manager.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_XRM_INT
 * @{
 */

#ifndef UCS_XRM_PV_H
#define UCS_XRM_PV_H

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_xrm_cfg.h"
#include "ucs_xrm.h"

/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes of function Xrm_PrepareJobDestruction(). */
typedef enum Xrm_PreJobDestrResult_
{
    XRM_PRE_JOB_DEST_TASKS_EXIST    = 0x00U,    /*!< \brief There are resources to destroy */
    XRM_PRE_JOB_DEST_NO_TASKS_EXIST = 0x01U,    /*!< \brief All resources already destroyed */
    XRM_PRE_JOB_DEST_DONE           = 0x02U,    /*!< \brief Only shared resources affected. Invoke
                                                 *          result callback immediately.
                                                 */
    XRM_PRE_JOB_DEST_BUSY           = 0x03U     /*!< \brief Preparation of JobDestruction is 
                                                 *          currently not possible. Other resources 
                                                 *          are currently being destroyed.
                                                 */
} Xrm_PreJobDestrResult_t; 

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Structure used to count the number of entries of a given resource handle. */
typedef struct Xrm_CntEntriesResHandle_
{
    /*! \brief Reference to the XRM instance to be looked for. */
    CExtendedResourceManager *xrm_inst;
    /*! \brief Reference the resource object to be looked for. */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr;
    /*! \brief Storage for the count result. */
    uint8_t * cnt_res;

} Xrm_CntEntriesResHandle_t;

/*------------------------------------------------------------------------------------------------*/
/* Internal constants (extern declarations only)                                                  */
/*------------------------------------------------------------------------------------------------*/
extern const uint8_t XRM_SRV_PRIO;
extern const Srv_Event_t XRM_EVENT_PROCESS;
extern const Srv_Event_t XRM_EVENT_ERROR;
extern const Srv_Event_t XRM_EVENT_REQ_INV_RES_LST;
extern const Srv_Event_t XRM_EVENT_DESTROY_INV_RES;
extern const Srv_Event_t XRM_EVENT_NOTIFY_AUTO_DEST_RES;
extern const Srv_Event_t XRM_EVENT_NOTIFY_AUTO_DEST_RESR;
extern const Srv_Event_t XRM_EVENT_SYNC_NEXT_DEV;
extern const Srv_Event_t XRM_EVENT_NOTIFY_DESTROYED_JOB;
extern const Srv_Event_t XRM_EVENT_RESUME_JOB_DESTRUCT;
extern const Srv_Event_t XRM_EVENT_RESET_RES_MONITOR;
extern const Srv_Event_t XRM_EVENT_STREAMPORT_CONFIG_SET;
extern const Srv_Event_t XRM_EVENT_STREAMPORT_CONFIG_GET;
extern const uint16_t XRM_INVALID_RESOURCE_HANDLE;
extern const uint16_t XRM_INVALID_CONNECTION_LABEL;
extern const uint16_t XRM_DEFAULT_SENDER_HANDLE;
extern const uint16_t XRM_INVALID_NODE_ADDRESS;
extern const uint16_t XRM_MASK_NETWORK_AVAILABILITY;
extern const uint16_t XRM_MASK_NETWORK_NODE_ADDRESS;

/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes class CExtendedResourceManager                                             */
/*------------------------------------------------------------------------------------------------*/
/* Basic functions */
extern void Xrm_Service(void *self);
extern bool Xrm_IsApiFree(CExtendedResourceManager *self);
extern bool Xrm_IsInMyJobList(CExtendedResourceManager *self, UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[]);
extern void Xrm_ApiLocking(CExtendedResourceManager *self, bool status);
extern void Xrm_WaitForTxMsgObj(CExtendedResourceManager *self, Srv_Event_t event_mask);
extern void Xrm_UninitializeService(void *self, void *error_code_ptr);
extern void Xrm_MnsNwStatusInfosCb(void *self, void *result_ptr);
extern void Xrm_MsgObjAvailCb(void *self, void *result_ptr);
extern void Xrm_RmtDevSyncLostCb(void *self, void *result_ptr);
extern Xrm_PreJobDestrResult_t Xrm_PrepareJobDestruction(CExtendedResourceManager *self);
extern Xrm_PreJobDestrResult_t Xrm_UnsafePrepareJobDestruction(CExtendedResourceManager *self);
extern void Xrm_ResumeJobDestruction(CExtendedResourceManager *self);
extern uint8_t Xrm_CountResourceObjects(CExtendedResourceManager *self, Xrm_Job_t *job_ptr);
extern Xrm_Job_t * Xrm_GetJob(CExtendedResourceManager *self,
                              UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[]);
extern void * Xrm_GetNextDevInUnnotifiedState(CExtendedResourceManager *self);
extern uint8_t Xrm_GetResourceObjectIndex(CExtendedResourceManager *self,
                                          Xrm_Job_t *job_ptr,
                                          UCS_XRM_CONST Ucs_Xrm_ResObject_t **obj_pptr);
extern bool Xrm_SearchNextResourceObject(CExtendedResourceManager *self);
extern bool Xrm_IsCurrDeviceAlreadyAttached(CExtendedResourceManager *self);
extern void Xrm_ProcessJob(CExtendedResourceManager *self);
extern bool Xrm_IsPartOfJobList (void * job_ptr, void * resrc_obj_ptr);
extern bool Xrm_IsDefaultCreatedPort(CExtendedResourceManager *self,
                                     UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr);
extern bool Xrm_StoreResourceHandle(CExtendedResourceManager *self,
                                    uint16_t resource_handle,
                                    Xrm_Job_t *job_ptr,
                                    UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr);
extern bool Xrm_SetJobAsInvalid(void * job_ptr, void * xrm_inst);
extern bool Xrm_MarkThisResrcAsInvalid (void *resrc_ptr, void * xrm_inst, void *ud_ptr2, void *ud_ptr3);
extern bool Xrm_FreeResrcHandleAndNtf(void *resrc_ptr, void *resrc_handle, void *job_ptr, void * user_arg);
extern bool Xrm_SetCurrJobPtr(void *resrc_ptr, void *resrc_handle, void *job_ptr, void * user_arg);
extern bool Xrm_ReleaseResrcHandle(void *resrc_ptr, void *job_ptr, void *resrc_obj_pptr, void * user_arg);
extern bool Xrm_CntResHandleEntries(void * job_ptr, void * param_ptr);
extern bool Xrm_IncrResHandleEntryCnt (void *resrc_ptr, void *job_ptr, void *param_ptr, void * user_arg);
extern bool Xrm_SetNtfForThisJob(void * job_ptr, void * ud_ptr);
extern bool Xrm_IsInMyJobsList (void * self, void * job_ptr);
extern uint16_t Xrm_GetResourceHandle(CExtendedResourceManager *self,
                                      Xrm_Job_t *job_ptr,
                                      UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr,
                                      Xrmp_CheckJobListFunc_t func_ptr);
extern uint8_t Xrm_CountResourceHandleEntries(CExtendedResourceManager *self,
                                       UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr);
extern void Xrm_ReleaseResourceHandle(CExtendedResourceManager *self,
                                      Xrm_Job_t *job_ptr,
                                      UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_ptr);
extern uint8_t Xrm_ReleaseResourceHandles(CExtendedResourceManager *self,
                                       Xrm_Job_t *job_ptr,
                                       uint16_t resource_handle_list[],
                                       uint8_t resource_handle_list_size,
                                       uint16_t failed_resource_handle);
extern void Xrm_ReleaseResrcHandles(CExtendedResourceManager *self);
extern void Xrm_HandleError(CExtendedResourceManager *self);
extern void Xrm_ReportAutoDestructionResult(CExtendedResourceManager *self);
extern void Xrm_MarkResrcAndJobsAsInvalid (CExtendedResourceManager *self);
extern void Xrm_MarkRemoteDevicesAsUnsync(CExtendedResourceManager *self);
extern void Xrm_ReportJobDestructionResult(CExtendedResourceManager *self);
extern void Xrm_FinishJob(CExtendedResourceManager *self);
extern void Xrm_NotifyInvalidJobs(CExtendedResourceManager *self);

/* INIC Resource Management API */
extern void Xrm_CreateMostSocket(CExtendedResourceManager *self);
extern void Xrm_CreateMlbPort(CExtendedResourceManager *self);
extern void Xrm_CreateMlbSocket(CExtendedResourceManager *self);
extern void Xrm_CreateUsbPort(CExtendedResourceManager *self);
extern void Xrm_CreateUsbSocket(CExtendedResourceManager *self);
extern void Xrm_CreateRmckPort(CExtendedResourceManager *self);
extern void Xrm_CreateStreamPort(CExtendedResourceManager *self);
extern void Xrm_CreateStreamSocket(CExtendedResourceManager *self);
extern void Xrm_CreateSyncCon(CExtendedResourceManager *self);
extern void Xrm_CreateDfiPhaseCon(CExtendedResourceManager *self);
extern void Xrm_CreateCombiner(CExtendedResourceManager *self);
extern void Xrm_CreateSplitter(CExtendedResourceManager *self);
extern void Xrm_CreateAvpCon(CExtendedResourceManager *self);
extern void Xrm_CreateQoSCon(CExtendedResourceManager *self);
extern void Xrm_ResourceMonitorCb(void *self, void *result_ptr);
extern void Xrm_RequestResourceList(CExtendedResourceManager *self);
extern void Xrm_RequestResourceListResultCb(void *self, void *result_ptr);
extern void Xrm_DestroyResources(CExtendedResourceManager *self, Sobs_UpdateCb_t result_fptr);
extern void Xrm_ResetResourceMonitor(CExtendedResourceManager *self);
extern void Xrm_DestroyResourcesResultCb(void *self, void *result_ptr);
extern void Xrm_DestroyJobResourcesResultCb(void *self, void *result_ptr);
extern void Xrm_StdResultCb(void *self, void *result_ptr);
extern void Xrm_Stream_PortConfigResult(void *self, void *result_ptr);
extern void Xrm_Most_PortEnableResult(void *self, void *result_ptr);
extern void Xrm_Most_PortEnFullStrResult(void *self, void *result_ptr);
extern Ucs_Return_t Xrm_RemoteDeviceAttach (CExtendedResourceManager *self, Srv_Event_t next_set_event);
extern void Xrm_RmtDevAttachResultCb(void *self, Rsm_Result_t result);
extern Ucs_Return_t Xrm_SetStreamPortConfiguration (CExtendedResourceManager *self);
extern Ucs_Return_t Xrm_GetStreamPortConfiguration (CExtendedResourceManager *self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_XRM_PV_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

