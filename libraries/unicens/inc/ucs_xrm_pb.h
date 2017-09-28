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
 * \brief Public header file of the Extended Resource Manager.
 */

#ifndef UCS_XRM_PB_H
#define UCS_XRM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_xrm_cfg.h"
#include "ucs_ret_pb.h"
#include "ucs_inic_pb.h"
#include "ucs_rsm_pv.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \def UCS_XRM_CONST
 *  \brief      Defines a ROM data qualifier for XRM data objects.
 *  \details    This macro is used to define a XRM ROM data qualifier. It is used for XRM 
 *              configuration structures and XRM resource object lists. The definition of this 
 *              macro must be part of the XRM configuration file ucs_xrm_cfg.h.
 */
#ifdef UCS_XRM_CONST
#    error UCS_XRM_CONST macro is not supported on UNICENS anymore. Please remove It from your config file.
#endif

#define UCS_XRM_CONST

/*! \def UCS_XRM_NUM_JOBS
 *  \brief      Defines the size of the internal job list.
 *  \details    The number of XRM jobs to execute. The value is used to specify the size of the
 *              internal job list. The definition of this macro must be part of the XRM 
 *              configuration file ucs_xrm_cfg.h.
 *              Valid range: 1..254. Default value: 1.
 *  \ingroup    G_UCS_XRM_CFG
 */
#ifndef UCS_XRM_NUM_JOBS
#   define UCS_XRM_NUM_JOBS
#   define XRM_NUM_JOBS             1U 
#else
#   define XRM_NUM_JOBS             ((uint8_t)UCS_XRM_NUM_JOBS)
#endif

/*! \def UCS_XRM_NUM_RESOURCES
 *  \brief      Defines the number of provided resources.
 *  \details    The number of required resources depends on the defined XRM jobs. The 
 *              definition of the public macro UCS_XRM_NUM_RESOURCES must be part of the 
 *              XRM configuration file ucs_xrm_cfg.h.
 *              Valid range: 1..254. Default value: 8.
 *  \ingroup    G_UCS_XRM_CFG
 */
#ifndef UCS_XRM_NUM_RESOURCES
#   define UCS_XRM_NUM_RESOURCES
#   define XRM_NUM_RESOURCE_HANDLES         8U 
#else
#   define XRM_NUM_RESOURCE_HANDLES         ((uint8_t)UCS_XRM_NUM_RESOURCES)
#endif

/*!
 * \addtogroup G_UCS_XRM_TYPES
 * @{
 */
/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Result codes of the Extended Resource Manager. */
typedef enum Ucs_Xrm_ResultCode_
{
    UCS_XRM_RES_SUCCESS_BUILD     = 0x00U,      /*!< \brief Build of connection succeeded. */
    UCS_XRM_RES_SUCCESS_DESTROY   = 0x01U,      /*!< \brief Destruction of connection succeeded. */
    UCS_XRM_RES_RC_AUTO_DESTROYED = 0x02U,      /*!< \brief Invalid resources have been successfully destroyed. */
    UCS_XRM_RES_ERR_CONFIG        = 0x03U,      /*!< \brief Invalid user settings in the XRM configuration file.
                                                 *   \details The number of jobs or resources exceeds the maximum allowed in your XRM configuration file. */
    UCS_XRM_RES_ERR_BUILD         = 0x04U,      /*!< \brief Build of connection failed due to a function-specific error found on target device or a transmission error on the MOST network.
                                                 *   \details The \em result_type section in Ucs_Xrm_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                 */
    UCS_XRM_RES_ERR_DESTROY       = 0x05U,      /*!< \brief Destruction of connection failed due to a function-specific error found on target device or a transmission error on the MOST network.
                                                 *   \details The \em result_type section in Ucs_Xrm_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                 */
    UCS_XRM_RES_ERR_INV_LIST      = 0x06U,      /*!< \brief Request of invalid resources failed due to a function-specific error found on target device or a transmission error on the MOST network.
                                                 *   \details The \em result_type section in Ucs_Xrm_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                 */
    UCS_XRM_RES_ERR_SYNC          = 0x07U,      /*!< \brief The remote synchronization of target device failed due to a function-specific error a transmission error on the MOST network.
                                                 *   \details The \em result_type section in Ucs_Xrm_ResultDetails_t will provide you with more detailed information concerning the error type.
                                                 */
    UCS_XRM_RES_UNKNOWN           = 0xFFU       /*!< \brief Result is unknown. */
} Ucs_Xrm_ResultCode_t;

/*! \brief INIC resource types used by the Extended Resource Manager. */
typedef enum Ucs_Xrm_ResourceType_
{
    UCS_XRM_RC_TYPE_DC_PORT      = 0x00U,       /*!< \brief Default created port */
    UCS_XRM_RC_TYPE_MOST_SOCKET  = 0x01U,       /*!< \brief MOST socket */
    UCS_XRM_RC_TYPE_MLB_PORT     = 0x02U,       /*!< \brief MediaLB port */
    UCS_XRM_RC_TYPE_MLB_SOCKET   = 0x03U,       /*!< \brief MediaLB socket */
    UCS_XRM_RC_TYPE_USB_PORT     = 0x04U,       /*!< \brief USB port */
    UCS_XRM_RC_TYPE_USB_SOCKET   = 0x05U,       /*!< \brief USB socket */
    UCS_XRM_RC_TYPE_RMCK_PORT    = 0x06U,       /*!< \brief RMCK port */
    UCS_XRM_RC_TYPE_STRM_PORT    = 0x07U,       /*!< \brief Streaming port */
    UCS_XRM_RC_TYPE_STRM_SOCKET  = 0x08U,       /*!< \brief Streaming socket */
    UCS_XRM_RC_TYPE_SYNC_CON     = 0x09U,       /*!< \brief Synchronous data connection */
    UCS_XRM_RC_TYPE_DFIPHASE_CON = 0x0AU,       /*!< \brief DiscreteFrame Isochronous streaming 
                                                 *          phase connection
                                                 */
    UCS_XRM_RC_TYPE_COMBINER     = 0x0BU,       /*!< \brief Combiner */
    UCS_XRM_RC_TYPE_SPLITTER     = 0x0CU,       /*!< \brief Splitter */
    UCS_XRM_RC_TYPE_AVP_CON      = 0x0DU,       /*!< \brief A/V packetized isochronous streaming
                                                 *          data connection
                                                 */
    UCS_XRM_RC_TYPE_QOS_CON      = 0x0EU        /*!< \brief Quality of Service IP streaming data
                                                 *          connection
                                                 */

} Ucs_Xrm_ResourceType_t;

/*! \brief Port types use for default created ports. */
typedef enum Ucs_Xrm_PortType_
{
    UCS_XRM_PORT_TYPE_MLB          = 0x0AU,     /*!< \brief MediaLB Port */
    UCS_XRM_PORT_TYPE_USB          = 0x12U,     /*!< \brief USB Port */
    UCS_XRM_PORT_TYPE_STRM         = 0x16U      /*!< \brief Streaming Port */

} Ucs_Xrm_PortType_t;

/*! \brief This enumerator specifies the kind of result - Internal, Target or Transmission. */
typedef enum Ucs_Xrm_ResultType_
{
    UCS_XRM_RESULT_TYPE_INT        = 0x00U,     /*!< \brief Specifies the internal results, typically standard return codes of MNS used for synchronous response. */
    UCS_XRM_RESULT_TYPE_TGT        = 0x01U,     /*!< \brief Specifies the target results, typically INIC function-specific error from target device. */
    UCS_XRM_RESULT_TYPE_TX         = 0x02U      /*!< \brief Specifies the transmission error information on the MOST network. */

} Ucs_Xrm_ResultType_t;

/*! \brief This enumerator specifies the type of resources information */
typedef enum Ucs_Xrm_ResourceInfos_
{
    UCS_XRM_INFOS_BUILT            = 0x00U,   /*!< \brief Signals that the resource is built */
    UCS_XRM_INFOS_DESTROYED        = 0x01U,   /*!< \brief Signals that the resource is destroyed */
    UCS_XRM_INFOS_ERR_BUILT        = 0x02U,   /*!< \brief Signals that XRM fails to build the resource */
    UCS_XRM_INFOS_ERR_DESTROYED    = 0x03U    /*!< \brief Signals that XRM fails to destroy the resource */

} Ucs_Xrm_ResourceInfos_t;


/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief INIC Resource Object used for Extended Resource Manager jobs. 
 *  \attention This resource object must be \b terminated with a \b NULL \b pointer to mark the end of the list. 
 */
typedef void Ucs_Xrm_ResObject_t;

/*! \brief Function signature of result callback used by Ucs_Xrm_Stream_SetPortConfig() and 
 *         Ucs_Xrm_Stream_GetPortConfig().
 *  \mns_res_inic{StreamPortConfiguration,MNSH3-StreamPortConfiguration680}
 *  \param node_address         The node address from which the results come 
 *  \param index                Streaming Port instance. \mns_name_inic{Index}
 *  \param op_mode              Operation mode of the Streaming Port. \mns_name_inic{OperationMode}
 *  \param port_option          Direction of the physical pins of the indexed Streaming Port. \mns_name_inic{PortOption}
 *  \param clock_mode           Configuration of the FSY/SCK signals. \mns_name_inic{ClockMode}
 *  \param clock_data_delay     Configuration of the FSY/SCK signals for Generic Streaming. \mns_name_inic{ClockDataDelay}
 *  \param result               Returned result of the operation
 *  \param user_ptr             User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Xrm_Stream_PortCfgResCb_t)(uint16_t node_address,
                                              uint8_t index,
                                              Ucs_Stream_PortOpMode_t op_mode,
                                              Ucs_Stream_PortOption_t port_option,
                                              Ucs_Stream_PortClockMode_t clock_mode,
                                              Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                              Ucs_StdResult_t result,
                                              void *user_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Detailed information used for Extended Resource Manager results. */
typedef struct Ucs_Xrm_ResultDetails_
{
    /*! \brief Specifies the INIC resource type for which the result has been returned. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Specifies the table index of the resource object for which the result has been returned. */
    uint8_t resource_index;
    /*! \brief Specifies the type of the current asynchronous result.
     *  \details The following briefly describes the different types of results available:
     *              - \b UCS_XRM_RESULT_TYPE_INT: internal results, typically standard return codes of MNS used for synchronous response. \n Refer to \em int_result to get the results.
     *              - \b UCS_XRM_RESULT_TYPE_TGT: target results, typically INIC function-specific error from target device. \n Refer to \em inic_result to get the results.
     *              - \b UCS_XRM_RESULT_TYPE_TX:  transmission results, typically transmission error on the MOST network. \n Refer to \em tx_result to get the results.
     */
    Ucs_Xrm_ResultType_t result_type;
    /*! \brief Holds the internal MNS results. */
    Ucs_Return_t int_result;
    /*! \brief Holds the INIC results. */
    Ucs_StdResult_t inic_result;
    /*! \brief Holds the transmission error information. */
    Ucs_MsgTxStatus_t tx_result;

} Ucs_Xrm_ResultDetails_t;

/*! \brief Result structure of the Extended Resource Manager. */
typedef struct Ucs_Xrm_Result_
{
    /*! \brief Result code. */
    Ucs_Xrm_ResultCode_t code;
    /*! \brief Detailed information on the returned result. */
    Ucs_Xrm_ResultDetails_t details;

} Ucs_Xrm_Result_t;

/*! \brief Resources Identity from user point of view. */
typedef struct Ucs_Xrm_ResIdentity_
{
    /*! \brief Result code. */
    Ucs_Xrm_ResObject_t * resource_store;
    /*! \brief Result code. */
    uint8_t resource_id;

} Ucs_Xrm_ResIdentity_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures used for INIC resource objects                                                      */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Configuration structure of a default created port. This structure is used for ports that 
 *         are configured via the INIC's Configuration String and are automatically created at 
 *         startup.
 */
typedef struct Ucs_Xrm_DefaultCreatedPort_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Type of the hardware port */
    Ucs_Xrm_PortType_t port_type;
    /*! \brief Port instance identifier */
    uint8_t index;

} Ucs_Xrm_DefaultCreatedPort_t;

/*! \brief Configuration structure of a MOST socket. */
typedef struct Ucs_Xrm_MostSocket_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Port resource handle. \mns_param_inic{MOSTPortHandle,MOSTSocketCreate,MNSH3-MOSTSocketCreate611}
     */
    uint16_t most_port_handle;
    /*! \brief Direction of data stream. \mns_param_inic{Direction,MOSTSocketCreate,MNSH3-MOSTSocketCreate611} 
     */
    Ucs_SocketDirection_t direction;
    /*! \brief Data type. \mns_param_inic{DataType,MOSTSocketCreate,MNSH3-MOSTSocketCreate611}
     */
    Ucs_Most_SocketDataType_t data_type;
    /*! \brief Required socket bandwidth. \mns_param_inic{Bandwidth,MOSTSocketCreate,MNSH3-MOSTSocketCreate611}
     */
    uint16_t bandwidth;

} Ucs_Xrm_MostSocket_t;

/*! \brief Configuration structure of a MediaLB port. */
typedef struct Ucs_Xrm_MlbPort_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief MLB port instance. \mns_param_inic{Index,MediaLBPortCreate,MNSH3-MediaLBPortCreate621}
     */
    uint8_t index;
    /*! \brief Clock speed configuration. \mns_param_inic{ClockConfig,MediaLBPortCreate,MNSH3-MediaLBPortCreate621}
     */
    Ucs_Mlb_ClockConfig_t clock_config;

} Ucs_Xrm_MlbPort_t;

/*! \brief Configuration structure of a MediaLB socket. */
typedef struct Ucs_Xrm_MlbSocket_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object the socket is attached to.  */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *mlb_port_obj_ptr;
    /*! \brief Direction of data stream. \mns_param_inic{Direction,MediaLBSocketCreate,MNSH3-MediaLBSocketCreate631}
     */
    Ucs_SocketDirection_t direction;
    /*! \brief Data type. \mns_param_inic{DataType,MediaLBSocketCreate,MNSH3-MediaLBSocketCreate631}
     */
    Ucs_Mlb_SocketDataType_t data_type;
    /*! \brief Required socket bandwidth. \mns_param_inic{Bandwidth,MediaLBSocketCreate,MNSH3-MediaLBSocketCreate631}
     */
    uint16_t bandwidth;
    /*! \brief MLB channel address. \mns_param_inic{ChannelAddress,MediaLBSocketCreate,MNSH3-MediaLBSocketCreate631}
     */
    uint16_t channel_address;

} Ucs_Xrm_MlbSocket_t;

/*! \brief Configuration structure of a USB port. */
typedef struct Ucs_Xrm_UsbPort_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief USB port instance. \mns_param_inic{Index,USBPortCreate,MNSH3-USBPortCreate661}
     */
    uint8_t index;
    /*! \brief interface of the USB Port’s physical layer. \mns_param_inic{PhysicalLayer,USBPortCreate,MNSH3-USBPortCreate661}
     */
    Ucs_Usb_PhysicalLayer_t physical_layer;
    /*! \brief USB devices interfaces mask. \mns_param_inic{DeviceInterfaces,USBPortCreate,MNSH3-USBPortCreate661}
     */
    uint16_t devices_interfaces;
    /*! \brief OUT Endpoints inside the streaming interface. \mns_param_inic{StreamingIfEpOutCount,USBPortCreate,MNSH3-USBPortCreate661}
     */
    uint8_t streaming_if_ep_out_count;
    /*! \brief IN Endpoints inside the streaming interface. \mns_param_inic{StreamingIfEpInCount,USBPortCreate,MNSH3-USBPortCreate661}
     */
    uint8_t streaming_if_ep_in_count;

} Ucs_Xrm_UsbPort_t;

/*! \brief Configuration structure of a USB socket. */
typedef struct Ucs_Xrm_UsbSocket_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object the socket is attached to. */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *usb_port_obj_ptr;
    /*! \brief Direction of the data stream. \mns_param_inic{Direction,USBSocketCreate,MNSH3-USBSocketCreate671}
     */
    Ucs_SocketDirection_t direction;
    /*! \brief Data type. \mns_param_inic{DataType,USBSocketCreate,MNSH3-USBSocketCreate671}
     */
    Ucs_Usb_SocketDataType_t data_type;
    /*! \brief Address of a USB Endpoint. \mns_param_inic{EndpointAddress,USBSocketCreate,MNSH3-USBSocketCreate671}
     */
    uint8_t end_point_addr;
    /*! \brief Number of MOST network frames/packets per one USB transaction. \mns_param_inic{FramesPerTransfer,USBSocketCreate,MNSH3-USBSocketCreate671}
     */
    uint16_t frames_per_transfer;

} Ucs_Xrm_UsbSocket_t;

/*! \brief Configuration structure of a RMCK port. */
typedef struct Ucs_Xrm_RmckPort_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief RMCK Port instance. \mns_param_inic{Index,RMCKPortCreate,MNSH3-RMCKPortCreate6A1}
     */
    uint8_t index;
    /*! \brief Source of the RMCK clock. \mns_param_inic{ClockSource,RMCKPortCreate,MNSH3-RMCKPortCreate6A1}
     */
    Ucs_Rmck_PortClockSource_t clock_source;
    /*! \brief Divisor of the clock source. \mns_param_inic{Divisor,RMCKPortCreate,MNSH3-RMCKPortCreate6A1}
     */
    uint16_t divisor;

} Ucs_Xrm_RmckPort_t;

/*! \brief Configuration structure of a streaming port. */
typedef struct Ucs_Xrm_StrmPort_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Streaming Port instance. \mns_param_inic{Index,StreamPortCreate,MNSH3-StreamPortCreate681}
     */
    uint8_t index;
    /*! \brief Clock speed configuration. \mns_param_inic{ClockConfig,StreamPortCreate,MNSH3-StreamPortCreate681}
     */
    Ucs_Stream_PortClockConfig_t clock_config;
    /*! \brief Alignment of the data bytes. \mns_param_inic{DataAlignment,StreamPortCreate,MNSH3-StreamPortCreate681}
     */
    Ucs_Stream_PortDataAlign_t data_alignment;

} Ucs_Xrm_StrmPort_t;

/*! \brief Configuration structure of a streaming data socket. */
typedef struct Ucs_Xrm_StrmSocket_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object the socket is attached to. */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *stream_port_obj_ptr;
    /*! \brief Direction of the data stream. \mns_param_inic{Direction,StreamSocketCreate,MNSH3-StreamSocketCreate691}
     */
    Ucs_SocketDirection_t direction;
    /*! \brief Data type. \mns_param_inic{DataType,StreamSocketCreate,MNSH3-StreamSocketCreate691}
     */
    Ucs_Stream_SocketDataType_t data_type;
    /*! \brief Required socket bandwidth in bytes. \mns_param_inic{Bandwidth,StreamSocketCreate,MNSH3-StreamSocketCreate691}
     */
    uint16_t bandwidth;
    /*! \brief ID of the serial interface pin. \mns_param_inic{StreamPinID,StreamSocketCreate,MNSH3-StreamSocketCreate691}
     */
    Ucs_Stream_PortPinId_t stream_pin_id;

} Ucs_Xrm_StrmSocket_t;

/*! \brief Configuration structure of a synchronous data connection. */
typedef struct Ucs_Xrm_SyncCon_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         starting point of the link. Must be a socket of type \c Input.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_in_obj_ptr;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         ending point of the link. Must be a socket of type \c Output.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_out_obj_ptr;
    /*! \brief Mode of operation of mute. \mns_param_inic{MuteMode,SyncCreate,MNSH3-SyncCreate871}  
     */
    Ucs_Sync_MuteMode_t mute_mode;
    /*! \brief Offset from where the socket data should be routed from a splitter. \mns_param_inic{Offset,SyncCreate,MNSH3-SyncCreate871}  
     */
    uint16_t offset;

} Ucs_Xrm_SyncCon_t;

/*! \brief Configuration structure of a DiscreteFrame Isochronous streaming phase connection. */
typedef struct Ucs_Xrm_DfiPhaseCon_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         starting point of the link. Must be a socket of type \c Input.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_in_obj_ptr;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         ending point of the link. Must be a socket of type \c Output.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_out_obj_ptr;

} Ucs_Xrm_DfiPhaseCon_t;

/*! \brief Configuration structure of a combiner resource. */
typedef struct Ucs_Xrm_Combiner_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the synchronous socket. */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *port_socket_obj_ptr;
    /*! \brief Port resource handle. \mns_param_inic{MOSTPortHandle,CombinerCreate,MNSH3-CombinerCreate901} 
     */
    uint16_t most_port_handle;
    /*! \brief Total number of data bytes to be transferred each MOST network frame. \mns_param_inic{BytesPerFrame,CombinerCreate,MNSH3-CombinerCreate901}  
     */
    uint16_t bytes_per_frame;

} Ucs_Xrm_Combiner_t;

/*! \brief Configuration structure of a splitter resource. */
typedef struct Ucs_Xrm_Splitter_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the synchronous socket. */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_in_obj_ptr;
    /*! \brief Port resource handle. \mns_param_inic{MOSTPortHandle,SplitterCreate,MNSH3-SplitterCreate911}
     */
    uint16_t most_port_handle;
    /*! \brief Total number of data bytes to be transferred each MOST network frame. \mns_param_inic{BytesPerFrame,SplitterCreate,MNSH3-SplitterCreate911}  
     */
    uint16_t bytes_per_frame;

} Ucs_Xrm_Splitter_t;

/*! \brief Configuration structure for a A/V Packetized isochronous streaming data connection. */
typedef struct Ucs_Xrm_AvpCon_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         starting point of the link. Must be a socket of type \c Input.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_in_obj_ptr;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         ending point of the link. Must be a socket of type \c Output.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_out_obj_ptr;
    /*! \brief Size of data packets. \mns_param_inic{IsocPacketSize,AVPacketizedCreate,MNSH3-AVPacketizedCreate861} 
     */
    Ucs_Avp_IsocPacketSize_t isoc_packet_size;

} Ucs_Xrm_AvpCon_t;

/*! \brief Configuration structure for a Quality of Service IP streaming data connection. */
typedef struct Ucs_Xrm_QoSCon_
{
    /*! \brief Type of the INIC resource object. */
    Ucs_Xrm_ResourceType_t resource_type;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         starting point of the link. Must be a socket of type \c Input.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_in_obj_ptr;
    /*! \brief Reference to the INIC resource object that specifies the socket that is the 
     *         ending point of the link. Must be a socket of type \c Output.
     */
    UCS_XRM_CONST Ucs_Xrm_ResObject_t *socket_out_obj_ptr;

} Ucs_Xrm_QoSCon_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Function signature used for the MOST Port status.
 *
 *  This callback function is called to report streaming-related information for a MOST Port.
 *  \param most_port_handle          Port resource handle.
 *  \param availability              State of the MOST port related to streaming connections.
 *  \param avail_info                Sub state to parameter \c availability.
 *  \param free_streaming_bw         Free streaming bandwidth for the dedicated MOST Port.
 *  \param user_ptr                  User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 *  \ingroup G_UCS_IRM
 */
typedef void (*Ucs_Xrm_Most_PortStatusCb_t)(uint16_t most_port_handle,
                                            Ucs_Most_PortAvail_t availability,
                                            Ucs_Most_PortAvailInfo_t avail_info,
                                            uint16_t free_streaming_bw,
                                            void* user_ptr);

/*! \brief  Function signature used for monitoring the XRM resources.
 *  \param  resource_type       The XRM resource type to be looked for
 *  \param  resource_ptr        Reference to the resource to be looked for
 *  \param  resource_infos      Resource information
 *  \param  endpoint_inst_ptr   Reference to the endpoint object that encapsulates the given resource.
 *  \param  user_ptr            User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 */
typedef void (*Ucs_Xrm_ResourceDebugCb_t)(Ucs_Xrm_ResourceType_t resource_type, Ucs_Xrm_ResObject_t *resource_ptr, Ucs_Xrm_ResourceInfos_t resource_infos, void *endpoint_inst_ptr, void *user_ptr);

/*! \brief Function signature used for the check unmute callback.
 *
 *   Whenever this callback function is called and the EHC has sink connections muted by the mute pin, the application has to ensure that this mute pin is not asserted before attempting unmute.
 *  \param  node_address  The node address of the device to be looked for.
 *  \ingroup G_UCS_IRM
 */
typedef void (*Ucs_Xrm_CheckUnmuteCb_t)(uint16_t node_address, void *user_ptr);


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_XRM_PB_H */

/*! @} */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

