/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcStatus.h>

#define PHFRINFC_LLCP_MIU_DEFAULT         128  /**< Default MIU value (in bytes).*/
#define PHFRINFC_LLCP_WKS_DEFAULT         1    /**< Default WKS value (bitfield).*/
#define PHFRINFC_LLCP_LTO_DEFAULT         10   /**< Default LTO value (in step of 10ms).*/
#define PHFRINFC_LLCP_RW_DEFAULT          1    /**< Default RW value (in frames).*/
#define PHFRINFC_LLCP_OPTION_DEFAULT      0    /**< Default OPTION value (in frames).*/
#define PHFRINFC_LLCP_MIUX_DEFAULT        0    /**< Default MIUX value (in bytes) */
#define PHFRINFC_LLCP_MIUX_MAX            0x7FF    /**< Max MIUX value (in bytes) */
#define PHFRINFC_LLCP_PDU_HEADER_MAX      3    /**< Max size of PDU header (in bytes) */
#define PHFRINFC_LLCP_SN_MAX_LENGTH       255  /**< Max length value for the Service Name */
#define PHFRINFC_LLCP_RW_MAX              15   /**< Max RW value (in frames).*/

#define PHFRINFC_LLCP_NB_SOCKET_MAX          5      /**< Max.number of simultaneous sockets */
#define PHFRINFC_LLCP_SNL_RESPONSE_MAX       256    /**< Max.number of simultaneous discovery requests */

#define PHFRINFC_LLCP_ERR_DISCONNECTED               0x00
#define PHFRINFC_LLCP_ERR_FRAME_REJECTED             0x01
#define PHFRINFC_LLCP_ERR_BUSY_CONDITION             0x02
#define PHFRINFC_LLCP_ERR_NOT_BUSY_CONDITION         0x03

#define PHFRINFC_LLCP_DM_OPCODE_DISCONNECTED               0x00
#define PHFRINFC_LLCP_DM_OPCODE_SAP_NOT_ACTIVE             0x01
#define PHFRINFC_LLCP_DM_OPCODE_SAP_NOT_FOUND              0x02
#define PHFRINFC_LLCP_DM_OPCODE_CONNECT_REJECTED           0x03
#define PHFRINFC_LLCP_DM_OPCODE_CONNECT_NOT_ACCEPTED       0x20
#define PHFRINFC_LLCP_DM_OPCODE_SOCKET_NOT_AVAILABLE       0x21

typedef enum phFriNfc_LlcpMac_eLinkStatus
{
   phFriNfc_LlcpMac_eLinkDefault,
   phFriNfc_LlcpMac_eLinkActivated,
   phFriNfc_LlcpMac_eLinkDeactivated
}phFriNfc_LlcpMac_eLinkStatus_t;

typedef enum phFriNfc_LlcpTransport_eSocketType
{
   phFriNfc_LlcpTransport_eDefaultType,
   phFriNfc_LlcpTransport_eConnectionOriented,
   phFriNfc_LlcpTransport_eConnectionLess
}phFriNfc_LlcpTransport_eSocketType_t;

typedef struct phFriNfc_LlcpTransport_sSocketOptions
{
    /** The remote Maximum Information Unit Extension (NOTE: this is MIUX, not MIU !)*/
    uint16_t miu;
   /** The Receive Window size (4 bits)*/
   uint8_t rw;

}phFriNfc_LlcpTransport_sSocketOptions_t;

typedef struct phFriNfc_Llcp_sLinkParameters
{
    /** The remote Maximum Information Unit (NOTE: this is MIU, not MIUX !)*/
    uint16_t   miu;
    /** The remote Well-Known Services*/
    uint16_t   wks;
    /** The remote Link TimeOut (in 1/100s)*/
    uint8_t    lto;
    /** The remote options*/
    uint8_t    option;

} phFriNfc_Llcp_sLinkParameters_t;
