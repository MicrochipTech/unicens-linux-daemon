/*------------------------------------------------------------------------------------------------*/
/* Console Print Component                                                                        */
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
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Console.h"

/*! \cond PRIVATE */
typedef struct
{
    /**Minimum priority to be printed. If lower, message will be discarded.*/
    ConsolePrio_t minPrio;
    /**If is in a critical segmented print, this variable will hold the prio for Start, Continue, Exit.*/
    ConsolePrio_t contPrio;
} LocalData_t;
/*! \endcond */

static LocalData_t data = { 0 };

void ConsoleSetPrio( ConsolePrio_t prio )
{
    data.minPrio = prio;
}

void ConsolePrintf( ConsolePrio_t prio, const char *statement, ... )
{
    va_list args;
    if( prio < data.minPrio || NULL == statement )
        return;
    va_start( args, statement );
    vfprintf( (PRIO_ERROR == prio ? stderr : stdout), statement, args );
    va_end( args );
}

void ConsolePrintfStart( ConsolePrio_t prio, const char *statement, ... )
{
    va_list args;
    data.contPrio = prio;
    if( data.contPrio < data.minPrio || NULL == statement )
        return;
    va_start( args, statement );
    vfprintf( (PRIO_ERROR == prio ? stderr : stdout), statement, args );
    va_end( args );
}

void ConsolePrintfContinue( const char *statement, ... )
{
    va_list args;
    if( data.contPrio < data.minPrio || NULL == statement )
        return;
    va_start( args, statement );
    vfprintf( (PRIO_ERROR == data.contPrio ? stderr : stdout), statement, args );
    va_end( args );
}

void ConsolePrintfExit( const char *statement, ... )
{
    va_list args;
    if( data.contPrio < data.minPrio || NULL == statement )
        return;
    va_start( args, statement );
    vfprintf( (PRIO_ERROR == data.contPrio ? stderr : stdout), statement, args );
    va_end( args );
}

static const char* ExtractFileName(const char* filePath)
{
	/* We could also use strrchr, but this works without a library function */
	int fileNameStartPos=0;
	int i;
	for (i=0; filePath[i] != 0; i++)
	{
		if ((filePath[i] == '\\') || (filePath[i] == '/'))
		{
			fileNameStartPos = i+1;
		}
	}
	return filePath + fileNameStartPos;
}

void ConsolePrintfError(const char* filePath, const int lineNumber, const int columnNumber, const char* severity, const char* statement, ... )
{
    va_list args;
	char buffer[1000];
    va_start(args, statement);
	sprintf(buffer, "%s:%d:%d: %s - %s", ExtractFileName(filePath), lineNumber, columnNumber, severity, statement);
    vfprintf(stderr, buffer, args);
    va_end(args);
}