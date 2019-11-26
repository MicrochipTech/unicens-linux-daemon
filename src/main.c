/*------------------------------------------------------------------------------------------------*/
/* UNICENS Daemon (unicensd) main-loop                                                            */
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "Console.h"
#include "task-unicens.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          USER ADJUSTABLE                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/* UNICENS daemon version number */
#define UNICENSD_VERSION    ("V5.1.1")

/* Character device to INIC control channel */
#define DEFAULT_CONTROL_CDEV_TX ("/dev/inic-control-tx")
#define DEFAULT_CONTROL_CDEV_RX ("/dev/inic-control-rx")

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                      PRIVATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static bool ParseCommandLine(int argc, char *argv[], TaskUnicens_t *pVar);
static void PrintHelp(void);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main(int argc, char *argv[])
{
    static TaskUnicens_t taskVars;
    ConsoleSetPrio(PRIO_HIGH);
    ConsolePrintf(PRIO_HIGH, YELLOW "------|UNICENS daemon %s (BUILD %s %s)|------" RESETCOLOR "\r\n", UNICENSD_VERSION, __DATE__, __TIME__);
    if (!ParseCommandLine(argc, argv, &taskVars))
    {
        ConsolePrintf(PRIO_ERROR, RED "Parsing command line failed" RESETCOLOR "\r\n");
        return -1;
    }
    if (!TaskUnicens_Init(&taskVars))
    {
        ConsolePrintf(PRIO_ERROR, RED "Initialization of UNICENS task failed" RESETCOLOR "\r\n");
        return -1;
    }
    while(true)
    {
        /* TaskUnicens_Service may block very long. 
         * You may call it in an own thread.
         */
        TaskUnicens_Service();
    }
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static bool ParseCommandLine(int argc, char *argv[], TaskUnicens_t *pVar)
{
    bool defaultSet = false;
    int32_t i;
    if (argc < 1 || NULL == argv || NULL == pVar)
        return false;
    memset(pVar, 0, sizeof(TaskUnicens_t));
    for (i = 1; i < argc; i++)
    {
        if ('-' != argv[i][0])
        {
            if (pVar->cfgFileName)
            {
                ConsolePrintf(PRIO_ERROR, RED "Filename is already set. Wrong parameter='%s'" RESETCOLOR "\r\n", argv[i]);
                return false;
            }
            pVar->cfgFileName = argv[i];
        }
        else if (0 == strcmp("-v", argv[i]))
        {
            ConsoleSetPrio(PRIO_MEDIUM);
        }
        else if (0 == strcmp("-vv", argv[i]))
        {
            ConsoleSetPrio(PRIO_LOW);
        }
        else if (0 == strcmp("-hide", argv[i]))
        {
            pVar->noRouteTable = true;
        }
        else if (0 == strcmp("--help", argv[i]))
        {
            PrintHelp();
            exit(0);
        }
        else if (0 == strcmp("-default", argv[i]))
        {
            defaultSet = true;
        }
        else if (0 == strcmp("-drv1", argv[i]) || 0 == strcmp("-drv2", argv[i]))
        {
            uint8_t j = 0;
            char *tkPtr;
            char *token;
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED "-drv1 or -drv2 parameter need additional node address of the local network controller" RESETCOLOR "\r\n");
                return false;
            }
            pVar->drvVersion = (0 == strcmp("-drv1", argv[i])) ? 1 : 2;
            token = strtok_r( argv[i + 1], ":", &tkPtr );
            while( NULL != token )
            {
                if (0 == j)
                    pVar->drvLocalNodeAddr = strtol( token, NULL, 0 );
                else if (1 == j)
                    pVar->drvFilter = token;
                token = strtok_r( NULL, ":", &tkPtr );
                ++j;
            }
            ++i;
        }
        else if (0 == strcmp("-lld", argv[i]))
        {
            pVar->lldTrace = true;
        }
        else if (0 == strcmp("-promisc", argv[i]))
        {
            ConsolePrintf(PRIO_ERROR, YELLOW "Promiscuous Mode active for all nodes" RESETCOLOR "\r\n");
            pVar->promiscuousMode = true;
        }
        else if (0 == strcmp("-local", argv[i]))
        {
            ConsolePrintf(PRIO_ERROR, YELLOW "Messages to local attached INIC will be copied to debug node address" RESETCOLOR "\r\n");
            pVar->debugLocalMsg = true;
        }
        else if (0 == strcmp("-crx", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED "-crx parameter needs additional path to RX character device" RESETCOLOR "\r\n");
                return false;
            }
            pVar->controlRxCdev = argv[i + 1];
            ++i;
        }
        else if (0 == strcmp("-ctx", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED "-ctx parameter needs additional path to TX character device" RESETCOLOR "\r\n");
                return false;
            }
            pVar->controlTxCdev = argv[i + 1];
            ++i;
        }
        else if (0 == strcmp("-program", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintf(PRIO_ERROR, RED "-program parameter needs additional amount of expected node count" RESETCOLOR "\r\n");
                return -1;
            }
            pVar->programNodeCnt = strtol( argv[i + 1], NULL, 0 );
            ++i;
            ConsolePrintf(PRIO_HIGH, YELLOW "Programming is enabled. Target node count is=%d" RESETCOLOR "\r\n", pVar->programNodeCnt);
        }
        else if (0 == strcmp("--persistent", argv[i]))
        {
            ConsolePrintf(PRIO_ERROR, YELLOW "Persistent programming mode chosen" RESETCOLOR "\r\n");
            pVar->programPersistent = true;
        }
        else
        {
            ConsolePrintf(PRIO_ERROR, RED "Invalid command line parameter='%s'" RESETCOLOR "\r\n", argv[i]);
            return false;
        }
    }
    if (!pVar->cfgFileName && !defaultSet)
        ConsolePrintf(PRIO_HIGH, YELLOW "No filename was provided, executing default configuration (default_config.c).\r\nUse \"--help\" for details. Use \"-default\" to suppress this waring." RESETCOLOR "\r\n");
    if (!pVar->cfgFileName && 0 != pVar->drvLocalNodeAddr)
    {
        ConsolePrintf(PRIO_ERROR, RED "-drv1 and -drv2 option only allowed, when specified an path to UNICENS XML file" RESETCOLOR "\r\n");
        return false;
    }
    if (0 == pVar->drvLocalNodeAddr && (NULL == pVar->controlRxCdev || NULL == pVar->controlTxCdev))
    {
        pVar->controlRxCdev = DEFAULT_CONTROL_CDEV_RX;
        pVar->controlTxCdev = DEFAULT_CONTROL_CDEV_TX;
    }
    return true;
}

static void PrintHelp(void)
{
    ConsolePrintfStart(PRIO_HIGH, "Usage: unicensd [OPTION]... [FILE]\r\n");
    ConsolePrintfContinue("Executes the UNICENS daemon to start and configure INICnet devices.\r\n\r\n");
    ConsolePrintfContinue("  [File]                   Path to UNICENS XML configuration file, if not set, the compiled default config will be used\r\n");
    ConsolePrintfContinue("  -v                       Verbose mode, prints debug informations\r\n");
    ConsolePrintfContinue("  -vv                      Very Verbose mode, prints even more debug informations\r\n");
    ConsolePrintfContinue("  -hide                    Disable node and route table printing\r\n");
    ConsolePrintfContinue("  -default                 Uses default configuration (default_config.c) instead of parsing XML file\r\n");
    ConsolePrintfContinue("  -crx [RX char device]    Path to the receiver character device\r\n");
    ConsolePrintfContinue("  -ctx [TX char device]    Path to the sender character device\r\n");
    ConsolePrintfContinue("  -drv1 [Node Addr:Filter] Configures the Microchip MOST Linux Driver V1.X with the XML file and the local node address\r\n");
    ConsolePrintfContinue("                           An additional filter string can be passed with a colon as delimiter. This filter applies to\r\n");
    ConsolePrintfContinue("                           description file inside the sys fs from the MOST Linux Driver.\r\n");
    ConsolePrintfContinue("  -drv2                    Configures the Microchip MOST Linux Driver V2.X (reserved)\r\n");
    ConsolePrintfContinue("  -promisc                 Enable promiscuous mode on all INICs.\r\n" \
                          "                           Promiscuous mode disables packet filter in all INICS, so all Ethernet packets will be received by all nodes.\r\n");
    ConsolePrintfContinue("  -local                   Special mode for INICnet sniffer. Messages sent to local attached INIC will be duplicated sent to debug node address.\r\n");
    ConsolePrintfContinue("  -lld                     Prints out the byte arrays send and received via Low Level Driver\r\n");
    ConsolePrintfContinue("  -program [Node Count]    Enables automatic reprogramming mode. If there is a node address collision,\r\n");
    ConsolePrintfContinue("                           the conflicting devices will get the next free node address assigned. By default the changes are\r\n");
    ConsolePrintfContinue("                           written to RAM only (not persistent). The programming starts when the number of devices found reaches\r\n");
    ConsolePrintfContinue("                           the given [Node Count] value.\r\n");
    ConsolePrintfContinue("  --persistent             Only valid along with -program parameter. If set, the changes are written into persistent memory (Flash or OTP)\r\n");
    ConsolePrintfContinue("                           !!WARNING: Use this parameter with care. On OS8121/0/2/4/6 you can only write changes two times!!\r\n");
    ConsolePrintfContinue("  --help                   Shows this help and exit\r\n\r\n");
    ConsolePrintfContinue("Examples:\r\n");
    ConsolePrintfExit("  unicensd -default\r\n");
    ConsolePrintfExit("  unicensd config.xml\r\n");
    ConsolePrintfExit("  unicensd config.xml -drv1 0x200\r\n");
    ConsolePrintfExit("  unicensd config.xml -drv1 0x200:1-1.3:1\r\n");
    ConsolePrintfExit("  unicensd -ctx /dev/inic-control-tx -crx /dev/inic-control-rx\r\n");
}
