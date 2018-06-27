/*------------------------------------------------------------------------------------------------*/
/* UNICENS XML to C-Struct converter tool                                                         */
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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "Console.h"
#include "UcsXml.h"
#include "Xml2Struct.h"
#include "Xml2Driver.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     USED ADJUSTABLE DEFINES                          */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#define NODE_START  "<<<<< FILE START 0x%03x >>>>>\n"
#define NODE_END    "<<<<< FILE END >>>>>\n"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        PRIVATE DEFINES                               */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

typedef enum
{
    JOB_PRINT_UCS_STRUCTURE,
    JOB_PRINT_UCS_HEADER,
    JOB_PRINT_SINGLE_DRIVER,
    JOB_PRINT_ALL_DRIVERS
} Job_t;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     PRIVTATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void PrintHelp(void);
static char *ReadFileToString(const char *fileName);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          PRIVTATE Variables                          */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static const char* globalFileName = NULL;
static char *variablePrefix = NULL;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main(int argc, char *argv[])
{
    bool found;
    Job_t job = JOB_PRINT_UCS_STRUCTURE;
    const char *fileName = NULL;
    uint16_t printNodeAddress = 0;
    uint16_t i;
    char *xmlContent;
    UcsXmlVal_t *cfg;
    if (1 == argc)
    {
        PrintHelp();
        return 0;
    }
    for (i = 1; i < argc; i++)
    {
        if ('-' != argv[i][0])
        {
            if (fileName)
            {
                ConsolePrintfError(fileName, 0 ,0, "Error", "Filename is already set. Wrong parameter='%s'\r\n", argv[i]);
                return -1;
            }
            fileName = argv[i];
        }
        else if (0 == strcmp("-ucs", argv[i]))
        {
            job = JOB_PRINT_UCS_STRUCTURE;
        }
        else if (0 == strcmp("-header", argv[i]))
        {
            job = JOB_PRINT_UCS_HEADER;
        }
        else if (0 == strcmp("-prefix", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintfError(fileName ? fileName : "NoFile", 0 ,0, "Error", "-prefix parameter needs additional name to be concated at the begin of each variable\r\n");
                return -1;
            }
            variablePrefix = argv[i + 1];
            ++i;
        }
        else if (0 == strcmp("-drv", argv[i]))
        {
            if (argc <= (i+1))
            {
                ConsolePrintfError(fileName ? fileName : "NoFile", 0 ,0, "Error", "-drv parameter needs additional node address of the device to be configured\r\n");
                return -1;
            }
            job = JOB_PRINT_SINGLE_DRIVER;
            printNodeAddress = strtol( argv[i + 1], NULL, 16 );
            ++i;
        }
        else if (0 == strcmp("-all", argv[i]))
        {
            job = JOB_PRINT_ALL_DRIVERS;
        }
        else if (0 == strcmp("--version", argv[i]))
        {
            ConsolePrintf(PRIO_HIGH, "%s\r\n", GetXml2StructVersion());
            return 0;
        }
        else if (0 == strcmp("--help", argv[i]))
        {
            PrintHelp();
            return 0;
        }
        else
        {
            ConsolePrintfError(fileName ? fileName : "NoFile", 0 ,0, "Error", "Unknown parameter '%s'\r\n", argv[i]);
            return -1;
        }
    }
    if (JOB_PRINT_UCS_HEADER == job)
    {
        /* Special handling for header file. It does not need a XML file as input */
        PrintHeaderFile(variablePrefix);
        return 0;
    }
    if (!fileName)
    {
        ConsolePrintfError("NoFile", 0 ,0, "Error", "Can not start, please provide path to XML\r\n");
        return -1;
    }
    globalFileName = fileName;
    xmlContent = ReadFileToString(fileName);
    if (NULL == xmlContent)
    {
        ConsolePrintfError(fileName, 0 ,0, "Error", "File could not be opened: '%s'\r\n", argv[1]);
        return -1;
    }
    cfg = UcsXml_Parse(xmlContent);
    free(xmlContent);
    if (NULL == cfg)
    {
        ConsolePrintfError(fileName, 0 ,0, "Error", "Could not parse UNICENS XML\r\n");
        return -1;
    }
    switch(job)
    {
    case JOB_PRINT_UCS_STRUCTURE:
        PrintUcsStructures(cfg->packetBw, cfg->pRoutes, cfg->routesSize, cfg->pNod, cfg->nodSize, variablePrefix);
        break;
    case JOB_PRINT_SINGLE_DRIVER:
        found = false;
        for(i = 0; !found && i < cfg->nodSize; i++)
        {
            if (cfg->pNod[i].signature_ptr->node_address == printNodeAddress)
                found = true;
        }
        if (found)
            PrintUcsDriver(printNodeAddress, cfg->ppDriver, cfg->driverSize);
        else
            ConsolePrintfError(fileName, 0 ,0, "Error", "Node address 0x%X is not defined in XML\r\n", printNodeAddress);
        break;
    case JOB_PRINT_ALL_DRIVERS:
        for(i = 0; i < cfg->nodSize; i++)
        {
            ConsolePrintf(PRIO_HIGH, NODE_START, cfg->pNod[i].signature_ptr->node_address);
            PrintUcsDriver(cfg->pNod[i].signature_ptr->node_address, cfg->ppDriver, cfg->driverSize);
            ConsolePrintf(PRIO_HIGH, NODE_END);
        }
        break;
    default:
        assert(false);
    }
    UcsXml_FreeVal(cfg);
    usleep(500); /*Give printf time to do its job*/
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
    ConsolePrintfError(globalFileName, 0 ,0, "Error", "XML-Parser error: '%s'\r\n", outbuf);
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  PRIVATE FUNCTION IMPLEMENTATIONS                    */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static void PrintHelp(void)
{
    ConsolePrintfStart(PRIO_HIGH, "Usage: xml2struct [OPTION]... [FILE]\r\n");
    ConsolePrintfContinue("Translate a UNICENS XML file into structures for the UNICENS library or structures for the MOST Linux Driver.\r\n\r\n");
    ConsolePrintfContinue("  -ucs                     Print UNICENS C structures\r\n");
    ConsolePrintfContinue("  -header                  Print UNICENS H file\r\n");
    ConsolePrintfContinue("  -prefix [Name]           Adds the given name before any variable or structure (-ucs and -header mode only)\r\n");
    ConsolePrintfContinue("  -drv [Address]           Print MOST Linux Driver structure for the given node address (value will be interpreted as hex)\r\n");
    ConsolePrintfContinue("  -all                     Print all possible MOST Linux Driver structures for all nodes.\r\n");
    ConsolePrintfContinue("                           There is a file seperator inserted after each configuration\r\n");
    ConsolePrintfContinue("  --version                Prints the version string of this program and exit\r\n\r\n");
    ConsolePrintfContinue("  --help                   Prints this help and exit\r\n\r\n");
    ConsolePrintfContinue("With no OPTION, UNICENS C structures are printed\r\n\r\n");
    ConsolePrintfContinue("Examples:\r\n");
    ConsolePrintfContinue("  xml2struct -drv 200 config.xml\r\n");
    ConsolePrintfContinue("  xml2struct -all config.xml\r\n");
    ConsolePrintfExit("  xml2stuct config.xml\r\n");
}

static char *ReadFileToString(const char *fileName)
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

