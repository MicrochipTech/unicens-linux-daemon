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
 * \brief Implementation of the Connection Storage Pool.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_XRM_INT
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_xrmpool.h"
#include "ucs_xrm_pv.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class XrmPool                                                                */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the XrmPool class.
 *  \param self        Instance pointer
 */
void Xrmp_Ctor(CXrmPool * self)
{
    uint8_t i;
    MISC_MEM_SET(self, 0, sizeof(CXrmPool));

    /* Initialize resource handle list */
    for(i=0U; i<XRM_NUM_RESOURCE_HANDLES; i++)
    {
        self->resource_handle_list[i].resource_handle       = XRM_INVALID_RESOURCE_HANDLE;
        self->resource_handle_list[i].job_ptr               = NULL;
        self->resource_handle_list[i].resource_object_ptr   = NULL;
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Stores the given resource handle in the resource handle list.
 *  \param  self_ptr            XrmPool Instance pointer
 *  \param  resource_handle     Resource handle to save
 *  \param  job_ptr             Reference to job
 *  \param  resource_object_ptr Reference to resource object
 *  \return \c true if free slot in handle list was found, otherwise \c false
 */
bool Xrmp_StoreResourceHandle(CXrmPool * self_ptr, uint16_t resource_handle, Xrm_Job_t * job_ptr, UCS_XRM_CONST Ucs_Xrm_ResObject_t * resource_object_ptr)
{
    bool ret_val = false;
    uint8_t i;

    for(i=0U; i<XRM_NUM_RESOURCE_HANDLES; i++)
    {
        if(self_ptr->resource_handle_list[i].job_ptr == NULL)
        {
            self_ptr->resource_handle_list[i].job_ptr = job_ptr;
            self_ptr->resource_handle_list[i].resource_object_ptr = resource_object_ptr;
            self_ptr->resource_handle_list[i].resource_handle = resource_handle;
            ret_val = true;
            break;
        }
    }

    return ret_val;
}

/*! \brief  Retrieves the resource handle identified by the given job reference and the given
 *          resource object reference.
 *  \param  self                    Instance pointer
 *  \param  job_ptr                 Reference to the job. Use NULL as wildcard.
 *  \param  resource_object_ptr     Reference to the resource object
 *  \param  func_ptr                Optional function pointer in order to check whether the found job belongs to the provided XRM instance.
 *  \param  usr_ptr                 User pointer used to store the XRM instance to be looked for
 *  \return Resource handle if handle was found, otherwise XRM_INVALID_RESOURCE_HANDLE.
 */
uint16_t Xrmp_GetResourceHandle(CXrmPool * self, Xrm_Job_t * job_ptr, UCS_XRM_CONST Ucs_Xrm_ResObject_t * resource_object_ptr, Xrmp_CheckJobListFunc_t func_ptr, void * usr_ptr)
{
    uint16_t ret_val = XRM_INVALID_RESOURCE_HANDLE;
    uint8_t i;
    bool job_found = true;

    for(i=0U; i<XRM_NUM_RESOURCE_HANDLES; i++)
    {
        if(((self->resource_handle_list[i].job_ptr == job_ptr) || (job_ptr == NULL)) &&
           (self->resource_handle_list[i].resource_object_ptr == resource_object_ptr))
        {
            if ((func_ptr != NULL) && (usr_ptr != NULL))
            {
                job_found = func_ptr(usr_ptr, self->resource_handle_list[i].job_ptr);
            }

            if (job_found)
            {
                ret_val = self->resource_handle_list[i].resource_handle;
                break;
            }
        }
    }

    return ret_val;
}

/*! \brief  Returns the table index of the given resource object.
 *  \param  self        Instance pointer
 *  \param  job_ptr     Reference to job
 *  \param  obj_pptr    Reference to array of references to INIC resource objects
 *  \return Table index of the given resource object. If entry is not found 0xFF is returned.
 */
uint8_t Xrmp_GetResourceHandleIdx(CXrmPool *self, Xrm_Job_t *job_ptr, UCS_XRM_CONST Ucs_Xrm_ResObject_t **obj_pptr)
{
    uint8_t i = 0U;
    uint8_t ret_val = 0xFFU;

    MISC_UNUSED(self);

    while(job_ptr->resource_object_list_ptr[i] != NULL)
    {
        if(job_ptr->resource_object_list_ptr[i] == *obj_pptr)
        {
            ret_val = i;
            break;
        }
        i++;
    }

    return ret_val;
}

/*! \brief  Returns the reference of the job that is identified by the given resource object list.
 *  \param  self                        Instance pointer
 *  \param  resource_object_list[]      Reference to array of references to INIC resource objects
 *  \return Reference to the desired job if the job was found, otherwise NULL.
 */
Xrm_Job_t * Xrmp_GetJob(CXrmPool * self, UCS_XRM_CONST Ucs_Xrm_ResObject_t * resource_object_list[])
{
    uint8_t i;
    Xrm_Job_t *ret_ptr = NULL;

    for(i=0U; i<(uint8_t)XRM_NUM_JOBS; i++)
    {
        if(self->job_list[i].resource_object_list_ptr == resource_object_list)
        {
            ret_ptr = &self->job_list[i];
            break;
        }
        else if((self->job_list[i].resource_object_list_ptr == NULL) && (ret_ptr == NULL))
        {
            ret_ptr = &self->job_list[i];
        }
    }

    return ret_ptr;
}

/*! \brief  Calls the given function for each node in the resource list. If the func_ptr 
 *          returns true the loop is stopped.
 *  \param  self            Instance pointer
 *  \param  func_ptr        Reference of the callback function which is called for each node
 *  \param  user_data_ptr1  Reference of optional user data 1 pass to func_ptr
 *  \param  user_data_ptr2  Reference of optional user data 2 pass to func_ptr
 *  \param  user_data_ptr3  Reference of optional user data 3 pass to func_ptr
 */
void Xrmp_Foreach(CXrmPool *self, Xrmp_ForeachFunc_t func_ptr, void *user_data_ptr1, void *user_data_ptr2, void *user_data_ptr3)
{
    uint8_t j;

    for(j=0U; j<XRM_NUM_RESOURCE_HANDLES; j++)
    {
        if (self->resource_handle_list[j].job_ptr != NULL)
        {
            if (func_ptr(&self->resource_handle_list[j], user_data_ptr1, user_data_ptr2, user_data_ptr3) != false) 
            {
                break;
            }
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

