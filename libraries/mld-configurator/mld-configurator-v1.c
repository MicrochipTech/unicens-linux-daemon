/* MOST Linux Driver Configurator                                                                 */
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

#define _DEFAULT_SOURCE /* usleep */

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mld-configurator-v1.h"

#define PATH_LEN    (80)
#define VAL_LEN     (40)

struct MldConfigLocal
{
    bool started;
    bool allowRun;
    DriverInformation_t **pConfig;
    uint16_t driverSize;
    uint16_t localNodeAddress;
    uint16_t pollTime;
    pthread_t workerThread;
    char descriptionFilter[VAL_LEN];
    char ctxName[VAL_LEN];
    char crxName[VAL_LEN];
};

static struct MldConfigLocal m = { 0 };
static const char *SYS_FS_PATH = ("/sys/class/most/mostcore/devices");
static void *Worker(void *tag);
static char *ExtendControlCdevName(char *out, char * in);
static bool DoesFileExist(const char *pPathToFile);
static bool ReadFromFile(const char *pFileName, char *pString, uint16_t bufferLen);
static bool WriteCharactersToFile( const char *path, const char *pFileName, const char *pString );
static bool WriteIntegerToFile( const char *path, const char *pFileName, int intValue );
static void ReplaceCharsInString(char *target, const char *pWrongChars, char replaceBy);
static void IterateDirectory(const char *path, char *intf, uint32_t intfLen, char *descr, uint32_t descrLen, const char *parent, const char *grandParent);
static void CheckDriverSettings(const char* channelName, const char* deviceName, const char *fullPath, const char *intf, const char *descr);
static bool ConfigureCdev(const char *fullPath, DriverInformation_t *drv);
static bool LinkCdev(const char* channelName, const char* deviceName, DriverInformation_t *driver);
static bool ConfigureAlsa(const char *fullPath, DriverInformation_t *drv);
static bool LinkAlsa(const char* channelName, const char* deviceName, DriverInformation_t *driver);
static bool ConfigureV4L2(const char *fullPath, DriverInformation_t *driver);
static bool LinkV4L2(const char* channelName, const char* deviceName, DriverInformation_t *driver);
static bool ConfigureNetwork(const char *fullPath, DriverInformation_t *driver);
static bool LinkNetwork(const char* channelName, const char* deviceName, DriverInformation_t *driver);

bool MldConfigV1_Start(DriverInformation_t **pConfig, uint16_t driverSize, uint16_t localNodeAddress, const char *descriptionFilter, uint16_t pollTime)
{
    if (NULL == pConfig || 0 == driverSize || 0 == pollTime) 
        return false;
    if (m.started)
        MldConfigV1_Stop();
    
    m.pConfig = pConfig;
    m.driverSize = driverSize;
    m.localNodeAddress = localNodeAddress;
    m.pollTime = pollTime;
    m.allowRun = true;
    if (NULL != descriptionFilter)
        strncpy(m.descriptionFilter, descriptionFilter, sizeof(m.descriptionFilter));
    m.started = (0 == pthread_create(&m.workerThread, NULL, Worker, &m));
    return m.started;
}

void MldConfigV1_Stop()
{
    void *returnVal;
    if (!m.started && !m.allowRun)
        return;
    m.allowRun = false;
    pthread_join(m.workerThread, &returnVal);
    m.started = false;
}

bool MldConfigV1_GetControlCdevName(char *pControlCdevTx, char *pControlCdevRx)
{
    if (NULL == pControlCdevTx || NULL == pControlCdevRx)
        return false;
    if (!DoesFileExist(ExtendControlCdevName(pControlCdevTx, "ep0f")) &&
        !DoesFileExist(ExtendControlCdevName(pControlCdevTx, "ep07")) &&
        !DoesFileExist(ExtendControlCdevName(pControlCdevTx, "ca4")))
    {
        return false;
    }
    if (!DoesFileExist(ExtendControlCdevName(pControlCdevRx, "ep8f")) &&
        !DoesFileExist(ExtendControlCdevName(pControlCdevRx, "ep87")) &&
        !DoesFileExist(ExtendControlCdevName(pControlCdevRx, "ca2")))
    {
        return false;
    }
    return true;
}

static void *Worker(void *tag)
{
    char intf[VAL_LEN];
    char descr[VAL_LEN];
    while(m.allowRun)
    {
        intf[0] = '\0';
        descr[0] = '\0';
        IterateDirectory(SYS_FS_PATH, intf, sizeof(intf), descr, sizeof(descr), NULL, NULL);
        usleep(1000 * m.pollTime);
    }
    return tag;
}

static char *ExtendControlCdevName(char *out, char * in)
{
    if (NULL == out || NULL == in)
        return NULL;
    strcpy(out, "/dev/inic-control-");
    if ('\0' != m.descriptionFilter[0])
    {
        strcat(out, m.descriptionFilter);
        strcat(out, "-");
    }
    ReplaceCharsInString(out, " .:;|!", '_');
    strcat(out, in);
    return out;
}

static bool DoesFileExist(const char *pPathToFile)
{
    return (-1 != access(pPathToFile, F_OK));
}

static bool ReadFromFile(const char *pFileName, char *pString, uint16_t bufferLen)
{
    FILE *fh;
    bool success = false;
    if( NULL == pString || 0 == bufferLen )
        return success;
    fh = fopen( pFileName, "r" );
    if( NULL != fh )
    {
        success = ( NULL != fgets( pString, bufferLen, fh ) );
        fclose( fh );
    } else {
        MldConfigV1_CB_OnMessage(true, "Read failed for '%s'", 1, pFileName);
    }
    /* Eliminate carriage return */
    if (success)
    {
        int32_t len = strlen(pString);
        if (0 != len && '\n' == pString[len -1])
            pString[len -1] = '\0';
    }
    return success;
}

static bool WriteCharactersToFile( const char *path, const char *pFileName, const char *pString )
{
    FILE *fh;
    bool success = false;
    char combined[PATH_LEN];
    snprintf(combined, sizeof(combined), "%s/%s", path, pFileName);
    fh = fopen( combined, "a" );
    if( NULL != fh )
    {
        int result = fputs( pString, fh );
        if( result >= 0 )
            fputc( '\n', fh );
        if( result >= 0 )
            success = true;
        fclose( fh );
    }
    MldConfigV1_CB_OnMessage(!success, "'%s/%s'='%s' %s", 4, path, pFileName, pString, success ? "success" : "fail");
    return success;
}

static bool WriteIntegerToFile( const char *path, const char *pFileName, int intValue )
{
    char tempBuffer[16];
    snprintf( tempBuffer, sizeof( tempBuffer ), "%d", intValue );
    return WriteCharactersToFile( path, pFileName, tempBuffer );
}

static void ReplaceCharsInString(char *target, const char *pWrongChars, char replaceBy)
{
    if (NULL == target || NULL == pWrongChars)
        return;
    while('\0' != *target)
    {
        const char *w = pWrongChars;
        while('\0' != *w)
        {
            if (*target == *w)
                *target = replaceBy;
            ++w;
        }
        ++target;
    }
}

static void IterateDirectory(const char *path, char *intf, uint32_t intfLen, char *descr, uint32_t descrLen, const char *parent, const char *grandParent)
{
    DIR *d;
    struct dirent *dir;
    char combined[PATH_LEN];
    /* First parse files */
    if(NULL == (d = opendir(path)))
    {
        MldConfigV1_CB_OnMessage(true, "Can not open dir '%s'", 1, path);
        return;
    }
    while( NULL != (dir = readdir(d)))
    {
        if (DT_REG != dir->d_type)
            continue;
        if (0 == strcmp("interface", dir->d_name))
        {
            snprintf(combined, sizeof(combined), "%s/%s", path, dir->d_name);
            ReadFromFile(combined, intf, intfLen);
        }
        else if (0 == strcmp("description", dir->d_name))
        {
            snprintf(combined, sizeof(combined), "%s/%s", path, dir->d_name);
            ReadFromFile(combined, descr, descrLen);
        }
        else if (0 == strcmp("set_buffer_size", dir->d_name))
        {
            CheckDriverSettings(parent, grandParent, path, intf, descr);
        }
    }
    closedir(d);

    /* Go deeper into sub directories */
    if(NULL == (d = opendir(path)))
        return;
    while( NULL != (dir = readdir(d)))
    {
        if (DT_DIR != dir->d_type || '.' == dir->d_name[0])
            continue;
        snprintf(combined, sizeof(combined), "%s/%s", path, dir->d_name);
        IterateDirectory(combined, intf, intfLen, descr, descrLen, dir->d_name, parent);
    }
    closedir(d);
}

static void CheckDriverSettings(const char* channelName, const char* deviceName, const char *fullPath, const char *intf, const char *descr)
{
    uint16_t i;
    char val[VAL_LEN];
    char path[PATH_LEN];
    DriverPhysicalLayer_t curPhy = DriverPhyUnknown;
    DriverInformation_t *drv = NULL;
    DriverInformation_t localDrv = { 0 };
    if (NULL == m.pConfig || 0 == m.driverSize)
        return;
    if ('\0' != *m.descriptionFilter && NULL != descr && NULL == strstr(descr, m.descriptionFilter))
        return;

    if (0 == strcmp("usb", intf))
        curPhy = DriverPhyUsb;
    else if (0 == strcmp("mlb_dim2", intf))
        curPhy = DriverPhyMlb;
    else return;

    if (0 == strcmp("ep0f", channelName) || 0 == strcmp("ep8f", channelName) ||
        0 == strcmp("ep07", channelName) || 0 == strcmp("ep87", channelName) ||
        0 == strcmp("ca2", channelName) || 0 == strcmp("ca4", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxCdev;
        localDrv.drv.LinuxCdev.aimName = "control";
        localDrv.drv.LinuxCdev.dataType = DriverCfgDataType_Control;
        localDrv.drv.LinuxCdev.numBuffers = 16;
        localDrv.drv.LinuxCdev.bufferSize = 64;
    }
    else if (0 == strcmp("ep0e", channelName) || 0 == strcmp("ep8e", channelName) ||
        0 == strcmp("ep06", channelName) || 0 == strcmp("ep86", channelName) ||
        0 == strcmp("ca6", channelName) || 0 == strcmp("ca8", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxNetwork;
        localDrv.drv.LinuxNetwork.aimName = "network";
        localDrv.drv.LinuxNetwork.dataType = DriverCfgDataType_Async;
        localDrv.drv.LinuxNetwork.numBuffers = 20;
        localDrv.drv.LinuxNetwork.bufferSize = 1522;
    }
    if (NULL != drv)
    {
        if (0 == strcmp("ep0f", channelName) || 0 == strcmp("ep07", channelName) || 0 == strcmp("ca4", channelName))
        {
            localDrv.drv.LinuxCdev.direction = DriverCfgDirection_Tx;
        }
        else if (0 == strcmp("ep8f", channelName) || 0 == strcmp("ep87", channelName) || 0 == strcmp("ca2", channelName))
        {
            localDrv.drv.LinuxCdev.direction = DriverCfgDirection_Rx;
        }
        else if (0 == strcmp("ep0e", channelName) || 0 == strcmp("ep06", channelName) || 0 == strcmp("ca8", channelName))
        {
            localDrv.drv.LinuxNetwork.direction = DriverCfgDirection_Tx;
        }
        if (0 == strcmp("ep8e", channelName) || 0 == strcmp("ep86", channelName) || 0 == strcmp("ca6", channelName))
        {
            localDrv.drv.LinuxNetwork.direction = DriverCfgDirection_Rx;
        }
    }
    for (i = 0; NULL == drv && i < m.driverSize; i++)
    {
        DriverInformation_t *tmp = m.pConfig[i];
        if (m.localNodeAddress != tmp->nodeAddress)
            continue;
        if (curPhy != tmp->phy)
            continue;
        switch(tmp->driverType)
        {
        case Driver_LinuxCdev:
            if (0 != strcmp(channelName, tmp->drv.LinuxCdev.channelName))
                continue;
            break;
        case Driver_LinuxAlsa:
            if (0 != strcmp(channelName, tmp->drv.LinuxAlsa.channelName))
                continue;
            break;
        case Driver_LinuxV4l2:
            if (0 != strcmp(channelName, tmp->drv.LinuxV4l2.channelName))
                continue;
            break;
        default:
            continue;
        }
        drv = tmp;
        break;
    }
    if (NULL == drv)
        return;
    /* Check if channel is already configured */
    snprintf(path, sizeof(path), "%s/%s", fullPath, "set_buffer_size");
    if (!ReadFromFile(path, val, sizeof(val)))
        return;
    if (0 != strcmp("0", val))
        return; /* Already configured */
    switch(drv->driverType)
    {
    case Driver_LinuxCdev:
        ConfigureCdev(fullPath, drv) && LinkCdev(channelName, deviceName, drv);
        break;
    case Driver_LinuxAlsa:
        ConfigureAlsa(fullPath, drv) && LinkAlsa(channelName, deviceName, drv);
        break;
    case Driver_LinuxV4l2:
        ConfigureV4L2(fullPath, drv) && LinkV4L2(channelName, deviceName, drv);
        break;
    case Driver_LinuxNetwork:
        ConfigureNetwork(fullPath, drv) && LinkNetwork(channelName, deviceName, drv);
        break;
    }
}

static bool ConfigureCdev(const char *fullPath, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverCdev_t *drv = &driver->drv.LinuxCdev;
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "isoc");
    else if (DriverCfgDataType_Control == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "control");
    else if (DriverCfgDataType_Async == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "async");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "set_subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "set_packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "set_number_of_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "set_buffer_size", drv->bufferSize);
    return success;
}

static bool LinkCdev(const char* channelName, const char* deviceName, DriverInformation_t *driver)
{
    char val[VAL_LEN];
    char aimName[VAL_LEN] = { 0 };
    LinuxDriverCdev_t *drv = &driver->drv.LinuxCdev;
    if (NULL != drv->aimName)
        strncpy(aimName, drv->aimName, sizeof(aimName));
    if ('\0' != m.descriptionFilter[0])
    {
        strncat(aimName, "-", (sizeof(aimName) - strlen(aimName) - 1));
        strncat(aimName, m.descriptionFilter, (sizeof(aimName) - strlen(aimName) - 1));
    }
    ReplaceCharsInString(aimName, " .:;/|!", '_');
    snprintf(val, sizeof(val), "%s:%s:inic-%s-%s", deviceName, channelName, aimName, channelName);
    return WriteCharactersToFile("/sys/devices/virtual/most/mostcore/aims/cdev", "add_link", val);
}

static bool ConfigureAlsa(const char *fullPath, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverAlsa_t *drv = &driver->drv.LinuxAlsa;
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "isoc");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "set_subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "set_packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "set_number_of_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "set_buffer_size", drv->bufferSize);
    return success;
}

static bool LinkAlsa(const char* channelName, const char* deviceName, DriverInformation_t *driver)
{
    char val[VAL_LEN];
    char aimName[VAL_LEN] = { 0 };
    LinuxDriverAlsa_t *drv = &driver->drv.LinuxAlsa;
    if (NULL != drv->aimName)
        strncpy(aimName, drv->aimName, sizeof(aimName));
    if ('\0' != m.descriptionFilter[0])
    {
        strncat(aimName, "-", (sizeof(aimName) - strlen(aimName) - 1));
        strncat(aimName, m.descriptionFilter, (sizeof(aimName) - strlen(aimName) - 1));
    }
    ReplaceCharsInString(aimName, " .:;/|!", '_');
    snprintf(val, sizeof(val), "%s:%s:inic-%s-%s.%dx%d", deviceName, channelName, 
        aimName,
        channelName,
        drv->amountOfChannels,
        drv->resolutionInBit
        );
    return WriteCharactersToFile("/sys/devices/virtual/most/mostcore/aims/sound", "add_link", val);
}

static bool ConfigureV4L2(const char *fullPath, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverV4l2_t *drv = &driver->drv.LinuxV4l2;
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "set_datatype", "isoc");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "set_subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "set_packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "set_number_of_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "set_buffer_size", drv->bufferSize);
    return success;
}

static bool LinkV4L2(const char* channelName, const char* deviceName, DriverInformation_t *driver)
{
    char val[VAL_LEN];
    char aimName[VAL_LEN] = { 0 };
    LinuxDriverV4l2_t *drv = &driver->drv.LinuxV4l2;
    if (NULL != drv->aimName)
        strncpy(aimName, drv->aimName, sizeof(aimName));
    if ('\0' != m.descriptionFilter[0])
    {
        strncat(aimName, "-", (sizeof(aimName) - strlen(aimName) - 1));
        strncat(aimName, m.descriptionFilter, (sizeof(aimName) - strlen(aimName) - 1));
    }
    ReplaceCharsInString(aimName, " .:;/|!", '_');
    snprintf(val, sizeof(val), "%s:%s:inic-%s-%s", deviceName, channelName, aimName, channelName);
    return WriteCharactersToFile("/sys/devices/virtual/most/mostcore/aims/v4l", "add_link", val);
}

static bool ConfigureNetwork(const char *fullPath, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverNetwork_t *drv = &driver->drv.LinuxNetwork;
    success &= WriteCharactersToFile(fullPath, "set_datatype", "async");

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "set_direction", "rx");
    else return false;

    success &= WriteIntegerToFile(fullPath, "set_number_of_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "set_buffer_size", drv->bufferSize);
    return success;
}

static bool LinkNetwork(const char* channelName, const char* deviceName, DriverInformation_t *driver)
{
    char val[VAL_LEN];
    char aimName[VAL_LEN] = { 0 };
    LinuxDriverNetwork_t *drv = &driver->drv.LinuxNetwork;
    if (NULL != drv->aimName)
        strncpy(aimName, drv->aimName, sizeof(aimName));
    if ('\0' != m.descriptionFilter[0])
    {
        strncat(aimName, "-", (sizeof(aimName) - strlen(aimName) - 1));
        strncat(aimName, m.descriptionFilter, (sizeof(aimName) - strlen(aimName) - 1));
    }
    ReplaceCharsInString(aimName, " .:;/|!", '_');
    snprintf(val, sizeof(val), "%s:%s:inic-%s-%s", deviceName, channelName, aimName, channelName);
    return WriteCharactersToFile("/sys/devices/virtual/most/mostcore/aims/networking", "add_link", val);
}