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
 * \brief Implementation of the observer library module. The module consists of the two classes 
 *        CSubject and CObserver.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_OBS
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_obs.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Sub_UpdateList(CSubject *self);
static bool Sub_CheckObserver(void *current_obs_ptr, void *subject_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CSubject                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the subject class. Initializes a subject which distributes its data to
 *         a list of observers.
 *  \param self        Instance pointer
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Sub_Ctor(CSubject *self, void *ucs_user_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->ucs_user_ptr = ucs_user_ptr;
    Dl_Ctor(&self->list, self->ucs_user_ptr);
    Dl_Ctor(&self->add_list, self->ucs_user_ptr);
}

/*! \brief  Adds an observer to a subjects list.
 *  \param  self       Instance pointer
 *  \param  obs_ptr    Pointer to observer instance
 *  \return \c SUB_OK: No error
 *  \return \c SUB_ALREADY_ADDED: Observer is already added
 *  \return \c SUB_UNKNOWN_OBSERVER: Given observer is not valid
 */
Sub_Ret_t Sub_AddObserver(CSubject *self, CObserver *obs_ptr)
{
    Sub_Ret_t ret_val;
    if(obs_ptr == NULL)
    {
        ret_val = SUB_UNKNOWN_OBSERVER;
    }
    else if(obs_ptr->valid != false)
    {
        ret_val = SUB_ALREADY_ADDED;
    }
    else if((self->notify != false) &&
            (Dl_IsNodeInList(&self->list, &obs_ptr->node) == false)  &&
            (Dl_IsNodeInList(&self->add_list, &obs_ptr->node) == false))
    {
        TR_ASSERT(self->ucs_user_ptr, "[OBS]", (self->num_observers < 0xFFU));
        Dl_InsertTail(&self->add_list, &obs_ptr->node);
        obs_ptr->valid = true;
        self->changed = true;
        ret_val = SUB_DELAYED;
    }
    else if((self->notify == false) && (Dl_IsNodeInList(&self->list, &obs_ptr->node) == false))
    {
        TR_ASSERT(self->ucs_user_ptr, "[OBS]", (self->num_observers < 0xFFU));
        ret_val = SUB_OK;
        Dl_InsertTail(&self->list, &obs_ptr->node);
        obs_ptr->valid = true;
        self->num_observers++;
    }
    else
    {
        ret_val = SUB_UNKNOWN_OBSERVER;
    }
    return ret_val;
}

/*! \brief  Removes an observer from a subjects list.
 *  \param  self       Instance pointer
 *  \param  obs_ptr    Pointer to observer instance
 *  \return \c SUB_OK: No error
 *  \return \c SUB_UNKNOWN_OBSERVER: Unknown observer is given
 *  \return \c SUB_UNKNOWN_OBSERVER: Given observer is not valid
 */
Sub_Ret_t Sub_RemoveObserver(CSubject *self, CObserver *obs_ptr)
{
    Sub_Ret_t ret_val;
    if(obs_ptr == NULL)
    {
        ret_val = SUB_UNKNOWN_OBSERVER;
    }
    else if(obs_ptr->valid == false)
    {
        ret_val = SUB_UNKNOWN_OBSERVER;
    }
    else if((self->notify != false) &&
            (Dl_IsNodeInList(&self->list, &obs_ptr->node) != false))
    {
        TR_ASSERT(self->ucs_user_ptr, "[OBS]", (self->num_observers > 0U));
        obs_ptr->valid = false;
        self->changed = true;
        self->num_observers--;
        ret_val = SUB_DELAYED;
    }
    else if((self->notify == false) &&
            (Dl_Remove(&self->list, &obs_ptr->node) == DL_OK))
    {
        TR_ASSERT(self->ucs_user_ptr, "[OBS]", (self->num_observers > 0U));
        self->num_observers--;
        ret_val = SUB_OK;
    }
    else
    {
        ret_val = SUB_UNKNOWN_OBSERVER;
    }
    return ret_val;
}

/*! \brief Notifies all registered observers of a subject.
 *  \param self   Instance pointer
 *  \param data_ptr  Reference to value to distribute (optional)
 */
void Sub_Notify(CSubject *self, void *data_ptr)
{
    if(self != NULL)
    {
        CDlNode *n_tmp = self->list.head;
        self->notify = true;
        self->changed = false;
        while(n_tmp != NULL)
        {
            CObserver *o_tmp = (CObserver *)n_tmp->data_ptr;
            if((o_tmp->update_fptr != NULL)  && (o_tmp->valid != false))
            {
                (o_tmp->update_fptr)(o_tmp->inst_ptr, data_ptr);
            }
            n_tmp = n_tmp->next;
        }
        if(self->changed != false)
        {
            Sub_UpdateList(self);
        }
        self->notify = false;
    }
}

/*! \brief Updates the list of observers. Delayed remove- and add-operations are processed.
 *  \param self   Instance pointer
 */
static void Sub_UpdateList(CSubject *self)
{
    (void)Dl_Foreach(&self->list, &Sub_CheckObserver, self);
    Dl_AppendList(&self->list, &self->add_list);
}

/*! \brief  Checks if the given observer is still valid. If the observer is invalid it will be
 *          removed from the list. This function is used by the foreach loop in Sub_UpdateList().
 *  \param  current_obs_ptr     Reference to the current observer object
 *  \param  subject_ptr         Reference to the subject object
 *  \return Returns always \c false. Force to process the whole list.
 */
static bool Sub_CheckObserver(void *current_obs_ptr, void *subject_ptr)
{
    CObserver *current_obs_ptr_ = (CObserver *)current_obs_ptr;
    CSubject *subject_ptr_ = (CSubject *)subject_ptr;

    if(current_obs_ptr_->valid == false) 
    {
        (void)Dl_Remove(&subject_ptr_->list, &current_obs_ptr_->node);
    }
    return false;
}

/*! \brief  Returns the number of registered observers of a subject.
 *  \param  self   Instance pointer
 *  \return The number of registered observers
 */
uint8_t Sub_GetNumObservers(CSubject *self)
{
    return self->num_observers;
}

/*! \brief  Switches all observers of the source-subject to the target-subject.
 *  \param  sub_target  Target subject
 *  \param  sub_source  Source subject
 *  \return \c SUB_OK: No error
 *  \return \c SUB_INVALID_OPERATION: Target and source must be different objects
 */
Sub_Ret_t Sub_SwitchObservers(CSubject *sub_target, CSubject *sub_source)
{
    Sub_Ret_t ret_val;

    if(sub_target == sub_source)
    {
        ret_val = SUB_INVALID_OPERATION;
    }
    else
    {
        Dl_AppendList(&sub_target->list, &sub_source->list);
        sub_target->num_observers += sub_source->num_observers;
        sub_source->num_observers = 0U;
        ret_val = SUB_OK;
    }
    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CObserver                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the observer class. Initializes an observer which is notified
 *         by a corresponding subject.
 *  \param self        Instance pointer
 *  \param inst_ptr    Instance pointer used by update_fptr()
 *  \param update_fptr Callback function to update the observer
 */
void Obs_Ctor(CObserver *self, void *inst_ptr, Obs_UpdateCb_t update_fptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->inst_ptr = inst_ptr;
    self->update_fptr = update_fptr;
    Dln_Ctor(&self->node, self);
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CSingleSubject                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the single-subject class. Initializes a single-subject which distributes 
 *         its data to the registered single-observer.
 *  \param self        Instance pointer
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Ssub_Ctor(CSingleSubject *self, void *ucs_user_ptr)
{
    self->observer_ptr = NULL;
    self->ucs_user_ptr = ucs_user_ptr;
    self->user_mask   = 0U;
}

/*! \brief  Adds a single-observer to a single-subject.
 *  \param  self       Instance pointer
 *  \param  obs_ptr    Pointer to single-observer instance
 *  \return \c SSUB_OK: No error
 *  \return \c SSUB_ALREADY_ADDED: Observer is already added
 *  \return \c SSUB_UNKNOWN_OBSERVER: Given observer is not valid
 */
Ssub_Ret_t Ssub_AddObserver(CSingleSubject *self, CSingleObserver *obs_ptr)
{
    Ssub_Ret_t ret_val;
    if(obs_ptr == NULL)
    {
        ret_val = SSUB_UNKNOWN_OBSERVER;
    }
    else if(self->observer_ptr != obs_ptr)
    {
#ifdef UCS_TR_INFO
        if(self->observer_ptr != NULL)
        {
            TR_INFO((self->ucs_user_ptr, "[SSUB]", "Observer callback has been overwritten", 0U));
        }
#endif
        ret_val = SSUB_OK;
        self->observer_ptr = obs_ptr;
    }
    else
    {
        ret_val = SSUB_ALREADY_ADDED;
    }

    return ret_val;
}

/*! \brief Removes an single-observer from a single-subject.
 *  \param self       Instance pointer
 */
void Ssub_RemoveObserver(CSingleSubject *self)
{
    self->observer_ptr = NULL;
}

/*! \brief Notifies the registered single-observer of the given single-subject.
 *  \param self   Instance pointer
 *  \param data_ptr        Reference to value to distribute (optional)
 *  \param auto_remove     If true the observer will be removed
 */
void Ssub_Notify(CSingleSubject *self, void *data_ptr, bool auto_remove)
{
    void *inst_ptr = NULL;
    Obs_UpdateCb_t update_fptr = NULL;
    if(self->observer_ptr != NULL)
    {
        inst_ptr = self->observer_ptr->inst_ptr;
        update_fptr = self->observer_ptr->update_fptr;
        if(auto_remove != false)
        {
            self->observer_ptr = NULL;
        }
    }
    if(update_fptr != NULL)
    {
        update_fptr(inst_ptr, data_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CSingleObserver                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the single-observer class. Initializes an single-observer which is
 *         notified by a corresponding single-subject.
 *  \param self        Instance pointer
 *  \param inst_ptr    Instance pointer used by update_fptr()
 *  \param update_fptr Callback function to update the observer
 */
void Sobs_Ctor(CSingleObserver *self, void *inst_ptr, Sobs_UpdateCb_t update_fptr)
{
    self->inst_ptr = inst_ptr;
    self->update_fptr = update_fptr;
}

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CMaskedObserver                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the masked-observer class. Initializes an observer which is notified
 *         by a corresponding subject.
 *  \param self                 Instance pointer
 *  \param inst_ptr             Instance pointer used by update_fptr()
 *  \param notification_mask    Notification bitmask
 *  \param update_fptr Callback function to update the observer
 */
void Mobs_Ctor(CMaskedObserver *self,
               void *inst_ptr,
               uint32_t notification_mask,
               Obs_UpdateCb_t update_fptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    Obs_Ctor(&self->parent, inst_ptr, update_fptr);
    self->notification_mask = notification_mask;
}

/*! \brief Sets the notification mask of a masked-observer.
 *  \param self     Instance pointer
 *  \param mask     Bitmask to set
 */
void Mobs_SetNotificationMask(CMaskedObserver *self, uint32_t mask)
{
    self->notification_mask = mask;
}

/*! \brief Retrieves the notification mask of a masked-observer.
 *  \param  self     Instance pointer
 *  \return Returns the current notification bitmask of the given observer
 */
uint32_t Mobs_GetNotificationMask(CMaskedObserver *self)
{
    return self->notification_mask;
}

/*------------------------------------------------------------------------------------------------*/
/* Additional methods of class CSubject used in combination with CMaskedObserver                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Adds an masked-observer to a masked-subjects list.
 *  \param  self       Instance pointer
 *  \param  obs_ptr    Pointer to observer instance
 *  \return \c SUB_OK: No error
 *  \return \c SUB_ALREADY_ADDED: Observer is already added
 *  \return \c SUB_UNKNOWN_OBSERVER: Given observer is not valid
 */
Sub_Ret_t Msub_AddObserver(CSubject *self, CMaskedObserver *obs_ptr)
{
    return Sub_AddObserver(self, &obs_ptr->parent);
}

/*! \brief  Removes an masked-observer from a subjects list.
 *  \param  self       Instance pointer
 *  \param  obs_ptr    Pointer to observer instance
 *  \return \c SUB_OK: No error
 *  \return \c SUB_UNKNOWN_OBSERVER: Unknown observer is given
 *  \return \c SUB_UNKNOWN_OBSERVER: Given observer is not valid
 */
Sub_Ret_t Msub_RemoveObserver(CSubject *self, CMaskedObserver *obs_ptr)
{
    return Sub_RemoveObserver(self, &obs_ptr->parent);
}

/*! \brief Notifies all registered masked-observers of a masked-subject.
 *  \param self                 Instance pointer
 *  \param data_ptr             Reference to value to distribute (optional)
 *  \param notification_mask    Bitmask indicates notified observers
 */
void Msub_Notify(CSubject *self, void *data_ptr, uint32_t notification_mask)
{
    if(self != NULL)
    {
        CDlNode *n_tmp = self->list.head;
        self->notify = true;
        self->changed = false;
        while(n_tmp != NULL)
        {
            CMaskedObserver *o_tmp = (CMaskedObserver *)n_tmp->data_ptr;
            if( (o_tmp->parent.update_fptr != NULL)  &&
                (o_tmp->parent.valid != false)      &&
                ((o_tmp->notification_mask & notification_mask) != 0U) )
            {
                (o_tmp->parent.update_fptr)(o_tmp->parent.inst_ptr, data_ptr);
            }
            n_tmp = n_tmp->next;
        }
        if(self->changed != false)
        {
            Sub_UpdateList(self);
        }
        self->notify = false;
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

