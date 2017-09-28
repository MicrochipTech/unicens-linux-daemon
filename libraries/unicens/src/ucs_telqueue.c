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
 * \brief Implementation of class CTelQueue
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_MSG_QUEUE
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_telqueue.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of class CTelQueue
 *  \param  self            The instance
 *  \param  ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Telq_Ctor(CTelQueue *self, void *ucs_user_ptr)
{
    self->ucs_user_ptr = ucs_user_ptr;
    Dl_Ctor(&self->list, self->ucs_user_ptr);
}

/*! \brief  Retrieves the head object of the telegram queue
 *  \param  self    The instance
 *  \return Reference to the telegram if a telegram object is available.
 *          Otherwise \c NULL.
 */
Msg_MostTel_t* Telq_Dequeue(CTelQueue *self)
{
    Msg_MostTel_t *tel_ptr = NULL;
    CDlNode *node_ptr = Dl_PopHead(&self->list);

    if (node_ptr != NULL)
    {
        tel_ptr = (Msg_MostTel_t*)Dln_GetData(node_ptr);
    }

    return tel_ptr;
}

/*! \brief  Retrieves a reference to the head object 
 *          without removing it from the telegram queue
 *  \param  self    The instance
 *  \return Reference to the telegram if a telegram object is available.
 *          Otherwise \c NULL.
 */
Msg_MostTel_t* Telq_Peek(CTelQueue *self)
{
    Msg_MostTel_t *tel_ptr = NULL;
    CDlNode *node_ptr = Dl_PeekHead(&self->list);

    if (node_ptr != NULL)
    {
        tel_ptr = (Msg_MostTel_t*)Dln_GetData(node_ptr);
    }

    return tel_ptr;
}

/*! \brief  Adds a telegram to the tail of the queue
 *  \param  self    The instance
 *  \param  tel_ptr Reference to the telegram 
 */
void Telq_Enqueue(CTelQueue *self, Msg_MostTel_t *tel_ptr)
{
    Dl_InsertTail(&self->list, Msg_GetNode((CMessage*)(void*)tel_ptr));
}

/*! \brief  Retrieves the current number of objects in the telegram queue 
 *  \param  self    The instance
 *  \return The current number of available telegram objects in the pool
 */
uint8_t Telq_GetSize(CTelQueue *self)
{
    return (uint8_t)Dl_GetSize(&self->list);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

