/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include <phTmlNfc.h>
#include "phNciNfc_CoreSend.h"
#include "phNciNfc_CoreRecv.h"

/**
* \ingroup grp_nci_nfc
* \brief The supported NCI version of the specification
*/
#define PH_NCINFC_VERSION_MAJOR_MASK                (0xF0)
#define PH_NCINFC_VERSION_MINOR_MASK                (0x0F)

#define PH_NCINFC_VERSION_MAJOR_1x                  (0x01)
#define PH_NCINFC_VERSION_MINOR_1x                  (0x00)
#define PH_NCINFC_VERSION_MAJOR_2x                  (0x02)
#define PH_NCINFC_VERSION_MINOR_2x                  (0x00)

#define PH_NCINFC_VERSION_1x                        ((PH_NCINFC_VERSION_MAJOR_1x << 4) | PH_NCINFC_VERSION_MINOR_1x)
#define PH_NCINFC_VERSION_2x                        ((PH_NCINFC_VERSION_MAJOR_2x << 4) | PH_NCINFC_VERSION_MINOR_2x)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Internal Callback for handling the sequence/state needs.
 *   This is supposed to be updated in NCIContext during CMD/Data send.
 *   param[in] pContext - Upper layer context
 *   param[in] wStatus - Status of the transaction
 */
typedef NFCSTATUS (*pphNciNfc_IfNciCoreNtf_t)(void* pContext, NFCSTATUS wStatus);

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Callback for handling the received data from NFCC.
 *   This is supposed to be passed/registered to NCI Core during Send function Call/Register NTF.
 *   param[in] pContext - Upper layer context
 *   param[in] pInfo - Containig the Transaction Info.
 *   param[in] wStatus - Status of the transaction
 */
typedef NFCSTATUS (*pphNciNfc_CoreIfNtf_t)(void* pContext, void *pInfo, NFCSTATUS wStatus);

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Callback for handling the received data from NFCC.
 *   This is supposed to be passed/registered to NCI Core during Send function Call/Register NTF.
 *   param[in] pContext - Upper layer context
 *   param[in] pInfo - Containig the Transaction Info.
 *   param[in] wStatus - Status of the transaction
 */
typedef NFCSTATUS (*pphNciNfc_CoreIfRspCb_t)(void* pContext, void *pInfo, NFCSTATUS wStatus);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Callback for updating the credits corresponding to a registered connId.
 *   This is supposed to be registered with LogConn Module during CreditAvailability check.
 *   param[in] pContext - Upper layer context
 *   param[in] bCredits - Updated Credits info
 *   param[in] wStatus - Status of the transaction
 */
typedef void (*pphNciNfc_ConnCreditsNtf_t)(void* pContext, uint8_t bCredits, NFCSTATUS wStatus);

/*-------------------------------------------------------------------------------*/
/* Byte offset values for refering Message Type, GID, OID, PBF and Connection ID
   present in the packet header*/
/*-------------------------------------------------------------------------------*/

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NCI Packet Header Length
  */
#define PHNCINFC_CORE_PKT_HEADER_LEN                (0x03)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NCI Packet Header Length
  */
#define PHNCINFC_CORE_MIN_CTRL_PKT_PAYLOAD_LEN      (32U)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering Message type from the packet
  *  header of 3 bytes
  */
#define PHNCINFC_CORE_MT_BYTE_OFFSET                (0x00)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering Packet bounday flag from the
  *  packet header of 3 bytes
  */
#define PHNCINFC_CORE_PBF_BYTE_OFFSET               (0x00)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering GID from the packet header
  *  of 3 bytes
  */
#define PHNCINFC_CORE_GID_BYTE_OFFSET               (0x00)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering Connection ID from the packet header
  *  of 3 bytes
  */
#define PHNCINFC_CORE_CONN_ID_BYTE_OFFSET           (0x00)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering OID from the packet header
  *  of 3 bytes
  */
#define PHNCINFC_CORE_OID_BYTE_OFFSET               (0x01)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering length of payload from the packet
  *  header of 3 bytes
  */
#define PHNCINFC_CORE_LENGTH_OFFSET (0x02U)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Byte Offset for refering the start of payload loaction in
  */
#define PHNCINFC_CORE_PAYLOAD_OFFSET    (0x03U)


/*-------------------------------------------------------------------------------*/
/* Bit offset values for refering Message Type, GID, OID, PBF and Connection ID
   from their respective byte locations present in the packet header*/
/*-------------------------------------------------------------------------------*/

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Bit Offset for refering Message type from 0th byte of the packet header
  */
#define PHNCINFC_CORE_MT_BIT_OFFSET                 (5U)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Bit Offset for refering Packet bounday flag from the 0th byte of the
  *  packet header
  */
#define PHNCINFC_CORE_PBF_BIT_OFFSET                (4U)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Core Reset Rsp length in NCI 1.x
  * 1 byte for status / 1 byte for NCI version / 1 byte for Configuration Status
  */
#define PHNCINFC_CORE_RESET_RSP_LEN_NCI1x           (3U)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Core Reset Rsp length in NCI 2.x
  * 1 byte for status
  */
#define PHNCINFC_CORE_RESET_RSP_LEN_NCI2x           (1U)


/*-------------------------------------------------------------------------------*/
/*Sets Header information in Byte format, Byte value to set is passed as n and
  v is the value*/
/*-------------------------------------------------------------------------------*/

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Add 3 bit(b7 b6 b5) MT value in the 0th octet of control/data packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_MT(pBuff,v)    { \
                        (pBuff)[PHNCINFC_CORE_MT_BYTE_OFFSET] &= (uint8_t)0x1FU; \
                        (pBuff)[PHNCINFC_CORE_MT_BYTE_OFFSET] |= (uint8_t)((v) << PHNCINFC_CORE_MT_BIT_OFFSET); \
                                         }

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Set 1 bit(b4) PBF value in the 0th octet of control/data packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_PBF(pBuff)     (((pBuff)[PHNCINFC_CORE_PBF_BYTE_OFFSET]) |= ((uint8_t)0x1U\
                                            << (PHNCINFC_CORE_PBF_BIT_OFFSET)))

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Clear 1 bit(b4) PBF value in the 0th octet of control/data packet header of 3 octets
  */
#define PHNCINFC_CORE_CLR_PBF(pBuff)      (((pBuff)[PHNCINFC_CORE_PBF_BYTE_OFFSET]) &= ((uint8_t)0xEFU ))

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Add 4 bit(b3 b2 b1 b0) GID value in 0th octet of control packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_GID(pBuff,v)   { \
                             (pBuff)[PHNCINFC_CORE_GID_BYTE_OFFSET] &= (uint8_t)0xF0U; \
                             (pBuff)[PHNCINFC_CORE_GID_BYTE_OFFSET] |= ((uint8_t)(0x0FU) & (v)); \
                                         }

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Add 6 bit(b5 b4 b3 b2 b1 b0) OID value in the 1st octet of control packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_OID(pBuff,v)   { \
                             (pBuff)[PHNCINFC_CORE_OID_BYTE_OFFSET] &= (uint8_t)0xC0U; \
                             (pBuff)[PHNCINFC_CORE_OID_BYTE_OFFSET] |= ((uint8_t)(0x3FU) & (v)); \
                                         }

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Add 4 bit(b3 b2 b1 b0) Connection Id value in the 0th octet of data packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_CONID(pBuff,v)  {\
                        ((pBuff)[PHNCINFC_CORE_CONN_ID_BYTE_OFFSET]) &= ((uint8_t)0xF0U);   \
                        ((pBuff)[PHNCINFC_CORE_CONN_ID_BYTE_OFFSET]) |= ((uint8_t)0x0FU & (v)); \
                        }

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Add 8 bits(b7 b6 b5 b4 b3 b2 b1 b0) payload length value in the 3rd octet of data packet header of 3 octets
  */
#define PHNCINFC_CORE_SET_LENBYTE(pBuff,v) (((pBuff)[PHNCINFC_CORE_LENGTH_OFFSET]) = (v))

/*-------------------------------------------------------------------------------*/
/*Get Header information in Byte format, Byte value to be passed as n*/
/*-------------------------------------------------------------------------------*/

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 3 bit(b7 b6 b5) MT value from control/data packet header byte
  */
#define PHNCINFC_CORE_GET_MT(pBuff)      ((((pBuff)[PHNCINFC_CORE_MT_BYTE_OFFSET]) & (0xE0U))\
                                            >> PHNCINFC_CORE_MT_BIT_OFFSET)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 1 bit(b4) PBF value from control/data packet header byte
  */
#define PHNCINFC_CORE_GET_PBF(pBuff)     ((((pBuff)[PHNCINFC_CORE_PBF_BYTE_OFFSET]) & (0x10U))\
                                            >> PHNCINFC_CORE_PBF_BIT_OFFSET)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 4 bit(b3 b2 b1 b0) GID value from control packet header byte
  */
#define PHNCINFC_CORE_GET_GID(pBuff)      (((pBuff)[PHNCINFC_CORE_GID_BYTE_OFFSET]) & (0x0FU))

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 6 bit(b5 b4 b3 b2 b1 b0) OID value from control packet header byte
  */
#define PHNCINFC_CORE_GET_OID(pBuff)      (((pBuff)[PHNCINFC_CORE_OID_BYTE_OFFSET]) & (0x3FU))

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 4 bit Connection ID value from control packet header byte
  */
#define PHNCINFC_CORE_GET_CONNID(pBuff)    (((pBuff)[PHNCINFC_CORE_CONN_ID_BYTE_OFFSET]) & (0x0FU))

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief Get 8 bit payload length value from control packet header byte
  */
#define PHNCINFC_CORE_GET_LENBYTE(pBuff) ((pBuff)[PHNCINFC_CORE_LENGTH_OFFSET])

/*-------------------------------------------------------------------------------*/
/*Generic*/
/*-------------------------------------------------------------------------------*/

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum size of a control or data packet
  */
#define PHNCINFC_CORE_MAX_PKT_SIZE              (260)       /**<Maximum Control and Data packet size */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum size of a buffer (having payload) that can be sent to NFCC
  */
#define PHNCINFC_CORE_MAX_BUFF_SIZE             (260)       /**<Maximum size of a buffer (containing payload) that
                                                                can be sent to NFCC */
/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Manufacturer Information size in NCI 1.x
  */
#define PHNCINFC_CORE_MANUF_INFO_LEN_NCI1x      (4)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Invalid connection Id
  */

#define PHNCINFC_CORE_INVALID_CONN_ID           (0xFF)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum connection Id value
  */
#define PHNCINFC_CORE_MAX_CONNID_VALUE          (0x0F)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Minimum length of the control or data packet to be received
  */

#define PH_NCINFC_CORE_MINPKT_LEN               (3)             /**< Size of Control or Data packet header */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum number of Rf interfaces supported
 */
#define PH_NCINFC_CORE_MAX_SUP_RF_INTFS         (0x8U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum call back function registrations allowed for response messages
 */
#define PHNCINFC_CORE_MAX_RSP_REGS              (1U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum call back function registrations allowed for notification messages
 */
#define PHNCINFC_CORE_MAX_NTF_REGS               (15U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum call back function registrations allowed for data messages
 */
#define PHNCINFC_CORE_MAX_DATA_REGS              (3U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Registration entry disabled
 */
#define PHNCINFC_CORE_DISABLE_REG_ENTRY         (0U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Maximum size of receive buffer
  */

#define PHNCINFC_CORE_MAX_RECV_BUFF_SIZE        (260U)

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains  NCI Message Types
  */

typedef enum phNciNfc_NciCoreMsgType
{
    phNciNfc_e_NciCoreMsgTypeData = 0,                      /**<Data message */
    phNciNfc_e_NciCoreMsgTypeCntrlCmd,                      /**<Control - Command message */
    phNciNfc_e_NciCoreMsgTypeCntrlRsp,                      /**<Control - Response message */
    phNciNfc_e_NciCoreMsgTypeCntrlNtf,                      /**<Control - Notification message */
    phNciNfc_e_NciCoreMsgTypeInvalid                        /**<Control - Invalid */
} phNciNfc_NciCoreMsgType_t ;


/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Group IDs (GID)
  */

typedef enum phNciNfc_CoreGid
{
    phNciNfc_e_CoreNciCoreGid = 0x00,                   /**<NCI core related group */
    phNciNfc_e_CoreRfMgtGid = 0x01,                     /**<Rf Management group */
    phNciNfc_e_CoreNfceeMgtGid = 0x02,                  /**<NFCEE Management group */
    phNciNfc_e_CoreInvalidGid,                          /**<Invalid GID */
    phNciNfc_e_CorePropGid = 0x0F                       /**<Proprietary group */
} phNciNfc_CoreGid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Command OIDs for NCI Core Group.
 * \brief The Command OIDs are sent from DH to NFCC.
  */

typedef enum phNciNfc_CoreNciCoreCmdOid
{
    phNciNfc_e_NciCoreResetCmdOid = 0x00,                   /**<NCI core Reset command OID */
    phNciNfc_e_NciCoreInitCmdOid = 0x01,                    /**<NCI Core Init command OID */
    phNciNfc_e_NciCoreSetConfigCmdOid = 0x02,               /**<NCI Core Set configuration command OID */
    phNciNfc_e_NciCoreGetConfigCmdOid = 0x03,               /**<NCI Core Get configuration command OID */
    phNciNfc_e_NciCoreConnCreateCmdOid = 0x04,              /**<NCI Core DH initiated Log. Conn. command OID */
    phNciNfc_e_NciCoreConnCloseCmdOid = 0x05,               /**<NCI Core Close Log. Conn. command  OID */
    phNciNfc_e_NciCoreSetPowerSubStateCmdOid = 0x09,        /**<NCI Core Set power sub state command OID */
    phNciNfc_e_NciCoreInvalidOid = 0x0F                     /**<NCI Core Invalid OID */
} phNciNfc_CoreNciCoreCmdOid_t ;


/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Response OIDs for NCI Core Group
 * \brief The Response OIDs are received by DH from NFCC as part of response to command OIDs.
  */

typedef enum phNciNfc_CoreNciCoreRspOid
{
    phNciNfc_e_NciCoreResetRspOid = 0x00,                   /**<NCI Core Reset Response OID */
    phNciNfc_e_NciCoreInitRspOid = 0x01,                    /**<NCI Core Init Response OID */
    phNciNfc_e_NciCoreSetConfigRspOid = 0x02,               /**<NCI Core Set configuration Response OID */
    phNciNfc_e_NciCoreGetConfigRspOid = 0x03,               /**<NCI Core Get configuration Response OID */
    phNciNfc_e_NciCoreDhConnRspOid = 0x04,                  /**<NCI Core DH initiated Log. Conn. Response OID*/
    phNciNfc_e_NciCoreConnCloseRspOid = 0x05,               /**<NCI Core Close Log. Conn. Response OID */
    phNciNfc_e_NciCoreSetPowerSubStateRspOid = 0x09,        /**<NCI Core Set power sub state Response OID */
}phNciNfc_CoreNciCoreRspOid_t ;


/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Notification OIDs for NCI Core Group
 * \brief The notification OIDs are received by DH as notification event
  */

typedef enum phNciNfc_CoreNciCoreNtfOid
{
    phNciNfc_e_NciCoreResetNtfOid = 0x00,             /**<NCI Core Reset notif OID */
    phNciNfc_e_NciCoreConnCreditNtfOid = 0x06,        /**<NCI Core NFCC Credit available for Log. Conn. notif OID */
    phNciNfc_e_NciCoreGenericErrNtfOid = 0x07,        /**<NCI Core NFCC Credit available for Log. Conn. notif OID */
    phNciNfc_e_NciCoreInterfaceErrNtfOid = 0x08       /**<NCI Core Interface error Notification OID */
}phNciNfc_CoreNciCoreNtfOid_t ;


/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Command OIDs for NCI RF Management Group
 * \brief The Command OIDs are sent from DH to NFCC.
  */

typedef enum phNciNfc_CoreRfMgtCmdOid
{
    phNciNfc_e_RfMgtRfDiscoverMapCmdOid = 0x00,       /**<RF Management RF discover map command OID */
    phNciNfc_e_RfMgtRfSetRoutingCmdOid = 0x01,        /**<RF Management RF Set listen mode routing command OID */
    phNciNfc_e_RfMgtRfGetRoutingCmdOid = 0x02,        /**<RF Management RF Get listen mode routing command OID */
    phNciNfc_e_RfMgtRfDiscoverCmdOid = 0x03,          /**<RF Management RF Discover command OID */
    phNciNfc_e_RfMgtRfDiscSelectCmdOid = 0x04,        /**<RF Management RF Discover select command OID */
    phNciNfc_e_RfMgtRfDeactivateCmdOid = 0x06,        /**<RF Management RF Deactivate command OID */
    phNciNfc_e_RfMgtRfT3tPollingCmdOid = 0x08,        /**<RF Management RF T3T polling command OID */
    phNciNfc_e_RfMgtRfParamUpdateCmdOid = 0x0B,       /**<RF Management RF Parameter update command OID */
    phNciNfc_e_RfMgtRfIsoDepPresChkCmdOid = 0x10,     /**<RF Management RF ISO presence check command OID */
}phNciNfc_CoreRfMgtCmdOid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Response OIDs for NCI RF Management Group
 * \brief The Response OIDs are received by DH from NFCC as part of response to command OIDs.
  */

typedef enum phNciNfc_CoreRfMgtRspOid
{
    phNciNfc_e_RfMgtRfDiscoverMapRspOid = 0x00,         /**<RF Management RF discover map response OID */
    phNciNfc_e_RfMgtRfSetRoutingRspOid = 0x01,          /**<RF Management RF Set listen mode routing response OID */
    phNciNfc_e_RfMgtRfGetRoutingRspOid = 0x02,          /**<RF Management RF Get listen mode routing response OID */
    phNciNfc_e_RfMgtRfDiscoverRspOid = 0x03,            /**<RF Management RF Discover response OID */
    phNciNfc_e_RfMgtRfDiscSelectRspOid = 0x04,          /**<RF Management RF Discover select response OID */
    phNciNfc_e_RfMgtRfDeactivateRspOid = 0x06,          /**<RF Management RF Deactivate response OID */
    phNciNfc_e_RfMgtRfT3tPollingRspOid = 0x08,          /**<RF Management RF T3T polling response OID */
    phNciNfc_e_RfMgtRfParamUpdateRspOid = 0x0B,         /**<RF Management RF Parameter update response OID */
    phNciNfc_e_RfMgtRfIsoDepPresChkRspOid = 0x10,       /**<RF Management RF ISO presence check response OID */
}phNciNfc_CoreRfMgtRspOid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Notification OIDs for NCI RF Management Group
 * \brief The notification OIDs are received by DH as notification event
  */

typedef enum phNciNfc_CoreRfMgtNtfOid
{
    phNciNfc_e_RfMgtRfGetListenModeRoutingNtfOid = 0x02,/**<RF Management RF Get listen mode routing notification OID */
    phNciNfc_e_RfMgtRfDiscoverNtfOid = 0x03,          /**<RF Management RF Discover notification OID */
    phNciNfc_e_RfMgtRfIntfActivatedNtfOid = 0x05,     /**<RF Management RF Interface activated notification OID */
    phNciNfc_e_RfMgtRfDeactivateNtfOid = 0x06,        /**<RF Management RF Deactivate notification OID */
    phNciNfc_e_RfMgtRfFieldInfoNtfOid = 0x07,         /**<RF Management RF Field info notification OID */
    phNciNfc_e_RfMgtRfT3tPollingNtfOid = 0x08,        /**<RF Management RF T3T polling notification OID */
    phNciNfc_e_RfMgtRfNfceeActionNtfOid = 0x09,       /**<RF Management RF NFCEE action notification OID */
    phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid = 0x0A, /**<RF Management RF NFCEE discovery request notification OID */
    phNciNfc_e_RfMgtRfIsoDepPresChkNtfOid = 0x10,     /**<RF Management RF ISO presence check notification OID */
}phNciNfc_CoreRfMgtNtfOid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Command OIDs for NCI NFCEE Management Group
 * \brief The Command OIDs are sent from DH to NFCC.
  */

typedef enum phNciNfc_CoreNfceeMgtCmdOid
{
    phNciNfc_e_NfceeMgtNfceeDiscCmdOid = 0x00,              /**<NFCEE Management NFCEE discover command OID */
    phNciNfc_e_NfceeMgtModeSetCmdOid = 0x01,                /**<NFCEE Management NFCEE mode set command OID */
    phNciNfc_e_NfceeMgtPowerAndLinkCtrlCmdOid = 0x03        /**<NFCEE Management NFCEE Power And Link Control command OID */
}phNciNfc_CoreNfceeMgtCmdOid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Response OIDs for NCI NFCEE Management Group
 * \brief The Response OIDs are received by DH from NFCC as part of response to command OIDs.
  */

typedef enum phNciNfc_CoreNfceeMgtRspOid
{
    phNciNfc_e_NfceeMgtNfceeDiscRspOid = 0x00,              /**<NFCEE Management NFCEE discover response OID */
    phNciNfc_e_NfceeMgtModeSetRspOid = 0x01,                /**<NFCEE Management NFCEE mode set response OID */
    phNciNfc_e_NfceeMgtPowerAndLinkCtrlRspOid = 0x03        /**<NFCEE Management NFCEE Power And Link Control command OID */
}phNciNfc_CoreNfceeMgtRspOid_t ;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Notification OIDs for NCI NFCEE Management Group
 * \brief The notification OIDs are received by DH as notification event
  */

typedef enum phNciNfc_CoreNfceeMgtNtfOid
{
    phNciNfc_e_NfceeMgtNfceeDiscNtfOid = 0x00,              /**<NFCEE Management NFCEE discover notification OID */
    phNciNfc_e_NfceeMgtModeSetNtfOid = 0x01,                /**<NFCEE Management NFCEE mode set notification OID */
    phNciNfc_e_NfceeMgtStatusNtfOid = 0x02,                 /**<NFCEE Management NFCEE status notification OID */
}phNciNfc_CoreNfceeMgtNtfOid_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Command OIDs for Proprietary Command Group
 */
typedef enum phNciNfc_CorePropCmdOid
{
    phNciNfc_e_CorePropSetPwrModeCmdOid = 0x00,      /**<Nci Extn Management Nci Set Power mode command OID */
    phNciNfc_e_CorePropEnableExtnCmdOid = 0x02,      /**<Nci Extn Management Nci extension command OID */
    phNciNfc_e_CorePropIsoDepPresChkCmdOid = 0x11,    /**<Prop Iso-Dep presence check command Oid*/
    phNciNfc_e_CorePropDhListenFilterCmdOid = 0x15,    /**<Prop Set DH Listen Filter command Oid*/
    phNciNfc_e_CorePropTestPrbsCmdOid      = 0x30,    /**<Prop Prbs test command Oid*/
    phNciNfc_e_CorePropTestAntennaCmdOid   = 0x3D,    /**<Antenna Self test command Oid */
    phNciNfc_e_CorePropTestSwpCmdOid       = 0x3E,    /**<Prop Swp test command Oid*/
}phNciNfc_CorePropCmdOid_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Response OIDs for Propritry Command Group
 */

typedef enum phNciNfc_CorePropRspOid
{
    phNciNfc_e_CorePropSetPwrModeRspOid = 0x00,      /**<Nci Extn Management Nci standby mode response OID */
    phNciNfc_e_CorePropEnableExtnRspOid = 0x02,      /**<Nci Extn Management Nci extension response OID */
    phNciNfc_e_CorePropIsoDepChkPresRspOid = 0x11,   /**<Prop Iso-Dep presence check response Oid*/
    phNciNfc_e_CorePropDhListenFilterRspOid = 0x15,  /**<Prop Config listen filter check response Oid*/
    phNciNfc_e_CorePropTestPrbsRspOid = 0x30,        /**<Prop Prbs test response Oid*/
    phNciNfc_e_CorePropTestAntennaRspOid = 0x3D,     /**<Prop Antenna test response Oid*/
    phNciNfc_e_CorePropTestSwpRspOid = 0x3E         /**<Prop Swp test response Oid*/
} phNciNfc_CorePropRspOid_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains NCI Notification OIDs for Propritry Command Group
 */

typedef enum phNciNfc_CorePropNtfOid
{
    phNciNfc_e_CorePropIsoDepPrsnChkNtfOid = 0x11,   /**<Prop Iso-Dep presence check notification Oid*/
    phNciNfc_e_CorePropTestSwpNtfOid       = 0x3E,   /**<Prop Swp test notification Oid*/
    phNciNfc_e_CorePropTagDetectorNtfOid =   0x13    /**<Prop Tag detector notification Oid*/
} phNciNfc_CorePropNtfOid_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Enum definition contains supported NFCEE Protocol/Interfaces
  */
typedef enum phNciNfc_CoreNfceeProtoInterfaces
{
    phNciNfc_e_NfceeProtoIntfApdu = 0x00,               /**<APDU interface */
    phNciNfc_e_NfceeProtoIntfHciAccess,                 /**<HCI access network interface */
    phNciNfc_e_NfceeProtoIntfT3tCmdSet,                 /**<Type 3 tag interfaces */
    phNciNfc_e_NfceeProtoIntfTransparent                /**<Transparent Interface */
} phNciNfc_CoreNfceeProtoInterfaces_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Struct to hold NFCC features supported by NFCC
  */
typedef struct phNciNfc_sCoreNfccFeatures
{
    uint8_t DiscConfSuprt;                                  /**<Discovery configuration supported */
#if 0
    union {
        uint8_t DiscConfSuprt;
        struct {
            uint32_t DiscFreqConf:1; /**<1b Discovery Frequency Configuration is supported by NFCC in RF_DISCOVER_CMD*/
            uint32_t DiscConfMode:2; /**<00b => Only DH Configures NFCC, 01b: NFCEE and DH both can configure NFCC*/
            uint32_t RFU:5;          /**<RFU and set to 0*/
        }DiscConfigInfo;
    }Byte0
#endif
   uint8_t RoutingType;              /**<Listen mode routing types supported by NFCC */
#if 0
    union {
        uint8_t RoutingType;         /**<If no routing type is supported then NFCC does not support listen
                                         based routing*/
        struct {
            uint32_t RFU:1;          /**<RFU and set to 0*/
            uint32_t TechBased:1;    /**<Technology Based routing, if set to 1*/
            uint32_t ProtoBased:1;   /**<Protocol Based routing, if set to 1*/
            uint32_t AidBased:1;     /**<AID Based routing, if set to 1*/
            uint32_t RFU4:4;         /**<RFU and set to 0*/
        }RoutingTypeInfo;
    }Byte1
#endif
    uint8_t PwrOffState;             /**<Power states supported by NFCC */
#if 0
    union {
        uint8_t PwrOffState;
        struct {
            uint32_t BattOff:1;      /**<Battery Off state supported if set to 1*/
            uint32_t SwtchOff:1;     /**<Switch Off state supported if set to 1*/
        }PwrOffStateInfo;
    }Byte2
#endif
    uint8_t Byte3;                   /**< Reserved for proprietary capabilities */
}phNciNfc_sCoreNfccFeatures_t, *pphNciNfc_sCoreNfccFeatures_t; /**< Pointer to #phNciNfc_sCoreNfccFeatures_t */


/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Struct to hold Init response parameters
  */
typedef struct phNciNfc_sInitRspParams
{
    phNciNfc_sCoreNfccFeatures_t NfccFeatures;          /**< Features supported by NFCC */
    uint8_t NoOfRfIfSuprt;                              /**< Number of Rf interfaces supported */
    uint8_t RfInterfaces[PH_NCINFC_CORE_MAX_SUP_RF_INTFS];   /**< Max No of RF Interfaces supported by NFCC
                                                            Its Dynamic but now static set the
                                                            appropriate value for no
                                                            of RF interfaces supported*/
    uint8_t MaxLogicalCon;                              /**<Max no of Dynamic Logical Connections supported by NFCC*/
    uint16_t RoutingTableSize;                          /**<Maximum Routing table size*/
    uint8_t CntrlPktPayloadLen;                         /**<Maximum payload length of a NCI control Packet Valid range
                                                            32 to 255*/
    uint8_t DataHCIPktPayloadLen;                       /**<Maximum payload length of a NCI data Packet that the NFCC
                                                            is able to receive on the static HCI Connection Valid range
                                                            32 to 255. If not, the value SHALL be 0*/
    uint8_t DataHCINumCredits;                          /**<Initial Number of Credits for this Connection*/
    uint16_t MaxNFCVFrameSize;                          /**<Maximum payload length of an NFC-V Standard Frame supported
                                                            by the NFC Controller for transfer of Commands and reception
                                                            of Responses, when configured to Poll for NFC-V technology.
                                                            Value should be at least 64 bytes*/
    uint16_t MaxSizeLarge;                              /**<The maximum size in octets for the sum of the
                                                            sizes of PB_H_INFO and LB_H_INFO_RESP parameter values.*/
    uint8_t ManufacturerId;                             /**<IC Manufacturer ID */
    struct                                              /**<NFCC manufacturer specific information*/
    {
        uint8_t Length;
        uint8_t *Buffer;                                /**<Manufacturer information NCI*/
    }ManufacturerInfo;
}phNciNfc_sInitRspParams_t, *pphNci_sInitRspParams_t; /**< pointer to #phNci_sInitRspParams_t */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Union of Nci Core Opcode ID's
  */
typedef union phNciNfc_CoreOid
{
    union
    {
        phNciNfc_CoreNciCoreCmdOid_t    NciCoreCmdOid;  /**< Nci Core command OID */
        phNciNfc_CoreRfMgtCmdOid_t      RfMgtCmdOid;    /**< Rf management commandOID */
        phNciNfc_CoreNfceeMgtCmdOid_t   NfceeMgtCmdOid; /**< Nfcee management command OID */
        phNciNfc_CorePropCmdOid_t       PropCmdOid;     /**< Proprietary CMD OID */

        phNciNfc_CoreNciCoreRspOid_t    NciCoreRspOid;  /**< Nci Core response OID */
        phNciNfc_CoreRfMgtRspOid_t      RfMgtRspOid;    /**< Rf management response OID */
        phNciNfc_CoreNfceeMgtRspOid_t   NfceeMgtRspOid; /**< Nfcee management response OID */
        phNciNfc_CorePropRspOid_t       PropRspOid;     /**< Proprietary RSP OID */

        phNciNfc_CoreNciCoreNtfOid_t    NciCoreNtfOid;  /**< Nci Core notification OID */
        phNciNfc_CoreRfMgtNtfOid_t      RfMgtNtfOid;    /**< Rf management notification OID */
        phNciNfc_CoreNfceeMgtNtfOid_t   NfceeMgtNtfOid; /**< Nfcee management notification OID */
        phNciNfc_CorePropNtfOid_t       PropNtfOid;     /**< Proprietary CMD OID */
    }OidType;      /**< Opcode ID */
    uint32_t Val;   /**< Direct access to all OID's listed in #OidType */
}phNciNfc_CoreOid_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Control and Data message information
  */
typedef struct phNciNfc_sCoreHeaderInfo
{
    phNciNfc_NciCoreMsgType_t eMsgType;                 /**< enum of type #phNciNfc_NciCoreMsgType_t*/
    /* Control Packet information */
    phNciNfc_CoreGid_t Group_ID;                        /**< enum of type #phNciNfc_CoreGid_t*/
    phNciNfc_CoreOid_t Opcode_ID;                       /**< enum of type #phNciNfc_CoreOid_t*/
    /* Data Packet information */
    uint8_t bConn_ID;                                   /**< Connection ID of the logical connection */
    uint8_t bEnabled;                                   /**< 0 - disabled,
                                                             1 - Auto-Deregistration enabled (after invoking call back function,
                                                                 it shall be de-registered automatically),
                                                             2 - Call back function can only be de-registered by calling
                                                                 #phNciNfc_CoreRecvMgrDeRegisterCb function */
}phNciNfc_sCoreHeaderInfo_t, *pphNciNfc_sCoreHeaderInfo_t;

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief Structure containing memory buffer to be used in singly linked list
  */
typedef struct phNciNfc_sCoreMem
{
    uint8_t  aBuffer[PHNCINFC_CORE_MAX_RECV_BUFF_SIZE];   /**<Buffer to be used for storing the received packet*/
    uint16_t wLen;                                  /**<Indicates length data in the buffer*/
}phNciNfc_sCoreMem_t,*pphNciNfc_sCoreMem_t; /**< pointer to #phNciNfc_sCoreMem_t structure */

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief Definition of node for singly linked list for storing received packets payload
  */
typedef struct phNciNfc_sCoreRecvBuff_List
{
    phNciNfc_sCoreMem_t tMem;     /**<Buffer for storing received packet's payload*/
    struct phNciNfc_sCoreRecvBuff_List *pNext;  /**<Pointer to the next node present in linked list*/
}phNciNfc_sCoreRecvBuff_List_t,*pphNciNfc_sCoreRecvBuff_List_t; /**<pointer to
                                                                 #phNciNfc_sCoreRecvBuff_List_t structure*/

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Singly linked list for storing the received packets payload
  */

typedef struct phNciNfc_sCoreReceiveInfo
{
    phNciNfc_sCoreRecvBuff_List_t ListHead;  /**<Linked list head*/
    uint16_t wNumOfNodes;                        /**<Total number of nodes in the lsit*/
    uint16_t wPayloadSize;                        /**<Cummulative size of payloads present in all nodes*/
    phNciNfc_sCoreHeaderInfo_t HeaderInfo; /**< Received packet header info*/
}phNciNfc_sCoreReceiveInfo_t,*pphNciNfc_sCoreReceiveInfo_t; /**< pointer to #phNciNfc_sCoreReceiveInfo_t */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Contains received packe's information.
  */
typedef struct phNciNfc_CoreSendInfo
{
    uint8_t aSendPktBuff[PHNCINFC_CORE_MAX_PKT_SIZE];   /**< Buffer to store command that needs to be sent*/
    uint32_t dwSendlength;                              /**< Length of the upper layer sending buffer */
}phNciNfc_CoreSendInfo_t,*pphNciNfc_CoreSendInfo_t; /**< pointer to #phNciNfc_CoreSendInfo_t */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Struct definition Packet Info
 */
typedef struct phNciNfc_CoreTxInfo
{
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo; /**< NCI header info of the packet currently is being sent*/
    uint8_t *Buff;                      /**< Data to Send */
    uint16_t wLen;                      /**< Length of data */
    uint16_t wWriteStatus;               /**< Tml write status (returned along with Tml write call back) */
}phNciNfc_CoreTxInfo_t, *pphNciNfc_CoreTxInfo_t; /**< pointer to #phNciNfc_CoreTxInfo_t */

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief Structure that has to be filled to register for a response/notificaiton/data
  */
typedef struct phNciNfc_CoreRegInfo
{
    uint8_t bEnabled; /**< 0 - disabled,
                           1 - Auto-Deregistration enabled (after invoking call back function,
                               it shall be de-registered automatically),
                           2 - Call back function can only be de-registered by calling
                               #phNciNfc_CoreRecvMgrDeRegisterCb function */
    uint8_t bGid;       /**< Group ID */
    uint8_t bOid;       /**< Oid */
    uint8_t bConnId;    /**< Connection Id (used incase of data messages) */
    void    *pContext;  /**< Upper layer context */
    pphNciNfc_CoreIfNtf_t pNotifyCb; /**< Function to be invoked upon response or notification */
}phNciNfc_CoreRegInfo_t, *pphNciNfc_CoreRegInfo_t; /**< pointer to #pphNciNfc_CoreRegInfo_t */


/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief This structure holds details of the registered response/notification message
  */
typedef struct phNciNfc_CoreRegRspNtfInfo
{
    uint8_t bEnabled; /**< 0 - disabled,
                           1 - Auto-Deregistration enabled (after invoking call back function,
                               it shall be de-registered automatically),
                           2 - Call back function can only be de-registered by calling
                               #phNciNfc_CoreRecvMgrDeRegisterCb function */
    uint8_t bGid;       /**< Group ID */
    uint8_t bOid;       /**< Oid */
    void    *pContext;  /**< Upper layer context */
    pphNciNfc_CoreIfNtf_t pNotifyCb; /**< Function to be invoked upon response or notification */
}phNciNfc_CoreRegRspNtfInfo_t, *pphNciNfc_CoreRegRspNtfInfo_t; /**< pointer to #phNciNfc_CoreRegInfo_t */

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief This structure holds details of the registered data message
  */
typedef struct phNciNfc_CoreRegDataInfo
{
    uint8_t bEnabled; /**< 0 - disabled,
                           1 - Auto-Deregistration enabled (after invoking call back function,
                               it shall be de-registered automatically),
                           2 - Call back function can only be de-registered by calling
                               #phNciNfc_CoreRecvMgrDeRegisterCb function */
    uint8_t bConnId;    /**< Connection Id (used incase of data messages) */
    void    *pContext;  /**< Upper layer context */
    pphNciNfc_CoreIfNtf_t pNotifyCb; /**< Function to be invoked upon response or notification */
}phNciNfc_CoreRegDataInfo_t, *pphNciNfc_CoreRegDataInfo_t; /**< pointer to #phNciNfc_CoreRegInfo_t */

/*!
* \ingroup grp_nci_nfc_core
*
* \brief Holds list of registered response call back functions details
*/
typedef struct phNciNfc_CoreRspRegContext
{
    phNciNfc_CoreRegRspNtfInfo_t aRspRegList[PHNCINFC_CORE_MAX_RSP_REGS]; /**< Registered call back functions list for
                                                                    response messages*/
}phNciNfc_CoreRspRegContext_t, *pphNciNfc_CoreRspRegContext_t; /**< Pointer to #phNciNfc_CoreRspRegContext_t*/

/*!
* \ingroup grp_nci_nfc_core
*
* \brief Holds list of registered notification call back functions details
*/
typedef struct phNciNfc_CoreNtfRegContext
{
    phNciNfc_CoreRegRspNtfInfo_t aNtfRegList[PHNCINFC_CORE_MAX_NTF_REGS]; /**< Registered call back functions list for
                                                                    notifications messages*/
}phNciNfc_CoreNtfRegContext_t, *pphNciNfc_CoreNtfRegContext_t; /**< Pointer to #phNciNfc_CoreNtfRegContext_t */

/*!
* \ingroup grp_nci_nfc_core
*
* \brief Holds list of registered notification call back functions details
*/
typedef struct phNciNfc_CoreDataRegContext
{
    phNciNfc_CoreRegDataInfo_t aDataRegList[PHNCINFC_CORE_MAX_DATA_REGS]; /**< Registered call back functions list for
                                                                      data messages*/
}phNciNfc_CoreDataRegContext_t, *pphNciNfc_CoreDataRegContext_t; /**< Pointer to #phNciNfc_CoreDataRegContext_t */

/*!
* \ingroup grp_nci_nfc_core
*
* \brief Context for the response timeout
*/
typedef struct phNciNfc_RspTimerInfo
{
    phNciNfc_sCoreHeaderInfo_t PktHeaderInfo;           /**< Packet Header info for which timer used*/
    uint32_t dwRspTimerId;                              /**< Timer for Core to handle response */
    uint8_t TimerStatus;                                /**< 0 = Timer not running 1 = timer running*/
}phNciNfc_RspTimerInfo_t;

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief This is a temporary structure which can be used to put any data types into this.
  */
typedef struct phNciNfc_TempInfo
{
    pphNciNfc_CoreTxInfo_t pTxInfo;
    uint32_t dwTimeOutMs;
    pphNciNfc_CoreIfNtf_t NciCb;
    void *pContext;
}phNciNfc_TempInfo_t, *pphNciNfc_TempInfo_t; /**< Pointer to #phNciNfc_TempInfo_t */

/**
 * \ingroup grp_nci_nfc_core
 *
 * \brief Nci Core context structure
  */
typedef struct phNciNfc_CoreContext
{
    phNciNfc_CoreSendInfo_t tSendInfo;              /**< Pointer to #phNciNfc_CoreSendInfo_t */
    phNciNfc_CoreTxInfo_t TxInfo;                   /**< Pointer to #phNciNfc_CoreTxInfo_t*/
    pphNciNfc_CoreIfNtf_t IntNtf;                   /**< Pointer to #pphNciNfc_CoreIfNtf_t */
    void *pNtfContext;
    uint8_t bCoreTxOnly;                            /**< Flag to identify which type of Send operation is in progress*/
    uint8_t IntNtfFlag;                             /**< To be invoked after send complete if set to != 0*/
    uint32_t dwRspTimerId;                          /**< Timer for Core to handle response */
    uint32_t dwRspTimeOutMs;                        /** < Response time out value in ms, set by upper layer*/
    uint8_t bMaxCtrlPktPayloadLen;                  /**< Stored during INIT response */
    uint32_t dwBytes_Remaining;                     /**< Number of bytes remaining to send */
    void *pHwRef;                                   /**< Information of the hardware */
    phNciNfc_sCoreReceiveInfo_t tReceiveInfo;       /**< Contains Received packet and its information */
    phNciNfc_RspTimerInfo_t TimerInfo;              /**< Timer context handled into core context*/
    phNciNfc_CoreRspRegContext_t tRspCtx;           /**< Holds information regarding registered GID,OID for Response*/
    phNciNfc_CoreNtfRegContext_t tNtfCtx;           /**< Holds information regarding registered GID,OID for Notification*/
    phNciNfc_CoreDataRegContext_t tDataCtx;         /**< Holds information regarding registered ConnID's for Data messages*/
    phNciNfc_SendStateContext_t SendStateContext;   /**< Core Send state context*/
    phNciNfc_RecvStateContext_t RecvStateContext;   /**< Core Send state context*/
    phNciNfc_TempInfo_t tTemp;                      /**< Temporary information to be passed to Send state machine*/
    phTmlNfc_TransactInfo_t pInfo;                  /**< Transaction info received from TML*/
    uint8_t bNciRelease;                            /**< This flag is used during NCI release */
    uint8_t bLogDataMessages;
}phNciNfc_CoreContext_t, *pphNciNfc_CoreContext_t;  /**< pointer to #phNciNfc_CoreContext_t */

extern pphNciNfc_CoreContext_t phNciNfc_GetCoreContext();

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall be invoked during NCI Initialization
 *
 *  \param[in] pContext - pointer to the NCI context structure
 *
 *  \return NFCSTATUS_INVALID_PARAMETER     - Invalid input parameters
 *  \return NFCSTATUS_PENDING               - Init pending
 */
extern NFCSTATUS
phNciNfc_CoreInitialise(
                        pphNciNfc_CoreContext_t pContext
                       );

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall release Nci core and its sub-modules
 *
 *  \param[in] pContext - pointer to the NCI context structure
 *
 *  \return NFCSTATUS_SUCCESS               - Init pending
 *  \return NFCSTATUS_INVALID_PARAMETER     - Invalid input parameters
 */
extern NFCSTATUS
phNciNfc_CoreRelease(
                     pphNciNfc_CoreContext_t pContext
                    );

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall clear core context structure
 *
 *  \param[in] pContext - pointer to the NCI context structure
 *
 *  \return None
 */
void phNciNfc_CleanCoreContext(pphNciNfc_CoreContext_t pContext);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function gets GID & OID, validates and forms command packet.
 *         If the command message is greater than Max_Ctrl_Packet_Payload_length,
 *         invokes segment() to segment the command message
 *
 *  \param[in] pContext - pointer to the Core context structure
 *  \param[in] pTxInfo - pointer to the Core send info structure
 *
 *  \return Returns Nfc status
 */

extern NFCSTATUS phNciNfc_CoreBuildCmdPkt(
                                        pphNciNfc_CoreContext_t pContext,
                                        pphNciNfc_CoreTxInfo_t pTxInfo);


/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall invoked TML_Write function for sending command or
 *         data packets.
 *
 *  \param[in] pCtx - pointer to the Core context structure
 *
 *  \return Returns Nfc status
 */

extern NFCSTATUS phNciNfc_CoreSend(pphNciNfc_CoreContext_t pCtx);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall invoked TML_Read function for receiving response or
 *         data packets.
 *
 *  \param[in] pCtx - pointer to the Core context structure
 *
 *  \return Returns Nfc status
 */
extern NFCSTATUS phNciNfc_CoreRecv(pphNciNfc_CoreContext_t pCtx);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall update the length of received packets payload into its respective
 *node.
 *
 *  \param[in] pCoreCtx - pointer to the Core context structure
 *  \pBuff[in] wLength - length of received packet's payload to be updated in its respective node
 *
 *  \return None
 */

extern void phNciNfc_CoreUpdatePacketLen(pphNciNfc_CoreContext_t pCoreCtx,uint16_t wLength);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief Deletes all node from the linked list
 *
 *  \param[in] pCoreCtx pointer to the NCI context structure
 *
 *  \return None
 */
extern void phNciNfc_CoreDeleteList(
                                pphNciNfc_CoreContext_t pCoreCtx
                               );

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall handles send states and invokes respective send function.
 *
 *  \param[in] pContext - pointer to the Core context structure
 *  \param[in] Status - NFC status
 *
 *  \return Returns Nfc status
 */

extern NFCSTATUS
phNciNfc_CoreSendStateHandler(phNciNfc_CoreContext_t *pContext,
                              NFCSTATUS Status);

/**
 *
 *  \brief This function Builds data packet
 *
 *  \param[in] pContext - pointer to the Core context structure
 *  \param[in] pTxInfo - pointer to the Core send info structure
 *
 *  \return Returns Nfc status
 */
NFCSTATUS phNciNfc_CoreBuildDataPkt(
                                    pphNciNfc_CoreContext_t pContext,
                                    pphNciNfc_CoreTxInfo_t pTxInfo
                                    );
