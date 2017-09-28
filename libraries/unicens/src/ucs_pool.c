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
 * \brief Implementation of message pool class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_POOL
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pool.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of message pool class
 *  \param  self            The instance
 *  \param  messages        Reference to an array of message objects
 *  \param  size            Number of message objects the \c messages array is comprising.
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Pool_Ctor(CPool *self, CMessage messages[], uint16_t size, void *ucs_user_ptr)
{
    uint16_t index;

    MISC_MEM_SET(self, 0, sizeof(*self));
    self->ucs_user_ptr = ucs_user_ptr;
    self->initial_size = size;
    self->messages = messages;

    Dl_Ctor(&self->message_list, self->ucs_user_ptr); 

    for (index = 0U; index < size; index++)
    {
        Msg_Ctor(&messages[index]);
        Msg_SetPoolReference(&messages[index], self);
        Dl_InsertTail(&self->message_list, Msg_GetNode(&messages[index]));
    }
}

/*! \brief  Retrieves a message object from the pool
 *  \param  self    The instance
 *  \return Reference to the CMessage structure if a message is available.
 *          Otherwise \c NULL.
 */
CMessage* Pool_GetMsg(CPool *self)
{
    CMessage *msg = NULL;
    CDlNode *node = Dl_PopHead(&self->message_list);

    if (node != NULL)
    {
        msg = (CMessage*)node->data_ptr;
    }

    return msg;
}

/*! \brief  Returns a message object to the pool pre-assigned pool
 *  \param  msg_ptr Reference to the message object which needs
 *                  to be returned to the pool. 
 */
void Pool_ReturnMsg(CMessage *msg_ptr)
{
    CPool *pool_ptr = (CPool*)Msg_GetPoolReference(msg_ptr);

    if (pool_ptr != NULL)
    {
        TR_ASSERT(pool_ptr->ucs_user_ptr, "[POOL]", (Pool_GetCurrentSize(pool_ptr) < pool_ptr->initial_size));
        Dl_InsertTail(&pool_ptr->message_list, Msg_GetNode(msg_ptr));
    }
    else
    {
        TR_ERROR((0U, "[POOL]", "Pool_ReturnMsg(): released msg_ptr=0x%p without pool reference", 1U, msg_ptr));
    }
}

/*! \brief  Retrieves the current number of available message objects in the pool 
 *  \param  self    The instance
 *  \return The current number of available message objects in the pool
 */
uint16_t Pool_GetCurrentSize(CPool *self)
{
    uint16_t list_size = Dl_GetSize(&self->message_list);

    return list_size;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

