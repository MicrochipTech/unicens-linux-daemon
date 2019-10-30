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

#define _DEFAULT_SOURCE /* usleep */

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mld-configurator-v2.h"

#define PATH_LEN    (80)
#define VAL_LEN     (40)

struct MldConfigLocal
{
    DriverInformation_t **pConfig;
    char descriptionFilter[VAL_LEN];
    char ctxName[VAL_LEN];
    char crxName[VAL_LEN];
    pthread_t workerThread;
    uint16_t driverSize;
    uint16_t localNodeAddress;
    uint16_t pollTime;
    bool started;
    bool allowRun;
    bool createAlsaCard;
};

static struct MldConfigLocal m = { 0 };
static const char *SYS_FS_PATH = ("/sys/bus/most/devices");
#ifdef ANDROID
static const char *CON_FS_PATH = ("/config");
#else
static const char *CON_FS_PATH = ("/sys/kernel/config");
#endif
static const char *AIM_CDEV = ("most_cdev");
static const char *AIM_SOUND = ("most_sound");
static const char *AIM_V4L = ("most_video");
static const char *AIM_NETWORK = ("most_net");
static const char *ALSA_CARD_NAME = ("card");
static void *Worker(void *tag);
static char *ExtendControlCdevName(char *out, char * in);
static bool CreateFolder(const char *pFullPath);
static bool DoesFileExist(const char *pPathToFile);
static bool ReadFromFile(const char *pFileName, char *pString, uint16_t bufferLen);
static bool WriteCharactersToFile( const char *path, const char *pFileName, const char *pString );
static bool WriteIntegerToFile( const char *path, const char *pFileName, int intValue );
static void ReplaceCharsInString(char *target, const char *pWrongChars, char replaceBy);
static void IterateDirectory(const char *path, char *intf, uint32_t intfLen, char *descr, uint32_t descrLen, const char *parent, const char *grandParent);
static void CheckDriverSettings(const char* channelName, const char* deviceName, const char *fullPath, const char *intf, const char *descr);
static void WriteDriverConfig(const char* channelName, const char* deviceName, const char *fullPath, DriverInformation_t *drv);
static bool ConfigureCdev(const char *deviceName, DriverInformation_t *drv);
static bool ConfigureAlsa(const char *deviceName, DriverInformation_t *drv);
static bool CreateAlsaCard(void);
static bool ConfigureV4L2(const char *deviceName, DriverInformation_t *driver);
static bool ConfigureNetwork(const char *deviceName, DriverInformation_t *driver);

bool MldConfigV2_Start(DriverInformation_t **pConfig, uint16_t driverSize, uint16_t localNodeAddress, const char *descriptionFilter, uint16_t pollTime)
{
    if (0 == pollTime) 
        return false;
    if (m.started)
        MldConfigV2_Stop();
    
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

void MldConfigV2_Stop()
{
    void *returnVal;
    if (!m.started && !m.allowRun)
        return;
    m.allowRun = false;
    pthread_join(m.workerThread, &returnVal);
    m.started = false;
}

bool MldConfigV2_GetControlCdevName(char *pControlCdevTx, char *pControlCdevRx)
{
    if (NULL == pControlCdevTx || NULL == pControlCdevRx)
        return false;
    if (!DoesFileExist(ExtendControlCdevName(pControlCdevTx, "tx")))
        return false;
    if (!DoesFileExist(ExtendControlCdevName(pControlCdevRx, "rx")))
        return false;
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
        if (m.createAlsaCard) {
            m.createAlsaCard = false;
            CreateAlsaCard();
        }
        usleep(1000 * m.pollTime);
    }
    return tag;
}

static char *ExtendControlCdevName(char *out, char * in)
{
    static const char EXTENSION[] = "/dev/inic-control-";
    if (NULL == out || NULL == in)
        return NULL;
    strncpy(out, EXTENSION, sizeof(EXTENSION));
    if ('\0' != m.descriptionFilter[0])
    {
        strncat(out, m.descriptionFilter, VAL_LEN);
        strncat(out, "-", 1);
    }
    ReplaceCharsInString(out, " .:;|!", '_');
    strncat(out, in, 32);
    return out;
}

static bool DoesFileExist(const char *pPathToFile)
{
    return (-1 != access(pPathToFile, F_OK));
}

static bool CreateFolder(const char *pFullPath)
{
    struct stat st = { 0 };
    if (stat(pFullPath, &st)) {
        return (-1 != mkdir(pFullPath, 0x777));
    }
    return true;
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
        MldConfigV2_CB_OnMessage(true, "Read failed for '%s'", 1, pFileName);
    }
    /* Eliminate carriage return */
    if (success)
    {
        int32_t len = strnlen(pString, bufferLen);
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
    if (!DoesFileExist(combined))
        return false;
    fh = fopen( combined, "w" );
    if( NULL != fh )
    {
        int result = fputs( pString, fh );
        if( result >= 0 )
            fputc( '\n', fh );
        if( result >= 0 )
            success = true;
        fclose( fh );
    }
    MldConfigV2_CB_OnMessage(!success, "'%s/%s'='%s' %s", 4, path, pFileName, pString, success ? "success" : "fail");
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
        return;
    }
    while( NULL != (dir = readdir(d)))
    {
        if (0 == strcmp("driver", dir->d_name) || 0 == strcmp("subsystem", dir->d_name))
        {
            /* driver or subsystem links to it self, do not go deeper. Endless recursion */
            continue;
        }
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
        if ('.' == dir->d_name[0])
            continue;
        if (0 == strcmp("driver", dir->d_name) || 0 == strcmp("subsystem", dir->d_name))
        {
            /* driver or subsystem links to it self, do not go deeper. Endless recursion */
            continue;
        }
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
    if ('\0' != *m.descriptionFilter && NULL != descr && NULL == strstr(descr, m.descriptionFilter))
        return;
    /* Check if channel is already configured */
    snprintf(path, sizeof(path), "%s/%s", fullPath, "set_buffer_size");
    if (!ReadFromFile(path, val, sizeof(val)))
        return;
    if (0 != strcmp("0", val))
        return; /* Already configured */

    if (0 == strcmp("usb", intf))
        curPhy = DriverPhyUsb;
    else if (0 == strcmp("mlb_dim2", intf))
        curPhy = DriverPhyMlb;
    else return;

    if (0 == strcmp("ep0f", channelName) ||
        0 == strcmp("ep07", channelName) ||
        0 == strcmp("ca4", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxCdev;
        localDrv.drv.LinuxCdev.channelName = channelName;
        localDrv.drv.LinuxCdev.aimName = "control-tx";
        localDrv.drv.LinuxCdev.dataType = DriverCfgDataType_Control;
        localDrv.drv.LinuxCdev.numBuffers = 16;
        localDrv.drv.LinuxCdev.bufferSize = 64;
    }
    else if (0 == strcmp("ep8f", channelName) ||
        0 == strcmp("ep87", channelName) ||
        0 == strcmp("ca2", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxCdev;
        localDrv.drv.LinuxCdev.channelName = channelName;
        localDrv.drv.LinuxCdev.aimName = "control-rx";
        localDrv.drv.LinuxCdev.dataType = DriverCfgDataType_Control;
        localDrv.drv.LinuxCdev.numBuffers = 16;
        localDrv.drv.LinuxCdev.bufferSize = 64;
    }
    else if (0 == strcmp("ep0e", channelName) ||
        0 == strcmp("ep06", channelName) ||
        0 == strcmp("ca8", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxNetwork;
        localDrv.drv.LinuxCdev.channelName = channelName;
        localDrv.drv.LinuxNetwork.aimName = "network-tx";
        localDrv.drv.LinuxNetwork.dataType = DriverCfgDataType_Async;
        localDrv.drv.LinuxNetwork.numBuffers = 20;
        localDrv.drv.LinuxNetwork.bufferSize = 1522;
    }
    else if (0 == strcmp("ep8e", channelName) ||
        0 == strcmp("ep86", channelName) ||
        0 == strcmp("ca6", channelName))
    {
        drv = &localDrv;
        localDrv.driverType = Driver_LinuxNetwork;
        localDrv.drv.LinuxCdev.channelName = channelName;
        localDrv.drv.LinuxNetwork.aimName = "network-rx";
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
        else if (0 == strcmp("ep8e", channelName) || 0 == strcmp("ep86", channelName) || 0 == strcmp("ca6", channelName))
        {
            localDrv.drv.LinuxNetwork.direction = DriverCfgDirection_Rx;
        }
        WriteDriverConfig(channelName, deviceName, fullPath, drv);
    }
    for (i = 0; NULL != m.pConfig && i < m.driverSize; i++)
    {
        drv = m.pConfig[i];
        if (!drv) continue;
        if (m.localNodeAddress != drv->nodeAddress) continue;
        if (curPhy != drv->phy) continue;
        switch(drv->driverType)
        {
        case Driver_LinuxCdev:
            if (0 != strcmp(channelName, drv->drv.LinuxCdev.channelName))
                continue;
            break;
        case Driver_LinuxAlsa:
            if (0 != strcmp(channelName, drv->drv.LinuxAlsa.channelName))
                continue;
            break;
        case Driver_LinuxV4l2:
            if (0 != strcmp(channelName, drv->drv.LinuxV4l2.channelName))
                continue;
            break;
        default:
            continue;
        }
        WriteDriverConfig(channelName, deviceName, fullPath, drv);
    }
}

static void WriteDriverConfig(const char* channelName, const char* deviceName, const char *fullPath, DriverInformation_t *drv)
{
    if (!channelName || !deviceName || !fullPath || !drv) return;
    switch(drv->driverType)
    {
    case Driver_LinuxCdev:
        ConfigureCdev(deviceName, drv);
        break;
    case Driver_LinuxAlsa:
        ConfigureAlsa(deviceName, drv);
        break;
    case Driver_LinuxV4l2:
        ConfigureV4L2(deviceName, drv);
        break;
    case Driver_LinuxNetwork:
        ConfigureNetwork(deviceName, drv);
        break;
    }
}

static bool ConfigureCdev(const char *deviceName, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverCdev_t *drv = &driver->drv.LinuxCdev;
    char fullPath[PATH_LEN];
    if (!DoesFileExist(CON_FS_PATH)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s", CON_FS_PATH, AIM_CDEV);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s/inic-%s", CON_FS_PATH, AIM_CDEV, drv->aimName);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    success &= WriteCharactersToFile(fullPath, "device", deviceName);
    success &= WriteCharactersToFile(fullPath, "channel", drv->channelName);
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "isoc");
    else if (DriverCfgDataType_Control == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "control");
    else if (DriverCfgDataType_Async == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "async");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "num_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "buffer_size", drv->bufferSize);
    success &= WriteIntegerToFile(fullPath, "create_link", 1);
    return success;
}

static bool ConfigureAlsa(const char *deviceName, DriverInformation_t *driver)
{
    char format[16];
    bool success = true;
    LinuxDriverAlsa_t *drv = &driver->drv.LinuxAlsa;
    char fullPath[PATH_LEN];
    if (!DoesFileExist(CON_FS_PATH)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s", CON_FS_PATH, AIM_SOUND);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s", CON_FS_PATH, AIM_SOUND, ALSA_CARD_NAME);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s/%s", CON_FS_PATH, AIM_SOUND, ALSA_CARD_NAME, drv->aimName);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    success &= WriteCharactersToFile(fullPath, "device", deviceName);
    success &= WriteCharactersToFile(fullPath, "channel", drv->channelName);
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "isoc");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "num_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "buffer_size", drv->bufferSize);

    snprintf(format, sizeof(format), "%dx%d", drv->amountOfChannels, drv->resolutionInBit);
    success &= WriteCharactersToFile(fullPath, "comp_params", format);
    success &= WriteIntegerToFile(fullPath, "create_link", 1);

    m.createAlsaCard |= success;
    return success;
}

static bool CreateAlsaCard(void)
{
    char fullPath[PATH_LEN];
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s", CON_FS_PATH, AIM_SOUND, ALSA_CARD_NAME);
    return WriteIntegerToFile(fullPath, "create_card", 1);
}

static bool ConfigureV4L2(const char *deviceName, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverV4l2_t *drv = &driver->drv.LinuxV4l2;
    char fullPath[PATH_LEN];
    if (!DoesFileExist(CON_FS_PATH)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s", CON_FS_PATH, AIM_V4L);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s", CON_FS_PATH, AIM_V4L, drv->aimName);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    success &= WriteCharactersToFile(fullPath, "device", deviceName);
    success &= WriteCharactersToFile(fullPath, "channel", drv->channelName);
    if (DriverCfgDataType_Sync == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "sync");
    else if (DriverCfgDataType_Isoc == drv->dataType)
        success &= WriteCharactersToFile(fullPath, "datatype", "isoc");
    else return false;

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "rx");
    else return false;

    if (0 != drv->subBufferSize)
        success &= WriteIntegerToFile(fullPath, "subbuffer_size", drv->subBufferSize);
    if (DriverPhyUsb == driver->phy)
        success &= WriteIntegerToFile(fullPath, "packets_per_xact", drv->packetsPerXact);

    success &= WriteIntegerToFile(fullPath, "num_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "buffer_size", drv->bufferSize);
    success &= WriteIntegerToFile(fullPath, "create_link", 1);
    return success;
}

static bool ConfigureNetwork(const char *deviceName, DriverInformation_t *driver)
{
    bool success = true;
    LinuxDriverNetwork_t *drv = &driver->drv.LinuxNetwork;
    char fullPath[PATH_LEN];
    if (!DoesFileExist(CON_FS_PATH)) {
        return false;
    }
     snprintf(fullPath, sizeof(fullPath), "%s/%s", CON_FS_PATH, AIM_NETWORK);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    snprintf(fullPath, sizeof(fullPath), "%s/%s/%s", CON_FS_PATH, AIM_NETWORK, drv->aimName);
    if (!CreateFolder(fullPath)) {
        return false;
    }
    success &= WriteCharactersToFile(fullPath, "device", deviceName);
    success &= WriteCharactersToFile(fullPath, "channel", drv->channelName);
    success &= WriteCharactersToFile(fullPath, "datatype", "async");

    if (DriverCfgDirection_Tx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "tx");
    else if (DriverCfgDirection_Rx == drv->direction)
        success &= WriteCharactersToFile(fullPath, "direction", "rx");
    else return false;

    success &= WriteIntegerToFile(fullPath, "num_buffers", drv->numBuffers);
    success &= WriteIntegerToFile(fullPath, "buffer_size", drv->bufferSize);
    success &= WriteIntegerToFile(fullPath, "create_link", 1);
    return success;
}
