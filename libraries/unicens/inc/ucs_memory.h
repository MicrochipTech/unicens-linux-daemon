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
 * \brief Declaration of internal memory buffer
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_MEMORY
 * @{
 */

#ifndef UCS_MEMORY_H
#define UCS_MEMORY_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_memory_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* IAllocator Types                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Callback function which frees memory 
 *  \param      allocator       Reference to the Mem_Allocator_t object
 *  \param      mem_ptr         Reference to memory chunk 
 *  \param      mem_info_ptr    Customer specific information needed to free 
 *                              the related memory chunk
 */
typedef void  (*Mem_Free_t)(void *allocator, void* mem_ptr, void* mem_info_ptr);

/*! \brief      Callback function which allocated memory 
 *  \param      allocator       Reference to the Mem_Allocator_t object
 *  \param      size            Size of the demanded memory chunk 
 *  \param      mem_info_ptr    Customer specific information needed to free 
 *                              the related memory chunk
 *  \return     Reference to a memory chunk with a minimum size of \c size. 
 *              Otherwise NULL.
 */
typedef void* (*Mem_Allocate_t)(void *allocator, uint16_t size, void** mem_info_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Interface IAllocator                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Interface which is needed to be implemented by a memory allocator */
typedef struct IAllocator_
{
    void*              base;            /*!< Reference to the base class */
    Mem_Allocate_t     allocate_fptr;   /*!< Callback function required to allocate memory */
    Mem_Free_t         free_fptr;       /*!< Callback function required to free memory */

} IAllocator;


/*------------------------------------------------------------------------------------------------*/
/* Memory buffer                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Memory chunk comprising non public fields */
typedef struct Mem_IntBuffer_
{
    Ucs_Mem_Buffer_t   public_buffer;  /*!< \brief      Public attributes of memory buffer
                                        *   \details    This has to be the first member in this 
                                        *               struct
                                        */
    IAllocator        *allocator_ptr;  /*!< \brief      Reference to the allocator which is 
                                        *               required to free the memory chunk 
                                        */
    void              *mem_info_ptr;   /*!< \brief      Customer specific information needed to 
                                        *               free the related memory chunk
                                        */
} Mem_IntBuffer_t;

#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_MEMORY_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

