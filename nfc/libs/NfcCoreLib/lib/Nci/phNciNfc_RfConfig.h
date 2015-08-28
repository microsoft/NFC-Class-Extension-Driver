/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfc_Context.h"

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Total discovery cycle duration tag
 */
#define PHNCINFC_RFCONFIG_TOTAL_DURATION            (0x00)      /**<Total Discovery cycle duration */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Connection devices limit tag
 */
#define PHNCINFC_RFCONFIG_CON_DEVICES_LIMIT         (0x01)      /**<Max number of devices allowed for collusion
                                                                resolution */
/* RFU                                           0x02 - 0x07 */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-A Discovery Parameters Bail-out tag
 */
#define PHNCINFC_RFCONFIG_PA_BAIL_OUT               (0x08)      /**<Bail Out after detecting NFC-A */

/* RFU                                          0x09 - 0x0F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-B Discovery Parameter Application Family Identifier tag
 */
#define PHNCINFC_RFCONFIG_PB_AFI                    (0x10)      /**<Application Family Identifier used for
                                                                NFC-B detection */
/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-B Discovery Parameter Bail-out tag
 */
#define PHNCINFC_RFCONFIG_PB_BAIL_OUT               (0x11)      /**<Bail Out after detecting NFC-B */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-B Discovery Parameter Param 1 of ATTRIB Command
 */
#define PHNCINFC_RFCONFIG_PB_ATTRIB_PARAM1          (0x12)      /**<Param 1 of ATTRIB Cmd for detecting
                                                                 NFC-B */

/* RFU                                          0x13 - 0x17 */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-F Discovery Parameters Bit rate tag
 */
#define PHNCINFC_RFCONFIG_PF_BIT_RATE               (0x18)      /**<Poll mode NFC-F Bit Rate */

/* RFU                                          0x19 - 0x1F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode ISO-DEP Discovery Parameters Higher layer information tag
 */
#define PHNCINFC_RFCONFIG_PB_H_INFO                 (0x20)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode ISO-DEP Discovery Parameters Bit rate tag
 */
#define PHNCINFC_RFCONFIG_PI_BIT_RATE               (0x21)

/* RFU                                          0x22 - 0x27 */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-DEP Discovery Parameters Bit rate tag
 */
#define PHNCINFC_RFCONFIG_BITR_NFC_DEP              (0x28)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-DEP Discovery Parameters ATR Request General Bytes
 */
#define PHNCINFC_RFCONFIG_ATR_REQ_GEN_BYTES         (0x29)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Poll mode NFC-DEP Discovery Parameters ATE Request config tag
 */
#define PHNCINFC_RFCONFIG_ATR_REQ_CONFIG            (0x2A)

/* RFU                                          0x2B - 0x2F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-A Discovery Parameters Bit frame single device detection tag
 */
#define PHNCINFC_RFCONFIG_LA_BIT_FRAME_SDD          (0x30)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-A Discovery Parameters platform config tag
 */
#define PHNCINFC_RFCONFIG_LA_PLATFORM_CONFIG        (0x31)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-A Discovery Parameters selection information tag
 */
#define PHNCINFC_RFCONFIG_LA_SEL_INFO               (0x32)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-A Discovery Parameters NFCID1 tag
 */
#define PHNCINFC_RFCONFIG_LA_NFCID1                 (0x33)      /**<NFCID1 of the Listen mode NFC-A device */

/* RFU                                          0x34 - 0x37 */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-B Discovery Parameters SENSB information tag
 */
#define PHNCINFC_RFCONFIG_LB_SENSB_INFO             (0x38)      /**<To inform NFC forum device in poll mode
                                                            about FSCI, TR2 and IS0/IEC 14443 compliance */
/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-B Discovery Parameters NFCID0 tag
 */
#define PHNCINFC_RFCONFIG_LB_NFCID0                 (0x39)      /**<NFCID0 of the Listen mdoe NFC-B device */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-B Discovery Parameters application data tag
 */
#define PHNCINFC_RFCONFIG_LB_APPLICATION_DATA       (0x3A)      /**<To inform NFC forum device in Poll mode
                                                            about which application are installed
                                                            in NFC forum device in Listen mode */
/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-B Discovery Parameters startup frame guard information tag
 */
#define PHNCINFC_RFCONFIG_LB_SFGI                   (0x3B)      /**<Start-Up Frame Guard Time */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-B Discovery Parameters Application data coding and Frame options tag
 */
#define PHNCINFC_RFCONFIG_LB_ADC_FO                 (0x3C)      /**<Application Data Coding method followed */

/* RFU                                          0x3D - 0x3F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 1 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_1      (0x40)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 2 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_2      (0x41)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 3 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_3      (0x42)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 4 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_4      (0x43)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 5 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_5      (0x44)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 6 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_6      (0x45)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 7 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_7      (0x46)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 8 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_8      (0x47)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 9 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_9      (0x48)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 10 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_10     (0x49)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 11 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_11     (0x4A)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 12 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_12     (0x4B)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 13 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_13     (0x4C)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 14 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_14     (0x4D)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 15 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_15     (0x4E)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters T3T identifier 16 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_IDENTIFIERS_16     (0x4F)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters protocol type tag
 */
#define PHNCINFC_RFCONFIG_LF_PROTOCOL_TYPE          (0x50)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters PMM tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_PMM                (0x51)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters maximum number of T3T ID's supported tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_MAX                (0x52)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters Flags2 tag
 */
#define PHNCINFC_RFCONFIG_LF_T3T_FLAGS2             (0x53)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-F Discovery Parameters bit rate tag
 */
#define PHNCINFC_RFCONFIG_LF_CON_BITR_F             (0x54)

/* RFU                                          0x55 - 0x57 */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode ISO-DEP Discovery Parameters Frame Waiting Time Integer tag
 */
#define PHNCINFC_RFCONFIG_FWI                       (0x58)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode ISO-DEP Discovery Parameters History bytes tag
 */
#define PHNCINFC_RFCONFIG_LA_HIST_BY                (0x59)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode ISO-DEP Discovery Parameters higher layer information response tag
 */
#define PHNCINFC_RFCONFIG_LB_H_INFO_RESP            (0x5A)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode ISO-DEP Discovery Parameters bit rate tag
 */
#define PHNCINFC_RFCONFIG_LI_BIT_RATE               (0x5B)

/* RFU                                          0x5CB - 0x5F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-DEP Discovery Parameters Waiting Time tag
 */
#define PHNCINFC_RFCONFIG_WT                        (0x60)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-DEP Discovery Parameters ATR response general bytes tag
 */
#define PHNCINFC_RFCONFIG_ATR_RES_GEN_BYTES         (0x61)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Listen mode NFC-DEP Discovery Parameters ATR response config tag
 */
#define PHNCINFC_RFCONFIG_ATR_RES_CONFIG            (0x62)

/* RFU                                          0x63 - 0x7F */

/**
 * \ingroup grp_nci_nfc
 *
 * \brief NFCEE Action tag
 */

#define PHNCINFC_RFCONFIG_RF_FIELD_INFO             (0x80)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief NFCEE Action tag
 */
#define PHNCINFC_RFCONFIG_RF_NFCEE_ACTION           (0x81)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief NFC-DEP operation parameters tag
 */
#define PHNCINFC_RFCONFIG_NFCDEP_OP                 (0x82)

/* RFU                                          (0x84 - 0x9F) */

/* Reserved for Proprietary use                 (0xA0 - 0xFE) */
#define PHNCINFC_RFCONFIG_PROP_MIN                  (0xA0)
#define PHNCINFC_RFCONFIG_PROP_MAX                  (0xFE)

/* Reserved for Extension */
/* RFU                                          (0xFF) */

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Minimum number of Rf interfaces supported by NFCC
  */
#define PHNCINFC_RFCONFIG_MINSUPPORTED_RFINTFS          (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Size of number of Rf protocol to Rf interface mappings variable
  */
#define PHNCINFC_RFCONFIG_NUMMAPENTRIES_SIZE            (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Rf protocol to Rf interface mapping payload fields (each of size 1 byte)
  */
#define PHNCINFC_RFCONFIG_NUMPAYLOADFIELDS              (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set listen mode routing configuration payload default size
  */
#define PHNCINFC_RFCONFIG_LSTN_RTNG_DEFAULT_SIZE        (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of the 'type' and 'length' fields of listen mode routing parameters
  */
#define PHNCINFC_RFCONFIG_TLV_HEADER_LEN                (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of the value field of technology based routing parameters
  */
#define PHNCINFC_RFCONFIG_TECH_RTNG_VALUE_LEN           (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of the value field of protocol based routing parameters
  */
#define PHNCINFC_RFCONFIG_PROTO_RTNG_VALUE_LEN          (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of the first two parameters of the value field of the AID based routing
  */
#define PHNCINFC_RFCONFIG_AID_VALUE_DEFAULT_LEN         (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Minimum length of AID (application identifier)
  */
#define PHNCINFC_RFCONFIG_AID_MINLEN                    (5U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Maximum length of AID (application identifier)
  */
#define PHNCINFC_RFCONFIG_AID_MAXLEN                    (16U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NFCEE or DH-NFCEE ID reserved for future
  */
#define PHNCINFC_RFCONFIG_NFCEE_ID_RFU                  (255U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit offset for 'Switched Off' bit in power state byte
  */
#define PHNCINFC_RFCONFIG_SW_OFF_OFFSET                 (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit offset for 'Battery Off' bit in power state byte
  */
#define PHNCINFC_RFCONFIG_BATT_OFF_OFFSET               (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit offset for 'Switched On Sub-State 1' bit in power state byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB1_OFFSET               (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit offset for 'Switched On Sub-State 2' bit in power state byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB2_OFFSET               (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit offset for 'Switched On Sub-State 3' bit in power state byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB3_OFFSET               (5U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Switched on' in the 0th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_SW_ON(pByte,val)              (*(pByte) |= (uint8_t ) (val))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Switched off' in the 1st bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_SW_OFF(pByte,val)              (*(pByte) |= \
                                                         (uint8_t )((uint8_t )(val) <<  \
                                                         (uint8_t)PHNCINFC_RFCONFIG_SW_OFF_OFFSET))
/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Battery off' in the 2nd bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_BATT_OFF(pByte,val)            (*(pByte) |= \
                                                         (uint8_t )((uint8_t )(val) <<  \
                                                         (uint8_t)PHNCINFC_RFCONFIG_BATT_OFF_OFFSET))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Switched on Sub-State 1' in the 3rd bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB1(pByte,val)          (*(pByte) |= \
                                                         (uint8_t )((uint8_t )(val) <<  \
                                                         (uint8_t)PHNCINFC_RFCONFIG_SW_ON_SUB1_OFFSET))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Switched on Sub-State 2' in the 4th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB2(pByte,val)          (*(pByte) |= \
                                                         (uint8_t )((uint8_t )(val) <<  \
                                                         (uint8_t)PHNCINFC_RFCONFIG_SW_ON_SUB2_OFFSET))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set/Clear 1 bit(b0) 'Switched on Sub-State 3' in the 5th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB3(pByte,val)          (*(pByte) |= \
                                                         (uint8_t )((uint8_t )(val) <<  \
                                                         (uint8_t)PHNCINFC_RFCONFIG_SW_ON_SUB3_OFFSET))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit mask to get Switch ON bit field of power state
  */
#define PHNCINFC_RFCONFIG_BITMASK                       (0x01U)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Bit mask to get Switch OFF bit field of power state
 */
#define PHNCINFC_RFCONFIG_SW_OFF_BITPOS                 (0x01U)

/**
 * \ingroup grp_nci_nfc
 *
 * \brief Bit mask to get Battery OFF bit field of power state
 */
#define PHNCINFC_RFCONFIG_BATT_OFF_BITPOS               (0x02U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit mask to get Switch ON Sub-State 1 bit field of power state
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB1_BITPOS             (0x03U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit mask to get Switch ON Sub-State 2 bit field of power state
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB2_BITPOS             (0x04U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit mask to get Switch ON Sub-State 3 bit field of power state
  */
#define PHNCINFC_RFCONFIG_SW_ON_SUB3_BITPOS             (0x05U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b0) 'Switched on' in the 0th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_SW_ON(pByte)              ((pByte) & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b1) 'Switched off' in the 1st bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_SW_OFF(pByte)             (((pByte) >> (PHNCINFC_RFCONFIG_SW_OFF_BITPOS)) \
                                                          & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b2) 'Battery off' in the 2nd bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_BATT_OFF(pByte)             (((pByte) >> (PHNCINFC_RFCONFIG_BATT_OFF_BITPOS)) \
                                                            & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b0) 'Switched on sub-state 1' in the 3rd bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_SW_ON_SUB1(pByte)            (((pByte) >> (PHNCINFC_RFCONFIG_SW_ON_SUB1_BITPOS)) \
                                                            & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b0) 'Switched on sub-state 2' in the 4th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_SW_ON_SUB2(pByte)            (((pByte) >> (PHNCINFC_RFCONFIG_SW_ON_SUB2_BITPOS)) \
                                                            & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Get 1 bit(b0) 'Switched on sub-state 3' in the 5th bit position of the input byte
  */
#define PHNCINFC_RFCONFIG_GET_SW_ON_SUB3(pByte)            (((pByte) >> (PHNCINFC_RFCONFIG_SW_ON_SUB3_BITPOS)) \
                                                            & (PHNCINFC_RFCONFIG_BITMASK))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Maximum value of listen mode Nfc-A Bit Frame SDD
  */
#define PHNCINFC_RFCONFIG_BITFRAME_SDD_MAX              (0xFFU)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Maximum value of listen mode Nfc-A platform configuration
  */
#define PHNCINFC_RFCONFIG_PLATFORM_CONF_MAX             (15U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NFCID1 4 bytes length
  */
#define PHNCINFC_RFCONFIG_NFCID1_NOTPRESENT             (0U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NFCID1 4 bytes length
  */
#define PHNCINFC_RFCONFIG_NFCID1_4BYTES                 (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NFCID1 7 bytes length
  */
#define PHNCINFC_RFCONFIG_NFCID1_7BYTES                 (7U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NFCID1 10 bytes length
  */
#define PHNCINFC_RFCONFIG_NFCID1_10BYTES                 (10U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Poll mode Nfc-Dep DID bit field offset
  */
#define PHNCINFC_RFCONFIG_PNFCDEP_DID_BITMASK           (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Poll mode Nfc-Dep length reduction (LR) bit field offset
  */
#define PHNCINFC_RFCONFIG_PNFCDEP_LR_BITMASK            (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-A IsoDep protocol support bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCA_ISODEP_OFFSET           (5U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-A NfcDep protocol support bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCA_NFCDEP_OFFSET           (6U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-B ADC FO bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCB_ADCFO_OFFSET            (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-F Bit rate listen 212kbps bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCF_LSTN212_OFFSET          (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-F Bit rate listen 424kbps bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCF_LSTN424_OFFSET          (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-F NfcDep protocol support bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCF_NFCDEP_OFFSET           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 1
  */
#define PHNCINFC_RFCONFIG_T3TID1                        (0U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 2
  */
#define PHNCINFC_RFCONFIG_T3TID2                        (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 3
  */
#define PHNCINFC_RFCONFIG_T3TID3                        (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 4
  */
#define PHNCINFC_RFCONFIG_T3TID4                        (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 5
  */
#define PHNCINFC_RFCONFIG_T3TID5                        (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 6
  */
#define PHNCINFC_RFCONFIG_T3TID6                        (5U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 7
  */
#define PHNCINFC_RFCONFIG_T3TID7                        (6U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 8
  */
#define PHNCINFC_RFCONFIG_T3TID8                        (7U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 9
  */
#define PHNCINFC_RFCONFIG_T3TID9                        (8U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 10
  */
#define PHNCINFC_RFCONFIG_T3TID10                       (9U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 11
  */
#define PHNCINFC_RFCONFIG_T3TID11                       (10U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 12
  */
#define PHNCINFC_RFCONFIG_T3TID12                       (11U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 13
  */
#define PHNCINFC_RFCONFIG_T3TID13                       (12U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 14
  */
#define PHNCINFC_RFCONFIG_T3TID14                       (13U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 15
  */
#define PHNCINFC_RFCONFIG_T3TID15                       (14U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief T3T Identifier 16
  */
#define PHNCINFC_RFCONFIG_T3TID16                       (15U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Listen Nfc-Dep ATR RES config Length Reduction (LR) bit field offset
  */
#define PHNCINFC_RFCONFIG_LNFCDEP_LR_OFFSET             (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NfcDep Operation parameter - Attention command bit field offset
  */
#define PHNCINFC_RFCONFIG_NFCDEP_ATTCMD_OFFSET          (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NfcDep Operation parameter - Information PDU bit field offset
  */
#define PHNCINFC_RFCONFIG_NFCDEP_INFPDU_OFFSET          (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief NfcDep Operation parameter - Maximum Transport data bytes bit field offset
  */
#define PHNCINFC_RFCONFIG_NFCDEP_MAXBYTES_OFFSET        (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Technology based routing bit offset
  */
#define PHNCINFC_RFCONFIG_TECH_ROUTING_OFFSET           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Protocol based routing bit offset
  */
#define PHNCINFC_RFCONFIG_PRO_ROUTING_OFFSET            (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Aid based routing bit offset
  */
#define PHNCINFC_RFCONFIG_AID_ROUTING_OFFSET            (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Poll Nfc-Dep configurable parameters
  */
#define PHNCINFC_RFCONFIG_PNFCDEP_PARAMS                (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-Dep Speed
  */
#define PHNCINFC_RFCONFIG_PNFCDEP_SPEED_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-Dep ATR_CONFIG parameter
  */
#define PHNCINFC_RFCONFIG_PNFCDEP_ATRCONFIG_LEN         (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Poll Iso-Dep configurable parameters
  */
#define PHNCINFC_RFCONFIG_PISODEP_PARAMS                (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Iso-Dep Bit Rate parameter
  */
#define PHNCINFC_RFCONFIG_PISODEP_BITRATE_LEN           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Poll Nfc-F configurable parameters
  */
#define PHNCINFC_RFCONFIG_PNFCF_PARAMS                  (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-F Bit Rate parameter
  */
#define PHNCINFC_RFCONFIG_PNFCF_BITRATE_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Poll Nfc-B configurable parameters
  */
#define PHNCINFC_RFCONFIG_PNFCB_PARAMS                  (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-B Afi parameter
  */
#define PHNCINFC_RFCONFIG_PNFCB_AFI_LEN                 (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-B Bail Out parameter
  */
#define PHNCINFC_RFCONFIG_PNFCB_BAILOUT_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Poll Nfc-A configurable parameters
  */
#define PHNCINFC_RFCONFIG_PNFCA_PARAMS                  (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Poll Nfc-A Bail Out parameter
  */
#define PHNCINFC_RFCONFIG_PNFCA_BAILOUT_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Listen Nfc-Dep configurable parameters
  */
#define PHNCINFC_RFCONFIG_LNFCDEP_PARAMS                (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-Dep Waiting Time parameter
  */
#define PHNCINFC_RFCONFIG_LNFCDEP_WT_LEN                (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-Dep ATR_RES_CONFIG parameter
  */
#define PHNCINFC_RFCONFIG_LNFCDEP_ATRRESCONFIG_LEN      (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Listen Iso-Dep configurable parameters
  */
#define PHNCINFC_RFCONFIG_LISODEP_PARAMS                (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Iso-Dep Frame Waiting Time parameter
  */
#define PHNCINFC_RFCONFIG_LISODEP_FWI_LEN               (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Iso-Dep Bit Rate parameter
  */
#define PHNCINFC_RFCONFIG_LISODEP_BITRATE_LEN           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Minimum number of Listen Nfc-F configurable parameters
  */
#define PHNCINFC_RFCONFIG_LNFCF_PARAMS                  (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-F Bit Rate parameter
  */
#define PHNCINFC_RFCONFIG_LNFCF_BITRATE_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-F Protocol type parameter
  */
#define PHNCINFC_RFCONFIG_LNFCF_PROTOTYPE_LEN           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-F Protocol type parameter
  */
#define PHNCINFC_RFCONFIG_LNFCF_PROTOTYPE_LEN           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-F T3t flags parameter
  */
#define PHNCINFC_RFCONFIG_LNFCF_T3TFLAGS_LEN            (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Listen Nfc-B configurable parameters
  */
#define PHNCINFC_RFCONFIG_LNFCB_PARAMS                  (5U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-B T3t flags parameter
  */
#define PHNCINFC_RFCONFIG_LNFCB_SENSBINFO_LEN           (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-B Startup frame guard time integer (SFGI) parameter
  */
#define PHNCINFC_RFCONFIG_LNFCB_SFGI_LEN                (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-B ADC_FO (ADC Frame options) parameter
  */
#define PHNCINFC_RFCONFIG_LNFCB_ADCFO_LEN               (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Listen Nfc-A configurable parameters
  */
#define PHNCINFC_RFCONFIG_LNFCA_PARAMS                  (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-A Bit rate SDD parameter
  */
#define PHNCINFC_RFCONFIG_LNFCA_BITRATE_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-A Platform config parameter
  */
#define PHNCINFC_RFCONFIG_LNFCA_PLATCONFIG_LEN          (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Listen Nfc-A SEL_INFO parameter
  */
#define PHNCINFC_RFCONFIG_LNFCA_SELINFO_LEN             (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Number of Common configurable parameters
  */
#define PHNCINFC_RFCONFIG_COMMON_PARAMS                 (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Minimum length of Total duration parameter
  */
#define PHNCINFC_RFCONFIG_TOTALDURATION_LEN             (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Connection device limit parameter
  */
#define PHNCINFC_RFCONFIG_CONDEVLIMIT_LEN               (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Rf field info parameter
  */
#define PHNCINFC_RFCONFIG_RFFIELDINFO_LEN               (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of Nfcee action parameter
  */
#define PHNCINFC_RFCONFIG_NFCEEACTION_LEN               (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Length of NfcDep Operation parameter
  */
#define PHNCINFC_RFCONFIG_NFCDEPOP_LEN                  (1U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit position of TR0 parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_TR0_BITPOS                    (6U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit position of TR1 parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_TR1_BITPOS                    (4U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit position of EOS parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_EOS_BITPOS                    (3U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Bit position of EOS parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SOS_BITPOS                    (2U)

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set TR0 parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SET_TR0(Buff,bVal)          ((Buff) | ((bVal) << PHNCINFC_RFCONFIG_TR0_BITPOS))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set TR1 parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SET_TR1(Buff,bVal)          ((Buff) | ((bVal) << PHNCINFC_RFCONFIG_TR1_BITPOS))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set TR2 parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SET_TR2(Buff,bVal)          ((Buff) | (bVal))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set Suppression of EOS parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SET_SUPP_EOS(Buff,bVal)     ((Buff) | ((bVal) << PHNCINFC_RFCONFIG_EOS_BITPOS))

/**
  * \ingroup grp_nci_nfc
  *
  * \brief Set Suppression of SOS parameter of Nfc-B data exchange configuration
  */
#define PHNCINFC_RFCONFIG_SET_SUPP_SOS(Buff,bVal)     ((Buff) | ((bVal) << PHNCINFC_RFCONFIG_SOS_BITPOS))

/** Value of DID in NFC-DEP Discovery parameter */
#define PHNCINFC_RFCONFIG_PNGET_DIDVAL                    (0x04)

/** Value of Length Reduction in NFC-DEP Discovery parameter */
#define PHNCINFC_RFCONFIG_PNGET_LRVAL                     (0x30)

/** Value of NFC-DEP protocol in Listen-A Discovery parameter */
#define PHNCINFC_RFCONFIG_LANFCDEP_PROTOCOL               (0x40)

/** Value of ISO-DEP protocol in Listen-A Discovery parameter */
#define PHNCINFC_RFCONFIG_LAISODEP_PROTOCOL               (0x20)

/** Value of ISO-DEP protocol in Listen-B Discovery parameter */
#define PHNCINFC_RFCONFIG_LBISODEP_PROTOCOL_SUPPORT       (0x01)

/** Value to fetch the DID from the byte in Listen-B params */
#define PHNCINFC_RFCONFIG_LBGET_DIDVAL                    (0x01)

/** Value to fetch the ADC from the byte in Listen-B params */
#define PHNCINFC_RFCONFIG_LBADC_CODING                    (0x03)

/** Value to check whether 212kbps is supported in Listen-F params */
#define PHNCINFC_RFCONFIG_LF_212KBPS                      (0x02)

/** Value to check whether 424kbps is supported in Listen-F params */
#define PHNCINFC_RFCONFIG_LF_424KBPS                      (0x04)

/** Value of NFC-DEP protocol in Listen-F Discovery parameter */
#define PHNCINFC_RFCONFIG_LFNFCDEP_PROTOCOL_SUPPORT       (0x02)

/** Value of Length Reduction in Listen-NFCDEP Discovery parameter */
#define PHNCINFC_RFCONFIG_LNNFCDEP_LR                     (0x03)

/** Value of Bit field indicates NFC-DEP Target SHALL NOT send RTOX requests */
#define PHNCINFC_RFCONFIG_RTOX_REQ                                (0x01)

/** Value of Bit field indicates NFC-DEP Initiator SHALL use
    the ATTENTION command only as part of the error recovery procedure */
#define PHNCINFC_RFCONFIG_NFCDEP_ATTCMD                           (0x02)

/** Value of Bit field indicates Information PDU with no
    transport data bytes SHALL NOT be sent */
#define PHNCINFC_RFCONFIG_NFCDEP_INFPDU                           (0x04)

/** Value of Bit field indicates all PDUs indicating chaining
    (MI bit set) SHALL use the maximum number of transport data bytes. */
#define PHNCINFC_RFCONFIG_NFCDEP_MAXBYTES                         (0x08)

/** Value indicates Count of Total number of Configuration parameters */
#define PHNCINFC_RFCONFIG_TOTALPARAM_COUNT                         (256)

/** Macro to Fetch the Parameter from the byte */
#define PHNCINFC_RFCONFIG_GETPARAM(Byte,val)              ((Byte) & ((uint8_t )(val)))

/* Set config timeout */
#define PHNCINFC_RFCONFIG_SETCONFIG_TIMEOUT                       (2000U)

/** Size of type and length fields (header) of a TLV */
#define PHNCINFC_RFCONFIG_TLV_HEADER_SIZE                         (2U)

/** Size of number of configurations field of Set config command */
#define PHNCINFC_RFCONFIG_NUM_CONFIGS_SIZE                        (1U)

extern phNciNfc_SequenceP_t gphNciNfc_SetConfigSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_SetConfigOptSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_GetConfigSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_GetConfigRawSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_ProtoIfMapSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_GetRtngConfigSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_RfParamUpdateSequence[];

/*===========================================================================*

* Public functions:

*============================================================================*/

extern NFCSTATUS
phNciNfc_ValidateSetConfParams(
                                pphNciNfc_RfDiscConfigParams_t pDiscConfParams,
                                uint16_t *pParamSize,
                                uint8_t *pNumParams
                               );

extern NFCSTATUS
phNciNfc_BuildSetConfPayload(
                              pphNciNfc_RfDiscConfigParams_t pDiscConfig,
                              uint8_t *pPayloadBuff,
                              uint16_t wPayloadSize
                             );

extern NFCSTATUS
phNciNfc_ValidateDiscMapParams(
                               uint8_t bNumMapEntries,
                               pphNciNfc_MappingConfig_t pProtoIfMapping
                              );


extern NFCSTATUS
phNciNfc_VerifySupportedRfIntfs(
                                pphNciNfc_Context_t pNciCtx,
                                uint8_t bNumMapEntries,
                                pphNciNfc_MappingConfig_t    pProtoIfMapping
                               );

extern void
phNciNfc_BuildDiscMapCmdPayload(
    _Out_writes_((bNumMapEntries * PHNCINFC_RFCONFIG_NUMPAYLOADFIELDS) + PHNCINFC_RFCONFIG_NUMMAPENTRIES_SIZE)
                                uint8_t *pBuffer,
                                _In_ uint8_t bNumMapEntries,
                                _In_ pphNciNfc_MappingConfig_t pProtoIfMapping
                            );

extern NFCSTATUS
phNciNfc_ValidateSetRtngParams(
                               uint8_t                bNumRtngEntries,
                               pphNciNfc_RtngConfig_t pRtngConfig,
                               uint16_t               *pSize
                              );

extern NFCSTATUS
phNciNfc_VerifySupportedRouting(pphNciNfc_NfccFeatures_t pNfccFeatures,
                                uint8_t                bNumRtngEntries,
                                pphNciNfc_RtngConfig_t pRtngConfig
                                );

extern void
phNciNfc_BuildSetLstnRtngCmdPayload(uint8_t                *pBuffer,
                                    uint8_t                bNumRtngEntries,
                                    pphNciNfc_RtngConfig_t pRtngConfig,
                                    uint8_t                bMore
                                    );

extern NFCSTATUS
phNciNfc_BuildRfParamUpdateCmdPayload(uint8_t *pBuff,
                                      uint8_t bNumParams,
                                      pphNciNfc_RfParamUpdate_t pRfParams
                                      );

extern NFCSTATUS
phNciNfc_SetRtngCmdHandler(pphNciNfc_Context_t pNciContext);

extern NFCSTATUS
phNciNfc_GetRfParams( pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                     uint8_t *pRfParams,uint8_t *pParamCount,
                     uint8_t *pParamLen
                     );
