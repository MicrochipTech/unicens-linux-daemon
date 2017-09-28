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
 * \brief Declaration of message encoder
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_ENCODER
 * @{
 */

#ifndef UCS_ENCODER_H
#define UCS_ENCODER_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_message.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Defines                                                                                        */
/*------------------------------------------------------------------------------------------------*/
#define ENC_MAX_SIZE_CONTENT    16U     /*!< \brief Maximum content size in bytes, quadlet aligned */

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Retrieves the size of a MOST message header
 *  \return The size of the MOST message header in bytes.
 */
typedef uint8_t (*Enc_GetSize_t)(void);

/*! \brief  Retrieves the content type of a MOST message header
 *  \return The content type of the MOST message header in bytes.
 */
typedef uint8_t (*Enc_GetContType_t)(void);

/*! \brief  Encodes a message telegram to the MOST message header 
 *  \param  tel_ptr     Reference to the Msg_MostTel_t structure 
 *  \param  header      The header buffer 
 */
typedef void    (*Enc_Encode_t)(Msg_MostTel_t *tel_ptr, uint8_t header[]);

/*! \brief  Decodes a MOST message header to a message telegram structure 
 *  \param  tel_ptr     Reference to the Msg_MostTel_t structure 
 *  \param  header      The header buffer 
 */
typedef void    (*Enc_Decode_t)(Msg_MostTel_t *tel_ptr, uint8_t header[]);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Identifier for a MOST Message Content */
typedef enum Enc_MsgContent_
{
    ENC_CONTENT_00 = 0x00,  /*!< \brief Content Type "0x00": Uncompressed, excluding retry values */
    ENC_CONTENT_80 = 0x80,  /*!< \brief Content Type "0x80": Compressed, excluding retry values */
    ENC_CONTENT_81 = 0x81   /*!< \brief Content Type "0x81": Compressed, including retry values */

} Enc_MsgContent_t;

/*! \brief      Interface for message encoder */
typedef struct IEncoder_
{
    Enc_MsgContent_t    content_type;   /*!< \brief Retrieves the content type of the MOST message header */
    uint8_t             pm_hdr_sz;      /*!< \brief Retrieves the size of the Port Message header */
    uint8_t             msg_hdr_sz;     /*!< \brief Retrieves the size of the MOST message header */
    Enc_Encode_t        encode_fptr;    /*!< \brief Function required to encode a MOST message header */
    Enc_Decode_t        decode_fptr;    /*!< \brief Function required to decode a MOST message header */

} IEncoder;

/*------------------------------------------------------------------------------------------------*/
/* Function prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
extern IEncoder *Enc_GetEncoder(Enc_MsgContent_t type);

#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_ENCODER_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

