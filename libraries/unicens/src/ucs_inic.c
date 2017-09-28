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
 * \brief   Implementation of FBlock INIC
 * \details Contains the general, device an network management parts of INIC management
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_INIC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"
#include "ucs_ret_pb.h"
#include "ucs_inic.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief List of all INIC messages */
static const Dec_FktOpIcm_t inic_handler[] =       /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { DEC_FKTOP(INIC_FID_NOTIFICATION,            UCS_OP_STATUS),     Inic_Notification_Status },
    { DEC_FKTOP(INIC_FID_NOTIFICATION,            UCS_OP_ERROR),      Inic_Notification_Error },
    { DEC_FKTOP(INIC_FID_DEVICE_STATUS,           UCS_OP_STATUS),     Inic_DeviceStatus_Status },
    { DEC_FKTOP(INIC_FID_DEVICE_STATUS,           UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_DEVICE_VERSION,          UCS_OP_STATUS),     Inic_DeviceVersion_Status },
    { DEC_FKTOP(INIC_FID_DEVICE_VERSION,          UCS_OP_ERROR),      Inic_DeviceVersion_Error },
    { DEC_FKTOP(INIC_FID_DEVICE_POWER_OFF,        UCS_OP_STATUS),     Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_DEVICE_POWER_OFF,        UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_DEVICE_ATTACH,           UCS_OP_RESULT),     Inic_DeviceAttach_Result },
    { DEC_FKTOP(INIC_FID_DEVICE_ATTACH,           UCS_OP_ERROR),      Inic_DeviceAttach_Error },
    { DEC_FKTOP(INIC_FID_DEVICE_SYNC,             UCS_OP_RESULT),     Inic_DeviceSync_Result },
    { DEC_FKTOP(INIC_FID_DEVICE_SYNC,             UCS_OP_ERROR),      Inic_DeviceSync_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_STATUS,          UCS_OP_STATUS),     Inic_NwStatus_Status },
    { DEC_FKTOP(INIC_FID_MOST_NW_STATUS,          UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_MOST_NW_CFG,             UCS_OP_STATUS),     Inic_NwConfig_Status },
    { DEC_FKTOP(INIC_FID_MOST_NW_CFG,             UCS_OP_ERROR),      Inic_NwConfig_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_FRAME_COUNTER,   UCS_OP_STATUS),     Inic_NwFrameCounter_Status },
    { DEC_FKTOP(INIC_FID_MOST_NW_FRAME_COUNTER,   UCS_OP_ERROR),      Inic_NwFrameCounter_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_STARTUP,         UCS_OP_RESULT),     Inic_NwStartup_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_STARTUP,         UCS_OP_ERROR),      Inic_NwStartup_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_SHUTDOWN,        UCS_OP_RESULT),     Inic_NwShutdown_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_SHUTDOWN,        UCS_OP_ERROR),      Inic_NwShutdown_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_TRIGGER_RBD,     UCS_OP_RESULT),     Inic_NwTriggerRbd_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_TRIGGER_RBD,     UCS_OP_ERROR),      Inic_NwTriggerRbd_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_RBD_RESULT,      UCS_OP_STATUS),     Inic_NwRbdResult_Status },
    { DEC_FKTOP(INIC_FID_MOST_NW_RBD_RESULT,      UCS_OP_ERROR),      Inic_NwRbdResult_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_ATTACH,          UCS_OP_RESULT),     Inic_NwAttach_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_ATTACH,          UCS_OP_ERROR),      Inic_NwAttach_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_FORCE_NO_AVAIL,  UCS_OP_STATUS),     Inic_NwForceNotAvailable_Status },
    { DEC_FKTOP(INIC_FID_MOST_NW_FORCE_NO_AVAIL,  UCS_OP_ERROR),      Inic_NwForceNotAvailable_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_SYS_DIAGNOSIS,   UCS_OP_RESULT),     Inic_NwSysDiagnosis_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_SYS_DIAGNOSIS,   UCS_OP_ERROR),      Inic_NwSysDiagnosis_Error },
    { DEC_FKTOP(INIC_FID_MOST_NW_SYS_DIAG_END,    UCS_OP_RESULT),     Inic_NwSysDiagEnd_Result },
    { DEC_FKTOP(INIC_FID_MOST_NW_SYS_DIAG_END,    UCS_OP_ERROR),      Inic_NwSysDiagEnd_Error },
    { DEC_FKTOP(INIC_FID_BACK_CHANNEL_DIAGNOSIS,  UCS_OP_RESULT),     Inic_BCDiagnosis_Result },
    { DEC_FKTOP(INIC_FID_BACK_CHANNEL_DIAGNOSIS,  UCS_OP_ERROR),      Inic_BCDiagnosis_Error },
    { DEC_FKTOP(INIC_FID_BACK_CHANNEL_DIAG_END,   UCS_OP_RESULT),     Inic_BCDiagEnd_Result },
    { DEC_FKTOP(INIC_FID_BACK_CHANNEL_DIAG_END,   UCS_OP_ERROR),      Inic_BCDiagEnd_Error },
    { DEC_FKTOP(INIC_FID_MOST_PORT_STATUS,        UCS_OP_STATUS),     Inic_MostPortStatus_Status },
    { DEC_FKTOP(INIC_FID_MOST_PORT_STATUS,        UCS_OP_ERROR),      Inic_MostPortStatus_Error },
    { DEC_FKTOP(INIC_FID_MOST_SOCKET_CREATE,      UCS_OP_RESULT),     Inic_MostSocketCreate_Result },
    { DEC_FKTOP(INIC_FID_MOST_SOCKET_CREATE,      UCS_OP_ERROR),      Inic_MostSocketCreate_Error },
    { DEC_FKTOP(INIC_FID_MOST_SOCKET_STATUS,      UCS_OP_STATUS),     Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_MOST_SOCKET_STATUS,      UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_MLB_PORT_CREATE,         UCS_OP_RESULT),     Inic_MlbPortCreate_Result },
    { DEC_FKTOP(INIC_FID_MLB_PORT_CREATE,         UCS_OP_ERROR),      Inic_MlbPortCreate_Error },
/*  { DEC_FKTOP(INIC_FID_MLB_PORT_ALLOCATE_ONLY,  UCS_OP_RESULT),     Inic_DummyHandler },    */
/*  { DEC_FKTOP(INIC_FID_MLB_PORT_ALLOCATE_ONLY,  UCS_OP_ERROR),      Inic_DummyHandler },    */
/*  { DEC_FKTOP(INIC_FID_MLB_PORT_DEALLOC_ONLY,   UCS_OP_RESULT),     Inic_DummyHandler },    */
/*  { DEC_FKTOP(INIC_FID_MLB_PORT_DEALLOC_ONLY,   UCS_OP_ERROR),      Inic_DummyHandler },    */
    { DEC_FKTOP(INIC_FID_MLB_SOCKET_CREATE,       UCS_OP_RESULT),     Inic_MlbSocketCreate_Result },
    { DEC_FKTOP(INIC_FID_MLB_SOCKET_CREATE,       UCS_OP_ERROR),      Inic_MlbSocketCreate_Error },
    { DEC_FKTOP(INIC_FID_SPI_PORT_CREATE,         UCS_OP_RESULT),     Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_SPI_PORT_CREATE,         UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_SPI_SOCKET_CREATE,       UCS_OP_RESULT),     Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_SPI_SOCKET_CREATE,       UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_USB_PORT_CREATE,         UCS_OP_RESULT),     Inic_UsbPortCreate_Result },
    { DEC_FKTOP(INIC_FID_USB_PORT_CREATE,         UCS_OP_ERROR),      Inic_UsbPortCreate_Error },
    { DEC_FKTOP(INIC_FID_USB_SOCKET_CREATE,       UCS_OP_RESULT),     Inic_UsbSocketCreate_Result },
    { DEC_FKTOP(INIC_FID_USB_SOCKET_CREATE,       UCS_OP_ERROR),      Inic_UsbSocketCreate_Error },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_CONFIG,      UCS_OP_STATUS),     Inic_StreamPortConfig_Status },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_CONFIG,      UCS_OP_ERROR),      Inic_StreamPortConfig_Error },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_CREATE,      UCS_OP_RESULT),     Inic_StreamPortCreate_Result },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_CREATE,      UCS_OP_ERROR),      Inic_StreamPortCreate_Error },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_LOOPBACK,    UCS_OP_STATUS),     Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_STREAM_PORT_LOOPBACK,    UCS_OP_ERROR),      Inic_DummyHandler },
    { DEC_FKTOP(INIC_FID_STREAM_SOCKET_CREATE,    UCS_OP_RESULT),     Inic_StreamSocketCreate_Result },
    { DEC_FKTOP(INIC_FID_STREAM_SOCKET_CREATE,    UCS_OP_ERROR),      Inic_StreamSocketCreate_Error },
    { DEC_FKTOP(INIC_FID_RMCK_PORT_CREATE,        UCS_OP_RESULT),     Inic_RmckPortCreate_Result },
    { DEC_FKTOP(INIC_FID_RMCK_PORT_CREATE,        UCS_OP_ERROR),      Inic_RmckPortCreate_Error },
    { DEC_FKTOP(INIC_FID_I2C_PORT_CREATE,         UCS_OP_RESULT),     Inic_I2cPortCreate_Result },
    { DEC_FKTOP(INIC_FID_I2C_PORT_CREATE,         UCS_OP_ERROR),      Inic_I2cPortCreate_Error },
    { DEC_FKTOP(INIC_FID_I2C_PORT_READ,           UCS_OP_RESULT),     Inic_I2cPortRead_Result },
    { DEC_FKTOP(INIC_FID_I2C_PORT_READ,           UCS_OP_ERROR),      Inic_I2cPortRead_Error },
    { DEC_FKTOP(INIC_FID_I2C_PORT_WRITE,          UCS_OP_RESULT),     Inic_I2cPortWrite_Result },
    { DEC_FKTOP(INIC_FID_I2C_PORT_WRITE,          UCS_OP_ERROR),      Inic_I2cPortWrite_Error },
    { DEC_FKTOP(INIC_FID_PCI_PORT_CREATE,         UCS_OP_RESULT),     Inic_PciPortCreate_Result },
    { DEC_FKTOP(INIC_FID_PCI_PORT_CREATE,         UCS_OP_ERROR),      Inic_PciPortCreate_Error },
    { DEC_FKTOP(INIC_FID_PCI_SOCKET_CREATE,       UCS_OP_RESULT),     Inic_PciSocketCreate_Result },
    { DEC_FKTOP(INIC_FID_PCI_SOCKET_CREATE,       UCS_OP_ERROR),      Inic_PciSocketCreate_Error },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_CREATE,        UCS_OP_RESULT),     Inic_GpioPortCreate_Result },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_CREATE,        UCS_OP_ERROR),      Inic_GpioPortCreate_Error },
    { DEC_FKTOP(INIC_FID_MOST_PORT_ENABLE,        UCS_OP_RESULT),     Inic_MostPortEnable_Result },
    { DEC_FKTOP(INIC_FID_MOST_PORT_ENABLE,        UCS_OP_ERROR),      Inic_MostPortEnable_Error },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_PIN_MODE,      UCS_OP_STATUS),     Inic_GpioPortPinMode_Status },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_PIN_MODE,      UCS_OP_ERROR),      Inic_GpioPortPinMode_Error },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_PIN_STATE,     UCS_OP_STATUS),     Inic_GpioPortPinState_Status },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_PIN_STATE,     UCS_OP_ERROR),      Inic_GpioPortPinState_Error },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_TRIGGER_EVENT, UCS_OP_STATUS),     Inic_GpioPortTrigger_Status },
    { DEC_FKTOP(INIC_FID_GPIO_PORT_TRIGGER_EVENT, UCS_OP_ERROR),      Inic_GpioPortTrigger_Error },
    { DEC_FKTOP(INIC_FID_RESOURCE_DESTROY,        UCS_OP_RESULT),     Inic_ResourceDestroy_Result },
    { DEC_FKTOP(INIC_FID_RESOURCE_DESTROY,        UCS_OP_ERROR),      Inic_ResourceDestroy_Error },
    { DEC_FKTOP(INIC_FID_RESOURCE_INVALID_LIST,   UCS_OP_STATUS),     Inic_ResourceInvalidList_Status },
    { DEC_FKTOP(INIC_FID_RESOURCE_INVALID_LIST,   UCS_OP_ERROR),      Inic_ResourceInvalidList_Error },
    { DEC_FKTOP(INIC_FID_RESOURCE_MONITOR,        UCS_OP_STATUS),     Inic_ResourceMonitor_Status },
    { DEC_FKTOP(INIC_FID_RESOURCE_MONITOR,        UCS_OP_ERROR),      Inic_ResourceMonitor_Error },
/*  { DEC_FKTOP(INIC_FID_PACKET_ATTACH_SOCKETS,   UCS_OP_RESULT),     Inic_DummyHandler },   */   
/*  { DEC_FKTOP(INIC_FID_PACKET_ATTACH_SOCKETS,   UCS_OP_ERROR),      Inic_DummyHandler },   */   
/*  { DEC_FKTOP(INIC_FID_PACKET_DETACH_SOCKETS,   UCS_OP_RESULT),     Inic_DummyHandler },   */   
/*  { DEC_FKTOP(INIC_FID_PACKET_DETACH_SOCKETS,   UCS_OP_ERROR),      Inic_DummyHandler },   */   
    { DEC_FKTOP(INIC_FID_QOS_CREATE,              UCS_OP_RESULT),     Inic_QoSCreate_Result },
    { DEC_FKTOP(INIC_FID_QOS_CREATE,              UCS_OP_ERROR),      Inic_QoSCreate_Error },
    { DEC_FKTOP(INIC_FID_AVP_CREATE,              UCS_OP_RESULT),     Inic_AvpCreate_Result },
    { DEC_FKTOP(INIC_FID_AVP_CREATE,              UCS_OP_ERROR),      Inic_AvpCreate_Error },
    { DEC_FKTOP(INIC_FID_SYNC_CREATE,             UCS_OP_RESULT),     Inic_SyncCreate_Result },
    { DEC_FKTOP(INIC_FID_SYNC_CREATE,             UCS_OP_ERROR),      Inic_SyncCreate_Error },
    { DEC_FKTOP(INIC_FID_SYNC_MUTE,               UCS_OP_RESULT),     Inic_SyncMute_Result },
    { DEC_FKTOP(INIC_FID_SYNC_MUTE,               UCS_OP_ERROR),      Inic_SyncMute_Error },
    { DEC_FKTOP(INIC_FID_SYNC_DEMUTE,             UCS_OP_RESULT),     Inic_SyncDemute_Result },
    { DEC_FKTOP(INIC_FID_SYNC_DEMUTE,             UCS_OP_ERROR),      Inic_SyncDemute_Error },
    { DEC_FKTOP(INIC_FID_DFIPHASE_CREATE,         UCS_OP_RESULT),     Inic_DfiPhaseCreate_Result },
    { DEC_FKTOP(INIC_FID_DFIPHASE_CREATE,         UCS_OP_ERROR),      Inic_DfiPhaseCreate_Error },
    { DEC_FKTOP(INIC_FID_IPC_CREATE,              UCS_OP_RESULT),     Inic_IpcCreate_Result },
    { DEC_FKTOP(INIC_FID_IPC_CREATE,              UCS_OP_ERROR),      Inic_IpcCreate_Error },
    { DEC_FKTOP(INIC_FID_COMBINER_CREATE,         UCS_OP_RESULT),     Inic_CombinerCreate_Result },
    { DEC_FKTOP(INIC_FID_COMBINER_CREATE,         UCS_OP_ERROR),      Inic_CombinerCreate_Error },
    { DEC_FKTOP(INIC_FID_SPLITTER_CREATE,         UCS_OP_RESULT),     Inic_SplitterCreate_Result },
    { DEC_FKTOP(INIC_FID_SPLITTER_CREATE,         UCS_OP_ERROR),      Inic_SplitterCreate_Error },
    { DEC_FKTOP_TERMINATION,                                          NULL }
};

/*------------------------------------------------------------------------------------------------*/
/* Internal definitions                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Bitmask for API method Inic_NwForceNotAvailable() used by API locking manager */
#define INIC_API_NW_FORCE_NA        0x01U
/*! \brief Bitmask for API method Inic_NwShutdown() used by API locking manager */
#define INIC_API_NW_SHUTDOWN        0x02U
/*! \brief Bitmask for API method Inic_NwFrameCounter_Get() used by API locking manager */
#define INIC_API_NW_FRAME_COUNTER   0x04U
/*! \brief Bitmask for API method Inic_NwTriggerRbd() used by API locking manager */
#define INIC_API_NW_TRIGGER_RBD     0x08U
/*! \brief Bitmask for API method Inic_NwRbdResult_Get() used by API locking manager */
#define INIC_API_NW_RBD_RESULT      0x10U
/*! \brief Bitmask for API method Inic_DeviceVersion_Get() used by API locking manager */
#define INIC_API_DEVICE_VERSION_GET 0x20U

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Inic_HandleInternalErrors(void *self, void *error_code_ptr);
static void Inic_HandleApiTimeout(void *self, void *method_mask_ptr);
static void Inic_DecodeIcm(CInic *self, Msg_MostTel_t *msg_ptr);
static void Inic_MsgTxStatusCb(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of class CInic.
 *  \param self         Reference to CInic instance
 *  \param init_ptr     Reference to initialization data
 */
void Inic_Ctor(CInic *self, Inic_InitData_t *init_ptr)
{
    uint8_t i;
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->base_ptr        = init_ptr->base_ptr;
    self->xcvr_ptr        = init_ptr->xcvr_ptr;
    self->fkt_op_list_ptr = &inic_handler[0];
    self->target_address  = init_ptr->tgt_addr;

    /* create instances of single-observers */
    for(i=0U; i<INIC_NUM_SSUB; i++)
    {
        Ssub_Ctor(&self->ssubs[i], self->base_ptr->ucs_user_ptr);
    }

    /* create instances of "normal" observers */
    for(i=0U; i<INIC_NUM_SUB; i++)
    {
        Sub_Ctor(&self->subs[i], self->base_ptr->ucs_user_ptr);
    }

    /* Observe internal errors and events */
    Mobs_Ctor(&self->internal_error_obs, self, EH_M_TERMINATION_EVENTS, &Inic_HandleInternalErrors);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->internal_error_obs);

    /* Initialize API locking mechanism */
    Sobs_Ctor(&self->lock.observer, self, &Inic_HandleApiTimeout);
    Al_Ctor(&self->lock.api, &self->lock.observer, self->base_ptr->ucs_user_ptr);
    Alm_RegisterApi(&self->base_ptr->alm, &self->lock.api);

    /* Initialize Resource Management part */
    Inic_InitResourceManagement(self);
}

/*! \brief  Handles internal errors and events
 *  \param  self            Instance pointer
 *  \param  error_code_ptr  Reference to reported error code
 */
static void Inic_HandleInternalErrors(void *self, void *error_code_ptr)
{
    uint8_t i;
    Inic_StdResult_t res_data;
    CInic *self_ = (CInic *)self;
    MISC_UNUSED(error_code_ptr);

    res_data.data_info        = NULL;
    res_data.result.code      = UCS_RES_ERR_SYSTEM;
    res_data.result.info_ptr  = NULL;
    res_data.result.info_size = 0U;

    /* Internal error has been occurred => Cancel running jobs */
    for(i=0U; i<INIC_NUM_SSUB; i++)
    {
        Ssub_Notify(&self_->ssubs[i], &res_data, true);
    }
}

/*! \brief  Handles an API timeout
 *  \param  self             Instance pointer
 *  \param  method_mask_ptr  Bitmask to signal which API method has caused the timeout
 */
static void Inic_HandleApiTimeout(void *self, void *method_mask_ptr)
{
    CInic *self_ = (CInic *)self;
    Alm_ModuleMask_t method_mask = *((Alm_ModuleMask_t *)method_mask_ptr);
    Inic_StdResult_t res_data;

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_ERR_TIMEOUT;
    res_data.result.info_ptr = NULL;

    switch(method_mask)
    {
        case INIC_API_NW_SHUTDOWN:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SHUTDOWN], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC]", "API locking timeout occurred for method Inic_NwShutdown().", 0U));
            break;
        case INIC_API_NW_FRAME_COUNTER:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_FRAME_COUNTER], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC]", "API locking timeout occurred for method Inic_NwFrameCounter_Get().", 0U));
            break;
        case INIC_API_NW_TRIGGER_RBD:
            self_->lock.rbd_trigger_timeout_counter++;
            if(self_->lock.rbd_trigger_timeout_counter < 5U)
            {
                (void)Al_Lock(&self_->lock.api, INIC_API_NW_TRIGGER_RBD);
            }
            else
            {
                Inic_StdResult_t rbd_result_data;
                Ucs_StdResult_t result    = {UCS_RES_ERR_TIMEOUT, NULL, 0U};
                rbd_result_data.data_info = NULL;
                rbd_result_data.result    = result;
                Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_TRIGGER_RBD], &rbd_result_data, true);
                TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC]", "API locking timeout occurred for method Inic_NwTriggerRbd().", 0U));
            }
            break;
        case INIC_API_NW_RBD_RESULT:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_RBD_RESULT], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC]", "API locking timeout occurred for method Inic_NwRbdResult_Get().", 0U));
            break;
        default:
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC]", "Unknown API locking bitmask detected. Mask: 0x%02X", 1U, method_mask));
            break;
    }
}

/*! \brief  Decode an ICM message
 *  \param  self     Instance pointer to FBlock INIC 
 *  \param  msg_ptr  pointer to the ICM message to decode
 */
static void Inic_DecodeIcm(CInic *self, Msg_MostTel_t *msg_ptr)
{
    Dec_Return_t result;
    uint16_t     index;

    result = Dec_SearchFktOpIcm(self->fkt_op_list_ptr, &index, msg_ptr->id.function_id, msg_ptr->id.op_type);

    if (result == DEC_RET_SUCCESS)
    {
        self->fkt_op_list_ptr[index].handler_function_ptr(self, msg_ptr);
    }
    else
    {
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[INIC]", "Unknown ICM received. FBlockId: 0x%02X, InstId: 0x%02X, FktId: 0x%04X, OPType: 0x%02X", 4U, msg_ptr->id.fblock_id, msg_ptr->id.instance_id, msg_ptr->id.function_id, msg_ptr->id.op_type));
    }
}

/*! \brief Receives ICMs
 *  \param  self     reference to INIC object
 *  \param  tel_ptr  received message
 */
void Inic_OnIcmRx(void *self, Msg_MostTel_t *tel_ptr)
{
    CInic *self_ = (CInic *)self;

    if ((tel_ptr->source_addr == MSG_ADDR_INIC) && (tel_ptr->destination_addr == MSG_ADDR_EHC_CFG))
    {
        Inic_DecodeIcm(self_, tel_ptr);
    }

    Trcv_RxReleaseMsg(self_->xcvr_ptr, tel_ptr); /* free Rx telegram */
}

/*! \brief   Filters RCM Rx messages
 *  \details The filter function shall not release the message object
 *  \param   self     Reference to INIC object
 *  \param   tel_ptr  Reference to the RCM Rx message object
 */
void Inic_OnRcmRxFilter(void *self, Msg_MostTel_t *tel_ptr)
{ 
    uint16_t     index;
    CInic *self_ = (CInic *)self;

    if (Dec_SearchFktOpIcm(self_->fkt_op_list_ptr, &index, tel_ptr->id.function_id, tel_ptr->id.op_type) == DEC_RET_SUCCESS)
    {
        self_->fkt_op_list_ptr[index].handler_function_ptr(self, tel_ptr);
    }
}

/*! \brief Handle message Tx status and free message objects
 *  \param self     The instance
 *  \param tel_ptr  Reference to transmitted message
 *  \param status   Status of the transmitted message
 */
static void Inic_MsgTxStatusCb(void *self, Msg_MostTel_t *tel_ptr, Ucs_MsgTxStatus_t status)
{
    CInic *self_ = (CInic *)self;

    if ((status != UCS_MSG_STAT_OK) && (tel_ptr->info_ptr != NULL))
    {
        Inic_StdResult_t res_data;
        CSingleSubject *ssub_ptr = (CSingleSubject *)tel_ptr->info_ptr;

        res_data.data_info        = &status;
        res_data.result.code      = UCS_RES_ERR_TRANSMISSION;
        res_data.result.info_ptr  = NULL;
        res_data.result.info_size = 0U;
        Ssub_Notify(ssub_ptr, &res_data, true);
    }
    Trcv_TxReleaseMsg(tel_ptr);

    /* ICM messages pending? */
    if (Sub_GetNumObservers(&self_->subs[INIC_SUB_TX_MSG_OBJ_AVAIL]) > 0U)
    {
        Sub_Notify(&self_->subs[INIC_SUB_TX_MSG_OBJ_AVAIL], NULL);
    }
}

/*! \brief Add an observer to be notified when a tx message object is available
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be informed
 */
void Inic_AddObsrvOnTxMsgObjAvail(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_TX_MSG_OBJ_AVAIL], obs_ptr);
}

/*! \brief Delete an observer set by Inic_AddObsrvOnTxMsgObjAvail()
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be removed
 */
void Inic_DelObsrvOnTxMsgObjAvail(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_TX_MSG_OBJ_AVAIL], obs_ptr);
}

/*! \brief Add an observer to the NetworkStatus subject
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be informed
 */
void Inic_AddObsrvNwStatus(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_NW_STATUS], obs_ptr);
}

/*! \brief Delete an observer to the NetworkStatus subject
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be removed
 */
void Inic_DelObsrvNwStatus(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_NW_STATUS], obs_ptr);
}

/*! \brief Add an observer to the NetworkConfiguration subject
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be informed
 */
void Inic_AddObsvrNwConfig(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_NW_CONFIG], obs_ptr);
}

/*! \brief Delete an observer to the NetworkConfiguration subject
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be removed
 */
void Inic_DelObsvrNwConfig(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_NW_CONFIG], obs_ptr);
}

/*! \brief   Add an observer to the DeviceStatus subject
 *  \details The provided data points to a \ref Inic_DeviceStatus_t structure
 *  \param   self     instance of CInic
 *  \param   obs_ptr  pointer to observer to be informed
 */
void Inic_AddObsvrDeviceStatus(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_DEVICE_STATUS], obs_ptr);
}

/*! \brief  Delete an observer to the DeviceStatus subject
 *  \param  self     instance of CInic
 *  \param  obs_ptr  pointer to observer to be removed
 */
void Inic_DelObsvrDeviceStatus(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_DEVICE_STATUS], obs_ptr);
}

/*------------------------------------------------------------------------------------------------*/
/* Internal API                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  This method requests the INIC version info
 *  \param  self        Reference to CInic instance
 *  \param  obs_ptr     Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_DeviceVersion_Get(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, INIC_API_DEVICE_VERSION_GET) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_DEVICE_VERSION;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_DEVICE_VERSION];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_DEVICE_VERSION], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_DEVICE_VERSION_GET);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Attach EHC to the INIC
 *  \param  self            Reference to CInic instance
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_DeviceAttach(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t     result = UCS_RET_SUCCESS;
    Msg_MostTel_t   *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_DEVICE_ATTACH;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_DEVICE_ATTACH];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_DEVICE_ATTACH], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  Attaches the given PMS channel to the network
 *  \param  self                Reference to CInic instance
 *  \param  pmp_channel_handle  Port message channel resource handle
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_NwAttach(CInic *self,
                           uint16_t pmp_channel_handle,
                           CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_MOST_NW_ATTACH;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->tel.tel_data_ptr[0] = MISC_HB(pmp_channel_handle);
        msg_ptr->tel.tel_data_ptr[1] = MISC_LB(pmp_channel_handle);

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_ATTACH];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_ATTACH], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  Starts the System diagnosis
 *  \param  self                Reference to CInic instance
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_NwSysDiagnosis(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_MOST_NW_SYS_DIAGNOSIS;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_SYS_DIAGNOSIS];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_SYS_DIAGNOSIS], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  Stops the System diagnosis
 *  \param  self                Reference to CInic instance
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_NwSysDiagEnd(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_MOST_NW_SYS_DIAG_END;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_SYS_DIAGEND];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_SYS_DIAGEND], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief Starts the Backchannel Diagnosis Mode
 *
 *  \param *self         Reference to CInic instance
 *  \param *obs_ptr      Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_BCDiagnosis(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = MSG_ADDR_INIC;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_BACK_CHANNEL_DIAGNOSIS;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_BC_DIAGNOSIS];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_BC_DIAGNOSIS], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief Stops the Backchannel Diagnosis Mode
 *
 *  \param *self         Reference to CInic instance
 *  \param *obs_ptr      Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_BCDiagEnd(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = MSG_ADDR_INIC;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_BACK_CHANNEL_DIAG_END;
        msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

        msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_BC_DIAG_END];
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_BC_DIAG_END], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  Requests the INIC.MOSTNetworRBDResult.Status message
 *  \param  self        Reference to CInic instance
 *  \param  obs_ptr     Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Inic_NwRbdResult_Get(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, INIC_API_NW_RBD_RESULT) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MOST_NW_RBD_RESULT;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_RBD_RESULT];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_RBD_RESULT], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_NW_RBD_RESULT);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}


/*! \brief  This functions starts up the MOST network.
 *  \param  self             Reference to CInic instance
 *  \param  auto_forced_na   The delay time to shutdown the network after INIC has entered the 
 *                           protected mode.
 *  \param  packet_bandwidth The desired packed bandwidth
 *  \param  obs_ptr          Reference to an optional observer. The result must be casted into type
 *                           Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NwStartup(CInic *self, uint16_t auto_forced_na,
                            uint16_t packet_bandwidth, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (self->startup_locked == false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            self->startup_locked = true;

            msg_ptr->destination_addr    = self->target_address;
            msg_ptr->id.fblock_id        = FB_INIC;
            msg_ptr->id.instance_id      = 0U;
            msg_ptr->id.function_id      = INIC_FID_MOST_NW_STARTUP;
            msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(auto_forced_na);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(auto_forced_na);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(packet_bandwidth);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(packet_bandwidth);

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_STARTUP];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_STARTUP], obs_ptr);
        }
        else
        {
            self->startup_locked = false;
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function shuts down the entire MOST network.
 *  \param  self            Reference to CInic instance
 *  \param  obs_ptr         Reference to an optional observer. The result must be casted into type
 *                          Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NwShutdown(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, INIC_API_NW_SHUTDOWN) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MOST_NW_SHUTDOWN;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_SHUTDOWN];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_SHUTDOWN], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_NW_SHUTDOWN);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief This function triggers the Ring Break Diagnosis.
 *  \param self             Reference to CInic instance
 *  \param type             Specifies if the INIC starts the RBD as a TimingMaster or TimingSlave.
 *  \param obs_ptr          Reference to an optional observer. The result must be casted into type
 *                          Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NwTriggerRbd(CInic *self, Ucs_Diag_RbdType_t type, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, INIC_API_NW_TRIGGER_RBD) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            self->lock.rbd_trigger_timeout_counter = 0U;

            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MOST_NW_TRIGGER_RBD;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = (uint8_t)type;

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_TRIGGER_RBD];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_TRIGGER_RBD], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_NW_TRIGGER_RBD);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief This function triggers the INIC to force the NotAvailable state
 *  \param self             Reference to CInic instance
 *  \param force            Is \c true if the INIC shall force the network in NotAvailable state. 
 *                          If \c false the INIC shall no no longer force the network to NotAvailable state.
 *  \param obs_ptr          Reference to an optional observer. The result must be casted into type
 *                          Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NwForceNotAvailable(CInic *self, bool force, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.api, INIC_API_NW_FORCE_NA) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;
            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MOST_NW_FORCE_NO_AVAIL;
            msg_ptr->id.op_type     = UCS_OP_SETGET;

            if (force == false)
            {
                msg_ptr->tel.tel_data_ptr[0] = 0U;
            }
            else
            {
                msg_ptr->tel.tel_data_ptr[0] = 1U;
            }

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_FORCE_NA];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_FORCE_NA], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_NW_FORCE_NA);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function modifies the INIC network configuration.
 *  \param  self    Reference to CInic instance
 *  \param  mask    Allows to change a single, multiple, or all parameters
 *  \param  config  Holds the parameter values
 *  \param  obs_ptr Reference to an optional observer. The result must be casted into type
 *                  Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 */
Ucs_Return_t Inic_NwConfig_SetGet(CInic *self, uint16_t mask, Inic_NetworkConfig_t config,
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 24U);

    if (msg_ptr != NULL)
    {
        mask = mask & 7U;       /* allow only bit 0..2 */
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_MOST_NW_CFG;
        msg_ptr->id.op_type     = UCS_OP_SETGET;

        msg_ptr->tel.tel_data_ptr[0]  = MISC_HB(mask);
        msg_ptr->tel.tel_data_ptr[1]  = MISC_LB(mask);
        msg_ptr->tel.tel_data_ptr[2]  = MISC_HB(config.node_address);
        msg_ptr->tel.tel_data_ptr[3]  = MISC_LB(config.node_address);
        msg_ptr->tel.tel_data_ptr[4]  = MISC_HB(config.group_address);
        msg_ptr->tel.tel_data_ptr[5]  = MISC_LB(config.group_address);
        msg_ptr->tel.tel_data_ptr[6]  = config.llrbc;
        MISC_MEM_SET(&msg_ptr->tel.tel_data_ptr[7], 0, 17U);

        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_CONFIG], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  Requests the INIC.NetworkConfiguration.Status message
 *  \param  self        Reference to CInic instance
 *  \param  obs_ptr     Reference to an optional observer. The result must be casted into type
 *                      Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_NwConfig_Get(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = self->target_address;

        msg_ptr->id.fblock_id   = FB_INIC;
        msg_ptr->id.instance_id = 0U;
        msg_ptr->id.function_id = INIC_FID_MOST_NW_CFG;
        msg_ptr->id.op_type     = UCS_OP_GET;

        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_CONFIG], obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  Requests the INIC.MOSTNetworkFrameCounter.Status message
 *  \param  self        Reference to CInic instance
 *  \param  reference   Reference counter value 
 *  \param  obs_ptr     Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NwFrameCounter_Get(CInic *self, uint32_t reference, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, INIC_API_NW_FRAME_COUNTER) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MOST_NW_FRAME_COUNTER;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->tel.tel_data_ptr[0]  = (uint8_t)(reference >> 24);
            msg_ptr->tel.tel_data_ptr[1]  = (uint8_t)(reference >> 16);
            msg_ptr->tel.tel_data_ptr[2]  = (uint8_t)(reference >>  8);
            msg_ptr->tel.tel_data_ptr[3]  = (uint8_t)reference;

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NW_FRAME_COUNTER];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_MsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NW_FRAME_COUNTER], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, INIC_API_NW_FRAME_COUNTER);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}





/*------------------------------------------------------------------------------------------------*/
/* Handler functions                                                                              */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Dummy handler function for unused INIC functions
 *
 * \param  self     instance of CInic
 * \param msg_ptr   Pointer to received message
 */
void Inic_DummyHandler(void *self, Msg_MostTel_t *msg_ptr)
{
    MISC_UNUSED(self);
    MISC_UNUSED(msg_ptr);
}

/*! \brief Handler function for INIC.DeviceStatus.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_DeviceStatus_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;

    if (msg_ptr->tel.tel_len > 0U)
    {
        TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[INIC]", (msg_ptr->tel.tel_len == 5U));
        self_->device_status.config_iface_state= (Inic_AttachState_t)msg_ptr->tel.tel_data_ptr[0];
        self_->device_status.app_iface_state   = (Inic_AttachState_t)msg_ptr->tel.tel_data_ptr[1];
        self_->device_status.power_state       = (Ucs_Inic_PowerState_t)msg_ptr->tel.tel_data_ptr[2];
        self_->device_status.bist              = (Inic_Bist_t)msg_ptr->tel.tel_data_ptr[3];
        self_->device_status.last_reset_reason = (Ucs_Inic_LastResetReason_t)msg_ptr->tel.tel_data_ptr[4];

        /* INIC BIST error detected */
        if (self_->device_status.bist == INIC_BIST_ERROR)
        {
            Eh_ReportEvent(&self_->base_ptr->eh, EH_E_BIST_FAILED);
        }

        Sub_Notify(&self_->subs[INIC_SUB_DEVICE_STATUS], &self_->device_status);
    }
}

/*! \brief Handler function for INIC.DeviceVersion.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_DeviceVersion_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        MISC_DECODE_DWORD(&(self_->device_version.product_identifier), &(msg_ptr->tel.tel_data_ptr[0]));
        self_->device_version.major_version = msg_ptr->tel.tel_data_ptr[4];
        self_->device_version.minor_version = msg_ptr->tel.tel_data_ptr[5];
        self_->device_version.release_version = msg_ptr->tel.tel_data_ptr[6];
        MISC_DECODE_DWORD(&(self_->device_version.build_version), &(msg_ptr->tel.tel_data_ptr[7]));
        self_->device_version.hw_revision = msg_ptr->tel.tel_data_ptr[11];
        MISC_DECODE_WORD(&(self_->device_version.diagnosis_id), &(msg_ptr->tel.tel_data_ptr[12]));

        TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[INIC]", (msg_ptr->tel.tel_data_ptr[14] == 0x01U)); /* ExtIdentifier == CFGS ? */

        self_->device_version.cs_major_version = msg_ptr->tel.tel_data_ptr[15];
        self_->device_version.cs_minor_version = msg_ptr->tel.tel_data_ptr[16];
        self_->device_version.cs_release_version = msg_ptr->tel.tel_data_ptr[17];


        res_data.data_info       = &self_->device_version;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;


        Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_VERSION], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_DEVICE_VERSION_GET);
}

/*! \brief Handler function for INIC.DeviceVersion.Error
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_DeviceVersion_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              msg_ptr->tel.tel_len);

        Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_VERSION], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_DEVICE_VERSION_GET);
}

/*! \brief Handler function for INIC.NetworkStatus.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwStatus_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info       = &self_->network_status;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[INIC]", (msg_ptr->tel.tel_len == 11U));
        MISC_DECODE_WORD(&(self_->network_status.events), &(msg_ptr->tel.tel_data_ptr[0]));
        self_->network_status.availability      = (Ucs_Network_Availability_t)msg_ptr->tel.tel_data_ptr[2];
        self_->network_status.avail_info        = (Ucs_Network_AvailInfo_t)msg_ptr->tel.tel_data_ptr[3];
        self_->network_status.avail_trans_cause = (Ucs_Network_AvailTransCause_t)msg_ptr->tel.tel_data_ptr[4];
        MISC_DECODE_WORD(&(self_->network_status.node_address), &(msg_ptr->tel.tel_data_ptr[5]));
        self_->network_status.node_position     = msg_ptr->tel.tel_data_ptr[7];
        self_->network_status.max_position      = msg_ptr->tel.tel_data_ptr[8];
        MISC_DECODE_WORD(&(self_->network_status.packet_bw), &(msg_ptr->tel.tel_data_ptr[9]));

        Sub_Notify(&self_->subs[INIC_SUB_NW_STATUS], &res_data);
    }
}

/*! \brief Handler function for INIC.NetworkConfiguration.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwConfig_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 4U)
    {
        res_data.data_info       = &self_->network_config;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        MISC_DECODE_WORD(&(self_->network_config.node_address), &(msg_ptr->tel.tel_data_ptr[0]));
        MISC_DECODE_WORD(&(self_->network_config.group_address), &(msg_ptr->tel.tel_data_ptr[2]));
        self_->network_config.llrbc   = msg_ptr->tel.tel_data_ptr[4];

        Sub_Notify(&self_->subs[INIC_SUB_NW_CONFIG], &res_data);
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_CONFIG], &res_data, true);
    }
}

/*! \brief Handler function for INIC.NetworkConfiguration.Error
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwConfig_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              msg_ptr->tel.tel_len);

        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_CONFIG], &res_data, true);
    }
}

/*! \brief Handler function for INIC.MOSTNetworkFrameCounter.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwFrameCounter_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;
    Inic_FrameCounterStatus_t frame_counter_status; 

    if (msg_ptr->tel.tel_len > 0U)
    {
        MISC_DECODE_DWORD(&frame_counter_status.reference, &(msg_ptr->tel.tel_data_ptr[0]));
        MISC_DECODE_DWORD(&frame_counter_status.frame_counter, &(msg_ptr->tel.tel_data_ptr[4]));
        frame_counter_status.lock = msg_ptr->tel.tel_data_ptr[8];
        res_data.data_info        = &frame_counter_status;
        res_data.result.code      = UCS_RES_SUCCESS;
        res_data.result.info_ptr  = NULL;

        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_FRAME_COUNTER], &res_data, true);    /* provides pointer to Inic_StdResult_t structure */
    }
    Al_Release(&self_->lock.api, INIC_SSUB_NW_FRAME_COUNTER);
}

/*! \brief Handler function for INIC.MOSTNetworkFrameCounter.Error
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwFrameCounter_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              msg_ptr->tel.tel_len);

        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_FRAME_COUNTER], &res_data, true);    /* provides pointer to Inic_StdResult_t structure */
    }
    Al_Release(&self_->lock.api, INIC_SSUB_NW_FRAME_COUNTER);
}

/*! \brief Handler function for INIC.MOSTNetworkStartup.ErrorAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwStartup_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    self_->startup_locked = false;
    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0], 
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_STARTUP], &res_data, true);
}

/*! \brief Handler function for INIC.MOSTNetworkStartup.ResultAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwStartup_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;
    MISC_UNUSED(msg_ptr);

    self_->startup_locked = false;
    res_data.data_info = NULL;
    res_data.result.code = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_STARTUP], &res_data, true);
}

/*! \brief Handler function for INIC.MOSTNetworkShutdown.ErrorAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwShutdown_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SHUTDOWN], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_NW_SHUTDOWN);
}

/*! \brief Handler function for INIC.MOSTNetworkShutdown.ResultAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwShutdown_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;
    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SHUTDOWN], &res_data, true);
    Al_Release(&self_->lock.api, INIC_API_NW_SHUTDOWN);
}

/*! \brief Handler function for INIC.MOSTNetworkTriggerRBD.ErrorAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwTriggerRbd_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_TRIGGER_RBD], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_NW_TRIGGER_RBD);
}

/*! \brief Handler function for INIC.MOSTNetworkTriggerRBD.ResultAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwTriggerRbd_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;
    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_TRIGGER_RBD], &res_data, true);
    Al_Release(&self_->lock.api, INIC_API_NW_TRIGGER_RBD);
}

/*! \brief Handler function for INIC.MOSTNetworkForceNotAvailable.ErrorAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwForceNotAvailable_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_FORCE_NA], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_NW_FORCE_NA);
}

/*! \brief Handler function for INIC.MOSTNetworkForceNotAvailable.ResultAck
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NwForceNotAvailable_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;
    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_FORCE_NA], &res_data, true);
    Al_Release(&self_->lock.api, INIC_API_NW_FORCE_NA);
}

/*! \brief Handler function for INIC.DeviceAttach.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_DeviceAttach_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_ATTACH], &res_data, true);
    }
}

/*! \brief Handler function for INIC.DeviceAttach.Result
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_DeviceAttach_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_ATTACH], &res_data, true);
}

/*! \brief Handler function for INIC.NetworkAttach.ErrorAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwAttach_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_ATTACH], &res_data, true);
    }
}

/*! \brief Handler function for INIC.NetworkAttach.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwAttach_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_ATTACH], &res_data, true);
}




/*! \brief Handler function for INIC.MOSTNetworkSystemDiagnosis.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwSysDiagnosis_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SYS_DIAGNOSIS], &res_data, true);
    }
}

/*! \brief Handler function for INIC.MOSTNetworkSystemDiagnosis.Result
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwSysDiagnosis_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SYS_DIAGNOSIS], &res_data, true);
}

/*! \brief Handler function for INIC.MOSTNetworkSystemDiagnosisEnd.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwSysDiagEnd_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              (msg_ptr->tel.tel_len));
        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SYS_DIAGEND], &res_data, true);
    }
}

/*! \brief Handler function for INIC.MOSTNetworkSystemDiagnosisEnd.Result
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwSysDiagEnd_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_SYS_DIAGEND], &res_data, true);
}



/*! \brief Handler function for INIC.BCDiag.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_BCDiagnosis_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0], 
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_BC_DIAGNOSIS], &res_data, true);
}

/*! \brief Handler function for INIC.BCDiag.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_BCDiagnosis_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_BC_DIAGNOSIS], &res_data, true);
}

/*! \brief Handler function for INIC.BCDiagEnd.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_BCDiagEnd_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0], 
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_BC_DIAG_END], &res_data, true);
}

/*! \brief Handler function for INIC.BCDiagEnd.Result
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_BCDiagEnd_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_BC_DIAG_END], &res_data, true);
}





/*! \brief Handler function for INIC.MOSTNetworkRBDResult.Status
 *  \param  self        Reference to INIC object
 *  \param  msg_ptr     Received message
 */
void Inic_NwRbdResult_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_RbdResult_t rbd_result_data;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        rbd_result_data.result   = (Ucs_Diag_RbdResult_t)msg_ptr->tel.tel_data_ptr[0];
        rbd_result_data.position = msg_ptr->tel.tel_data_ptr[1];
        rbd_result_data.status   = msg_ptr->tel.tel_data_ptr[2];
        MISC_DECODE_WORD(&(rbd_result_data.diag_id), &(msg_ptr->tel.tel_data_ptr[3]));
        res_data.data_info       = &rbd_result_data;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_RBD_RESULT], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_NW_RBD_RESULT);
}

/*! \brief Handler function for INIC.MOSTNetworkRBDResult.Error
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_NwRbdResult_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Inic_TranslateError(self_,
                                              &msg_ptr->tel.tel_data_ptr[0], 
                                              msg_ptr->tel.tel_len);

        Ssub_Notify(&self_->ssubs[INIC_SSUB_NW_RBD_RESULT], &res_data, true);
    }
    Al_Release(&self_->lock.api, INIC_API_NW_RBD_RESULT);
}




/*------------------------------------------------------------------------------------------------*/
/* Helper functions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Translates INIC error codes into UNICENS error codes and wraps the raw INIC
 *         error data to a byte stream.
 *  \param self         Instance of CInic
 *  \param error_data[] INIC error data
 *  \param error_size   Size of INIC error data in bytes
 *  \return             The formatted error
 */
Ucs_StdResult_t Inic_TranslateError(CInic *self, uint8_t error_data[], uint8_t error_size)
{
    Ucs_StdResult_t ret_val;
    MISC_UNUSED(self);

    if(error_data[0] != 0x20U)
    {
        ret_val.code = UCS_RES_ERR_MOST_STANDARD;
    }
    else
    {
        ret_val.code = (Ucs_Result_t)(error_data[1] + 1U);
    }

    ret_val.info_ptr  = &error_data[0];
    ret_val.info_size = error_size;

    return ret_val;
}


/*------------------------------------------------------------------------------------------------*/
/* Synchronous Getters                                                                            */
/*------------------------------------------------------------------------------------------------*/
uint16_t Inic_GetGroupAddress(CInic *self)
{
    return self->network_config.group_address;
}

uint16_t Inic_GetPacketDataBandwidth(CInic *self)
{
    return self->network_status.packet_bw;
}

uint16_t Inic_GetNodeAddress(CInic *self)
{
    return self->network_status.node_address;
}

uint8_t Inic_GetNodePosition(CInic *self)
{
    return self->network_status.node_position;
}

uint8_t Inic_GetNumberOfNodes(CInic *self)
{
    return self->network_status.max_position;
}

uint8_t Inic_GetInicLlrbc(CInic *self)
{
    return self->network_config.llrbc;
}

Ucs_Inic_Version_t Inic_GetDeviceVersion(CInic *self)
{
    return self->device_version;
}

Ucs_Inic_LastResetReason_t Inic_GetLastResetReason(CInic *self)
{
    return self->device_status.last_reset_reason;
}

Ucs_Inic_PowerState_t Inic_GetDevicePowerState(CInic *self)
{
    return self->device_status.power_state;
}

Ucs_Network_Availability_t Inic_GetAvailability(CInic *self)
{
    return self->network_status.availability;
}

uint16_t Inic_GetTargetAddress (CInic *self)
{
    return self->target_address;
}

/*!
 * @}
 * \endcond
 */
/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

