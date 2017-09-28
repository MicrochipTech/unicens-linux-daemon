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
 * \brief Implementation of the BackChannel Diagnosis.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_BACKCHANNEL_DIAG
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic_pb.h"
#include "ucs_bc_diag.h"
#include "ucs_misc.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define BCD_NUM_STATES               7U     /*!< \brief Number of state machine states */
#define BCD_NUM_EVENTS              12U     /*!< \brief Number of state machine events */

#define BCD_TIMEOUT_COMMAND        100U     /*!< \brief supervise EXC commands */

#define BCD_SIGNATURE_VERSION        1U     /*!< \brief signature version used for BackChannel Diagnosis */

#define BCD_T_SEND              0x0100U
#define BCD_T_WAIT4DUT          0x1000U
#define BCD_T_SWITCH            0x0100U
#define BCD_T_BACK              0x2000U
#define BCD_TIMEOUT2            0x3000U
#define BCD_T_SIGNAL_ON         100U
#define BCD_T_LOCK              100U
#define BCD_T_LIGHT_PROGRESS    20U
#define BCD_AUTOBACK            (true)
#define ADMIN_BASE_ADDR         0x0F00U

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the BackChannel Diagnosis used by scheduler */
static const uint8_t BCD_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the BackChannel Diagnosis */
static const Srv_Event_t BCD_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the BackChannel Diagnosis state machine */
typedef enum Bcd_Events_
{
    BCD_E_NIL                = 0U,      /*!< \brief NIL Event */
    BCD_E_START              = 1U,      /*!< \brief API start command was called. */
    BCD_E_DIAGMODE_END       = 2U,      /*!< \brief INIC.BCDiagEnd.Result successful. */
    BCD_E_DIAG_MODE_STARTED  = 3U,      /*!< \brief INIC.BCDiag.Result successful. */
    BCD_E_DIAG_MODE_FAILED   = 4U,      /*!< \brief INIC.BCDiag.Error received. */
    BCD_E_TX_ENABLE_SUCCESS  = 5U,      /*!< \brief EXC.BCEnableTx successful */
    BCD_E_TX_ENABLE_FAILED   = 6U,      /*!< \brief EXC.BCEnableTx failed. */
    BCD_E_DIAG_RESULT_OK     = 7U,      /*!< \brief EXC.BCDIAG.Result Ok received. */
    BCD_E_DIAG_RESULT_NOTOK  = 8U,      /*!< \brief EXC.BCDIAG.Result NotOk received. */
    BCD_E_NET_OFF            = 9U,      /*!< \brief NetOff occurred. */
    BCD_E_TIMEOUT            = 10U,     /*!< \brief Timeout occurred. */
    BCD_E_ERROR              = 11U      /*!< \brief An unexpected error occurred. */

} Bcd_Events_t;


/*! \brief States of the BackChannel Diagnosis state machine */
typedef enum Bcd_State_
{
    BCD_S_IDLE            =  0U,     /*!< \brief Idle state */
    BCD_S_STARTED         =  1U,     /*!< \brief BackChannel Diagnosis started */
    BCD_S_WAIT_ENABLED    =  2U,     /*!< \brief Wait for BCEnableTx.Result */
    BCD_S_WAIT_SIG_PROP   =  3U,     /*!< \brief Wait for signal propagating through the following nodes */    
    BCD_S_WAIT_SIGNAL_ON  =  4U,     /*!< \brief Wait for t_SignalOn to expire. */
    BCD_S_WAIT_RESULT     =  5U,     /*!< \brief Wait for ENC.BCDiag.Result */
    BCD_S_END             =  6U      /*!< \brief BackChannel Diagnosis ends. */
} Bcd_State_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Bcd_Service(void *self);

static void Bcd_InicBcdStartCb(void *self, void *result_ptr);
static void Bcd_EnableTxResultCb(void *self, void *result_ptr);
static void Bcd_DiagnosisResultCb(void *self, void *result_ptr);
static void Bcd_InicBcdEndCb(void *self, void *result_ptr);

static void Bcd_OnTerminateEventCb(void *self, void *result_ptr);
static void Bcd_NetworkStatusCb(void *self, void *result_ptr);

static void Bcd_A_Start(void *self);
static void Bcd_A_EnableTx(void *self);
static void Bcd_A_DiagStart(void *self);
static void Bcd_A_NextSeg(void *self);
static void Bcd_A_StopDiag(void *self);
static void Bcd_A_Error(void *self);
static void Bcd_A_EndDiag(void *self);
static void Bcd_A_Timeout2(void *self);
static void Bcd_A_WaitLight(void *self);


static Ucs_Return_t Bcd_EnableTx(void *self, uint8_t port);

static void Bcd_TimerCb(void *self);

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t bcd_trans_tab[BCD_NUM_STATES][BCD_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State BCD_S_IDLE */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_START              */ {Bcd_A_Start,            BCD_S_STARTED           },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_TIMEOUT            */ {NULL,                   BCD_S_IDLE              },
        /* BCD_E_ERROR              */ {NULL,                   BCD_S_IDLE              }
    },
    { /* State BCD_S_STARTED */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_START              */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_DIAG_MODE_STARTED  */ {Bcd_A_EnableTx,         BCD_S_WAIT_ENABLED      },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_STARTED           },
        /* BCD_E_TIMEOUT            */ {Bcd_A_Timeout2,         BCD_S_IDLE              },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    },
    { /* State BCD_S_WAIT_ENABLED */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_START              */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {Bcd_A_WaitLight,        BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_TX_ENABLE_FAILED   */ {Bcd_A_Error,            BCD_S_IDLE              },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_WAIT_ENABLED      },
        /* BCD_E_TIMEOUT            */ {Bcd_A_Timeout2,         BCD_S_IDLE              },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    },
    { /* State BCD_S_WAIT_SIG_PROP */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_START              */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_WAIT_SIG_PROP     },
        /* BCD_E_TIMEOUT            */ {Bcd_A_DiagStart,        BCD_S_WAIT_RESULT       },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    },
    { /* State BCD_S_WAIT_SIGNAL_ON */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_START              */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_TIMEOUT            */ {Bcd_A_EnableTx,         BCD_S_WAIT_ENABLED      },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    },
    { /* State BCD_S_WAIT_RESULT */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_START              */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_DIAGMODE_END       */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_DIAG_RESULT_OK     */ {Bcd_A_NextSeg,          BCD_S_WAIT_SIGNAL_ON    },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {Bcd_A_StopDiag,         BCD_S_END               },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_WAIT_RESULT       },
        /* BCD_E_TIMEOUT            */ {Bcd_A_Timeout2,         BCD_S_IDLE              },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    },
    { /* State BCD_S_END    */
        /* BCD_E_NIL                */ {NULL,                   BCD_S_END               },
        /* BCD_E_START              */ {NULL,                   BCD_S_END               },
        /* BCD_E_DIAGMODE_END       */ {Bcd_A_EndDiag,          BCD_S_IDLE              },
        /* BCD_E_DIAG_MODE_STARTED  */ {NULL,                   BCD_S_END               },
        /* BCD_E_DIAG_MODE_FAILED   */ {NULL,                   BCD_S_END               },
        /* BCD_E_TX_ENABLE_SUCCESS  */ {NULL,                   BCD_S_END               },
        /* BCD_E_TX_ENABLE_FAILED   */ {NULL,                   BCD_S_END               },
        /* BCD_E_DIAG_RESULT_OK     */ {NULL,                   BCD_S_END               },
        /* BCD_E_DIAG_RESULT_NOTOK  */ {NULL,                   BCD_S_END               },
        /* BCD_E_NET_OFF            */ {NULL,                   BCD_S_END               },
        /* BCD_E_TIMEOUT            */ {Bcd_A_Timeout2,         BCD_S_IDLE              },
        /* BCD_E_ERROR              */ {Bcd_A_Error,            BCD_S_IDLE              }
    }
};


/*! \brief Constructor of class CBackChannelDiag.
 *  \param self         Reference to CBackChannelDiag instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 /*  \param init_ptr    Report callback function*/
void Bcd_Ctor(CBackChannelDiag *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic       = inic;
    self->exc        = exc;
    self->base       = base;

    Fsm_Ctor(&self->fsm, self, &(bcd_trans_tab[0][0]), BCD_NUM_EVENTS, BCD_E_NIL);


    Sobs_Ctor(&self->bcd_inic_bcd_start, self, &Bcd_InicBcdStartCb);
    Sobs_Ctor(&self->bcd_inic_bcd_end,   self, &Bcd_InicBcdEndCb);
    Sobs_Ctor(&self->bcd_enabletx,       self, &Bcd_EnableTxResultCb);
    Sobs_Ctor(&self->bcd_diagnosis,      self, &Bcd_DiagnosisResultCb);


    /* register termination events */
    Mobs_Ctor(&self->bcd_terminate, self, EH_M_TERMINATION_EVENTS, &Bcd_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->bcd_terminate);

    /* Register NetOn and MPR events */
    Obs_Ctor(&self->bcd_nwstatus, self, &Bcd_NetworkStatusCb);
    Inic_AddObsrvNwStatus(self->inic,  &self->bcd_nwstatus);
    self->neton = false;

    /* Initialize Node Discovery service */
    Srv_Ctor(&self->service, BCD_SRV_PRIO, self, &Bcd_Service);
    /* Add Node Discovery service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the Node Discovery service.
 *  \param self    Reference to Node Discovery object
 */
static void Bcd_Service(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if(BCD_EVENT_SERVICE == (event_mask & BCD_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, BCD_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "FSM __ %d %d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[BCD]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "FSM -> %d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}


/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*! \brief Program a node
 *
 *  \param *self        Reference to BackChannel Diagnosis object
 *  \param *report_fptr Reference to result callback used by BackChannel Diagnosis
*/
void Bcd_Start(CBackChannelDiag *self, Ucs_Bcd_ReportCb_t report_fptr)
{
    self->report_fptr = report_fptr;

    Fsm_SetEvent(&self->fsm, BCD_E_START);
    Srv_SetEvent(&self->service, BCD_EVENT_SERVICE);

    TR_INFO((self->base->ucs_user_ptr, "[BCD]", "Bcd_Start", 0U));

}



/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
static void Bcd_A_Start(void *self)
{
    Ucs_Return_t ret_val;
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    /* send INIC.BCDiag.StartResult */
    ret_val = Inic_BCDiagnosis(self_->inic,  &self_->bcd_inic_bcd_start);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Bcd_TimerCb,
                    self_,
                    BCD_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
    }

    self_->current_segment = 0U;

    TR_ASSERT(self_->base->ucs_user_ptr, "[BCD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}

static void Bcd_A_EnableTx(void *self)
{
    Ucs_Return_t ret_val;
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    /* send ENC.EnableTx */
    ret_val = Bcd_EnableTx(self, 0U);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Bcd_TimerCb,
                    self_,
                    BCD_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
    }

    TR_ASSERT(self_->base->ucs_user_ptr, "[BCD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}

/*! Starts the diagnosis command for one certain segment.
 *
 * \param *self The instance
 */
static void Bcd_A_DiagStart(void *self)
{
    Ucs_Return_t ret_val;
    uint16_t t_send     = BCD_T_SEND;
    uint16_t t_wait4dut = BCD_T_WAIT4DUT;
    uint16_t t_switch   = BCD_T_SWITCH;
    uint16_t t_back     = BCD_T_BACK;
    bool     autoback   = BCD_AUTOBACK; 

    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    ret_val = Exc_BCDiag_Start(self_->exc,
                               self_->current_segment,
                               ADMIN_BASE_ADDR + self_->current_segment,
                               t_send,
                               t_wait4dut,
                               t_switch,
                               t_back,
                               autoback,
                               &self_->bcd_diagnosis);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Bcd_TimerCb,
                    self_,
                    BCD_TIMEOUT2,
                    0U);

    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
    }


    MISC_UNUSED(ret_val);
}


static void Bcd_A_NextSeg(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    self_->report_fptr(UCS_BCD_RES_SUCCESS, 
                       (uint8_t)(self_->bcd_result.admin_addr - ADMIN_BASE_ADDR), 
                       self_->base->ucs_user_ptr);
    self_->current_segment += 1U;       /* switch to next segment. */

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Bcd_TimerCb,
                self_,
                BCD_T_SIGNAL_ON,
                0U);
}

static void Bcd_A_StopDiag(void *self)
{
    Ucs_Return_t ret_val;
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    switch(self_->bcd_result.diag_result)
    {
    case DUT_MASTER:
        self_->report_fptr(UCS_BCD_RES_NO_RING_BREAK, 
                           (uint8_t)(self_->bcd_result.admin_addr - ADMIN_BASE_ADDR), 
                           self_->base->ucs_user_ptr);
        break;

    case DUT_NO_ANSWER:
        self_->report_fptr(UCS_BCD_RES_RING_BREAK, 
                           (uint8_t)(self_->bcd_result.admin_addr - ADMIN_BASE_ADDR), 
                           self_->base->ucs_user_ptr);
        break;

    case DUT_TIMEOUT:
        self_->report_fptr(UCS_BCD_RES_TIMEOUT1,  
                           (uint8_t)(self_->bcd_result.admin_addr - ADMIN_BASE_ADDR), 
                           self_->base->ucs_user_ptr);
        break;

    default:
        break;
    }

    /* finish Back Channel Diagnosis Mode: send INIC.BCDiagEnd.StartResult */
    ret_val = Inic_BCDiagEnd(self_->inic,  &self_->bcd_inic_bcd_end);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Bcd_TimerCb,
                    self_,
                    BCD_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
    }

    MISC_UNUSED(ret_val);
}


static void Bcd_A_EndDiag(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(UCS_BCD_RES_END, UCS_BCD_DUMMY_SEGMENT, self_->base->ucs_user_ptr);
    }
}

static void Bcd_A_Timeout2(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(UCS_BCD_RES_TIMEOUT2, UCS_BCD_DUMMY_SEGMENT, self_->base->ucs_user_ptr);
    }
}

static void Bcd_A_WaitLight(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Bcd_TimerCb,
                self_,
                BCD_T_LOCK + (BCD_T_LIGHT_PROGRESS * (self_->current_segment + 1U)),
                0U);
}




/*! \brief An unecpected error occurred
 *
 * \param *self Reference to BackChannelDiagnosis object
 */
static void Bcd_A_Error(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(UCS_BCD_RES_ERROR, UCS_BCD_DUMMY_SEGMENT, self_->base->ucs_user_ptr);
    }

}


/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! \brief  Function is called on reception of the Welcome.Result messsage
 *  \param  self        Reference to BackChannelDiagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Bcd_InicBcdStartCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_        = (CBackChannelDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_DIAG_MODE_STARTED);
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_InicBcdStartCb BCD_E_DIAG_MODE_STARTED", 0U));
    }
    else
    {
        uint8_t i;

        Fsm_SetEvent(&self_->fsm, BCD_E_DIAG_MODE_FAILED);
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_InicBcdStartCb Error (code) 0x%x", 1U, result_ptr_->result.code));
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_InicBcdStartCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
        }
    }

    Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
}



/*! \brief  Function is called on reception of the BCEnableTx.Result messsage
 *  \param  self        Reference to BackChannelDiagnosis object
 *  \param  result_ptr  Pointer to the result of the BCEnableTx message
 */
static void Bcd_EnableTxResultCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_        = (CBackChannelDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /*        self_->signature_status = *(Exc_SignatureStatus_t *)(result_ptr_->data_info);*/
        Fsm_SetEvent(&self_->fsm, BCD_E_TX_ENABLE_SUCCESS);
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_EnableTxResultCb BCD_E_TX_ENABLE_SUCCESS", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_TX_ENABLE_FAILED);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Bcd_EnableTxResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the ENC.BCDiag.Result messsage
 *  \param  self        Reference to BackChannelDiagnosis object
 *  \param  result_ptr  Pointer to the result of the BCDiag message
 */
static void Bcd_DiagnosisResultCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_      = (CBackChannelDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->bcd_result = *((Exc_BCDiagResult *)(result_ptr_->data_info));
        switch (self_->bcd_result.diag_result)
        {
        case DUT_SLAVE:
            /* node reported working segment */
            Fsm_SetEvent(&self_->fsm, BCD_E_DIAG_RESULT_OK);
            TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_DiagnosisResultCb DUT_SLAVE", 0U));
            break;

        case DUT_MASTER:        /* all segments are ok */
        case DUT_NO_ANSWER:     /* ring break found */
        case DUT_TIMEOUT:       /* no communication on back channel */
            Fsm_SetEvent(&self_->fsm, BCD_E_DIAG_RESULT_NOTOK);
            TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_DiagnosisResultCb others", 0U));
            break;

        default:
            /* report error */
            break;
        }
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Bcd_DiagnosisResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the INIC.BCDiagEnd.Result messsage
 *  \param  self        Reference to BackChannel Diagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Bcd_InicBcdEndCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_        = (CBackChannelDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_DIAGMODE_END);
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_InicBcdEndCb BCD_E_DIAGMODE_END", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, BCD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[ND]", "Bcd_InicBcdEndCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
}


/*!  Function is called on severe internal errors
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Reference to data
 */
static void Bcd_OnTerminateEventCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != BCD_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);
        if (self_->report_fptr != NULL)
        {
            self_->report_fptr(UCS_BCD_RES_ERROR, UCS_BCD_DUMMY_SEGMENT, self_->base->ucs_user_ptr);
        }
    }
}


/*! \brief Callback function for the INIC.NetworkStatus status and error messages
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Pointer to the result of the INIC.NetworkStatus message
 */
static void Bcd_NetworkStatusCb(void *self, void *result_ptr)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_NetworkStatusCb  0x%x", 1U, result_ptr_->result.code));
        /* check for NetOn/NetOff events */
        if (    (self_->neton == true)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_NOT_AVAILABLE) )
        {
            self_->neton = false;
            Fsm_SetEvent(&self_->fsm, BCD_E_NET_OFF);
            Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
        }
        /* check for NetOn/NetOff events */
        else if (    (self_->neton == false)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_AVAILABLE) )
        {
/*            self_->neton = true;
            self_->hello_neton_request = true;
            Fsm_SetEvent(&self_->fsm, BCD_E_CHECK);*/
        }
        /* check for MPR event */
        else if (   (((Inic_NetworkStatus_t *)(result_ptr_->data_info))->events & UCS_NETWORK_EVENT_NCE)
            == UCS_NETWORK_EVENT_NCE)
        {
/*            self_->hello_mpr_request = true;
            Fsm_SetEvent(&self_->fsm, BCD_E_CHECK);*/
        }
    }

}


/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to Node Discovery object
 */
static void Bcd_TimerCb(void *self)
{
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    Fsm_SetEvent(&self_->fsm, BCD_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[BCD]", "Bcd_TimerCb BCD_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->service, BCD_EVENT_SERVICE);
}


/**************************************************************************************************/
/*  Helper functions                                                                              */
/**************************************************************************************************/
static Ucs_Return_t Bcd_EnableTx(void *self, uint8_t port)
{
    Ucs_Return_t ret_val;
    CBackChannelDiag *self_ = (CBackChannelDiag *)self;

    /* send INIC.BCDiag.StartResult */
    ret_val = Exc_BCEnableTx_StartResult(self_->exc, port, &self_->bcd_enabletx);

    TR_ASSERT(self_->base->ucs_user_ptr, "[BCD]", ret_val == UCS_RET_SUCCESS);
    return ret_val;
}




/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

