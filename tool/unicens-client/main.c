/*------------------------------------------------------------------------------------------------*/
/* UNICENS Client (unicensc) main-loop                                                            */
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
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "Console.h"
#include "UcsXml.h"
#include "mld-configurator-v1.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/* UNICENS daemon version number */
#define UNICENSC_VERSION    ("V5.1.0")

#ifdef NO_RAW_CLOCK
#define CLOCK_SRC CLOCK_MONOTONIC
#else
#define CLOCK_SRC CLOCK_MONOTONIC_RAW
#endif

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      DEFINES AND LOCAL VARIABLES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef struct
{
    bool allowRun;
    bool enableDrv1;
    uint16_t drvNodeAddr;
    char *drvFilter;
} LocalVar_t;

static LocalVar_t m;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     PRIVTATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void PrintHelp(void);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main(int argc, char *argv[])
{
    bool fileNameSet = false;
    int32_t i;
    UcsXmlVal_t *cfg = NULL;
    memset(&m, 0, sizeof(LocalVar_t));
    ConsolePrintf(PRIO_HIGH, YELLOW "------|UNICENS client %s (BUILD %s %s)|------" RESETCOLOR "\r\n", UNICENSC_VERSION, __DATE__, __TIME__);
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

            cfg = UcsXml_ParseFile(argv[i]);
            if (NULL == cfg)
            {
                ConsolePrintf(PRIO_ERROR, RED"Could not parse UNICENS XML"RESETCOLOR"\r\n");
                return -1;
            }
        }
        else if (0 == strcmp("--help", argv[i]))
        {
            PrintHelp();
            return 0;
        }
        else if (0 == strcmp("-drv1", argv[i]))
        {
            uint8_t j = 0;
            char *tkPtr;
            char *token;
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED"-drv1 parameter needs additional node address of the local network controller"RESETCOLOR"\r\n");
                return -1;
            }
            m.enableDrv1 = true;
            token = strtok_r( argv[i + 1], ":", &tkPtr );
            while( NULL != token )
            {
                if (0 == j)
                    m.drvNodeAddr = strtol( token, NULL, 0 );
                else if (1 == j)
                    m.drvFilter = token;
                token = strtok_r( NULL, ":", &tkPtr );
                ++j;
            }
            ++i;
        }
        else if (0 == strcmp("-drv2", argv[i]))
        {
            ConsolePrintf(PRIO_ERROR, RED"-drv2 is currently reserved"RESETCOLOR"\r\n");
        }
        else
        {
            ConsolePrintf(PRIO_ERROR, RED"Invalid command line parameter='%s'"RESETCOLOR"\r\n", argv[i]);
            return -1;
        }
    }
    if (!cfg)
    {
        ConsolePrintf(PRIO_ERROR, RED"No valid UNICENS configuration found"RESETCOLOR"\r\n");
        return -1;
    }
    if (!m.enableDrv1)
    {
        ConsolePrintf(PRIO_ERROR, RED"Driver configuration was not enabled (-drv1)"RESETCOLOR"\r\n");
        return -1;
    }
    if (NULL != cfg->ppDriver && 0 != cfg->driverSize)
    {
        if (!MldConfigV1_Start(cfg->ppDriver, cfg->driverSize, m.drvNodeAddr, m.drvFilter, 1000))
        {
            ConsolePrintf(PRIO_ERROR, RED"Could not start driver configuration service"RESETCOLOR"\r\n");
            return -1;
        }
    } else {
        ConsolePrintf(PRIO_ERROR, RED"MOST Linux Driver Configurator V1 is enabled, but the XML does not provide any information"RESETCOLOR"\r\n");
    }
    m.allowRun = true;
    while (m.allowRun)
    {
	sleep(1);
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
        ConsolePrintf(PRIO_ERROR, RED"Driver config error: %s"RESETCOLOR"\r\n", outbuf);
    else
        ConsolePrintf(PRIO_MEDIUM, "Driver config: %s\r\n", outbuf);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void PrintHelp(void)
{
    ConsolePrintfStart(PRIO_HIGH, "Usage: unicensc [OPTION]... [FILE]\r\n");
    ConsolePrintfContinue("Executes the UNICENS client configure the MOST Linux Driver according by the given XML file.\r\n\r\n");
    ConsolePrintfContinue("  [File]                   Path to UNICENS XML configuration file, if not set, the program will fail\r\n");
    ConsolePrintfContinue("  -drv1 [Node Addr:Filter] Configures the Microchip MOST Linux Driver V1.X with the XML file and the local node address\r\n");
    ConsolePrintfContinue("                           An additional filter string can be passed with a colon as delimiter. This filter applies to\r\n");
    ConsolePrintfContinue("                           description file inside the sys fs from the MOST Linux Driver.\r\n");
    ConsolePrintfContinue("  -drv2                    Configures the Microchip MOST Linux Driver V2.X (reserved)\r\n");
    ConsolePrintfContinue("  --help                   Shows this help and exit\r\n\r\n");
    ConsolePrintfContinue("Examples:\r\n");
    ConsolePrintfExit("  unicensd config.xml -drv1 0x200\r\n");
}

