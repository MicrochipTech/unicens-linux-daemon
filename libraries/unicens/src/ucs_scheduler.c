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
 * \brief Implementation of the scheduler module. The module consists of the two classes
 *        CScheduler and CService.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SCHEDULER
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_scheduler.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Constants                                                                                      */
/*------------------------------------------------------------------------------------------------*/
const Srv_Event_t SRV_EMPTY_EVENT_MASK = (Srv_Event_t)0x00000000;   /*!< \brief Empty event mask */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static bool Scd_SearchSlot(void *current_prio_ptr, void *new_prio_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CScheduler                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the scheduler class.
 *  \param self        Instance pointer
 *  \param init_ptr    Reference to the initialization data
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Scd_Ctor(CScheduler *self, Scd_InitData_t *init_ptr, void * ucs_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->ucs_user_ptr = ucs_user_ptr;
    Dl_Ctor(&self->srv_list, ucs_user_ptr);
    Ssub_Ctor(&self->service_request_subject, ucs_user_ptr);
    (void)Ssub_AddObserver(&self->service_request_subject,
                           init_ptr->service_request_obs_ptr);
    self->scd_srv_is_running = false;
}

/*! \brief  Add the given service to the scheduler. All services are arranged in priority order.
 *          A service with a higher priority will execute before a service with a lower priority.
 *  \param  self       Instance pointer
 *  \param  srv_ptr    Reference of the service which shall be added
 *  \return SCD_OK: Service added
 *  \return SCD_SRV_ALREADY_LISTED: Services already listed
 */
Scd_Ret_t Scd_AddService(CScheduler *self, CService *srv_ptr)
{
    Scd_Ret_t ret_val;

    /* Check that service is not already part of scheduler */
    if(Dl_IsNodeInList(&self->srv_list, &srv_ptr->list_node) == false) 
    {
        /* Search slot where the service must be inserted depending on the priority value.  */
        CDlNode *result_ptr = Dl_Foreach(&self->srv_list, &Scd_SearchSlot, &srv_ptr->priority);

        if(result_ptr != NULL)   /* Slot found? */
        {
            Dl_InsertBefore(&self->srv_list, result_ptr, &srv_ptr->list_node);
        }
        else                    /* No slot found -> Insert as last node */
        {
            Dl_InsertTail(&self->srv_list, &srv_ptr->list_node);
        }
        /* Create back link service -> scheduler */
        srv_ptr->scd_ptr = self;
        Dln_SetData(&srv_ptr->list_node, &srv_ptr->priority);
        ret_val = SCD_OK;
    }
    else    /* Service is already part of schedulers list */
    {
        ret_val = SCD_SRV_ALREADY_LISTED;
    }

    return ret_val;
}

/*! \brief  Remove the given service from the schedulers list.
 *  \param  self       Instance pointer
 *  \param  srv_ptr    Reference of the service which shall be removed
 *  \return SCD_OK: Service removed
 *  \return SCD_UNKNOWN_SRV: Unknown service can not be removed
 */
Scd_Ret_t Scd_RemoveService(CScheduler *self, CService *srv_ptr)
{
    Scd_Ret_t ret_val = SCD_OK;

    /* Error occurred? */
    if(Dl_Remove(&self->srv_list, &srv_ptr->list_node) == DL_UNKNOWN_NODE)
    {
        ret_val = SCD_UNKNOWN_SRV;
    }

    return ret_val;
}

/*! \brief Service function of the scheduler module.
 *  \param self   Instance pointer
 */
void Scd_Service(CScheduler *self)
{
    CService *current_srv_ptr = (CService *)(void*)self->srv_list.head;

    /* Scheduler service is running. Important for event handling */
    self->scd_srv_is_running = true;

    while(current_srv_ptr != NULL)   /* Process registered services */
    {
        if(current_srv_ptr->service_fptr != NULL)
        {
            /* Are events pending for the current service */
            if(current_srv_ptr->event_mask != SRV_EMPTY_EVENT_MASK)
            {
                /* Execute service callback function */
                current_srv_ptr->service_fptr(current_srv_ptr->instance_ptr);
                /* Was the current service removed from the schedulers list? */
                if((current_srv_ptr->list_node.prev == NULL) && (current_srv_ptr->list_node.next == NULL))
                {
                    break;  /* Abort scheduler service */
                }
            }
        }
        current_srv_ptr = (CService *)(void*)current_srv_ptr->list_node.next;
    }
    /* Scheduler services finished */
    self->scd_srv_is_running = false;
}

/*! \brief  Searches for pending events.
 *  \param  self   Instance pointer
 *  \return true: At least one event is active
 *  \return false: No event is pending
 */
bool Scd_AreEventsPending(CScheduler *self)
{
    bool ret_val = false;
    CService *current_srv_ptr = (CService *)(void*)self->srv_list.head;

    while(current_srv_ptr != NULL)
    {
        if(current_srv_ptr->event_mask != SRV_EMPTY_EVENT_MASK)
        {
            ret_val = true;
            break;
        }
        current_srv_ptr = (CService *)(void*)current_srv_ptr->list_node.next;
    }

    return ret_val;
}

/*! \brief  Searches the slot where the new service has to be inserted. The position depends on 
 *          the given priority. If a the priority of the new service is higher than the priority
 *          of the current service \c true is returned which stops the search.
 *  \param  current_prio_ptr   Current service which is analyzed 
 *  \param  new_prio_ptr       Priority of the new service
 *  \return false: The priority of the current service is greater than the new priority
 *  \return true: The priority of the current service is less than or equal to the new priority
 */
static bool Scd_SearchSlot(void *current_prio_ptr, void *new_prio_ptr)
{
    uint8_t current_prio_ptr_ = *((uint8_t *)current_prio_ptr);
    uint8_t new_prio_ = *((uint8_t*)new_prio_ptr);
    bool ret_val = false;

    if(current_prio_ptr_ <= new_prio_)
    {
        ret_val = true;
    }

    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CService                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Parameter constructor of the service class.
 *  \param self            Instance pointer
 *  \param instance_ptr    Reference to object which contains the corresponding service
 *  \param priority        Priority of the service
 *  \param service_fptr    Service callback
 */
void Srv_Ctor(CService *self, uint8_t priority, void *instance_ptr, Srv_Cb_t service_fptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    Dln_Ctor(&self->list_node, NULL);
    self->priority = priority;
    self->instance_ptr = instance_ptr;
    self->service_fptr = service_fptr;
}

/*! \brief Sets events for the given service according to the given event mask.
 *  \param self        Instance pointer
 *  \param event_mask  Mask of the events to be set
 */
void Srv_SetEvent(CService *self, Srv_Event_t event_mask)
{
    self->event_mask |= event_mask;
    if(self->scd_ptr->scd_srv_is_running == false) 
    {
        Ssub_Notify(&self->scd_ptr->service_request_subject, NULL, false);
    }
}

/*! \brief The function returns the current state of all event bits of the service.
 *  \param self            Instance pointer
 *  \param event_mask_ptr  Reference to the memory of the returned event mask
 */
void Srv_GetEvent(CService *self, Srv_Event_t *event_mask_ptr)
{
    *event_mask_ptr = self->event_mask;
}

/*! \brief Clears events for the given service according to the given event mask.
 *  \param self       Instance pointer
 *  \param event_mask Mask of the events to be clear
 */
void Srv_ClearEvent(CService *self, Srv_Event_t event_mask)
{
    self->event_mask &= ~event_mask;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

