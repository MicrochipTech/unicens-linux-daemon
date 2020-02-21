/*------------------------------------------------------------------------------------------------*/
/* Task for UNICENS integration                                                                   */
/* Copyright 2019, Microchip Technology Inc. and its subsidiaries.                                */
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include "Console.h"
#include "ucsi_api.h"
#include "UcsXml.h"
#include "CdevHandler.h"
#include "mld-configurator-v1.h"
#include "mld-configurator-v2.h"
#include "default_config.h"
#include "task-unicens.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#ifdef NO_RAW_CLOCK
#define CLOCK_SRC CLOCK_MONOTONIC
#else
#define CLOCK_SRC CLOCK_MONOTONIC_RAW
#endif

#define CDEV_PATH_LEN (64)
#define DEBUG_TABLE_PRINT_TIME_MS  (250)
#define CABLE_DIAGNOSYS_DELAY      (1000)

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef struct
{
    bool allowRun;
    bool lldTrace;
    bool noRouteTable;
    UcsXmlVal_t *cfg;
    UCSI_Data_t unicens;
    bool unicensRunning;
    bool unicensTimeout;
    bool unicensTrigger;
    bool promiscuousMode;
    bool amsReceived;
    bool unicensDataAvailable;
    bool txErrorState;
    uint32_t cableDiagnosisTimer;
    timer_t ucsTimer;
    sem_t serviceSem;
    CdevData_t ctrlTx;
    CdevData_t ctrlRx;
    char controlRxCdev[CDEV_PATH_LEN];
    char controlTxCdev[CDEV_PATH_LEN];
    uint8_t programNodeCnt;
    bool programPersistent;
} LocalVar_t;

static LocalVar_t m;
static char m_outbuf[300];

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     PRIVATE FUNCTION PROTOTYPES                      */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static bool TimerInitialize(void);
static void TimerSetTimeOut(uint16_t timeout, timer_t timer);
static void UcsTimerOnTimeout(union sigval sv);
static bool SemInitialize(void);
static void SemWait(void);
static void SemPost(void);
static bool InitializeCdevs(void);
static uint32_t GetTicks(void);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

bool TaskUnicens_Init(TaskUnicens_t *pVar)
{
    if (NULL == pVar)
        return false;
    memset(&m, 0, sizeof(LocalVar_t));
    if (NULL != pVar->controlRxCdev && NULL != pVar->controlTxCdev)
    {
        strncpy(m.controlRxCdev, pVar->controlRxCdev, sizeof(m.controlRxCdev));
        strncpy(m.controlTxCdev, pVar->controlTxCdev, sizeof(m.controlTxCdev));
    }
    m.noRouteTable = pVar->noRouteTable;
    m.lldTrace = pVar->lldTrace;
    m.promiscuousMode = pVar->promiscuousMode;
    m.programNodeCnt = pVar->programNodeCnt;
    m.programPersistent = pVar->programPersistent;
    if (!TimerInitialize() || !SemInitialize())
    {
        ConsolePrintf(PRIO_ERROR, RED "Failed to initialize timer/threading resources" RESETCOLOR "\r\n");
        return false;
    }
    if (NULL != pVar->cfgFileName)
    {
        m.cfg = UcsXml_ParseFile(pVar->cfgFileName);
        if (NULL == m.cfg)
        {
            ConsolePrintf(PRIO_ERROR, RED "XML Parser error" RESETCOLOR "\r\n");
            return false;
        }
    }
    /* Initialize UNICENS */
    UCSI_Init(&m.unicens, &m, pVar->debugLocalMsg);
    if (m.programPersistent && 0 == m.programNodeCnt)
    {
        ConsolePrintf(PRIO_ERROR, RED "Can not program persistent without setting amount of nodes (use additional -program)" RESETCOLOR "\r\n");
        return false;
    }
    if (m.cfg)
    {
        if (1 == pVar->drvVersion && 0 != pVar->drvLocalNodeAddr)
        {
            struct timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 300000000l;
            if (!MldConfigV1_Start(m.cfg->ppDriver, m.cfg->driverSize, pVar->drvLocalNodeAddr, pVar->drvFilter, 1000))
            {
                ConsolePrintf(PRIO_ERROR, RED "Could not start driver V1 configuration service" RESETCOLOR "\r\n");
                return false;
            }
            if (NULL == pVar->controlRxCdev && NULL == pVar->controlTxCdev)
            {
                nanosleep(&t, NULL);
                while(!MldConfigV1_GetControlCdevName(m.controlTxCdev, m.controlRxCdev, sizeof(m.controlTxCdev)))
                {
                    ConsolePrintf(PRIO_ERROR, YELLOW "Wait for INICs control channel to appear" RESETCOLOR "\r\n");
                    nanosleep(&t, NULL);
                }
            }
        }
        else if (2 == pVar->drvVersion && 0 != pVar->drvLocalNodeAddr)
        {
            struct timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 300000000l;
            if (!MldConfigV2_Start(m.cfg->ppDriver, m.cfg->driverSize, pVar->drvLocalNodeAddr, pVar->drvFilter, 1000))
            {
                ConsolePrintf(PRIO_ERROR, RED "Could not start driver V2 configuration service" RESETCOLOR "\r\n");
                return false;
            }
            if (NULL == pVar->controlRxCdev && NULL == pVar->controlTxCdev)
            {
                nanosleep(&t, NULL);
                while(!MldConfigV2_GetControlCdevName(m.controlTxCdev, m.controlRxCdev, sizeof(m.controlTxCdev)))
                {
                    ConsolePrintf(PRIO_ERROR, YELLOW "Wait for INICs control channel to appear" RESETCOLOR "\r\n");
                    nanosleep(&t, NULL);
                }
            }
        }
        if (!UCSI_NewConfig(&m.unicens, m.cfg->packetBw, m.cfg->proxyBw, m.cfg->pRoutes, m.cfg->routesSize, m.cfg->pNod, m.cfg->nodSize, m.programNodeCnt, m.programPersistent))
        {
            ConsolePrintf(PRIO_ERROR, RED "Could not enqueue XML generated UNICENS config" RESETCOLOR "\r\n");
            assert(false);
            return false;
        }
    }
    else
    {
        if (!UCSI_NewConfig(&m.unicens, PacketBandwidth, ProxyBandwidth, AllRoutes, RoutesSize, AllNodes, NodeSize, m.programNodeCnt, m.programPersistent))
        {
            ConsolePrintf(PRIO_ERROR, RED "Could not enqueue default UNICENS config" RESETCOLOR "\r\n");
            assert(false);
            return false;
        }
    }
    if (!InitializeCdevs())
    {
        ConsolePrintf(PRIO_ERROR, RED "Failed to initialize Control CDEVs" RESETCOLOR "\r\n");
        return false;
    }
    m.allowRun = true;
    return true;
}

void TaskUnicens_Service(void)
{
    /* UNICENS Service */
    if (m.unicensTrigger)
    {
        m.unicensTrigger = false;
        UCSI_Service(&m.unicens);
    }
    if (m.unicensTimeout)
    {
        m.unicensTimeout = false;
        UCSI_Timeout(&m.unicens);
    }
    if (m.unicensDataAvailable)
    {
        uint8_t *pData;
        uint32_t len;
        if (Cdev_GetRx(&m.ctrlRx, &pData, &len))
        {
            if (!m.unicensRunning)
            {
                /* Discard data, UNICENS is not yet ready */
                m.unicensDataAvailable = false;
                Cdev_PopRx(&m.ctrlRx);
            }
            else if (UCSI_ProcessRxData(&m.unicens, pData, len))
            {
                if (m.lldTrace)
                {
                    uint32_t i;
                    ConsolePrintfStart( PRIO_HIGH, YELLOW "%08d: MSG_RX: ", GetTicks());
                    for ( i = 0; i < len; i++ )
                    {
                        ConsolePrintfContinue( "%02X ", pData[i] );
                    }
                    ConsolePrintfExit(RESETCOLOR"\n");
                }
                /*Remove flag only in case of successful enqueuing*/
                m.unicensDataAvailable = false;
                Cdev_PopRx(&m.ctrlRx);
            }
            else
            {
                ConsolePrintf(PRIO_ERROR, "RX buffer overflow\r\n");
                /* UNICENS is busy. Try to reactive it, by calling service routine */
                m.unicensTrigger = true;
            }
        }
        else assert(false); /*Must not happen*/
    }
    if (m.amsReceived)
    {
        uint16_t amsId = 0xFFFF;
        uint16_t sourceAddress = 0xFFFF;
        uint8_t *pBuf = NULL;
        uint32_t len = 0;
        m.amsReceived = false;
        if (UCSI_GetAmsMessage(&m.unicens, &amsId, &sourceAddress, &pBuf, &len))
        {
            if (m.lldTrace)
            {
                ConsolePrintf(PRIO_HIGH, "Received AMS, id=0x%X, source=0x%X, len=%u\r\n", amsId, sourceAddress, len);
            }
            UCSI_ReleaseAmsMessage(&m.unicens);
        }
        else assert(false);
    }
    if (m.cableDiagnosisTimer && GetTicks() >= m.cableDiagnosisTimer) {
        m.cableDiagnosisTimer = 0;
        ConsolePrintf(PRIO_HIGH, "Starting network diagnosis..\r\n");
        UCSI_RunCableDiagnosis(&m.unicens);
    }
    SemWait();
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  CALLBACK FUNCTION FROM XML PARSER                   */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void UcsXml_CB_OnError(const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    va_start(argptr, vargsCnt);
    vsnprintf(m_outbuf, sizeof(m_outbuf), format, argptr);
    va_end(argptr);
    ConsolePrintf(PRIO_ERROR, RED "XML-Parser error: '%s'" RESETCOLOR "\r\n", m_outbuf);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*               CALLBACK FUNCTION FROM CDEV RX THREAD                  */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void Cdev_CB_OnDataAvailable()
{
    m.unicensDataAvailable = true;
    SemPost();
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  CALLBACK FUNCTIONS FROM UNICENS                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void UCSI_CB_OnCommandResult(void *pTag, UnicensCmd_t command, bool success, uint16_t nodeAddress)
{
    pTag = pTag;
    command = command;
    success = success;
    nodeAddress = nodeAddress;
}

uint16_t UCSI_CB_OnGetTime(void *pTag)
{
    pTag = pTag;
    return GetTicks();
}

void UCSI_CB_OnSetServiceTimer(void *pTag, uint16_t timeout)
{
    pTag = pTag;
    TimerSetTimeOut(timeout, m.ucsTimer);
}

void UCSI_CB_OnNetworkState(void *pTag, bool isAvailable, uint16_t packetBandwidth, uint8_t amountOfNodes)
{
    pTag = pTag;
    ConsolePrintf(PRIO_HIGH, YELLOW "Network isAvailable=%s, packetBW=%d, nodeCount=%d" RESETCOLOR "\r\n",
                  isAvailable ? "yes" : "no",
                  packetBandwidth,
                  amountOfNodes);
    if (isAvailable) {
        m.cableDiagnosisTimer = 0;
    } else {
        m.cableDiagnosisTimer = GetTicks() + CABLE_DIAGNOSYS_DELAY;
    }
}

void UCSI_CB_OnUserMessage(void *pTag, UCSI_UserMessageUrgency_t urgency, const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    pTag = pTag;
    va_start(argptr, vargsCnt);
    vsnprintf(outbuf, sizeof(outbuf), format, argptr);
    va_end(argptr);
    switch(urgency)
    {
        case UCSI_MsgError:
            ConsolePrintf(PRIO_ERROR, RED "%s" RESETCOLOR "\r\n", outbuf);
            break;
        case UCSI_MsgUrgent:
            ConsolePrintf(PRIO_HIGH, YELLOW "%s" RESETCOLOR "\r\n", outbuf);
            break;            
        case UCSI_MsgDebug:
        default:
            ConsolePrintf(PRIO_LOW, "%s\r\n", outbuf);
            break;
    }
}

void UCSI_CB_OnPrintRouteTable(void *pTag, const char pString[])
{
    ConsolePrintf(PRIO_HIGH, "%s\r\n", pString);
}

void UCSI_CB_OnServiceRequired(void *pTag)
{
    pTag = pTag;
    m.unicensTrigger = true;
    SemPost();
}

void UCSI_CB_OnResetInic(void *pTag)
{
    pTag = pTag;
    /* TODO: implement */
}

void UCSI_CB_OnTxRequest(void *pTag,
    const uint8_t *pPayload, uint32_t payloadLen)
{
    pTag = pTag;
    if (m.lldTrace)
    {
        uint32_t i;
        ConsolePrintfStart( PRIO_HIGH, BLUE "%08d: MSG_TX: ", GetTicks());
        for ( i = 0; i < payloadLen; i++ )
        {
            ConsolePrintfContinue( "%02X ", pPayload[i] );
        }
        ConsolePrintfExit(RESETCOLOR"\n");
    }
    if(Cdev_Write(&m.ctrlTx, pPayload, payloadLen))
    {
        if (m.txErrorState)
        {
            m.txErrorState = false;
            ConsolePrintf(PRIO_ERROR, GREEN "CDEV TX (%s) opened" RESETCOLOR "\r\n",
                    m.controlTxCdev);
        }
    }
    else if (!m.txErrorState)
    {
        m.txErrorState = true;
        ConsolePrintf(PRIO_ERROR, RED "CDEV TX error (%s), reason='%s'" RESETCOLOR "\r\n",
            m.controlTxCdev, GetErrnoString());
    }
}

void UCSI_CB_OnStart(void *pTag)
{
    pTag = pTag;
    m.unicensRunning = true;
}

void UCSI_CB_OnStop(void *pTag)
{
    pTag = pTag;
    m.unicensRunning = false;
}

void UCSI_CB_OnAmsMessageReceived(void *pTag)
{
    pTag = pTag;
    assert(pTag == &m);
    m.amsReceived = true;
    SemPost();
}

void UCSI_CB_OnRouteResult(void *pTag, uint16_t routeId, bool isActive, uint16_t connectionLabel)
{
    pTag = pTag;
    if (isActive)
        ConsolePrintf(PRIO_MEDIUM, "Route id=0x%X isActive=true ConLabel=0x%X\r\n", routeId, connectionLabel);
    else
        ConsolePrintf(PRIO_MEDIUM, "Route id=0x%X isActive=" YELLOW "false" RESETCOLOR " ConLabel=0x%X\r\n", routeId, connectionLabel);
}

void UCSI_CB_OnGpioStateChange(void *pTag, uint16_t nodeAddress, uint8_t gpioPinId, bool isHighState)
{
    pTag = pTag;
    ConsolePrintf(PRIO_HIGH, "GPIO state changed, nodeAddress=0x%X, gpioPinId=%d, isHighState=%s\r\n",
                  nodeAddress, gpioPinId, isHighState ? "yes" : "no");
}

void UCSI_CB_OnMgrReport(void *pTag, Ucs_Supv_Report_t code, Ucs_Signature_t *signature, Ucs_Rm_Node_t *pNode)
{
    pTag = pTag;
    if (NULL != signature && UCS_SUPV_REP_AVAILABLE == code) {
        ConsolePrintf(PRIO_MEDIUM, GREEN "*********************************************\r\n");
        ConsolePrintf(PRIO_MEDIUM, "* NODE SIGNATURE:\r\n");
        ConsolePrintf(PRIO_MEDIUM, "* Node Addr=0x%X\r\n", signature->node_address);
        ConsolePrintf(PRIO_MEDIUM, "* Group Addr=0x%X\r\n", signature->group_address);
        ConsolePrintf(PRIO_MEDIUM, "* MAC Addr=0x%04X%04X%04X\r\n", signature->mac_47_32, signature->mac_31_16, signature->mac_15_0);
        ConsolePrintf(PRIO_MEDIUM, "* Node Pos Addr=0x%X\r\n", signature->node_pos_addr);
        ConsolePrintf(PRIO_MEDIUM, "* Diagosis Id=0x%X\r\n", signature->diagnosis_id);
        ConsolePrintf(PRIO_MEDIUM, "* Num Ports=%d\r\n", signature->num_ports);
        ConsolePrintf(PRIO_MEDIUM, "* Chip ID=0x%X\r\n", signature->chip_id);
        ConsolePrintf(PRIO_MEDIUM, "* Firmware=%d.%d.%d.%ld\r\n", signature->fw_major, signature->fw_minor, signature->fw_release, signature->fw_build);
        ConsolePrintf(PRIO_MEDIUM, "* CS=%d.%d.%d\r\n", signature->cs_major, signature->cs_minor, signature->cs_release);
        ConsolePrintf(PRIO_MEDIUM, "*********************************************" RESETCOLOR "\r\n");

        if (m.promiscuousMode) {
            uint16_t nodeAddr = signature->node_address;
            UCSI_EnablePromiscuousMode(&m.unicens, nodeAddr, true);
        }
    }
}

void UCSI_CB_OnI2CRead(void *pTag, bool success, uint16_t targetAddress, uint8_t slaveAddr, const uint8_t *pBuffer, uint32_t bufLen)
{
    if(!success)
         ConsolePrintf(PRIO_ERROR, RED "I2C read failed for node=0x%X slave=0x%X" RESETCOLOR "\r\n", targetAddress, slaveAddr);
}

void UCSI_CB_OnCableDiagnosisResult(void *pTag, uint16_t *pNodeAddrArray, uint8_t arrayLen)
{
    uint8_t i;
    assert(pNodeAddrArray);
    ConsolePrintfStart(PRIO_HIGH, "Cable Diagnosis Result = { ");
    for (i = 0; i < arrayLen; i++) {
        ConsolePrintfContinue("[Pos %d]=0x%X ",i , pNodeAddrArray[i]);
    }
    ConsolePrintfExit("}\r\n");
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      Linux Driver Configurator                       */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void MldConfigV1_CB_OnMessage(bool isError, const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    va_start(argptr, vargsCnt);
    vsprintf(outbuf, format, argptr);
    va_end(argptr);
    if (isError)
        ConsolePrintf(PRIO_ERROR, RED "Driver V1 config error: %s" RESETCOLOR "\r\n", outbuf);
    else
        ConsolePrintf(PRIO_LOW, "Driver V1 config: %s\r\n", outbuf);
}

void MldConfigV2_CB_OnMessage(bool isError, const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    va_start(argptr, vargsCnt);
    vsprintf(outbuf, format, argptr);
    va_end(argptr);
    if (isError)
        ConsolePrintf(PRIO_ERROR, RED "Driver V2 config error: %s" RESETCOLOR "\r\n", outbuf);
    else
        ConsolePrintf(PRIO_LOW, "Driver V2 config: %s\r\n", outbuf);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static bool TimerInitialize(void)
{
    struct sigevent t_sev;
    memset(&t_sev, 0, sizeof(t_sev));
    t_sev.sigev_notify = SIGEV_THREAD;
    t_sev.sigev_notify_function = &UcsTimerOnTimeout;
    t_sev.sigev_value.sival_ptr = NULL;
    if (0 != timer_create(CLOCK_MONOTONIC, &t_sev, &m.ucsTimer))
        return false;
    return true;
}

static void TimerSetTimeOut(uint16_t timeout, timer_t timer)
{
    struct itimerspec t_spec;
    memset(&t_spec, 0, sizeof(t_spec));
    t_spec.it_value.tv_sec = timeout / 1000;
    t_spec.it_value.tv_nsec = (timeout % 1000) * 1000000U;  /* value '0' disarms the timer */
    timer_settime(timer, 0, &t_spec, NULL);
}

static void UcsTimerOnTimeout(union sigval sv)
{
    m.unicensTimeout = true;
    SemPost();
}

static bool SemInitialize(void)
{
    if (-1 == (sem_init(&m.serviceSem, 0, 0)))
        return false;
    return true;
}

static void SemWait(void)
{
    sem_wait(&m.serviceSem);
}

static void SemPost(void)
{
    sem_post(&m.serviceSem);
}

static bool InitializeCdevs(void)
{
    ConsolePrintf(PRIO_LOW, "RX-CDEV='%s', TX-CDEV='%s'\r\n", m.controlRxCdev, m.controlTxCdev);
    if(!Cdev_Init(&m.ctrlTx, m.controlTxCdev, false, true))
        return false;
    if(!Cdev_Init(&m.ctrlRx, m.controlRxCdev, true, false))
        return false;
    if(!Cdev_StartReading(&m.ctrlRx))
        return false;
    return true;
}

static uint32_t GetTicks( void )
{
    struct timespec currentTime;
    if (clock_gettime(CLOCK_SRC, &currentTime))
    {
        assert(false);
        return 0;
    }
    return ( currentTime.tv_sec * 1000 ) + ( currentTime.tv_nsec / 1000000 );
}
