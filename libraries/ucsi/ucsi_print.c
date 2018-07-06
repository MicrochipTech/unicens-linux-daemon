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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Console.h"
#include "ucsi_cfg.h"
#include "ucsi_print.h"

#ifdef ENABLE_RESOURCE_PRINT

#define RESETCOLOR "\033[0m"
#define GREEN      "\033[0;32m"
#define RED        "\033[0;31m"
#define YELLOW     "\033[1;33m"
#define BLUE       "\033[0;34m"

#define STR_BUF_LEN (200)
#define STR_RES_LEN (60)

struct ResourceList
{
    Ucs_Xrm_ResObject_t *element;
    UCSIPrint_ObjectState_t state;
};

struct NodeList
{
    bool isValid;
    bool isAvailable;
    uint16_t node;
};

struct LocalVar
{
    bool initialized;
    void *tag;
    Ucs_Rm_Route_t *pRoutes;
    uint16_t routesSize;
    struct ResourceList rList[UCSI_PRINT_MAX_RESOURCES];
    struct NodeList nList[UCSI_PRINT_MAX_NODES];
};

static struct LocalVar m = { 0 };
static char strBuf[STR_BUF_LEN];

static void ParseResources(Ucs_Xrm_ResObject_t **ppJobList, char *pBuf, uint32_t bufLen);
static bool IsNodeAvailable(uint16_t nodeAddress);

void UCSIPrint_Init(Ucs_Rm_Route_t *pRoutes, uint16_t routesSize, void *tag)
{
    memset(&m, 0, sizeof(struct LocalVar));
    if (NULL == pRoutes || 0 == routesSize)
        return;
    m.tag = tag;
    m.pRoutes = pRoutes;
    m.routesSize = routesSize;
    m.initialized = true;
}

void UCSIPrint_ShowTable(void)
{
    uint16_t i;
    char inRes[STR_RES_LEN];
    char outRes[STR_RES_LEN];
    if (!m.initialized)
        return;
    UCSIPrint_CB_OnUserMessage(m.tag, "-----------------------------------------------------------------------");
    UCSIPrint_CB_OnUserMessage(m.tag, "Source | Sink  | Act | ID     | Resources");
    for (i = 0; i < m.routesSize; i++)
    {
        const char *sourceAvail = "";
        const char *sinkAvail = "";
        const char *resetCol = "";
        uint16_t srcAddr = m.pRoutes[i].source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
        uint16_t snkAddr = m.pRoutes[i].sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
        uint8_t active = m.pRoutes[i].active;
        uint16_t id = m.pRoutes[i].route_id;
        ParseResources(m.pRoutes[i].source_endpoint_ptr->jobs_list_ptr, inRes, sizeof(inRes));
        ParseResources(m.pRoutes[i].sink_endpoint_ptr->jobs_list_ptr, outRes, sizeof(outRes));
        if (IsNodeAvailable(srcAddr))
        {
            sourceAvail = GREEN;
            resetCol = RESETCOLOR;
        }
        if (IsNodeAvailable(snkAddr))
        {
            sinkAvail = GREEN;
            resetCol = RESETCOLOR;
        }
        snprintf(strBuf, STR_BUF_LEN, "%s0x%03X%s  | %s0x%03X%s |  %d  | 0x%04X | Src:%s  Snk:%s",
            sourceAvail, srcAddr, resetCol, sinkAvail, snkAddr, resetCol, active, id, inRes, outRes);
        UCSIPrint_CB_OnUserMessage(m.tag, strBuf);
    }
    UCSIPrint_CB_OnUserMessage(m.tag, "-----------------------------------------------------------------------");
}

void UCSIPrint_SetNodeAvailable(uint16_t nodeAddress, bool isAvailable)
{
    uint16_t i;
    if (!m.initialized)
        return;
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && nodeAddress == m.nList[i].node)
        {
            m.nList[i].isAvailable = isAvailable;
            return;
        }
    }
    /* Find empty entry and store it there */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (!m.nList[i].isValid)
        {
            m.nList[i].node = nodeAddress;
            m.nList[i].isAvailable = isAvailable;
            m.nList[i].isValid = true;
            return;
        }
    }
    UCSIPrint_CB_OnUserMessage(m.tag, RED "Not enough resources for UCSIPrint component, increase UCSI_PRINT_MAX_NODES" RESETCOLOR "\r\n");
}

void UCSIPrint_SetObjectState(Ucs_Xrm_ResObject_t *element, UCSIPrint_ObjectState_t state)
{
    uint16_t i;
    if (!m.initialized)
        return;
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_RESOURCES; i++)
    {
        if (element == m.rList[i].element)
        {
            m.rList[i].state = state;
            return;
        }
    }
    /* Find empty entry and store it there */
    for (i = 0; i < UCSI_PRINT_MAX_RESOURCES; i++)
    {
        if (NULL == m.rList[i].element)
        {
            m.rList[i].element = element;
            m.rList[i].state = state;
            return;
        }
    }
    UCSIPrint_CB_OnUserMessage(m.tag, RED "Not enough resources for UCSIPrint component, increase UCSI_PRINT_MAX_RESOURCES" RESETCOLOR "\r\n");
}

static void ParseResources(Ucs_Xrm_ResObject_t **ppJobList, char *pBuf, uint32_t bufLen)
{
    uint16_t i, j;
    Ucs_Xrm_ResObject_t *job;
    UCSIPrint_ObjectState_t oldState = ObjState_Unused;
    UCSIPrint_ObjectState_t newState = ObjState_Unused;
    assert(NULL != pBuf && 0 != bufLen);
    pBuf[0] = '\0';
    if (NULL == ppJobList)
        return;
    for (i = 0; NULL != (job = ppJobList[i]); i++)
    {
        Ucs_Xrm_ResourceType_t typ = *((Ucs_Xrm_ResourceType_t *)job);
        assert(UCS_XRM_RC_TYPE_QOS_CON >= typ);
        /* Silently ignore default created port */
        if (UCS_XRM_RC_TYPE_DC_PORT == typ)
            continue;
        for (j = 0; j < UCSI_PRINT_MAX_RESOURCES; j++)
        {
            if (NULL == m.rList[j].element)
                break;
            newState = ObjState_Unused;
            if (job == m.rList[j].element)
            {
                newState = m.rList[j].state;
                break;
            }
        }
        if (oldState != newState)
        {
            oldState = newState;
            if (ObjState_Build == newState)
                strcat(pBuf, GREEN);
            else if (ObjState_Build == newState)
                strcat(pBuf, RED);
            else
                strcat(pBuf, RESETCOLOR);
        }
        switch(typ)
        {
        case UCS_XRM_RC_TYPE_MOST_SOCKET:
            strcat(pBuf, "NS ");
            break;
        case UCS_XRM_RC_TYPE_MLB_PORT:
            strcat(pBuf, "MP ");
            break;
        case UCS_XRM_RC_TYPE_MLB_SOCKET:
            strcat(pBuf, "MS ");
            break;
        case UCS_XRM_RC_TYPE_USB_PORT:
            strcat(pBuf, "UP ");
            break;
        case UCS_XRM_RC_TYPE_USB_SOCKET:
            strcat(pBuf, "US ");
            break;
        case UCS_XRM_RC_TYPE_STRM_PORT:
            strcat(pBuf, "SP ");
            break;
        case UCS_XRM_RC_TYPE_STRM_SOCKET:
            strcat(pBuf, "SS ");
            break;
        case UCS_XRM_RC_TYPE_SYNC_CON:
            strcat(pBuf, "SC ");
            break;
        case UCS_XRM_RC_TYPE_COMBINER:
            strcat(pBuf, "C ");
            break;
        case UCS_XRM_RC_TYPE_SPLITTER:
            strcat(pBuf, "S ");
            break;
        case UCS_XRM_RC_TYPE_AVP_CON:
            strcat(pBuf, "AC ");
            break;
        default:
            strcat(pBuf, "E ");
            break;
        }
    }
    if (ObjState_Unused != newState)
        strcat(pBuf, RESETCOLOR);
    assert(strlen(pBuf) < bufLen);
}

static bool IsNodeAvailable(uint16_t nodeAddress)
{
    uint16_t i;
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && nodeAddress == m.nList[i].node)
        {
            return m.nList[i].isAvailable;
        }
    }
    return false;
}

#else /* ENABLE_RESOURCE_PRINT */
void UCSIPrint_Init(Ucs_Rm_Route_t *pRoutes, uint16_t routesSize, void *tag) {}
void UCSIPrint_ShowTable(void) {}
void UCSIPrint_SetNodeAvailable(uint16_t nodeAddress, bool isAvailable) {}
void UCSIPrint_SetObjectState(Ucs_Xrm_ResObject_t *element, UCSIPrint_ObjectState_t state) {}
#endif /* ENABLE_RESOURCE_PRINT */
