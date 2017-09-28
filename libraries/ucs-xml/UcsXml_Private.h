/*------------------------------------------------------------------------------------------------*/
/* Unicens XML Parser                                                                             */
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
#ifndef UCSXML_PRIVATE_H_
#define UCSXML_PRIVATE_H_

#include <stdbool.h>
#include <stdint.h>
#include "ucs_api.h"

typedef enum
{
    SYNC_DATA        = 0,      /*!< \brief  Specifies the synchronous streaming data type */
    CONTROL_DATA     = 2,      /*!< \brief  Specifies the control data type */
    AV_PACKETIZED    = 3,      /*!< \brief  Specifies the A/V Packetized Isochronous
                                            streaming data type */
    QOS_IP           = 4,      /*!< \brief  Specifies the Quality of Service IP
                                            streaming data type*/
    DISC_FRAME_PHASE = 5,      /*!< \brief  Specifies the DiscreteFrame Isochronous
                                            streaming phase data type */
    IPC_PACKET       = 7,       /*!< \brief Specifies the IPC packet data type */
    INVALID          = 0xFF     /*!< \brief Defined invalid value */
} MDataType_t;

struct UcsXmlObjectList
{
    void *obj;
    struct UcsXmlObjectList *next;
};

void *MCalloc(struct UcsXmlObjectList *list, uint32_t nElem, uint32_t elemSize);
void FreeObjList(struct UcsXmlObjectList *cur);

struct MostSocketParameters
{
    struct UcsXmlObjectList *list;
    bool isSource;
    MDataType_t dataType;
    uint16_t bandwidth;
};
bool GetMostSocket(Ucs_Xrm_MostSocket_t **mostSoc, struct MostSocketParameters *param);

struct UsbPortParameters
{
    struct UcsXmlObjectList *list;
    const char *physicalLayer;
    const char *deviceInterfaces;
    const char *streamInCount;
    const char *streamOutCount;
};
bool GetUsbPort(Ucs_Xrm_UsbPort_t **usbPort, struct UsbPortParameters *param);
bool GetUsbPortDefaultCreated(Ucs_Xrm_ResObject_t **usbPort, struct UcsXmlObjectList *list);

struct UsbSocketParameters
{
    struct UcsXmlObjectList *list;
    bool isSource;
    MDataType_t dataType;
    const char *endpointAddress;
    const char *framesPerTrans;
    Ucs_Xrm_ResObject_t *usbPort; /** Must be either Ucs_Xrm_UsbPort_t or Ucs_Xrm_DefaultCreatedPort_t  */
};
bool GetUsbSocket(Ucs_Xrm_UsbSocket_t **usbSoc, struct UsbSocketParameters *param);

struct MlbPortParameters
{
    struct UcsXmlObjectList *list;
    const char *clockConfig;
};
bool GetMlbPort(Ucs_Xrm_MlbPort_t **mlbPort, struct MlbPortParameters *param);
bool GetMlbPortDefaultCreated(Ucs_Xrm_ResObject_t **mlbPort, struct UcsXmlObjectList *list);

struct MlbSocketParameters
{
    struct UcsXmlObjectList *list;
    bool isSource;
    MDataType_t dataType;
    uint16_t bandwidth;
    const char *channelAddress;
    Ucs_Xrm_ResObject_t *mlbPort; /** Must be either Ucs_Xrm_MlbPort_t or Ucs_Xrm_DefaultCreatedPort_t  */
};
bool GetMlbSocket(Ucs_Xrm_MlbSocket_t **mlbSoc, struct MlbSocketParameters *param);

struct StrmPortParameters
{
    uint8_t index; /** Always create two Streaming Ports one with index 0 and one with index 1 */
    struct UcsXmlObjectList *list;
    const char *clockConfig;
    const char *dataAlignment;
};
bool GetStrmPort(Ucs_Xrm_StrmPort_t **strmPort, struct StrmPortParameters *param);

struct StrmSocketParameters
{
    struct UcsXmlObjectList *list;
    bool isSource;
    MDataType_t dataType;
    uint16_t bandwidth;
    const char *streamPin;
    Ucs_Xrm_StrmPort_t *streamPortA; /* Mandatory, set with index 0 */
    Ucs_Xrm_StrmPort_t *streamPortB; /* Mandatory, set with index 1 */
};
bool GetStrmSocket(Ucs_Xrm_StrmSocket_t **strmSoc, struct StrmSocketParameters *param);

struct SplitterParameters
{
    struct UcsXmlObjectList *list;
    uint16_t bytesPerFrame;
    Ucs_Xrm_ResObject_t *inSoc;
};
bool GetSplitter(Ucs_Xrm_Splitter_t **splitter, struct SplitterParameters *param);

struct CombinerParameters
{
    struct UcsXmlObjectList *list;
    uint16_t bytesPerFrame;
    Ucs_Xrm_ResObject_t *outSoc;
};
bool GetCombiner(Ucs_Xrm_Combiner_t **combiner, struct CombinerParameters *param);

struct SyncConParameters
{
    struct UcsXmlObjectList *list;
    const char *muteMode;
    const char *optional_offset;
    Ucs_Xrm_ResObject_t *inSoc;
    Ucs_Xrm_ResObject_t *outSoc;
};
bool GetSyncCon(Ucs_Xrm_SyncCon_t **syncCon, struct SyncConParameters *param);

struct AvpConParameters
{
    struct UcsXmlObjectList *list;
    const char *optional_isocPacketSize;
    Ucs_Xrm_ResObject_t *inSoc;
    Ucs_Xrm_ResObject_t *outSoc;
};
bool GetAvpCon(Ucs_Xrm_AvpCon_t **avpCon, struct AvpConParameters *param);


#endif /* UCSXML_PRIVATE_H_ */