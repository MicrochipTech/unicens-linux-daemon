/*------------------------------------------------------------------------------------------------*/
/* UNICENS Stucture Printing module                                                               */
/* Copyright 2018, Microchip Technology Inc. and its subsidiaries.                                */
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
#ifndef UCSI_PRINT_H_
#define UCSI_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>
#include "ucs_api.h"
#include "ucs_cfg.h"
#include "ucs_xrm_cfg.h"

#define UCSI_PRINT_MAX_NODES (UCS_NUM_REMOTE_DEVICES + 1)
#define UCSI_PRINT_MAX_RESOURCES (UCS_XRM_NUM_RESOURCES)

typedef enum
{
    ObjState_Unused,
    ObjState_Build,
    ObjState_Failed
} UCSIPrint_ObjectState_t;

typedef enum
{
    NodeState_NotAvailable,
    NodeState_Ignored,
    NodeState_Available
} UCSIPrint_NodeState_t;

void UCSIPrint_Init(Ucs_Rm_Route_t *pRoutes, uint16_t routesSize, void *tag);
void UCSIPrint_Service(uint32_t timestamp);
void UCSIPrint_SetNetworkAvailable(bool available, uint8_t maxPos);
void UCSIPrint_SetNodeAvailable(uint16_t nodeAddress, UCSIPrint_NodeState_t nodeState);
void UCSIPrint_SetRouteState(uint16_t routeId, bool isActive, uint16_t connectionLabel);
void UCSIPrint_SetObjectState(Ucs_Xrm_ResObject_t *element, UCSIPrint_ObjectState_t state);
void UCSIPrint_UnicensActivity(void);

/**
 * \brief Callback when ever UNICENS_PRINT needs to be serviced. Call UCSIPrint_Service in next service cycle.
 * \param tag - user pointer given along with UCSIPrint_Init
 */
extern void UCSIPrint_CB_NeedService(void *tag);

/**
 * \brief Callback when ever UNICENS_PRINT forms a human readable message.
 * \param tag - user pointer given along with UCSIPrint_Init
 * \param pMsg - zero terminated human readable string
 */
extern void UCSIPrint_CB_OnUserMessage(void *tag, const char pMsg[]);

#ifdef __cplusplus
}
#endif

#endif
