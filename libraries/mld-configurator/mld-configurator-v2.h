/*------------------------------------------------------------------------------------------------*/
/* MOST Linux Driver Configurator for MLD Driver V2.x                                             */
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
#ifndef MLD_CONFIGURATOR_V2_H
#define MLD_CONFIGURATOR_V2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <UcsXmlDriverConfig.h>

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                            Public API                                */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Starts the background service to configure the MOST Linux Driver
 *
 * \note In case of errors the callback MldConfigV2_CB_OnError will be raised
 * \param pConfig - Array of structures holding driver information. To make live easier, this structure is taken from the XML parser component
 * \param driverSize - Array length
 * \param localNodeAddress - Specify what node address the local node has. Then only informations related to that address will be used
 * \param descriptionFilter - If set, the device must contain this string in its SYSFS description to be accepted. Passing NULL poiner, will disable filter, any file will be accepted
 * \param pollTime - Service sleep time interval in milliseconds.
 * \return true if successfully started the service, false otherwise
 */
bool MldConfigV2_Start(DriverInformation_t **pConfig, uint16_t driverSize, uint16_t localNodeAddress, const char *descriptionFilter, uint16_t pollTime);

/**
 * \brief Stops the background service
 *
 * \note This function will block until the background thread has been terminated
 */
void MldConfigV2_Stop();

/**
 * \brief Get the full path of control character devices
 * \note Buffers for pControlCdevTx and pControlCdevRx must be allocated by the integrator.
 *
 * \param pControlCdevTx - Zero terminated string containing path to control TX CDEV or NULL if not found
 * \param pControlCdevRx - Zero terminated string containing path to control RX CDEV or NULL if not found
 * \return true if both devices are available. false, otherwise, then pControlCdevTx and pControlCdevRx are invalid.
 */
bool MldConfigV2_GetControlCdevName(char *pControlCdevTx, char *pControlCdevRx);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        CALLBACK SECTION                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Callback whenever a human readable message was formed.
 * \note This function must be implemented by the integrator.
 *
 * \param isError - true if error, false is only an information for debugging purpose
 * \param format - Zero terminated format string (following printf rules)
 * \param vargsCnt - Amount of parameters stored in "..."
 */
extern void MldConfigV2_CB_OnMessage(bool isError, const char format[], uint16_t vargsCnt, ...);
    
#ifdef __cplusplus
}
#endif
    
#endif /* MLD_CONFIGURATOR_V2_H */