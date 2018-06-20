/*------------------------------------------------------------------------------------------------*/
/* UNICENS Integration Helper Component                                                           */
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
#ifndef UCSI_H_
#define UCSI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ucsi_cfg.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                            Public API                                */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Initializes UNICENS Integration module.
 * \note Must be called before any other function of this component
 *
 * \param pPriv - External allocated memory area for this particular
 *                instance (static allocated or allocated with malloc)
 * \param pTag - Pointer given by the integrator. This pointer will be
 *               returned by any callback function of this component
 */
void UCSI_Init(UCSI_Data_t *pPriv, void *pTag);

/**
 * \brief Executes the given configuration. If already started, all
 *        existing local and remote INIC resources will be destroyed
 * \note All given pointers must stay valid until this callback is
 *       raised: "UCSI_CB_OnStop"
 *
 * \param pPriv - private data section of this instance
 * \param packetBw - The amount of bytes per frame, reserved for Ethernet channel.
 * \param pRoutesList - Reference to a list of routes
 * \param routesListSize - Number of routes in the list
 * \param pNodesList - Reference to the list of nodes
 * \param nodesListSize - Reference to a list of routes
 * \return true, configuration successfully enqueued, false otherwise
 */
bool UCSI_NewConfig(UCSI_Data_t *pPriv,
    uint16_t packetBw, Ucs_Rm_Route_t *pRoutesList, uint16_t routesListSize,
    Ucs_Rm_Node_t *pNodesList, uint16_t nodesListSize);


/**
 * \brief Executes the given script. If already started, all
 *        existing local and remote INIC resources will be destroyed
 * \note pScriptList pointer must stay valid until this callback is
 *       raised: "UCSI_CB_OnStop"
 * \note UCSI_NewConfig must called first, before calling the function
 *
 * \param pPriv - private data section of this instance
 * \param targetAddress - targetAddress - The target node address
 * \param pScriptList - Pointer to the array of scripts
 * \param scriptListLength - Number of scripts in the array
 * \return true, script successfully enqueued, false otherwise
 */
bool UCSI_ExecuteScript(UCSI_Data_t *pPriv, uint16_t targetAddress, Ucs_Ns_Script_t *pScriptList, uint8_t scriptListLength);

/**
 * \brief Offer the received control data from LLD to UNICENS
 * \note Call this function only from single context (not from ISR)
 * \note This function can be called repeated until it return false
 *
 * \param pPriv - private data section of this instance
 * \param pBuffer - Received bytes from MOST control channel
 * \param len - Length of the received data array
 * \return true, if the data could be enqueued for processing, remove
 *         the data from LLD queue in this case.
 *         false, data could not be processed due to lag of resources.
 *         In this case do not discard the data. Offer the same
 *         data again after UCSI_CB_OnServiceRequired was
 *         raised or any time later.
 */
bool UCSI_ProcessRxData(UCSI_Data_t *pPriv,
    const uint8_t *pBuffer, uint32_t len);

/**
 * \brief Gives UNICENS Integration module time to do its job
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 */
void UCSI_Service(UCSI_Data_t *pPriv);


/**
 * \brief Call after timer set by UCSI_CB_OnSetServiceTimer
 *        expired.
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 */
void UCSI_Timeout(UCSI_Data_t *pPriv);

/**
 * \brief Sends an AMS message to the control channel
 *
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 * \param msgId - The AMS message id
 * \param targetAddress - The node / group target address
 * \param pPayload - The AMS payload to be sent
 * \param payloadLen - The length of the AMS payload
 *
 * \return true, if operation was successful. false if the message could not be sent.
 */
bool UCSI_SendAmsMessage(UCSI_Data_t *my, uint16_t msgId, uint16_t targetAddress, uint8_t *pPayload, uint32_t payloadLen);

/**
 * \brief Gets the queued AMS message from UNICENS stack
 *
 * \note Call this function only from single context (not from ISR)
 * \note This function may be called cyclic or when UCSI_CB_OnAmsMessageReceived was raised
 *
 * \param pPriv - private data section of this instance
 * \param pMsgId - The received AMS message id will be written to this pointer
 * \param pSourceAddress - The received AMS source address will be written to this pointer
 * \param pPayload - The received AMS payload will be written to this pointer
 * \param pPayloadLen - The received AMS payload length will be written to this pointer
 *
 * \return true, if operation was successful. false if no message got be retrieved.
 */
bool UCSI_GetAmsMessage(UCSI_Data_t *my, uint16_t *pMsgId, uint16_t *pSourceAddress, uint8_t **pPayload, uint32_t *pPayloadLen);

/**
 * \brief Releases the message memory returned by UCSI_GetAmsMessage.
 *
 * \note Call this function only from single context (not from ISR)
 * \note This function must be called when the data of UCSI_GetAmsMessage has been processed.
 *       If this function is not called, UCSI_GetAmsMessage will return always the reference to the same data.
 * \note UCSI_Service may also free the data returned by UCSI_GetAmsMessage!
 *
 * \param pPriv - private data section of this instance
 */
void UCSI_ReleaseAmsMessage(UCSI_Data_t *my);

/**
 * \brief Enables or disables a route by the given routeId
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 * \param routeId - identifier as given in XML file along with MOST socket (unique)
 * \param isActive - true, route will become active. false, route will be deallocated
 *
 * \return true, if route was found and the specific command was enqueued to UNICENS.
 */
bool UCSI_SetRouteActive(UCSI_Data_t *pPriv, uint16_t routeId, bool isActive);

/**
 * \brief Performs an remote I2C write command
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 * \param targetAddress - targetAddress - The node / group target address
 * \param isBurst - true, write blockCount I2C telegrams dataLen with a single call. false, write a single I2C message.
 * \param blockCount - amount of blocks to write. Only used when isBurst is set to true.
 * \param slaveAddr - The I2C address.
 * \param timeout - Timeout in milliseconds.
 * \param dataLen - Amount of bytes to send via I2C
 * \param pData - The payload to be send.
 *
 * \return true, if route command was enqueued to UNICENS.
 */
bool UCSI_I2CWrite(UCSI_Data_t *pPriv, uint16_t targetAddress, bool isBurst, uint8_t blockCount,
    uint8_t slaveAddr, uint16_t timeout, uint8_t dataLen, const uint8_t *pData);

/**
 * \brief Performs an remote I2C read command.
 * \note UCSI_CB_OnI2CRead will be called after this command has been executed
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 * \param targetAddress - targetAddress - The node / group target address
 * \param slaveAddr - The I2C address.
 * \param timeout - Timeout in milliseconds.
 * \param dataLen - Amount of bytes to send via I2C
 *
 * \return true, if route command was enqueued to UNICENS.
 */
bool UCSI_I2CRead(UCSI_Data_t *pPriv, uint16_t targetAddress,
    uint8_t slaveAddr, uint16_t timeout, uint8_t dataLen);

/**
 * \brief Enables or disables a route by the given routeId
 * \note Call this function only from single context (not from ISR)
 *
 * \param pPriv - private data section of this instance
 * \param targetAddress - targetAddress - The node / group target address
 * \param gpioPinId - INIC GPIO PIN starting with 0 for the first GPIO.
 * \param isHighState - true, high state = 3,3V. false, low state = 0V.
 *
 * \return true, if GPIO command was enqueued to UNICENS.
 */
bool UCSI_SetGpioState(UCSI_Data_t *pPriv, uint16_t targetAddress, uint8_t gpioPinId, bool isHighState);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        CALLBACK SECTION                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Callback when ever a function above was tried to be executed.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param command - Enumeration value, identify the used command
 * \param success - true, if given command was successfully executed. false, either the direct call of the command or its async callback signaled an error
 * \param nodeAddress - the address of the node reporting this event. 0x1 in case of the local master node. 0xFFFF in case if the node address is unknown.
 */
extern void UCSI_CB_OnCommandResult(void *pTag, UnicensCmd_t command, bool success, uint16_t nodeAddress);

/**
 * \brief Callback when ever a timestamp is needed
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \return timestamp in milliseconds
 */
extern uint16_t UCSI_CB_OnGetTime(void *pTag);


/**
 * \brief Callback when the implementer needs to arm a timer.
 * \note This function must be implemented by the integrator
 * \note After timer expired, call the UCSI_Timeout from service
 *       Thread. (Not from callback!)
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param timeout - milliseconds from now on to call back. (0=disable)
 */
extern void UCSI_CB_OnSetServiceTimer(void *pTag, uint16_t timeout);

/**
 * \brief Callback when ever the state of the Network has changed.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param isAvailable - true, if the network is operable. false, network is down. No message or stream can be sent or received.
 * \param packetBandwidth - The amount of bytes per frame reserved for the Ethernet channel. Must match to the given packetBw value passed to UCSI_NewConfig.
 * \param amountOfNodes - The amount of network devices found in the ring.
 */
extern void UCSI_CB_OnNetworkState(void *pTag, bool isAvailable, uint16_t packetBandwidth, uint8_t amountOfNodes);

/**
 * \brief Callback when ever UNICENS forms a human readable message.
 *        This can be error events or when enabled also debug messages.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param isError - true, if this message is an important error message. false, user/debug message, not important.
 * \param format - Zero terminated format string (following printf rules)
 * \param vargsCnt - Amount of parameters stored in "..."
 */
extern void UCSI_CB_OnUserMessage(void *pTag, bool isError, const char format[],
    uint16_t vargsCnt, ...);

/**
 * \brief Callback when ever this instance needs to be serviced.
 * \note Call UCSI_Service by your scheduler at the next run
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 */
extern void UCSI_CB_OnServiceRequired(void *pTag);

/**
 * \brief Callback when ever this instance of UNICENS wants to send control data to the LLD.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param pPayload - Byte array to be sent on the INIC control channel
 * \param payloadLen - Length of pPayload in Byte
 */
extern void UCSI_CB_OnTxRequest(void *pTag,
    const uint8_t *pPayload, uint32_t payloadLen);

/**
 * \brief Callback when UNICENS instance has been started.
 * \note This event can be used to enable control message reception
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 */
extern void UCSI_CB_OnStart(void *pTag);

/**
 * \brief Callback when UNICENS instance has been stopped.
 * \note This event can be used to free memory holding the resources
 *       passed with UCSI_NewConfig
 * \note This event can be used to stop control message reception
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 */
extern void UCSI_CB_OnStop(void *pTag);

/**
 * \brief Callback when UNICENS instance has received an AMS message
 * \note This function must be implemented by the integrator
 * \note After this callback, call UCSI_GetAmsMessage indirect by setting a flag
 * \param pTag - Pointer given by the integrator by UCSI_Init
 */
extern void UCSI_CB_OnAmsMessageReceived(void *pTag);

/**
 * \brief Callback when a route become active / inactive.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param routeId - identifier as given in XML file along with MOST socket (unique)
 * \param isActive - true, if the route is now in use. false, the route is not established.
 * \param connectionLabel - The connection label used on the Network. Only valid, if isActive=true
 */
extern void UCSI_CB_OnRouteResult(void *pTag, uint16_t routeId, bool isActive, uint16_t connectionLabel);

/**
 * \brief Callback when a INIC GPIO changes its state
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param nodeAddress - Node Address of the INIC sending the update.
 * \param gpioPinId - INIC GPIO PIN starting with 0 for the first GPIO.
 * \param isHighState - true, high state = 3,3V. false, low state = 0V.
 */
extern void UCSI_CB_OnGpioStateChange(void *pTag, uint16_t nodeAddress, uint8_t gpioPinId, bool isHighState);

/**
 * \brief Callback when an I2C Read (triggered by UCSI_I2CRead) command has been executed
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param success - true, if the data could be read. false, otherwise
 * \param targetAddress - targetAddress - The node / group target address
 * \param slaveAddr - The I2C address.
 * \param pBuffer - If success is set to true, then the payload in this buffer is valid
 * \param bufLen - Length of buffer
 */
extern void UCSI_CB_OnI2CRead(void *pTag, bool success, uint16_t targetAddress, uint8_t slaveAddr, const uint8_t *pBuffer, uint32_t bufLen);

#ifdef __cplusplus
}
#endif

#endif /* UCSI_H_ */