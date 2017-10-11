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
#ifndef UNICENSINTEGRATION_H_
#define UNICENSINTEGRATION_H_

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                  USER ADJUSTABLE VALUES                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#define ENABLE_INIC_WATCHDOG    (true)
#define ENABLE_AMS_LIB          (true)
#define DEBUG_XRM
#define BOARD_PMS_TX_SIZE       (72)
#define CMD_QUEUE_LEN           (4)
#define I2C_WRITE_MAX_LEN       (32)

#include <string.h>
#include <stdarg.h>

#include "ucs_cfg.h"
#include "ucs_api.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                          PRIVATE SECTION                             */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Internal enum for UNICENS Integration
 */
typedef enum
{
    /**Result is OK and the processing is finished. Safe to dequeue this command.*/
    UniCmdResult_OK_ProcessFinished,
    /**Result is OK but the processing is ongoing. Must wait for callback.*/
    UniCmdResult_OK_NeedToWaitForCB,
    /**Result is error and the processing is finished. Safe to dequeue this command.*/
    UniCmdResult_ERROR_ProcessFinished
} UnicensCmdResult_t;

/**
 * \brief Internal enum for UNICENS Integration
 */
typedef enum
{
    UnicensCmd_Unknown,
    UnicensCmd_Init,
    UnicensCmd_Stop,
    UnicensCmd_RmSetRoute,
    UnicensCmd_NsRun,
    UnicensCmd_GpioCreatePort,
    UnicensCmd_GpioWritePort,
    UnicensCmd_I2CWrite
} UnicensCmd_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    const Ucs_InitData_t *init_ptr;
} UnicensCmdInit_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    Ucs_Rm_Route_t *routePtr;
    bool isActive;
} UnicensCmdRmSetRoute_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    Ucs_Rm_Node_t * node_ptr;
} UnicensCmdNsRun_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    uint16_t destination;
    uint16_t debounceTime;
} UnicensCmdGpioCreatePort_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    uint16_t destination;
    uint16_t mask;
    uint16_t data;
} UnicensCmdGpioWritePort_t;

/**
 * \brief Internal struct for UNICENS Integration
 */
typedef struct
{
    uint16_t destination;
    bool isBurst;
    uint8_t blockCount;
    uint8_t slaveAddr;
    uint16_t timeout;
    uint8_t dataLen;
    uint8_t data[I2C_WRITE_MAX_LEN];
} UnicensCmdI2CWrite_t;

/**
 * \brief Internal struct for Unicens Integration
 */
typedef struct
{
    UnicensCmd_t cmd;
    union
    {
        UnicensCmdInit_t Init;
        UnicensCmdRmSetRoute_t RmSetRoute;
        UnicensCmdNsRun_t NsRun;
        UnicensCmdGpioCreatePort_t GpioCreatePort;
        UnicensCmdGpioWritePort_t GpioWritePort;
        UnicensCmdI2CWrite_t I2CWrite;
    } val;
} UnicensCmdEntry_t;

/**
 * \brief Internal variables for one instance of UNICENS Integration
 * \note Never touch any of this fields!
 */
typedef struct {
    volatile uint8_t *dataQueue;
    volatile uint8_t *pRx;
    volatile uint8_t *pTx;
    volatile uint32_t amountOfEntries;
    volatile uint32_t sizeOfEntry;
    volatile uint32_t rxPos;
    volatile uint32_t txPos;
} RB_t;

/**
 * \brief Internal variables for one instance of UNICENS Integration
 * \note Allocate this structure for each instance (static or malloc)
 *        and pass it to UCSI_Init()
 * \note Never touch any of this fields!
 */
typedef struct
{
    uint32_t magic;
    void *tag;
    bool initialized;
    RB_t rb;
    uint8_t rbBuf[(CMD_QUEUE_LEN * sizeof(UnicensCmdEntry_t))];
    Ucs_Inst_t *unicens;
    Ucs_InitData_t uniInitData;
    bool triggerService;
    Ucs_Lld_Api_t *uniLld;
    void *uniLldHPtr;
    UnicensCmdEntry_t *currentCmd;
} UCSI_Data_t;

#endif /* UNICENSINTEGRATION_H_ */