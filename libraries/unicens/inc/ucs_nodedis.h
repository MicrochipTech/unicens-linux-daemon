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
 * \brief Internal header file of class CNodeDiscovery.
 *
 * \cond UCS_INTERNAL_DOC
 */
#ifndef UCS_NODEDIS_H
#define UCS_NODEDIS_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief max number of nodes 
 *  \ingroup G_UCS_NODE_DISCOVERY
 */

#define ND_NUM_NODES            40U     



/*! \brief Function signature of node evaluation callback used by Node Discovery service.
 *
 *  The Node Discovery service announces the signature of each node it has found to the 
 *  application via the evaluation function. In this function the application 
 *  decides how the Node Discovery service shall proceed with the node.
 *  The application maintains two lists:  
 * 
 *  <dl> 
 *      <dt> *set_list* </dt>
 *      <dd> Contains the signatures of the nodes the system shall contain
 *  
 *      <dt> *device_list* </dt>
 *      <dd> Contains the signatures of the nodes detected in the system
 *  </dl>
 *
 *  The evaluation has to follow these rules: 
 *  - If the node is not part of the *set_list*, it is regarded as unknown (\ref UCS_ND_CHK_UNKNOWN)
 *    and will be ignored. 
 *  - If the node is part of the *set_list* and is not yet in the *device_list*, the Node Discovery 
 *    Service shall try to add the node to network (\ref UCS_ND_CHK_WELCOME). 
 *  - If the node is already part of the *device_list*, there are two possibilities: the node in the 
 *    *device_list* experienced a reset or there are two nodes with the same signature. Evaluation 
 *    result is \ref UCS_ND_CHK_UNIQUE. The Node Discovery service will perform further tests.
 *
 *  \param    self         The instance
 *  \param    signature    Signature of the respective node
 *  \returns  UCS_ND_CHK_WELCOME  Node is ok, try to add it to the network.
 *  \returns  UCS_ND_CHK_UNIQUE   Test if this node is unique.
 *  \returns  UCS_ND_CHK_UNKNOWN  Node is unknown, no further action.
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef Ucs_Nd_CheckResult_t (*Nd_EvalCb_t)(void *self, Ucs_Signature_t *signature);

/*! \brief Function signature of result callback used by Node Discovery service.
 *
 *  The Node Discovery service reports the result of each node and some system events by
 *  this callback function.
 *  
 *  \note The parameter <b>signature</b> will be NULL, if parameter <b>code</b> is 
 *  \ref UCS_ND_RES_STOPPED, \ref UCS_ND_RES_NETOFF or \ref UCS_ND_RES_ERROR.
 *
 *  \param   self         The instance
 *  \param   code         Result code 
 *  \param   signature    Signature of the respective node
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef void (*Nd_ReportCb_t)(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature);


/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Structure decribing a node. 
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef struct Nd_Node_
{
    /*bool               available; */      /*!< \brief node available? *//*! i todo RWI:  */
    /*uint16_t           node_address; */   /*!< \brief node address used for welcome command */
    /*uint8_t            result;     */     /*!< \brief result parameter of Welcome.Result message */
    /*uint8_t            version;  */       /*!< \brief version parameter of Hello and Welcome messages */
    Ucs_Signature_t    signature;           /*!< \brief signature of the node */
    CDlNode            node;                /*!< \brief enables listing */  

} Nd_Node;


/*! \brief  Initialization structure of the Node Discovery service. 
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef struct Nd_InitData_
{
    void               *inst_ptr;           /*!< \brief The instance used when invoking the callback functions */           
    Nd_ReportCb_t       report_fptr;        /*!< \brief Report callback function */
    Nd_EvalCb_t         eval_fptr;          /*!< \brief Evaluation callback function */

} Nd_InitData_t;




/*! \brief   Structure of class CNodeDiscovery. 
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef struct CNodeDiscovery_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    bool    running;                    /*!< \brief Indicates th Node Discovery is running. */
    CSingleObserver nd_hello;           /*!< \brief Observes the Hello  result */
    CSingleObserver nd_welcome;         /*!< \brief Observes the Welcome result */
    CSingleObserver nd_signature;       /*!< \brief Observes the Signature result */
    CSingleObserver nd_init;            /*!< \brief Observes the DeviceInit result */ 

    CMaskedObserver nd_terminate;       /*!< \brief Observes events leading to termination */

    CObserver       nd_nwstatus;        /*!< \brief Observes the MOST Network status */

    CFsm     fsm;                       /*!< \brief Node Discovery state machine  */
    CService service;                   /*!< \brief Service instance for the scheduler */

    CTimer   timer;                     /*!< \brief timer for monitoring messages */

    bool     debounce_flag;             /*!< \brief Prevents that a Hello.Get is sent while waiting 
                                                    for answers of a previous Hello.Get command. */
    CTimer   debounce_timer;            /*!< \brief debounces Hello.Get requests. */


    CDlList  new_list;                  /*!< \brief list of detected nodes */
    CDlList  unused_list;               /*!< \brief list of unused node elements */
    Nd_Node  nodes[ND_NUM_NODES];       /*!< \brief device nodes */ 
    Ucs_Signature_t  current_sig;       /*!< \brief node which is checked currently */ 

    Exc_WelcomeResult_t   welcome_result;       /*!< \brief buffer for welcome result */
    Exc_SignatureStatus_t signature_status;     /*!< \brief buffer for signature status */

    bool stop_request;                  /*!< \brief indicates a request to stop node discovery */
    bool hello_mpr_request;             /*!< \brief indicates an Hello.Get request due to an MPR event*/
    bool hello_neton_request;           /*!< \brief indicates an Hello.Get request due to an NetOn event*/
    bool neton;                         /*!< \brief indicates Network availability */

    void *cb_inst_ptr;                  /*!< \brief Instance required for callback functions */
    Nd_ReportCb_t report_fptr;          /*!< \brief Report callback function */
    Nd_EvalCb_t eval_fptr;              /*!< \brief Node evaluation callback function */

}CNodeDiscovery;



/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Nd_Ctor(CNodeDiscovery *self, 
             CInic *inic, 
             CBase *base, 
             CExc *exc, 
             Nd_InitData_t *init_ptr);


extern Ucs_Return_t Nd_Start(CNodeDiscovery *self);
extern Ucs_Return_t Nd_Stop(CNodeDiscovery *self);
extern void Nd_InitAll(CNodeDiscovery *self);




#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_NODEDIS_H */
/*!
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

