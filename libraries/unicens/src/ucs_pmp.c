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
 * \brief Implementation of Port Message Protocol
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PMH
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_pmp.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* PMP Indexes                                                                                    */
/*------------------------------------------------------------------------------------------------*/
#define PMP_IDX_PML_H           0U
#define PMP_IDX_PML_L           1U
#define PMP_IDX_PMHL            2U
#define PMP_IDX_FPH             3U
#define PMP_IDX_SID             4U
#define PMP_IDX_EXT_TYPE        5U

/*------------------------------------------------------------------------------------------------*/
/* Masks and shifts for bit fields                                                                */
/*------------------------------------------------------------------------------------------------*/
#define PMP_PMHL_MASK           0x1FU      /* 0b00011111 */
#define PMP_VERSION_MASK        0xE0U      /* 0b11100000 */
#define PMP_VERSION             0x40U      /* Version: "2" */
#define PMP_FPH_TYPE_POS        1U
#define PMP_FPH_TYPE_MASK       0x06U      /* 0b00000110 */
#define PMP_FPH_ID_POS          3U
#define PMP_FPH_ID_MASK         0x38U      /* 0b00111000 */
#define PMP_FPH_DIR_RX          0x01U      /* RX: "1" */
#define PMP_FPH_DIR_MASK        0x01U      /* 0b00000001 */
#define PMP_EXT_TYPE_POS        5U
#define PMP_EXT_TYPE_MASK       0xE0U      /* 0b11100000 */
#define PMP_EXT_CODE_MASK       0x1FU      /* 0b00011111 */

/*------------------------------------------------------------------------------------------------*/
/* PMP Verification                                                                               */
/*------------------------------------------------------------------------------------------------*/
#define PMP_PML_MAX_SIZE_CTRL   69U
#define PMP_PMHL_MIN_SIZE       3U
#define PMP_PMHL_MAX_SIZE       5U

/*------------------------------------------------------------------------------------------------*/
/* Macro like functions                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Sets the port message length within a given message header
 *  \param header The message header 
 *  \param length The port message length
 */
void Pmp_SetPml(uint8_t header[], uint8_t length)
{
    header[PMP_IDX_PML_H] = 0U;
    header[PMP_IDX_PML_L] = length;
}

/*! \brief Sets the port message header length within a given message header
 *  \param header The message header 
 *  \param length The port message header length. Valid values: 3..5.
 *                Invalid values will set the PMHL to \c 0.
 */
void Pmp_SetPmhl(uint8_t header[], uint8_t length)
{
    if ((length < PMP_PMHL_MIN_SIZE) || (length > PMP_PMHL_MAX_SIZE))
    {
        length = 0U;
    }

    header[PMP_IDX_PMHL] = length | PMP_VERSION;
}

/*! \brief Sets the FIFO protocol header within a given message header
 *  \param header The message header 
 *  \param id   The FIFO id
 *  \param type The port message type
 */
void Pmp_SetFph(uint8_t header[], Pmp_FifoId_t id, Pmp_MsgType_t type)
{
    header[PMP_IDX_FPH] = (uint8_t)((uint8_t)type << PMP_FPH_TYPE_POS) | (uint8_t)((uint8_t)id << PMP_FPH_ID_POS) | (uint8_t)PMP_DIR_TX;
}

/*! \brief Sets the field ExtType within a given message header
 *  \param header The message header 
 *  \param type   The command or status type
 *  \param code   The command or status code
 */
void Pmp_SetExtType(uint8_t header[], uint8_t type, uint8_t code)
{
    header[PMP_IDX_EXT_TYPE] = (uint8_t)((type << PMP_EXT_TYPE_POS) & PMP_EXT_TYPE_MASK) | (uint8_t)(code & PMP_EXT_CODE_MASK);
}

/*! \brief Sets the sequence id within a given message header
 *  \param header The message header 
 *  \param sid    The sequence id
 */
void Pmp_SetSid(uint8_t header[], uint8_t sid)
{
    header[PMP_IDX_SID] = sid;
}

/*! \brief  Retrieves the port message length of a given port message buffer 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \return The port message length in bytes or 0 if the PML is greater than 255.
 */
uint8_t Pmp_GetPml(uint8_t header[])
{
    uint8_t pml;
    if (header[PMP_IDX_PML_H] != 0U)
    {
        pml = 0U;
    }
    else
    {
        pml = header[PMP_IDX_PML_L];
    }

    return pml;
}

/*! \brief  Retrieves the port message header length of a given port message buffer 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \return The port message header length in bytes
 */
uint8_t Pmp_GetPmhl(uint8_t header[])
{
    return ((uint8_t)(header[PMP_IDX_PMHL] & (uint8_t)PMP_PMHL_MASK));
}

/*! \brief  Retrieves the FIFO number of a given port message buffer 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \return The FIFO number
 */
Pmp_FifoId_t Pmp_GetFifoId(uint8_t header[])
{
    return (Pmp_FifoId_t)(((uint8_t)PMP_FPH_ID_MASK & (header)[PMP_IDX_FPH]) >> PMP_FPH_ID_POS);
}

/*! \brief  Retrieves the FIFO Type of a given port message buffer 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \return The FIFO type
 */
Pmp_MsgType_t Pmp_GetMsgType(uint8_t header[])
{
    return ((Pmp_MsgType_t)((PMP_FPH_TYPE_MASK & (header)[PMP_IDX_FPH]) >> PMP_FPH_TYPE_POS));
}

/*! \brief  Retrieves the SequenceID of a given port message buffer 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \return The SequenceID
 */
uint8_t Pmp_GetSid(uint8_t header[])
{
    return ((header)[PMP_IDX_SID]);
}

/*! \brief  Retrieves payload data of a port message
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \param  index   The index of the payload byte starting with '0'
 *  \return The content of a payload data byte
 */
uint8_t Pmp_GetData(uint8_t header[], uint8_t index)
{
    return header[Pmp_GetPmhl(header) + 3U + index];
}

/*! \brief   Retrieves the payload size of the port message
 *  \param   header  Data buffer containing the port message.
 *                   The required size of this buffer is 6 bytes.
 *  \details The function Pmp_VerifyHeader() must be called before
 *           to verify that the port message fields are consistent.
 *  \return  The payload size of a port message
 */
uint8_t Pmp_GetDataSize(uint8_t header[])
{
    return Pmp_GetPml(header) - (Pmp_GetPmhl(header) + 1U);
}

/*! \brief  Checks if header length fields are set to valid values 
 *  \param  header  Data buffer containing the port message.
 *                  The required size of this buffer is 6 bytes.
 *  \param  buf_len Length of the complete port message in bytes
 *  \return Returns \c true if the header was checked successfully,
 *          otherwise \c false.
 */
bool Pmp_VerifyHeader(uint8_t header[], uint8_t buf_len)
{
    uint8_t pml = Pmp_GetPml(header);
    uint8_t pmhl = Pmp_GetPmhl(header);
    bool ok = true;

    ok = ((pmhl >= 3U)&&(pmhl <= 5U))       ? ok : false;
    ok = ((header[PMP_IDX_PMHL] & PMP_VERSION_MASK) == PMP_VERSION) ? ok : false;
    ok = (pml >= (pmhl + 1U))               ? ok : false;
    ok = ((pml + 2U) <= buf_len)            ? ok : false;
    ok = (pml <= PMP_PML_MAX_SIZE_CTRL)     ? ok : false;
    ok = ((header[PMP_IDX_FPH] & PMP_FPH_DIR_MASK) == PMP_FPH_DIR_RX) ? ok : false;

    return ok;
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Creates a Port Message Header instance 
 *  \param  self  The instance
 */
void Pmh_Ctor(CPmh *self)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
}

/*! \brief  Inserts a port message header of the specified size into a given buffer 
 *  \param  self        Header content to be written to the buffer (source)
 *  \param  data        Data buffer the header shall be written to (target)
 */
void Pmh_BuildHeader(CPmh *self, uint8_t data[])
{
    uint8_t cnt;

    data[PMP_IDX_PML_H]   = 0U;
    data[PMP_IDX_PML_L]   = (uint8_t)self->pml;
    data[PMP_IDX_PMHL]    = (uint8_t)PMP_VERSION | self->pmhl;
    data[PMP_IDX_FPH]     = (uint8_t)PMP_DIR_TX | ((uint8_t)((self->fifo_id) << PMP_FPH_ID_POS)) |
                            ((uint8_t)((self->msg_type) << PMP_FPH_TYPE_POS));

    data[PMP_IDX_SID]     = self->sid;
    data[PMP_IDX_EXT_TYPE]= self->ext_type;

    for (cnt=3U; cnt < self->pmhl; cnt++)
    {
        data[3U + cnt] = 0U;                        /* add stuffing bytes */
    }
}

/*! \brief  Decodes a given data buffer into a provided port message header structure 
 *  \param  self        Header content structure (target)
 *  \param  data        Data buffer containing the port message with a minimum size
 *                      of 6 bytes (source)
 */
void Pmh_DecodeHeader(CPmh *self, uint8_t data[])
{
    self->pml = Pmp_GetPml(data);
    self->pmhl  = data[PMP_IDX_PMHL] & PMP_PMHL_MASK;        /* ignore version */

    self->fifo_id = Pmp_GetFifoId(data);
    self->msg_type = Pmp_GetMsgType(data);
    self->sid   = data[PMP_IDX_SID];
    self->ext_type = data[PMP_IDX_EXT_TYPE];
}

/*! \brief      Setter function for FIFO protocol header which contains several subfields
 *  \details    The "retransmitted" flag is currently not supported (always Tx)
 *  \param      self        Reference to the PM content structure that shall be modified
 *  \param      fifo_id     Id of the PM FIFO
 *  \param      msg_type    PM type
 */
void Pmh_SetFph(CPmh *self, Pmp_FifoId_t fifo_id, Pmp_MsgType_t msg_type)
{
    self->msg_type = msg_type;
    self->fifo_id = fifo_id;
}

/*! \brief  Retrieves the ExtType StatusType
 *  \param  self    The instance
 *  \return Returns The Status Type
 */
Pmp_StatusType_t Pmh_GetExtStatusType(CPmh *self)
{
    return ((Pmp_StatusType_t)((uint8_t)(PMP_EXT_TYPE_MASK & self->ext_type) >> PMP_EXT_TYPE_POS));
}

/*! \brief  Retrieves the ExtType StatusCode
 *  \param  self    The instance
 *  \return Returns The Status Code
 */
Pmp_CommandCode_t Pmh_GetExtCommandCode(CPmh *self)
{
    return ((Pmp_CommandCode_t)(uint8_t)(PMP_EXT_CODE_MASK & self->ext_type));
}

/*! \brief  Retrieves the ExtType StatusType
 *  \param  self    The instance
 *  \return Returns The Status Type
 */
Pmp_CommandType_t Pmh_GetExtCommandType(CPmh *self)
{
    return ((Pmp_CommandType_t)((uint8_t)(PMP_EXT_TYPE_MASK & self->ext_type) >> PMP_EXT_TYPE_POS));
}

/*! \brief  Retrieves the ExtType StatusCode
 *  \param  self    The instance
 *  \return Returns The Status Code
 */
Pmp_StatusCode_t Pmh_GetExtStatusCode(CPmh *self)
{
    return ((Pmp_StatusCode_t)(uint8_t)(PMP_EXT_CODE_MASK & self->ext_type));
}

/*! \brief      Sets the ExtType field by passing the values for type and code 
 *  \details    The function is applicable for status and command
 *  \param      self    The Instance
 *  \param      type    The status or command type
 *  \param      code    The status or command code
 */
void Pmh_SetExtType(CPmh *self, uint8_t type, uint8_t code)
{
    self->ext_type = (uint8_t)((type << PMP_EXT_TYPE_POS) & PMP_EXT_TYPE_MASK) | (uint8_t)(code & PMP_EXT_CODE_MASK);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

