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
 * \brief Implementation of the doubly linked list.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DL
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_dl.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CDlList                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the doubly linked list class.
 *  \param self         Instance pointer
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
void Dl_Ctor(CDlList *self, void *ucs_user_ptr)
{
    self->head = NULL;
    self->tail = NULL;
    self->size = 0U;
    self->ucs_user_ptr = ucs_user_ptr;
}

/*! \brief Inserts a new node after an arbitrary node.
 *  \param self       Instance pointer
 *  \param node       Reference of the initial node
 *  \param new_node   Reference of the new node are to be inserted
 */
void Dl_InsertAfter(CDlList *self, CDlNode *node, CDlNode *new_node)
{
    TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size <= 0xFFFFU));
    new_node->prev = node;
    new_node->next = node->next;
    if(node->next == NULL)                      /* Is initial node last node in list? */
    {
        self->tail = new_node;                  /* Set new node as tail of list */
    }
    else
    {
        node->next->prev = new_node;            /* Adjust follower node */
    }
    node->next = new_node;                      /* Adjust parent node */
    new_node->in_use = true;                    /* Signals that node is part of a list */
    self->size++;                               /* Increment number of nodes */
}

/*! \brief Inserts a new node before an arbitrary node.
 *  \param self       Instance pointer
 *  \param node       Reference of the initial node
 *  \param new_node   Reference of the new node are to be inserted
 */
void Dl_InsertBefore(CDlList *self, CDlNode *node, CDlNode *new_node)
{
    TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size <= 0xFFFFU));
    new_node->prev = node->prev;
    new_node->next = node;
    if(node->prev == NULL)                      /* Is initial node first node in list? */
    {
        self->head = new_node;                  /* Set new node as head of list */
    }
    else
    {
        node->prev->next = new_node;            /* Adjust parent node */
    }
    node->prev = new_node;                      /* Adjust follower node */
    new_node->in_use = true;                    /* Signals that node is part of a list */
    self->size++;                               /* Increment number of nodes */
}

/*! \brief Sets the new node as head of a doubly linked list.
 *  \param self       Instance pointer
 *  \param new_node   Reference of the new node are to be placed as head of the list
 */
void Dl_InsertHead(CDlList *self, CDlNode *new_node)
{
    if(self->head == NULL)                      /* Is list empty? */
    {
        TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size <= 0xFFFFU));
        self->head = new_node;
        self->tail = new_node;
        new_node->prev = NULL;
        new_node->next = NULL;
        new_node->in_use = true;                /* Signals that node is part of a list */
        self->size++;                           /* Increment number of nodes */
    }
    else
    {
        Dl_InsertBefore(self, self->head, new_node);
    }
}

/*! \brief Inserts the new node at the end of a doubly linked list.
 *  \param self       Instance pointer
 *  \param new_node   Reference of the new node are to be placed at the end of the list
 */
void Dl_InsertTail(CDlList *self, CDlNode *new_node)
{
    if(self->tail == NULL)                      /* Is list empty? */
    {
        Dl_InsertHead(self, new_node);
    }
    else
    {
        Dl_InsertAfter(self, self->tail, new_node);
    }
}

/*! \brief  Removes an arbitrary node from a doubly linked list.
 *  \param  self   Instance pointer
 *  \param  node   Reference of the node are to be removed from the list
 *  \return \c DL_OK: No error
 *  \return \c DL_UNKNOWN_NODE: Given node is not part of this list
 */
Dl_Ret_t Dl_Remove(CDlList *self, CDlNode *node)
{
    Dl_Ret_t ret_val = DL_UNKNOWN_NODE;

    if(Dl_IsNodeInList(self, node) != false)    /* Is node part of list? */
    {
        TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size > 0U));
        if(node->prev == NULL)                  /* First node in list? */
        {
            self->head = node->next;            /* Replace head node with next node in list */
        }
        else                                    /* -> Not first node in list  */
        {
            node->prev->next = node->next;      /* Set next pointer of previous node to next node */
        }
        if(node->next == NULL)                  /* Last node in list? */
        {
            self->tail = node->prev;            /* Replace tail node with previous node in list */
        }
        else                                    /* -> Not last node in list */
        {
            node->next->prev = node->prev;      /* Set previous ptr of next node to previous node */
        }
        node->prev = NULL;
        node->next = NULL;
        node->in_use = false;                   /* Signals that node is not part of a list */
        ret_val = DL_OK;
        self->size--;                           /* Decrement number of nodes */
    }

    return ret_val;
}

/*! \brief  Removes the first node in a doubly linked list.
 *  \param  self   Instance pointer
 *  \return The reference of the removed head node or \c NULL if the list is empty.
 */
CDlNode * Dl_PopHead(CDlList *self)
{
    CDlNode *node = self->head; 

    if(node != NULL)                             /* Is list not empty? */
    {
        TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size > 0U));
        self->head = node->next;                /* Replace head node with next node in list */
        if(node->next == NULL)                  /* Last node in list? */
        {
            self->tail = NULL;                  /* Replace tail node and set list's tail pointer 
                                                 *  to NULL
                                                 */
        }
        else                                    /* -> Not last node in list */
        {
            node->next->prev = NULL;            /* Set previous pointer of next node to NULL */
        }
        node->prev = NULL;
        node->next = NULL;
        node->in_use = false;                   /* Signals that node is not part of a list */
        self->size--;                           /* Decrement number of nodes */
    }

    return node;
}

/*! \brief  Removes the last node in a doubly linked list.
 *  \param  self   Instance pointer
 *  \return The reference of the removed tail node or \c NULL if the list is empty.
 */
CDlNode * Dl_PopTail(CDlList *self)
{
    CDlNode *node = self->tail;

    if(node != NULL)                             /* Is list not empty? */
    {
        TR_ASSERT(self->ucs_user_ptr, "[DL]", (self->size > 0U));
        if(node->prev == NULL)                  /* First node in list? */
        {
            self->head = NULL;                  /* Replace head node and set list's head pointer 
                                                 * to NULL
                                                 */
        }
        else                                    /* -> Not first node in list  */
        {
            node->prev->next = NULL;            /* Set next pointer of previous node to NULL */
        }
        self->tail = node->prev;                /* Replace tail node with previous node in list */
        node->prev = NULL;
        node->next = NULL;
        node->in_use = false;                   /* Signals that node is not part of a list */
        self->size--;                           /* Decrement number of nodes */
    }

    return node;
}

/*! \brief  Returns the reference of the first node in a doubly linked list.
 *  \param  self   Instance pointer
 *  \return The reference of the head node or \c NULL if the list is empty.
 */
CDlNode * Dl_PeekHead(CDlList *self)
{
    return self->head;
}

/*! \brief  Returns the reference of the last node in a doubly linked list.
 *  \param  self   Instance pointer
 *  \return The reference of the tail node or NULL if the list is empty.
 */
CDlNode * Dl_PeekTail(CDlList *self)
{
    return self->tail;
}

/*! \brief  Calls the given function for each node in the doubly linked list. If the func_ptr 
 *          returns true the loop is stopped and the current node will be returned.
 *  \param  self           Instance pointer
 *  \param  func_ptr       Reference of the callback function which is called for each node
 *  \param  user_data_ptr  Reference of optional user data given to func_ptr
 *  \return Returns the current node or \c NULL if the whole list is processed.
 */
CDlNode * Dl_Foreach(CDlList *self, Dl_ForeachFunc_t func_ptr, void *user_data_ptr)
{
    CDlNode *ret_val = NULL;
    CDlNode *node = self->head;

    while(node != NULL)                                          /* End of list reached? */
    {
        if(func_ptr(node->data_ptr, user_data_ptr) != false)    /* Data found? */
        {
            ret_val = node;
            break;
        }
        node = node->next;
    }
    return ret_val;
}

/*! \brief  Checks if a node is part of the given doubly linked list.
 *  \param  self   Instance pointer
 *  \param  node   Reference of the searched node
 *  \return \c true: Node is part of the given list
 *  \return \c false: Node is not part of the given list
 */
bool Dl_IsNodeInList(CDlList *self, const CDlNode *node)
{
    bool ret_val = false;
    CDlNode *current_node = self->head;

    while(current_node != NULL)                  /* End of list reached? */
    {
        if(current_node == node)                /* Is current node the searched one */
        {
            ret_val = true;
            break;
        }
        current_node = current_node->next;
    }
    return ret_val;
}

/*! \brief Appends one doubly linked list to another doubly linked list.
 *  \param self       Instance pointer
 *  \param list_ptr   Reference to the doubly linked list
 */
void Dl_AppendList(CDlList *self, CDlList *list_ptr)
{
    TR_ASSERT(self->ucs_user_ptr, "[DL]", (list_ptr != NULL));
    if(list_ptr->head != NULL)
    {
        if(self->tail == NULL)             /* Is list empty? */
        {
            self->head = list_ptr->head;
            self->tail = list_ptr->tail;
            self->size = list_ptr->size;
        }
        else
        {
            list_ptr->head->prev = self->tail;
            self->tail->next = list_ptr->head;
            self->tail = list_ptr->tail;
            self->size += list_ptr->size;
        }
        list_ptr->head = NULL;
        list_ptr->tail = NULL;
        list_ptr->size = 0U;
    }
}

/*! \brief  Interface function to retrieve the list size.
 *  \param  self   Instance pointer
 *  \return Size of the list
 */
uint16_t Dl_GetSize(CDlList *self)
{
    return self->size;
}


/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CDlNode                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of doubly linked list nodes.
 *  \param self        Instance pointer
 *  \param data_ptr    Optional reference to data
 */
void Dln_Ctor(CDlNode *self, void *data_ptr)
{
    self->next = NULL;
    self->prev = NULL;
    self->in_use = false;
    self->data_ptr = data_ptr;
}

/*! \brief Interface function to set the data pointer of the given node.
 *  \param self        Instance pointer
 *  \param data_ptr    Reference of the new data
 */
void Dln_SetData(CDlNode *self, void *data_ptr)
{
    self->data_ptr = data_ptr;
}

/*! \brief Interface function to request the data pointer of the given node.
 *  \param self       Instance pointer
 */
void * Dln_GetData(CDlNode *self)
{
    return self->data_ptr;
}

/*! \brief  Checks if a node is part of a doubly linked list.
 *  \param  self   Instance pointer of the searched node
 *  \return \c true:  Node is part of a list
 *  \return \c false: Node is not part of a list
 */
bool Dln_IsNodePartOfAList(CDlNode *self)
{
    return self->in_use;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

