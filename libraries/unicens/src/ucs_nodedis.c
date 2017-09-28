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
 * \brief Implementation of the Node Discovery.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NODE_DIS
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic_pb.h"
#include "ucs_nodedis.h"
#include "ucs_misc.h"



/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define ND_NUM_STATES             5U    /*!< \brief Number of state machine states */
#define ND_NUM_EVENTS            14U    /*!< \brief Number of state machine events */

#define ND_TIMEOUT_PERIODIC     5000U   /*!< \brief 5s timeout */
#define ND_TIMEOUT_WELCOME       100U   /*!< \brief Supervises EXC.Welcome.StartResult command */
#define ND_TIMEOUT_SIGNATURE     300U   /*!< \brief Supervises EXC.Signature.Get command, takes 
                                                    LLRs into account. */
#define ND_TIMEOUT_DEBOUNCE      200U   /*!< \brief Prevents Hello.Get being sent while waiting for 
                                                    answers of a previous Hello.Get command. */

#define ND_SIGNATURE_VERSION       1U   /*!< \brief Signature version used for Node Discovery. */

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the Node Discovery service used by scheduler */
static const uint8_t ND_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the Node Discovery service */
static const Srv_Event_t ND_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the Node Discovery state machine */
typedef enum Nd_Events_
{
    ND_E_NIL                = 0U,      /*!< \brief NIL Event */
    ND_E_START              = 1U,      /*!< \brief API start command was called. */
    ND_E_STOP               = 2U,      /*!< \brief Stop request occurred. */
    ND_E_CHECK              = 3U,      /*!< \brief Check conditions in CHECK_HELLO state. */ 
    ND_E_NET_OFF            = 4U,      /*!< \brief NetOff occurred. */
    ND_E_HELLO_STATUS       = 5U,      /*!< \brief Hello.Status message available to be processed. */
    ND_E_RES_NODE_OK        = 6U,      /*!< \brief Evaluation result of node: ok. */
    ND_E_RES_UNKNOWN        = 7U,      /*!< \brief Evaluation result of node: unknown node. */
    ND_E_RES_CHECK_UNIQUE   = 8U,      /*!< \brief Evaluation result of node: check if node is unique. */
    ND_E_WELCOME_SUCCESS    = 9U,      /*!< \brief Welcome command was successful. */
    ND_E_WELCOME_NOSUCCESS  = 10U,     /*!< \brief Welcome command was not successful. */
    ND_E_SIGNATURE_SUCCESS  = 11U,     /*!< \brief Signature command was successful. */
    ND_E_TIMEOUT            = 12U,     /*!< \brief Timeout occurred. */
    ND_E_ERROR              = 13U      /*!< \brief An unexpected error occurred. */ 

} Nd_Events_t;


/*! \brief States of the Node Discovery state machine */
typedef enum Nd_State_
{
    ND_S_IDLE            =  0U,     /*!< \brief Idle state */
    ND_S_CHECK_HELLO     =  1U,     /*!< \brief Node Discovery started */
    ND_S_WAIT_EVAL       =  2U,     /*!< \brief Evaluate next Hello.Status message */
    ND_S_WAIT_WELCOME    =  3U,     /*!< \brief Wait for Welcome.Status */
    ND_S_WAIT_PING       =  4U      /*!< \brief Wait for Signature.Status */
} Nd_State_t;




/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nd_Service(void *self);

static void Nd_HelloStatusCb(void *self, void *result_ptr);
static void Nd_WelcomeResultCb(void *self, void *result_ptr);
static void Nd_SignatureStatusCb(void *self, void *result_ptr);
static void Nd_InitCb(void *self, void *result_ptr);
static void Nd_TimerCb(void *self);
static void Nd_DebounceTimerCb(void *self);
static void Nd_OnTerminateEventCb(void *self, void *result_ptr);
static void Nd_NetworkStatusCb(void *self, void *result_ptr);

static void Nd_Reset_Lists(void *self);

static void Nd_A_Start(void *self);
static void Nd_A_Stop(void *self);
static void Nd_A_CheckConditions(void *self);
static void Nd_A_Eval_Hello(void *self);
static void Nd_A_Welcome(void *self);
static void Nd_A_Unknown(void *self);
static void Nd_A_CheckUnique(void *self);
static void Nd_A_WelcomeSuccess(void *self);
static void Nd_A_WelcomeNoSuccess(void *self);
static void Nd_A_WelcomeTimeout(void *self);
static void Nd_A_Timeout_Hello(void *self);
static void Nd_A_NetOff(void *self);
static void Nd_A_Signature_Timeout(void *self);
static void Nd_A_Signature_Success(void *self);
static void Nd_A_Error(void *self);

static void Nd_Send_Hello_Get(void *self);
static void Nd_Start_Periodic_Timer(void *self);
static void Nd_Start_Debounce_Timer(void *self);
static void Nd_Send_Welcome_SR(void *self, Ucs_Signature_t *signature);
static void Nd_Send_Signature_Get(void *self, uint16_t target_address);


/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t nd_trans_tab[ND_NUM_STATES][ND_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State ND_S_IDLE */
        /* ND_E_NIL                */ {NULL,                          ND_S_IDLE            },
        /* ND_E_START              */ {&Nd_A_Start,                   ND_S_CHECK_HELLO     },
        /* ND_E_STOP               */ {NULL,                          ND_S_IDLE            },
        /* ND_E_CHECK              */ {NULL,                          ND_S_IDLE            },
        /* ND_E_NET_OFF            */ {NULL,                          ND_S_IDLE            },
        /* ND_E_HELLO_STATUS       */ {NULL,                          ND_S_IDLE            },
        /* ND_E_RES_NODE_OK        */ {NULL,                          ND_S_IDLE            },
        /* ND_E_RES_UNKNOWN        */ {NULL,                          ND_S_IDLE            },
        /* ND_E_RES_CHECK_UNIQUE   */ {NULL,                          ND_S_IDLE            },
        /* ND_E_WELCOME_SUCCESS    */ {NULL,                          ND_S_IDLE            },
        /* ND_E_WELCOME_NOSUCCESS  */ {NULL,                          ND_S_IDLE            },
        /* ND_E_SIGNATURE_SUCCESS  */ {NULL,                          ND_S_IDLE            },
        /* ND_E_TIMEOUT            */ {NULL,                          ND_S_IDLE            },
        /* ND_E_ERROR              */ {NULL,                          ND_S_IDLE            }

    },
    { /* State ND_S_CHECK_HELLO */
        /* ND_E_NIL                */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_START              */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_STOP               */ {&Nd_A_Stop,                    ND_S_IDLE            },
        /* ND_E_CHECK              */ {&Nd_A_CheckConditions,         ND_S_CHECK_HELLO     },
        /* ND_E_NET_OFF            */ {&Nd_A_NetOff,                  ND_S_CHECK_HELLO     },
        /* ND_E_HELLO_STATUS       */ {&Nd_A_Eval_Hello,              ND_S_WAIT_EVAL       },
        /* ND_E_RES_NODE_OK        */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_RES_UNKNOWN        */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_RES_CHECK_UNIQUE   */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_WELCOME_SUCCESS    */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_WELCOME_NOSUCCESS  */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_SIGNATURE_SUCCESS  */ {NULL,                          ND_S_CHECK_HELLO     },
        /* ND_E_TIMEOUT            */ {&Nd_A_Timeout_Hello,           ND_S_CHECK_HELLO     },
        /* ND_E_ERROR              */ {&Nd_A_Error,                   ND_S_IDLE            }
    },
    { /* State ND_S_WAIT_EVAL */
        /* ND_E_NIL                */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_START              */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_STOP               */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_CHECK              */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_NET_OFF            */ {&Nd_A_NetOff,                  ND_S_CHECK_HELLO     },
        /* ND_E_HELLO_STATUS       */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_RES_NODE_OK        */ {&Nd_A_Welcome,                 ND_S_WAIT_WELCOME    },
        /* ND_E_RES_UNKNOWN        */ {&Nd_A_Unknown,                 ND_S_CHECK_HELLO     },
        /* ND_E_RES_CHECK_UNIQUE   */ {&Nd_A_CheckUnique,             ND_S_WAIT_PING       },
        /* ND_E_WELCOME_SUCCESS    */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_WELCOME_NOSUCCESS  */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_SIGNATURE_SUCCESS  */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_TIMEOUT            */ {NULL,                          ND_S_WAIT_EVAL       },
        /* ND_E_ERROR              */ {&Nd_A_Error,                   ND_S_IDLE            }
    },                             

    {/* ND_S_WAIT_WELCOME */       
        /* ND_E_NIL                */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_START              */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_STOP               */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_CHECK              */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_NET_OFF            */ {&Nd_A_NetOff,                  ND_S_CHECK_HELLO     },
        /* ND_E_HELLO_STATUS       */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_RES_NODE_OK        */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_RES_UNKNOWN        */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_RES_CHECK_UNIQUE   */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_WELCOME_SUCCESS    */ {&Nd_A_WelcomeSuccess,          ND_S_CHECK_HELLO     },
        /* ND_E_WELCOME_NOSUCCESS  */ {&Nd_A_WelcomeNoSuccess,        ND_S_CHECK_HELLO     },
        /* ND_E_SIGNATURE_SUCCESS  */ {NULL,                          ND_S_WAIT_WELCOME    },
        /* ND_E_TIMEOUT            */ {&Nd_A_WelcomeTimeout,          ND_S_CHECK_HELLO     },
        /* ND_E_ERROR              */ {&Nd_A_Error,                   ND_S_IDLE            }
    },                             
    {/* ND_S_WAIT_PING */          
        /* ND_E_NIL                */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_START              */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_STOP               */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_CHECK              */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_NET_OFF            */ {&Nd_A_NetOff,                  ND_S_CHECK_HELLO     },
        /* ND_E_HELLO_STATUS       */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_RES_NODE_OK        */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_RES_UNKNOWN        */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_RES_CHECK_UNIQUE   */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_WELCOME_SUCCESS    */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_WELCOME_NOSUCCESS  */ {NULL,                          ND_S_WAIT_PING       },
        /* ND_E_SIGNATURE_SUCCESS  */ {&Nd_A_Signature_Success,       ND_S_CHECK_HELLO     },
        /* ND_E_TIMEOUT            */ {&Nd_A_Signature_Timeout,       ND_S_WAIT_WELCOME    },
        /* ND_E_ERROR              */ {&Nd_A_Error,                   ND_S_IDLE            }
    }
};




/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of class CNodeDiscovery.
 *  \param self         Reference to CNodeDiscovery instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 *  \param init_ptr    Report callback function
 */
void Nd_Ctor(CNodeDiscovery *self, CInic *inic, CBase *base, CExc *exc, Nd_InitData_t *init_ptr)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic       = inic;
    self->exc        = exc;
    self->base       = base;
    self->cb_inst_ptr = init_ptr->inst_ptr;
    self->report_fptr = init_ptr->report_fptr;
    self->eval_fptr   = init_ptr->eval_fptr;

    Fsm_Ctor(&self->fsm, self, &(nd_trans_tab[0][0]), ND_NUM_EVENTS, ND_E_NIL);

    Nd_Reset_Lists(self);

    Sobs_Ctor(&self->nd_hello,          self, &Nd_HelloStatusCb);
    Sobs_Ctor(&self->nd_welcome,        self, &Nd_WelcomeResultCb);
    Sobs_Ctor(&self->nd_signature,      self, &Nd_SignatureStatusCb);
    Sobs_Ctor(&self->nd_init,           self, &Nd_InitCb);

    /* register termination events */
    Mobs_Ctor(&self->nd_terminate, self, EH_M_TERMINATION_EVENTS, &Nd_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->nd_terminate);

    /* Register NetOn and MPR events */
    Obs_Ctor(&self->nd_nwstatus, self, &Nd_NetworkStatusCb);
    Inic_AddObsrvNwStatus(self->inic,  &self->nd_nwstatus);
    self->neton = false;

    /* Initialize Node Discovery service */
    Srv_Ctor(&self->service, ND_SRV_PRIO, self, &Nd_Service);
    /* Add Node Discovery service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the Node Discovery service.
 *  \param self    Reference to Node Discovery object
 */
static void Nd_Service(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if(ND_EVENT_SERVICE == (event_mask & ND_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, ND_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "FSM __ %d %d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[ND]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "FSM -> %d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}



/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*! \brief Start the Node Discovery
 *
 * \param  *self    Reference to Node Discovery object
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_API_LOCKED       Node Discovery was already started
 */
Ucs_Return_t Nd_Start(CNodeDiscovery *self)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;


    if (self->running == false)
    {
        Fsm_SetEvent(&self->fsm, ND_E_START);
        Srv_SetEvent(&self->service, ND_EVENT_SERVICE);
        self->running = true;
        self->debounce_flag = false;
        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_Start", 0U));
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_Start failed", 0U));
    }

    return ret_val;




}

/*! \brief Stops the Node Discovery
 *
 * \param *self Reference to Node Discovery object
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_NOT_AVAILABLE    Node Discovery not running
 */
Ucs_Return_t Nd_Stop(CNodeDiscovery *self)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->running == true)       /* check if Node Discovery was started */
    {
        self->stop_request = true;
        Fsm_SetEvent(&self->fsm, ND_E_CHECK);
        Srv_SetEvent(&self->service, ND_EVENT_SERVICE);

        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_Stop", 0U));
    }
    else
    {
        ret_val = UCS_RET_ERR_NOT_AVAILABLE;
        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_Stop failed", 0U));
    }

    return ret_val;
}


/*! \brief Sends the Init command to all nodes 
 *
 * \param *self Reference to Node Discovery object
 */
void Nd_InitAll(CNodeDiscovery *self)
{
    Ucs_Return_t result;

    result = Exc_DeviceInit_Start(self->exc, UCS_ADDR_BROADCAST_BLOCKING, NULL);
    if (result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_InitAll", 0U));
    }
    else
    {
        TR_INFO((self->base->ucs_user_ptr, "[ND]", "Nd_InitAll failed", 0U));
    }

}




/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
/*! \brief Action on start event
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Start(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    /* empty new_list*/
    Nd_Reset_Lists(self_);

    Nd_Send_Hello_Get(self_);

    Nd_Start_Periodic_Timer(self_);
    
    self_->stop_request  = false;
    self_->hello_mpr_request   = false;
    self_->hello_neton_request = false;
}

/*! \brief Action on stop event
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Stop(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t *dummy = NULL;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_STOPPED, dummy);
    }
    self_->running = false;
}

/*! \brief Check conditions 
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_CheckConditions(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    if (self_->stop_request == true)
    {
        Fsm_SetEvent(&self_->fsm, ND_E_STOP);
        Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
    }
    else if (   (self_->hello_mpr_request == true)
             && (self_->debounce_flag     == false) )
    {
        Nd_Reset_Lists(self_);
        Nd_Send_Hello_Get(self_);
        Nd_Start_Periodic_Timer(self_);
        self_->hello_mpr_request   = false;
        self_->hello_neton_request = false;
    }
    else if (   (self_->hello_neton_request == true)
             && (self_->debounce_flag       == false) )
    {
        Nd_Send_Hello_Get(self_);
        Nd_Start_Periodic_Timer(self_);
        self_->hello_neton_request = false;
    }
    else if (Dl_GetSize(&(self_->new_list)) > 0U)
    {
        Fsm_SetEvent(&self_->fsm, ND_E_HELLO_STATUS);
        Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
    }
    else
    {
        Nd_Start_Periodic_Timer(self_);
    }

}


/*! \brief Evaluate the signature of the next node
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Eval_Hello(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    CDlNode *node;
    Ucs_Nd_CheckResult_t result;
    bool service_flag = false;
    Ucs_Signature_t temp_sig;

    if (Dl_GetSize(&(self_->new_list)) > 0U)
    {
        node = Dl_PopHead(&(self_->new_list));
        self_->current_sig = *((Ucs_Signature_t *)(node->data_ptr));
        Dl_InsertTail(&(self_->unused_list), node);

        if (self_->eval_fptr != NULL)
        {
            temp_sig = self_->current_sig;             /* provide only a copy to the application */
            result = self_->eval_fptr(self_->cb_inst_ptr, &temp_sig);

            switch (result)
            {
            case UCS_ND_CHK_UNKNOWN:
                Fsm_SetEvent(&self_->fsm, ND_E_RES_UNKNOWN);
                service_flag = true;
                break;

            case UCS_ND_CHK_WELCOME:
                Fsm_SetEvent(&self_->fsm, ND_E_RES_NODE_OK);
                service_flag = true;
                break;

            case UCS_ND_CHK_UNIQUE:
                Fsm_SetEvent(&self_->fsm, ND_E_RES_CHECK_UNIQUE);
                service_flag = true;
                break;

            default: 
                Fsm_SetEvent(&self_->fsm, ND_E_ERROR);
                service_flag = true;
                break;
            }
        }
    }

    if (service_flag == true)
    {
        Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
    }
}


/*! \brief Sends a Welcome message to the current node
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Welcome(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Nd_Send_Welcome_SR(self, &self_->current_sig);
}


/*! \brief Report the current node as unknown
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Unknown(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t temp_sig;

    if (self_->report_fptr != NULL)
    {
        temp_sig = self_->current_sig;                 /* provide only a copy to the application */
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_UNKNOWN, &temp_sig);
    }

    Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}

/*! \brief Check if the current node has already got a Welcome message
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_CheckUnique(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Nd_Send_Signature_Get(self, self_->current_sig.node_address);

}


/*! \brief Report a successful Welcome.Result
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_WelcomeSuccess(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t temp_sig;

    if (self_->report_fptr != NULL)
    {
        temp_sig = self_->current_sig;                 /* provide only a copy to the application */
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_WELCOME_SUCCESS, &temp_sig);
    }

    /* initiate a Hello.Get if the current node is the local INIC */
    if (self_->current_sig.node_pos_addr == 0x0400U)
    {
        Nd_Send_Hello_Get(self_);
        Nd_Start_Periodic_Timer(self_);
    }

    Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief Report an unsuccessful Welcome.Result
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_WelcomeNoSuccess(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    /* same reaction as for MPR event */
    self_->hello_mpr_request = true;

    Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief Reaction on a timeout for the Welcome messsage
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_WelcomeTimeout(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    /* same reaction as for MPR event */
    self_->hello_mpr_request = true;

    Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief The periodic timer elapsed
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Timeout_Hello(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Nd_Send_Hello_Get(self_);
    Nd_Start_Periodic_Timer(self_);
}


/*! \brief  Reaction on a NetOff event
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_NetOff(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t *dummy = NULL;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_NETOFF, dummy);
    }

    Nd_Reset_Lists(self_);

}


/*! \brief Reaction on a timeout of the Signature command
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Signature_Timeout(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Nd_Send_Welcome_SR(self, &self_->current_sig);
}


/*! \brief Reaction on a successful Signature answer
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Signature_Success(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t temp_sig;

    if (self_->report_fptr != NULL)
    {
        temp_sig = self_->current_sig;                 /* provide only a copy to the application */
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_MULTI, &temp_sig);
    }
}


/*! \brief An unecpected error occurred
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_A_Error(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t *dummy = NULL;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_ERROR, dummy);
    }
    self_->running = false;
}


/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! Callback function for the Exc.Hello.Status message
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Result of the Exc_Hello_Get() command
 */
static void Nd_HelloStatusCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_        = (CNodeDiscovery *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;
    CDlNode *node;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it in the new_list */
        node = Dl_PopHead(&(self_->unused_list)); /* get an unused list element */
        if (node != NULL)
        {
            ((Nd_Node *)(node->data_ptr))->signature = (*(Exc_HelloStatus_t *)(result_ptr_->data_info)).signature;
            Dl_InsertTail(&(self_->new_list), node);

            Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
            TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_HelloStatusCb UCS_RES_SUCCESS", 0U));
        }
        else
        {
            TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_HelloStatusCb No list entry av.", 0U));
        }
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, ND_E_ERROR); 
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_HelloStatusCb ND_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the Welcome.Result messsage
 *  \param  self        Reference to Node Discovery object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Nd_WelcomeResultCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_        = (CNodeDiscovery *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it */
        self_->welcome_result = *(Exc_WelcomeResult_t *)(result_ptr_->data_info);
        if (self_->welcome_result.res == EXC_WELCOME_SUCCESS)
        {
            Fsm_SetEvent(&self_->fsm, ND_E_WELCOME_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_WelcomeResultCb ND_E_WELCOME_SUCCESS", 0U));
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, ND_E_WELCOME_NOSUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_WelcomeResultCb ND_E_WELCOME_NOSUCCESS", 0U));
        }
    }
    else
    {
        uint8_t i;

        if (    (result_ptr_->result.info_size   == 3U)
             && (result_ptr_->result.info_ptr[0] == 0x20U)
             && (result_ptr_->result.info_ptr[1] == 0x03U)
             && (result_ptr_->result.info_ptr[2] == 0x31U))
        {   /* Device has not yet received an ExtendedNetworkControl.Hello.Get() message. */
            Fsm_SetEvent(&self_->fsm, ND_E_WELCOME_NOSUCCESS);
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, ND_E_ERROR);
            TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_WelcomeResultCb Error (code) 0x%x", 1U, result_ptr_->result.code));
            for (i=0U; i< result_ptr_->result.info_size; ++i)
            {
                TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_WelcomeResultCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
            }
        }
    }

    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief Callback function for Signature status and error messages
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Pointer to the result of the Signature message
 */
static void Nd_SignatureStatusCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_        = (CNodeDiscovery *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->signature_status = *(Exc_SignatureStatus_t *)(result_ptr_->data_info);
        Fsm_SetEvent(&self_->fsm, ND_E_SIGNATURE_SUCCESS);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_SignatureStatusCb ND_E_SIGNATURE_SUCCESS", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, ND_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_SignatureStatusCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief Callback function for Init error messages
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Pointer to the result of the Init message
 */
static void Nd_InitCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_        = (CNodeDiscovery *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    MISC_UNUSED(self_);
    MISC_UNUSED(result_ptr_);

}


/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to Node Discovery object
 */
static void Nd_TimerCb(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Fsm_SetEvent(&self_->fsm, ND_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_TimerCb ND_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}

/*! \brief       Timer callback used for debouncinng Hello.Get request.
 *  \details     Prevents that a Hello.Get is sent while waiting for answers of a previous Hello.Get command.
 *  \param self  Reference to Node Discovery object
 */
static void Nd_DebounceTimerCb(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    self_->debounce_flag = false;
    TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_DebounceTimerCb", 0U));

    Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}


/*! \brief Function is called on severe internal errors
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Reference to data
 */
static void Nd_OnTerminateEventCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Ucs_Signature_t *dummy = NULL;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != ND_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);
        if (self_->report_fptr != NULL)
        {
            self_->report_fptr(self_->cb_inst_ptr, UCS_ND_RES_ERROR, dummy);
        }
        Nd_Reset_Lists(self_);
    }
}


/*! \brief Callback function for the INIC.NetworkStatus status and error messages
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Pointer to the result of the INIC.NetworkStatus message
 */
static void Nd_NetworkStatusCb(void *self, void *result_ptr)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Nd_NetworkStatusCb  0x%x", 1U, result_ptr_->result.code));
        /* check for NetOn/NetOff events */
        if (    (self_->neton == true)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_NOT_AVAILABLE) )
        {
            self_->neton = false;
            Fsm_SetEvent(&self_->fsm, ND_E_NET_OFF);
        }
        /* check for NetOn/NetOff events */
        else if (    (self_->neton == false)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_AVAILABLE) )
        {
            self_->neton = true;
            self_->hello_neton_request = true;
            Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
        }
        /* check for MPR event */
        else if (   (((Inic_NetworkStatus_t *)(result_ptr_->data_info))->events & UCS_NETWORK_EVENT_NCE)
            == UCS_NETWORK_EVENT_NCE)
        {
            self_->hello_mpr_request = true;
            Fsm_SetEvent(&self_->fsm, ND_E_CHECK);
        }
    }

    Srv_SetEvent(&self_->service, ND_EVENT_SERVICE);
}



/**************************************************************************************************/
/*  Helper functions                                                                              */
/**************************************************************************************************/
/*! \brief Reset the list of new detected nodes
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_Reset_Lists(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;
    uint16_t i;

    Dl_Ctor(&self_->new_list, self_->base->ucs_user_ptr);
    Dl_Ctor(&self_->unused_list, self_->base->ucs_user_ptr);

    for(i=0U; i < ND_NUM_NODES; ++i)
    {
        Dln_Ctor(&(self_->nodes[i]).node, &(self_->nodes[i]));
        Dl_InsertTail(&(self_->unused_list), &(self_->nodes[i]).node);
    }
}


/*! \brief Send the Hello.Get message
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_Send_Hello_Get(void *self)
{
    Ucs_Return_t ret_val;
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Nd_Reset_Lists(self_);  /* clear list to avoid double entries */

    ret_val = Exc_Hello_Get(self_->exc, UCS_ADDR_BROADCAST_BLOCKING, 
                            ND_SIGNATURE_VERSION, &self_->nd_hello);
    self_->debounce_flag = true;
    Nd_Start_Debounce_Timer(self_);

    TR_ASSERT(self_->base->ucs_user_ptr, "[ND]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! \brief Send the Welcome.StartResult message
 *
 * \param *self Reference to Node Discovery object
 * \param *signature signature parameter
 */
static void Nd_Send_Welcome_SR(void *self, Ucs_Signature_t *signature)
{
    Ucs_Return_t    ret_val;
    CNodeDiscovery  *self_ = (CNodeDiscovery *)self;
    uint16_t        target_address;

    if (signature->node_pos_addr == 0x0400U)
    {
        target_address = 0x0001U;
    }
    else
    {
        target_address = signature->node_pos_addr;
    }

    ret_val = Exc_Welcome_Sr(self_->exc,
                             target_address,
                             0xFFFFU,
                             ND_SIGNATURE_VERSION,
                             *signature,
                             &self_->nd_welcome);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Nd_TimerCb,
                self_,
                ND_TIMEOUT_WELCOME,
                0U);
    TR_ASSERT(self_->base->ucs_user_ptr, "[ND]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! \brief Send the Signature.Get message
 *
 * \param *self          Reference to Node Discovery object
 * \param target_address target address for the command
 */
static void Nd_Send_Signature_Get(void *self, uint16_t target_address)
{
    Ucs_Return_t   ret_val;
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    ret_val = Exc_Signature_Get(self_->exc, target_address, ND_SIGNATURE_VERSION, &self_->nd_signature);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Nd_TimerCb,
                self_,
                ND_TIMEOUT_SIGNATURE,
                0U);
    TR_ASSERT(self_->base->ucs_user_ptr, "[ND]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}

/*! \brief  Starts the periodic timer
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_Start_Periodic_Timer(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Nd_TimerCb,
                self,
                ND_TIMEOUT_PERIODIC,
                0U);
}

/*! \brief  Starts the debounce timer
 *
 * \param *self Reference to Node Discovery object
 */
static void Nd_Start_Debounce_Timer(void *self)
{
    CNodeDiscovery *self_ = (CNodeDiscovery *)self;

    Tm_SetTimer(&self_->base->tm,
                &self_->debounce_timer,
                &Nd_DebounceTimerCb,
                self,
                ND_TIMEOUT_DEBOUNCE,
                0U);
}

/*!
 * @}
 * \endcond
 */


/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

