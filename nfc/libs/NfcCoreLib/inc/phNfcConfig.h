/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

/*=========== Discovery parameter configuration ===========*/

#define NFC_LA_BIT_FRAME_SDD        (0x04U)
#define NFC_LA_PLATFORM_CONFIG      (0x03U)

#define NFC_LA_SEL_INFO

#define NFC_DTA_LN_WT               (0x08)
#define NFC_LN_WT                   (0x07)

#define NFC_PF_BIT_RATE
#define NFC_PN_NFC_DEP_SPEED

#define NFC_LF_PROTOCOL_TYPE_NFC_DEP_SUPPORT
#define NFC_LI_BIT_RATE

/*===========  NFC-A POLL MODE DISCOVERY PARAMS ===========*/

/*=========== NFC-B POLL MODE DISCOVERY PARAMS ===========*/

/**<  Application Family Identifier
      0x00U     No bail out during Poll Mode in Discovery activity
      0x01U     Bail out when NFC-B Technology has been detected
                during Poll Mode in Discovery activity
      */
#ifndef NFC_PB_AFI
#define NFC_PB_AFI_DEFAULT         0x00U
#endif

/**<  Param1 for ATTRIB command for NFC-B technology
      0x00U     Default value
      Note : This is a Read-Only parameter.
             DH shall not attempt to write this parameter.
      */
#ifndef NFC_PB_ATTRIB_PARAM1
#define NFC_PB_ATTRIB_PARAM1_DEFAULT         0x00U
#endif

/*===========  NFC-F POLL MODE DISCOVERY PARAMS ===========*/

/**<  Bit Rate for NFC-F technology
       0x00     106Kbits/Sec
       0x01     212Kbits/Sec
       0x02     424Kbits/Sec
       0x03     848Kbits/Sec
       0x04     1695Kbits/Sec
       0x05     3390Kbits/Sec
       0x06     6780Kbits/Sec
      */
#ifndef NFC_PF_BIT_RATE
#define NFC_PF_BIT_RATE_DEFAULT         0x01U
#endif

/*=========== NFC-ISODEP POLL MODE DISCOVERY PARAMS ===========*/

/**<  Higher layer INF field of the ATTRIB Command.
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_PB_H_INFO
#define NFC_PB_H_INFO_DEFAULT
#endif

#ifdef NFC_PB_H_INFO
#define NFC_PB_H_INFO_VALUE {0x00, 0x01, 0x02}
#define NFC_PB_H_INFO_LEN   (0x03)
#endif

/**<  Bit Rate for NFC-ISODEP parameters
       0x00     106Kbits/Sec
       0x01     212Kbits/Sec
       0x02     424Kbits/Sec
       0x03     848Kbits/Sec
       0x04     1695Kbits/Sec
       0x05     3390Kbits/Sec
       0x06     6780Kbits/Sec
      */

#define NFC_PI_BIT_RATE_106        0x00U
#define NFC_PI_BIT_RATE_212        0x01U
#define NFC_PI_BIT_RATE_424        0x02U
#define NFC_PI_BIT_RATE_848        0x03U

#define NFC_PI_BIT_RATE

#ifdef NFC_PI_BIT_RATE
#define NFC_PI_BIT_RATE_SPEED      NFC_PI_BIT_RATE_106
#endif

/*=========== NFC-NFCDEP POLL MODE DISCOVERY PARAMS ===========*/

/**<  Bit rates for Data exchange in NFC-DEP.
      0x00U     Highest available bit rates used for data exchange.
      0x01U     Use Same bit rates for data exchanges as were used for
                Device Activation.
      */
#ifndef NFC_PN_NFC_DEP_SPEED
#define NFC_PN_NFC_DEP_SPEED_DEFAULT         0x00U
#endif

#ifdef NFC_PN_NFC_DEP_SPEED
#define NFC_PN_NFC_DEP_SPEED_VALUE          0x00U
#endif

/**<  General Bytes for ATR_REQ
      Default :Empty
      */
#ifndef NFC_PN_ATR_REQ_GEN_BYTES
#define NFC_PN_ATR_REQ_GEN_BYTES_DEFAULT
#endif

#ifdef NFC_PN_ATR_REQ_GEN_BYTES
#define NFC_NFC_PN_ATR_REQ_GEN_BYTES_VALUE {0x00, 0x01, 0x02}
#define NFC_NFC_PN_ATR_REQ_GEN_BYTES_LEN   (0x03)
#endif

/**<  Configuration to be used in the Optional Parameters
      */
#ifndef NFC_PN_ATR_REQ_CONFIG
#define NFC_PN_ATR_REQ_DID                  0x00
#define NFC_PN_ATR_REQ_LR                   0x03
#endif

/*=========== NFC-A LISTEN MODE DISCOVERY PARAMS ===========*/

/**<  Bit Frame SDD value to be sent in Byte 1 of SENS_RES.
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LA_BIT_FRAME_SDD
#define NFC_LA_BIT_FRAME_SDD_DEFAULT         0x00U
#endif

/**<  Platform Configuration value to be sent in Byte 2 of SENS_RES
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LA_PLATFORM_CONFIG
#define NFC_LA_PLATFORM_CONFIG_DEFAULT         0x00U
#endif

/**<  This value is used to generate SEL_RES
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LA_SEL_INFO
#define NFC_LA_SEL_INFO_DEFAULT         0x00U
#endif

/**<  NFCID1 value
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LA_NFCID1
#define NFC_LA_NFCID1_DEFAULT
#endif

#ifdef NFC_LA_NFCID1
#define NFC_LA_NFCID1_VALUE {0x00, 0x01, 0x02, 0x04}
#define NFC_LA_NFCID1_LEN   (0x04)
#endif

/*=========== NFC-B LISTEN MODE DISCOVERY PARAMS ===========*/

/**<  Used to generate Byte 2 of Protocol Info within SENSB_RES.
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LB_SENSB_INFO
#define NFC_LB_SENSB_INFO_DEFAULT         0x00U
#endif

/**<  NFCID0 value
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LB_NFCID0
#define NFC_LB_NFCID0_DEFAULT
#endif

#ifdef NFC_LB_NFCID0
#define NFC_LB_NFCID0_VALUE {0x00, 0x01, 0x02, 0x04}
#define NFC_LB_NFCID0_LEN   (0x04)
#endif

/**<  Application Data (Bytes 6-9) of SENSB_RES.
      Default : All octets are set to 0x00.
      */
#ifndef NFC_LB_APPLICATION_DATA
#define NFC_LB_APPLICATION_DATA_DEFAULT
#endif

#ifdef NFC_LB_APPLICATION_DATA
#define NFC_LB_APPLICATION_DATA_VALUE {0x00, 0x00, 0x00, 0x00}
#define NFC_LB_APPLICATION_DATA_LEN   (0x04)
#endif

/**<  Start-Up Frame Guard Time
      Default : NFCC decides according to Digital Specification
      */
#ifndef NFC_LB_SFGI
#define NFC_LB_SFGI_DEFAULT         0x00U
#endif

/**<  Byte 3 of Protocol Info within SENSB_RES
      */
#ifndef NFC_LB_ADC_FO
#define NFC_LB_ADC_FO_DEFAULT         0x05U
#endif

/*=========== NFC-F LISTEN MODE DISCOVERY PARAMS ===========*/

#define BITRATE_LISTEN_F_UNDEF      0
#define BITRATE_LISTEN_F_212        1
#define BITRATE_LISTEN_F_424        2
#define BITRATE_LISTEN_F_212_424    3

#define NFC_LISTEN_F_BITRATE_SEL    BITRATE_LISTEN_F_212_424 /* Set to one of BITRATE_LISTEN_F_<value> */

#define NFC_LF_NFCID2_LENGTH        8

#if (NFC_LISTEN_F_BITRATE_SEL)
/* Enable Listen F Bitrate configuration */
#define NFC_LF_CON_BITR_F           1
#endif

/**<  Protocols supported by the NFC Forum Device in Listen Mode for NFC-F
      */
#ifndef NFC_LF_PROTOCOL_TYPE_NFC_DEP_SUPPORT
#define NFC_LF_PROTOCOL_TYPE_DEFAULT         0x01U
#endif

/* LF_T3T_MAX is a Read Only parameter. Hence DH shall not set this parameter */
/**<  The maximum index of LF_T3T_IDENTIFIERS supported by the NFCC.
      */
#ifndef NFC_LF_T3T_MAX
#define NFC_LF_T3T_MAX_DEFAULT         0x00U
#endif

/**<  LF_T3T_IDENTIFIERS
      Octet 0 and Octet 1 indicate the System Code of a Type 3 Tag
      Emulation occurring on the DH.
      Octet 2 – Octet 9 indicates NFCID2 for the Type 3 Tag Platform.

      Default :Octet 0 and 1 SHALL be set to 0xFF.
                Octet 2 SHALL be set to 0x02.
                Octet 3 SHALL be set to 0xFE.
                Octets 4-9 SHALL be set to 0x00
      */

/**<  LF_T3T_IDENTIFIERS 1 */
#ifndef NFC_LF_T3T_IDENTIFIERS_1
#define NFC_LF_T3T_IDENTIFIERS_1_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_1
#define NFC_LF_T3T_IDENTIFIERS_1_VALUE {0xFF, 0xFF, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_1_LEN   (0x0A)
#endif

/**<  LF_T3T_IDENTIFIERS 2 */
#ifndef NFC_LF_T3T_IDENTIFIERS_2
#define NFC_LF_T3T_IDENTIFIERS_2_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_2
#define NFC_LF_T3T_IDENTIFIERS_2_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_2_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 3 */
#ifndef NFC_LF_T3T_IDENTIFIERS_3
#define NFC_LF_T3T_IDENTIFIERS_3_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_3
#define NFC_LF_T3T_IDENTIFIERS_3_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_3_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 4 */
#ifndef NFC_LF_T3T_IDENTIFIERS_4
#define NFC_LF_T3T_IDENTIFIERS_4_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_4
#define NFC_LF_T3T_IDENTIFIERS_4_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_4_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 5 */
#ifndef NFC_LF_T3T_IDENTIFIERS_5
#define NFC_LF_T3T_IDENTIFIERS_5_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_5
#define NFC_LF_T3T_IDENTIFIERS_5_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_5_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 6 */
#ifndef NFC_LF_T3T_IDENTIFIERS_6
#define NFC_LF_T3T_IDENTIFIERS_6_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_6
#define NFC_LF_T3T_IDENTIFIERS_6_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_6_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 7 */
#ifndef NFC_LF_T3T_IDENTIFIERS_7
#define NFC_LF_T3T_IDENTIFIERS_7_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_7
#define NFC_LF_T3T_IDENTIFIERS_7_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_7_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 8 */
#ifndef NFC_LF_T3T_IDENTIFIERS_8
#define NFC_LF_T3T_IDENTIFIERS_8_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_8
#define NFC_LF_T3T_IDENTIFIERS_8_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_8_LEN   (0x0A)
#endif

/**<  LF_T3T_IDENTIFIERS 9 */
#ifdef NFC_LF_T3T_IDENTIFIERS_9
#define NFC_LF_T3T_IDENTIFIERS_9_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_9
#define NFC_LF_T3T_IDENTIFIERS_9_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_9_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 10 */
#ifndef NFC_LF_T3T_IDENTIFIERS_10
#define NFC_LF_T3T_IDENTIFIERS_10_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_10
#define NFC_LF_T3T_IDENTIFIERS_10_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_10_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 11 */
#ifndef NFC_LF_T3T_IDENTIFIERS_11
#define NFC_LF_T3T_IDENTIFIERS_11_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_11
#define NFC_LF_T3T_IDENTIFIERS_11_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_11_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 12 */
#ifndef NFC_LF_T3T_IDENTIFIERS_12
#define NFC_LF_T3T_IDENTIFIERS_12_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_12
#define NFC_LF_T3T_IDENTIFIERS_12_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_12_LEN   (0x0A)
#endif


/**<  LF_T3T_IDENTIFIERS 13 */
#ifndef NFC_LF_T3T_IDENTIFIERS_13
#define NFC_LF_T3T_IDENTIFIERS_13_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_13
#define NFC_LF_T3T_IDENTIFIERS_13_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_13_LEN   (0x0A)
#endif

/**<  LF_T3T_IDENTIFIERS 14 */
#ifndef NFC_LF_T3T_IDENTIFIERS_14
#define NFC_LF_T3T_IDENTIFIERS_14_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_14
#define NFC_LF_T3T_IDENTIFIERS_14_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_14_LEN   (0x0A)
#endif

/**<  LF_T3T_IDENTIFIERS 15 */
#ifndef NFC_LF_T3T_IDENTIFIERS_15
#define NFC_LF_T3T_IDENTIFIERS_15_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_15
#define NFC_LF_T3T_IDENTIFIERS_15_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_15_LEN   (0x0A)
#endif

/**<  LF_T3T_IDENTIFIERS 16 */
#ifndef NFC_LF_T3T_IDENTIFIERS_16
#define NFC_LF_T3T_IDENTIFIERS_16_DEFAULT
#endif

#ifdef NFC_LF_T3T_IDENTIFIERS_16
#define NFC_LF_T3T_IDENTIFIERS_16_VALUE {0xFF, 0xFF, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define NFC_LF_T3T_IDENTIFIERS_16_LEN   (0x0A)
#endif

/**<  PAD0, PAD1, MRTI_check, MRTI_update and PAD2 of SENSF_RES
      Default : All octets are set to 0xFF.
      */
#ifndef NFC_LF_T3T_PMM
#define NFC_LF_T3T_PMM_DEFAULT
#endif

#ifdef NFC_LF_T3T_PMM
#define NFC_LF_T3T_PMM_VALUE {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define NFC_LF_T3T_PMM_LEN   (0x08)
#endif

/**<  PAD0, PAD1, MRTI_check, MRTI_update and PAD2 of SENSF_RES
      Default : All octets are set to 0x00.
      */
#ifndef NFC_LF_T3T_FLAGS
#define NFC_LF_T3T_FLAGS_DEFAULT
#endif

#ifdef NFC_LF_T3T_FLAGS
#define NFC_LF_T3T_FLAGS_VALUE {0x00, 0x00}
#define NFC_LF_T3T_FLAGS_LEN   (0x02)
#endif

/*=========== NFC-ISODEP LISTEN MODE DISCOVERY PARAMS ===========*/

/**<  Frame Waiting Time Integer
      */
#ifndef NFC_LI_FWI
#define NFC_LI_FWI_DEFAULT         0x04U
#endif

/**<  Historical Bytes (only applicable for Type 4A Tag)
      Default :empty (do not send historical bytes)
      */
#ifndef NFC_LA_HIST_BY
#define NFC_LA_HIST_BY_DEFAULT
#endif

/**<  Higher Layer - Response field of the ATTRIB response
      Default :empty (send ATTRIB response without Higher Layer – Response field).
      */
#ifndef NFC_LB_H_INFO_RESP
#define NFC_LB_H_INFO_RESP_DEFAULT
#endif

/**<  Maximum supported bit rate.
       0x00     106Kbits/Sec
       0x01     212Kbits/Sec
       0x02     424Kbits/Sec
       0x03     848Kbits/Sec
       0x04     1695Kbits/Sec
       0x05     3390Kbits/Sec
       0x06     6780Kbits/Sec
       Default : 0x00
      */
#ifndef NFC_LI_BIT_RATE
#define NFC_LI_BIT_RATE_DEFAULT         0x00U
#endif

#ifdef NFC_LI_BIT_RATE
#define NFC_LI_BIT_RATE_SPEED           0x00U
#endif

/*=========== NFC-NFCDEP LISTEN MODE DISCOVERY PARAMS ===========*/

/**<  Waiting Time Integer
      */
#ifndef NFC_LN_WT
#define NFC_LN_WT_DEFAULT         14U
#endif

/**<  General Bytes in ATR_RES
      Default :: empty (no General Bytes SHALL be sent in ATR_RES).
      */
#ifndef NFC_LN_ATR_RES_GEN_BYTES
#define NFC_LN_ATR_RES_GEN_BYTES_DEFAULT
#endif

/**<  Used to generate the Optional parameters (PPT) in ATR_RES.
      Default : (NFCC indicates a maximum payload size of 254 bytes).
      */
#ifndef NFC_LN_ATR_RES_CONFIG
#define NFC_LN_ATR_RES_CONFIG_DEFAULT         0x30U
#endif

/*=========== OTHER PARAMS ===========*/

/** Enable NFC-DEP listen device to send RTOX request */
#define PH_LIBNFC_ENABLE_NFCDEP_RTOX                1

/** Macro to Set configuration to get RF_FIELD_INFO_NTF from NFCC */
#define PH_LIBNFC_ENABLE_RFFIELD_INFO_NTF           1

/** Macro to enable RF NFCEE Action NTF */
#define PH_LIBNFC_ENABLE_RF_NFCEE_ACTION_NTF        1

/*=========== COMMON DISCOVERY PARAMS ===========*/

#define NFC_TOTAL_DURATION

/**<  Total Duration of the single discovery period in [ms].
      Default :NFCC decides.
      */
#ifndef NFC_TOTAL_DURATION
#define NFC_TOTAL_DURATION_DEFAULT
#endif

#ifdef NFC_TOTAL_DURATION
#define NFC_TOTAL_DURATION_VALUE 0x64
#endif

/**<  Number of Devices for which Collision resolution can be done.
      Default :NFCC decides (based on its capabilities).
      */
#ifndef NFC_CON_DEVICES_LIMIT
#define NFC_CON_DEVICES_LIMIT_DEFAULT         0x00U
#endif

/**< Max number of remote devices supported */

#ifndef MAX_REMOTE_DEVICES
#define MAX_REMOTE_DEVICES        0x0A
#endif
