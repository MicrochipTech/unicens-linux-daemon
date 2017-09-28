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
 * \brief Implementation of the Command Interpreter.
 *
 * \cond UCS_INTERNAL_DOC
 *
 * \addtogroup  G_UCS_CMD_INT
 * @{
 */


/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_cmd.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/


static Ucs_Cmd_Return_t Cmd_SearchMsgId(Ucs_Cmd_MsgId_t msg_id_tab[], uint16_t *index_ptr, 
                                               uint16_t message_id);


/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
void Cmd_Ctor(CCmd *self, CBase *base_ptr)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));                 /* reset members to "0" */

    self->msg_id_tab_ptr = NULL;
    self->ucs_user_ptr   = base_ptr->ucs_user_ptr;
}


/*! \brief  Add a MessageId Table to the Command Interpreter.
 *  \param  self            Instance pointer
 *  \param  msg_id_tab_ptr    Reference to a MessageId Table
 *  \return  Possible return values are shown in the table below.
 *  Value                           | Description 
 *  ------------------------------- | ------------------------------------
 *  UCS_CMD_RET_SUCCESS             | MessageId Table was successfully added
 *  UCS_CMD_RET_ERR_ALREADY_ENTERED | MessageId Table already added 
 */
Ucs_Cmd_Return_t Cmd_AddMsgIdTable(CCmd *self, Ucs_Cmd_MsgId_t *msg_id_tab_ptr)
{
    Ucs_Cmd_Return_t ret_val = UCS_CMD_RET_SUCCESS;


    if (self->msg_id_tab_ptr != NULL)
    {
        ret_val = UCS_CMD_RET_ERR_ALREADY_ENTERED;
    }
    else
    {
        self->msg_id_tab_ptr = msg_id_tab_ptr;
    }

    return ret_val;
}

/*! \brief   Remove an MessageId Table from the Command Interpreter.
 *  \param   self  Instance pointer of Cmd
 *  \return  Possible return values are shown in the table below.
 *  Value                        | Description 
 *  ---------------------------- | ------------------------------------
 * UCS_CMD_RET_SUCCESS           | MessageId Table was successfully removed
 */
Ucs_Cmd_Return_t Cmd_RemoveMsgIdTable(CCmd *self)
{
    Ucs_Cmd_Return_t ret_val = UCS_CMD_RET_SUCCESS;

    self->msg_id_tab_ptr = NULL;

    return ret_val;
}


/*! \brief  Decode an MCM message 
 *  \param  self            Instance pointer
 *  \param  msg_rx_ptr      Pointer to the message to decode
 *  \return  Possible return values are shown in the table below.
 *  Value                            | Description 
 *  -------------------------------- | ------------------------------------
 *  UCS_CMD_RET_SUCCESS              | decoding was successful
 *  UCS_CMD_RET_ERR_MSGID_NOTAVAIL   | MessageId not found 
 *  UCS_CMD_RET_ERR_TX_BUSY          | no Tx Buffer available
 *  UCS_CMD_RET_ERR_APPL             | error happened in handler function
 *  UCS_CMD_RET_ERR_NULL_PTR         | No MessageId Table available
 */
Ucs_Cmd_Return_t Cmd_DecodeMsg(CCmd *self, Ucs_AmsRx_Msg_t *msg_rx_ptr)
{
    Ucs_Cmd_Return_t result = UCS_CMD_RET_SUCCESS;
    uint16_t         index;

    result = Cmd_SearchMsgId(self->msg_id_tab_ptr, &index, msg_rx_ptr->msg_id);

    if (result == UCS_CMD_RET_SUCCESS)
    {
        /* call handler function */
        result = (Ucs_Cmd_Return_t)(self->msg_id_tab_ptr[index].handler_function_ptr(msg_rx_ptr, self->ucs_user_ptr));
    }

    return result;
}


/*! \brief  Search in a MessageId Table for matching MessageId
 *  \details Function expects that the MessageId Table ends with a termination entry 
 *           (handler_function_ptr == NULL). If this entry is not present, the search may end in an 
 *           endless loop. 
 *  \param   msg_id_tab         MessageId Table
 *  \param   index_ptr          pointer to the matching element
 *  \param   message_id         MessageId
 *  \return  Possible return values are shown in the table below.
 *  Value                           | Description 
 *  ------------------------------- | ------------------------------------
 *  UCS_CMD_RET_SUCCESS             | decoding was successful
 *  UCS_CMD_RET_ERR_MSGID_NOTAVAIL  | MessageId not found
 *  UCS_CMD_RET_ERR_NULL_PTR        | No MessageId Table available
 */
static Ucs_Cmd_Return_t Cmd_SearchMsgId(Ucs_Cmd_MsgId_t msg_id_tab[], uint16_t *index_ptr, 
                                        uint16_t message_id)
{
    Ucs_Cmd_Return_t ret_val = UCS_CMD_RET_SUCCESS;
    uint16_t i = 0U;

    if (msg_id_tab == NULL)
    {
        ret_val = UCS_CMD_RET_ERR_NULL_PTR;
    }
    else
    {
        while (msg_id_tab[i].handler_function_ptr != NULL)        /* last entry */
        {
            if (msg_id_tab[i].msg_id != message_id)
            {
                ++i;                                        /* goto next list element */
            }
            else
            {
                *index_ptr   = i;
                break;
            }
        }

        if (msg_id_tab[i].handler_function_ptr == NULL)               /* no match found */
        {
            ret_val = UCS_CMD_RET_ERR_MSGID_NOTAVAIL;
        }
    }
    return ret_val;
}

/*!
 * @}
 * \endcond
 */





/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

