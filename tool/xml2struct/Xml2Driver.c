/*------------------------------------------------------------------------------------------------*/
/* UNICENS Driver Printing module                                                                 */
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

#include <stddef.h>
#include "Console.h"
#include "Xml2Struct.h"
#include "Xml2Driver.h"

#define STRING_DRIVER_CH_RX       "MOST_CH_RX"
#define STRING_DRIVER_CH_TX       "MOST_CH_TX"

#define STRING_DRIVER_CH_CONTROL  "MOST_CH_CONTROL"
#define STRING_DRIVER_CH_ASYNC    "MOST_CH_ASYNC"
#define STRING_DRIVER_CH_SYNC     "MOST_CH_SYNC"
#define STRING_DRIVER_CH_ISOC     "MOST_CH_ISOC"

#define STRING_DRIVER_CH_CDEV     "cdev"
#define STRING_DRIVER_CH_V4L2     "v4l"
#define STRING_DRIVER_CH_ALSA     "sound"
#define STRING_DRIVER_CH_NETWORK  "networking"

typedef struct
{
    const char *comment;
    const char *linkName;
    const char *aimName;
    const char *aimParam;
    const char *channelName;
    const char *direction;
    const char *dataType;
    uint16_t numBuffers;
    uint16_t bufferSize;
    uint16_t subBufferSize;
    uint16_t packetsPerXact;
    uint8_t amountOfChannels;
    uint8_t resolutionInBit;
} DrvInfo_t;

static const DrvInfo_t controlRx118 =
{
    "OS81118 Control RX", NULL, STRING_DRIVER_CH_CDEV, "control-rx", "ep8f",
    STRING_DRIVER_CH_RX, STRING_DRIVER_CH_CONTROL, 16, 72, 0, 0, 0, 0
};

static const DrvInfo_t controlTx118 =
{
    "OS81118 Control TX", NULL, STRING_DRIVER_CH_CDEV, "control-tx", "ep0f",
    STRING_DRIVER_CH_TX, STRING_DRIVER_CH_CONTROL, 16, 72, 0, 0, 0, 0
};

static const DrvInfo_t asyncRx118 =
{
    "OS81118 Async (Ethernet) RX", NULL, STRING_DRIVER_CH_NETWORK, "network-rx", "ep8e",
    STRING_DRIVER_CH_RX, STRING_DRIVER_CH_ASYNC, 20, 1522, 0, 0, 0, 0
};

static const DrvInfo_t asyncTx118 =
{
    "OS81118 Async (Ethernet) TX", NULL, STRING_DRIVER_CH_NETWORK, "network-tx", "ep0e",
    STRING_DRIVER_CH_TX, STRING_DRIVER_CH_ASYNC, 20, 1522, 0, 0, 0, 0
};

static const DrvInfo_t controlRx210 =
{
    "OS81118 Control RX", NULL, STRING_DRIVER_CH_CDEV, "control-rx", "ep87",
    STRING_DRIVER_CH_RX, STRING_DRIVER_CH_CONTROL, 16, 72, 0, 0, 0, 0
};

static const DrvInfo_t controlTx210 =
{
    "OS81118 Control TX", NULL, STRING_DRIVER_CH_CDEV, "control-tx", "ep07",
    STRING_DRIVER_CH_TX, STRING_DRIVER_CH_CONTROL, 16, 72, 0, 0
};

static const DrvInfo_t asyncRx210 =
{
    "OS81118 Async (Ethernet) RX", NULL, STRING_DRIVER_CH_NETWORK, "network-rx", "ep86",
    STRING_DRIVER_CH_RX, STRING_DRIVER_CH_ASYNC, 20, 1522, 0, 0
};

static const DrvInfo_t asyncTx210 =
{
    "OS81118 Async (Ethernet) TX", NULL, STRING_DRIVER_CH_NETWORK, "network-tx", "ep06",
    STRING_DRIVER_CH_TX, STRING_DRIVER_CH_ASYNC, 20, 1522, 0, 0
};

static void PrintDriverInfo(DriverInformation_t *pDriver);
static void PrintDriverStructure(const DrvInfo_t *inf);
static void PrintHeader(uint16_t nodeAddress);
static void PrintFooter(uint16_t nodeAddress);
static const char *GetDirection(DriverCfgDirection_t dir);
static const char *GetDataType(DriverCfgDataType_t dtyp);

void PrintUcsDriver(uint16_t nodeAddress, DriverInformation_t **ppDriver, uint16_t driverSize)
{
    uint16_t i;
    if (NULL == ppDriver || 0 == driverSize)
        return;
    PrintHeader(nodeAddress);
    PrintDriverStructure(&controlRx118);
    PrintDriverStructure(&controlTx118);
    PrintDriverStructure(&asyncRx118);
    PrintDriverStructure(&asyncTx118);
    
    PrintDriverStructure(&controlRx210);
    PrintDriverStructure(&controlTx210);
    PrintDriverStructure(&asyncRx210);
    PrintDriverStructure(&asyncTx210);
    for (i = 0; i < driverSize; i++)
    {
        if (nodeAddress != ppDriver[i]->nodeAddress)
            continue;
        PrintDriverInfo(ppDriver[i]);
    }
    PrintFooter(nodeAddress);
}

static void PrintDriverInfo(DriverInformation_t *pDriver)
{
    DrvInfo_t inf = { 0 };
    if (NULL == pDriver) return;
    switch(pDriver->driverType)
    {
    case Driver_LinuxCdev:
        inf.aimName = STRING_DRIVER_CH_CDEV;
        inf.linkName = pDriver->linkName;
        inf.aimParam = pDriver->drv.LinuxCdev.aimName;
        inf.channelName = pDriver->drv.LinuxCdev.channelName;
        inf.direction = GetDirection(pDriver->drv.LinuxCdev.direction);
        inf.dataType = GetDataType(pDriver->drv.LinuxCdev.dataType);
        inf.numBuffers = pDriver->drv.LinuxCdev.numBuffers;
        inf.bufferSize = pDriver->drv.LinuxCdev.bufferSize;
        inf.subBufferSize = pDriver->drv.LinuxCdev.subBufferSize;
        inf.packetsPerXact = pDriver->drv.LinuxCdev.packetsPerXact;
        PrintDriverStructure(&inf);
        break;
    case Driver_LinuxV4l2:
        inf.aimName = STRING_DRIVER_CH_V4L2;
        inf.linkName = pDriver->linkName;
        inf.aimParam = pDriver->drv.LinuxV4l2.aimName;
        inf.channelName = pDriver->drv.LinuxV4l2.channelName;
        inf.direction = GetDirection(pDriver->drv.LinuxV4l2.direction);
        inf.dataType = GetDataType(pDriver->drv.LinuxV4l2.dataType);
        inf.numBuffers = pDriver->drv.LinuxV4l2.numBuffers;
        inf.bufferSize = pDriver->drv.LinuxV4l2.bufferSize;
        inf.subBufferSize = pDriver->drv.LinuxV4l2.subBufferSize;
        inf.packetsPerXact = pDriver->drv.LinuxV4l2.packetsPerXact;
        PrintDriverStructure(&inf);
        break;
    case Driver_LinuxAlsa:
        inf.aimName = STRING_DRIVER_CH_ALSA;
        inf.linkName = pDriver->linkName;
        inf.aimParam = pDriver->drv.LinuxAlsa.aimName;
        inf.channelName = pDriver->drv.LinuxAlsa.channelName;
        inf.direction = GetDirection(pDriver->drv.LinuxAlsa.direction);
        inf.dataType = GetDataType(pDriver->drv.LinuxAlsa.dataType);
        inf.numBuffers = pDriver->drv.LinuxAlsa.numBuffers;
        inf.bufferSize = pDriver->drv.LinuxAlsa.bufferSize;
        inf.subBufferSize = pDriver->drv.LinuxAlsa.subBufferSize;
        inf.packetsPerXact = pDriver->drv.LinuxAlsa.packetsPerXact;
        inf.amountOfChannels = pDriver->drv.LinuxAlsa.amountOfChannels;
        inf.resolutionInBit = pDriver->drv.LinuxAlsa.resolutionInBit;
        PrintDriverStructure(&inf);
        break;
    default:
        return;
    }
}

static void PrintDriverStructure(const DrvInfo_t *inf)
{
    if (inf->comment)
        ConsolePrintfStart(PRIO_HIGH, "	/* %s */\n", inf->comment);
    else
        ConsolePrintfStart(PRIO_HIGH, "	/* %s - %s - %s */\n", inf->linkName, inf->aimName, inf->channelName);
    ConsolePrintfContinue("	{\n");
    ConsolePrintfContinue("		.ch_name = \"%s\",\n", inf->channelName);
    ConsolePrintfContinue("		.cfg = {\n");
    ConsolePrintfContinue("			.direction = %s,\n", inf->direction);
    ConsolePrintfContinue("			.data_type = %s,\n", inf->dataType);
    ConsolePrintfContinue("			.num_buffers = %d,\n", inf->numBuffers);
    ConsolePrintfContinue("			.buffer_size = %d,\n", inf->bufferSize);
    if (inf->subBufferSize)
        ConsolePrintfContinue("			.subbuffer_size = %d,\n", inf->subBufferSize);
    if (inf->packetsPerXact)
        ConsolePrintfContinue("			.packets_per_xact = %d,\n", inf->packetsPerXact);
    ConsolePrintfContinue("		},\n");
    ConsolePrintfContinue("		.aim_name = \"%s\",\n", inf->aimName);
    if (inf->amountOfChannels && inf->resolutionInBit)
        ConsolePrintfContinue("		.aim_param = \"inic-%s.%dx%d\",\n", inf->aimParam, inf->amountOfChannels, inf->resolutionInBit);
    else
        ConsolePrintfContinue("		.aim_param = \"inic-%s\",\n", inf->aimParam);
    ConsolePrintfExit("	},\n");
}

static void PrintHeader(uint16_t nodeAddress)
{    
    ConsolePrintfStart(PRIO_HIGH, "/*\n");
    ConsolePrintfContinue(" * default_conf.c - Default configuration for Node 0x%X.\n", nodeAddress);
    ConsolePrintfContinue(" *\n");
    ConsolePrintfContinue(" * Generator: xml2struct for Linux %s\n", GetXml2StructVersion());
    ConsolePrintfContinue(" */\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("#include \"mostcore.h\"\n");
    ConsolePrintfContinue("#include <linux/module.h>\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("static struct most_config_probe config_probes[] = {\n");
    ConsolePrintfContinue("\n");
}

static void PrintFooter(uint16_t nodeAddress)
{
    ConsolePrintfStart(PRIO_HIGH, "        /* sentinel */\n");
    ConsolePrintfContinue("	{}\n");
    ConsolePrintfContinue("};\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("static struct most_config_set config_set = {\n");
    ConsolePrintfContinue("	.probes = config_probes\n");
    ConsolePrintfContinue("};\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("static int __init mod_init(void)\n");
    ConsolePrintfContinue("{\n");
    ConsolePrintfContinue("	most_register_config_set(&config_set);\n");
    ConsolePrintfContinue("	return 0;\n");
    ConsolePrintfContinue("}\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("static void __exit mod_exit(void)\n");
    ConsolePrintfContinue("{\n");
    ConsolePrintfContinue("	most_deregister_config_set(&config_set);\n");
    ConsolePrintfContinue("}\n");
    ConsolePrintfContinue("\n");
    ConsolePrintfContinue("module_init(mod_init);\n");
    ConsolePrintfContinue("module_exit(mod_exit);\n");
    ConsolePrintfContinue("MODULE_LICENSE(\"GPL\");\n");
    ConsolePrintfContinue("MODULE_AUTHOR(\"Generated by xml2struct for Linux %s\");\n", GetXml2StructVersion());
    ConsolePrintfExit("MODULE_DESCRIPTION(\"Default configuration for Node 0x%X\");\n", nodeAddress);
}

static const char *GetDirection(DriverCfgDirection_t dir)
{
    switch(dir)
    {
    case DriverCfgDirection_Tx:
        return STRING_DRIVER_CH_TX;
    case DriverCfgDirection_Rx:
        return STRING_DRIVER_CH_RX;
    default:
        break;
    }
    return "UNKNOWN";
}

static const char *GetDataType(DriverCfgDataType_t dtyp)
{
    switch(dtyp)
    {
    case DriverCfgDataType_Control:
        return STRING_DRIVER_CH_CONTROL;
    case DriverCfgDataType_Async:
        return STRING_DRIVER_CH_ASYNC;
    case DriverCfgDataType_Sync:
        return STRING_DRIVER_CH_SYNC;
    case DriverCfgDataType_Isoc:
        return STRING_DRIVER_CH_ISOC;
    default:
        break;
    }
    return "UNKNOWN";
}