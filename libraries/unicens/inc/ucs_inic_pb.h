/*------------------------------------------------------------------------------------------------*/
/* UNICENS V2.1.0-3564                                                                            */
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

/*!
 * \file
 * \brief Public header file of class CInic.
 */

#ifndef UCS_INIC_PB_H
#define UCS_INIC_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_ret_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Bitmask used for network status event "Network Change Event".
 *  \ingroup G_UCS_NET_TYPES
 */
#define UCS_NETWORK_EVENT_NCE                   0x0001U

/*! \brief   Signature version limit of EXC commands. Denotes the maximum signature version
 *           number the INIC can handle.
 *  \ingroup G_UCS_NET_TYPES
 */
#define UCS_EXC_SIGNATURE_VERSION_LIMIT         1U


/*! \brief No evaluable segment information available for BackChannel Diagnosis. 
 *  \ingroup G_UCS_BC_DIAG_TYPES
 */
#define UCS_BCD_DUMMY_SEGMENT                    0xFFU


/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Data Type which describes the last reset reason of the device.
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef enum Ucs_Inic_LastResetReason_
{
    /*! \brief Reset due to Power on reset or reset pin (!RST) is held low. */
    UCS_INIC_RST_STARTUP        = 0x00U,
    /*! \brief Reset due to hardware watchdog that had snapped */
    UCS_INIC_RST_HW_WATCHDOG    = 0x01U,
    /*! \brief Reset due to stack overflow */
    UCS_INIC_RST_STACK_OVERFLOW = 0x02U

} Ucs_Inic_LastResetReason_t;

/*! \brief   The current power state of the INICs power management interface (PS0/PS1). 
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef enum Ucs_Inic_PowerState_
{
    /*! \brief Power state "UNormal" */
    UCS_INIC_PWS_U_NORMAL       = 0x00U,
    /*! \brief Power state "ULow" */
    UCS_INIC_PWS_U_LOW          = 0x01U,
    /*! \brief Power State "STP" */
    UCS_INIC_PWS_STP            = 0x02U,
    /*! \brief Power State "UCritical" */
    UCS_INIC_PWS_U_CRITICAL     = 0x03U,
    /*! \brief No power state. Power management monitoring is disabled in INIC configuration string. */
    UCS_INIC_PWS_NO_MONITORING  = 0xFFU

} Ucs_Inic_PowerState_t;


/*! \brief Defines the flags set in \c change_mask used by NetworkStatus.Status 
 *  \ingroup G_UCS_NET_TYPES
 */

typedef enum Ucs_Network_StatusMask_
{
    UCS_NW_M_EVENTS           = 0x01U,    /*!< \brief Flag for notification of event changes */
    UCS_NW_M_AVAIL            = 0x02U,    /*!< \brief Flag for notification of availability changes */
    UCS_NW_M_AVAIL_INFO       = 0x04U,    /*!< \brief Flag for notification of availability info changes */
    UCS_NW_M_AVAIL_TR_CAUSE   = 0x08U,    /*!< \brief Flag for notification of availability transition cause changes */
    UCS_NW_M_NODE_ADDR        = 0x10U,    /*!< \brief Flag for notification of node address changes */
    UCS_NW_M_NODE_POS         = 0x20U,    /*!< \brief Flag for notification of node position changes */
    UCS_NW_M_MAX_POS          = 0x40U,    /*!< \brief Flag for notification of MPR changes */
    UCS_NW_M_PACKET_BW        = 0x80U     /*!< \brief Flag for notification of packet bandwidth changes */

} Ucs_Network_StatusMask_t;


/*! \brief   MOST Network Availability
 *  \ingroup G_UCS_NET_TYPES
 */
typedef enum Ucs_Network_Availability_
{
    UCS_NW_NOT_AVAILABLE = 0x00U,       /*!< \brief MOST network is not available */
    UCS_NW_AVAILABLE     = 0x01U        /*!< \brief MOST network is available  */

} Ucs_Network_Availability_t;

/*! \brief   MOST Network Availability Information.
 *  \details AvailabilityInfo is a sub state of Availability (\ref Ucs_Network_Availability_t)
 *           Possible pairs of Availability and Availability Information
 *  Availability            | Availability Information 
 *  ------------------------| ------------------------------------
 *  UCS_NW_NOT_AVAILABLE    | UCS_NW_AVAIL_INFO_REGULAR
 *  UCS_NW_NOT_AVAILABLE    | UCS_NW_AVAIL_INFO_DIAGNOSIS
 *  UCS_NW_NOT_AVAILABLE    | UCS_NW_AVAIL_INFO_FORCED_NA
 *  UCS_NW_AVAILABLE        | UCS_NW_AVAIL_INFO_UNSTABLE
 *  UCS_NW_AVAILABLE        | UCS_NW_AVAIL_INFO_STABLE
 *  \ingroup G_UCS_NET_TYPES
 */
typedef enum Ucs_Network_AvailInfo_
{
    /*! \brief The network is not available because it is in NetInterface Off or Init state. 
     *         It is pending to get available again. 
     */
    UCS_NW_AVAIL_INFO_REGULAR       = 0x00U,
    /*! \brief The network is not available because it performs a ring break diagnosis or
     *         physical layer test.
     */
    UCS_NW_AVAIL_INFO_DIAGNOSIS     = 0x02U,
    /*! \brief The INIC forces the network to stay in "not available" state. The  
     *         application may enter or leave this state by calling 
     *         Ucs_Network_ForceNotAvailable(). Also see Ucs_Network_Startup().
     */
    UCS_NW_AVAIL_INFO_FORCED_NA     = 0x06U,
    /*! \brief Network is available. Unlocks have been detected. */
    UCS_NW_AVAIL_INFO_UNSTABLE      = 0x10U,
    /*! \brief Network is available. Network is in Stable Lock. */
    UCS_NW_AVAIL_INFO_STABLE        = 0x11U

} Ucs_Network_AvailInfo_t;

/*! \brief MOST Network Availability Transition Cause
 *  \ingroup G_UCS_NET_TYPES
 */
typedef enum Ucs_Network_AvailTransCause_
{
    /*! \brief Start-up is initiated by chip e.g., INIC.MOSTNetworkStartup() */
    UCS_NW_AV_TR_CA_CMD             = 0x00U,
    /*! \brief Chip is woken up by network activity. */
    UCS_NW_AV_TR_CA_RX_ACTIVITY     = 0x01U,
    /*! \brief Network is typically shutdown by an INIC.MOSTNetworkShutdown() command initiated 
     *         locally or by a node positioned upstream (in the latter case, the shutdown flag 
     *         indicates a Normal Shutdown).
     */
    UCS_NW_AV_TR_CA_NORMAL          = 0x10U,
    /*! \brief Network is shutdown due to an error. In this case the shutdown reason was a sudden 
     *         signal off. No shutdown flag is present.
     */
    UCS_NW_AV_TR_CA_ERR_SSO         = 0x11U,
    /*! \brief Network is shutdown due to an error. In this case the shutdown reason was a critical 
     *         unlock. No shutdown flag is present.
     */
    UCS_NW_AV_TR_CA_ERR_CRIT_UNLOCK = 0x12U,
    /*! \brief Network is shutdown due to a chip or system error. Possible reasons are:
     *         - INIC enters ForcedNA state
     *         - The AutoShutdownDownDelay time expires after the EHC has detached.
     */
    UCS_NW_AV_TR_CA_ERR_SYSTEM      = 0x13U,
    /*! \brief No transition */
    UCS_NW_AV_TR_CA_NO_TRANSITION   = 0xFFU

} Ucs_Network_AvailTransCause_t;

/*! \brief Result values for the Ring Break Diagnosis
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_RbdResult_
{
    UCS_DIAG_RBD_NO_ERROR       = 0x00U,     /*!< \brief No error */
    UCS_DIAG_RBD_POS_DETECTED   = 0x01U,     /*!< \brief Position detected */
    UCS_DIAG_RBD_DIAG_FAILED    = 0x02U,     /*!< \brief Diagnosis failed */
    UCS_DIAG_RBD_POS_0_WEAK_SIG = 0x03U      /*!< \brief PosDetected = 0 and un-lockable signal on
                                              *          Rx was detected
                                              */
} Ucs_Diag_RbdResult_t;

/*! \brief Data packet size of the isochronous channel 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Avp_IsocPacketSize_
{
    UCS_ISOC_PCKT_SIZE_188 = 188U,  /*!< \brief Standard MPEG2 Transport Stream packet size, no
                                     *          encryption
                                     */ 
    UCS_ISOC_PCKT_SIZE_196 = 196U,  /*!< \brief DTCP Supplement B, DTCP over MOST */
    UCS_ISOC_PCKT_SIZE_206 = 206U   /*!< \brief DTCP Supplement E, DTCP over IP */

} Ucs_Avp_IsocPacketSize_t;

/*! \brief Mute Mode
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Sync_MuteMode_
{
    UCS_SYNC_MUTE_MODE_NO_MUTING   = 0x00U,  /*!< \brief No mute monitoring */
    UCS_SYNC_MUTE_MODE_MUTE_SIGNAL = 0x01U   /*!< \brief Mute signal. The MUTE pin will be asserted if any registered connection may stream 
                                              *          corrupted data.
                                              */

} Ucs_Sync_MuteMode_t;

/*! \brief Direction of the data stream from the perspective of the INIC 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_SocketDirection_
{
    UCS_SOCKET_DIR_INPUT  = 0U,     /*!< \brief Socket transfers data into INIC */
    UCS_SOCKET_DIR_OUTPUT = 1U      /*!< \brief Socket transfers data out of INIC */

} Ucs_SocketDirection_t;

/*! \brief Data type of MOST Sockets 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Most_SocketDataType_
{
    UCS_MOST_SCKT_SYNC_DATA        = 0U,    /*!< \brief Specifies the synchronous streaming data type */
    UCS_MOST_SCKT_AV_PACKETIZED    = 3U,    /*!< \brief Specifies the A/V Packetized Isochronous 
                                                        streaming data type*/
    UCS_MOST_SCKT_QOS_IP           = 4U,    /*!< \brief Specifies the Quality of Service IP 
                                                        streaming data type*/
    UCS_MOST_SCKT_DISC_FRAME_PHASE = 5U     /*!< \brief Specifies the DiscreteFrame Isochronous
                                                        streaming phase data type */
} Ucs_Most_SocketDataType_t;

/*! \brief Data type of MediaLB Sockets 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Mlb_SocketDataType_
{
    UCS_MLB_SCKT_SYNC_DATA        = 0U,     /*!< \brief Specifies the synchronous streaming data type */
    UCS_MLB_SCKT_CONTROL_DATA     = 2U,     /*!< \brief Specifies the control data type */
    UCS_MLB_SCKT_AV_PACKETIZED    = 3U,     /*!< \brief Specifies the A/V Packetized Isochronous 
                                                        streaming data type */
    UCS_MLB_SCKT_QOS_IP           = 4U,     /*!< \brief Specifies the Quality of Service IP 
                                                        streaming data type*/
    UCS_MLB_SCKT_DISC_FRAME_PHASE = 5U,     /*!< \brief Specifies the DiscreteFrame Isochronous
                                                        streaming phase data type */
    UCS_MLB_SCKT_IPC_PACKET       = 7U      /*!< \brief Specifies the IPC packet data type */

} Ucs_Mlb_SocketDataType_t;

/*! \brief Data type of USB Sockets 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Usb_SocketDataType_
{
    UCS_USB_SCKT_SYNC_DATA     = 0U,    /*!< \brief Specifies the synchronous streaming data type */
    UCS_USB_SCKT_CONTROL_DATA  = 2U,    /*!< \brief Specifies the control data type */
    UCS_USB_SCKT_AV_PACKETIZED = 3U,    /*!< \brief Specifies the A/V Packetized Isochronous 
                                                    streaming data type */
    UCS_USB_SCKT_IPC_PACKET    = 7U     /*!< \brief Specifies the IPC packet data type */

} Ucs_Usb_SocketDataType_t;

/*! \brief Physical interface of the USB Port
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Usb_PhysicalLayer_
{
    UCS_USB_PHY_LAYER_STANDARD = 0U,    /*!< \brief Standard - USB uses the standard physical 
                                         *          interface with analog transceivers for board
                                         *          communication
                                         */
    UCS_USB_PHY_LAYER_HSCI     = 1U     /*!< \brief HSIC - USB uses the High-Speed Inter-Chip 
                                         *          interface without analog transceivers for board 
                                         *          communication.
                                         */
} Ucs_Usb_PhysicalLayer_t;

/*! \brief MediaLB clock speed configuration 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Mlb_ClockConfig_
{
    UCS_MLB_CLK_CFG_256_FS  = 0U,       /*!< \brief 256 Fs */
    UCS_MLB_CLK_CFG_512_FS  = 1U,       /*!< \brief 512 Fs */
    UCS_MLB_CLK_CFG_1024_FS = 2U,       /*!< \brief 1024 Fs */
    UCS_MLB_CLK_CFG_2048_FS = 3U,       /*!< \brief 2048 Fs */
    UCS_MLB_CLK_CFG_3072_FS = 4U,       /*!< \brief 3072 Fs */
    UCS_MLB_CLK_CFG_4096_FS = 5U,       /*!< \brief 4096 Fs */
    UCS_MLB_CLK_CFG_6144_FS = 6U,       /*!< \brief 6144 Fs */
    UCS_MLB_CLK_CFG_8192_FS = 7U,       /*!< \brief 8192 Fs */
    UCS_MLB_CLK_CFG_WILDCARD = 0xFFU    /*!< \brief Uses the corresponding parameter in the INIC
                                                    Configuration String */
} Ucs_Mlb_ClockConfig_t;

/*! \brief Source of the RMCK clock 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Rmck_PortClockSource_
{
    UCS_RMCK_PORT_CLK_SRC_NW_SYSTEM = 0x01U /*!< \brief RMCK is locked to the system clock */

} Ucs_Rmck_PortClockSource_t;

/*! \brief Data type of PCIe Sockets 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Pci_SocketDataType_
{
    UCS_PCI_SCKT_AV_PACKETIZED = 3U     /*!< \brief Specifies the A/V Packetized Isochronous 
                                                    streaming data type */

} Ucs_Pci_SocketDataType_t;

/*! \brief Operation mode of the Streaming Port 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortOpMode_
{
    UCS_STREAM_PORT_OP_MODE_GENERIC = 0x00U /*!< \brief If Index = PortB, data pins are linked 
                                                        to PortA clock configuration. */

} Ucs_Stream_PortOpMode_t;

/*! \brief Direction of the physical pins of the indexed Streaming Port 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortOption_
{
    UCS_STREAM_PORT_OPT_IN_OUT   = 0x00U,    /*!< \brief Two serial interface pins are available;
                                                         one for direction IN and one for direction
                                                         OUT. */
    UCS_STREAM_PORT_OPT_DUAL_IN  = 0x01U,    /*!< \brief Tow serial interface pins are available
                                                         for direction IN. */
    UCS_STREAM_PORT_OPT_DUAL_OUT = 0x02U     /*!< \brief Tow serial interface pins are available
                                                         for direction OUT. */

} Ucs_Stream_PortOption_t;

/*! \brief Indicates if FSY/SCK signals are configured as outputs or inputs. 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortClockMode_
{
    /*! \brief INIC drives the FSY/SCK signals as outputs, frequency locked to the network clock. */
    UCS_STREAM_PORT_CLK_MODE_OUTPUT = 0x00U,
    /*! \brief FSY/SCK signals are configured as inputs and are driven from outside the INIC. Use 
     *         RMCK, frequency locked to the network clock, as reference for clock generation.
     */
    UCS_STREAM_PORT_CLK_MODE_INPUT  = 0x01U,
    /*! \brief Wildcard */
    UCS_STREAM_PORT_CLK_MODE_WILD   = 0xFFU

} Ucs_Stream_PortClockMode_t;

/*! \brief This setting is only applicable to data pins used for Generic Streaming including any 
           linked pins to Streaming Port B. All data pins share the same  FSY / SCK  signals, hence 
           this setting applies to all data pins. 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortClockDataDelay_
{
    /*! \brief Data is not delayed by a single SCK clock delay. */
    UCS_STREAM_PORT_CLK_DLY_NONE    = 0x00U,
    /*! \brief There is a single SCK clock delay between the start of frame (falling edge of FSY)
     *         and the start of the frame data on the data pins.
     */
    UCS_STREAM_PORT_CLK_DLY_DELAYED = 0x01U,
    /*! \brief Wildcard */
    UCS_STREAM_PORT_CLK_DLY_WILD    = 0xFFU

} Ucs_Stream_PortClockDataDelay_t;

/*! \brief Clock speed configuration of the  SCK  signal. 
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortClockConfig_
{
    /*! \brief 8 x Fs. All data pins must be configured for sequential routing. */
    UCS_STREAM_PORT_CLK_CFG_8FS   = 0x00U,
    /*! \brief 16 x Fs. All data pins must be configured for sequential routing. */
    UCS_STREAM_PORT_CLK_CFG_16FS  = 0x01U,
    /*! \brief 32 x Fs. All data pins must be configured for sequential routing. */
    UCS_STREAM_PORT_CLK_CFG_32FS  = 0x02U,
    /*! \brief 64 x Fs */
    UCS_STREAM_PORT_CLK_CFG_64FS  = 0x03U,
    /*! \brief 128 x Fs */
    UCS_STREAM_PORT_CLK_CFG_128FS = 0x04U,
    /*! \brief 256 x Fs */
    UCS_STREAM_PORT_CLK_CFG_256FS = 0x05U,
    /*! \brief 512 x Fs */
    UCS_STREAM_PORT_CLK_CFG_512FS = 0x06U,
    /*! \brief Wildcard */
    UCS_STREAM_PORT_CLK_CFG_WILD  = 0xFFU

} Ucs_Stream_PortClockConfig_t;

/*! \brief Data types of Streaming Sockets
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_SocketDataType_
{
    /*! \brief Specifies the synchronous streaming data type. */
    UCS_STREAM_PORT_SCKT_SYNC_DATA = 0x00U

} Ucs_Stream_SocketDataType_t;

/*! \brief ID of the serial interface pin of the addressed Streaming Port instance to which the 
           socket should be attached to.
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortPinId_
{
    UCS_STREAM_PORT_PIN_ID_SRXA0 = 0x00U,       /*!< \brief PortA, pin 5. */
    UCS_STREAM_PORT_PIN_ID_SRXA1 = 0x01U,       /*!< \brief PortA, pin 6. */
    UCS_STREAM_PORT_PIN_ID_SRXB0 = 0x10U,       /*!< \brief PortB, pin 7. */
    UCS_STREAM_PORT_PIN_ID_SRXB1 = 0x11U        /*!< \brief PortB, pin 8. */

} Ucs_Stream_PortPinId_t;

/*! \brief Defines the alignment of the data bytes within the streaming port frame.
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Stream_PortDataAlign_
{
    UCS_STREAM_PORT_ALGN_LEFT16BIT  = 0x00U,    /*!< \brief Left-justified, 16 bit, legacy */
    UCS_STREAM_PORT_ALGN_LEFT24BIT  = 0x01U,    /*!< \brief Left-justified, 24 bit, legacy */
    UCS_STREAM_PORT_ALGN_RIGHT16BIT = 0x02U,    /*!< \brief Right-justified, 16 bit, legacy */
    UCS_STREAM_PORT_ALGN_RIGHT24BIT = 0x03U,    /*!< \brief Right-justified, 16 bit, legacy */
    UCS_STREAM_PORT_ALGN_SEQ        = 0x04U     /*!< \brief Sequential */

} Ucs_Stream_PortDataAlign_t;

/*! \brief Indicates if the MOST Network Port is available and ready for streaming data connections.
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Most_PortAvail_
{
    /*! \brief MOST Network Port is available and it is possible to have streaming data 
     *         connections.
     */
    UCS_MOST_PORT_AVAIL     = 0x01U,
    /*! \brief MOST Network Port is not available for streaming data. FreeStreamingBW gets 0. 
     *         All created sockets on this port get invalid.
     */
    UCS_MOST_PORT_NOT_AVAIL = 0x00U

} Ucs_Most_PortAvail_t;

/*! \brief Indicates the sub state to parameter Available.
 *  \ingroup G_UCS_XRM_ENUM
 */
typedef enum Ucs_Most_PortAvailInfo_
{
    /*! \brief MOST Network Port is not available for streaming data. This is for instance the 
     *         case if the MOST network is shut down or Ring Break Diagnosis is running.
     */
    UCS_MOST_PRT_AVL_INF_REGULAR  = 0x00U,
    /*! \brief Unlocks have been detected at the port and streaming is temporarily not 
     *         available.
     */
    UCS_MOST_PRT_AVL_INF_UNSTABLE = 0x10U,
    /*! \brief Port is in Stable Lock. */
    UCS_MOST_PRT_AVL_INF_STABLE   = 0x11U

} Ucs_Most_PortAvailInfo_t;

/*! \brief Indicates the type of the Physical Layer Test.
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_PhyTest_Type_
{
    UCS_DIAG_PHYTEST_MASTER = 1U,   /*!< \brief Force Retimed Bypass TimingMaster mode  */
    UCS_DIAG_PHYTEST_SLAVE  = 2U    /*!< \brief Force Retimed Bypass TimingSlave mode  */

} Ucs_Diag_PhyTest_Type_t;


/*! \brief Specifies whether the the INIC behaves as a TimingMaster or TimingSlave device
 *         during the Ring Break Diagnosis (RBD).
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_RbdType_
{
    UCS_DIAG_RBDTYPE_SLAVE  = 0U,   /*!< \brief The INIC starts the RBD as a TimingSlave */
    UCS_DIAG_RBDTYPE_MASTER = 1U    /*!< \brief The INIC starts the RBD as a TimingMaster */

} Ucs_Diag_RbdType_t;

/*! \brief The speed grade of the I2C Port. 
 *  \ingroup G_UCS_I2C_TYPES
 */
typedef enum Ucs_I2c_Speed_
{
    UCS_I2C_SLOW_MODE      = 0x00U,      /*!< \brief Speed grade of the port is 100 kHz. */
    UCS_I2C_FAST_MODE      = 0x01U       /*!< \brief Speed grade of the port is 400 kHz. */

} Ucs_I2c_Speed_t;

/*! \brief The write transfer mode. 
 *  \ingroup G_UCS_I2C_TYPES
 */
typedef enum Ucs_I2c_TrMode_
{
    UCS_I2C_DEFAULT_MODE      = 0x00U,      /*!< \brief Default mode of the I2C write transfer */
    UCS_I2C_REPEATED_MODE     = 0x01U,      /*!< \brief Repeated Mode of the I2C write transfer */
    UCS_I2C_BURST_MODE        = 0x02U       /*!< \brief Burst mode of the I2C write transfer */

} Ucs_I2c_TrMode_t;

/*! \brief The mode of the GPIO pin. 
 *  \ingroup G_UCS_GPIO_TYPES
 */
typedef enum Ucs_Gpio_PinMode_
{
    UCS_GPIO_UNAVAILABLE        = 0x00U,   /*!< \brief Unavailable Mode */
    UCS_GPIO_UNUSED             = 0x01U,   /*!< \brief Unused Mode */   
    UCS_GPIO_INPUT              = 0x10U,   /*!< \brief Input Mode */
    UCS_GPIO_IN_STICKY_HL       = 0x11U,   /*!< \brief InputStickyHighLevel Mode */
    UCS_GPIO_IN_STICKY_LL       = 0x12U,   /*!< \brief InputStickyLowLevel Mode */
    UCS_GPIO_IN_TRIGGER_RE      = 0x13U,   /*!< \brief InputTriggerRisingEdge Mode */
    UCS_GPIO_IN_TRIGGER_FE      = 0x14U,   /*!< \brief InputTriggerFallingEdge Mode */
    UCS_GPIO_IN_TRIGGER_HL      = 0x16U,   /*!< \brief InputTriggerHighLevel Mode */
    UCS_GPIO_IN_TRIGGER_LL      = 0x17U,   /*!< \brief InputTriggerLowLevel Mode */
    UCS_GPIO_IN_DEBOUNCED       = 0x30U,   /*!< \brief InputDebounced Mode */
    UCS_GPIO_IN_DB_TRIGGER_RE   = 0x33U,   /*!< \brief InputDebouncedTriggerRisingEdge Mode */
    UCS_GPIO_IN_DB_TRIGGER_FE   = 0x34U,   /*!< \brief InputDebouncedTriggerFallingEdge Mode */
    UCS_GPIO_IN_DB_TRIGGER_HL   = 0x36U,   /*!< \brief InputDebouncedTriggerHighLevel Mode */
    UCS_GPIO_IN_DB_TRIGGER_LL   = 0x37U,   /*!< \brief InputDebouncedTriggerLowLevel Mode */
    UCS_GPIO_OUT_DEFAULT_LOW    = 0x40U,   /*!< \brief OutputDefaultLow Mode */
    UCS_GPIO_OUT_DEFAULT_HIGH   = 0x41U,   /*!< \brief OutputDefaultHigh Mode */
    UCS_GPIO_OUT_OPEN_DRAIN     = 0x50U,   /*!< \brief OutputOpenDrain Mode */
    UCS_GPIO_OUT_OD_TRIGGER_RE  = 0x53U,   /*!< \brief OutputOpenDrainTriggerRisingEdge Mode */
    UCS_GPIO_OUT_OD_TRIGGER_FE  = 0x54U,   /*!< \brief OutputOpenDrainTriggerFallingEdge Mode */
    UCS_GPIO_OUT_OD_TRIGGER_HL  = 0x56U,   /*!< \brief OutputOpenDrainTriggerHighLevel Mode */
    UCS_GPIO_OUT_OD_TRIGGER_LL  = 0x57U    /*!< \brief OutputOpenDrainTriggerLowLevel Mode */

} Ucs_Gpio_PinMode_t;

/*! \brief Type of System Diagnosis Report. 
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef enum Ucs_Sd_ResCode_
{
    UCS_SD_TARGET_FOUND         = 0x01U,    /*!< \brief Segment description */
    UCS_SD_FINISHED             = 0x02U,    /*!< \brief System Diagnosis finished */
    UCS_SD_CABLE_LINK_RES       = 0x03U,    /*!< \brief Cable Link Diagnosis was executed. */
    UCS_SD_ABORTED              = 0x04U,    /*!< \brief System Diagnosis stopped by application command */
    UCS_SD_ERROR                = 0x05U     /*!< \brief System Diagnosis detected unexpected error */

} Ucs_Sd_ResCode_t;

/*! \brief Type of System Diagnosis Error Codes. 
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef enum Ucs_Sd_ErrCode_
{
    /*! \brief An internal error occurred during System Diagnosis. */ 
    UCS_SD_ERR_UNSPECIFIED          = 0x01U,    
    /*! \brief INIC answered with "NoSuccess" to a Welcome.StartResult command. */
    UCS_SD_ERR_WELCOME_NO_SUCCESS   = 0x02U,    
    /*! \brief Stopping the System Diagnosis mode on INIC failed. The INIC may remain in System 
                Diagnosis mode. */
    UCS_SD_ERR_STOP_SYSDIAG_FAILED  = 0x03U,    
    /*! \brief System Diagnosis stopped due to a severe error. The INIC may remain in System 
                Diagnosis mode. */
    UCS_SD_ERR_TERMINATED           = 0x04U     

} Ucs_Sd_ErrCode_t;



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   This structure contains information on the hardware and firmware modules of the INIC.
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef struct Ucs_Inic_Version_
{
    uint32_t product_identifier; /*!< \brief Unique identifier that represents the product name.\mns_name_inic{ProductIdentifier} */
    uint32_t build_version;      /*!< \brief Firmware build version number.\mns_name_inic{BuildVersion}  */
    uint8_t  major_version;      /*!< \brief Firmware major version number.\mns_name_inic{MajorVersion} */
    uint8_t  minor_version;      /*!< \brief Firmware build version number.\mns_name_inic{MinorVersion} */
    uint8_t  release_version;    /*!< \brief Firmware release version number.\mns_name_inic{ReleaseVersion} */
    uint8_t  hw_revision;        /*!< \brief Chip revision number.\mns_name_inic{HardwareRevision} */
    uint16_t diagnosis_id;       /*!< \brief Diagnosis identifier of the INIC.\mns_name_inic{DiagnosisID} */
    uint8_t  cs_major_version;   /*!< \brief Configuration String major version number.\mns_name_inic{ExtMajorVersion} */
    uint8_t  cs_minor_version;   /*!< \brief Configuration String minor version number.\mns_name_inic{ExtMinorVersion} */
    uint8_t  cs_release_version; /*!< \brief Configuration String release version number.\mns_name_inic{ExtReleaseVersion} */

} Ucs_Inic_Version_t;

/*! \brief   This structure contains information on the GPIO pin configuration.
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef struct Ucs_Gpio_PinConfiguration_
{
    uint8_t pin;                /*!< \brief The GPIO pin that is to be configured  */
    Ucs_Gpio_PinMode_t mode;    /*!< \brief The mode of the GPIO pin  */

} Ucs_Gpio_PinConfiguration_t;






/*! \brief   This structure holds the signature of the Hello, Welcome and Signature messages. 
 *           It supports the signature v1 only.
 *  \ingroup G_INIC_TYPES
 */
typedef struct Ucs_Signature_t_
{
    uint16_t node_address;      /*!< \brief NodeAddress */
    uint16_t group_address;     /*!< \brief GroupAddress */
    uint16_t mac_47_32;         /*!< \brief MACAddress_47_32 */
    uint16_t mac_31_16;         /*!< \brief MACAddress_31_16 */    
    uint16_t mac_15_0;          /*!< \brief MACAddress_15_0 */
    uint16_t node_pos_addr;     /*!< \brief NodePositionAddress */
    uint16_t diagnosis_id;      /*!< \brief DiagnosisID */
    uint8_t  num_ports;         /*!< \brief NumberOfPorts */
    uint8_t  chip_id;           /*!< \brief ChipID */
    uint8_t  fw_major;          /*!< \brief FWVersion_Major */
    uint8_t  fw_minor;          /*!< \brief FWVersion_Minor */
    uint8_t  fw_release;        /*!< \brief FWVersion_Release */
    uint32_t fw_build;          /*!< \brief FWVersion_Build */
    uint8_t  cs_major;          /*!< \brief CSVersion_Major */
    uint8_t  cs_minor;          /*!< \brief CSVersion_Minor */
    uint8_t  cs_release;        /*!< \brief CSVersion_Release */
/*    uint8_t  uid_persistency;*/   /*!< \brief UIDPersistency */
/*    uint32_t uid;*/               /*!< \brief UID */

} Ucs_Signature_t;              

/*------------------------------------------------------------------------------------------------*/
/*  System Diagnosis                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   This structure holds the segment information of the system diagnosis 
 *  \ingroup G_INIC_TYPES
 */
typedef struct Ucs_Sd_Segment_t_
{
    uint8_t branch;             /*!< \brief Number of the currently tested branch. Numbering starts 
                                            with 0 and corresponds to the port number if the Timing 
                                            Master is a multi port INIC */
    uint8_t num;                /*!< \brief Segment number inside the tested branch. Numbering starts with 1 */
    Ucs_Signature_t source;     /*!< \brief Signature of the first node of the segment 
                                            \mns_param_inic{Signature,Hello,MNSH2-Hello200} */
    Ucs_Signature_t target;     /*!< \brief Signature of the second node of the segment  
                                            \mns_param_exc{Signature,Hello,MNSH2-Hello200} */
} Ucs_Sd_Segment_t;


/*! \brief   This structure holds the results of the system diagnosis 
 *  \ingroup G_INIC_TYPES
 */
typedef struct Ucs_Sd_Report_t_
{
    Ucs_Sd_ResCode_t code;      /*!< \brief Result code */
    Ucs_Sd_Segment_t segment;   /*!< \brief Information about tested segment */
    uint8_t cable_link_info;    /*!< \brief Result of a cable link diagnosis. 
                                            \mns_param_exc{Result,CableLinkDiagnosis,MNSH2-CableLinkDiagnosis211} */
    Ucs_Sd_ErrCode_t err_info;  /*!< \brief Error codes, values are defined in Ucs_Sd_ErrCode_t */

} Ucs_Sd_Report_t;


/*------------------------------------------------------------------------------------------------*/
/*  Node Discovery                                                                                */
/*------------------------------------------------------------------------------------------------*/


/*! \brief Result values of the Node Discovery service. 
 *  \ingroup G_UCS_NODE_DISCOVERY_TYPES
 */
typedef enum Ucs_Nd_ResCode_t_
{
    UCS_ND_RES_WELCOME_SUCCESS   = 0x01U,    /*!< \brief Node was successfully added to the network. */
    UCS_ND_RES_UNKNOWN           = 0x02U,    /*!< \brief Node signature is unknown to the application, node will be ignored. */
    UCS_ND_RES_MULTI             = 0x03U,    /*!< \brief A node with the same signature is already part of the system. The new node will be ignored. */
    UCS_ND_RES_STOPPED           = 0x04U,    /*!< \brief The Node Discovery service was stopped by API function Ucs_Nd_Stop(). Ucs_Nd_Start() has to be called to start again. */
    UCS_ND_RES_NETOFF            = 0x05U,    /*!< \brief The Node Discovery service detected a NetOff event and pauses . It resumes automatically as soon as NetOn occurs. */
    UCS_ND_RES_ERROR             = 0x06U     /*!< \brief An unexpected error occurred. Node Discovery service was stopped. Ucs_Nd_Start() has to be called to start again. */

} Ucs_Nd_ResCode_t;

/*! \brief   Result values of the application's evaluation function (type \ref Ucs_Nd_EvalCb_t).
 *  \ingroup G_UCS_NODE_DISCOVERY_TYPES
 */
typedef enum Ucs_Nd_CheckResult_t_
{
    UCS_ND_CHK_WELCOME              = 0x01U,    /*!< \brief Node is ok, try to add it to the network. */
    UCS_ND_CHK_UNIQUE               = 0x02U,    /*!< \brief Test if this node is unique. */
    UCS_ND_CHK_UNKNOWN              = 0x03U     /*!< \brief The node is unknown, no further action. */

} Ucs_Nd_CheckResult_t;


/*! \brief Function signature of node evaluation callback used by Node Discovery service.
 *
 *  The Node Discovery service announces the signature of each node it has found to the 
 *  application via the evaluation function. In this function the application 
 *  decides how the Node Discovery service shall proceed with the node.
 *  The application maintains two lists:  
 * 
 *  <dl> 
 *      <dt> *set_list* </dt>
 *      <dd> Contains the signatures of the nodes the system shall contain
 *  
 *      <dt> *device_list* </dt>
 *      <dd> Contains the signatures of the nodes detected in the system
 *  </dl>
 *
 *  The evaluation has to follow these rules: 
 *  - If the node is not part of the *set_list*, it is regarded as unknown (\ref UCS_ND_CHK_UNKNOWN)
 *    and will be ignored. 
 *  - If the node is part of the *set_list* and is not yet in the *device_list*, the Node Discovery 
 *    Service shall try to add the node to network (\ref UCS_ND_CHK_WELCOME). 
 *  - If the node is already part of the *device_list*, there are two possibilities: the node in the 
 *    *device_list* experienced a reset or there are two nodes with the same signature. Evaluation 
 *    result is \ref UCS_ND_CHK_UNIQUE. The Node Discovery service will perform further tests.
 *
 *  \param    signature   Signature of the respective node
 *  \param    user_ptr    User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \returns  UCS_ND_CHK_WELCOME  Node is ok, try to add it to the network.
 *  \returns  UCS_ND_CHK_UNIQUE   Test if this node is unique.
 *  \returns  UCS_ND_CHK_UNKNOWN  Node is unknown, no further action.
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef Ucs_Nd_CheckResult_t (*Ucs_Nd_EvalCb_t)(Ucs_Signature_t *signature, void *user_ptr);

/*! \brief Function signature of result callback used by Node Discovery service.
 *
 *  The Node Discovery service reports the result of each node and some system events by
 *  this callback function.
 *  
 *  \note The parameter <b>signature</b> will be NULL, if parameter <b>code</b> is 
 *  \ref UCS_ND_RES_STOPPED, \ref UCS_ND_RES_NETOFF or \ref UCS_ND_RES_ERROR.
 *
 *  \param   code         Result code 
 *  \param   signature    Signature of the respective node
 *  \param   user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
typedef void (*Ucs_Nd_ReportCb_t)(Ucs_Nd_ResCode_t code, 
                                  Ucs_Signature_t *signature,
                                  void *user_ptr);


/*------------------------------------------------------------------------------------------------*/
/*  Programming service                                                                              */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Defines the set of MemIDs and the memory access types. 
 *  \ingroup G_UCS_PROG_MODE_TYPES
 */
typedef enum Ucs_Prg_SessionType_ 
{
    UCS_PRG_ST_CS          = 0x01U,    /*!< \brief Writes to configuration string */
    UCS_PRG_ST_IS          = 0x02U,    /*!< \brief Writes to identification string */
    UCS_PRG_ST_CS_IS       = 0x04U,    /*!< \brief Writes to configuration and identification string  */
    UCS_PRG_ST_ERASE_EM    = 0x08U,    /*!< \brief Erases the error memory */
    UCS_PRG_ST_CFG_READ    = 0x10U     /*!< \brief Reads data from all configuration memories */
} Ucs_Prg_SessionType_t;


/*! \brief Represents the memory resource to be written. 
 *  \ingroup G_UCS_PROG_MODE_TYPES
 */
typedef enum Ucs_Prg_MemId_ 
{
    UCS_PRG_MID_CS        = 0x00U, /*!< \brief Writes the configuration string */
    UCS_PRG_MID_IS        = 0x01U, /*!< \brief Writes the identification string */
    UCS_PRG_MID_CSTEST    = 0x0CU, /*!< \brief Writes the test configuration string */
    UCS_PRG_MID_ISTEST    = 0x0DU  /*!< \brief Writes the test identification string */
} Ucs_Prg_MemId_t;

/*! \brief Represents a programming task. 
 *  \ingroup G_UCS_PROG_MODE_TYPES
 */
typedef struct Ucs_Prg_Command_
{ 
    Ucs_Prg_MemId_t mem_id;         /*!< \brief Represents the memory resource to be written. */
    uint32_t        address;        /*!< \brief Defines the memory location at which the writing 
                                                operation starts. */
    uint8_t         unit_length;    /*!< \brief Sets the number of memory units to be written. 
                                                Memory units can be unsigned bytes, unsigned words 
                                                or unsigned masked data depending on the memory type. */
    uint8_t         data_length;    /*!< \brief Lenght of data */
    uint8_t        *data;           /*!< \brief Contains the actual data written to the memory 
                                                resource and formatted as memory units. */
} Ucs_Prg_Command_t;

/*! \brief Result values of the Programming service. 
 *  \ingroup G_UCS_PROG_MODE_TYPES
 */
typedef enum Ucs_Prg_ResCode_t_
{
    UCS_PRG_RES_SUCCESS     = 0x01U,    /*!< \brief Node was successfully programmed. */
    UCS_PRG_RES_TIMEOUT     = 0x02U,    /*!< \brief Node did not answer timely. */
    UCS_PRG_RES_NET_OFF     = 0x03U,    /*!< \brief A NetOff event occurred during programming. */
    UCS_PRG_RES_FKT_SYNCH   = 0x04U,    /*!< \brief The call of the internal API function returned an error, 
                                                    so the command was not sent to the node.*/
    UCS_PRG_RES_FKT_ASYNCH  = 0x05U,    /*!< \brief Node returned an error message as result. */
    UCS_PRG_RES_ERROR       = 0x06U     /*!< \brief An unexcpected error occurred. Programming service was stopped. */

} Ucs_Prg_ResCode_t;

/*! \brief Denotes the function where an error occurred.
 *  \ingroup G_UCS_PROG_MODE_TYPES
 */
typedef enum Ucs_Prg_Func_t_
{
    UCS_PRG_FKT_DUMMY               = 0x00U,    /*!< \brief Dummy value, used in case of UCS_PRG_RES_SUCCESS */
    UCS_PRG_FKT_WELCOME             = 0x01U,    /*!< \brief Error occurred in the context of function Welcome */
    UCS_PRG_FKT_WELCOME_NOSUCCESS   = 0x02U,    /*!< \brief Welcome result was No Success */
    UCS_PRG_FKT_MEM_OPEN            = 0x03U,    /*!< \brief Error occurred in the context of function MemorySessionOpen */
    UCS_PRG_FKT_MEM_WRITE           = 0x04U,    /*!< \brief Error occurred in the context of function MemoryWrite */
    UCS_PRG_FKT_MEM_CLOSE           = 0x05U,    /*!< \brief Error occurred in the context of function MemorySessionClose */
    UCS_PRG_FKT_INIT                = 0x06U     /*!< \brief Error occurred in the context of function Init */
} Ucs_Prg_Func_t;

/*! \brief Function signature of result callback used by Programming service.
 *
 *  The Programming service reports the result of programming a certain device by
 *  this callback function.
 *  
 *
 *  \param   code         Result values of the Programming service 
 *  \param   function     Signature of the node to be programmed.
 *  \param   ret_len      Length of the error parameter field parm. It is 0 if no error occurred. 
 *  \param   parm         Pointer to the parameters of a potential error message.
 *  \param   user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_PROG_MODE
 */
typedef void (*Ucs_Prg_ReportCb_t)(Ucs_Prg_ResCode_t code, 
                                   Ucs_Prg_Func_t function,
                                   uint8_t ret_len,
                                   uint8_t parm[],
                                   void *user_ptr);


/*------------------------------------------------------------------------------------------------*/
/*  BackChannel Diagnosis                                                                         */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Result values of the BackChannel Diagnosis. 
 *  \ingroup G_UCS_BC_DIAG_TYPES
 */
typedef enum Ucs_Bcd_ResCode_t_
{
    UCS_BCD_RES_SUCCESS        = 0x01U,    /*!< \brief current segment is not broken */
    UCS_BCD_RES_NO_RING_BREAK  = 0x02U,    /*!< \brief TM answered: no ring break. */
    UCS_BCD_RES_RING_BREAK     = 0x03U,    /*!< \brief Ring break detected in current segment. */
    UCS_BCD_RES_TIMEOUT1       = 0x04U,    /*!< \brief No communication on back channel. */
    UCS_BCD_RES_TIMEOUT2       = 0x05U,    /*!< \brief No result from INIC received. */
    UCS_BCD_RES_ERROR          = 0x06U,    /*!< \brief An unexpected error occurred. BackChannel Diagnosis was stopped. */
    UCS_BCD_RES_END            = 0x07U     /*!< \brief BackChannel Diagnosis ended regularly. */  
} Ucs_Bcd_ResCode_t;


/*! \brief Function signature of result callback used by BackChannel Diagnosis.
 *
 *  The BackChannel Diagnosis reports the result of certain segment by
 *  this callback function.
 *  
 *  \param   code           Result code 
 *  \param   segment        Number of the segment which was inspected. Numbering starts with 0 denoting the segment following the TimingMaster. The number is increased for each following segment.
 *  \param   user_ptr       User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_BC_DIAG
 */
typedef void (*Ucs_Bcd_ReportCb_t)(Ucs_Bcd_ResCode_t code, 
                                   uint8_t segment,
                                   void *user_ptr);



/*------------------------------------------------------------------------------------------------*/
/*  Network functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature of result callback used by Ucs_Network_GetFrameCounter().
 *  \mns_res_inic{MOSTNetworkFrameCounter,MNSH3-MOSTNetworkFrameCounter523}
 *  \mns_ic_manual{ See also <i>User Manual</i>, section \ref P_UM_SYNC_AND_ASYNC_RESULTS. }
 *  \param frame_counter    The MOST network frame count.\mns_name_inic{FrameCounter}
 *  \param reference        Reference value that was passed to Mns_Network_GetFrameCounter().\mns_name_inic{Reference}
 *  \param lock             Indicates if the TimingSlave device is locked to the MOST network. For a 
                            TimingMaster device this value is always True.
 *  \param result           Returned result of the operation
 *  \param user_pter     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_NET
 */
typedef void (*Ucs_Network_FrameCounterCb_t)(uint32_t reference,
                                             uint32_t frame_counter,
                                             bool     frame_lock,
                                             Ucs_StdResult_t result,
                                             void * user_pointer);





#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_INIC_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

