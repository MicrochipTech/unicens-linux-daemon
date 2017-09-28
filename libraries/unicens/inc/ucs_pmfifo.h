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
 * \brief Internal header file of Port Message FIFO
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_PMF
 * @{
 */

#ifndef UCS_PMFIFO_H
#define UCS_PMFIFO_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_base.h"
#include "ucs_lld_pb.h"
#include "ucs_message.h"
#include "ucs_encoder.h"
#include "ucs_pmp.h"
#include "ucs_lldpool.h"
#include "ucs_pmchannel.h"
#include "ucs_pmcmd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Number of LLD Tx handles dedicated to each FIFO */
#define FIFO_TX_HANDLES     5U

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked when receiving an Rx message 
 *  \param  self    The Instance (of the host)
 *  \param  msg_ptr The Rx message
 */
typedef void (*Fifo_OnRxMsg_t)(void *self, CMessage *msg_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Initialization structure of class Port Message FIFO */
typedef struct Fifo_InitData_
{
    CBase              *base_ptr;       /*!< \brief Reference to base module */
    CPmChannel         *channel_ptr;    /*!< \brief Points to channel object which is needed to communicate with
                                         *          the driver */
    IEncoder           *tx_encoder_ptr; /*!< \brief Encoder for Tx messages */
    IEncoder           *rx_encoder_ptr; /*!< \brief Encoder for Rx messages */
    Fifo_OnRxMsg_t      rx_cb_fptr;     /*!< \brief Callback function invoked for Rx */
    void               *rx_cb_inst;     /*!< \brief Instance which is referred when invoking rx_cb_fptr */

} Fifo_InitData_t;

/*! \brief  Initialization structure of class Port Message FIFO */
typedef struct Fifo_Config_
{
    Pmp_FifoId_t    fifo_id;            /*!< \brief Identifier of message FIFO.
                                         *   \details It is required that the fifo_id has the same value as
                                         *            specified in PMP.
                                         */
    uint8_t         rx_credits;         /*!< \brief Number of Rx credits, i.e. reserved Rx messages */
    uint8_t         rx_threshold;       /*!< \brief Number of Rx credits which are acknowledged in a single status.
                                         *   \details The value needs to be smaller or equal than \c rx_credits.
                                         *          Valid values are:
                                         *          - 0,1:              Single message acknowledge
                                         *          - 2..rx_credits:    Implicit acknowledge is triggered after 
                                         *                              the specified number of messages.
                                         */
    uint8_t         tx_wd_timeout;      /*!< \brief Idle timeout in x100ms. Formerly known as watchdog timeout */
    uint16_t        tx_wd_timer_value;  /*!< \brief Timer value used to trigger the watchdog in ms */
    uint8_t         rx_ack_timeout;     /*!< \brief Rx status timeout in x100ms. */
    uint8_t         rx_busy_allowed;    /*!< \brief Number of allowed RxStatus busy responds. 0..14, or 0xF (infinite) */

} Fifo_Config_t;

/*------------------------------------------------------------------------------------------------*/
/* Internal types                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The synchronization status  */
typedef enum Fifo_SyncState_
{ 
    FIFO_S_UNSYNCED_INIT,
    FIFO_S_SYNCING,
    FIFO_S_UNSYNCED_BUSY,
    FIFO_S_UNSYNCED_READY,
    FIFO_S_SYNCED,
    FIFO_S_UNSYNCING

} Fifo_SyncState_t;

/*! \brief The class CPmFifo*/
typedef struct CPmFifo_
{
    Fifo_InitData_t         init;               /*!< \brief Initialization data */
    Fifo_Config_t           config;             /*!< \brief Configuration data */

    CService                service;            /*!< \brief Service object */
    Fifo_SyncState_t        sync_state;         /*!< \brief Synchronization state of the FIFO */
    CSubject                sync_state_subject; /*!< \brief Notification of changed synchronization state */
    uint8_t                 sync_params[4];     /*!< \brief Synchronization parameters */
    uint8_t                 sync_cnt;           /*!< \brief Counts the number of synchronization attempts */

    struct CPmFifo_wd_
    {
        CTimer              timer;              /*!< \brief The timer object */
        CPmCommand          wd_cmd;             /*!< \brief The watchdog command message */
        uint16_t            timer_value;        /*!< \brief The internal timer value used by PMC to trigger the watchdog */
        bool                request_started;    /*!< \brief Is used to check if the INIC responds with a status before the 
                                                 *          next Cmd.REQUEST_STATUS is triggered.
                                                 */
    } wd; 

    struct CPmFifo_rx_
    {
        CDlList queue;                          /*!< \brief Message queue containing all incoming messages */

        IEncoder *encoder_ptr;                  /*!< \brief Encoder for Rx messages */
        Fifo_OnRxMsg_t on_complete_fptr;        /*!< \brief Callback function invoked for Rx */
        void *on_complete_inst;                 /*!< \brief Instance which is referred when invoking rx_cb_fptr */

        uint8_t ack_threshold;                  /*!< \brief Number of unacknowledged Rx credits */
        uint8_t ack_last_ok_sid;                /*!< \brief Latest SID which was acknowledged with "success" */ 
        uint8_t expected_sid;                   /*!< \brief The next expected Rx message SeqId */
        uint8_t busy_num;                       /*!< \brief The number of currently processing data messages */

        bool wait_processing;                   /*!< \brief If set: Wait until transmission of e.g. NACK has finished 
                                                 *          before continuing with further Rx message processing.
                                                 *          The flag is used if a status must be sent explicitly.
                                                 */
        CPmCommand status;                      /*!< \brief Rx status channel control  */

    } rx;

    struct CPmFifo_tx_
    {
        CDlList waiting_queue;                  /*!< \brief Queue containing all outgoing messages */
        CDlList pending_q;                      /*!< \brief Queue containing all messages waiting for Tx status */
        IEncoder *encoder_ptr;                  /*!< \brief Encoder for Tx messages */
        uint8_t credits;                        /*!< \brief Remaining Tx credits */

        CLldPool lld_pool;                      /*!< \brief Pool of LLD Tx messages, used for data messages */

        CPmh pm_header;                         /*!< \brief Temporary header which is used to build the FIFO data messages*/
        CPmCommand cancel_cmd;                  /*!< \brief Tx cancel command message */
        CPmCommand sync_cmd;                    /*!< \brief Sync command message */

        uint8_t sid_next_to_use;                /*!< \brief SID that shall be used for the next transmission */
        uint8_t sid_last_completed;             /*!< \brief Latest SID that was acknowledged by the INIC */
        uint8_t current_sid;                    /*!< \brief Tracks the latest valid FIFO status SID received from the INIC */
        Pmp_StatusType_t current_type;          /*!< \brief Tracks the latest valid FIFO status type received from the INIC */
        uint8_t current_code;                   /*!< \brief Tracks the latest valid FIFO status code received from the INIC */

        bool status_waiting_release;            /*!< \brief Is \c true if status notification wasn't completed due to messages
                                                 *          which are not yet released by the LLD. 
                                                 */
        bool cancel_all_running;                /*!< \brief Is \c true during pending command CANCEL_ALL. This command is required
                                                 *          if the front-most message is segmented which requires to discard all
                                                 *          belonging segments (same \c cancel_id) after the CANCEL_ALL was completed.
                                                 */
        uint8_t failure_status;                 /*!< \brief Stores the Tx status until the message is canceled */
        uint8_t failure_sid;                    /*!< \brief Stores the SID of the last cancelled data message */
    } tx;

} CPmFifo;


/*------------------------------------------------------------------------------------------------*/
/* Function Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
extern void Fifo_Ctor(CPmFifo *self, const Fifo_InitData_t *init_ptr, const Fifo_Config_t *config_ptr);
extern void Fifo_Stop(CPmFifo *self, Fifo_SyncState_t new_state, bool allow_notification);
extern void Fifo_Cleanup(CPmFifo *self);

extern void Fifo_Synchronize(CPmFifo *self);
extern void Fifo_Unsynchronize(CPmFifo *self);
extern Fifo_SyncState_t Fifo_GetState(CPmFifo *self);
extern void Fifo_AddStateObserver(CPmFifo *self, CObserver *obs_ptr);
extern void Fifo_RemoveStateObserver(CPmFifo *self, CObserver *obs_ptr);

/* Rx interface */
extern void Fifo_RxReleaseMsg(CPmFifo *self, CMessage *msg_ptr);

/* Tx interface */
extern void Fifo_Tx(CPmFifo *self, CMessage *msg_ptr, bool bypass);
extern void Fifo_TxOnRelease(void *self, Ucs_Lld_TxMsg_t *handle_ptr);

#ifdef __cplusplus
}                                                   /* extern "C" */
#endif

#endif                                              /* UCS_PMFIFO_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

