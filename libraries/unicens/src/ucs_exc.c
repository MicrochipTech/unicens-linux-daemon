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
 * \brief   Implementation of FBlock ExtendedNetworkControl
 * \details Contains the housekeeping functions of INIC management
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EXC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"
#include "ucs_ret_pb.h"
#include "ucs_exc.h"



/*------------------------------------------------------------------------------------------------*/
/* Internal definitions                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Bitmask for API method Exc_PhyTestResult_Get() used by API locking manager */
#define EXC_API_PHY_LAY_TEST_RESULT     0x01U
/*! \brief Bitmask for API method Exc_MemSessionOpen_Sr() used by API locking manager */
#define EXC_API_MEM_SESSION_OPEN        0x02U
/*! \brief Bitmask for API method Exc_MemSessionClose_Sr() used by API locking manager */
#define EXC_API_MEM_SESSION_CLOSE       0x04U
/*! \brief Bitmask for API method Exc_MemoryRead_Sr() used by API locking manager */
#define EXC_API_MEM_READ                0x08U
/*! \brief Bitmask for API method Exc_MemoryWrite_Sr() used by API locking manager */
#define EXC_API_MEM_WRITE               0x10U

/*! \brief max. number of elements used in MemoryWrite and MemoryWrite messages */
#define MAX_UNIT_LEN                    18U

/*! \brief  length of signature (V1) */
#define EXC_SIGNATURE_LEN_V1            26U


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Exc_DecodeMsg(CExc *self, Msg_MostTel_t *msg_rx_ptr);
static void Exc_EnablePort_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_EnablePort_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Hello_Status(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Hello_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Welcome_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Welcome_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Signature_Status(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_Signature_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_DeviceInit_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_CableLinkDiag_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_CableLinkDiag_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_NwPhyTest_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_NwPhyTestResult_Status(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_NwPhyTestResult_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_BC_Diag_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_BC_Diag_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_BC_EnableTx_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_BC_EnableTx_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemoryRead_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemoryRead_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemoryWrite_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemoryWrite_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemSessionOpen_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemSessionOpen_Error(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemSessionClose_Result(void *self, Msg_MostTel_t *msg_ptr);
static void Exc_MemSessionClose_Error(void *self, Msg_MostTel_t *msg_ptr);

static void Exc_HandleApiTimeout(void *self, void *method_mask_ptr);

static Ucs_StdResult_t Exc_TranslateError(CExc *self, uint8_t error_data[], uint8_t error_size);
static void Exc_Read_Signature(Ucs_Signature_t *dest, uint8_t source[]);


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief List of all EXC messages */
static const Dec_FktOpIsh_t exc_handler[] =       /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { DEC_FKTOP(EXC_FID_HELLO,                  UCS_OP_STATUS),  Exc_Hello_Status },
    { DEC_FKTOP(EXC_FID_HELLO,                  UCS_OP_ERROR),   Exc_Hello_Error },
    { DEC_FKTOP(EXC_FID_WELCOME,                UCS_OP_RESULT),  Exc_Welcome_Result },
    { DEC_FKTOP(EXC_FID_WELCOME,                UCS_OP_ERROR),   Exc_Welcome_Error },
    { DEC_FKTOP(EXC_FID_SIGNATURE,              UCS_OP_STATUS),  Exc_Signature_Status },
    { DEC_FKTOP(EXC_FID_SIGNATURE,              UCS_OP_ERROR),   Exc_Signature_Error },
    { DEC_FKTOP(EXC_FID_DEVICE_INIT,            UCS_OP_ERROR),   Exc_DeviceInit_Error },
    { DEC_FKTOP(EXC_FID_ENABLEPORT,             UCS_OP_RESULT),  Exc_EnablePort_Result },
    { DEC_FKTOP(EXC_FID_ENABLEPORT,             UCS_OP_ERROR),   Exc_EnablePort_Error },
    { DEC_FKTOP(EXC_FID_CABLE_LINK_DIAG,        UCS_OP_RESULT),  Exc_CableLinkDiag_Result },
    { DEC_FKTOP(EXC_FID_CABLE_LINK_DIAG,        UCS_OP_ERROR),   Exc_CableLinkDiag_Error },
    { DEC_FKTOP(EXC_FID_PHY_LAY_TEST,           UCS_OP_ERROR),   Exc_NwPhyTest_Error },
    { DEC_FKTOP(EXC_FID_PHY_LAY_TEST_RES,       UCS_OP_STATUS),  Exc_NwPhyTestResult_Status },
    { DEC_FKTOP(EXC_FID_PHY_LAY_TEST_RES,       UCS_OP_ERROR),   Exc_NwPhyTestResult_Error },
    { DEC_FKTOP(EXC_FID_BC_DIAG,                UCS_OP_RESULT),  Exc_BC_Diag_Result },
    { DEC_FKTOP(EXC_FID_BC_DIAG,                UCS_OP_ERROR),   Exc_BC_Diag_Error },
    { DEC_FKTOP(EXC_FID_BC_ENABLE_TX,           UCS_OP_RESULT),  Exc_BC_EnableTx_Result },
    { DEC_FKTOP(EXC_FID_BC_ENABLE_TX,           UCS_OP_ERROR),   Exc_BC_EnableTx_Error },
    { DEC_FKTOP(EXC_FID_MEM_SESSION_OPEN,       UCS_OP_RESULT),  Exc_MemSessionOpen_Result },
    { DEC_FKTOP(EXC_FID_MEM_SESSION_OPEN,       UCS_OP_ERROR),   Exc_MemSessionOpen_Error },
    { DEC_FKTOP(EXC_FID_MEM_SESSION_CLOSE,      UCS_OP_RESULT),  Exc_MemSessionClose_Result },
    { DEC_FKTOP(EXC_FID_MEM_SESSION_CLOSE,      UCS_OP_ERROR),   Exc_MemSessionClose_Error },
    { DEC_FKTOP(EXC_FID_MEMORY_READ,            UCS_OP_RESULT),  Exc_MemoryRead_Result },
    { DEC_FKTOP(EXC_FID_MEMORY_READ,            UCS_OP_ERROR),   Exc_MemoryRead_Error },
    { DEC_FKTOP(EXC_FID_MEMORY_WRITE,           UCS_OP_RESULT),  Exc_MemoryWrite_Result },
    { DEC_FKTOP(EXC_FID_MEMORY_WRITE,           UCS_OP_ERROR),   Exc_MemoryWrite_Error },
    { DEC_FKTOP_TERMINATION,                                     NULL }
};


/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of class CExc.
 *  \param self         Reference to CExc instance
 *  \param base_ptr     Reference to a Base instance
 *  \param rcm_ptr      Reference to Transceiver instance
 */
void Exc_Ctor(CExc *self, CBase *base_ptr, CTransceiver *rcm_ptr)
{

    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->base_ptr = base_ptr;
    self->xcvr_ptr = rcm_ptr;

    self->fkt_op_list_ptr = &exc_handler[0];


    /* Initialize API locking mechanism */
    Sobs_Ctor(&self->lock.observer, self, &Exc_HandleApiTimeout);
    Al_Ctor(&self->lock.api, &self->lock.observer, self->base_ptr->ucs_user_ptr);
    Alm_RegisterApi(&self->base_ptr->alm, &self->lock.api);

}


/*! \brief   Callback function to filter RCM Rx messages
 *  \details Do not release the message object here
 *  \param   self     reference to INIC object
 *  \param   tel_ptr  received message
 */
void Exc_OnRcmRxFilter(void *self, Msg_MostTel_t *tel_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_DecodeMsg(self_, tel_ptr);

}


/*! \brief  Decode a message for FBlock EXC
 *  \param  self        Instance pointer to FBlock EXC
 *  \param  msg_rx_ptr  pointer to the MCM message to decode
 */
static void Exc_DecodeMsg(CExc *self, Msg_MostTel_t *msg_rx_ptr)
{
    Dec_Return_t result;
    uint16_t     index;

    result = Dec_SearchFktOpIsh(self->fkt_op_list_ptr, &index, msg_rx_ptr->id.function_id, msg_rx_ptr->id.op_type);

    if (result == DEC_RET_SUCCESS)
    {
        self->fkt_op_list_ptr[index].handler_function_ptr(self, msg_rx_ptr);
    }
    else
    {
        /* no handling of decoding error for shadow OpTypes */
    }
}



/*! \brief  Handles an API timeout
 *  \param  self             Instance pointer
 *  \param  method_mask_ptr  Bitmask to signal which API method has caused the timeout
 */
static void Exc_HandleApiTimeout(void *self, void *method_mask_ptr)
{
    CExc *self_ = (CExc *)self;
    Alm_ModuleMask_t method_mask = *((Alm_ModuleMask_t *)method_mask_ptr);
    Exc_StdResult_t res_data;

    res_data.result.code      = UCS_RES_ERR_TIMEOUT;
    res_data.result.info_ptr  = NULL;
    res_data.result.info_size = 0U;
    res_data.data_info        = NULL;

    switch(method_mask)
    {
#if 0   /* System Diagnosis supervises timeouts for these functions  */
        case EXC_API_ENABLE_PORT:
            Ssub_Notify(&self_->ssubs.enableport, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_EnablePort_Sr().", 0U));
            break;
        case EXC_API_HELLO:
            Ssub_Notify(&self_->ssubs.hello, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_Hello_Get().", 0U));
            break;
        case EXC_API_WELCOME:
            Ssub_Notify(&self_->ssubs.welcome, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_Welcome_Sr().", 0U));
            break;
        case EXC_API_CABLE_LINK_DIAG:
            Ssub_Notify(&self_->ssubs.cablelinkdiag, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_CableLinkDiagnosis_Start().", 0U));
            break;
#endif
        case EXC_API_PHY_LAY_TEST_RESULT:
            Ssub_Notify(&self_->ssubs.phylaytestresult, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_PhyTestResult_Get().", 0U));
            break;
        case EXC_API_MEM_SESSION_OPEN:
            Ssub_Notify(&self_->ssubs.memsessionopen, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_MemSessionOpen_Sr().", 0U));
            break;
        case EXC_API_MEM_SESSION_CLOSE:
            Ssub_Notify(&self_->ssubs.memsessionclose, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_MemSessionClose_Sr().", 0U));
            break;
        case EXC_API_MEM_READ:
            Ssub_Notify(&self_->ssubs.memoryread, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_MemoryRead_Sr().", 0U));
            break;
        case EXC_API_MEM_WRITE:
            Ssub_Notify(&self_->ssubs.memorywrite, &res_data, false);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "API locking timeout occurred for method Exc_MemoryWrite_Sr().", 0U));
            break;

        default:
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[EXC]", "Unknown API locking bitmask detected. Mask: 0x%02X", 1U, method_mask));
            break;
    }
}




/*------------------------------------------------------------------------------------------------*/
/* Internal API                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  This method sends the Hello.Get message
 *  \param  self            Reference to CExc instance
 *  \param  target_address  Target address
 *  \param  version_limit   Signature version limit 
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_Hello_Get(CExc *self, 
                           uint16_t target_address, 
                           uint8_t version_limit,
                           CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

    if (msg_ptr != NULL)
    {
        if (version_limit > UCS_EXC_SIGNATURE_VERSION_LIMIT)
        {
            version_limit = UCS_EXC_SIGNATURE_VERSION_LIMIT;
        }

        msg_ptr->destination_addr    = target_address;

        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_HELLO;
        msg_ptr->id.op_type          = UCS_OP_GET;
        msg_ptr->tel.tel_data_ptr[0] = version_limit;

        msg_ptr->info_ptr = &self->ssubs.hello;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.hello, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  This method send the Welcome.StartResult message
 *  \param  self                Reference to CExc instance
 *  \param  target_address      Target address
 *  \param  admin_node_address  The node address used during system diagnosis
 *  \param  version             Signature version 
 *  \param  signature           Signature of the device
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_Welcome_Sr(CExc *self, 
                            uint16_t target_address, 
                            uint16_t admin_node_address,
                            uint8_t version,
                            Ucs_Signature_t  signature,
                            CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, EXC_SIGNATURE_LEN_V1 + 3U);    /* Signature v1 */

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr     = target_address;
        msg_ptr->id.fblock_id         = FB_EXC;
        msg_ptr->id.instance_id       = 0U;
        msg_ptr->id.function_id       = EXC_FID_WELCOME;
        msg_ptr->id.op_type           = UCS_OP_STARTRESULT;
        msg_ptr->tel.tel_data_ptr[0]  = MISC_HB(admin_node_address);
        msg_ptr->tel.tel_data_ptr[1]  = MISC_LB(admin_node_address);
        msg_ptr->tel.tel_data_ptr[2]  = version;
        msg_ptr->tel.tel_data_ptr[3]  = MISC_HB(signature.node_address);
        msg_ptr->tel.tel_data_ptr[4]  = MISC_LB(signature.node_address);
        msg_ptr->tel.tel_data_ptr[5]  = MISC_HB(signature.group_address);
        msg_ptr->tel.tel_data_ptr[6]  = MISC_LB(signature.group_address);
        msg_ptr->tel.tel_data_ptr[7]  = MISC_HB(signature.mac_47_32);
        msg_ptr->tel.tel_data_ptr[8]  = MISC_LB(signature.mac_47_32);
        msg_ptr->tel.tel_data_ptr[9]  = MISC_HB(signature.mac_31_16);
        msg_ptr->tel.tel_data_ptr[10] = MISC_LB(signature.mac_31_16);
        msg_ptr->tel.tel_data_ptr[11] = MISC_HB(signature.mac_15_0);
        msg_ptr->tel.tel_data_ptr[12] = MISC_LB(signature.mac_15_0);
        msg_ptr->tel.tel_data_ptr[13] = MISC_HB(signature.node_pos_addr);
        msg_ptr->tel.tel_data_ptr[14] = MISC_LB(signature.node_pos_addr);
        msg_ptr->tel.tel_data_ptr[15] = MISC_HB(signature.diagnosis_id);
        msg_ptr->tel.tel_data_ptr[16] = MISC_LB(signature.diagnosis_id);
        msg_ptr->tel.tel_data_ptr[17] = signature.num_ports;
        msg_ptr->tel.tel_data_ptr[18] = signature.chip_id;
        msg_ptr->tel.tel_data_ptr[19] = signature.fw_major;
        msg_ptr->tel.tel_data_ptr[20] = signature.fw_minor;
        msg_ptr->tel.tel_data_ptr[21] = signature.fw_release;
        msg_ptr->tel.tel_data_ptr[22] = MISC_HB((signature.fw_build) >>16U);
        msg_ptr->tel.tel_data_ptr[23] = MISC_LB((signature.fw_build) >>16U);
        msg_ptr->tel.tel_data_ptr[24] = MISC_HB(signature.fw_build);
        msg_ptr->tel.tel_data_ptr[25] = MISC_LB(signature.fw_build);
        msg_ptr->tel.tel_data_ptr[26] = signature.cs_major;
        msg_ptr->tel.tel_data_ptr[27] = signature.cs_minor;
        msg_ptr->tel.tel_data_ptr[28] = signature.cs_release;
/*        msg_ptr->tel.tel_data_ptr[29] = signature.uid_persistency;
        msg_ptr->tel.tel_data_ptr[30] = MISC_HB((signature.uid) >>16U);
        msg_ptr->tel.tel_data_ptr[31] = MISC_LB((signature.uid) >>16U);
        msg_ptr->tel.tel_data_ptr[32] = MISC_HB(signature.uid);
        msg_ptr->tel.tel_data_ptr[33] = MISC_LB(signature.uid);
*/

        msg_ptr->info_ptr = &self->ssubs.welcome;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.welcome, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  This method sends the Signature.Get message
 *  \param  self            Reference to CExc instance
 *  \param  target_address  Target address
 *  \param  version_limit   Signature version limit 
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_Signature_Get(CExc *self, 
                               uint16_t target_address, 
                               uint8_t version_limit, 
                               CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

    if (msg_ptr != NULL)
    {
        if (version_limit > UCS_EXC_SIGNATURE_VERSION_LIMIT)
        {
            version_limit = UCS_EXC_SIGNATURE_VERSION_LIMIT;
        }

        msg_ptr->destination_addr  = target_address;

        msg_ptr->id.fblock_id      = FB_EXC;
        msg_ptr->id.instance_id    = 0U;
        msg_ptr->id.function_id    = EXC_FID_SIGNATURE;
        msg_ptr->id.op_type        = UCS_OP_GET;
        msg_ptr->tel.tel_data_ptr[0] = version_limit;

        msg_ptr->info_ptr = &self->ssubs.signature;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.signature, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  This method sends the DeviceInit.Start message
 *  \param  self            Reference to CExc instance
 *  \param  target_address  Target address
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_DeviceInit_Start(CExc *self, 
                                  uint16_t target_address, 
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr  = target_address;

        msg_ptr->id.fblock_id      = FB_EXC;
        msg_ptr->id.instance_id    = 0U;
        msg_ptr->id.function_id    = EXC_FID_DEVICE_INIT;
        msg_ptr->id.op_type        = UCS_OP_START;

        msg_ptr->info_ptr = &self->ssubs.deviceinit;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.deviceinit, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  This method enables a port
 *  \param  self            Reference to CExc instance
 *  \param  target_address  Target address
 *  \param  port_number     PortNumber 
 *  \param  enabled         Enabled 
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_EnablePort_Sr(CExc *self, 
                               uint16_t target_address, 
                               uint8_t port_number, 
                               bool enabled, 
                               CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U); 

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = target_address;

        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_ENABLEPORT;
        msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
        msg_ptr->tel.tel_data_ptr[0] = port_number;
        msg_ptr->tel.tel_data_ptr[1] = (uint8_t)enabled;

        msg_ptr->info_ptr = &self->ssubs.enableport;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.enableport, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  This method starts the Cable Link Diagnosis
 *  \param  self            Reference to CExc instance
 *  \param  target_address  Target address
 *  \param  port_number     PortNumber 
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_CableLinkDiagnosis_Start(CExc *self, 
                                          uint16_t target_address, 
                                          uint8_t port_number, 
                                          CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = target_address;

        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_CABLE_LINK_DIAG;
        msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
        msg_ptr->tel.tel_data_ptr[0] = port_number;

        msg_ptr->info_ptr = &self->ssubs.cablelinkdiag;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.cablelinkdiag, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  This method starts the Physical Layer Test
 *  \param  self            Reference to CExc instance
 *  \param  port_number     PortNumber 
 *  \param  type            Type
 *  \param  lead_in         Lead-in
 *  \param  duration        Duration
 *  \param  lead_out        Lead-out
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_PhyTest_Start(CExc *self, 
                               uint8_t port_number, 
                               Ucs_Diag_PhyTest_Type_t type, 
                               uint16_t lead_in, 
                               uint32_t duration, 
                               uint16_t lead_out, 
                               CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 10U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = MSG_ADDR_INIC; 

        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_PHY_LAY_TEST;
        msg_ptr->id.op_type          = UCS_OP_START;
        msg_ptr->tel.tel_data_ptr[0] = port_number; 
        msg_ptr->tel.tel_data_ptr[1] = (uint8_t)type;
        msg_ptr->tel.tel_data_ptr[2] = MISC_HB(lead_in);
        msg_ptr->tel.tel_data_ptr[3] = MISC_LB(lead_in);
        msg_ptr->tel.tel_data_ptr[4] = (uint8_t)((duration) >> 24);
        msg_ptr->tel.tel_data_ptr[5] = (uint8_t)((duration) >> 16);
        msg_ptr->tel.tel_data_ptr[6] = (uint8_t)((duration) >> 8);
        msg_ptr->tel.tel_data_ptr[7] = (uint8_t)(duration & (uint32_t)0xFF);
        msg_ptr->tel.tel_data_ptr[8] = MISC_HB(lead_out);
        msg_ptr->tel.tel_data_ptr[9] = MISC_LB(lead_out);


        msg_ptr->info_ptr = &self->ssubs.phylaytest;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.phylaytest, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief  Requests the EXC.PhysicalLayerTestResult.Status message
 *  \param  self        Reference to CExc instance
 *  \param  obs_ptr     Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created 
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Exc_PhyTestResult_Get(CExc *self, 
                                    CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, EXC_API_PHY_LAY_TEST_RESULT) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = MSG_ADDR_INIC;

            msg_ptr->id.fblock_id      = FB_EXC;
            msg_ptr->id.instance_id    = 0U;
            msg_ptr->id.function_id    = EXC_FID_PHY_LAY_TEST_RES;
            msg_ptr->id.op_type        = UCS_OP_GET;

            msg_ptr->info_ptr = &self->ssubs.phylaytestresult;
            Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

            (void)Ssub_AddObserver(&self->ssubs.phylaytestresult, obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, EXC_API_PHY_LAY_TEST_RESULT);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}



/*! Sends the BCDiag.Startresult command
 *
 * \param *self         Reference to CExc instance
 * \param position      Position of the segment to be checked.
 * \param admin_na      Admin Node Address
 * \param t_send        Timing parameter t_Send
 * \param t_wait4dut    Timing parameter t_WaitForDUT
 * \param t_switch      Timing parameter t_Switch
 * \param t_back        Timing parameter t_Back
 * \param autoback      TBD
 * \param *obs_ptr      Reference to an optional observer
 * \return UCS_RET_SUCCESS               message was created 
 * \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_BCDiag_Start(CExc *self, 
                              uint8_t position, 
                              uint16_t admin_na,
                              uint16_t t_send,
                              uint16_t t_wait4dut, 
                              uint16_t t_switch,
                              uint16_t t_back,
                              bool     autoback,
                              CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 12U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = UCS_ADDR_BROADCAST_BLOCKING; 
        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_BC_DIAG;
        msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
        msg_ptr->tel.tel_data_ptr[0] = position; 
        msg_ptr->tel.tel_data_ptr[1] = MISC_HB(admin_na);
        msg_ptr->tel.tel_data_ptr[2] = MISC_LB(admin_na);
        msg_ptr->tel.tel_data_ptr[3] = MISC_HB(t_send);
        msg_ptr->tel.tel_data_ptr[4] = MISC_LB(t_send);
        msg_ptr->tel.tel_data_ptr[5] = MISC_HB(t_wait4dut);
        msg_ptr->tel.tel_data_ptr[6] = MISC_LB(t_wait4dut);
        msg_ptr->tel.tel_data_ptr[7] = MISC_HB(t_switch);
        msg_ptr->tel.tel_data_ptr[8] = MISC_LB(t_switch);
        msg_ptr->tel.tel_data_ptr[9] = MISC_HB(t_back);
        msg_ptr->tel.tel_data_ptr[10] = MISC_LB(t_back);
        msg_ptr->tel.tel_data_ptr[11] = (uint8_t)autoback;


        msg_ptr->info_ptr = &self->ssubs.bcdiag;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.bcdiag, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! Enables the signal during backChannel Diagnosis
 *
 * \param *self         Reference to CExc instance
 * \param port          Number of port which has to be enabled.
 * \param *obs_ptr      Reference to an optional observer
 * \return UCS_RET_SUCCESS               message was created 
 * \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Exc_BCEnableTx_StartResult(CExc *self, 
                                        uint8_t port,
                                        CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = UCS_ADDR_BROADCAST_BLOCKING; 
        msg_ptr->id.fblock_id        = FB_EXC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = EXC_FID_BC_ENABLE_TX;
        msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
        msg_ptr->tel.tel_data_ptr[0] = port;

        msg_ptr->info_ptr = &self->ssubs.enabletx;
        Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

        (void)Ssub_AddObserver(&self->ssubs.enabletx, obs_ptr);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}


/*! \brief This function is used to open a memory session. 
 *
 *  A memory session is used to control access to the memory resources. Before a memory could 
 *  be read or written, a session of the appropriate type has to be opened.
 *  Only a single memory session is supported. Once opened, the session must be first
 *  closed before a new session of a different type could be used. Some session types
 *  (0x01, 0x02 and 0x04) require a hardware reset after they were closed.
 *  Function Exc_MemSessionOpen_Sr() also performs some preprocessing,
 *  depending on the session_type. This includes clearing of the configuration
 *  and identification strings before the error memory is programmed or erased.
 *
 * \param *self           Reference to CExc instance 
 * \param target_address  Target address
 * \param session_type    Defines the set of MemIDs and the memory access type(s) (read and/or write)
 * \param *obs_ptr        Reference to an optional observer
 *
 * \return UCS_RET_SUCCESS message was created and sent to INIC
 * \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 * \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Exc_MemSessionOpen_Sr(CExc *self, 
                                   uint16_t target_address, 
                                   uint8_t session_type,
                                   CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, EXC_API_MEM_SESSION_OPEN) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = target_address;

            msg_ptr->id.fblock_id        = FB_EXC;
            msg_ptr->id.instance_id      = 0U;
            msg_ptr->id.function_id      = EXC_FID_MEM_SESSION_OPEN;
            msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
            msg_ptr->tel.tel_data_ptr[0] = session_type; 

            msg_ptr->info_ptr = &self->ssubs.memsessionopen;
            Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

            (void)Ssub_AddObserver(&self->ssubs.memsessionopen, obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, EXC_API_MEM_SESSION_OPEN);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}


/*! \brief This function is used to close an active memory session that was previously opened by
 *         function Exc_MemSessionOpen_Sr(). 
 *
 *  In addition, the function performs some post-processing on given session types. This includes 
 *  validation of the newly programmed configuration and identification strings as well as 
 *  the deactivation of the current configuration and identification strings. In these cases, 
 *  the new configuration becomes active after a hardware reset.
 *
 * \param *self             Reference to CExc instance 
 * \param target_address    Target address
 * \param session_handle    Unique number assigned to the active memory session
 * \param *obs_ptr          Reference to an optional observer
 *
 * \return UCS_RET_SUCCESS message was created and sent to INIC
 * \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available 
 * \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Exc_MemSessionClose_Sr(CExc *self, 
                                    uint16_t target_address, 
                                    uint16_t session_handle,
                                    CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, EXC_API_MEM_SESSION_CLOSE) != false)
    {
        Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr    = target_address;

            msg_ptr->id.fblock_id        = FB_EXC;
            msg_ptr->id.instance_id      = 0U;
            msg_ptr->id.function_id      = EXC_FID_MEM_SESSION_CLOSE;
            msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(session_handle); 
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(session_handle); 

            msg_ptr->info_ptr = &self->ssubs.memsessionclose;
            Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

            (void)Ssub_AddObserver(&self->ssubs.memsessionclose, obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.api, EXC_API_MEM_SESSION_CLOSE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}


/*! \brief This function provides read access to the memories described by parameter MemID.
 *
 *  In addition, the function can be used to retrieve the active Configuration String and 
 *  Identification String.
 *  Reading the memory can only be done within an active memory session. Parameter
 *  session_handle authorizes the access to the memory resource defined by parameter
 *  MemID. The session_handle is provided by function Exc_MemSessionOpen_Sr(),
 *  which must be called in advance to memory access.
 *
 * \param *self             Reference to CExc instance 
 * \param target_address    Target address
 * \param session_handle    Unique number assigned to the active memory session
 * \param mem_id            Represents the memory resource to be read
 * \param address           Defines the memory location at which the reading operation starts
 * \param unit_len          Sets the number of memory units to be read. Memory units can be 
 *                          unsigned bytes, unsigned words or unsigned masked data depending 
 *                          on the memory type.
 * \param *obs_ptr          Reference to an optional observer
 *
 * \return UCS_RET_SUCCESS              message was created and sent to INIC
 * \return UCS_RET_ERR_BUFFER_OVERFLOW  no message buffer available 
 * \return UCS_RET_ERR_PARAM            parameter ubit_len ist too big
 * \return UCS_RET_ERR_API_LOCKED       Resource API is already used by another command
 */
Ucs_Return_t Exc_MemoryRead_Sr(CExc *self, 
                               uint16_t target_address, 
                               uint16_t session_handle,
                               uint8_t  mem_id,
                               uint32_t address,
                               uint8_t  unit_len,
                               CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, EXC_API_MEM_READ) != false)
    {
        if (unit_len > MAX_UNIT_LEN)
        {
            result = UCS_RET_ERR_PARAM;
        }

        if (result == UCS_RET_SUCCESS)
        {
            Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 8U);

            if (msg_ptr != NULL)
            {
                msg_ptr->destination_addr    = target_address;
                msg_ptr->id.fblock_id        = FB_EXC;
                msg_ptr->id.instance_id      = 0U;
                msg_ptr->id.function_id      = EXC_FID_MEMORY_READ;
                msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
                msg_ptr->tel.tel_data_ptr[0] = MISC_HB(session_handle); 
                msg_ptr->tel.tel_data_ptr[1] = MISC_LB(session_handle); 
                msg_ptr->tel.tel_data_ptr[2] = mem_id; 
                msg_ptr->tel.tel_data_ptr[3] = (uint8_t)((address) >> 24);
                msg_ptr->tel.tel_data_ptr[4] = (uint8_t)((address) >> 16);
                msg_ptr->tel.tel_data_ptr[5] = (uint8_t)((address) >> 8);
                msg_ptr->tel.tel_data_ptr[6] = (uint8_t)(address & (uint32_t)0xFF);
                msg_ptr->tel.tel_data_ptr[7] = unit_len; 

                msg_ptr->info_ptr = &self->ssubs.memoryread;
                Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

                (void)Ssub_AddObserver(&self->ssubs.memoryread, obs_ptr);
            }
            else
            {
                Al_Release(&self->lock.api, EXC_API_MEM_READ);
                result = UCS_RET_ERR_BUFFER_OVERFLOW;
            }
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}


/*! \brief This function provides write access to the memories described by parameter MemID. 
 *
 *  In addition, the function can be used to program a new Configuration String and Identification
 *  String.
 *  Writing the memory can only be done within an active memory session. Parameter
 *  SessionHandle authorizes the access to the memory resource defined by parameter
 *  MemID. The SessionHandle is provided by function ExtendedNetworkControl.MemorySessionOpen(),
 *  which must be called in advance to memory access.
 *
 * \param *self             Reference to CExc instance 
 * \param target_address    Target address
 * \param session_handle    Unique number assigned to the active memory session
 * \param mem_id            Represents the memory resource to be read
 * \param address           Defines the memory location at which the reading operation starts
 * \param unit_len          Sets the number of memory units to be read. Memory units can be 
 *                          unsigned bytes, unsigned words or unsigned masked data depending 
 *                          on the memory type.
 * \param *unit_data        Contains the actual data written to the memory resource and formatted 
 *                          as memory units
 * \param *obs_ptr          Reference to an optional observer
 *
 * \return UCS_RET_SUCCESS              message was created and sent to INIC
 * \return UCS_RET_ERR_BUFFER_OVERFLOW  no message buffer available 
 * \return UCS_RET_ERR_PARAM            parameter ubit_len ist too big
 * \return UCS_RET_ERR_API_LOCKED       Resource API is already used by another command
 */
Ucs_Return_t Exc_MemoryWrite_Sr(CExc *self, 
                                uint16_t target_address, 
                                uint16_t session_handle,
                                uint8_t  mem_id,
                                uint32_t address,
                                uint8_t  unit_len,
                                uint8_t  unit_data[],
                                CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if(Al_Lock(&self->lock.api, EXC_API_MEM_WRITE) != false)
    {
        if (unit_len > MAX_UNIT_LEN)
        {
            result = UCS_RET_ERR_PARAM;
        }

        if (result == UCS_RET_SUCCESS)
        {
            Msg_MostTel_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 8U + unit_len);

            if (msg_ptr != NULL)
            {
                uint8_t i;

                msg_ptr->destination_addr    = target_address;
                msg_ptr->id.fblock_id        = FB_EXC;
                msg_ptr->id.instance_id      = 0U;
                msg_ptr->id.function_id      = EXC_FID_MEMORY_WRITE;
                msg_ptr->id.op_type          = UCS_OP_STARTRESULT;
                msg_ptr->tel.tel_data_ptr[0] = MISC_HB(session_handle); 
                msg_ptr->tel.tel_data_ptr[1] = MISC_LB(session_handle); 
                msg_ptr->tel.tel_data_ptr[2] = mem_id; 
                msg_ptr->tel.tel_data_ptr[3] = (uint8_t)((address) >> 24);
                msg_ptr->tel.tel_data_ptr[4] = (uint8_t)((address) >> 16);
                msg_ptr->tel.tel_data_ptr[5] = (uint8_t)((address) >> 8);
                msg_ptr->tel.tel_data_ptr[6] = (uint8_t)(address & (uint32_t)0xFF);
                msg_ptr->tel.tel_data_ptr[7] = unit_len; 
                for (i=0U; i<unit_len; ++i)
                {
                    msg_ptr->tel.tel_data_ptr[8U+i] = *(unit_data + i);
                }

                msg_ptr->info_ptr = &self->ssubs.memorywrite;
                Trcv_TxSendMsg(self->xcvr_ptr, msg_ptr);

                (void)Ssub_AddObserver(&self->ssubs.memorywrite, obs_ptr);
            }
            else
            {
                Al_Release(&self->lock.api, EXC_API_MEM_WRITE);
                result = UCS_RET_ERR_BUFFER_OVERFLOW;
            }
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
/*! \brief Handler function for EXC.Hello.Status
 *  \param  self        Reference to EXC object
 *  \param  msg_ptr     Received message
 */
static void Exc_Hello_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_HelloStatus_t hello_data; 
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len >= (EXC_SIGNATURE_LEN_V1  + 1U))
    {
        hello_data.version =    msg_ptr->tel.tel_data_ptr[0];
        Exc_Read_Signature(&(hello_data.signature), &(msg_ptr->tel.tel_data_ptr[1]));

        res_data.data_info        = &hello_data;
        res_data.result.code      = UCS_RES_SUCCESS;
        res_data.result.info_ptr  = NULL;
        res_data.result.info_size = 0U;

        /* Node Discovery sends the Hello.Get as broadcast message. So we will need the observer
           several times. */ 
        Ssub_Notify(&self_->ssubs.hello, &res_data, false); 
    }
}


/*! \brief Handler function for EXC.Hello.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_Hello_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        /* Node Discovery sends the Hello.Get as broadcast message. So we will need the observer
           several times. */ 
        Ssub_Notify(&self_->ssubs.hello, &res_data, false);
    }
}


/*! \brief Handler function for EXC.Welcome.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_Welcome_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.welcome, &res_data, true);
    }
}

/*! \brief Handler function for the EXC.Welcome.Result message
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_Welcome_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_WelcomeResult_t welcome_data; 
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len >= (EXC_SIGNATURE_LEN_V1  + 2U))
    {
        welcome_data.res = msg_ptr->tel.tel_data_ptr[0];
        welcome_data.version = msg_ptr->tel.tel_data_ptr[1];
        Exc_Read_Signature(&(welcome_data.signature), &(msg_ptr->tel.tel_data_ptr[2]));
        res_data.data_info       = &welcome_data;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.welcome, &res_data, true);
    }
}


/*! Handler function for the EXC.Signature.Status message
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_Signature_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_SignatureStatus_t signature_data; 
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len >= (EXC_SIGNATURE_LEN_V1  + 1U))
    {
        signature_data.version =    msg_ptr->tel.tel_data_ptr[0];
        Exc_Read_Signature(&(signature_data.signature), &(msg_ptr->tel.tel_data_ptr[1]));

        res_data.data_info       = &signature_data;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;
        res_data.result.info_size = 0U;

        Ssub_Notify(&self_->ssubs.signature, &res_data, true);
    }
}


/*! Handler function for the EXC.Signature.Error message
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_Signature_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.signature, &res_data, true);
    }
}


/*! Handler function for the EXC.DeviceInit.Error message
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_DeviceInit_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len >0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.deviceinit, &res_data, true);
    }
}


/*! \brief Handler function for EXC.EnablePort.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_EnablePort_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.result = Exc_TranslateError(self_, 
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.enableport, &res_data, true);
    }
}

/*! \brief Handler function for EXC.EnablePort.Result
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_EnablePort_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    MISC_UNUSED(msg_ptr);

    res_data.result.code = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs.enableport, &res_data, true);
}


/*! \brief Handler function for EXC.CableLinkDiag.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_CableLinkDiag_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)   
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0],
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.cablelinkdiag, &res_data, true);
    }
}

/*! \brief Handler function for EXC.CableLinkDiag.Result
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_CableLinkDiag_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_CableLinkDiagResult_t cable_link_diag_result_data; 
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        cable_link_diag_result_data.port_number = msg_ptr->tel.tel_data_ptr[0];
        cable_link_diag_result_data.result      = msg_ptr->tel.tel_data_ptr[1];
        res_data.data_info       = &cable_link_diag_result_data;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.cablelinkdiag, &res_data, true);
    }
}


/*! \brief Handler function for EXC.PhysicalLayerTest.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */static void Exc_NwPhyTest_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.phylaytest, &res_data, true);
    }
}


/*! \brief Handler function for EXC.MOSTNetworkPhysicalLayerTestResult.Status
 *  \param  self        Reference to EXC object
 *  \param  msg_ptr     Received message
 */
static void Exc_NwPhyTestResult_Status(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_PhyTestResult_t phy_test_result;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        phy_test_result.port_number = msg_ptr->tel.tel_data_ptr[0];
        phy_test_result.lock_status = (msg_ptr->tel.tel_data_ptr[1] != 0U) ? true : false;
        MISC_DECODE_WORD(&(phy_test_result.err_count), &(msg_ptr->tel.tel_data_ptr[2]));
        res_data.data_info       = &phy_test_result;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.phylaytestresult, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_PHY_LAY_TEST_RESULT);
}


/*! \brief Handler function for EXC.MOSTNetworkPhysicalLayerTestResult.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_NwPhyTestResult_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.phylaytestresult, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_PHY_LAY_TEST_RESULT);
}



/*! \brief Handler function for EXC.BCDiag.Status
 *  \param  self        Reference to EXC object
 *  \param  msg_ptr     Received message
 */
static void Exc_BC_Diag_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_BCDiagResult bcd_result;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 1U)
    {
        bcd_result.diag_result = (Exc_BCDiagResValue)(msg_ptr->tel.tel_data_ptr[0] >> 4U);
        MISC_DECODE_WORD(&(bcd_result.admin_addr), &(msg_ptr->tel.tel_data_ptr[0]));
        bcd_result.admin_addr   &= 0x0FFFU;
        res_data.data_info       = &bcd_result;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.bcdiag, &res_data, true);
    }
}


/*! \brief Handler function for EXC.BCDiag.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_BC_Diag_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.bcdiag, &res_data, true);
    }
}




/*! \brief Handler function for EXC.BCEnableTx.Result
 *  \param  self        Reference to EXC object
 *  \param  msg_ptr     Received message
 */
static void Exc_BC_EnableTx_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    Ssub_Notify(&self_->ssubs.enabletx, &res_data, true);

    MISC_UNUSED(msg_ptr);
}


/*! \brief Handler function for EXC.BCEnableTx.Error
 * \param  self     reference to EXC object
 * \param  msg_ptr  received message
 */
static void Exc_BC_EnableTx_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.enabletx, &res_data, true);
    }
}


/*! \brief Handler function for EXC.MemorySessionOpen.Result
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemSessionOpen_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    uint16_t session_handle;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        MISC_DECODE_WORD(&(session_handle),  &(msg_ptr->tel.tel_data_ptr[0]));
        res_data.data_info       = &session_handle;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.memsessionopen, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_SESSION_OPEN);
}


/*! \brief Handler function for EXC.MemorySessionOpen.Error
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemSessionOpen_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.memsessionopen, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_SESSION_OPEN);
}


/*! \brief Handler function for EXC.MemorySessionClose.Result
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemSessionClose_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    uint8_t session_result;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        session_result = msg_ptr->tel.tel_data_ptr[0];
        res_data.data_info       = &session_result;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.memsessionclose, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_SESSION_CLOSE);
}

/*! \brief Handler function for EXC.MemorySessionClose.Error
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemSessionClose_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.memsessionclose, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_SESSION_CLOSE);
}

/*! \brief Handler function for EXC.MemoryRead.Result
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemoryRead_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_MemReadResult_t mem_read_result; 
    Exc_StdResult_t res_data;
    uint8_t i;

    if (msg_ptr->tel.tel_len > 0U)
    {
        MISC_DECODE_WORD(&(mem_read_result.session_handle), &(msg_ptr->tel.tel_data_ptr[0]));
        mem_read_result.mem_id = msg_ptr->tel.tel_data_ptr[2];
        MISC_DECODE_DWORD(&(mem_read_result.address), &(msg_ptr->tel.tel_data_ptr[3]));
        mem_read_result.unit_len = msg_ptr->tel.tel_data_ptr[7];
        for (i=0U; (i<mem_read_result.unit_len) && (i<MAX_UNIT_LEN); ++i)
        {
            mem_read_result.unit_data[i] = msg_ptr->tel.tel_data_ptr[8U+i];
        }

        res_data.data_info       = &mem_read_result;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.memoryread, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_READ);
}


/*! \brief Handler function for EXC.MemoryRead.Error
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemoryRead_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.memoryread, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_READ);
}


/*! \brief Handler function for EXC.MemoryWrite.Result
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemoryWrite_Result(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_MemWriteResult_t mem_write_result; 
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        MISC_DECODE_WORD(&(mem_write_result.session_handle), &(msg_ptr->tel.tel_data_ptr[0]));
        mem_write_result.mem_id = msg_ptr->tel.tel_data_ptr[2];

        res_data.data_info       = &mem_write_result;
        res_data.result.code     = UCS_RES_SUCCESS;
        res_data.result.info_ptr = NULL;

        Ssub_Notify(&self_->ssubs.memorywrite, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_WRITE);
}


/*! \brief Handler function for EXC.MemoryWrite.Error
 *  \param  self     reference to EXC object
 *  \param  msg_ptr  received message
 */
static void Exc_MemoryWrite_Error(void *self, Msg_MostTel_t *msg_ptr)
{
    CExc *self_ = (CExc *)self;
    Exc_StdResult_t res_data;

    if (msg_ptr->tel.tel_len > 0U)
    {
        res_data.data_info  = NULL;
        res_data.result = Exc_TranslateError(self_,
                                             &msg_ptr->tel.tel_data_ptr[0], 
                                             (uint8_t)(msg_ptr->tel.tel_len));

        Ssub_Notify(&self_->ssubs.memorywrite, &res_data, true);
    }
    Al_Release(&self_->lock.api, EXC_API_MEM_WRITE);
}



/*------------------------------------------------------------------------------------------------*/
/* Helper functions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Translates EXC error codes into UNICENS error codes and wraps the raw INIC
 *         error data to a byte stream.
 *  \param self         Instance of CExc
 *  \param error_data[] EXC error data
 *  \param error_size   Size of EXC error data in bytes
 *  \return             The formatted error
 */
static Ucs_StdResult_t Exc_TranslateError(CExc *self, uint8_t error_data[], uint8_t error_size)
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


/*! \brief  Reads a signature from a message's payload 
 *
 * \param dest      Pointer to signature 
 * \param source    Pointer to start of signature inabyte array
 */
static void Exc_Read_Signature(Ucs_Signature_t *dest, uint8_t source[])
{
    MISC_DECODE_WORD(&(dest->node_address),  source);
    MISC_DECODE_WORD(&(dest->group_address), &(source[2]));
    MISC_DECODE_WORD(&(dest->mac_47_32),     &(source[4]));
    MISC_DECODE_WORD(&(dest->mac_31_16),     &(source[6]));
    MISC_DECODE_WORD(&(dest->mac_15_0),      &(source[8]));
    MISC_DECODE_WORD(&(dest->node_pos_addr), &(source[10]));
    MISC_DECODE_WORD(&(dest->diagnosis_id),  &(source[12]));
    dest->num_ports  = source[14];
    dest->chip_id    = source[15];
    dest->fw_major   = source[16];
    dest->fw_minor   = source[17];
    dest->fw_release = source[18];
    MISC_DECODE_DWORD(&(dest->fw_build),  &(source[19]));
    dest->cs_major   = source[23];
    dest->cs_minor   = source[24];
    dest->cs_release = source[25];
/*    dest->uid_persistency = source[26];*/                 /* Signature v1 */
/*    MISC_DECODE_DWORD(&(dest->uid),  &(source[27]));*/

}
/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

