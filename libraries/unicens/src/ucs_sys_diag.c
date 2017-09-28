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
 * \brief   Implementation of the System Diagnosis class
 * \details Performs the System Diagnosis
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SYS_DIAG
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"
#include "ucs_ret_pb.h"
#include "ucs_sys_diag.h"
/*#include "ucs_mnsa.h"*/



/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define SYS_DIAG_NUM_STATES            10U    /*!< \brief Number of state machine states */
#define SYS_DIAG_NUM_EVENTS            17U    /*!< \brief Number of state machine events */

#define SD_NUM_HELLO                   10U    /*!< \brief Number of Hello.Get Retries */
#define SD_TIMEOUT_HELLO              150U    /*!< \brief timeout used for repeating Hello.Get messages */
#define SD_TIMEOUT_COMMAND            100U    /*!< \brief timeout used for supervising INIC commands */
#define SD_TIMEOUT_CABLE_DIAGNOSIS   3000U    /*!< \brief timeout used for supervising cable link diagnosis */
#define SD_DIAG_ADDR_BASE          0x0500U    /*!< \brief Diagnosis Node Address of own node */

#define SD_WELCOME_SUCCESS              0U    /*!< \brief Welcome.Result reports success */

#define SD_SIGNATURE_VERSION            1U    /*!< \brief signature version used for System Diagnosis */


/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the System Diagnosis service used by scheduler */
static const uint8_t SD_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the System Diagnosis service */
static const Srv_Event_t SD_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the system diagnosis state machine */
typedef enum SysDiag_Events_
{
    SD_E_NIL             = 0U,      /*!< \brief NIL Event */
    SD_E_STARTDIAG       = 1U,      /*!< \brief StartDiag API function was called */
    SD_E_SD_RES_OK       = 2U,      /*!< \brief MOSTNetworkSystemDiagnosis.Result received  */
    SD_E_ABORT           = 3U,      /*!< \brief Application requires stop of System Diagnosis */
    SD_E_HELLO_OK        = 4U,      /*!< \brief Hello.Status received */
    SD_E_HELLO_RETRY     = 5U,      /*!< \brief Retry the Hello.Get command */
    SD_E_HELLO_ALL_DONE  = 6U,      /*!< \brief All retries of the Hello.Get command are done */
    SD_E_WELCOME         = 7U,      /*!< \brief Welcome.Result, may be Ok or NotOk*/
    SD_E_ALL_DONE        = 8U,      /*!< \brief All branches and segments of the network were explored*/
    SD_E_PORT_FOUND      = 9U,      /*!< \brief An unexplored port was found */
    SD_E_PORT_ENABLED    = 10U,     /*!< \brief A port was succesful enabled */
    SD_E_PORT_DISABLED   = 11U,     /*!< \brief A port was succesful disabled */
    SD_E_BRANCH_FOUND    = 12U,     /*!< \brief Another branch was found */
    SD_E_CABLE_LINK_RES  = 13U,     /*!< \brief The CableLinkDiagnosis reported a result */
    SD_E_ERROR           = 14U,     /*!< \brief An error was detected */
    SD_E_TIMEOUT         = 15U,     /*!< \brief An timeout has been occurred */
    SD_E_NO_SUCCESS      = 16U      /*!< \brief Welcome result was NoSuccess */
} SysDiag_Events_t;

/*! \brief States of the system diagnosis state machine */
typedef enum SysDiag_State_
{
    SD_S_IDLE            =  0U,     /*!< \brief Idle state */
    SD_S_WAIT_DIAG       =  1U,     /*!< \brief System Diagnosis started */
    SD_S_WAIT_HELLO      =  2U,     /*!< \brief Hello command sent */
    SD_S_HELLO_TIMEOUT   =  3U,     /*!< \brief Hello command timed out */
    SD_S_WAIT_WELCOME    =  4U,     /*!< \brief Welcome sent */
    SD_S_NEXT_PORT       =  5U,     /*!< \brief Next port found to be tested */
    SD_S_WAIT_ENABLE     =  6U,     /*!< \brief Port Enable sent */
    SD_S_WAIT_DISABLE    =  7U,     /*!< \brief Port Disable sent */
    SD_S_CABLE_LINK_DIAG =  8U,      /*!< \brief Wait for CableL Link Diagnosis Result */
    SD_S_END             =  9U      /*!< \brief Wait for System Diagnosis stop */
} SysDiag_State_t;



/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Sd_Service(void *self);

static void Sd_SysDiagInit(void* self);
static void Sd_SysDiagStart(void *self);
static void Sd_SysDiagStop(void *self);
static void Sd_SendHello(void *self);
static void Sd_Error(void *self);
static void Sd_ErrorWelcome(void *self);
static void Sd_SendWelcome(void *self);
static void Sd_CableLinkDiagnosis(void *self);
static void Sd_CalcPort(void *self);
static void Sd_AllDone(void *self);
static void Sd_EnablePort(void *self);
static void Sd_DisablePort(void *self);
static void Sd_Finish(void *self);
static void Sd_Abort(void *self);
static void Sd_StopDiagFailed(void *self);

static void Sd_HelloTimeout(void *self);
static void Sd_SysDiagTimeout(void *self);
static void Sd_WelcomeTimeout(void *self);
static void Sd_EnablePortTimeout(void *self);
static void Sd_DisablePortTimeout(void *self);
static void Sd_CableLinkDiagnosisTimeout(void *self);

static void Sd_SysDiagStartResultCb(void *self, void *result_ptr);
static void Sd_SysDiagStopResultCb(void *self, void *result_ptr);
static void Sd_HelloStatusCb(void *self, void *result_ptr);
static void Sd_WelcomeResultCb(void *self, void *result_ptr);
static void Sd_EnablePortResultCb(void *self, void *result_ptr);
static void Sd_DisablePortResultCb(void *self, void *result_ptr);
static void Sd_CableLinkDiagnosisResultCb(void *self, void *result_ptr);
static void Sd_OnTerminateEventCb(void *self, void *result_ptr);
static void Sd_TimerCb(void *self);




/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t sys_diag_trans_tab[SYS_DIAG_NUM_STATES][SYS_DIAG_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{

    { /* State SD_S_IDLE */
        /* SD_E_NIL            */ {NULL,                          SD_S_IDLE            },
        /* SD_E_STARTDIAG      */ {&Sd_SysDiagStart,              SD_S_WAIT_DIAG       },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_IDLE            },
        /* SD_E_ABORT          */ {NULL,                          SD_S_IDLE            },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_IDLE            },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_IDLE            },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_IDLE            },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_IDLE            },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_IDLE            },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_IDLE            },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_IDLE            },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_IDLE            },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_IDLE            },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_IDLE            },
        /* SD_E_ERROR          */ {NULL,                          SD_S_IDLE            },
        /* SD_E_TIMEOUT        */ {NULL,                          SD_S_IDLE            },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_IDLE            }
    },

    { /* State SD_S_WAIT_DIAG */
        /* SD_E_NIL            */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_SD_RES_OK      */ {&Sd_SendHello,                 SD_S_WAIT_HELLO      },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_WAIT_DIAG       },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_SysDiagTimeout,            SD_S_END             },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_WAIT_DIAG       }
    },

    { /* State  SD_S_WAIT_HELLO*/
        /* SD_E_NIL            */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {&Sd_SendWelcome,               SD_S_WAIT_WELCOME    },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_WAIT_HELLO      },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_HelloTimeout,              SD_S_HELLO_TIMEOUT   },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_WAIT_HELLO      }
    },

    { /* State SD_S_HELLO_TIMEOUT */
        /* SD_E_NIL            */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_HELLO_RETRY    */ {&Sd_SendHello,                 SD_S_WAIT_HELLO      },
        /* SD_E_HELLO_ALL_DONE */ {&Sd_CableLinkDiagnosis,        SD_S_CABLE_LINK_DIAG },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {NULL,                          SD_S_HELLO_TIMEOUT   },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_HELLO_TIMEOUT   }
    },

    { /* State SD_S_WAIT_WELCOME */
        /* SD_E_NIL            */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_WELCOME        */ {&Sd_CalcPort,                  SD_S_NEXT_PORT       },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_WAIT_WELCOME    },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_WelcomeTimeout,            SD_S_END             },
        /* SD_E_NO_SUCCESS     */ {&Sd_ErrorWelcome,              SD_S_END             }
    },

    { /* State SD_S_NEXT_PORT */
        /* SD_E_NIL            */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_ALL_DONE       */ {&Sd_AllDone,                   SD_S_END             },
        /* SD_E_PORT_FOUND     */ {&Sd_EnablePort,                SD_S_WAIT_ENABLE     },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_BRANCH_FOUND   */ {&Sd_DisablePort,               SD_S_WAIT_DISABLE    },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {NULL,                          SD_S_NEXT_PORT       },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_NEXT_PORT       }
    },

    { /* State SD_S_WAIT_ENABLE */
        /* SD_E_NIL            */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_PORT_ENABLED   */ {&Sd_SendHello,                 SD_S_WAIT_HELLO      },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_WAIT_ENABLE     },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_EnablePortTimeout,         SD_S_END             },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_WAIT_ENABLE     }
    },

    { /* State SD_S_WAIT_DISABLE */
        /* SD_E_NIL            */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_PORT_DISABLED  */ {&Sd_EnablePort,                SD_S_WAIT_ENABLE     },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_WAIT_DISABLE    },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_DisablePortTimeout,        SD_S_END             },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_WAIT_DISABLE    }
    },

    { /* State SD_S_CABLE_LINK_DIAG */
        /* SD_E_NIL            */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_SD_RES_OK      */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_ABORT          */ {&Sd_Abort,                     SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_CABLE_LINK_DIAG },
        /* SD_E_CABLE_LINK_RES */ {&Sd_CalcPort,                  SD_S_NEXT_PORT       },
        /* SD_E_ERROR          */ {&Sd_Error,                     SD_S_END             },
        /* SD_E_TIMEOUT        */ {&Sd_CableLinkDiagnosisTimeout, SD_S_END             },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_CABLE_LINK_DIAG }
    },

    { /* State SD_S_END */
        /* SD_E_NIL            */ {NULL,                          SD_S_END             },
        /* SD_E_STARTDIAG      */ {NULL,                          SD_S_END             },
        /* SD_E_SD_RES_OK      */ {Sd_Finish,                     SD_S_IDLE            },
        /* SD_E_ABORT          */ {NULL,                          SD_S_END             },
        /* SD_E_HELLO_OK       */ {NULL,                          SD_S_END             },
        /* SD_E_HELLO_RETRY    */ {NULL,                          SD_S_END             },
        /* SD_E_HELLO_ALL_DONE */ {NULL,                          SD_S_END             },
        /* SD_E_WELCOME        */ {NULL,                          SD_S_END             },
        /* SD_E_ALL_DONE       */ {NULL,                          SD_S_END             },
        /* SD_E_PORT_FOUND     */ {NULL,                          SD_S_END             },
        /* SD_E_PORT_ENABLED   */ {NULL,                          SD_S_END             },
        /* SD_E_PORT_DISABLED  */ {NULL,                          SD_S_END             },
        /* SD_E_BRANCH_FOUND   */ {NULL,                          SD_S_END             },
        /* SD_E_CABLE_LINK_RES */ {NULL,                          SD_S_END             },
        /* SD_E_ERROR          */ {Sd_StopDiagFailed,             SD_S_IDLE            },
        /* SD_E_TIMEOUT        */ {Sd_StopDiagFailed,             SD_S_IDLE            },
        /* SD_E_NO_SUCCESS     */ {NULL,                          SD_S_END             }
    }

};



/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of class CSysDiag.
 *  \param self         Reference to CSysDiag instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 void SysDiag_Ctor(CSysDiag *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic = inic;
    self->exc  = exc;
    self->base = base;

    Fsm_Ctor(&self->fsm, self, &(sys_diag_trans_tab[0][0]), SYS_DIAG_NUM_EVENTS, SD_E_NIL);

    Sobs_Ctor(&self->sys_diag_start,           self, &Sd_SysDiagStartResultCb);
    Sobs_Ctor(&self->sys_diag_stop,            self, &Sd_SysDiagStopResultCb);
    Sobs_Ctor(&self->sys_hello,                self, &Sd_HelloStatusCb);
    Sobs_Ctor(&self->sys_welcome,              self, &Sd_WelcomeResultCb);
    Sobs_Ctor(&self->sys_enable_port,          self, &Sd_EnablePortResultCb);
    Sobs_Ctor(&self->sys_disable_port,         self, &Sd_DisablePortResultCb);
    Sobs_Ctor(&self->sys_cable_link_diagnosis, self, &Sd_CableLinkDiagnosisResultCb);

    /* register termination events */
    Mobs_Ctor(&self->sys_terminate, self, EH_M_TERMINATION_EVENTS, &Sd_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->sys_terminate);

    /* Initialize System Diagnosis service */
    Srv_Ctor(&self->sd_srv, SD_SRV_PRIO, self, &Sd_Service);
    /* Add System Diagnosis service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->sd_srv);

}

/*! \brief Service function of the System Diagnosis service.
 *  \param self    Reference to System Diagnosis object
 */
static void Sd_Service(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->sd_srv, &event_mask);
    if(SD_EVENT_SERVICE == (event_mask & SD_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->sd_srv, SD_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "FSM __ %d %d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "FSM -> %d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}


/*! \brief Starts the System Diagnosis State machine
 *
 * \param *self         Reference to System Diagnosis object
 * \param *obs_ptr      Observer pointer
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_API_LOCKED       System Diagnosis was already started
 * \return UCS_RET_ERR_BUFFER_OVERFLOW  Invalid observer
 */
Ucs_Return_t SysDiag_Run(CSysDiag *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->startup_locked == false)
    {
        Ssub_Ret_t ret_ssub;

        ret_ssub = Ssub_AddObserver(&self->sysdiag, obs_ptr);
        if (ret_ssub != SSUB_UNKNOWN_OBSERVER)  /* obs_ptr == NULL ? */
        {
            self->startup_locked = true;

            Sd_SysDiagInit(self);

            Fsm_SetEvent(&self->fsm, SD_E_STARTDIAG);
            Srv_SetEvent(&self->sd_srv, SD_EVENT_SERVICE);

            TR_INFO((self->base->ucs_user_ptr, "[SD]", "SysDiag_Run", 0U));
        }
        else
        {
            ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;  /* obs_ptr was invalid */
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
    }

    return ret_val;
}


/*! \brief Aborts the System Diagnosis State machine
 *
 * \param *self         Reference to System Diagnosis object
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_NOT_AVAILABLE    System Diagnosis not running
 */
Ucs_Return_t SysDiag_Abort(CSysDiag *self)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->startup_locked == true)       /* check if System Diagnosis was started */
    {
        Tm_ClearTimer(&self->base->tm, &self->timer);

        Fsm_SetEvent(&self->fsm, SD_E_ABORT);
        Srv_SetEvent(&self->sd_srv, SD_EVENT_SERVICE);
        TR_INFO((self->base->ucs_user_ptr, "[SD]", "SysDiag_Abort", 0U));
    }
    else
    {
        ret_val = UCS_RET_ERR_NOT_AVAILABLE;
    }

    return ret_val;
}

/*!  Initialize the System Diagnosis
 *
 * \param self Reference to System Diagnosis object
 */
static void Sd_SysDiagInit(void* self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    self_->hello_retry              = SD_NUM_HELLO;
    self_->segment_nr               = 0U;
    self_->num_ports                = 0U;
    self_->curr_branch              = 0U;
    self_->source.node_address      = 0xFFFFU;
    self_->source.available         = false;
    self_->last_result              = SD_INIT;

    self_->target.node_address      = 0x0001U; /* address of own INIC */
    self_->target.available         = false;

    self_->admin_node_address        = SD_DIAG_ADDR_BASE;
}


/*! FSM action function: sets the INIC into System Diagnosis Mode
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_SysDiagStart(void *self)
{
    Ucs_Return_t ret_val;

    CSysDiag *self_ = (CSysDiag *)self;

    ret_val = Inic_NwSysDiagnosis(self_->inic, &self_->sys_diag_start);
    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStart", 0U));

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Sd_TimerCb,
                self_,
                SD_TIMEOUT_COMMAND,
                0U);

    MISC_UNUSED(ret_val);
}


/*! Callback function for the Inic_NwSysDiagnosis() command
 *
 * \param *self         Reference to System Diagnosis object
 * \param *result_ptr   Result of the Inic_NwSysDiagnosis() command
 */
static void Sd_SysDiagStartResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_               = (CSysDiag *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, SD_E_SD_RES_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStartResultCb SD_E_SD_RES_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStartResultCb SD_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}


/*! FSM action function: Timeout occured
 *
 * \param *self Reference to System Diagnosis object
 */
static void Sd_SysDiagTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[SD]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_SD_ERROR;
    self_->report.err_info        = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    Sd_SysDiagStop(self_);
}

/*! FSM action function: Timeout occured
 *
 * \param *self Reference to System Diagnosis object
 */
static void Sd_EnablePortTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[SD]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_SD_ERROR;
    self_->report.err_info        = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    Sd_SysDiagStop(self_);
}

/*! FSM action function: Timeout occured
 *
 * \param *self Reference to System Diagnosis object
 */
static void Sd_DisablePortTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[SD]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_SD_ERROR;
    self_->report.err_info        = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    Sd_SysDiagStop(self_);
}

/*! Helper function. Stops the System Diagnosis
 *
 * \param *self Reference to System Diagnosis object
 */
static void Sd_SysDiagStop(void *self)
{
    Ucs_Return_t ret_val;

    CSysDiag *self_ = (CSysDiag *)self;

    ret_val = Inic_NwSysDiagEnd(self_->inic, &self_->sys_diag_stop);
    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStop", 0U));
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Sd_TimerCb,
                    self_,
                    SD_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code      = UCS_SD_ERROR;
        self_->report.err_info  = UCS_SD_ERR_STOP_SYSDIAG_FAILED;

        Ssub_Notify(&self_->sysdiag, &self_->report, false);

        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
    }
}


/*! \brief Callback function for the Inic_NwSysDiagEnd() command
 *
 * \param *self         Reference to System Diagnosis object
 * \param *result_ptr   Result of the Inic_NwSysDiagEnd() command
 */
static void Sd_SysDiagStopResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_               = (CSysDiag *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", UCS_RES_SUCCESS == result_ptr_->result.code);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, SD_E_SD_RES_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStopResultCb SD_E_SD_RES_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_SysDiagStopResultCb SD_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}



/*! FSM action function: Send Hello.Get command
 *
 * \param *self Reference to System Diagnosis object
 */
static void Sd_SendHello(void *self)
{
    Ucs_Return_t ret_val;

    CSysDiag *self_ = (CSysDiag *)self;

    ret_val = Exc_Hello_Get(self_->exc, 
                            UCS_ADDR_BROADCAST_BLOCKING, 
                            SD_SIGNATURE_VERSION, 
                            &self_->sys_hello);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Sd_TimerCb,
                self_,
                SD_TIMEOUT_HELLO,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}

/*! Callback function for the Enc.Hello.Status message
 *
 * \param *self         Reference to System Diagnosis object
 * \param *result_ptr   Result of the Exc_Hello_Get() command
 */
static void Sd_HelloStatusCb(void *self, void *result_ptr)
{
    CSysDiag *self_              = (CSysDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it for the Welcome command */
        self_->target.signature = (*(Exc_HelloStatus_t *)(result_ptr_->data_info)).signature;
        self_->target.version   = (*(Exc_HelloStatus_t *)(result_ptr_->data_info)).version;

        if (self_->segment_nr != 0U)
        {
            self_->target.node_address = self_->segment_nr + 0x0400U;

        }

        Fsm_SetEvent(&self_->fsm, SD_E_HELLO_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_HelloStatusCb SD_E_SD_RES_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_HelloStatusCb SD_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}

/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to System Diagnosis object
 */
static void Sd_TimerCb(void *self)
{
    CSysDiag *self_              = (CSysDiag *)self;

    Fsm_SetEvent(&self_->fsm, SD_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_TimerCb SD_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}


/*! FSM action function: retry hello command or start CableLinkDiagnosis
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_HelloTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    if (self_->hello_retry > 0U)
    {
        --self_->hello_retry;
        Fsm_SetEvent(&self_->fsm, SD_E_HELLO_RETRY);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_HelloTimeout SD_E_HELLO_RETRY", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_HELLO_ALL_DONE);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_HelloTimeout SD_E_HELLO_ALL_DONE", 0U));
    }

    /*Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);*/
}


/*! FSM action function: Send Welcome message
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_SendWelcome(void *self)
{
    Ucs_Return_t  ret_val;
    CSysDiag     *self_ = (CSysDiag *)self;

    self_->admin_node_address = SD_DIAG_ADDR_BASE + self_->segment_nr;

    ret_val = Exc_Welcome_Sr(self_->exc,
                             self_->target.node_address,
                             self_->admin_node_address,
                             SD_SIGNATURE_VERSION,
                             self_->target.signature,
                             &self_->sys_welcome);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Sd_TimerCb,
                self_,
                SD_TIMEOUT_COMMAND,
                0U);
    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! \brief  Function is called on reception of the Welcome.Result messsage
 *  \param  self        Reference to System Diagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Sd_WelcomeResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_              = (CSysDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it for the Welcome command */
        self_->target.result = (*(Exc_WelcomeResult_t *)(result_ptr_->data_info)).res;

        if (self_->target.result == SD_WELCOME_SUCCESS)
        {
            self_->target.available = true;

            if (self_->segment_nr == 0U)
            {
                self_->num_ports = self_->target.signature.num_ports;
            }
            else
            {
                self_->last_result = SD_SEGMENT;
            }
            /* do not report result for own node */
            if (self_->segment_nr != 0U)
            {
                MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

                self_->report.code            = UCS_SD_TARGET_FOUND;
                self_->report.segment.branch  = self_->curr_branch;
                self_->report.segment.num     = self_->segment_nr;
                self_->report.segment.source  = self_->source.signature;
                self_->report.segment.target  = self_->target.signature;
                /*self_->report.cable_link_info = 0U;*/     /* element is not written deliberately */
                /*self_->report.err_info        = 0U;*/     /* element is not written deliberately */

                Ssub_Notify(&self_->sysdiag, &self_->report, false);
                TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_WelcomeResultCb ReportSegment", 0U));
            }

            Fsm_SetEvent(&self_->fsm, SD_E_WELCOME);
            TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_WelcomeResultCb SD_E_WELCOME", 0U));
        }
        else
        {
            MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

            self_->report.code            = UCS_SD_ERROR;
            self_->report.segment.branch  = self_->curr_branch;
            self_->report.segment.num     = self_->segment_nr;
            self_->report.segment.source  = self_->source.signature;
            self_->report.segment.target  = self_->target.signature;
            /*self_->report.cable_link_info = 0U;*/     /* element is not written deliberately */
            self_->report.err_info        = UCS_SD_ERR_WELCOME_NO_SUCCESS;

            Ssub_Notify(&self_->sysdiag, &self_->report, false);

            Fsm_SetEvent(&self_->fsm, SD_E_NO_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_WelcomeResultCb reported NoSuccess", 0U));
        }

    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_WelcomeResultCb Error SD_E_ERROR 0x%x", 1U, result_ptr_->result.code));

    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}


/*! \brief FSM action function:  Calculate the next port tobe examined
 *  \param  self     Reference to System Diagnosis object
 */
static void Sd_CalcPort(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    switch (self_->last_result)
    {
        case SD_INIT:
            self_->curr_branch  = 0U;             /* Master device has at least one port */
            self_->source = self_->target;
            self_->master = self_->target;

            MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
            self_->last_result = SD_SEGMENT;
            Fsm_SetEvent(&self_->fsm, SD_E_PORT_FOUND);
            TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_PORT_FOUND", 0U));
            break;

        case SD_SEGMENT:
            if (self_->target.signature.num_ports > 1U)
            {
                self_->source = self_->target;
                MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                Fsm_SetEvent(&self_->fsm, SD_E_PORT_FOUND);
                TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_PORT_FOUND", 0U));
            }
            else                                /* switch to next branch if possible*/
            {
                if (self_->num_ports == (self_->curr_branch + 1U))     /* last branch */
                {
                    Fsm_SetEvent(&self_->fsm, SD_E_ALL_DONE);
                    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_ALL_DONE", 0U));
                }
                else
                {
                    self_->segment_nr = 1U;                         /* reset segment number */
                    self_->curr_branch++;                           /* switch to next port */
                    self_->source = self_->master;
                    MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                    Fsm_SetEvent(&self_->fsm, SD_E_BRANCH_FOUND);
                    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_BRANCH_FOUND", 0U));
                }
            }
            break;

        case SD_CABLE_LINK:
            if (self_->num_ports == (self_->curr_branch + 1U))     /* last branch */
            {
                Fsm_SetEvent(&self_->fsm, SD_E_ALL_DONE);
                TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_ALL_DONE", 0U));
            }
            else
            {
                self_->segment_nr = 1U;                             /* reset segment number */
                self_->curr_branch++;                               /* switch to next port */
                self_->source = self_->master;
                MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                Fsm_SetEvent(&self_->fsm, SD_E_BRANCH_FOUND);
                TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CalcPort SD_E_BRANCH_FOUND", 0U));
            }
            break;

        default:
            break;
    }

    /*Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);*/
}


/*! \brief FSM action function: Enable port
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_EnablePort(void *self)
{
    CSysDiag     *self_ = (CSysDiag *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;

    if (self_->segment_nr == 0U)
    {
        port_number    = self_->curr_branch;
        target_address = 0x0001U;
    }
    else
    {
        port_number    = 1U;
        target_address = self_->source.node_address;
    }

    ret_val = Exc_EnablePort_Sr(self_->exc, target_address, port_number, true, &self_->sys_enable_port);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Sd_TimerCb,
                self_,
                SD_TIMEOUT_COMMAND,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! Function is called on reception of the EnablePort.Result messsage
 *
 * \param *self         Reference to System Diagnosis object
 * \param *result_ptr
 */
static void Sd_EnablePortResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_              = (CSysDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->segment_nr++;
        Fsm_SetEvent(&self_->fsm, SD_E_PORT_ENABLED);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_EnablePortResultCb SD_E_PORT_ENABLED", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_EnablePortResultCb SD_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}


/*! \brief FSM action function:
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_DisablePort(void *self)
{
    CSysDiag     *self_ = (CSysDiag *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;

    target_address = self_->admin_node_address;
    port_number = self_->curr_branch;

    ret_val = Exc_EnablePort_Sr(self_->exc, target_address, port_number, false, &self_->sys_disable_port);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Sd_TimerCb,
                self_,
                SD_TIMEOUT_COMMAND,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


static void Sd_DisablePortResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_              = (CSysDiag *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, SD_E_PORT_DISABLED);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_DisablePortResultCb SD_E_PORT_DISABLED", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_DisablePortResultCb SD_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
}


/*! \brief FSM action function: Start CableLinkDiagnosis
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_CableLinkDiagnosis(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;


    if (self_->segment_nr != 0U)    /* do not start CableLinkDiagnosis when connecting to local INIC */
    {
    target_address = self_->source.node_address;

    if (self_->segment_nr == 1U)
    {
        port_number = self_->curr_branch;
    }
    else
    {
        port_number = 1U;                   /* OS81119: always port 1 */
    }

    self_->last_result = SD_CABLE_LINK;

    ret_val = Exc_CableLinkDiagnosis_Start(self_->exc, target_address, port_number, &self_->sys_cable_link_diagnosis);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CableLinkDiagnosis", 0U));

    Tm_SetTimer(&self_->base->tm,
            &self_->timer,
            &Sd_TimerCb,
            self_,
            SD_TIMEOUT_CABLE_DIAGNOSIS,
            0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[SD]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}
    else    /* stop SystemDiagnosis when connecting to local INIC failed */
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);
    }
}


static void Sd_CableLinkDiagnosisResultCb(void *self, void *result_ptr)
{
    CSysDiag *self_              = (CSysDiag *)self;

    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

        self_->report.code            = UCS_SD_CABLE_LINK_RES;
        self_->report.segment.branch  = self_->curr_branch;
        self_->report.segment.num     = self_->segment_nr;
        self_->report.segment.source  = self_->source.signature;
        /*self_->report.segment.target  = self_->target.signature;*/ /* structure is not written deliberately */
        self_->report.cable_link_info = (*(Exc_CableLinkDiagResult_t *)(result_ptr_->data_info)).result;
        /*self_->report.err_info        = 0U;*/     /* element is not written deliberately */

        Ssub_Notify(&self_->sysdiag, &self_->report, false);


        Fsm_SetEvent(&self_->fsm, SD_E_CABLE_LINK_RES);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CableLinkDiagnosisResultCb SD_E_CABLE_LINK_RES", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, SD_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_CableLinkDiagnosisResultCb SD_E_ERROR %02X %02X %02X", 3U, result_ptr_->result.info_ptr[0], result_ptr_->result.info_ptr[1], result_ptr_->result.info_ptr[2]));
    }

    Srv_SetEvent(&self_->sd_srv, SD_EVENT_SERVICE);

}


/*! \brief FSM action function: React on Timeout of CableLinkDiagnosis
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_CableLinkDiagnosisTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code      = UCS_SD_ERROR;
    self_->report.err_info  = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[SD]");
    Sd_SysDiagStop(self_);
}

/*! \brief FSM action function: React on Timeout of Welcome
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_WelcomeTimeout(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code      = UCS_SD_ERROR;
    self_->report.err_info  = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[SD]");
    Sd_SysDiagStop(self_);
}




/*! \brief FSM action function: All branches and segments explored, finish System Diagnosis
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_AllDone(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_AllDone", 0U));

    Sd_SysDiagStop(self_);
}


/*! \brief FSM action function: INIC system Diagnosis mode ended
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_Finish(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

    self_->report.code = UCS_SD_FINISHED;
    Ssub_Notify(&self_->sysdiag, &self_->report, true);

    self_->startup_locked = false;

    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_Finish", 0U));
}

/*! \brief FSM action function: An unexpected error occurred.
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_Error(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code     = UCS_SD_ERROR;
    self_->report.err_info = UCS_SD_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    Sd_SysDiagStop(self_);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_Error", 0U));
}

/*! \brief FSM action function: Welcome reports NoSuccess.
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_ErrorWelcome(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    Sd_SysDiagStop(self_);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_ErrorWelcome", 0U));
}

/*! \brief FSM action function: stopping system diagnosis mode failed
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_StopDiagFailed(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code     = UCS_SD_ERROR;
    self_->report.err_info = UCS_SD_ERR_STOP_SYSDIAG_FAILED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    /* always finish the System Diagnosis with event UCS_SD_FINISHED */
    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code = UCS_SD_FINISHED;
    Ssub_Notify(&self_->sysdiag, &self_->report, true);     /* remove the observer function */

    self_->startup_locked = false;

    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_StopDiagFailed", 0U));
}

/*! \brief FSM action function: Application requested to abort the System Diagnosis.
 *
 * \param *self     Reference to System Diagnosis object
 */
static void Sd_Abort(void *self)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code = UCS_SD_ABORTED;
    Ssub_Notify(&self_->sysdiag, &self_->report, false);

    Sd_SysDiagStop(self_);
    TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_Abort", 0U));
}



/*!  Function is called on severe internal errors
 *
 * \param *self         Reference to System Diagnosis object
 * \param *result_ptr   Reference to data
 */
static void Sd_OnTerminateEventCb(void *self, void *result_ptr)
{
    CSysDiag *self_ = (CSysDiag *)self;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != SD_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);

        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code     = UCS_SD_ERROR;
        self_->report.err_info = UCS_SD_ERR_TERMINATED;
        Ssub_Notify(&self_->sysdiag, &self_->report, false);

        /* always finish the System Diagnosis with event UCS_SD_FINISHED */
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code = UCS_SD_FINISHED;
        Ssub_Notify(&self_->sysdiag, &self_->report, true);     /* remove the observer function */

        TR_INFO((self_->base->ucs_user_ptr, "[SD]", "Sd_OnTerminateEventCb", 0U));

        /* reset FSM */
        self_->startup_locked = false;
        Sd_SysDiagInit(self_);
        self_->fsm.current_state = SD_S_IDLE;
    }
}




/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

