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
 * \brief Implementation of Application Message Distributor
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_AMD
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_amd.h"
#include "ucs_misc.h"

/*! \brief Priority of the Application Message Distribution */
static const uint8_t AMD_SRV_PRIO           = 248U; /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief Event which starts the Rx message distribution */
static const Srv_Event_t AMD_EV_NOTIFY_RX   = 1U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Amd_Service(void *self);
static void Amd_OnAmsComplete(void* self, Ucs_AmsRx_Msg_t* msg_ptr);
static void Amd_OnEvent(void *self, void *error_code_ptr);
static void Amd_OnTerminateEvent(void *self, void *error_code_ptr);
static void Amd_RxFlush(CAmd *self, CDlList *list_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Initialization                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of class CAmd
 *  \param self     The instance
 *  \param base_ptr Reference to base services
 *  \param ams_ptr  Reference to the AMS
 */
void Amd_Ctor(CAmd *self, CBase *base_ptr, CAms *ams_ptr)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));                 /* reset members to "0" */

    self->base_ptr = base_ptr;
    self->ams_ptr = ams_ptr;

    self->started = false;
    Srv_Ctor(&self->service, AMD_SRV_PRIO, self, &Amd_Service);   /* register service */
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);

    Dl_Ctor(&self->pre_queue, self->base_ptr->ucs_user_ptr);       /* init preprocessor queue */
    Dl_Ctor(&self->rx_queue, self->base_ptr->ucs_user_ptr);        /* init Rx queue */
                                                                  /* register event observer */
    Mobs_Ctor(&self->event_observer, self, EH_E_INIT_SUCCEEDED, &Amd_OnEvent);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_observer);
                                                                  /* register termination events */
    Mobs_Ctor(&self->terminate_observer, self, EH_M_TERMINATION_EVENTS, &Amd_OnTerminateEvent);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->terminate_observer);

    Ams_RxAssignReceiver(self->ams_ptr, &Amd_OnAmsComplete, self);
}

/*! \brief   Assigns a pre-processor callback function for Rx messages
 *  \details This function must be called during initialization time.
 *           The AMS shall not already run.
 *  \param   self          The instance
 *  \param   callback_fptr Reference to the callback function
 *  \param   inst_ptr      Reference to the pre-processor
 */
void Amd_AssignPreprocessor(CAmd *self, Amd_RxMsgCompleteCb_t callback_fptr, void *inst_ptr)
{
    if (callback_fptr != NULL)
    {
        self->preprocess_fptr = callback_fptr;
        self->preprocess_inst_ptr = inst_ptr;

        self->first_receive_fptr = callback_fptr;
        self->first_receive_inst_ptr = inst_ptr;
        self->first_q_ptr = &self->pre_queue;
    }
}

/*! \brief   Assigns a receiver callback function for Rx messages
 *  \details This function must be called during initialization time.
 *           The AMS shall not already run.
 *  \param   self          The instance
 *  \param   callback_fptr Reference to the callback function
 *  \param   inst_ptr      Reference to the receiver
 */
void Amd_AssignReceiver(CAmd *self, Amd_RxMsgCompleteCb_t callback_fptr, void *inst_ptr)
{
    if (callback_fptr != NULL)
    {
        self->receive_fptr = callback_fptr;
        self->receive_inst_ptr = inst_ptr;

        if (self->first_receive_fptr == NULL)
        {
            self->first_receive_fptr = callback_fptr;
            self->first_receive_inst_ptr = inst_ptr;
            self->first_q_ptr = &self->rx_queue;
        }
    }
}

/*! \brief   Assigns as callback function which is able to read and modify the Rx message
 *  \param   self          The instance
 *  \param   callback_fptr Reference to the callback function
 *  \param   inst_ptr      Reference to the instance (owner of the callback function)
 */
void Amd_RxAssignModificator(CAmd *self, Amd_RxModificationCb_t callback_fptr, void *inst_ptr)
{
    if (callback_fptr != NULL)
    {
        self->rx_modification_fptr = callback_fptr;
        self->rx_modification_inst_ptr = inst_ptr;
    }
}

/*! \brief   Service function of CAmd
 *  \details The processing of the Rx queues shall be started asynchronously
 *           after the initialization has succeeded.
 *  \param   self          The instance
 */
static void Amd_Service(void *self)
{
    CAmd *self_ = (CAmd*)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);

    if((event_mask & AMD_EV_NOTIFY_RX) == AMD_EV_NOTIFY_RX)     /* triggered on internal transmission */
    {
        Srv_ClearEvent(&self_->service, AMD_EV_NOTIFY_RX);
        if ((self_->started != false) && (self_->first_receive_fptr != NULL))
        {
            uint16_t size = Dl_GetSize(self_->first_q_ptr);
            if (size > 0U)
            {
                self_->first_receive_fptr(self_->first_receive_inst_ptr);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Events                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked on completed application message reception
 *  \param  self        The instance
 *  \param  msg_ptr     Reference to the completed application message
 */
static void Amd_OnAmsComplete(void *self, Ucs_AmsRx_Msg_t *msg_ptr)
{
    CAmd *self_ = (CAmd*)self;

    if (self_->rx_modification_fptr != NULL)
    {
        self_->rx_modification_fptr(self_->rx_modification_inst_ptr, msg_ptr);
    }

    if (self_->first_receive_fptr != NULL)
    {
        Amsg_RxEnqueue(msg_ptr, self_->first_q_ptr);

        if (self_->started != false)
        {
            self_->first_receive_fptr(self_->first_receive_inst_ptr);
        }
    }
    else
    {
        Ams_RxFreeMsg(self_->ams_ptr, msg_ptr);
    }
}

/*! \brief  Callback function if an events leads to the termination of the MNS
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Amd_OnTerminateEvent(void *self, void *error_code_ptr)
{
    CAmd *self_ = (CAmd*)self;
    MISC_UNUSED(error_code_ptr);

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[AMD]", "Starting AMD Cleanup", 0U));
    Amd_RxFlush(self_, &self_->pre_queue);
    Amd_RxFlush(self_, &self_->rx_queue);
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[AMD]", "Finished AMD Cleanup", 0U));
}

/*! \brief  Callback function which is invoked if the initialization is complete
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Amd_OnEvent(void *self, void *error_code_ptr)
{
    CAmd *self_ = (CAmd*)self;
    MISC_UNUSED(error_code_ptr);

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[AMD]", "Received init complete event", 0U));
    self_->started = true;
    Srv_SetEvent(&self_->service, AMD_EV_NOTIFY_RX);
}

/*! \brief  Flushes a given application Rx message queue
 *  \param  self        The instance
 *  \param  list_ptr    Reference to a list containing application Rx message objects
 */
static void Amd_RxFlush(CAmd *self, CDlList *list_ptr)
{
    Ucs_AmsRx_Msg_t *msg_ptr;

    for (msg_ptr = Amsg_RxDequeue(list_ptr); msg_ptr != NULL; msg_ptr = Amsg_RxDequeue(list_ptr))
    {
        Ams_RxFreeMsg(self->ams_ptr, msg_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Pre-processor methods                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Peeks the front-most application message from the preprocessing queue
 *  \param  self        The instance
 *  \return Returns a reference to the front-most application message or \c NULL if the queue
 *          is empty.
 */
Ucs_AmsRx_Msg_t* Amd_PrePeekMsg(CAmd *self)
{
    return (Ucs_AmsRx_Msg_t*)(void*)Amsg_RxPeek(&self->pre_queue);
}

/*! \brief  Removes the front-most application message from the preprocessing queue and frees it
 *  \param  self        The instance
 */
void Amd_PreReleaseMsg(CAmd *self)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Amsg_RxDequeue(&self->pre_queue);

    if (msg_ptr != NULL)
    {
        Ams_RxFreeMsg(self->ams_ptr, msg_ptr);
    }
}

/*! \brief  Forwards the front-most application message from the preprocessing queue to the Rx queue
 *  \param  self        The instance
 */
void Amd_PreForwardMsg(CAmd *self)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Amsg_RxDequeue(&self->pre_queue);

    if (msg_ptr != NULL)
    {
        if (self->receive_fptr != NULL)
        {
            Amsg_RxEnqueue(msg_ptr, &self->rx_queue);
            self->receive_fptr(self->receive_inst_ptr);
        }
        else
        {
            Ams_RxFreeMsg(self->ams_ptr, msg_ptr);
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Receiver methods                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Peeks the front-most application message from the Rx queue
 *  \param  self        The instance
 *  \return Returns a reference to the front-most application message or \c NULL if the queue
 *          is empty.
 */
Ucs_AmsRx_Msg_t* Amd_RxPeekMsg(CAmd *self)
{
    return (Ucs_AmsRx_Msg_t*)(void*)Amsg_RxPeek(&self->rx_queue);
}

/*! \brief  Removes the front-most application message from the Rx queue and frees it
 *  \param  self        The instance
 */
void Amd_RxReleaseMsg(CAmd *self)
{
    Ucs_AmsRx_Msg_t *msg_ptr = Amsg_RxDequeue(&self->rx_queue);

    if (msg_ptr != NULL)
    {
        Ams_RxFreeMsg(self->ams_ptr, msg_ptr);
    }
}

/*! \brief  Retrieves the number of messages which are appended to the Rx queue
 *  \param  self        The instance
 *  \return Returns the number of messages.
 */
uint16_t Amd_RxGetMsgCnt(CAmd *self)
{
    return Dl_GetSize(&self->rx_queue);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

