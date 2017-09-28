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
 * \brief Internal header file of AMS Segmentation Class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_AMSSEGM
 * @{
 */

#ifndef UCS_SEGMENTATION_H
#define UCS_SEGMENTATION_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_message.h"
#include "ucs_amsmessage.h"
#include "ucs_amspool.h"
#include "ucs_base.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Defines the maximum payload size of a single transfer in bytes */
#define SEGM_MAX_SIZE_TEL  45U

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Collection of possible segmentation errors */
typedef enum Segm_Error_
{
    SEGM_ERR_1 = 1,     /*!< \brief  The first segment is missing. */

    SEGM_ERR_2 = 2,     /*!< \brief  The device is not able to receive a message of this size.
                         *           MNS specific: The allocation of user provided payload failed. 
                         */
    SEGM_ERR_3 = 3,     /*!< \brief  Unexpected segment number. */

    SEGM_ERR_4 = 4,     /*!< \brief  Too many unfinished segmentation messages were pending. */

    SEGM_ERR_5 = 5,     /*!< \brief  A timeout occurred while waiting for the next segment. */

    SEGM_ERR_6 = 6,     /*!< \brief  The Device is not capable to handle segmented messages.
                         *           MNS specific: The application did not assign the payload allocation 
                         *           function in Ucs_Ams_InitData_t prior calling Ucs_Init().
                         */
    SEGM_ERR_7 = 7      /*!< \brief  Segmented message has not been finished before the arrival of 
                         *           another message with the identical FBlockID, InstID, FktID, and 
                         *           OPType sent by the same node.
                         */
} Segm_Error_t;

/*! \brief Segmentation result */
typedef enum Segm_Result_
{
    SEGM_RES_OK,        /*!< \brief Telegram was processed */
    SEGM_RES_RETRY      /*!< \brief Telegram shall be processed again as soon as messages are freed to the Rx pool */

} Segm_Result_t;

/*! \brief  Callback function to notify that a segmentation error has occurred
 *  \param  self    The instance
 *  \param  tel_ptr The affected telegram
 *  \param  error   The segmentation error code (1..7)
 */
typedef void (*Segm_OnError_t)(void *self, Msg_MostTel_t *tel_ptr, Segm_Error_t error);

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      AMS Segmentation Class
 *  \details    Performs Tx and Rx Segmentation
 */
typedef struct CSegmentation_
{
    CBase                       *base_ptr;              /*!< \brief Reference to base services */
    CAmsMsgPool                 *pool_ptr;              /*!< \brief Reference to object/payload pool */

    Segm_OnError_t               error_fptr;            /*!< \brief Callback function to notify segmentation errors */
    void                        *error_inst;            /*!< \brief Instance which is notified on segmentation errors */

    CDlList                      processing_list;       /*!< \brief  Segmented and un-finished Rx messages */
    CTimer                       gc_timer;              /*!< \brief  Timer to trigger the garbage collector */
    uint16_t                     rx_default_payload_sz; /*!< \brief  Payload size that shall be allocated if size-prefixes
                                                         *           segmentation message is missing */

} CSegmentation;

/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
extern void Segm_Ctor(CSegmentation *self, CBase *base_ptr, CAmsMsgPool *pool_ptr, uint16_t rx_def_payload_sz);
extern void Segm_AssignRxErrorHandler(CSegmentation *self, Segm_OnError_t error_fptr, void *error_inst);
extern void Segm_Cleanup(CSegmentation *self);

/*------------------------------------------------------------------------------------------------*/
/* Public method prototypes                                                                       */
/*------------------------------------------------------------------------------------------------*/
extern bool Segm_TxBuildSegment(CSegmentation *self, Ucs_AmsTx_Msg_t *msg_ptr, Msg_MostTel_t *tel_ptr);
extern Ucs_AmsRx_Msg_t* Segm_RxExecuteSegmentation(CSegmentation *self, Msg_MostTel_t *tel_ptr, Segm_Result_t *result_ptr);
extern void Segm_RxGcScanProcessingHandles(void *self);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_SEGMENTATION_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

