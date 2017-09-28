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
 * \brief Declaration of class CPmCommand
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_PM_CMD
 * @{
 */

#ifndef UCS_PMCMD_H
#define UCS_PMCMD_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"
#include "ucs_memory.h"
#include "ucs_lldpool.h"
#include "ucs_pmp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Class CPmCommand                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Class CPmCommand  */
typedef struct CPmCommand_
{
    Lld_IntTxMsg_t      tx_obj;     /*!< \brief  Required LLD Tx structure, must be first attribute */
    uint8_t             data[10];   /*!< \brief  Reserved memory space */
    Ucs_Mem_Buffer_t    memory;     /*!< \brief  Public memory structure */
    bool                reserved;   /*!< \brief  \c true if the command is in use, otherwise \c false. */
    bool                trigger;    /*!< \brief  \c true if the command is triggered, otherwise \c false. */

} CPmCommand;


/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Pmcmd_Ctor(CPmCommand *self, Pmp_FifoId_t fifo, Pmp_MsgType_t type);
extern Ucs_Lld_TxMsg_t* Pmcmd_GetLldTxObject(CPmCommand *self);
extern bool Pmcmd_Reserve(CPmCommand *self);
extern void Pmcmd_Release(CPmCommand *self);
extern void Pmcmd_SetContent(CPmCommand *self, uint8_t sid, uint8_t ext_type, 
                             uint8_t ext_code, uint8_t add_data_ptr[], uint8_t add_data_sz);
extern void Pmcmd_UpdateContent(CPmCommand *self, uint8_t sid, uint8_t ext_type, uint8_t ext_code);
extern void Pmcmd_SetTrigger(CPmCommand *self, bool trigger);
extern bool Pmcmd_IsTriggered(CPmCommand *self);


#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif /* #ifndef UCS_PMCMD_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

