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
 * \brief Internal header file of the observer library module. The module consists of the two
 *        classes CSubject and CObserver.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_OBS
 * @{
 */

#ifndef UCS_OBS_H
#define UCS_OBS_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"
#include "ucs_dl.h"
#include "ucs_ret_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature used for callback functions which notifies the observers.
 *  \param self     Instance pointer
 *  \param data_ptr Reference to optional data
 */
typedef void (*Obs_UpdateCb_t)(void *self, void *data_ptr);

/*! \brief Function signature used for callback functions which notifies the single-observers.
 *  \param self     Instance pointer
 *  \param data_ptr Reference to optional data
 */
typedef void (*Sobs_UpdateCb_t)(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Standard return values of the subject class. */
typedef enum Sub_Ret_
{
    SUB_OK,                 /*!< \brief No error */
    SUB_DELAYED,            /*!< \brief Operation is queued since notification is still active */
    SUB_ALREADY_ADDED,      /*!< \brief Observer already added */
    SUB_UNKNOWN_OBSERVER,   /*!< \brief Unknown observer */
    SUB_INVALID_OPERATION   /*!< \brief Invalid operation */

} Sub_Ret_t;

/*! \brief Standard return values of the single-subject class. */
typedef enum Ssub_Ret_
{
    SSUB_OK,                /*!< \brief No error */
    SSUB_ALREADY_ADDED,     /*!< \brief Observer already added */
    SSUB_UNKNOWN_OBSERVER   /*!< \brief Unknown observer */

} Ssub_Ret_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Class structure of observers which are notified by subjects. */
typedef struct CObserver_
{
    CDlNode node;               /*!< \brief Node element to be able to add observer to list */
    void *inst_ptr;             /*!< \brief Reference to instance used by update_fptr() */
    Obs_UpdateCb_t update_fptr; /*!< \brief Callback function to update the observer */
    bool valid;                 /*!< \brief Used for queued remove operation */

} CObserver;

/*! \brief Class structure of subjects. */
typedef struct CSubject_
{
    CDlList list;               /*!< \brief Doubly linked list to manage observers */
    CDlList add_list;           /*!< \brief List to manage delayed add operations */
    uint8_t num_observers;      /*!< \brief Number of added observers */
    bool notify;                /*!< \brief Signals that the notification is in progress */
    bool changed;               /*!< \brief Signals that an add- or a remove-operation
                                            has been queued */
    void *ucs_user_ptr;         /*!< \brief User reference that needs to be passed in every callback function */

} CSubject;

/*! \brief Class structure of a single-observer which is notified by a single-subject. */
typedef struct CSingleObserver_
{
    void *inst_ptr;             /*!< \brief Reference to instance used by update_fptr() */
    Obs_UpdateCb_t update_fptr; /*!< \brief Callback function to update the observer */

} CSingleObserver;

/*! \brief Class structure of a single-subject. */
typedef struct CSingleSubject_
{
    CSingleObserver *observer_ptr;  /*!< \brief Reference to the assigned single-observer */
    void *ucs_user_ptr;         /*!< \brief User reference that needs to be passed in every callback function */
    uint32_t user_mask;             /*!< \brief Current user mask to the single observer */

} CSingleSubject;

/*! \brief Class structure of masked observers which are notified by subjects. */
typedef struct CMaskedObserver_
{
    CObserver parent;               /*!< \brief Parent class instance */
    uint32_t notification_mask;     /*!< \brief Notification bitmask */

} CMaskedObserver;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CSubject                                                                   */
/*------------------------------------------------------------------------------------------------*/
extern void Sub_Ctor(CSubject *self, void *ucs_user_ptr);
extern Sub_Ret_t Sub_AddObserver(CSubject *self, CObserver *obs_ptr);
extern Sub_Ret_t Sub_RemoveObserver(CSubject *self, CObserver *obs_ptr);
extern void Sub_Notify(CSubject *self, void *data_ptr);
extern uint8_t Sub_GetNumObservers(CSubject *self);
extern Sub_Ret_t Sub_SwitchObservers(CSubject *sub_target, CSubject *sub_source);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CObserver                                                                  */
/*------------------------------------------------------------------------------------------------*/
extern void Obs_Ctor(CObserver *self, void *inst_ptr, Obs_UpdateCb_t update_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CSingleSubject                                                             */
/*------------------------------------------------------------------------------------------------*/
extern void Ssub_Ctor(CSingleSubject *self, void *ucs_user_ptr);
extern Ssub_Ret_t Ssub_AddObserver(CSingleSubject *self, CSingleObserver *obs_ptr);
extern void Ssub_RemoveObserver(CSingleSubject *self);
extern void Ssub_Notify(CSingleSubject *self, void *data_ptr, bool auto_remove);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CSingleObserver                                                            */
/*------------------------------------------------------------------------------------------------*/
extern void Sobs_Ctor(CSingleObserver *self, void *inst_ptr, Sobs_UpdateCb_t update_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CMaskedObserver                                                            */
/*------------------------------------------------------------------------------------------------*/
extern void Mobs_Ctor(CMaskedObserver *self,
                      void *inst_ptr,
                      uint32_t notification_mask,
                      Obs_UpdateCb_t update_fptr);
extern void Mobs_SetNotificationMask(CMaskedObserver *self, uint32_t mask);
extern uint32_t Mobs_GetNotificationMask(CMaskedObserver *self);

/*------------------------------------------------------------------------------------------------*/
/* Additional prototypes of class CSubject used in combination with CMaskedObserver               */
/*------------------------------------------------------------------------------------------------*/
extern Sub_Ret_t Msub_AddObserver(CSubject *self, CMaskedObserver *obs_ptr);
extern Sub_Ret_t Msub_RemoveObserver(CSubject *self, CMaskedObserver *obs_ptr);
extern void Msub_Notify(CSubject *self, void *data_ptr, uint32_t notification_mask);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_OBS_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

