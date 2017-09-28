/*------------------------------------------------------------------------------------------------*/
/* Unicens XML to C-Struct converter tool                                                         */
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
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "Console.h"
#include "UcsXml.h"
#include "Xml2Struct.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                     PRIVTATE FUNCTION PROTOTYPES                     */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

static char *ReadFile(const char *fileName);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                         PUBLIC FUNCTIONS                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

int main(int argc, char *argv[])
{
    char *xmlContent;
    UcsXmlVal_t *cfg;
    if (2 != argc)
    {
        ConsolePrintf(PRIO_ERROR, RED"Can not start, please provide path to XML"RESETCOLOR"\r\n");
        return -1;
    }
    xmlContent = ReadFile(argv[1]);
    if (NULL == xmlContent)
    {
        ConsolePrintf(PRIO_ERROR, RED"File could not be opened: '%s'"RESETCOLOR"\r\n", argv[1]);
        return -1;
    }
    cfg = UcsXml_Parse(xmlContent);
    free(xmlContent);
    if (NULL == cfg)
    {
        ConsolePrintf(PRIO_ERROR, RED"Could not parse Unicens XML"RESETCOLOR"\r\n");
        return -1;
    }
    PrintUcsStructures(cfg->packetBw, cfg->pRoutes, cfg->routesSize, cfg->pNod, cfg->nodSize);
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
    ConsolePrintf(PRIO_ERROR, RED"XML-Parser error: '%s'"RESETCOLOR"\r\n", outbuf);
    exit(-1);
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

