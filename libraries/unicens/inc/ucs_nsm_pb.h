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
 * \brief Public header file of the Node Script Management.
 */

#ifndef UCS_NSM_PB_H
#define UCS_NSM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_rm_pv.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes of the Node Script Management. 
 *  \ingroup G_UCS_SCRIPTING
 */
typedef enum Ucs_Ns_ResultCode_
{
    UCS_NS_RES_SUCCESS     = 0x00U,     /*!< \brief Transmission of script(s) was successful. */
    UCS_NS_RES_ERROR       = 0x01U      /*!< \brief Transmission of script(s) failed. */

} Ucs_Ns_ResultCode_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Structure of a ConfigMsg used in Node-Script.
 *  \ingroup G_UCS_SCRIPTING
 */
typedef struct Ucs_Ns_ConfigMsg_
{
    /*! \brief FBlockId of the config msg. */
    uint8_t FBlockId;
    /*! \brief InstId of the config msg. */
    uint8_t InstId;
    /*! \brief FunktId of the config msg. */
    uint16_t FunktId;
    /*! \brief OpCode of the config msg. */
    uint8_t OpCode;
    /*! \brief Data length. */
    uint8_t DataLen;
    /*! \brief Reference to the Data */
    uint8_t * DataPtr;

} Ucs_Ns_ConfigMsg_t;

/*! \brief Structure of a node-script used to configure a remote node.
 *  \attention The Node Scripting module is designed and intended for the use of \b I2C and \b GPIO commands only. That is, using the Scripting for any other FBlock INIC commands 
 *  (for example MOST, MediaLB, USB, Streaming, Connections, etc.) is expressly \b prohibited.
 *  \ingroup G_UCS_SCRIPTING
 */
typedef struct Ucs_Ns_Script_
{
    /*! \brief Specifies the pause which shall be set before sending
     *         the configuration message. 
     */
    uint16_t pause;
    /*! \brief Command to be transmitted. */
    Ucs_Ns_ConfigMsg_t * send_cmd;
    /*! \brief Expected result. */
    Ucs_Ns_ConfigMsg_t * exp_result;

} Ucs_Ns_Script_t;

/*! \brief Configuration structure of a Node. 
 *
 *  \attention Use the \ref UCS_ADDR_LOCAL_DEV macro to address your local device when specifying routes to/from It.  
 *             \n The following address ranges are supported:
 *                 - [0x10  ... 0x2FF]
 *                 - [0x500 ... 0xFEF]
 *                 - UCS_ADDR_LOCAL_DEV
 *  \ingroup G_UCS_ROUTING_TYPES
 */
typedef struct Ucs_Rm_Node_
{
    /*! \brief node signature. */
    Ucs_Signature_t * signature_ptr;
    /*! \brief Reference to a list of configuration scripts. */
    Ucs_Ns_Script_t * script_list_ptr;
    /*! \brief size of the scripts table. */
    uint8_t script_list_size;
    /*! \brief Internal information of this node object. */
    Ucs_Rm_NodeInt_t internal_infos;

} Ucs_Rm_Node_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for the results of the Scripting Manager.
 *  \param  node_ptr          Reference to the node instance
 *  \param  result            Result of the scripting operation.
 *  \param  ucs_user_ptr      User reference for API callback functions.
 *  \ingroup G_UCS_SCRIPTING
 */
typedef void (*Ucs_Ns_ResultCb_t)(Ucs_Rm_Node_t * node_ptr, Ucs_Ns_ResultCode_t result, void *ucs_user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_NSM_PB_H */
/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

