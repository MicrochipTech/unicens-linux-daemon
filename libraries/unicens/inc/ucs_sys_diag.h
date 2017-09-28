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
 * \brief Internal header file of class CSysDiag.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SYS_DIAG
 * @{
 */

#ifndef UCS_SYS_DIAG_H
#define UCS_SYS_DIAG_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_obs.h"
#include "ucs_fsm.h"
#include "ucs_inic.h"
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif




/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Result codes of a tested segment. */
typedef enum Sd_ResultCode_
{
    SD_INIT         = 0x01U,    /*!< \brief initialized */
    SD_SEGMENT      = 0x02U,    /*!< \brief segment explored  */
    SD_CABLE_LINK   = 0x03U     /*!< \brief cable link diagnosis executed */

} Sd_ResultCode_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure decribing a node of the segment to be tested. */
typedef struct Sd_Node_
{
    bool            available;          /*!< \brief node available? *//*! i todo RWI:  */
    uint16_t        node_address;       /*!< \brief node address used for welcome command */
    uint8_t         result;             /*!< \brief result parameter of Welcome.Result message */
    uint8_t         version;            /*!< \brief version parameter of Hello and Welcome messages */
    Ucs_Signature_t signature;          /*!< \brief signature of the node */

} Sd_Node;



/*! \brief   Structure of class CSysDiag. */
typedef struct CSysDiag_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    bool     startup_locked;            /*!< \brief Locking of NetworkStartup without timeout */
    CSingleSubject sysdiag;             /*!< \brief Subject for the System Diagnosis reports */

    CSingleObserver sys_diag_start;     /*!< \brief Observes the Inic_NwSysDiagnosis() command */
    CSingleObserver sys_diag_stop;      /*!< \brief Observes the Inic_NwSysDiagEnd() command */
    CSingleObserver sys_hello;          /*!< \brief Observes the Hello  result */
    CSingleObserver sys_welcome;        /*!< \brief Observes the Welcome result */
    CSingleObserver sys_enable_port;    /*!< \brief Observes enabling a port */
    CSingleObserver sys_disable_port;   /*!< \brief Observes disabling a port */
    CSingleObserver sys_cable_link_diagnosis;   /*!< \brief Observes the CableLinkDiagnosis result */
    CMaskedObserver sys_terminate;      /*!< \brief  Observes events leading to termination */

    CFsm     fsm;                       /*!< \brief System Diagnosis state machine  */
    CService sd_srv;                    /*!< \brief Service instance for the scheduler */

    uint8_t  segment_nr;                /*!< \brief segment number which is currently checked*/
    uint8_t  num_ports;                 /*!< \brief number of ports of master node */
    uint8_t  curr_branch;               /*!< \brief branch which is currently examined */
    uint16_t admin_node_address;        /*!< \brief node address used during system diagnosis */
    Sd_ResultCode_t last_result;        /*!< \brief result of last segment
                                        */
    Sd_Node  master;                    /*!< \brief Timing Master node */
    Sd_Node  source;                    /*!< \brief Source node of segment to be tested  */
    Sd_Node  target;                    /*!< \brief Target node of segment to be tested  */
    uint16_t hello_retry;               /*!< \brief retry counter for hello message  */
    CTimer   timer;                     /*!< \brief timer for monitoring messages */

    Ucs_Sd_Report_t report;             /*!< \brief reports segment results */

} CSysDiag;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void SysDiag_Ctor(CSysDiag *self, CInic *inic, CBase *base, CExc *exc);
extern Ucs_Return_t SysDiag_Run(CSysDiag *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t SysDiag_Abort(CSysDiag *self);




#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_SYS_DIAG_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

