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
#ifndef CDEVHANDLER_H
#define CDEVHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_FILENAME_LEN (100)
#define RX_BUFFER (64)

/** Internal structure, enabling multiple instances of this component.
 * \note Do not access any of this variables.
 *  */
typedef struct
{
    bool allowThreadRun;
    bool rxThreadRuns;
    int fileHandle;
    int fileFlags;
    char fileName[MAX_FILENAME_LEN];
    uint8_t rxBuffer[RX_BUFFER];
    uint32_t rxLen;
    pthread_t rxThread;
    sem_t rxSem;
} CdevData_t;

/**
 * \brief Initialize this component.
 * \note Do not call any function of this component, before calling this function.
 * \param d - Pointer to external allocated memory holding the structure needed by this component.
 * \param fileName - Full path to the CDEV (e.g. "/dev/inic-usb-ctx").
 * \param read - true, if CDEV supports read access.
 * \param write - true, if CDEV supports write access.
 * \note Microchips MOST Linux driver forms for RX and TX different CDEVs.
 *       But anyway, this component can support read and write on a single CDEV.
 * \return true, if successful. false, otherwise, do not call any other function in this case.
 */
bool Cdev_Init(CdevData_t *d, const char *fileName, bool read, bool write);

/**
 * \brief Starts the background reader thread.
 * \note This function will fail, if read was disabled in Cdev_Init.
 * \param d - Pointer to external allocated memory holding the structure needed by this component.
 * \return true, if successful. false, otherwise.
 */
bool Cdev_StartReading(CdevData_t *d);

/**
 * \brief Writes to the CDEV.
 * \note This function will fail, if write was disabled in Cdev_Init.
 * \param d - Pointer to external allocated memory holding the structure needed by this component.
 * \param pData - Pointer to the payload to be sent.
 * \param len - Length in bytes of the payload.
 * \return true, if successful. false, otherwise.
 */
bool Cdev_Write(CdevData_t *d, const uint8_t *pData, uint32_t len);

/**
 * \brief Gets data from the receiving thread. The content will stay valid until
 *        Cdev_PopRx is called.
 * \note You are only allowed to call this function, when the callback
 *       Cdev_CB_OnDataAvailable was raised.
 * \note This function will fail, if read was disabled in Cdev_Init.
 * \param d - Pointer to external allocated memory holding the structure needed by this component.
 * \param pData - To this Pointer the pointer of RX payload will be written.
 * \param len - To this pointer the length in bytes of RX payload will be written..
 * \return true, if successful. false, otherwise.
 */
bool Cdev_GetRx(CdevData_t *d, uint8_t **pData, uint32_t *len);

/**
 * \brief Releases the data provided by Cdev_GetRx.
 * \note You are only allowed to call this function, when the callback
 *       Cdev_CB_OnDataAvailable was raised.
 * \note This function will fail, if read was disabled in Cdev_Init.
 * \param d - Pointer to external allocated memory holding the structure needed by this component.
 * \return true, if successful. false, otherwise.
 */
bool Cdev_PopRx(CdevData_t *d);

/**
 * \brief Gets a human readable zero terminated string explaining the last CDEV error.
 * \return Zero terminated string.
 */
const char *GetErrnoString();

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        CALLBACK SECTION                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Callback when ever the RX CDEV delivered data.
 * \note This function must be implemented by the integrator.
 * \note Do not call any functions of this component inside this callback.
 */
extern void Cdev_CB_OnDataAvailable(void);

#ifdef __cplusplus
}
#endif

#endif /* CDEVHANDLER_H */
