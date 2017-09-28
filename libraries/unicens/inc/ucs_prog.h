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

#ifndef UCS_PROG_H
#define UCS_PROG_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

#define PRG_MAX_LEN_ERROR 3U

typedef struct Prg_Error_t_
{
    Ucs_Prg_ResCode_t code;
    Ucs_Prg_Func_t function;
    uint8_t ret_len;
    uint8_t parm[PRG_MAX_LEN_ERROR];
} Prg_Error_t;




/*! \brief   Structure of class CProgramming. */
typedef struct CProgramming_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    CSingleObserver prg_welcome;        /*!< \brief Observes the Welcome result */
    CSingleObserver prg_memopen;        /*!< \brief Observes the MemSessionOpen result */
    CSingleObserver prg_memwrite;       /*!< \brief Observes the MemoryWrite result */
    CSingleObserver prg_memclose;       /*!< \brief Observes the MemSessionClose result */

    CMaskedObserver prg_terminate;      /*!< \brief Observes events leading to termination */
    CObserver       prg_nwstatus;       /*!< \brief Observes the MOST Network status */

    CFsm            fsm;                /*!< \brief Node Discovery state machine  */
    CService        service;            /*!< \brief Service instance for the scheduler */
    CTimer          timer;              /*!< \brief timer for monitoring messages */
    bool            neton;              /*!< \brief indicates Network availability */

    uint16_t              node_id;              /*!< \brief Position address of the node to be programmed. */
    uint16_t              target_address;       /*!< \brief Actual target address */
    Ucs_Signature_t       signature;            /*!< \brief Signature of the node to be programmed. */
    Ucs_Prg_SessionType_t session_type;         /*!< \brief Defines the memory access type. */
    Ucs_Prg_Command_t*    command_list;         /*!< \brief Refers to array of programming tasks. */
    uint8_t               command_index;        /*!< \brief index for command_list */
    uint16_t              admin_node_address;   /*!< \brief Admin Node Address */
    Ucs_Prg_ReportCb_t    report_fptr;          /*!< \brief Report callback function */
    uint16_t              session_handle;       /*!< \brief Unique number used to authorize memory access. */
    Ucs_Prg_Func_t        current_function;     /*!< \brief last used function. */
    Prg_Error_t           error;                /*!< \brief stores the current error information */
}CProgramming;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Prg_Ctor(CProgramming *self, 
              CInic *inic, 
              CBase *base, 
              CExc *exc);

extern void Prg_Start(CProgramming *self,
                      uint16_t node_id, 
                      Ucs_Signature_t *signature,
                      Ucs_Prg_SessionType_t session_type, 
                      Ucs_Prg_Command_t* command_list, 
                      Ucs_Prg_ReportCb_t report_fptr);


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_PROG_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

