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
 * \brief Implementation of the Programming Service.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_PROG_MODE
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic_pb.h"
#include "ucs_prog.h"
#include "ucs_misc.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define PRG_NUM_STATES             6U    /*!< \brief Number of state machine states */
#define PRG_NUM_EVENTS            13U    /*!< \brief Number of state machine events */

#define PRG_TIMEOUT_COMMAND      100U    /*!< \brief supervise EXC commands */

#define PRG_SIGNATURE_VERSION      1U    /*!< \brief signature version used for Node Discovery */

#define PRG_ADMIN_BASE_ADDR   0x0F00U    /*!< \brief bas admin address */


/* Error values */
#define PRG_HW_RESET_REQ        0x200110U   /* HW reset required */
#define PRG_SESSION_ACTIVE      0x200111U   /* Session already active */
#define PRG_CFG_STRING_ERROR    0x200220U   /* A configuration string erase error has occurred. */
#define PRG_MEM_ERASE_ERROR     0x200221U   /* An error memory erase error has occurred.*/
#define PRG_CFG_WRITE_ERROR     0x200225U   /* Configuration memory write error. */
#define PRG_CFG_FULL_ERROR      0x200226U   /* Configuration memory is full. */
#define PRG_HDL_MATCH_ERROR     0x200330U   /* The SessionHandle does not match the current memory session. */
#define PRG_MEMID_ERROR         0x200331U   /* The memory session does not support the requested MemID. */
#define PRG_ADDR_EVEN_ERROR     0x200332U   /* The Address is not even when writing the configuration memory. */
#define PRG_LEN_EVEN_ERROR      0x200333U   /* The UnitLen is not even when writing the configuration memory. */

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the Programming service used by scheduler */
static const uint8_t PRG_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the Programming service */
static const Srv_Event_t PRG_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the system diagnosis state machine */
typedef enum Prg_Events_
{
    PRG_E_NIL                = 0U,      /*!< \brief NIL Event */
    PRG_E_START              = 1U,      /*!< \brief API start command was called. */
    PRG_E_STOP               = 2U,      /*!< \brief Stop request occurred. */
    PRG_E_WELCOME_SUCCESS    = 3U,      /*!< \brief Welcome command was successful. */
    PRG_E_WELCOME_NOSUCCESS  = 4U,      /*!< \brief Welcome command was not successful. */
    PRG_E_MEM_WRITE_CMD      = 5U,      /*!< \brief MemorySessionOpen command was succcessful */
    PRG_E_MEM_WRITE_FINISH   = 6U,      /*!< \brief MemoryWrite command was succcessful */
    PRG_E_MEM_CLOSE_SUCCESS  = 7U,      /*!< \brief MemorySessionClose command was succcessful */
    PRG_E_NET_OFF            = 8U,      /*!< \brief NetOff occurred. */
    PRG_E_TIMEOUT            = 9U,      /*!< \brief Timeout occurred. */
    PRG_E_ERROR             = 10U,      /*!< \brief An error occurred which requires no command to be sent to the INIC. */
    PRG_E_ERROR_INIT        = 11U,      /*!< \brief Error requires Init.Start to be sent. */
    PRG_E_ERROR_CLOSE_INIT  = 12U       /*!< \brief Error requires MemorySessionClose.SR and Init.Start to be sent. */
} Prg_Events_t;


/*! \brief States of the node discovery state machine */
typedef enum Prg_State_
{
    PRG_S_IDLE                =  0U,     /*!< \brief Idle state. */
    PRG_S_WAIT_WELCOME        =  1U,     /*!< \brief Programming started. */
    PRG_S_WAIT_MEM_OPEN       =  2U,     /*!< \brief Wait for MemorySessionOpen result. */
    PRG_S_WAIT_MEM_WRITE      =  3U,     /*!< \brief Wait for MemoryWrite result. */
    PRG_S_WAIT_MEM_CLOSE      =  4U,     /*!< \brief Wait for MemorySessionClose result. */
    PRG_S_WAIT_MEM_ERR_CLOSE  =  5U      /*!< \brief Wait for MemorySessionClose result in error case. */
} Prg_State_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Prg_Service(void *self);

static void Prg_WelcomeResultCb(void *self, void *result_ptr);
static void Prg_MemOpenResultCb(void *self, void *result_ptr);
static void Prg_MemWriteResultCb(void *self, void *result_ptr);
static void Prg_MemCloseResultCb(void *self, void *result_ptr);

static void Prg_OnTerminateEventCb(void *self, void *result_ptr);
static void Prg_NetworkStatusCb(void *self, void *result_ptr);

static void Prg_A_Start(void *self);
static void Prg_A_MemOpen(void *self);
static void Prg_A_MemWrite(void *self);
static void Prg_A_MemClose(void *self);
static void Prg_A_InitDevice(void *self);
static void Prg_A_NetOff(void *self);
static void Prg_A_Timeout(void *self);
static void Prg_A_Error(void *self);
static void Prg_A_Error_Init(void *self);
static void Prg_A_Error_Close_Init(void *self);


static void Prg_Check_RetVal(CProgramming *self, Ucs_Return_t  ret_val);
static uint32_t Prg_CalcError(uint8_t val[]);

static void Prg_TimerCb(void *self);

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t prg_trans_tab[PRG_NUM_STATES][PRG_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State PRG_S_IDLE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_START              */ {Prg_A_Start,                PRG_S_WAIT_WELCOME       },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_IDLE               },
    },
    { /* State PRG_S_WAIT_WELCOME */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_WELCOME_SUCCESS    */ {Prg_A_MemOpen,              PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_NOSUCCESS  */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_WAIT_WELCOME       }
    },
    { /* State PRG_S_WAIT_MEM_OPEN */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_MEM_WRITE_CMD      */ {Prg_A_MemWrite,             PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {Prg_A_MemClose,             PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error_Close_Init,     PRG_S_WAIT_MEM_ERR_CLOSE }
    },
    { /* State PRG_S_WAIT_MEM_WRITE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {Prg_A_MemClose,             PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error_Close_Init,     PRG_S_WAIT_MEM_ERR_CLOSE }
    },
    { /* State PRG_S_WAIT_MEM_CLOSE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {Prg_A_InitDevice,           PRG_S_IDLE               },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error,                PRG_S_IDLE               },
    },
    { /* State PRG_S_WAIT_MEM_ERR_CLOSE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_CLOSE_SUCCESS  */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error,                PRG_S_IDLE               },
    }

};


/*! \brief Constructor of class CProgramming.
 *  \param self         Reference to CProgramming instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 /*  \param init_ptr    Report callback function*/
void Prg_Ctor(CProgramming *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic       = inic;
    self->exc        = exc;
    self->base       = base;

    Fsm_Ctor(&self->fsm, self, &(prg_trans_tab[0][0]), PRG_NUM_EVENTS, PRG_E_NIL);

    Sobs_Ctor(&self->prg_welcome,       self, &Prg_WelcomeResultCb);
    Sobs_Ctor(&self->prg_memopen,       self, &Prg_MemOpenResultCb);
    Sobs_Ctor(&self->prg_memwrite,      self, &Prg_MemWriteResultCb);
    Sobs_Ctor(&self->prg_memclose,      self, &Prg_MemCloseResultCb);

    /* register termination events */
    Mobs_Ctor(&self->prg_terminate, self, EH_M_TERMINATION_EVENTS, &Prg_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->prg_terminate);

    /* Register NetOn and MPR events */
    Obs_Ctor(&self->prg_nwstatus, self, &Prg_NetworkStatusCb);
    Inic_AddObsrvNwStatus(self->inic,  &self->prg_nwstatus);
    self->neton = false;

    /* Initialize Programming service */
    Srv_Ctor(&self->service, PRG_SRV_PRIO, self, &Prg_Service);
    /* Add Programming service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the Node Discovery service.
 *  \param self    Reference to Programming service object
 */
static void Prg_Service(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if(PRG_EVENT_SERVICE == (event_mask & PRG_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, PRG_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "FSM __ %d %d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[PRG]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "FSM -> %d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}



/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*!
 *
 * \param *self Reference to Programming service object
 */
/*! \brief Program a node
 *
 * \param *self         Reference to Programming service object
 * \param node_id       Node position address of the node to be programmed
 * \param *signature    Signature of the node to be programmed
 * \param session_type  Defines the memory access type.
 * \param command_list  Refers to array of programming tasks.
 * \param report_fptr   Report callback function
 */
void Prg_Start(CProgramming *self,
               uint16_t node_id,
               Ucs_Signature_t *signature,
               Ucs_Prg_SessionType_t session_type,
               Ucs_Prg_Command_t* command_list,
               Ucs_Prg_ReportCb_t report_fptr)
{


    self->node_id          = node_id;
    self->signature        = *signature;
    self->session_type     = session_type;
    self->command_list     = command_list;
    self->report_fptr      = report_fptr;
    self->current_function = UCS_PRG_FKT_DUMMY;

    if (self->neton == true)
    {
        Fsm_SetEvent(&self->fsm, PRG_E_START);
        Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);

        TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start", 0U));
    }
    else
    {
        if (self->report_fptr != NULL)
        {
            self->report_fptr(UCS_PRG_RES_NET_OFF,
                              self->current_function,
                              0U,
                              NULL,
                              self->base->ucs_user_ptr);
        }
        TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start failed: NET_OFF", 0U));
    }
}



/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
/*!  Action on Start command
 *
 * \param *self Reference to Node Discovery object
 */
static void Prg_A_Start(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    if (self_->node_id == 0x0400U)
    {
        self_->target_address = UCS_ADDR_LOCAL_INIC;
    }
    else
    {
        self_->target_address = self_->node_id;
    }

    self_->admin_node_address = PRG_ADMIN_BASE_ADDR + ((self_->node_id) & 0x00FFU);
    self_->current_function = UCS_PRG_FKT_WELCOME;

    ret_val = Exc_Welcome_Sr(self_->exc,
                             self_->target_address,
                             self_->admin_node_address,
                             PRG_SIGNATURE_VERSION,
                             self_->signature,
                             &self_->prg_welcome);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_MemOpen(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_MEM_OPEN;

    ret_val = Exc_MemSessionOpen_Sr(self_->exc,
                                    self_->admin_node_address,
                                    self_->session_type,
                                    &self_->prg_memopen);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_MemWrite(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_MEM_WRITE;

    ret_val = Exc_MemoryWrite_Sr(self_->exc,
                                 self_->admin_node_address,
                                 self_->session_handle,
                                 self_->command_list[self_->command_index].mem_id,
                                 self_->command_list[self_->command_index].address,
                                 self_->command_list[self_->command_index].unit_length,
                                 self_->command_list[self_->command_index].data,
                                 &self_->prg_memwrite);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_MemClose(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_MEM_CLOSE;
    ret_val = Exc_MemSessionClose_Sr(self_->exc,
                                     self_->admin_node_address,
                                     self_->session_handle,
                                     &self_->prg_memclose);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_InitDevice(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_INIT;
    ret_val = Exc_DeviceInit_Start(self_->exc,
                                   self_->admin_node_address,
                                   NULL);
    Prg_Check_RetVal(self_, ret_val);

    if (ret_val == UCS_RET_SUCCESS)
    {
        if (self_->report_fptr != NULL)
        {
            self_->report_fptr(UCS_PRG_RES_SUCCESS,
                               UCS_PRG_FKT_DUMMY,
                               0U,
                               NULL,
                               self_->base->ucs_user_ptr);
        }
    }
}

static void Prg_A_NetOff(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(UCS_PRG_RES_NET_OFF,
                           self_->current_function,
                           0U,
                           NULL,
                           self_->base->ucs_user_ptr);
    }
}

static void Prg_A_Timeout(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(UCS_PRG_RES_TIMEOUT,
                           self_->current_function,
                           0U,
                           NULL,
                           self_->base->ucs_user_ptr);
    }
}

static void Prg_A_Error(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;

    if (   (self_->error.code == UCS_PRG_RES_FKT_ASYNCH)
        && (self_->error.ret_len != 0U))
    {
        data_ptr = &(self_->error.parm[0]);
    }

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->error.code,
                           self_->error.function,
                           self_->error.ret_len,
                           data_ptr,
                           self_->base->ucs_user_ptr);
    }
}


static void Prg_A_Error_Init(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;
    Ucs_Return_t  ret_val;

    ret_val = Exc_DeviceInit_Start(self_->exc,
                                     self_->admin_node_address,
                                     NULL);
    Prg_Check_RetVal(self_, ret_val);

    if (   (self_->error.code == UCS_PRG_RES_FKT_ASYNCH)
        && (self_->error.ret_len != 0U))
    {
        data_ptr = &(self_->error.parm[0]);
    }

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->error.code,
                           self_->error.function,
                           self_->error.ret_len,
                           data_ptr,
                           self_->base->ucs_user_ptr);
    }
}

static void Prg_A_Error_Close_Init(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;
    Ucs_Return_t  ret_val;

    ret_val = Exc_DeviceInit_Start(self_->exc,
                                     self_->admin_node_address,
                                     NULL);
    Prg_Check_RetVal(self_, ret_val);


    if (   (self_->error.code == UCS_PRG_RES_FKT_ASYNCH)
        && (self_->error.ret_len != 0U))
    {
        data_ptr = &(self_->error.parm[0]);
    }

    if (self_->report_fptr != NULL)
    {
        self_->report_fptr(self_->error.code,
                           self_->error.function,
                           self_->error.ret_len,
                           data_ptr,
                           self_->base->ucs_user_ptr);
    }
}


/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! \brief  Function is called on reception of the Welcome.Result messsage
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_WelcomeResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Exc_WelcomeResult_t  welcome_result;
        /* read signature and store it */
        welcome_result = *(Exc_WelcomeResult_t *)(result_ptr_->data_info);
        if (welcome_result.res == EXC_WELCOME_SUCCESS)
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_WELCOME_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb PRG_E_WELCOME_SUCCESS", 0U));
        }
        else
        {
            /* store error paramters */
            self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
            self_->error.function = UCS_PRG_FKT_WELCOME_NOSUCCESS;
            self_->error.ret_len  = 0U;

            Fsm_SetEvent(&self_->fsm, PRG_E_WELCOME_NOSUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb PRG_E_WELCOME_NOSUCCESS", 0U)); 
        }
    }
    else
    {
        uint8_t i;
        /* store error paramters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_WELCOME;
        self_->error.ret_len  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.parm[i] = result_ptr_->result.info_ptr[i];
        }

        Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb Error (code) 0x%x", 1U, result_ptr_->result.code));
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
        }
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/*! \brief  Function is called on reception of the MemorySessionOpen.Result messsage
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemOpenResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->session_handle = *(uint16_t *)(result_ptr_->data_info);
        self_->command_index = 0U;

        if (   (self_->command_list[self_->command_index].data_length == 0U)
            || (self_->command_list[self_->command_index].data        == NULL))
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_FINISH);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemOpenResultCb No Tasks", 0U));
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_CMD);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemOpenResultCb successful", 0U));
        }
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error paramters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_OPEN;
        self_->error.ret_len  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.parm[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.parm[0]));

        switch (fs_error)
        {
            case PRG_HW_RESET_REQ:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
                break;

            case PRG_SESSION_ACTIVE:
                self_->session_handle = (uint16_t)(((uint16_t)(self_->error.parm[3])) << 8U) + self_->error.parm[4]; /* get correct session handle */
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            default:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
                break;
        }

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemOpenResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the MemoryWrite.Result messsage
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemWriteResultCb(void *self, void *result_ptr)
{
    CProgramming *self_        = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->command_index++;
        if (   (self_->command_list[self_->command_index].data_length == 0U)
            || (self_->command_list[self_->command_index].data        == NULL))
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_FINISH);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb PRG_E_MEM_WRITE_FINISH", 0U));
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_CMD);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb successful", 0U));
        }
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error paramters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_WRITE;
        self_->error.ret_len  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.parm[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.parm[0]));

        switch (fs_error)
        {
            case PRG_CFG_WRITE_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_CFG_FULL_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_HDL_MATCH_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
                break;

            case PRG_MEMID_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_ADDR_EVEN_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_LEN_EVEN_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            default:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
                break;
        }
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the MemorySessionClose.Result messsage
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemCloseResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        uint8_t session_result = *(uint8_t *)(result_ptr_->data_info);

        if (session_result == 0U)
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_CLOSE_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb PRG_E_MEM_CLOSE_SUCCESS", 0U));
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb ErrResult PRG_E_ERROR_INIT", 0U));
        }
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error paramters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_CLOSE;
        self_->error.ret_len  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.parm[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.parm[0]));

        if (fs_error == PRG_HDL_MATCH_ERROR)
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
        }

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}




/*!  Function is called on severe internal errors
 *
 * \param *self         Reference to Programming object
 * \param *result_ptr   Reference to data
 */
static void Prg_OnTerminateEventCb(void *self, void *result_ptr)
{
    CProgramming *self_ = (CProgramming *)self;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != PRG_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);
        if (self_->report_fptr != NULL)
        {
            self_->report_fptr(UCS_PRG_RES_ERROR, 
                               self_->current_function, 
                               0U, 
                               NULL, 
                               self_->base->ucs_user_ptr);
        }

        /* reset FSM */
        self_->fsm.current_state = PRG_S_IDLE;
    }
}


/*! \brief Callback function for the INIC.NetworkStatus status and error messages
 *
 * \param *self         Reference to Node Discovery object
 * \param *result_ptr   Pointer to the result of the INIC.NetworkStatus message
 */
static void Prg_NetworkStatusCb(void *self, void *result_ptr)
{
    CProgramming *self_ = (CProgramming *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_NetworkStatusCb  0x%x", 1U, result_ptr_->result.code));
        /* check for NetOn/NetOff events */
        if (    (self_->neton == true)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_NOT_AVAILABLE) )
        {
            self_->neton = false;
            Fsm_SetEvent(&self_->fsm, PRG_E_NET_OFF);
        }
        else if (    (self_->neton == false)
                && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_AVAILABLE) )
        {
            self_->neton = true;
        }
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to System Diagnosis object
 */
static void Prg_TimerCb(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    Fsm_SetEvent(&self_->fsm, PRG_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_TimerCb PRG_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/**************************************************************************************************/
/*  Helper functions                                                                              */
/**************************************************************************************************/

static void Prg_Check_RetVal(CProgramming *self, Ucs_Return_t  ret_val)
{
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self->base->tm,
                    &self->timer,
                    &Prg_TimerCb,
                    self,
                    PRG_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        TR_ASSERT(self->base->ucs_user_ptr, "[PRG]", ret_val == UCS_RET_SUCCESS);

        /* store error paramter */
        self->error.code     = UCS_PRG_RES_FKT_SYNCH;
        self->error.function = self->current_function;
        self->error.ret_len  = (uint8_t)ret_val;

        Fsm_SetEvent(&self->fsm, PRG_E_ERROR);
        Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);
    }
}


static uint32_t Prg_CalcError(uint8_t val[])
{
    uint32_t temp;

    temp = val[0] + (((uint32_t)val[1]) << 8U) + (((uint32_t)val[2]) << 16U);

    return temp;
}





/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

