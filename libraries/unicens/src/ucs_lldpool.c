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
 * \brief Implementation of LLD Message Pool
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMF
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_lldpool.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Returns an unused LLD Tx message object back to the pool
 *  \param  self        The instance
 *  \param  owner_ptr   Assigns messages to the respective FIFO
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Lldp_Ctor(CLldPool *self, void *owner_ptr, void* ucs_user_ptr)
{
    uint8_t cnt;
    MISC_MEM_SET(self, 0, sizeof(*self));

    Dl_Ctor(&self->list, ucs_user_ptr);

    for (cnt = 0U; cnt < LLDP_NUM_HANDLES; cnt++)         /* setup LLD Tx handles */
    {
        TR_ASSERT(ucs_user_ptr, "[FIFO]", (self->messages[cnt].msg_ptr == NULL) );
        Dln_Ctor(&self->messages[cnt].node, &self->messages[cnt]);
        self->messages[cnt].owner_ptr = owner_ptr;
        Dl_InsertTail(&self->list, &self->messages[cnt].node);
    }
}

/*! \brief  Returns an unused LLD Tx message object back to the pool
 *  \param  self    The instance
 *  \param  msg_ptr The unused LLD Tx message object 
 */
void Lldp_ReturnTxToPool(CLldPool *self, Lld_IntTxMsg_t *msg_ptr)
{
    Dl_InsertTail(&self->list, &msg_ptr->node);
}

/*! \brief  Allocates an unused LLD Tx message object from the pool
 *  \param  self    The instance
 *  \return An internal LLD Tx message object or \c NULL if no message object is 
 *          available.
 */
Lld_IntTxMsg_t* Lldp_GetTxFromPool(CLldPool *self)
{
    CDlNode *node_ptr = NULL;
    Lld_IntTxMsg_t *handle_ptr = NULL;

    node_ptr = Dl_PopHead(&self->list);

    if (node_ptr != NULL)
    {
        handle_ptr = (Lld_IntTxMsg_t*)Dln_GetData(node_ptr);
    }

    return handle_ptr;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

