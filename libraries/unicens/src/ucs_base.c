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
 * \brief Implementation of the Base class.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_BASE
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_misc.h"
#include "ucs_message.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CBase                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of the Base class.
 *  \param  self        Instance pointer
 *  \param  init_ptr    Reference to the initialization data
 */
void Base_Ctor(CBase *self, Base_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    /* Save instance ID and user pointer */
    self->ucs_inst_id = init_ptr->ucs_inst_id;
    self->ucs_user_ptr = init_ptr->ucs_user_ptr;
    /* Create the scheduler instance */
    Scd_Ctor(&self->scd, &init_ptr->scd, init_ptr->ucs_user_ptr);
    /* Create the timer management instance */
    Tm_Ctor(&self->tm, &self->scd, &init_ptr->tm, init_ptr->ucs_user_ptr);
    /* Create the event handler instance */
    Eh_Ctor(&self->eh, init_ptr->ucs_user_ptr);
    /* Create the API locking manager instance */
    Alm_Ctor(&self->alm, &self->tm, &self->eh, init_ptr->ucs_user_ptr);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

