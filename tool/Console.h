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

/*----------------------------------------------------------*/
/*! \file
 *  \brief This file contains C-functions starting with "Console" to provide
 *         process and thread safe access to the console output.
 */
/*----------------------------------------------------------*/
#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define ENABLE_COLOR

#ifdef ENABLE_COLOR
#define RESETCOLOR "\033[0m"
#define GREEN      "\033[0;32m"
#define RED        "\033[0;31m"
#define YELLOW     "\033[1;33m"
#define BLUE       "\033[0;34m"
#else
#define RESETCOLOR
#define GREEN
#define RED
#define YELLOW
#define BLUE
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        PRIO_LOW = 0,
        PRIO_MEDIUM = 1,
        PRIO_HIGH = 2,
        PRIO_ERROR = 0xFF
    } ConsolePrio_t;


    /*----------------------------------------------------------*/
    /*! \brief Sets the minimum priority to be displayed. Lower priority messages are discarded
     *  \param prio - The minimum priority to display
     */
    /*----------------------------------------------------------*/
    void ConsoleSetPrio( ConsolePrio_t prio );

    /*----------------------------------------------------------*/
    /*! \brief Uses the board specific PRINT mechanism and provides thread and process safety.
     *
     */
    /*----------------------------------------------------------*/
    void ConsolePrintf( ConsolePrio_t prio, const char *statement, ... ) __attribute__ ((format (gnu_printf, 2, 3)));


    /*----------------------------------------------------------*/
    /*! \brief Starts to print and stay blocked after exit of this function
     *
     */
    /*----------------------------------------------------------*/
    void ConsolePrintfStart( ConsolePrio_t prio, const char *statement, ... ) __attribute__ ((format (gnu_printf, 2, 3)));

    /*----------------------------------------------------------*/
    /*! \brief Continue to print and stay blocked after exit of this function
     *  \note ConsolePrintfStart must be called before and when finished ConsolePrintfExit must be called.
     *  \note This function may be called multiple times.
     */
    /*----------------------------------------------------------*/
    void ConsolePrintfContinue( const char *statement, ... ) __attribute__ ((format (gnu_printf, 1, 2)));

    /*----------------------------------------------------------*/
    /*! \brief Continue to print and unblock after finishing.
     *  \note ConsolePrintfStart must be called before. ConsolePrintfContinue may have been called before multiple times.
     */
    /*----------------------------------------------------------*/
    void ConsolePrintfExit( const char *statement, ... ) __attribute__ ((format (gnu_printf, 1, 2)));
    

#ifdef __cplusplus
}
#endif

#endif /*_CONSOLE_H_*/
