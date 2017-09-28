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
 * \brief Implementation of the Command Interpreter Module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DEC_INT
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_dec.h"
#include "ucs_misc.h"
#include "ucs_ret_pb.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Search in a FktOp table for matching FktID and OPType. This function is used for
 *          incoming ICM messages.
 *  \param   list            FktOp table
 *  \param   index_ptr       Reference to array index of the matching array element
 *  \param   function_id     FktID
 *  \param   op_type         OPType
 *  \return  DEC_RET_SUCCESS                 Decoding was successful
 *           DEC_RET_FKTID_NOT_FOUND         FktID/OPType not found
 */
Dec_Return_t Dec_SearchFktOpIcm(Dec_FktOpIcm_t const list[], uint16_t *index_ptr, 
                                uint16_t function_id, Ucs_OpType_t op_type)
{
    uint16_t     fktop;
    uint16_t     i       = 0U;
    Dec_Return_t ret_val = DEC_RET_FKTID_NOT_FOUND;
    bool         loop    = true;

    fktop = DEC_FKTOP(function_id, op_type);
    *index_ptr = 0U;

    while ((list[i].handler_function_ptr != NULL)  && (loop != false))
    {
        if(list[i].fkt_op == fktop)
        {
            ret_val = DEC_RET_SUCCESS;
            *index_ptr = i;
            loop = false;
        }
        else if (list[i].fkt_op > fktop)
        {
            loop = false;
        }
        else
        {
            i++;
        }
    }

    return ret_val;
}

/*! \brief  Search in a FktOp table for matching FktID and OPType. This function is used for
 *          MCM messages coming from FBlocks inside the INIC.
 *  \param   list            FktOp table
 *  \param   index_ptr       Reference to array index of the matching array element
 *  \param   function_id     FktID
 *  \param   op_type         OPType
 *  \return  DEC_RET_SUCCESS                 Decoding was successful
 *           DEC_RET_FKTID_NOT_FOUND         FktID/OPType not found
 */
Dec_Return_t Dec_SearchFktOpIsh(Dec_FktOpIsh_t const list[], uint16_t *index_ptr, 
                                uint16_t function_id, Ucs_OpType_t op_type)
{
    uint16_t     fktop;
    uint16_t     i       = 0U;
    Dec_Return_t ret_val = DEC_RET_FKTID_NOT_FOUND;
    bool         loop    = true;

    fktop = DEC_FKTOP(function_id, op_type);
    *index_ptr = 0U;

    while ((list[i].handler_function_ptr != NULL)  && (loop != false))
    {
        if(list[i].fkt_op == fktop)
        {
            ret_val = DEC_RET_SUCCESS;
            *index_ptr = i;
            loop = false;
        }
        else if (list[i].fkt_op > fktop)
        {
            loop = false;
        }
        else
        {
            i++;
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

