/*------------------------------------------------------------------------------------------------*/
/* UNICENS XML Parser                                                                             */
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
#ifndef UCSXMLDRIVERCONFIG_H_
#define UCSXMLDRIVERCONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DriverPhyUnknown,
    DriverPhyUsb,
    DriverPhyMlb
} DriverPhysicalLayer_t;

typedef enum
{
    DriverCfgDirection_Tx,
    DriverCfgDirection_Rx
} DriverCfgDirection_t;

typedef enum
{
    DriverCfgDataType_Control,
    DriverCfgDataType_Async,
    DriverCfgDataType_Sync,
    DriverCfgDataType_Isoc
} DriverCfgDataType_t;

typedef enum
{
    Driver_LinuxCdev,
    Driver_LinuxAlsa,
    Driver_LinuxV4l2,
    Driver_LinuxNetwork
} DriverType_t;

typedef struct
{
    const char *channelName;
    const char *aimName;
    DriverCfgDataType_t dataType;
    DriverCfgDirection_t direction;
    uint16_t numBuffers;
    uint16_t bufferSize;
    uint16_t subBufferSize;
    uint16_t packetsPerXact;
} LinuxDriverCdev_t;

typedef struct
{
    const char *channelName;
    const char *aimName;
    DriverCfgDataType_t dataType;
    DriverCfgDirection_t direction;
    uint16_t numBuffers;
    uint16_t bufferSize;
    uint16_t subBufferSize;
    uint16_t packetsPerXact;
} LinuxDriverV4l2_t;

typedef struct
{
    const char *channelName;
    const char *aimName;
    DriverCfgDataType_t dataType;
    DriverCfgDirection_t direction;
    uint16_t numBuffers;
    uint16_t bufferSize;
    uint16_t subBufferSize;
    uint16_t packetsPerXact;
    uint8_t amountOfChannels;
    uint8_t resolutionInBit;
} LinuxDriverAlsa_t;

typedef struct
{
    const char *channelName;
    const char *aimName;
    DriverCfgDataType_t dataType;
    DriverCfgDirection_t direction;
    uint16_t numBuffers;
    uint16_t bufferSize;
} LinuxDriverNetwork_t;

typedef struct
{
    const char *linkName;
    DriverPhysicalLayer_t phy;
    uint16_t nodeAddress;
    DriverType_t driverType;
    union
    {
        LinuxDriverCdev_t LinuxCdev;
        LinuxDriverV4l2_t LinuxV4l2;
        LinuxDriverAlsa_t LinuxAlsa;
        LinuxDriverNetwork_t LinuxNetwork;
    } drv;
} DriverInformation_t;
    
#ifdef __cplusplus
}
#endif

#endif /* UCSXMLDRIVERCONFIG_H_ */