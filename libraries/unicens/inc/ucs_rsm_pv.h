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
 * \brief Public header file of the Extended Resource Manager.
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_RSM
 * @{
 */

#ifndef UCS_RSM_PB_H
#define UCS_RSM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_message_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief SyncLost Causes 
 *  \ingroup G_UCS_RSM_TYPES
 */
typedef enum Rsm_SyncLostCause_
{
    RSM_SLC_NWSHUTDOWN,    /*!< \brief "Network Shutdown" causes the SyncLost */
    RSM_SLC_CFGNOTOK,      /*!< \brief "Config Not Ok" causes the SyncLost */
    RSM_SLC_SYSMODIF       /*!< \brief "System Changes like own node address or MPR changes" cause the SyncLost */

} Rsm_SyncLostCause_t;

/*! \brief  RSM Sync states 
 *  \ingroup G_UCS_RSM_TYPES
 */
typedef enum Rsm_DevSyncState_
{
    RSM_DEV_UNSYNCED,  /*!< \brief RSM device is "Unsynced", i.e. not in remote control mode */
    RSM_DEV_SYNCING,   /*!< \brief RSM device is "Synching" */
    RSM_DEV_SYNCED     /*!< \brief RSM device is "Synced", i.e. in remote control mode */

} Rsm_DevSyncState_t;

/*! \brief Result codes of the Extended Resource Manager. 
 *  \ingroup G_UCS_RSM_TYPES
 */
typedef enum Rsm_ResultCode_
{
    RSM_RES_SUCCESS,          /*!< \brief Device Sync succeeded */
    RSM_RES_ERR_SYNC          /*!< \brief Device Sync failed because of not Remote Control Mode */

} Rsm_ResultCode_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Detailed information used for Extended Resource Manager results. */
typedef struct Rsm_ResultDetails_
{
    /*! \brief Holds the status of the transmission. */
    Ucs_MsgTxStatus_t tx_result;
    /*! \brief Holds the results of the target device. */
    Ucs_StdResult_t inic_result;

} Rsm_ResultDetails_t;

/*! \brief Result structure of the Extended Resource Manager. */
typedef struct Rsm_Result_
{
    /*! \brief Result code. */
    Rsm_ResultCode_t code;
    /*! \brief Detailed information on the returned result. */
    Rsm_ResultDetails_t details;

} Rsm_Result_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function type to retrieve a RSM result 
 *  \param  self          The instance (optional)
 *  \param  result        The result message object
 *  \ingroup G_UCS_RSM_TYPES
 */
typedef void (*Rsm_ResultCb_t)(void * self, Rsm_Result_t result);

/*!
 * @}
 * \endcond
 */

/*!
 *  \def     UCS_NUM_REMOTE_DEVICES
 *  \brief   Customer assignment for number of remote devices required by Resources Management modules.
 *  \details If the macro is not defined, the UNICENS library will use a default value of 0. The user 
 *           can overwrite this default value by defining the macro. Valid values are in the range 
 *           from 0 to 63. 
  *  \ingroup G_UCS_XRM_CFG
 */ 
#ifndef UCS_NUM_REMOTE_DEVICES
#    define UCS_NUM_REMOTE_DEVICES   0U
#endif

/*! \def UCS_ADDR_LOCAL_DEV
 *  \brief      Defines the address of the local device.
 *  \details    This macro is used to define the address of the local device. It should be used by 
 *              the application to trigger jobs on the local device. 
 *  \ingroup    G_UCS_IRM
 */
#define UCS_ADDR_LOCAL_DEV    0x0001U

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_RSM_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

