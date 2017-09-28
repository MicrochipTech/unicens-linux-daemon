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
 * \brief Implementation of CNodeObserver class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_NODEOBSERVER
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nodeobserver.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define NOBS_ADDR_ADMIN_MIN         0xF80U  /*!< \brief Start of address range to park unknown devices */
#define NOBS_ADDR_ADMIN_MAX         0xFDFU  /*!< \brief End of address range to park unknown devices */

#define NOBS_ADDR_RANGE1_MIN        0x200U  /*!< \brief Start of first static address range */
#define NOBS_ADDR_RANGE1_MAX        0x2FFU  /*!< \brief End of first static address range */
#define NOBS_ADDR_RANGE2_MIN        0x500U  /*!< \brief Start of second static address range */
#define NOBS_ADDR_RANGE2_MAX        0xEFFU  /*!< \brief End of second static address range */

#define NOSB_JOIN_NO                0x00U
#define NOSB_JOIN_WAIT              0x01U
#define NOSB_JOIN_YES               0x02U

#define NOBS_WAIT_TIME              200U    /*!< \brief Wait time between node not_available -> available */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nobs_OnInitComplete(void *self, void *error_code_ptr);
static void Nobs_OnWakeupTimer(void *self);
static bool Nobs_CheckAddrRange(CNodeObserver *self, Ucs_Signature_t *signature_ptr);
static void Nobs_InitAllNodes(CNodeObserver *self);
static void Nobs_InvalidateAllNodes(CNodeObserver *self);
static void Nobs_InvalidateNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr);
static Ucs_Rm_Node_t* Nobs_GetNodeBySignature(CNodeObserver *self, Ucs_Signature_t *signature_ptr);
static void Nobs_NotifyApp(CNodeObserver *self, Ucs_MgrReport_t code, uint16_t node_address, Ucs_Rm_Node_t *node_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of NodeObserver class 
 *  \param self         The instance
 *  \param base_ptr     Reference to base component
 *  \param nd_ptr       Reference to NodeDiscovery component
 *  \param rtm_ptr      Reference to RoutingManagement component
 *  \param init_ptr     Reference to initialization data
 */
void Nobs_Ctor(CNodeObserver *self, CBase *base_ptr, CNodeDiscovery *nd_ptr, CRouteManagement *rtm_ptr, Ucs_Mgr_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->base_ptr = base_ptr;
    self->nd_ptr = nd_ptr;
    self->rtm_ptr = rtm_ptr;
    if (init_ptr != NULL)
    {
        self->init_data = *init_ptr;
    }
    Nobs_InitAllNodes(self);
    T_Ctor(&self->wakeup_timer);

    Mobs_Ctor(&self->event_observer, self, EH_E_INIT_SUCCEEDED, &Nobs_OnInitComplete);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_observer);
}

/*! \brief  Callback function which is invoked if the initialization is complete
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Nobs_OnInitComplete(void *self, void *error_code_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    MISC_UNUSED(error_code_ptr);

    (void)Nd_Start(self_->nd_ptr);
    (void)Rtm_StartProcess(self_->rtm_ptr, self_->init_data.routes_list_ptr, self_->init_data.routes_list_size);
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Methods                                                                               */
/*------------------------------------------------------------------------------------------------*/
Ucs_Nd_CheckResult_t Nobs_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Ucs_Rm_Node_t *node_ptr = NULL;
    Ucs_Nd_CheckResult_t ret = UCS_ND_CHK_UNKNOWN;  /* ignore by default */

    if (signature_ptr != NULL)
    {
        if (Nobs_CheckAddrRange(self_, signature_ptr) != false)
        {
            node_ptr = Nobs_GetNodeBySignature(self_, signature_ptr);

            if (node_ptr != NULL)
            {
                if (node_ptr->internal_infos.mgr_joined == NOSB_JOIN_NO)
                {
                    ret = UCS_ND_CHK_WELCOME;       /* welcome new node */
                }
                else if (node_ptr->internal_infos.mgr_joined == NOSB_JOIN_YES)
                {
                    ret = UCS_ND_CHK_UNIQUE;        /* node already available - check for reset */
                }
                /* else if (node_ptr->internal_infos.mgr_joined == NOSB_JOIN_WAIT) --> ignore waiting nodes */
                /* future version compare node position to improve handling */
            }
        }

        self_->eval_signature = *signature_ptr;
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdEvaluate(): node=0x%03X, node_pos=0x%03X, ret=0x%02X", 2U, signature_ptr->node_address, signature_ptr->node_pos_addr, ret));
    }
    else
    {
        MISC_MEM_SET(&self_->eval_signature, 0, sizeof(self_->eval_signature));     /* reset signature */
        TR_FAILED_ASSERT(self_->base_ptr->ucs_user_ptr, "[NOBS]");                   /* signature missing - it is evident for evaluation */
    }

    self_->eval_node_ptr = node_ptr;
    self_->eval_action = ret;

    if ((ret == UCS_ND_CHK_UNKNOWN) && (signature_ptr != NULL))                      /* notify unknown node */
    {
        Nobs_NotifyApp(self_, UCS_MGR_REP_IGNORED_UNKNOWN, signature_ptr->node_address, NULL);
    }

    return ret;
}

void Nobs_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Ucs_Rm_Node_t *node_ptr = NULL;

    if (signature_ptr != NULL)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): code=0x%02X, node=0x%03X, node_pos=0x%03X", 3U, code, signature_ptr->node_address, signature_ptr->node_pos_addr));
        node_ptr = Nobs_GetNodeBySignature(self_, signature_ptr);
        if (node_ptr != self_->eval_node_ptr)       /* if signature available -> expecting the same node_ptr as previously announced in Nobs_OnNdEvaluate */
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): sanity check failed node_ptr=%p, eval_node_ptr=%p", 2U, node_ptr, self_->eval_node_ptr));
            node_ptr = NULL;                        /* do not handle node */
        }
    }
    else
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): code=0x%02X", 1U, code));
    }

    if (code == UCS_ND_RES_NETOFF)
    {
        Nobs_InvalidateAllNodes(self_);
    }
    else if (node_ptr == NULL)
    {
        /* no not handle events with unspecified node */
    }
    else if ((code == UCS_ND_RES_WELCOME_SUCCESS) && (self_->eval_action == UCS_ND_CHK_WELCOME))    /* is new node? */
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): welcome of new node=0x%03X", 1U, signature_ptr->node_address));
        node_ptr->internal_infos.mgr_joined = NOSB_JOIN_YES;
        (void)Rtm_SetNodeAvailable(self_->rtm_ptr, node_ptr, true);
        Nobs_NotifyApp(self_, UCS_MGR_REP_AVAILABLE, signature_ptr->node_address, node_ptr);
    }
    else if ((code == UCS_ND_RES_WELCOME_SUCCESS) && (self_->eval_action == UCS_ND_CHK_UNIQUE))     /* is node that previously joined */
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): welcome of existing node=0x%03X (RESET -> not_available)", 1U, signature_ptr->node_address));
        node_ptr->internal_infos.mgr_joined = NOSB_JOIN_WAIT;
        (void)Rtm_SetNodeAvailable(self_->rtm_ptr, node_ptr, false);
        Nobs_NotifyApp(self_, UCS_MGR_REP_NOT_AVAILABLE, signature_ptr->node_address, node_ptr);
        (void)Nd_Stop(self_->nd_ptr);                                                               /* stop node discovery and restart after timeout, */
        Tm_SetTimer(&self_->base_ptr->tm, &self_->wakeup_timer, &Nobs_OnWakeupTimer,                /* transition from node not_available -> available */
                    self,                                                                           /* needs some time and no callback is provided. */
                    NOBS_WAIT_TIME, 
                    0U
                    );
    }
    else if ((code == UCS_ND_RES_MULTI) && (self_->eval_action == UCS_ND_CHK_UNIQUE))               /* is node that causes address conflict */
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring duplicate node=0x%03X", 1U, signature_ptr->node_address));
        Nobs_NotifyApp(self_, UCS_MGR_REP_IGNORED_DUPLICATE, signature_ptr->node_address, NULL);
    }
    else if (code == UCS_ND_RES_UNKNOWN)
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring unknown node=0x%03X", 1U, signature_ptr->node_address));
    }
    else
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring node in unexpected, node=0x%03X, code=0x02X ", 2U, signature_ptr->node_address, code));
    }
}

static void Nobs_OnWakeupTimer(void *self)
{
    CNodeObserver *self_ = (CNodeObserver*)self;

    if (self_->eval_node_ptr != NULL)
    {
        if (self_->eval_node_ptr->internal_infos.mgr_joined == NOSB_JOIN_WAIT)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnWakeupTimer(): welcome of existing node 0x%03X (RESET -> available)", 1U, self_->eval_node_ptr->signature_ptr->node_address));
            self_->eval_node_ptr->internal_infos.mgr_joined = NOSB_JOIN_YES;
            (void)Rtm_SetNodeAvailable(self_->rtm_ptr, self_->eval_node_ptr, true);
            Nobs_NotifyApp(self_, UCS_MGR_REP_AVAILABLE, self_->eval_signature.node_address, self_->eval_node_ptr);
        }
    }
    (void)Nd_Start(self_->nd_ptr);
}


/*------------------------------------------------------------------------------------------------*/
/* Helper Methods                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Checks if the node address in signature is in supported address range
 *  \param  self           The instance
 *  \param  signature_ptr  Reference to the nodes signature
 *  \return Returns \c true if the address in signature is supported, otherwise \c false.
 */
static bool Nobs_CheckAddrRange(CNodeObserver *self, Ucs_Signature_t *signature_ptr)
{
    bool ret = false;

    if (((signature_ptr->node_address >= NOBS_ADDR_RANGE1_MIN) && (signature_ptr->node_address <= NOBS_ADDR_RANGE1_MAX)) ||
        ((signature_ptr->node_address >= NOBS_ADDR_RANGE2_MIN) && (signature_ptr->node_address <= NOBS_ADDR_RANGE2_MAX)))
    {
        ret = true;
    }
    MISC_UNUSED(self);

    return ret;
}

static void Nobs_InitAllNodes(CNodeObserver *self)
{
    if (self->init_data.nodes_list_ptr != NULL)
    {
        uint32_t cnt = 0U;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            self->init_data.nodes_list_ptr[cnt].internal_infos.available = 0U;
            self->init_data.nodes_list_ptr[cnt].internal_infos.mgr_joined = NOSB_JOIN_NO;
        }
    }
}

static void Nobs_InvalidateAllNodes(CNodeObserver *self)
{
    if (self->init_data.nodes_list_ptr != NULL)
    {
        uint32_t cnt = 0U;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            Nobs_InvalidateNode(self, &self->init_data.nodes_list_ptr[cnt]);
        }
    }
}

static void Nobs_InvalidateNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr)
{
    if (node_ptr->internal_infos.mgr_joined == NOSB_JOIN_YES)       /* notify welcomed nodes as invalid */
    {
        Nobs_NotifyApp(self, UCS_MGR_REP_NOT_AVAILABLE, node_ptr->signature_ptr->node_address, node_ptr);
    }

    node_ptr->internal_infos.mgr_joined = NOSB_JOIN_NO;
    /* RoutingManagement individually cares for network-not-available event */
    /* (void)Rtm_SetNodeAvailable(self->rtm_ptr, &self->init_data.nodes_list_ptr[cnt], false); */
}

static Ucs_Rm_Node_t* Nobs_GetNodeBySignature(CNodeObserver *self, Ucs_Signature_t *signature_ptr)
{
    Ucs_Rm_Node_t* ret = NULL;

    if ((signature_ptr != NULL) && (self->init_data.nodes_list_ptr != NULL))
    {
        uint32_t cnt = 0U;
        uint16_t node_addr = signature_ptr->node_address;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            if (node_addr == self->init_data.nodes_list_ptr[cnt].signature_ptr->node_address)
            {
                ret = &self->init_data.nodes_list_ptr[cnt];
                break;
            }
        }
    }

    return ret;
}

static void Nobs_NotifyApp(CNodeObserver *self, Ucs_MgrReport_t code, uint16_t node_address, Ucs_Rm_Node_t *node_ptr)
{
    if (self->init_data.report_fptr != NULL)
    {
        self->init_data.report_fptr(code, node_address, node_ptr, self->base_ptr->ucs_user_ptr);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

