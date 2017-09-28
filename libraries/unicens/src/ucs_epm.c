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
 * \brief Implementation of the EndPoint Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EPM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_epm.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Epm_XrmReportCb (uint16_t node_address, uint16_t connection_label, Ucs_Xrm_Result_t result, void * user_arg);
static bool Epm_RsmReportSyncLost (Fac_Inst_t inst_type, void * inst_ptr, void *ud_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CEndpointManagement                                                     */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Remote Sync Manager class.
 *  \param self        Instance pointer
 *  \param init_ptr    init data_ptr
 */
void Epm_Ctor(CEndpointManagement *self, Epm_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CEndpointManagement));

    /* Init all instances */
    self->fac_ptr = init_ptr->fac_ptr;
    self->base_ptr = init_ptr->base_ptr;
    self->res_debugging_fptr = init_ptr->res_debugging_fptr;
    self->check_unmute_fptr  = init_ptr->check_unmute_fptr;
}

/*! \brief Initializes the internal information of the given endpoint object.
 *
 *  Initialization is performed only if the magic number is not set.
 *
 *  \param self      Instance pointer
 *  \param ep_ptr    Reference to the endpoint to be looked for
 */
void Epm_InitInternalInfos(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    if ((self != NULL) && (ep_ptr != NULL))
    {
        if (ep_ptr->internal_infos.magic_number != (uint32_t)0x0BADC0DE)
        {
            MISC_MEM_SET(&ep_ptr->internal_infos, 0, sizeof(Ucs_Rm_EndPointInt_t));

            ep_ptr->internal_infos.magic_number = (uint32_t)0x0BADC0DE;
            Sub_Ctor(&ep_ptr->internal_infos.subject_obj, self->base_ptr->ucs_user_ptr);
            /* Set the EndpointManagement instance */
            ep_ptr->internal_infos.epm_inst = (Epm_Inst_t *)(void *)self;
        }
    }
}

/*! \brief Clears the internal information of the given endpoint object.
 *
 * Resetting the magic number of the given endpoint will enforce Its Re-Initialization.
 *
 *  \param self      Instance pointer
 *  \param ep_ptr    Reference to the endpoint to be cleared.
 */
void Epm_ClearIntInfos(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    MISC_UNUSED (self);
    if (ep_ptr != NULL)
    {
        ep_ptr->internal_infos.magic_number = 0x0U;
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Add an observer to the Endpoint's subject.
 *  \param ep_ptr     Reference to the endpoint instance
 *  \param obs_ptr    Reference to the observer object
 */
void Epm_AddObserver(Ucs_Rm_EndPoint_t * ep_ptr, CObserver * obs_ptr)
{
    Sub_Ret_t ret_val = SUB_UNKNOWN_OBSERVER;

    ret_val = Sub_AddObserver(&ep_ptr->internal_infos.subject_obj, obs_ptr);
    if (ret_val == SUB_OK)
    {
        if ((ep_ptr != NULL) && (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE))
        {
            if ((ep_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT) && (ep_ptr->internal_infos.reference_cnt > 0U))
            {
                ep_ptr->internal_infos.reference_cnt++;
            }
        }
    }
}

/*! \brief Removes an observer registered by Epm_AddObserver
 *  \param ep_ptr     Reference to the endpoint instance
 *  \param obs_ptr    Reference to the observer object
 */
void Epm_DelObserver(Ucs_Rm_EndPoint_t * ep_ptr, CObserver * obs_ptr)
{
    (void)Sub_RemoveObserver(&ep_ptr->internal_infos.subject_obj, obs_ptr);  
}

/*! \brief Processes the construction of the given endpoint
 *  \param self               Instance pointer
 *  \param ep_ptr             reference to an endpoint 
 *  \return Possible return values are
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked. Endpoint is currently being processed.
 *          - \c UCS_RET_SUCCESS the build process was set successfully
 *          - \c UCS_RET_ERR_PARAM NULL pointer detected in the parameter list
 *          - \c UCS_RET_ERR_ALREADY_SET the endpoint has already been set
 */
Ucs_Return_t Epm_SetBuildProcess(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((self != NULL)  && (ep_ptr != NULL))
    {
        /* Process Endpoint construction by XRM */
        result = Xrm_Process(Fac_GetXrm(self->fac_ptr, ep_ptr->node_obj_ptr->signature_ptr->node_address, &Epm_XrmResDebugCb, self->check_unmute_fptr),
                            ep_ptr->jobs_list_ptr, ep_ptr->internal_infos.connection_label, 
                            (void *)ep_ptr, &Epm_XrmReportCb);
        if (result == UCS_RET_SUCCESS)
        {
            if (ep_ptr->internal_infos.endpoint_state != UCS_RM_EP_BUILT)
            {
                ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_XRMPROCESSING;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "XRM has been ordered to create following Endpoint: %X", 1U, ep_ptr));
            }
            else
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has already been built", 1U, ep_ptr));
            }
        }
        else if (result == UCS_RET_ERR_ALREADY_SET)
        {
            if (ep_ptr->internal_infos.endpoint_state == UCS_RM_EP_IDLE)
            {
                ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_BUILT;          
                TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has already been built", 1U, ep_ptr));
            }
        }
        else if (result == UCS_RET_ERR_NOT_AVAILABLE)
        {
            /* Set the internal error */
            ep_ptr->internal_infos.endpoint_state  = UCS_RM_EP_IDLE;
            ep_ptr->internal_infos.xrm_result.code = UCS_XRM_RES_ERR_BUILD;
            ep_ptr->internal_infos.xrm_result.details.result_type = UCS_XRM_RESULT_TYPE_INT;
            ep_ptr->internal_infos.xrm_result.details.int_result  = result;
        }
    }

    return result;
}

/*! \brief Processes the destruction of the given endpoint
 *  \param self               Instance pointer
 *  \param ep_ptr             reference to an endpoint 
 *  \return Possible return values are
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked. Endpoint is currently being processed.
 *          - \c UCS_RET_SUCCESS the build process was set successfully
 *          - \c UCS_RET_ERR_PARAM At least one parameter is not correct, either NULL pointer in the param list or reference_cnt of the endpoint is NULL.
 *          - \c UCS_RET_ERR_ALREADY_SET the endpoint has already been set
 *          - \c UCS_RET_ERR_NOT_AVAILABLE the endpoint cannot be destroyed since its reference_cnt is greater than 1, i.e. it's in use.
 *          - \c UCS_RET_ERR_INVALID_SHADOW the endpoint cannot be destroyed since its reference_cnt is greater than 1, i.e. it's in use.
 */
Ucs_Return_t Epm_SetDestroyProcess(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;
    bool can_be_destroyed = true;

    if ((self != NULL)  && (ep_ptr != NULL) )
    {
        if (UCS_RM_EP_SOURCE == ep_ptr->endpoint_type)
        {
            if (ep_ptr->internal_infos.reference_cnt == 0U)
            {
                can_be_destroyed = false;
                result = UCS_RET_ERR_PARAM;
            }
            else if (ep_ptr->internal_infos.reference_cnt > 1U)
            {
                ep_ptr->internal_infos.reference_cnt--;
                can_be_destroyed = false;
                result = UCS_RET_ERR_INVALID_SHADOW;
            }
        }

        if (can_be_destroyed)
        {
            result = Xrm_Destroy(Fac_GetXrmByJobList(self->fac_ptr, ep_ptr->jobs_list_ptr), ep_ptr->jobs_list_ptr);
            if (result == UCS_RET_SUCCESS)
            {
                ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_XRMPROCESSING;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "XRM has been ordered to destroy following Endpoint {%X}", 1U, ep_ptr));
            }
            else if (result == UCS_RET_ERR_ALREADY_SET)
            {
                if (ep_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT)
                {
                    ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;          
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has already been destroyed", 1U, ep_ptr));
                }
            }
            else if (result == UCS_RET_ERR_NOT_AVAILABLE)
            {
                if (ep_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT)
                {
                    ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has already been destroyed", 1U, ep_ptr));
                }
            }
        }
    }

    return result;
}

/*! \brief Returns the state (idle, processing or built) of the given endpoint.
 *  \param self   Instance pointer.
 *  \param ep_ptr Reference to the endpoint to be looked for
 *  \return state of the endpoint.
 */
Ucs_Rm_EndPointState_t Epm_GetState(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    MISC_UNUSED (self);

    return (ep_ptr != NULL) ? ep_ptr->internal_infos.endpoint_state:UCS_RM_EP_IDLE;
}

/*! \brief Forces EPM to reset the state of this endpoint.
 *  \param self      Instance pointer.
 *  \param ep_ptr    Reference to the endpoint to be looked for.
 */
void Epm_ResetState(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    MISC_UNUSED (self);

    if (ep_ptr != NULL)
    {
        ep_ptr->internal_infos.endpoint_state  = UCS_RM_EP_IDLE;
        ep_ptr->internal_infos.xrm_result.code = UCS_XRM_RES_UNKNOWN;
    }
}

/*! \brief Sets the connection label of the given endpoint.
 *  \param self       Instance pointer.
 *  \param ep_ptr     Reference to the endpoint to be looked for
 *  \param conn_label connection label to be set
 */
void Epm_SetConnectionLabel(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr, uint16_t conn_label)
{
    MISC_UNUSED (self);

    if (ep_ptr != NULL)
    {
        ep_ptr->internal_infos.connection_label = conn_label;
    }
}

/*! \brief Returns the connection label of the given endpoint.
 *  \param self   Instance pointer.
 *  \param ep_ptr Reference to the endpoint to be looked for
 *  \return connection label of the endpoint.
 */
uint16_t Epm_GetConnectionLabel(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr)
{
    MISC_UNUSED (self);

    return (ep_ptr != NULL) ? ep_ptr->internal_infos.connection_label:0U;
}

/*! \brief  This function must be called when a device get invalid. 
 *  \param  self                Reference to the MNS instance.
 *  \param  destination_address MOST device address of the target.
 */
void Epm_ReportInvalidDevice(CEndpointManagement *self, uint16_t destination_address)
{
    if (MSG_ADDR_INIC != destination_address)
    {
        CRemoteSyncManagement * rsm_inst = Fac_FindRsm(self->fac_ptr, destination_address);
        if (NULL != rsm_inst)
        {
            Rsm_ReportSyncLost(rsm_inst);
        }
    }
}

/*! \brief Whenever this function has been called, the EndpointManager has to inform his sub-modules that a shutdown occurred. 
 *         This function forwards the Network "NotAvailable" information
 *  \param self   Instance pointer.
 */
void Epm_ReportShutDown(CEndpointManagement * self)
{
    Fac_Foreach(self->fac_ptr, FAC_INST_RSM, &Epm_RsmReportSyncLost, NULL);
}

/*! \brief  Function signature used for monitoring the XRM resources.
 *  \param  resource_type       The XRM resource type to be looked for
 *  \param  resource_ptr        Reference to the resource to be looked for
 *  \param  resource_infos      Resource information
 *  \param  endpoint_inst_ptr   Reference to the endpoint object that encapsulates the given resource.
 *  \param  user_ptr            User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
void Epm_XrmResDebugCb (Ucs_Xrm_ResourceType_t resource_type, Ucs_Xrm_ResObject_t *resource_ptr, 
                               Ucs_Xrm_ResourceInfos_t resource_infos, void *endpoint_inst_ptr, void *user_ptr)
{
    Ucs_Rm_EndPoint_t * ep_ptr = (Ucs_Rm_EndPoint_t *)endpoint_inst_ptr;
    if (ep_ptr != NULL)
    {
        CEndpointManagement * self = (CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst;
        if (self->res_debugging_fptr != NULL)
        {
            self->res_debugging_fptr(resource_type, resource_ptr, resource_infos, ep_ptr, user_ptr);
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Reports "SyncLost" to the RSM instance returned.
 *  \param inst_type  The instance type to be looked for.
 *  \param inst_ptr   Reference to the instance to be looked for.
 *  \param ud_ptr     Reference to the user data.
 *  \return false in order to retrieve the next instance of the given type, otherwise false.
 */
static bool Epm_RsmReportSyncLost(Fac_Inst_t inst_type, void * inst_ptr, void *ud_ptr)
{
    bool ret_val = false;
    MISC_UNUSED(ud_ptr);

    switch (inst_type)
    {
        case FAC_INST_RSM:
            Rsm_ReportSyncLost((CRemoteSyncManagement *)inst_ptr);
            break;

        default:
            ret_val = true;
            break;
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  XRM report callback function.
 *  \param  node_address        The node address from which the results come 
 *  \param  connection_label    Returned MOST network connection label
 *  \param  result              Result of the job
 *  \param  user_arg            Reference to the user argument
 */
static void Epm_XrmReportCb(uint16_t node_address, uint16_t connection_label, Ucs_Xrm_Result_t result, void * user_arg)
{
    Ucs_Rm_EndPoint_t * ep_ptr = (Ucs_Rm_EndPoint_t *)user_arg;
    uint8_t handle_not_found   = 0x32U;
    uint8_t error_id = 2U;

    MISC_UNUSED (node_address);

    if (ep_ptr != NULL)
    {
        ep_ptr->internal_infos.xrm_result = result;
        switch (result.code)
        {
        case UCS_XRM_RES_SUCCESS_BUILD:
            ep_ptr->internal_infos.connection_label = connection_label;
            ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_BUILT;
            if (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                ep_ptr->internal_infos.reference_cnt++;
            }            
            TR_INFO((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has been successfully built", 1U, ep_ptr));
            break;

        case UCS_XRM_RES_SUCCESS_DESTROY:
            ep_ptr->internal_infos.connection_label = 0xFFFFU;
            ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
            if (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                if (ep_ptr->internal_infos.reference_cnt > 0U)
                {
                    ep_ptr->internal_infos.reference_cnt--;
                }
            }
            TR_INFO((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has been successfully destroyed", 1U, ep_ptr));
            break;

        case UCS_XRM_RES_RC_AUTO_DESTROYED:
            TR_ERROR((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Following Endpoint {%X} has been auto destroyed.", 1U, ep_ptr));
            ep_ptr->internal_infos.connection_label = 0xFFFFU;
            ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
            if (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                ep_ptr->internal_infos.reference_cnt = 0U;
            }
            if(Sub_GetNumObservers(&ep_ptr->internal_infos.subject_obj) > 0U)
            {
                Sub_Notify(&ep_ptr->internal_infos.subject_obj, (void *)ep_ptr);
            }
            break;

        case UCS_XRM_RES_ERR_CONFIG:
        case UCS_XRM_RES_ERR_SYNC:
        case UCS_XRM_RES_ERR_BUILD:
            ep_ptr->internal_infos.connection_label = 0xFFFFU;
            ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
            TR_ERROR((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Building endpoint {%X} failed. Error_Code: 0x%02X", 2U, ep_ptr, result.code));
            break;

        case UCS_XRM_RES_ERR_DESTROY:
            ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
            if (ep_ptr->internal_infos.xrm_result.details.result_type == UCS_XRM_RESULT_TYPE_TGT)
            {
                if ((ep_ptr->internal_infos.xrm_result.details.inic_result.code == UCS_RES_ERR_CONFIGURATION) &&
                    (ep_ptr->internal_infos.xrm_result.details.inic_result.info_ptr != NULL) && 
                    (ep_ptr->internal_infos.xrm_result.details.inic_result.info_size > 2U))
                {
                    if (ep_ptr->internal_infos.xrm_result.details.inic_result.info_ptr[error_id] == handle_not_found)
                    {
                        ep_ptr->internal_infos.xrm_result.code = UCS_XRM_RES_SUCCESS_DESTROY;
                    }
                }
            }
            if (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                ep_ptr->internal_infos.reference_cnt = 0U;
            }
            TR_ERROR((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Destroying endpoint {%X} failed. Error_Code: 0x%02X", 2U, ep_ptr, result.code));
            break;

        case UCS_XRM_RES_ERR_INV_LIST:
            TR_ERROR((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Request of invalid lists on endpoint {%X} failed.", 1U, ep_ptr));
            if (ep_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT)
            {
                ep_ptr->internal_infos.connection_label = 0xFFFFU;
                ep_ptr->internal_infos.endpoint_state = UCS_RM_EP_IDLE;
                if(Sub_GetNumObservers(&ep_ptr->internal_infos.subject_obj) > 0U)
                {
                    Sub_Notify(&ep_ptr->internal_infos.subject_obj, (void *)ep_ptr);
                }
            }
            break;

        default:
            TR_ERROR((((CEndpointManagement *)(void *)ep_ptr->internal_infos.epm_inst)->base_ptr->ucs_user_ptr, "[EPM]", "Processing endpoint {%X} failed. Unknown Error_Code: 0x%02X", 2U, ep_ptr, result.code));
            break;
        }
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

