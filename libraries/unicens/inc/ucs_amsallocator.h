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
 * \brief Internal header file of AMS Allocator Interface
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMSC
 * @{
 */

#ifndef UCS_AMSALLOCATOR_H
#define UCS_AMSALLOCATOR_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Defines the usage of a requested memory chunk */
typedef enum Ams_MemUsage_
{
    AMS_MU_RX_OBJECT,            /*!< \brief  Memory is required to allocate an Rx message object */
    AMS_MU_RX_PAYLOAD,           /*!< \brief  Memory is required to allocate Rx message payload */
    AMS_MU_TX_OBJECT,            /*!< \brief  Memory is required to allocate a Tx message object */
    AMS_MU_TX_PAYLOAD            /*!< \brief  Memory is required to allocate Tx message payload */

} Ams_MemUsage_t;

/*! \brief  Callback function type that is invoked to allocate external payload for a segmented Rx message
 *  \param  inst_ptr            Reference to the (external) memory management
 *  \param  mem_size            Reference to the required memory size in bytes. Valid values: 0..65535.
 *  \param  type                Declares how the memory is used by UNICENS
 *  \param  custom_info_pptr    Reference which is related to the memory chunk and can be set by 
 *                              the application.
 *  \return Pointer to the provided memory chunk. The application has to guarantee that the memory size
 *          is equal or greater than \c mem_size. The application has to return \c NULL if it is not able 
 *          to allocate the required memory at this moment.
 */
typedef void* (*Ams_AllocMemCb_t)(void *inst_ptr, uint16_t mem_size, Ams_MemUsage_t type, void** custom_info_pptr);

/*! \brief  Callback function type that is invoked to free external payload for a segmented Rx message
 *  \param  inst_ptr        Reference to the (external) memory management
 *  \param  mem_ptr         Reference to the external payload memory
 *  \param  type            Declares how the memory is used by UNICENS
 *  \param  custom_info_ptr Reference to memory related information which was set by the application 
 *                          during memory allocation
 */
typedef void (*Ams_FreeMemCb_t)(void *inst_ptr, void *mem_ptr, Ams_MemUsage_t type, void* custom_info_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Allocator interface                                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Keeps callback functions to an external memory management for Rx payload  */
typedef struct Ams_MemAllocator_
{
    void *inst_ptr;                /*!< \brief The instance of the (external) memory management */
    Ams_AllocMemCb_t alloc_fptr;   /*!< \brief This function is invoked to allocate Rx user payload */
    Ams_FreeMemCb_t  free_fptr;    /*!< \brief This function is invoked to free Rx user payload */

} Ams_MemAllocator_t;

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_AMSALLOCATOR_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

