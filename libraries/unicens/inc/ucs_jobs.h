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
 * \brief Internal header file of Job classes
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_JOBS
 * @{
 */

#ifndef UCS_JOBS_H
#define UCS_JOBS_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
struct CJob_;
typedef struct CJob_ CJob;

/*! \brief   Starts up the MOST Network
 *  \param   self       The instance
 */
typedef void (*Job_StartCb_t)(void *self);

/*------------------------------------------------------------------------------------------------*/
/* CJob Class                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Job state */
typedef enum Job_State_
{
    JOB_S_STOPPED  = 0U,
    JOB_S_STARTED  = 1U,
    JOB_S_FINISHED = 2U

} Job_State_t;

/*! \brief Job result */
typedef enum Job_Result_
{
    JOB_R_NA       = 0U,
    JOB_R_SUCCESS  = 1U,
    JOB_R_FAILED   = 2U

} Job_Result_t;

/*! \brief      Job class
 *  \details    Definition of job class
 */
struct CJob_
{
    void           *inst_ptr;
    Job_StartCb_t   start_fptr;
    Job_State_t     state;
    Job_Result_t    result;
    CSingleSubject  subject;
};

/*------------------------------------------------------------------------------------------------*/
/* CJob Methods                                                                                   */
/*------------------------------------------------------------------------------------------------*/
void Job_Ctor(CJob *self, Job_StartCb_t start_fptr, void *inst_ptr);
void Job_Start(CJob *self, CSingleObserver *result_obs_ptr);
void Job_Stop(CJob *self);
void Job_SetResult(CJob *self, Job_Result_t result);
Job_State_t Job_GetState(CJob *self);
Job_Result_t Job_GetResult(CJob *self);


/*------------------------------------------------------------------------------------------------*/
/* CJobQ Class                                                                                    */
/*------------------------------------------------------------------------------------------------*/
struct CJobService_;
typedef struct CJobService_ CJobService;

/*! \brief      JobQ class
 *  \details    Definition of job queue class
 */
typedef struct CJobQ_
{
    Srv_Event_t     event_id;
    Job_State_t     state;
    Job_Result_t    result;
    CJob**          job_list;
    CJobService*    job_service_ptr;
    
    uint8_t         index;          /*! \brief The index of current job */
    CSingleObserver result_obs;     /*! \brief Required to get the job result */
    CSingleSubject  q_subject;      /*! \brief Notifies the JobQ result */
    CDlNode         node;           /*! \brief Required node to add JobQ to JobService class*/

} CJobQ;

/*------------------------------------------------------------------------------------------------*/
/* CJobQ Methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
void Jbq_Ctor(CJobQ *self, CJobService *job_service_ptr, Srv_Event_t event_id, CJob *job_list[]);
void Jbq_Start(CJobQ *self, CSingleObserver *result_obs_ptr);
void Jbq_Stop(CJobQ *self);
void Jbq_Service(CJobQ *self);
Srv_Event_t Jbq_GetEventId(CJobQ *self);


/*------------------------------------------------------------------------------------------------*/
/* CJobService Class                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Job Service class
 *  \details    Definition of job service class
 */
struct CJobService_
{
    CBase          *base_ptr;
    CDlList         list;
    CService        service;
};

/*------------------------------------------------------------------------------------------------*/
/* CJobService Methods                                                                            */
/*------------------------------------------------------------------------------------------------*/
void Jbs_Ctor(CJobService *self, CBase *base_ptr);
void Jbs_RegisterJobQ(CJobService *self, CDlNode *job_q_node);
void Jbs_TriggerEvent(CJobService *self, Srv_Event_t id);


#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_JOBS_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

