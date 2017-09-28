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
 * \brief Internal header file of the EndPoint Manager.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EPM
 * @{
 */


#ifndef UCS_EPM_H
#define UCS_EPM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_ret_pb.h"
#include "ucs_rm_pb.h"
#include "ucs_xrm.h"
#include "ucs_factory.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*!< \brief  Stores data required by EPM during initialization. */
typedef struct Epm_InitData_
{
    CBase * base_ptr;                               /*!< \brief Reference to a base instance */
    CFactory * fac_ptr;                             /*!< \brief Reference to factory instance */
    Ucs_Rm_XrmResDebugCb_t res_debugging_fptr;      /*!< \brief Reference to the observer callback function for XRM resources */
    Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr;      /*!< \brief Reference to the callback function pointer to signal "check unmute" of devices */

} Epm_InitData_t;

/*! \brief  Class structure of the EndPoint Management. */
typedef struct CEndpointManagement_
{
    /*!< \brief Reference to a base instance */
    CBase *base_ptr;
    /*!< \brief Reference to factory instance */
    CFactory * fac_ptr;
    /*!< \brief Reference to the application debugging callback function for XRM resources */
    Ucs_Rm_XrmResDebugCb_t res_debugging_fptr;
    /*!< \brief Reference to the callback function pointer to signal "check unmute" of devices */
    Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr;

} CEndpointManagement;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CEndpointManagement                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Epm_Ctor(CEndpointManagement * self, Epm_InitData_t * init_ptr);
extern void Epm_InitInternalInfos (CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern void Epm_ClearIntInfos(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern void Epm_AddObserver(Ucs_Rm_EndPoint_t * ep_ptr, CObserver * obs_ptr);
extern void Epm_DelObserver(Ucs_Rm_EndPoint_t * ep_ptr, CObserver * obs_ptr);
extern Ucs_Return_t Epm_SetBuildProcess(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern Ucs_Return_t Epm_SetDestroyProcess(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern uint16_t Epm_GetConnectionLabel(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern void Epm_SetConnectionLabel(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr, uint16_t conn_label);
extern Ucs_Rm_EndPointState_t Epm_GetState(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern void Epm_ResetState(CEndpointManagement * self, Ucs_Rm_EndPoint_t * ep_ptr);
extern void Epm_ReportShutDown(CEndpointManagement * self);
extern void Epm_ReportInvalidDevice(CEndpointManagement *self, uint16_t destination_address);
extern void Epm_XrmResDebugCb(Ucs_Xrm_ResourceType_t resource_type, Ucs_Xrm_ResObject_t *resource_ptr, 
                              Ucs_Xrm_ResourceInfos_t resource_infos, void *endpoint_inst_ptr, void *user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_EPM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

