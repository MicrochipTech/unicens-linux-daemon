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
 * \brief Internal header file of the Node Scripting Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NSM
 * @{
 */
#ifndef UCS_NSM_H
#define UCS_NSM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_ret_pb.h"
#include "ucs_rsm.h"
#include "ucs_nsm_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief This enumerator specifies the kind of result - Target or Transmission. */
typedef enum Ns_ResultType_
{
    /*!< \brief Specifies the result of the scripting from target device (typically INIC function-specific error) */
    NS_RESULT_TYPE_TGT_SCRIPT  = 0x00U,
    /*!< \brief Specifies the result of the remote synchronization from target device (typically INIC function-specific error) */
    NS_RESULT_TYPE_TGT_SYNC    = 0x01U,
    /*!< \brief Specifies the transmission error information that occurred on the MOST network. */
    NS_RESULT_TYPE_TX          = 0x02U      

} Ns_ResultType_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Detailed information used for I2C results. */
typedef struct Nsm_ResultDetails_
{
    /*! \brief Specifies the type of the current asynchronous result.
     *  \details The following briefly describes the different types of results available:
     *              - \b NS_RESULT_TYPE_TGT: target results, typically INIC function-specific error found on target device. \n Refer to \em inic_result to get the detailed information.
     *              - \b NS_RESULT_TYPE_TX:  transmission results, typically transmission error on the MOST network. \n Refer to \em tx_result to get the transmission information.
     */
    Ns_ResultType_t result_type;
    /*! \brief Holds the status of the transmission. */
    Ucs_MsgTxStatus_t tx_result;
    /*! \brief Holds the results of the target device. */
    Ucs_StdResult_t inic_result;

} Nsm_ResultDetails_t;
    
/*! \brief  Stores the NodeScript result for internal use. */
typedef struct Nsm_Result_
{
    /*! \brief Result code. */
    Ucs_Ns_ResultCode_t code;  
    /*! \brief Detailed information on the returned result. */
    Nsm_ResultDetails_t details;

} Nsm_Result_t;

/*! \brief  Stores data required by NSM during initialization. */
typedef struct Nsm_InitData_
{
    CBase *base_ptr;                    /*!< \brief Reference to base instance */
    CTransceiver * rcm_ptr;             /*!< \brief Reference to RCM transceiver instance */
    CRemoteSyncManagement * rsm_ptr;    /*!< \brief Reference to RSM instance */

} Nsm_InitData_t;

/*! \brief Structure holds parameters for API locking */
typedef struct Script_ApiLock_
{
    /*! \brief Flag to lock the API */
    bool api;
    /*! \brief API locking instance for Scripting function */
    CApiLocking     rcm_api;
    /*! \brief Observer used for locking timeouts for Scripting function */
    CSingleObserver observer;

} Script_ApiLock_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for the results of the Scripting Manager.
 *  \param  user_ptr     Reference to the called user instance.
 *  \param  result       Result of the scripting operation.
 */
typedef void (*Nsm_ResultCb_t)(void * user_ptr, Nsm_Result_t result);

/*! \brief  Function signature used for the results of the Scripting Manager.
 *  \param  tel_ptr          Reference to the message object.
 *  \param  user_ptr         Reference to the user argument.
 *  \return  Returns \c true to discard the message and free it to the pool (no-pass). Otherwise, returns 
 *           \c false (pass).
 */
typedef bool (*Nsm_RxFilterCb_t)(Msg_MostTel_t *tel_ptr, void *user_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Class structure of the Node Scripting Management. */
typedef struct CNodeScriptManagement_
{
    /*!< \brief Reference to a base instance */
    CBase *base_ptr;
    /*!< \brief Reference to RCM instance */
    CTransceiver * rcm_ptr;
    /*!< \brief Reference to RSM instance */
    CRemoteSyncManagement * rsm_ptr;
    /*!< \brief Reference to the timer management */ 
    CTimerManagement * tm_ptr;
    /*!< \brief Timer for pausing script */
    CTimer script_pause;
    /*!< \brief Service instance for the scheduler */
    CService nsm_srv;
    /*!< \brief Observer used to monitor UCS initialization result */
    CMaskedObserver ucsinit_observer;
    /*!< \brief Observer used to monitor UCS termination event */
    CMaskedObserver ucstermination_observer;
    /*!< \brief Flag to lock the API */
    Script_ApiLock_t lock;
    /*!< \brief Current reference to the script table */
    Ucs_Ns_Script_t * curr_sript_ptr;
    /*!< \brief Current result for internal use */
    Nsm_Result_t curr_internal_result;
    /*!< \brief Current script size */
    uint8_t curr_sript_size;
    /*!< \brief Current script pause */
    uint16_t curr_pause;
    /*!< \brief Flag to determine whether the private api is used or not */
    bool is_private_api_used;
    /*!< \brief Reference to the user instance */
    void * curr_user_ptr;
    /*!< \brief RX filter callback function */
    Nsm_RxFilterCb_t curr_rxfilter_fptr;
    /*!< \brief Private result callback function pointer for current script */
    Nsm_ResultCb_t curr_pv_result_fptr;
    /*!< \brief Current reference to the Node used in public API */
    Ucs_Rm_Node_t * curr_node_ptr;
    /*!< \brief Public result callback function pointer for current script */
    Ucs_Ns_ResultCb_t curr_pb_result_fptr;
    /*!< \brief Target address of the device to be looked for */
    uint16_t target_address;

} CNodeScriptManagement;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CRemoteSyncManagement                                                      */
/*------------------------------------------------------------------------------------------------*/
extern void Nsm_Ctor(CNodeScriptManagement * self, Nsm_InitData_t * init_ptr);
extern Ucs_Return_t Nsm_Run_Pb(CNodeScriptManagement * self, Ucs_Rm_Node_t * node_ptr, Ucs_Ns_ResultCb_t pb_result_fptr);
extern Ucs_Return_t Nsm_Run_Pv(CNodeScriptManagement * self, Ucs_Ns_Script_t * script, uint8_t size, void * user_ptr, Nsm_RxFilterCb_t rx_filter_fptr, Nsm_ResultCb_t result_fptr);
extern bool Nsm_OnRcmRxFilter(void *self, Msg_MostTel_t *tel_ptr);
extern bool Nsm_IsLocked(CNodeScriptManagement * self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef UCS_NSM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

