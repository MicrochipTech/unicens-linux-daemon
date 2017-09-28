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
 * \brief Implementation of the library module which contains miscellaneous helper functions.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_MISC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief UNICENS internal memset-function.
 *  \param dst_ptr Pointer to the block of memory to fill
 *  \param value   Value to be set
 *  \param size    Number of bytes to be set to the value
 */
void Misc_MemSet(void *dst_ptr, int32_t value, uint32_t size)
{
    uint8_t *dst_ptr_ = (uint8_t *)dst_ptr;
    uint32_t i;

    for(i=0U; i<size; i++)
    {
        dst_ptr_[i] = (uint8_t)value;   /* parasoft-suppress  MISRA2004-17_4 "void pointer required for memset-function signature (stdlib)" */
    }
}

/*! \brief UNICENS internal memcpy-function.
 *  \param dst_ptr Pointer to the destination array where the content is to be copied
 *  \param src_ptr Pointer to the source of data to be copied
 *  \param size    Number of bytes to copy
 */
void Misc_MemCpy(void *dst_ptr, void *src_ptr, uint32_t size)
{
    uint8_t *dst_ptr_ = (uint8_t *)dst_ptr;
    uint8_t *src_ptr_ = (uint8_t *)src_ptr;
    uint32_t i;

    for(i=0U; i<size; i++)
    {
        dst_ptr_[i] = src_ptr_[i];  /* parasoft-suppress  MISRA2004-17_4 "void pointers required for memcpy-function signature (stdlib)" */
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

