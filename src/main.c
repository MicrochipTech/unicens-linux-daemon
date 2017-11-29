/*------------------------------------------------------------------------------------------------*/
/* UNICENS Daemon (unicensd) main-loop                                                            */
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include "Console.h"
#include "ucsi_api.h"
#include "UcsXml.h"
#include "CdevHandler.h"
#include "default_config.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/* Character device to INIC control channel */
#define DEFAULT_CONTROL_CDEV_TX ("/dev/inic-usb-ctx")
#define DEFAULT_CONTROL_CDEV_RX ("/dev/inic-usb-crx")

/* Debug feature */
/*#define LLD_TRACE*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef struct
{
    bool allowRun;
    UCSI_Data_t unicens;
    bool unicensTimeout;
    bool unicensTrigger;
    bool amsReceived;
    bool unicensDataAvailable;
    bool txErrorState;
    timer_t ucsTimer;
    sem_t serviceSem;
    CdevData_t ctrlTx;
    CdevData_t ctrlRx;
    const char *controlRxCdev;
    const char *controlTxCdev;
} LocalVar_t;

static LocalVar_t m;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     PRIVTATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static char *ReadFile(const char *fileName);
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

int main(int argc, const char *argv[])
{
    bool fileNameSet = false;
    int32_t i;
    char *xmlContent;
    UcsXmlVal_t *cfg = NULL;
    memset(&m, 0, sizeof(LocalVar_t));
    m.controlRxCdev = DEFAULT_CONTROL_CDEV_RX;
    m.controlTxCdev = DEFAULT_CONTROL_CDEV_TX;
    for (i = 1; i < argc; i++)
    {
        if ('-' != argv[i][0])
        {
            if (fileNameSet)
            {
                ConsolePrintf(PRIO_ERROR, RED"Filename is already set. Wrong parameter='%s'"RESETCOLOR"\r\n", argv[i]);
                return -1;
            }
            fileNameSet = true;

            xmlContent = ReadFile(argv[i]);
            if (NULL == xmlContent)
            {
                ConsolePrintf(PRIO_ERROR, RED"File could not be opened: '%s', Reason:'%s'"RESETCOLOR"\r\n",
                    argv[1], GetErrnoString());
                return -1;
            }
            cfg = UcsXml_Parse(xmlContent);
            free(xmlContent);
            if (NULL == cfg)
            {
                ConsolePrintf(PRIO_ERROR, RED"Could not parse UNICENS XML"RESETCOLOR"\r\n");
                return -1;
            }
        }
        else if (0 == strcmp("-crx", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED"-crx parameter needs additional path to RX character device"RESETCOLOR"\r\n");
                return -1;
            }
            m.controlRxCdev = argv[i + 1];
            ++i;
        }
        else if (0 == strcmp("-ctx", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED"-ctx parameter needs additional path to TX character device"RESETCOLOR"\r\n");
                return -1;
            }
            m.controlTxCdev = argv[i + 1];
            ++i;
        }
        else
        {
            ConsolePrintf(PRIO_ERROR, RED"Invalid command line parameter='%s'"RESETCOLOR"\r\n", argv[i]);
            return -1;
        }
    }
    if (!TimerInitialize() || !SemInitialize())
    {
        ConsolePrintf(PRIO_ERROR, RED"Failed to initialize timer/threading resources"RESETCOLOR"\r\n");
        return -1;
    }
    if (!InitializeCdevs())
    {
        ConsolePrintf(PRIO_ERROR, RED"Failed to initialize Control CDEVs"RESETCOLOR"\r\n");
        return -1;
    }

    /* Initialize UNICENS */
    UCSI_Init(&m.unicens, &m);
    if (cfg)
    {
        if (!UCSI_NewConfig(&m.unicens, cfg->packetBw, cfg->pRoutes, cfg->routesSize, cfg->pNod, cfg->nodSize))
        {
            ConsolePrintf(PRIO_ERROR, RED"Could not enqueue new UNICENS config"RESETCOLOR"\r\n");
            assert(false);
            return -1;
        }
    }
    else
    {
        ConsolePrintf(PRIO_HIGH, YELLOW"No filename was provided, executing default configuration (default_config.c)"RESETCOLOR"\r\n");
        if (!UCSI_NewConfig(&m.unicens, PacketBandwidth, AllRoutes, RoutesSize, AllNodes, NodeSize))
        {
            ConsolePrintf(PRIO_ERROR, RED"Could not enqueue new UNICENS config"RESETCOLOR"\r\n");
            assert(false);
            return -1;
        }
    }
    m.allowRun = true;
    while (m.allowRun)
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
                if (UCSI_ProcessRxData(&m.unicens, pData, len))
                {
#ifdef LLD_TRACE
                    uint32_t i;
                    ConsolePrintfStart( PRIO_HIGH, YELLOW"%08d: MSG_RX: ", GetTicks());
                    for ( i = 0; i < len; i++ )
                    {
                        ConsolePrintfContinue( "%02X ", pData[i] );
                    }
                    ConsolePrintfExit(RESETCOLOR"\n");
#endif
                    /*Remove flag only in case of successful enqueuing*/
                    m.unicensDataAvailable = false;
                    Cdev_PopRx(&m.ctrlRx);
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
#ifdef LLD_TRACE
                ConsolePrintf(PRIO_HIGH, "Received AMS, id=0x%X, source=0x%X, len=%u\r\n", amsId, sourceAddress, len);
#endif
                UCSI_ReleaseAmsMessage(&m.unicens);
            }
            else assert(false);
        }
        SemWait();
    }
    UcsXml_FreeVal(cfg);
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  CALLBACK FUNCTION FROM XML PARSER                   */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

void UcsXml_CB_OnError(const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    va_start(argptr, vargsCnt);
    vsprintf(outbuf, format, argptr);
    va_end(argptr);
    ConsolePrintf(PRIO_ERROR, RED"XML-Parser error: '%s'"RESETCOLOR"\r\n", outbuf);
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

uint16_t UCSI_CB_OnGetTime(void *pTag)
{
    pTag = pTag;
    return GetTicks();
}

/* Callback from UNICENS Integration component */
void UCSI_CB_OnSetServiceTimer(void *pTag, uint16_t timeout)
{
    pTag = pTag;
    TimerSetTimeOut(timeout, m.ucsTimer);
}

void UCSI_CB_OnNetworkState(void *pTag, bool isAvailable, uint16_t packetBandwidth, uint8_t amountOfNodes)
{
    pTag = pTag;
    ConsolePrintf(PRIO_HIGH, YELLOW"Network isAvailable=%s, packetBW=%d, nodeCount=%d"RESETCOLOR"\r\n",
                  isAvailable ? "yes" : "no",
                  packetBandwidth,
                  amountOfNodes);
}

/* Callback from UNICENS Integration component */
void UCSI_CB_OnUserMessage(void *pTag, bool isError, const char format[], uint16_t vargsCnt, ...)
{
    va_list argptr;
    char outbuf[300];
    pTag = pTag;
    va_start(argptr, vargsCnt);
    vsprintf(outbuf, format, argptr);
    va_end(argptr);
    if (isError)
        ConsolePrintf(PRIO_ERROR, RED"%s"RESETCOLOR"\r\n", outbuf);
    else
        ConsolePrintf(PRIO_HIGH, "%s\r\n", outbuf);
}

/* Callback from UNICENS Integration component */
void UCSI_CB_OnServiceRequired(void *pTag)
{
    pTag = pTag;
    m.unicensTrigger = true;
    SemPost();
}

void UCSI_CB_OnTxRequest(void *pTag,
    const uint8_t *pPayload, uint32_t payloadLen)
{
    pTag = pTag;
#ifdef LLD_TRACE
    {
        uint32_t i;
        ConsolePrintfStart( PRIO_HIGH, BLUE"%08d: MSG_TX: ", GetTicks());
        for ( i = 0; i < payloadLen; i++ )
        {
            ConsolePrintfContinue( "%02X ", pPayload[i] );
        }
        ConsolePrintfExit(RESETCOLOR"\n");
    }
#endif
    if(Cdev_Write(&m.ctrlTx, pPayload, payloadLen))
    {
        if (m.txErrorState)
        {
            m.txErrorState = false;
            ConsolePrintf(PRIO_ERROR, GREEN"CDEV TX (%s) opened"RESETCOLOR"\r\n",
                    m.controlTxCdev);
        }
    }
    else if (!m.txErrorState)
    {
        m.txErrorState = true;
        ConsolePrintf(PRIO_ERROR, RED"CDEV TX error (%s), reason='%s'"RESETCOLOR"\r\n",
            m.controlTxCdev, GetErrnoString());
    }
}

void UCSI_CB_OnStop(void *pTag)
{
    pTag = pTag;
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
    ConsolePrintf(PRIO_HIGH, "Route id=0x%X isActive=%s ConLabel=0x%X\r\n", routeId,
        (isActive ? "true" : "false"), connectionLabel);
}

void UCSI_CB_OnGpioStateChange(void *pTag, uint16_t nodeAddress, uint8_t gpioPinId, bool isHighState)
{
    pTag = pTag;
    ConsolePrintf(PRIO_HIGH, "GPIO state changed, nodeAddress=0x%X, gpioPinId=%d, isHighState=%s\r\n",
                  nodeAddress, gpioPinId, isHighState ? "yes" : "no");
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static char *ReadFile(const char *fileName)
{
    char *buffer;
    int stringSize, readSize;
    FILE *fh = fopen(fileName, "r");
    if (!fh) return NULL;
    fseek(fh, 0, SEEK_END);
    stringSize = ftell(fh);
    rewind(fh);
    buffer = (char *)malloc(stringSize + 1);
    readSize = fread(buffer, sizeof(char), stringSize, fh);
    buffer[stringSize] = '\0'; /*In any case, terminate it.*/
    if (stringSize != readSize)
    {
        free(buffer);
        buffer = NULL;
    }
    fclose(fh);
    return buffer;
}

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
    ConsolePrintf(PRIO_HIGH, "RX-CDEV='%s', TX-CDEV='%s'\r\n", m.controlRxCdev, m.controlTxCdev);
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
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &currentTime))
    {
        assert(false);
        return 0;
    }
    return ( currentTime.tv_sec * 1000 ) + ( currentTime.tv_nsec / 1000000 );
}
