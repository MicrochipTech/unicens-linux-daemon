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
 * \brief Implementation of the Extended Resource Manager. This file contains the implementation of
 *        the INIC Resource Management functions and result/error handlers.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_XRM_INT
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_xrm.h"
#include "ucs_xrm_pv.h"
#include "ucs_xrm_cfg.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static uint16_t Xrm_CreatePortHandle(CExtendedResourceManager *self,
                                     Ucs_Xrm_PortType_t port_type,
                                     uint8_t index);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CExtendedResourceManager (Handling of resource objects)                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Creates the corresponding INIC port handle depending on the given port type and the
 *          given port instance id.
 *  \param  self            Instance pointer
 *  \param  port_type       Type of the port
 *  \param  index           Port instance id
 *  \return Returns the created INIC port handle.
 */
static uint16_t Xrm_CreatePortHandle(CExtendedResourceManager *self,
                                     Ucs_Xrm_PortType_t port_type,
                                     uint8_t index)
{
    MISC_UNUSED(self);
    return ((uint16_t)((uint16_t)port_type << 8) | (uint16_t)index);
}

/*! \brief  Activates remote synchronization on the current device
 *  \param  self            Instance pointer
 *  \param  next_set_event  Next event to set once the remote synchronization succeeded
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           -------------------------   | ------------------------------------
 *           UCS_RET_SUCCESS             | No error 
 *           UCS_RET_ERR_BUFFER_OVERFLOW | no message buffer available
 */
extern Ucs_Return_t Xrm_RemoteDeviceAttach (CExtendedResourceManager *self, Srv_Event_t next_set_event)
{
    Ucs_Return_t result;

    result = Rsm_SyncDev(self->rsm_ptr, self, &Xrm_RmtDevAttachResultCb);

    if(result == UCS_RET_SUCCESS)
    {
        self->queued_event_mask |= next_set_event;
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start Synchronization of remote device", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, next_set_event);
    }

    return result;
}

/*! \brief  Triggers the creation of a MOST socket.
 *  \param  self    Instance pointer
 */
void Xrm_CreateMostSocket(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_MostSocket_t *cfg_ptr = (UCS_XRM_CONST  Ucs_Xrm_MostSocket_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t con_label = (cfg_ptr->direction == UCS_SOCKET_DIR_INPUT) ? self->current_job_ptr->most_network_connection_label : 0xFFFFU;
    Ucs_Return_t result = Inic_MostSocketCreate(self->inic_ptr,
                                                cfg_ptr->most_port_handle,
                                                cfg_ptr->direction,
                                                cfg_ptr->data_type,
                                                cfg_ptr->bandwidth,
                                                con_label,
                                                &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MOST socket", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_MOST_SOCKET;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MOST socket failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of the MediaLB port.
 *  \param  self    Instance pointer
 */
void Xrm_CreateMlbPort(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_MlbPort_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_MlbPort_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    Ucs_Return_t result = Inic_MlbPortCreate(self->inic_ptr,
                                             cfg_ptr->index,
                                             cfg_ptr->clock_config,
                                             &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MediaLB port", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_MLB_PORT;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MediaLB port failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a MediaLB socket.
 *  \param  self    Instance pointer
 */
void Xrm_CreateMlbSocket(CExtendedResourceManager *self)
{
    Ucs_Return_t result;
    uint16_t mlb_port_handle;
    UCS_XRM_CONST Ucs_Xrm_MlbSocket_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_MlbSocket_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    if(Xrm_IsDefaultCreatedPort(self, cfg_ptr->mlb_port_obj_ptr) != false)
    {
        mlb_port_handle = Xrm_CreatePortHandle(self,
                                               UCS_XRM_PORT_TYPE_MLB,
                                               ((UCS_XRM_CONST Ucs_Xrm_DefaultCreatedPort_t *)(UCS_XRM_CONST void*)(cfg_ptr->mlb_port_obj_ptr))->index);
    }
    else
    {
        mlb_port_handle= Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->mlb_port_obj_ptr, NULL);
    }
    result = Inic_MlbSocketCreate(self->inic_ptr,
                                  mlb_port_handle,
                                  cfg_ptr->direction,
                                  cfg_ptr->data_type,
                                  cfg_ptr->bandwidth,
                                  cfg_ptr->channel_address,
                                  &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MediaLB socket", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_MLB_SOCKET;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating MediaLB socket failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of the USB port.
 *  \param  self    Instance pointer
 */
void Xrm_CreateUsbPort(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_UsbPort_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_UsbPort_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    Ucs_Return_t result = Inic_UsbPortCreate(self->inic_ptr,
                                             cfg_ptr->index,
                                             cfg_ptr->physical_layer,
                                             cfg_ptr->devices_interfaces,
                                             cfg_ptr->streaming_if_ep_out_count,
                                             cfg_ptr->streaming_if_ep_in_count,
                                             &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating USB port", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_USB_PORT;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating USB port failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a USB socket.
 *  \param  self    Instance pointer
 */
void Xrm_CreateUsbSocket(CExtendedResourceManager *self)
{
    Ucs_Return_t result;
    uint16_t usb_port_handle;
    UCS_XRM_CONST Ucs_Xrm_UsbSocket_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_UsbSocket_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    if(Xrm_IsDefaultCreatedPort(self, cfg_ptr->usb_port_obj_ptr) != false)
    {
        usb_port_handle = Xrm_CreatePortHandle(self,
                                               UCS_XRM_PORT_TYPE_USB,
                                               ((UCS_XRM_CONST Ucs_Xrm_DefaultCreatedPort_t *)(UCS_XRM_CONST void*)(cfg_ptr->usb_port_obj_ptr))->index);
    }
    else
    {
        usb_port_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->usb_port_obj_ptr, NULL);
    }
    result = Inic_UsbSocketCreate(self->inic_ptr,
                                  usb_port_handle,
                                  cfg_ptr->direction,
                                  cfg_ptr->data_type,
                                  cfg_ptr->end_point_addr,
                                  cfg_ptr->frames_per_transfer,
                                  &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating USB socket", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_USB_SOCKET;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating USB socket failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of the RMCK port.
 *  \param  self    Instance pointer
 */
void Xrm_CreateRmckPort(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_RmckPort_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_RmckPort_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    Ucs_Return_t result = Inic_RmckPortCreate(self->inic_ptr,
                                                 cfg_ptr->index,
                                                 cfg_ptr->clock_source,
                                                 cfg_ptr->divisor,
                                                 &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating RMCK port", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_RMCK_PORT;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating RMCK port failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a streaming port.
 *  \param  self    Instance pointer
 */
void Xrm_CreateStreamPort(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_StrmPort_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_StrmPort_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    Ucs_Return_t result = Inic_StreamPortCreate(self->inic_ptr,
                                                cfg_ptr->index,
                                                cfg_ptr->clock_config,
                                                cfg_ptr->data_alignment,
                                                &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating streaming port", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_STRM_PORT;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating streaming port failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a streaming data socket.
 *  \param  self    Instance pointer
 */
void Xrm_CreateStreamSocket(CExtendedResourceManager *self)
{
    Ucs_Return_t result;
    uint16_t stream_port_handle;
    UCS_XRM_CONST Ucs_Xrm_StrmSocket_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_StrmSocket_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    if(Xrm_IsDefaultCreatedPort(self, cfg_ptr->stream_port_obj_ptr) != false)
    {
        stream_port_handle = Xrm_CreatePortHandle(self,
                                                  UCS_XRM_PORT_TYPE_STRM,
                                                  ((UCS_XRM_CONST Ucs_Xrm_DefaultCreatedPort_t *)(UCS_XRM_CONST void*)(cfg_ptr->stream_port_obj_ptr))->index);
    }
    else
    {
        stream_port_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->stream_port_obj_ptr, NULL);
    }
    result = Inic_StreamSocketCreate(self->inic_ptr,
                                     stream_port_handle,
                                     cfg_ptr->direction,
                                     cfg_ptr->data_type,
                                     cfg_ptr->bandwidth,
                                     cfg_ptr->stream_pin_id,
                                     &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating streaming data socket", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_STRM_SOCKET;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating streaming data socket failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a synchronous data connection.
 *  \param  self    Instance pointer
 */
void Xrm_CreateSyncCon(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_SyncCon_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_SyncCon_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t in_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_in_obj_ptr, NULL);
    uint16_t out_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_out_obj_ptr, NULL);
    Ucs_Return_t result = Inic_SyncCreate(self->inic_ptr,
                                          in_socket_handle,
                                          out_socket_handle,
                                          false,
                                          cfg_ptr->mute_mode,
                                          cfg_ptr->offset,
                                          &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating synchronous data connection", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_SYNC_CON;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating synchronous data connection failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a DiscreteFrame Isochronous streaming phase connection.
 *  \param  self    Instance pointer
 */
void Xrm_CreateDfiPhaseCon(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_DfiPhaseCon_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_DfiPhaseCon_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t in_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_in_obj_ptr, NULL);
    uint16_t out_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_out_obj_ptr, NULL);
    Ucs_Return_t result = Inic_DfiPhaseCreate(self->inic_ptr,
                                              in_socket_handle,
                                              out_socket_handle,
                                              &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating DFIPhase connection", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_DFIPHASE_CON;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating DFIPhase connection failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a combiner resource.
 *  \param  self    Instance pointer
 */
void Xrm_CreateCombiner(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_Combiner_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_Combiner_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t port_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->port_socket_obj_ptr, NULL);
    Ucs_Return_t result = Inic_CombinerCreate(self->inic_ptr,
                                              port_socket_handle,
                                              cfg_ptr->most_port_handle,
                                              cfg_ptr->bytes_per_frame,
                                              &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating combiner resource", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_COMBINER;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating combiner resource failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a splitter resource.
 *  \param  self    Instance pointer
 */
void Xrm_CreateSplitter(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_Splitter_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_Splitter_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t socket_handle_in = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_in_obj_ptr, NULL);
    Ucs_Return_t result = Inic_SplitterCreate(self->inic_ptr,
                                              socket_handle_in,
                                              cfg_ptr->most_port_handle,
                                              cfg_ptr->bytes_per_frame,
                                              &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating splitter resource", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_SPLITTER;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating splitter resource failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a A/V packetized isochronous streaming data connection.
 *  \param  self    Instance pointer
 */
void Xrm_CreateAvpCon(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_AvpCon_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_AvpCon_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t in_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_in_obj_ptr, NULL);
    uint16_t out_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_out_obj_ptr, NULL);
    Ucs_Return_t result = Inic_AvpCreate(self->inic_ptr,
                                         in_socket_handle,
                                         out_socket_handle,
                                         cfg_ptr->isoc_packet_size,
                                         &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating A/V packetized isochronous streaming data connection", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_AVP_CON;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating A/V packetized isochronous streaming data connection failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Triggers the creation of a Quality of Service IP streaming data connection.
 *  \param  self    Instance pointer
 */
void Xrm_CreateQoSCon(CExtendedResourceManager *self)
{
    UCS_XRM_CONST Ucs_Xrm_QoSCon_t *cfg_ptr = (UCS_XRM_CONST Ucs_Xrm_QoSCon_t *)(UCS_XRM_CONST void*)(*self->current_obj_pptr);
    uint16_t in_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_in_obj_ptr, NULL);
    uint16_t out_socket_handle = Xrm_GetResourceHandle(self, self->current_job_ptr, cfg_ptr->socket_out_obj_ptr, NULL);
    Ucs_Return_t result = Inic_QoSCreate(self->inic_ptr,
                                         in_socket_handle,
                                         out_socket_handle,
                                         &self->obs.std_result_obs);
    if(result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating QoS IP streaming data connection", 0U));
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_PROCESS);
    }
    else
    {
        self->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self->report_result.details.resource_type = UCS_XRM_RC_TYPE_QOS_CON;
        self->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self,
                                                                                self->current_job_ptr,
                                                                                self->current_obj_pptr);
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Xrm_HandleError(self);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start creating QoS IP streaming data connection failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Process the result of the INIC resource monitor.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Reference to result data. Result must be casted into data type 
 *                      Inic_StdResult_t.
 */
void Xrm_ResourceMonitorCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if((result_ptr_->result.code == UCS_RES_SUCCESS) && (result_ptr_->data_info != NULL))
    {
        Ucs_Resource_MonitorState_t state = *((Ucs_Resource_MonitorState_t *)result_ptr_->data_info);
        if(state == UCS_INIC_RES_MON_STATE_ACT_REQ)
        {
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_REQ_INV_RES_LST);
        }
        else if((state == UCS_INIC_RES_MON_STATE_OK) && (self_->obs.check_unmute_fptr != NULL))
        {
            self_->obs.check_unmute_fptr(Inic_GetTargetAddress(self_->inic_ptr), self_->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Retrieves the list of invalid resources.
 *  \param  self    Instance pointer
 */
void Xrm_RequestResourceList(CExtendedResourceManager *self)
{
    if(Xrm_IsApiFree(self) != false)
    {
        Ucs_Return_t result;
        result = Inic_ResourceInvalidList_Get(self->inic_ptr,
                                                &self->obs.resource_invalid_list_obs);
        if(result == UCS_RET_SUCCESS)
        {
            Xrm_ApiLocking(self, true);
        }
        else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
        {
            Xrm_WaitForTxMsgObj(self, XRM_EVENT_REQ_INV_RES_LST);
        }
        else
        {
            self->report_result.code = UCS_XRM_RES_ERR_INV_LIST;
            self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
            self->report_result.details.int_result = result;
            Srv_SetEvent(&self->xrm_srv, XRM_EVENT_ERROR);
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start requesting invalid resources failed. Return value: 0x%02X", 1U, result));
        }
    }
    else
    {
        self->queued_event_mask |= XRM_EVENT_REQ_INV_RES_LST;
    }
}

/*! \brief  Process the received list of invalid resources.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Reference to result data. Result must be casted into data type 
 *                      Inic_StdResult_t.
 */
void Xrm_RequestResourceListResultCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if((result_ptr_->result.code == UCS_RES_SUCCESS) && (result_ptr_->data_info != NULL))
    {
        Inic_ResHandleList_t resource_handle_list = *((Inic_ResHandleList_t *)result_ptr_->data_info);
        if((resource_handle_list.res_handles != NULL) && (resource_handle_list.num_handles > 0U))
        {
            MISC_MEM_CPY(&self_->inv_resource_handle_list[0],
                         &resource_handle_list.res_handles[0],
                         (resource_handle_list.num_handles * sizeof(resource_handle_list.res_handles[0])));
        }
        self_->inv_resource_handle_list_size = resource_handle_list.num_handles;
        self_->inv_resource_handle_index     = 0U;

        Xrmp_Foreach(self_->xrmp_ptr, &Xrm_SetCurrJobPtr, &resource_handle_list.res_handles[0], NULL, self);
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_DESTROY_INV_RES);
    }
    else
    {
        self_->report_result.code = UCS_XRM_RES_ERR_INV_LIST;
        if (result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
        {
            self_->report_result.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TX;
        }
        else
        {
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TGT;
        }

        self_->report_result.details.inic_result = result_ptr_->result;
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Request of invalid resources failed. Return value: 0x%02X", 1U, result_ptr_->result.code));
        TR_ERROR_INIC_RESULT(self_->base_ptr->ucs_user_ptr, "[XRM]", result_ptr_->result.info_ptr, result_ptr_->result.info_size);
    }

    Xrm_ApiLocking(self_, false);
}

/*! \brief  Triggers the destruction of INIC resources.
 *  \param  self        Instance pointer
 *  \param  result_fptr Result callback function pointer
 */
void Xrm_DestroyResources(CExtendedResourceManager *self, Sobs_UpdateCb_t result_fptr)
{
    Ucs_Return_t result;
    Inic_ResHandleList_t list;
    list.res_handles = &self->inv_resource_handle_list[self->inv_resource_handle_index];
    if (self->inv_resource_handle_list_size > MAX_INVALID_HANDLES_LIST)
    {
        list.num_handles = MAX_INVALID_HANDLES_LIST;
        self->curr_dest_resource_handle_size = list.num_handles;
    }
    else
    {
        list.num_handles = self->inv_resource_handle_list_size;
        self->curr_dest_resource_handle_size = list.num_handles;
        if(self->inv_resource_handle_list[(self->inv_resource_handle_index + self->inv_resource_handle_list_size) - 1U] == XRM_INVALID_RESOURCE_HANDLE)
        {
            list.num_handles--;
        }
    }
    Sobs_Ctor(&self->obs.resource_destroy_obs, self, result_fptr);
    result = Inic_ResourceDestroy(self->inic_ptr,
                                  list, 
                                  &self->obs.resource_destroy_obs);
    if(result == UCS_RET_SUCCESS)
    {
        /* No error */
#ifdef UCS_TR_INFO
        uint8_t i;
        TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "Destruction of invalid resource handles been successfully started:", 0U));
        for(i=0U; i<list.num_handles; i++)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[XRM]", "--> Handle: 0x%04X", 1U, list.res_handles[i]));
        }
#endif
    }
    else if (result == UCS_RET_ERR_PARAM)
    {
        /* empty list */
        if ((list.num_handles == 0U) && (list.res_handles[0] == XRM_INVALID_RESOURCE_HANDLE))
        {
            self->inv_resource_handle_list_size = 0U;
            Srv_SetEvent(&self->xrm_srv, XRM_EVENT_RESET_RES_MONITOR);
        }
    }
    else if(result == UCS_RET_ERR_BUFFER_OVERFLOW)
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_DESTROY_INV_RES);
    }
    else
    {
        self->inv_resource_handle_list_size = 0U;
        self->report_result.code = UCS_XRM_RES_ERR_DESTROY;
        self->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
        self->report_result.details.int_result = result;
        Srv_SetEvent(&self->xrm_srv, XRM_EVENT_ERROR);
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[XRM]", "Start destroying invalid resources failed. Return value: 0x%02X", 1U, result));
    }
}

/*! \brief  Resets the INIC's Resource Monitor.
 *  \param  self    Instance pointer
 */
void Xrm_ResetResourceMonitor(CExtendedResourceManager *self)
{
    Ucs_Return_t result = Inic_ResourceMonitor_Set(self->inic_ptr, UCS_INIC_RES_MON_CTRL_RESET);
    if(result == UCS_RET_SUCCESS)
    {
        Srv_SetEvent(&self->xrm_srv, XRM_EVENT_NOTIFY_AUTO_DEST_RES);
    }
    else
    {
        Xrm_WaitForTxMsgObj(self, XRM_EVENT_RESET_RES_MONITOR);
    }
}


/*! \brief  Handles the result of resource destructions.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Reference to result data. Result must be casted into data type 
 *                      Inic_StdResult_t.
 */
void Xrm_DestroyResourcesResultCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
    if(result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        (void)Xrm_ReleaseResourceHandles(self_,
                                   NULL,
                                   &self_->inv_resource_handle_list[self_->inv_resource_handle_index],
                                   self_->curr_dest_resource_handle_size,
                                   XRM_INVALID_RESOURCE_HANDLE);

        if (self_->inv_resource_handle_list_size >= self_->curr_dest_resource_handle_size)
        {
            self_->inv_resource_handle_list_size -= self_->curr_dest_resource_handle_size;
            if (self_->inv_resource_handle_list_size > 0U)
            {
                self_->inv_resource_handle_index += self_->curr_dest_resource_handle_size;
                Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_DESTROY_INV_RES);
            }
            else
            {
                if(self_->inv_resource_handle_list[(self_->inv_resource_handle_index + self_->curr_dest_resource_handle_size) - 1U] != XRM_INVALID_RESOURCE_HANDLE)
                {
                    Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_REQ_INV_RES_LST);
                }
                else
                {
                    Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_RESET_RES_MONITOR);
                }
                TR_INFO((self_->base_ptr->ucs_user_ptr, "[XRM]", "INIC resources been successfully destroyed.", 0U));
            }
        }
        else
        {
            self_->inv_resource_handle_list_size = 0U;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.code = UCS_XRM_RES_ERR_DESTROY;
            self_->report_result.details.inic_result = result_ptr_->result;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Destruction of invalid resources failed. Internal resources handles List is corrupted", 0U));
        }
    }
    else if(result_ptr_->result.code == UCS_RES_ERR_BUSY)
    {
        uint8_t stop_index;
        uint16_t failed_resource_handle;
        MISC_DECODE_WORD(&(failed_resource_handle), &(result_ptr_->result.info_ptr[3]));
        stop_index = Xrm_ReleaseResourceHandles(self_,
                                   NULL,
                                   &self_->inv_resource_handle_list[self_->inv_resource_handle_index],
                                   self_->curr_dest_resource_handle_size,
                                   failed_resource_handle);

        if (stop_index > 0U)
        {
            self_->inv_resource_handle_index = stop_index;
            self_->inv_resource_handle_list_size -= stop_index;
        }
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_DESTROY_INV_RES);
    }
    else
    {
        self_->inv_resource_handle_list_size = 0U;
        if (result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
        {
            self_->report_result.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TX;
        }
        else
        {
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TGT;
        }
        self_->report_result.code = UCS_XRM_RES_ERR_DESTROY;
        self_->report_result.details.inic_result = result_ptr_->result;
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Destruction of invalid resources failed. Return value: 0x%02X", 1U, result_ptr_->result.code));
        TR_ERROR_INIC_RESULT(self_->base_ptr->ucs_user_ptr, "[XRM]", result_ptr_->result.info_ptr, result_ptr_->result.info_size);
    }
}

/*! \brief  Handles the result of resource destructions for all resources of a job.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Reference to result data. Result must be casted into data type 
 *                      Inic_StdResult_t.
 */
void Xrm_DestroyJobResourcesResultCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
    if(result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        (void)Xrm_ReleaseResourceHandles(self_,
                                   self_->current_job_ptr,
                                   &self_->inv_resource_handle_list[self_->inv_resource_handle_index],
                                   self_->curr_dest_resource_handle_size,
                                   XRM_INVALID_RESOURCE_HANDLE);

        if (self_->inv_resource_handle_list_size >= self_->curr_dest_resource_handle_size)
        {
            self_->inv_resource_handle_list_size -= self_->curr_dest_resource_handle_size;
            self_->inv_resource_handle_index = 0U;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_RESUME_JOB_DESTRUCT);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[XRM]", "INIC resources been successfully destroyed.", 0U));
        }
        else
        {
            self_->inv_resource_handle_list_size = 0U;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.code = UCS_XRM_RES_ERR_DESTROY;
            self_->report_result.details.inic_result = result_ptr_->result;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Destruction of invalid resources failed. Internal resources handles List is corrupted", 0U));
        }
    }
    else if(result_ptr_->result.code == UCS_RES_ERR_BUSY)
    {
        uint16_t failed_handle;
        MISC_DECODE_WORD(&(failed_handle), &(result_ptr_->result.info_ptr[3]));
        (void)Xrm_ReleaseResourceHandles(self_,
                                   NULL,
                                   &self_->inv_resource_handle_list[self_->inv_resource_handle_index],
                                   self_->curr_dest_resource_handle_size,
                                   failed_handle);

        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_RESUME_JOB_DESTRUCT);
    }
    else
    {
        self_->inv_resource_handle_list_size = 0U;
        if (result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
        {
            self_->report_result.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TX;
        }
        else
        {
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TGT;
        }

        self_->report_result.code = UCS_XRM_RES_ERR_DESTROY;
        self_->report_result.details.inic_result = result_ptr_->result;
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Destruction of invalid resources failed. Return value: 0x%02X", 1U, result_ptr_->result.code));
        TR_ERROR_INIC_RESULT(self_->base_ptr->ucs_user_ptr, "[XRM]", result_ptr_->result.info_ptr, result_ptr_->result.info_size);
    }
}

/*! \brief  Handles the result of "create port", "create socket" and "create connection" operations.
 *  \param  self        Instance pointer
 *  \param  result_ptr  Reference to result data. Result must be casted into data type 
 *                      Inic_StdResult_t.
 */
void Xrm_StdResultCb(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if((result_ptr_->result.code == UCS_RES_SUCCESS) && (result_ptr_->data_info != NULL))
    {
        uint16_t resource_handle = 0U;
        if(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self_->current_obj_pptr) == UCS_XRM_RC_TYPE_MOST_SOCKET)
        {
            Inic_MostSocketCreate_Result_t res_ack = {0U, 0U};
            res_ack = *((Inic_MostSocketCreate_Result_t *)result_ptr_->data_info);
            resource_handle = res_ack.most_socket_handle;
            self_->current_job_ptr->connection_label = res_ack.conn_label;
        }
        else
        {
            resource_handle = *((uint16_t *)result_ptr_->data_info);
        }

        if(Xrm_StoreResourceHandle(self_, resource_handle, self_->current_job_ptr, *self_->current_obj_pptr) != false)
        {
            if (self_->res_debugging_fptr != NULL)
            {
                self_->res_debugging_fptr(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self_->current_obj_pptr), *self_->current_obj_pptr, UCS_XRM_INFOS_BUILT, 
                                            self_->current_job_ptr->user_arg, self_->base_ptr->ucs_user_ptr);
            }

            self_->current_obj_pptr++;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_PROCESS);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[XRM]", "Resource has been successfully created. Handle: 0x%04X", 1U, resource_handle));
        }
        else
        {
            self_->report_result.code = UCS_XRM_RES_ERR_CONFIG;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Misconfiguration. Resource handle list is too small.", 0U));
        }
    }
    else
    {
        self_->current_job_ptr->valid = false;

        if (result_ptr_->result.code == UCS_RES_ERR_TRANSMISSION)
        {
            self_->report_result.details.tx_result = *(Ucs_MsgTxStatus_t *)(result_ptr_->data_info);
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TX;
        }
        else
        {
            self_->report_result.details.tx_result = UCS_MSG_STAT_OK;
            self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TGT;
        }

        self_->report_result.code = UCS_XRM_RES_ERR_BUILD;
        self_->report_result.details.resource_type = *(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self_->current_obj_pptr);
        self_->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self_,
                                                                                 self_->current_job_ptr,
                                                                                 self_->current_obj_pptr);
        self_->report_result.details.inic_result = result_ptr_->result;
        Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);

        if (self_->res_debugging_fptr != NULL)
        {
            self_->res_debugging_fptr(*(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self_->current_obj_pptr),
                                  *self_->current_obj_pptr, UCS_XRM_INFOS_ERR_BUILT, self_->current_job_ptr->user_arg, self_->base_ptr->ucs_user_ptr);
        }
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Creation of resource failed. Result code: 0x%02X", 1U, result_ptr_->result.code));
        if (result_ptr_->result.info_ptr != NULL)
        {
            TR_ERROR_INIC_RESULT(self_->base_ptr->ucs_user_ptr, "[XRM]", result_ptr_->result.info_ptr, result_ptr_->result.info_size);
        }
    }
}

/*! \brief  Handles the result of "device.sync" operations.
 *  \param  self        Instance pointer
 *  \param  result      RSM result
 */
void Xrm_RmtDevAttachResultCb(void *self, Rsm_Result_t result)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    if (result.code == RSM_RES_SUCCESS)
    {
        Srv_SetEvent(&self_->xrm_srv, self_->queued_event_mask);
        self_->queued_event_mask = 0U;
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[XRM]", "Remote device has been successfully synchronized.", 0U));
    }
    else
    {
        /* In case of StreamingConfig, simulate an error configuration since there 
         * is currently no possibility to signal SyncLost  
         */
        if ((self_->queued_event_mask  == XRM_EVENT_STREAMPORT_CONFIG_SET) ||
            (self_->queued_event_mask  == XRM_EVENT_STREAMPORT_CONFIG_GET))
        {
            Inic_StdResult_t sim_inic_res;
            sim_inic_res.result.code = result.details.inic_result.code;
            sim_inic_res.result.info_ptr = NULL;
            sim_inic_res.result.info_size = 0U;
            sim_inic_res.data_info = NULL;

            self_->queued_event_mask = 0U;
            /* Force a Notification of the Streaming observer */
            self_->obs.stream_port_config_fptr = self_->current_streamport_config.result_fptr;
            self_->obs.stream_port_config_obs.update_fptr(self, &sim_inic_res);
        }
        else
        {
            if (result.details.inic_result.code == UCS_RES_ERR_TRANSMISSION)
            {
                self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TX;
            }
            else
            {
                self_->report_result.details.result_type = UCS_XRM_RESULT_TYPE_TGT;
            }
            self_->report_result.code = UCS_XRM_RES_ERR_SYNC;
            self_->report_result.details.inic_result.code = result.details.inic_result.code;
            self_->report_result.details.inic_result.info_ptr  = result.details.inic_result.info_ptr;
            self_->report_result.details.inic_result.info_size = result.details.inic_result.info_size;
            self_->report_result.details.resource_type = *(UCS_XRM_CONST Ucs_Xrm_ResourceType_t *)(UCS_XRM_CONST void*)(*self_->current_obj_pptr);
            self_->report_result.details.resource_index = Xrm_GetResourceObjectIndex(self_,
                                                                                        self_->current_job_ptr,
                                                                                        self_->current_obj_pptr);
            self_->report_result.details.tx_result = (Ucs_MsgTxStatus_t)result.details.tx_result;

            self_->queued_event_mask = 0U;
            Srv_SetEvent(&self_->xrm_srv, XRM_EVENT_ERROR);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[XRM]", "Synchronization to the remote device failed. Result code: 0x%02X", 1U, result));
        }
    }
}


/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CExtendedResourceManager (INIC Resource Management API)                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   This function is used to configure a Streaming Port.
 *  \param   self               Instance pointer
 *  \param   index              Streaming Port instance.
 *  \param   op_mode            Operation mode of the Streaming Port.
 *  \param   port_option        Direction of the physical pins of the indexed Streaming Port.
 *  \param   clock_mode         Configuration of the FSY/SCK signals.
 *  \param   clock_data_delay   Configuration of the FSY/SCK signals for Generic Streaming.
 *  \param   result_fptr        Required result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Invalid callback pointer
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Xrm_Stream_SetPortConfig(CExtendedResourceManager *self,
                                      uint8_t index,
                                      Ucs_Stream_PortOpMode_t op_mode,
                                      Ucs_Stream_PortOption_t port_option,
                                      Ucs_Stream_PortClockMode_t clock_mode,
                                      Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                      Ucs_Xrm_Stream_PortCfgResCb_t result_fptr)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_API_LOCKED;

    if ((self != NULL)  && (result_fptr != NULL) )
    {
        if(Xrm_IsApiFree(self) != false)
        {
            Xrm_ApiLocking(self, true);

            self->current_streamport_config.index = index;
            self->current_streamport_config.op_mode = op_mode;
            self->current_streamport_config.port_option = port_option;
            self->current_streamport_config.clock_mode = clock_mode;
            self->current_streamport_config.clock_data_delay = clock_data_delay;
            self->current_streamport_config.result_fptr = result_fptr;

            ret_val = Xrm_SetStreamPortConfiguration(self);
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_PARAM;
    }

    return ret_val;
}

/*! \brief   This function is used to configure a Streaming Port.
 *  \param   self               Instance pointer
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Invalid callback pointer
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 */
Ucs_Return_t Xrm_SetStreamPortConfiguration (CExtendedResourceManager *self)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (Xrm_IsCurrDeviceAlreadyAttached(self) == false)
    {
        ret_val = Xrm_RemoteDeviceAttach(self, XRM_EVENT_STREAMPORT_CONFIG_SET);
    }
    else
    {
        ret_val = Inic_StreamPortConfig_SetGet(self->inic_ptr,
                                                self->current_streamport_config.index,
                                                self->current_streamport_config.op_mode,
                                                self->current_streamport_config.port_option,
                                                self->current_streamport_config.clock_mode,
                                                self->current_streamport_config.clock_data_delay,
                                                &self->obs.stream_port_config_obs);
        if(ret_val == UCS_RET_SUCCESS)
        {
            self->obs.stream_port_config_fptr = self->current_streamport_config.result_fptr;
        }
        else if(ret_val == UCS_RET_ERR_BUFFER_OVERFLOW)
        {
            Xrm_WaitForTxMsgObj(self, XRM_EVENT_STREAMPORT_CONFIG_SET);
        }
        else if (ret_val == UCS_RET_ERR_API_LOCKED)
        {
            Xrm_ApiLocking(self, false);
        }
    }

    return ret_val;
}

/*! \brief   This function requests the configurations of a Streaming Port.
 *  \param   self           Instance pointer
 *  \param   index          Streaming Port instance.
 *  \param   result_fptr    Required result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ----------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_AVAILABLE   | Associated device not found
 */
Ucs_Return_t Xrm_Stream_GetPortConfig(CExtendedResourceManager *self,
                                      uint8_t index,
                                      Ucs_Xrm_Stream_PortCfgResCb_t result_fptr)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_API_LOCKED;

    if((self != NULL) && (result_fptr != NULL) )
    {
        if(Xrm_IsApiFree(self) != false)
        {
            Xrm_ApiLocking(self, true);

            self->current_streamport_config.index = index;
            self->current_streamport_config.result_fptr = result_fptr;

            ret_val = Xrm_GetStreamPortConfiguration(self);
            if (ret_val == UCS_RET_ERR_API_LOCKED)
            {
                /* from another process locked */
                Xrm_ApiLocking(self, false);
            }
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_PARAM;
    }

    return ret_val;
}

/*! \brief   This function is used to configure a Streaming Port.
 *  \param   self               Instance pointer
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Invalid callback pointer
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available 
 */
Ucs_Return_t Xrm_GetStreamPortConfiguration (CExtendedResourceManager *self)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if (Xrm_IsCurrDeviceAlreadyAttached(self) == false)
    {
        ret_val = Xrm_RemoteDeviceAttach(self, XRM_EVENT_STREAMPORT_CONFIG_GET);
    }
    else
    {
        ret_val = Inic_StreamPortConfig_Get(self->inic_ptr,
                                            self->current_streamport_config.index,
                                            &self->obs.stream_port_config_obs);
        if(ret_val == UCS_RET_SUCCESS)
        {
            self->obs.stream_port_config_fptr = self->current_streamport_config.result_fptr;
        }
        else if(ret_val == UCS_RET_ERR_BUFFER_OVERFLOW)
        {
            Xrm_WaitForTxMsgObj(self, XRM_EVENT_STREAMPORT_CONFIG_GET);
        }
    }

    return ret_val;
}

/*! \brief Observer callback for Inic_StreamPortConfig_Get(). Casts the result and invokes
 *         the application result callback.
 *  \param self         Instance pointer
 *  \param result_ptr   Reference to result
 */
void Xrm_Stream_PortConfigResult(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    Inic_StreamPortConfigStatus_t status;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
    Ucs_StdResult_t res = result_ptr_->result;

    if(self_->obs.stream_port_config_fptr != NULL)
    {
        if((result_ptr_->result.code == UCS_RES_SUCCESS) && (result_ptr_->data_info != NULL))
        {
            status = *((Inic_StreamPortConfigStatus_t *)result_ptr_->data_info);
        }
        else
        {
            /* Fill param callback function with default (dummy) values */
            status.index            = 0x00U;
            status.op_mode          = UCS_STREAM_PORT_OP_MODE_GENERIC;
            status.port_option      = UCS_STREAM_PORT_OPT_IN_OUT;
            status.clock_mode       = UCS_STREAM_PORT_CLK_MODE_OUTPUT;
            status.clock_data_delay = UCS_STREAM_PORT_CLK_DLY_NONE;
        }
        self_->obs.stream_port_config_fptr(Inic_GetTargetAddress(self_->inic_ptr),
                                           status.index,
                                           status.op_mode,
                                           status.port_option,
                                           status.clock_mode,
                                           status.clock_data_delay,
                                           res,
                                           self_->base_ptr->ucs_user_ptr);
    }

    Xrm_ApiLocking(self_, false);
}

/*! \brief   Enables or disables a specific MOST Network Port.
 *  \param   self               Instance pointer
 *  \param   most_port_handle   Port resource handle.
 *  \param   enabled            State of the MOST Port.
 *  \param   result_fptr        Optional result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available 
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 */
Ucs_Return_t Xrm_Most_EnablePort(CExtendedResourceManager *self,
                                 uint16_t most_port_handle, 
                                 bool enabled, 
                                 Ucs_StdResultCb_t result_fptr)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if(Xrm_IsApiFree(self) != false)
    {
        ret_val = Inic_MostPortEnable(self->inic_ptr,
                                        most_port_handle,
                                        enabled,
                                        &self->obs.most_port_enable_obs);
        if(ret_val == UCS_RET_SUCCESS)
        {
            self->obs.most_port_enable_fptr = result_fptr;
        }
    }

    return ret_val;
}

/*! \brief   Enables full streaming for a specific MOST Network Port.
 *  \param   self               Instance pointer
 *  \param   most_port_handle   Port resource handle.
 *  \param   enabled            State of the MOST Port related to full streaming.
 *  \param   result_fptr        Optional result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error 
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available 
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 */
Ucs_Return_t Xrm_Most_PortEnFullStr(CExtendedResourceManager *self,
                                    uint16_t most_port_handle, 
                                    bool enabled, 
                                    Ucs_StdResultCb_t result_fptr)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_INITIALIZED;

    if(Xrm_IsApiFree(self) != false)
    {
        ret_val = Inic_MostPortEnFullStr(self->inic_ptr,
                                            most_port_handle,
                                            enabled,
                                            &self->obs.most_port_en_full_str_obs);
        if(ret_val == UCS_RET_SUCCESS)
        {
            self->obs.most_port_en_full_str_fptr = result_fptr;
        }
    }

    return ret_val;
}

/*! \brief Observer callback for Inic_MostPortEnable(). Casts the result and invokes
 *         the application result callback.
 *  \param self         Instance pointer
 *  \param result_ptr   Reference to result
 */
void Xrm_Most_PortEnableResult(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    if(self_->obs.most_port_enable_fptr != NULL)
    {
        Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
        self_->obs.most_port_enable_fptr(result_ptr_->result, self_->base_ptr->ucs_user_ptr);
    }
}

/*! \brief Observer callback for Inic_MostPortEnFullStr(). Casts the result and invokes
 *         the application result callback.
 *  \param self         Instance pointer
 *  \param result_ptr   Reference to result
 */
void Xrm_Most_PortEnFullStrResult(void *self, void *result_ptr)
{
    CExtendedResourceManager *self_ = (CExtendedResourceManager *)self;
    if(self_->obs.most_port_en_full_str_fptr != NULL)
    {
        Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;
        self_->obs.most_port_en_full_str_fptr(result_ptr_->result, self_->base_ptr->ucs_user_ptr);
    }
}

/*! \brief  Sets the current job pointer of the CExtendedResourceManager instance.
 *  \param  resrc_ptr       Reference to the resource handle list to be looked for.
 *  \param  resrc_handle    Reference to the resource handle to be found.
 *  \param  job_ptr         Reference to the job to be looked for.
 *  \param  user_arg        Reference to a user argument.
 *  \return \c false to continue the for-each-loop of the resources list table, otherwise \c true
 */
bool Xrm_SetCurrJobPtr(void *resrc_ptr, void *resrc_handle, void *job_ptr, void * user_arg)
{
    bool ret_val = false;
    Xrm_ResourceHandleListItem_t * resrc_ptr_ = (Xrm_ResourceHandleListItem_t *)resrc_ptr;
    uint16_t * resrc_handle_ = (uint16_t *)resrc_handle;
    CExtendedResourceManager *self = (CExtendedResourceManager *)user_arg;

    MISC_UNUSED(job_ptr);

    if ((resrc_ptr_->resource_handle == *resrc_handle_) &&
        (*resrc_handle_ != XRM_INVALID_RESOURCE_HANDLE) &&
        (Dl_IsNodeInList(&self->job_list, &resrc_ptr_->job_ptr->node)))
    {
        self->current_job_ptr = resrc_ptr_->job_ptr;
        ret_val = true;
    }

    return ret_val;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

