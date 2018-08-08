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
#include "ucsi_cfg.h"
#include "ucsi_print.h"

#ifdef ENABLE_RESOURCE_PRINT

#define SERVICE_TIME (500)
#define MAX_TIMEOUT  (30000)
#define MPR_RETRIES  (5)
#define INVALID_CON_LABEL (0xDEAD)

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

struct ConnectionList
{
    bool isValid;
    bool isActive;
    uint16_t routeId;
    uint16_t connectionLabel;
};

struct NodeList
{
    bool isValid;
    UCSIPrint_NodeState_t nodeState;
    uint16_t node;
};

struct LocalVar
{
    bool initialized;
    bool triggerService;
    uint32_t nextService;
    uint32_t timeOut;
    void *tag;
    Ucs_Rm_Route_t *pRoutes;
    uint16_t routesSize;
    bool networkAvailable;
    uint8_t mpr;
    uint8_t waitForMprRetries;
    struct ResourceList rList[UCSI_PRINT_MAX_RESOURCES];
    struct ConnectionList cList[UCSI_PRINT_MAX_RESOURCES];
    struct NodeList nList[UCSI_PRINT_MAX_NODES];
};

static struct LocalVar m = { 0 };
static char strBuf[STR_BUF_LEN];
static void PrintTable(void);
static void ParseResources(Ucs_Xrm_ResObject_t **ppJobList, char *pBuf, uint32_t bufLen);
static bool GetIgnoredNodeString(char *pBuf, uint32_t bufLen);
static UCSIPrint_NodeState_t GetNodeState(uint16_t nodeAddress);
static uint8_t GetNodeCount(void);
static bool GetRouteState(uint16_t routeId, bool *pIsActive, uint16_t *pConLabel);
static void RequestTrigger(void);

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

void UCSIPrint_Service(uint32_t timestamp)
{
    bool exec = false;
    if (!m.networkAvailable)
        return;
    if (m.triggerService)
    {
        m.triggerService = false;
        m.nextService = timestamp + SERVICE_TIME;
        if (0 == m.timeOut)
            m.timeOut = timestamp + MAX_TIMEOUT;
        UCSIPrint_CB_NeedService(m.tag);
        return;
    }
    if (0 == m.nextService || 0 == m.timeOut)
        return;
    if (timestamp >= m.timeOut)
    {
        UCSIPrint_CB_OnUserMessage(m.tag, RED "UCSI-Watchdog:Max timeout reached" RESETCOLOR);
        exec = true;
    }
    else if (timestamp >= m.nextService)
    {
        if (m.mpr != GetNodeCount() && ++m.waitForMprRetries <= MPR_RETRIES)
        {
            m.nextService = timestamp + SERVICE_TIME;
            return;
        }
        exec = true;
    }
    if (exec)
    {
        m.nextService = 0;
        m.timeOut = 0;
        PrintTable();
    }
    else
    {
        UCSIPrint_CB_NeedService(m.tag);
    }
}

void UCSIPrint_SetNetworkAvailable(bool available, uint8_t maxPos)
{
    m.networkAvailable = available;
    m.mpr = maxPos;
    m.waitForMprRetries = 0;
    if (available)
    {
        RequestTrigger();
    } else {
       m.triggerService = false;
       m.nextService = 0;
    }
}

void UCSIPrint_SetNodeAvailable(uint16_t nodeAddress, UCSIPrint_NodeState_t nodeState)
{
    uint16_t i;
    if (!m.initialized)
        return;
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && nodeAddress == m.nList[i].node)
        {
            if (m.nList[i].nodeState != nodeState)
            {
                m.nList[i].nodeState = nodeState;
                RequestTrigger();
            }
            return;
        }
    }
    /* Find empty entry and store it there */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (!m.nList[i].isValid)
        {
            m.nList[i].node = nodeAddress;
            m.nList[i].nodeState = nodeState;
            m.nList[i].isValid = true;
            RequestTrigger();
            return;
        }
    }
    UCSIPrint_CB_OnUserMessage(m.tag, RED "UCSI-Watchdog:Could not store node availability, increase UCSI_PRINT_MAX_NODES" RESETCOLOR);
}

void UCSIPrint_SetRouteState(uint16_t routeId, bool isActive, uint16_t connectionLabel)
{
    uint16_t i;
    if (!m.initialized)
        return;
    RequestTrigger();
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_RESOURCES; i++)
    {
        if (m.cList[i].isValid && routeId == m.cList[i].routeId)
        {
            m.cList[i].connectionLabel = connectionLabel;
            m.cList[i].isActive = isActive;
            return;
        }
    }
    /* Find empty entry and store it there */
    for (i = 0; i < UCSI_PRINT_MAX_RESOURCES; i++)
    {
        if (!m.cList[i].isValid)
        {
            m.cList[i].routeId = routeId;
            m.cList[i].isActive = isActive;
            m.cList[i].connectionLabel = connectionLabel;
            m.cList[i].isValid = true;
            return;
        }
    }
    UCSIPrint_CB_OnUserMessage(m.tag, RED "UCSI-Watchdog:Could not store connection label, increase UCSI_PRINT_MAX_RESOURCES" RESETCOLOR);
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
            if (m.rList[i].state != state)
            {
                m.rList[i].state = state;
                RequestTrigger();
            }
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
            RequestTrigger();
            return;
        }
    }
    UCSIPrint_CB_OnUserMessage(m.tag, RED "UCSI-Watchdog:Could not store object state, increase UCSI_PRINT_MAX_RESOURCES" RESETCOLOR);
}

void UCSIPrint_UnicensActivity(void)
{
    if (0 != m.nextService)
        RequestTrigger();
    else
        UCSIPrint_CB_NeedService(m.tag);
}

static void PrintTable(void)
{
    uint16_t i;
    static char inRes[STR_RES_LEN];
    static char outRes[STR_RES_LEN];
    if (!m.initialized)
        return;
    if (!m.networkAvailable)
        return;
    UCSIPrint_CB_OnUserMessage(m.tag, "---------------------------------------------------------------------------------------");
    UCSIPrint_CB_OnUserMessage(m.tag, " Source | Sink   | Active   | ID     | Label  | Resources");
    for (i = 0; i < m.routesSize; i++)
    {
        const char *sourceAvail = " ";
        const char *sourceReset = "";
        const char *sinkAvail = " ";
        const char *sinkReset = "";
        const char *routeAvail = " ";
        const char *routeReset = "";
        uint16_t srcAddr = m.pRoutes[i].source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
        uint16_t snkAddr = m.pRoutes[i].sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
        uint8_t shallActive = m.pRoutes[i].active;
        uint16_t id = m.pRoutes[i].route_id;
        bool isActive = false;
        uint16_t label = INVALID_CON_LABEL;
        UCSIPrint_NodeState_t srcState = GetNodeState(srcAddr);
        UCSIPrint_NodeState_t snkState = GetNodeState(snkAddr);
        GetRouteState(id, &isActive, &label);
        ParseResources(m.pRoutes[i].source_endpoint_ptr->jobs_list_ptr, inRes, sizeof(inRes));
        ParseResources(m.pRoutes[i].sink_endpoint_ptr->jobs_list_ptr, outRes, sizeof(outRes));
        if (NodeState_Available == srcState)
        {
            sourceAvail = GREEN "^";
            sourceReset = RESETCOLOR;
        }
        else if (NodeState_Ignored == srcState)
        {
            sourceAvail = RED "^";
            sourceReset = RESETCOLOR;
        }
        if (NodeState_Available == snkState)
        {
            sinkAvail = GREEN "^";
            sinkReset = RESETCOLOR;
        }
        else if (NodeState_Ignored == snkState)
        {
            sinkAvail = RED "!";
            sinkReset = RESETCOLOR;
        }
        if (NodeState_Available == srcState && NodeState_Available == snkState)
        {
            if (shallActive == isActive)
                routeAvail = GREEN "^";
            else
                routeAvail = RED "!";
            routeReset = RESETCOLOR;
        }
        snprintf(strBuf, STR_BUF_LEN, "%s0x%03X%s  | %s0x%03X%s | S:%d I:%s%d%s | 0x%04X | 0x%04X | Src:%s  Snk:%s",
            sourceAvail, srcAddr, sourceReset, sinkAvail, snkAddr, sinkReset, 
            shallActive, routeAvail, isActive, routeReset, id, label, inRes, outRes);
        UCSIPrint_CB_OnUserMessage(m.tag, strBuf);
    }
    UCSIPrint_CB_OnUserMessage(m.tag, "---------------------------------------------------------------------------------------");
    if (GetIgnoredNodeString(inRes, sizeof(inRes)))
    {
        snprintf(strBuf, STR_BUF_LEN, RED "Ignored nodes = { %s }" RESETCOLOR, inRes);
        UCSIPrint_CB_OnUserMessage(m.tag, strBuf);
        UCSIPrint_CB_OnUserMessage(m.tag, "---------------------------------------------------------------------------------------");
    }
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
            else if (ObjState_Failed == newState)
                strcat(pBuf, RED);
            else
                strcat(pBuf, RESETCOLOR);
        }
        if (ObjState_Build == newState)
            strcat(pBuf, "^");
        else if (ObjState_Failed == newState)
            strcat(pBuf, "!");
        else
            strcat(pBuf, " ");
        switch(typ)
        {
        case UCS_XRM_RC_TYPE_MOST_SOCKET:
            strcat(pBuf, "NS");
            break;
        case UCS_XRM_RC_TYPE_MLB_PORT:
            strcat(pBuf, "MP");
            break;
        case UCS_XRM_RC_TYPE_MLB_SOCKET:
            strcat(pBuf, "MS");
            break;
        case UCS_XRM_RC_TYPE_USB_PORT:
            strcat(pBuf, "UP");
            break;
        case UCS_XRM_RC_TYPE_USB_SOCKET:
            strcat(pBuf, "US");
            break;
        case UCS_XRM_RC_TYPE_STRM_PORT:
            strcat(pBuf, "SP");
            break;
        case UCS_XRM_RC_TYPE_STRM_SOCKET:
            strcat(pBuf, "SS");
            break;
        case UCS_XRM_RC_TYPE_SYNC_CON:
            strcat(pBuf, "SC");
            break;
        case UCS_XRM_RC_TYPE_COMBINER:
            strcat(pBuf, "C");
            break;
        case UCS_XRM_RC_TYPE_SPLITTER:
            strcat(pBuf, "S");
            break;
        case UCS_XRM_RC_TYPE_AVP_CON:
            strcat(pBuf, "AC");
            break;
        default:
            strcat(pBuf, "E");
            break;
        }
    }
    if (ObjState_Unused != newState)
        strcat(pBuf, RESETCOLOR);
    assert(strlen(pBuf) < bufLen);
}

static bool GetIgnoredNodeString(char *pBuf, uint32_t bufLen)
{
    uint16_t i;
    char pTmp[8];
    bool foundNodes = false;
    assert(NULL != pBuf && 0 != bufLen);
    pBuf[0] = '\0';
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && NodeState_Ignored == m.nList[i].nodeState)
        {
            foundNodes = true;
            snprintf(pTmp, sizeof(pTmp), "0x%X ", m.nList[i].node);
            strcat(pBuf, pTmp);
        }
    }
    assert(strlen(pBuf) < bufLen);
    return foundNodes;
}

static UCSIPrint_NodeState_t GetNodeState(uint16_t nodeAddress)
{
    uint16_t i;
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && nodeAddress == m.nList[i].node)
        {
            return m.nList[i].nodeState;
        }
    }
    return NodeState_NotAvailable;
}

static uint8_t GetNodeCount(void)
{
    uint16_t i;
    uint8_t cnt = 0;
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.nList[i].isValid && NodeState_NotAvailable != m.nList[i].nodeState)
            ++cnt;
    }
    return cnt;
}

static bool GetRouteState(uint16_t routeId, bool *pIsActive, uint16_t *pConLabel)
{
    uint16_t i;
    assert(NULL != pIsActive);
    assert(NULL != pConLabel);
    /* Find existing entry */
    for (i = 0; i < UCSI_PRINT_MAX_NODES; i++)
    {
        if (m.cList[i].isValid && routeId == m.cList[i].routeId)
        {
            *pIsActive = m.cList[i].isActive;
            *pConLabel = m.cList[i].connectionLabel;
            return true;
        }
    }
    return false;
}

static void RequestTrigger(void)
{
    m.triggerService = true;
    UCSIPrint_CB_NeedService(m.tag);
}
#else /* ENABLE_RESOURCE_PRINT */
void UCSIPrint_Init(Ucs_Rm_Route_t *pRoutes, uint16_t routesSize, void *tag) {}
void UCSIPrint_Service(uint32_t timestamp) {}
void UCSIPrint_SetNetworkAvailable(bool available, uint8_t maxPos) {}
void UCSIPrint_SetNodeAvailable(uint16_t nodeAddress, UCSIPrint_NodeState_t nodeState) {}
void UCSIPrint_SetRouteState(uint16_t routeId, bool isActive, uint16_t connectionLabel) {}
void UCSIPrint_SetObjectState(Ucs_Xrm_ResObject_t *element, UCSIPrint_ObjectState_t state) {}
void UCSIPrint_UnicensActivity(void) {}
#endif
