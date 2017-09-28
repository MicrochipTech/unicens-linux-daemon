/*------------------------------------------------------------------------------------------------*/
/* Character Device Handler Component                                                             */
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
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "CdevHandler.h"

static void *ReceiveThread(void *tag);

bool Cdev_Init(CdevData_t *d, const char *fileName, bool read, bool write)
{
    if (NULL == d || NULL == fileName)
        return false;
    memset(d, 0, sizeof(CdevData_t));
    strncpy(d->fileName, fileName, MAX_FILENAME_LEN);
    d->fileHandle = -1;
    d->allowThreadRun = true;
    if (read && write)
        d->fileFlags = O_RDWR;
    else if (read)
        d->fileFlags = O_RDONLY;
    else if (write)
        d->fileFlags = O_WRONLY;
    else
        return false;
    return true;
}

bool Cdev_StartReading(CdevData_t *d)
{
    if (NULL == d) return false;
    if (O_WRONLY == d->fileFlags) return false;
    if (d->rxThreadRuns) return false;
    if (-1 == (sem_init(&d->rxSem, 0, 0))) return false;
    return (0 == pthread_create(&d->rxThread, NULL, ReceiveThread, d));
}

bool Cdev_Write(CdevData_t *d, const uint8_t *pData, uint32_t len)
{
    uint32_t total = 0;
    if (NULL == d || NULL == pData || 0 == len) return false;
    if (O_RDONLY == d->fileFlags) return false;
    if (-1 == d->fileHandle)
        d->fileHandle = open(d->fileName, d->fileFlags);
    if (-1 == d->fileHandle)
        return false;
    while(total < len)
    {
        ssize_t written = write(d->fileHandle, &pData[total], (len - total));
        if (0 >= written)
        {
            d->fileHandle = -1;
            return false;
        }
        total += written;
    }
    return true;
}

bool Cdev_GetRx(CdevData_t *d, uint8_t **pData, uint32_t *len)
{
    if (NULL == d || NULL == pData || NULL == len) return false;
    if (O_WRONLY == d->fileFlags) return false;
    if (0 == len) return false;
    *pData = d->rxBuffer;
    *len = d->rxLen;
    return true;
}

bool Cdev_PopRx(CdevData_t *d)
{
    if (NULL == d) return false;
    if (O_WRONLY == d->fileFlags) return false;
    sem_post(&d->rxSem);
    return true;
}

const char *GetErrnoString()
{
    switch( errno )
    {
    case 0:
        return "Nothing stored in errno";
    case 1:
        return "Operation not permitted";
    case 2:
        return "No such file or directory";
    case 3:
        return "No such process";
    case 4:
        return "Interrupted system call";
    case 5:
        return "I/O error";
    case 6:
        return "No such device or address";
    case 7:
        return "Argument list too long";
    case 8:
        return "Exec format error";
    case 9:
        return "Bad file number";
    case 10:
        return "No child processes";
    case 11:
        return "Try again";
    case 12:
        return "Out of memory";
    case 13:
        return "Permission denied";
    case 14:
        return "Bad address";
    case 15:
        return "Block device required";
    case 16:
        return "Device or resource busy";
    case 17:
        return "File exists";
    case 18:
        return "Cross-device link";
    case 19:
        return "No such device";
    case 20:
        return "Not a directory";
    case 21:
        return "Is a directory";
    case 22:
        return "Invalid argument";
    case 23:
        return "File table overflow";
    case 24:
        return "Too many open files";
    case 25:
        return "Not a typewriter";
    case 26:
        return "Text file busy";
    case 27:
        return "File too large";
    case 28:
        return "No space left on device";
    case 29:
        return "Illegal seek";
    case 30:
        return "Read-only file system";
    case 31:
        return "Too many links";
    case 32:
        return "Broken pipe";
    case 33:
        return "Math argument out of domain of func";
    case 34:
        return "Math result not representable";
    default:
        break;
    }
    return "Unknown";
}

static void *ReceiveThread(void *tag)
{
    CdevData_t *d = tag;
    assert(NULL != d);
    d->rxThreadRuns = true;
    while(d->allowThreadRun)
    {
        ssize_t rx;
        if (-1 == d->fileHandle)
            d->fileHandle = open(d->fileName, d->fileFlags);
        if (-1 == d->fileHandle)
        {
            sleep(1);
            continue;
        }
        d->rxLen = 0;
        rx = read(d->fileHandle, d->rxBuffer, sizeof(d->rxBuffer));
        if (0 >= rx)
        {
            d->fileHandle = -1;
            continue;
        }
        d->rxLen = rx;
        Cdev_CB_OnDataAvailable();
        sem_wait(&d->rxSem);
    }
    d->rxThreadRuns = false;
    return tag;
}