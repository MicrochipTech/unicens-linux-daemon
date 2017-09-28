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
 * \brief Internal header file of the doubly linked list.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DL
 * @{
 */

#ifndef UCS_DL_H
#define UCS_DL_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_types_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Callback signature used by foreach-function
 *  \param d_ptr    Reference to the data of the current node
 *  \param up_ptr   Reference to the user data
 *  \return true: Stop the for-each-loop
 *  \return false: Continue the for-each-loop
 */
typedef bool(*Dl_ForeachFunc_t)(void *d_ptr, void *ud_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Standard return values of the list module. */
typedef enum Dl_Ret_
{
    DL_OK,                      /*!< \brief No error */
    DL_UNKNOWN_NODE,            /*!< \brief Unknown node */
    DL_STOPPED                  /*!< \brief Search process stopped */

} Dl_Ret_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Class structure of doubly linked list node. */
typedef struct DlNode_
{
    struct DlNode_ *prev;       /*!< \brief Reference to previous node in list */
    struct DlNode_ *next;       /*!< \brief Reference to next node in list */
    void *data_ptr;             /*!< \brief Reference to optional data */
    bool in_use;                /*!< \brief Flag which signals that the node is in use */

} CDlNode;

/*! \brief Class structure of the doubly linked list. */
typedef struct CDlList_
{
    struct DlNode_ *head;       /*!< \brief Reference to head of the list */
    struct DlNode_ *tail;       /*!< \brief Reference to tail of the list */
    uint16_t size;              /*!< \brief Number of nodes in the list */
    void *ucs_user_ptr;         /*!< \brief User reference that needs to be passed in every callback function */

} CDlList;

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CDlList                                                                    */
/*------------------------------------------------------------------------------------------------*/
extern void Dl_Ctor(CDlList *self, void *ucs_user_ptr);
extern void Dl_InsertAfter(CDlList *self, CDlNode *node, CDlNode *new_node);
extern void Dl_InsertBefore(CDlList *self, CDlNode *node, CDlNode *new_node);
extern void Dl_InsertHead(CDlList *self, CDlNode *new_node);
extern void Dl_InsertTail(CDlList *self, CDlNode *new_node);
extern Dl_Ret_t Dl_Remove(CDlList *self, CDlNode *node);
extern CDlNode * Dl_PopHead(CDlList *self);
extern CDlNode * Dl_PopTail(CDlList *self);
extern CDlNode * Dl_PeekHead(CDlList *self);
extern CDlNode * Dl_PeekTail(CDlList *self);
extern CDlNode * Dl_Foreach(CDlList *self, Dl_ForeachFunc_t func_ptr, void *user_data_ptr);
extern bool Dl_IsNodeInList(CDlList *self, const CDlNode *node);
extern void Dl_AppendList(CDlList *self, CDlList *list_ptr);
extern uint16_t Dl_GetSize(CDlList *self);

/*------------------------------------------------------------------------------------------------*/
/* Prototypes of class CDlNode                                                                    */
/*------------------------------------------------------------------------------------------------*/
extern void Dln_Ctor(CDlNode *self, void *data_ptr);
extern void Dln_SetData(CDlNode *self, void *data_ptr);
extern void * Dln_GetData(CDlNode *self);
extern bool Dln_IsNodePartOfAList(CDlNode *self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_DL_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

