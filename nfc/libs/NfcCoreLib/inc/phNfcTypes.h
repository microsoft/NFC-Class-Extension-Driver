/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

typedef unsigned int BitField_t;

#include <stdio.h>
#define snprintf _snprintf

#include <phNfcConfig.h>

/* Basic Type Definitions */

#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed   char   int8_t;         /**<  \ingroup grp_nfc_common
                                               8 bit signed integer */
#endif

#ifndef __int16_t_defined
#define __int16_t_defined
typedef signed   short  int16_t;        /**< \ingroup grp_nfc_common
                                             16 bit signed integer */
#endif

#ifndef __int32_t_defined
#define __int32_t_defined
typedef signed   long   int32_t;        /**< \ingroup grp_nfc_common
                                             32 bit signed integer */
#endif

#ifndef __uint8_t_defined
#define __uint8_t_defined
typedef unsigned char   uint8_t;        /**<  \ingroup grp_nfc_common
                                              8 bit unsigned integer */
#endif

#ifndef __uint16_t_defined
#define __uint16_t_defined
typedef unsigned short  uint16_t;       /**< \ingroup grp_nfc_common
                                             16 bit unsigned integer */
#endif

#ifndef __uint32_t_defined
#define __uint32_t_defined
typedef unsigned long   uint32_t;       /**< \ingroup grp_nfc_common
                                             32 bit unsigned integer */
#endif

typedef uint8_t         utf8_t;             /**< \ingroup grp_nfc_common
                                             UTF8 Character String */

typedef uint8_t         bool_t;             /**< \ingroup grp_nfc_common
                                                boolean data type */

typedef
_Return_type_success_(return == 0)
uint16_t        NFCSTATUS;                  /**< \ingroup grp_nfc_common
                                                NFC-FRI and HAL return values
                                                \ref phNfcStatus.h for different status
                                                values */

#ifndef NULL
#define NULL  ((void *)0)
#endif

#define STATIC static

/* This Macro to be used to resolve Unused and unreference
 * compiler warnings.
 */

#define PHNFC_UNUSED_VARIABLE(_x_) UNREFERENCED_PARAMETER(_x_)

/**
 *  \name HAL Overall Definitions
 *  Definitions applicable to a variety of purposes and functions/features.
 */

#define PHHAL_NFCID_LENGTH              0x0AU  /**< \ingroup grp_hal_common
                                                     Maximum length of NFCID 1..3. */

#define PHHAL_MAX_DATASIZE              0xFBU       /* 256 * Maximum Data size sent
                                                     * by the HAL
                                                     */

#define PHHAL_ATQA_LENGTH               0x02U       /**< ATQA length */
#define PHHAL_MAX_UID_LENGTH            0x0AU       /**< Maximum UID length expected */
#define PHHAL_MAX_ATR_LENGTH            0x30U       /**< Maximum ATR_RES (General Bytes)
                                                     *   length expected */
#define PHHAL_ATQB_LENGTH               0x0BU       /**< ATQB length */
#define PHHAL_PUPI_LENGTH               0x04U       /**< PUPI length */
#define PHHAL_APP_DATA_B_LENGTH         0x04U       /**< Application Data length for Type B */
#define PHHAL_PROT_INFO_B_LENGTH        0x03U       /**< Protocol info length for Type B */
#define PHHAL_FEL_SYS_CODE_LEN          0x02U       /**< Felica System Code Length */
#define PHHAL_FEL_ID_LEN                0x08U       /**< Felica current ID Length */
#define PHHAL_FEL_PM_LEN                0x08U       /**< Felica current PM Length */
#define PHHAL_15693_UID_LENGTH          0x08U       /**< Length of the Inventory bytes for
                                                         ISO15693 Tag */

#define SESSIONID_SIZE                  0x08U
#define MAX_AID_LEN                     0x10U
#define MAX_UICC_PARAM_LEN              0xFFU

typedef struct phNfc_KeyInfo
{
    uint8_t bKeyNum; /**< Key number in Key table Min 2 Max 15, currently 0 and 1 is reserved for NDEF usages*/
    uint8_t aKey[6]; /**< 6 byte Mifare Key Value*/
}phNfc_KeyInfo_t, *pphNfc_KeyInfo_t;

typedef struct phNfc_MifareKeyInfo
{
    pphNfc_KeyInfo_t pKeyInfo;  /**< Key information to be updated/deleted*/
    uint8_t bNumOfKeys;         /*Number of key information available, Min 0, Max 13,*/
}phNfc_MifareKeyInfo_t, *pphNfc_MifareKeyInfo_t;

typedef struct phNfc_MifareKeyParam
{
    phNfc_KeyInfo_t KeyInfo[14];
    uint8_t bTotalnum;
}phNfc_MifareKeyParam_t;

typedef struct phNfc_sData
{
    _Field_size_bytes_(length)
    uint8_t             *buffer;
    uint32_t            length;
} phNfc_sData_t;


typedef struct phNfc_SeRFParamInfo_ATech
{
    uint8_t bPipeStatusCeA;
    uint8_t bModeCeA;
    uint8_t bUidRegSizeCeA;
    uint8_t aUidRegCeA[10];
    uint8_t bSakCeA;
    uint8_t aATQACeA[2];
    uint8_t bApplicationDataSizeCeA;
    uint8_t aApplicationDataCeA[15];
    uint8_t bFWI_SFGICeA;
    uint8_t bCidSupportCeA;
    uint8_t bCltSupportCeA;
    uint8_t aDataRateMaxCeA[3];
}phNfc_SeRFInfo_ATech_t;


typedef struct phNfc_SeRFParamInfo_BTech
{
    uint8_t bPipeStatusCeB;
    uint8_t bModeCeB;
    uint8_t aPupiRegCeB[4];
    uint8_t bAfiCeB;
    uint8_t aATQBCeB[4];
    uint8_t bHighLayerRspSizeCeB;
    uint8_t aHighLayerRspCeB[15];
    uint8_t aDataRateMaxCeB[3];
}phNfc_SeRFInfo_BTech_t;

typedef struct phNfc_SeRFParamInfo
{
    phNfc_SeRFInfo_ATech_t RfParamsA;
    phNfc_SeRFInfo_BTech_t RfParamsB;
}phNfc_SeRFParamInfo_t;

/**
 * \ingroup grp_lib_nfc
 * NFC Message structure contains message specific details like
 * message type, message specific data block details, etc.
 */
typedef struct phLibNfc_Message
{
    uint32_t eMsgType;/**< Type of the message to be posted*/
    void   * pMsgData;/**< Pointer to message specific data block in case any*/
    uint32_t Size;/**< Size of the datablock*/
} phLibNfc_Message_t,*pphLibNfc_Message_t;

#define PHNFC_NFCID_LENGTH              PHHAL_NFCID_LENGTH          /**< Maximum length of NFCID 1..3. */
#define PHNFC_ATQA_LENGTH               PHHAL_ATQA_LENGTH           /**< ATQA length */
#define PHNFC_MAX_UID_LENGTH            PHHAL_MAX_UID_LENGTH        /**< Maximum UID length expected */
#define PHNFC_MAX_ATR_LENGTH            PHHAL_MAX_ATR_LENGTH        /**< Maximum ATR_RES (General Bytes) length expected */
#define PHNFC_ATQB_LENGTH               PHHAL_ATQB_LENGTH           /**< ATQB length */
#define PHNFC_PUPI_LENGTH               PHHAL_PUPI_LENGTH           /**< PUPI length */
#define PHNFC_APP_DATA_B_LENGTH         PHHAL_APP_DATA_B_LENGTH     /**< Application Data length for Type B */
#define PHNFC_PROT_INFO_B_LENGTH        PHHAL_PROT_INFO_B_LENGTH    /**< Protocol info length for Type B */
#define PHNFC_FEL_SYS_CODE_LEN          PHHAL_FEL_SYS_CODE_LEN      /**< Felica System Code Length */
#define PHNFC_FEL_ID_LEN                PHHAL_FEL_ID_LEN            /**< Felica current ID Length */
#define PHNFC_FEL_PM_LEN                PHHAL_FEL_PM_LEN            /**< Felica current PM Length */
#define PHNFC_15693_UID_LENGTH          PHHAL_15693_UID_LENGTH      /**< Length of the Inventory bytes for ISO15693 Tag */
#define PHNFC_MAX_DATASIZE              PHHAL_MAX_DATASIZE

#define NFC_RF_DISCOVERY_CONTINUE       0x00
#define NFC_RF_DISCOVERY_REPOLL         0x01
#define NFC_RF_DISCOVERY_HALT           0xFF

/** \ingroup  grp_hal_common
 *
 *  The <em> Supported Protocols Structure </em> holds all protocol supported by the current NFC
 *  device.
 */
typedef struct phNfc_sSupProtocol
{
    unsigned int MifareUL    : 1;  /**< Protocol Mifare Ultra Light or any NFC Forum Type-2 tags */
    unsigned int MifareStd   : 1;  /**< Protocol Mifare Standard. */
    unsigned int ISO14443_4A : 1;  /**< Protocol ISO14443-4 Type A.  */
    unsigned int ISO14443_4B : 1;  /**< Protocol ISO14443-4 Type B.  */
    unsigned int ISO15693    : 1;  /**< Protocol ISO15693 HiTag.  */
    unsigned int Felica      : 1;  /**< Protocol Felica. */
    unsigned int NFC         : 1;  /**< Protocol NFC. */
    unsigned int Jewel       : 1;  /**< Protocol Innovision Jewel Tag. or Any T1T*/
    unsigned int Desfire     : 1;  /**<TRUE indicates specified feature (mapping
                                   or formatting)for DESFire tag supported else not supported.*/
    unsigned int Kovio       : 1;   /**< Protocol Kovio Tag*/
    unsigned int HID         : 1;   /**< Protocol HID(Picopass) Tag*/
    unsigned int Bprime      : 1;   /**< Protocol BPrime Tag*/
    unsigned int EPCGEN2     : 1;   /**< Protocol EPCGEN2 Tag*/
}phNfc_sSupProtocol_t;

/** \ingroup grp_hal_common
 *
 *  The <em> Device Information Structure </em> holds information
 *  related to the NFC IC read during initialization time.
 *  It allows the caller firmware, hardware version, the model id,
 *  NCI verison supported and vendor name. Refer to the NFC Device
 *  User Manual on how to interpret each of the values. In addition
 *  it also contains capabilities of the NFC Device such as the
 *  protocols supported in Reader and emulation mode
 */

typedef struct phNfc_sDeviceCapabilities
{
    uint8_t NciVersion;

    uint8_t ManufacturerId;

    struct
    {
        uint8_t Byte0;                      /**< Byte 0*/
        uint8_t Byte1;                      /**< Byte 1*/
        uint8_t Byte2;                      /**< Byte 2*/
        uint8_t Byte3;                      /**< Byte 3*/
    } ManufactureInfo;

    struct
    {
        BitField_t SwitchOffState:1;        /**<Supported if the bit is set to 1b*/
        BitField_t BatteryOffState:1;       /**<Supported if the bit is set to 1b*/
    } PowerStateInfo;                       /**<Power states information*/

    struct
    {
        BitField_t          AidBasedRouting : 1;       /**<Supported if the bit is set to 1b*/
        BitField_t          ProtocolBasedRouting : 1;  /**<Supported if the bit is set to 1b*/
        BitField_t          TechnBasedRouting : 1;     /**<Supported if the bit is set to 1b*/
    } RoutingInfo;                                     /**<Types of routing suported by NFCC*/

    uint16_t                RoutingTableSize;          /**<Maximum Routing table size*/

    phNfc_sSupProtocol_t    ReaderSupProtocol;         /**< Supported protocols in Reader mode. */
    phNfc_sSupProtocol_t    EmulationSupProtocol;      /**< Supported protocols in Emulation mode. */
}phNfc_sDeviceCapabilities_t;

/**
 * \ingroup grp_hal_common
 *
 *  The Hardware Reference structure is filled as part of the open function and
 *  contains information regarding connected peripheral NFC device. It also
 *  stores the refernce to the communication driver passed by the HAL client
 *  for usage during communication with the NFC Device
 *
 * \note The caller can consider this structure atomic, no interpretation is required
 *       for HAL operation.
 *
 */
typedef struct phNfc_sHwReference
{
    void *pDriverHandle;     /**  Will be valid after the Open function. */
    void *pNciHandle;        /**  Context of the NCI Layer */
} phNfc_sHwReference_t;

typedef struct phNfc_sDepFlags
{
   unsigned int MetaChaining : 1;
   unsigned int NADPresent   : 1;
} phNfc_sDepFlags_t;


typedef struct phNfc_sDepAdditionalInfo
{
    phNfc_sDepFlags_t DepFlags;
    uint8_t NodeAddress;
} phNfc_sDepAdditionalInfo_t;

typedef enum phNfc_eMifareCmdList
{
    phNfc_eMifareRaw        = 0x00U,     /**< This command performs raw transcations.
                                              Format of the #phNfc_sTransceiveInfo_t
                                              content in this case shall be as below:

                                                -  \b cmd: filed shall set to  phHal_eMifareRaw .
                                                -  \b addr: doesn't carry any significance.
                                                -  \b sSendData: Shall contain formatted raw buffer
                                                                based on MIFARE commands type used.
                                                                Formatted buffer shall follow below
                                                                formating scheme.

                                              CmdType+ Block No + CommandSpecific data + 2 byte CRC
                                              Ex: With  Write  4 byte command  on block 8  looks as
                                              " 0xA2,0x08,0x01,0x02,0x03,0x04,CRC1,CRC2
                                              Note :  For MIFARE Std card we recommend use MIFARE
                                                      commands directly.
                                           */
    phNfc_eMifareAuthentA   = 0x60U,     /**< Mifare Standard:\n
                                              This command performs an authentication with KEY A for a sector.\n
                                              Format of the #phNfc_sTransceiveInfo_t content in this case is :
                                                - \b cmd: field shall set to  phHal_eMifareAuthentA .
                                                - \b addr: indicates MIFARE block address.
                                                      Ex: 0x08 indicates block 8 needs to be authenticated.
                                                - \b sSendData: Shall contain authentication key values.
                                                                    sSendData ,buffer shall contain authentication
                                                                    key values 01 02 03 04 05 06 authenticates
                                                                    block 08 with the key 0x01[..]06. If this
                                                                    command fails, then  user needs to reactivate
                                                                    the remote Mifare card.
                                          */
    phNfc_eMifareAuthentB   = 0x61U,     /**< Mifare Standard:\n
                                              This command performs an authentication with KEY B for a sector.\n
                                              Format of the #phNfc_sTransceiveInfo_t content in this case is :
                                                - \b cmd: field shall set to  phHal_eMifareAuthentB .
                                                - \b addr: indicates MIFARE block address.
                                                           Ex: 0x08 indicates block 8 needs to be authenticated.
                                                - \b sSendData: Shall contain authentication key values.
                                                                    sSendData ,buffer shall contain authentication
                                                                    key values 01 02 03 04 05 06 authenticates
                                                                    block 08 with the key 0x01[..]06. If this
                                                                    command fails, then  user needs to reactivate
                                                                    the remote Mifare card.
                                          */
    phNfc_eMifareRead16     = 0x30U,     /**< Mifare Standard and Ultra Light:\n
                                              Read 16 Bytes from a Mifare Standard block or 4 Mifare Ultra Light pages.\n
                                              Format of the #phNfc_sTransceiveInfo_t content in this case is :
                                                - \b cmd: field shall set to  phHal_eMifareRead16 .
                                                - \b addr: memory adress to read.
                                                - \b sRecvData : Shall contain buffer of size 16
                                                                    to read the data into.

                                              If this command fails, the user needs to reactivate the
                                              the remote Mifare card
                                          */
    phNfc_eMifareRead       = 0x30U,
    phNfc_eMifareWrite16    = 0xA0U,     /**< Mifare Standard and Ultra Light:\n
                                              Write 16 Bytes to a Mifare Standard block or 4 Mifare Ultra Light pages.\n
                                              Format of the #phNfc_sTransceiveInfo_t content in this case is :
                                                - \b cmd: field shall set to  phHal_eMifareWrite16 .
                                                - \b addr: starting memory adress to write from.
                                                - \b sSendData: Shall contain buffer of size 16 containing
                                                                    the data bytes to be written.

                                              If this command fails, the user needs to reactivate the
                                              the remote Mifare card
                                          */
    phNfc_eMifareWrite4     = 0xA2U,     /**< Mifare Ultra Light:\n
                                              Write 4 bytes.\n
                                              Format of the #phNfc_sTransceiveInfo_t content in this case is :
                                                - \b cmd: field shall set to  phHal_eMifareWrite4 .
                                                - \b addr: starting memory adress to write from.
                                                - \b sSendData: Shall contain buffer of size 4 containing
                                                                    the data bytes to be written.

                                              If this command fails, the user needs to reactivate the
                                              the remote Mifare card
                                          */
    phNfc_eMifareInc        = 0xC1U,     /**< Increment. */
    phNfc_eMifareDec        = 0xC0U,     /**< Decrement. */
    phNfc_eMifareTransfer   = 0xB0U,     /**< Tranfer.   */
    phNfc_eMifareRestore    = 0xC2U,     /**< Restore.   */
    phNfc_eMifareReadSector = 0x38U,     /**< Read Sector.   */
    phNfc_eMifareWriteSector= 0xA8U,     /**< Write Sector.   */
    /*Above commands could be used for preparing raw command but below one can not be*/
    phNfc_eMifareReadN  = 0x01,          /**< prop*/
    phNfc_eMifareWriteN = 0x02,          /**< prop*/
    phNfc_eMifareSectorSel = 0x03,       /**< prop*/
    phNfc_eMifareAuth = 0x04,           /**< prop*/
    phNfc_eMifareProxCheck = 0x05,      /**< prop*/
    phNfc_eMifareAuthKeyNumA = 0x80,
    phNfc_eMifareAuthKeyNumB = 0x81,
    phNfc_eMifareInvalidCmd = 0xFFU      /**< Invalid Command */
} phNfc_eMifareCmdList_t;

typedef enum phNfc_eIso14443_4_CmdList
{
    phNfc_eIso14443_4_Raw             = 0x00U /**< ISO 14443-4 Exchange command:\n
                                                 - This command sends the data buffer directly
                                                 to the remote device */
} phNfc_eIso14443_4_CmdList_t;

typedef enum phNfc_eNfcIP1CmdList
{
       phNfc_eNfcIP1_Raw             = 0x00U /**< NfcIP Exchange command:\n
                                                 - This command sends the data buffer directly
                                                  to the remote device */
}phNfc_eNfcIP1CmdList_t;

typedef enum phNfc_eIso15693_CmdList
{
    phNfc_eIso15693_Raw             = 0x00U, /**< ISO 15693 Exchange Raw command:\n
                                                 - This command sends the data buffer directly
                                                 to the remote device */
    phNfc_eIso15693_Cmd             = 0x20U, /**< ISO 15693 Exchange command:\n
                                                 - This command is used to access the card
                                                 to the remote device */
    phNfc_eIso15693_Invalid         = 0xFFU      /**< Invalid Command */
} phNfc_eIso15693_CmdList_t;

typedef enum phNfc_eFelicaCmdList
{
    phNfc_eFelica_Raw             = 0xF0U, /**< Felica Raw command:
                                                 - This command sends the data buffer directly
                                                 to the remote device */
    phNfc_eFelica_Check           = 0x00, /**< Felica Check command:
                                                 - This command checks the data from the Felica
                                                  remote device */
    phNfc_eFelica_Update          = 0x01, /**< Felica Update command:
                                                 - This command updates the data onto the Felica
                                                  remote device */
    phNfc_eFelica_Invalid         = 0xFFU      /**< Invalid Command */
} phNfc_eFelicaCmdList_t;


typedef enum phNfc_eJewelCmdList
{
    phNfc_eJewel_Raw            = 0x00U, /**< Jewel command:
                                                 - This command sends the data buffer directly
                                                 to the remote device */
    phNfc_eJewel_WriteN         = 0x01U, /**< Write N blocks*/
    phNfc_eJewel_Invalid        = 0xFFU  /**< Invalid jewel command */
}phNfc_eJewelCmdList_t;

typedef enum phNfc_eEpcGenCmdList
{
    phNfc_eEpcGen_Raw           = 0x00U,/**< EpcGen2 Exchange command:\n
                                                - This command sends the data buffer directly
                                                  to the remote device */
    phNfc_eEpcGen_Read          = 0xC2, /**< EpcGen Update command:
                                                 - This command shall read memory from EpcGen
                                                  remote device */
    phNfc_eEpcGen_Write         = 0xC3, /**< EpcGen Write command:
                                                 - This command shall write 1 word data (2 bytes) to EpcGen
                                                  remote device */
    phNfc_eEpcGen_Access        = 0xC6, /**< EpcGen Access command::
                                                 - This command shall authenticate the EpcGen
                                                  remote device */
    phNfc_eEpcGen_BlockWrite    = 0xC7, /**< EpcGen BlockWrite command:
                                                 - This command write the required number of words to EpcGen
                                                  remote device */
    phNfc_eEpcGen_BlockErase    = 0xC8, /**< EpcGen BlockErase command:
                                                 - This command shall erase the specified number of blocks from
                                                  remote device */
    phNfc_eEpcGen_Invalid       = 0xFFU      /**< Invalid Command */
}phNfc_eEpcGenCmdList_t;

typedef enum phNfc_eHid_CmdList
{
    phNfc_eHeidRead /* Todo */
}phNfc_eHid_CmdList_t;

/**
 * The <em> Reader A structure </em> includes the available information
 * related to the discovered ISO14443A remote device. This information
 * is updated for every device discovery.
 */
typedef struct phNfc_sIso14443AInfo
{
    uint8_t         Uid[PHNFC_MAX_UID_LENGTH];      /**< UID information of the TYPE A Tag Discovered */
    _Field_range_(<=, PHNFC_MAX_UID_LENGTH)
    uint8_t         UidLength;                      /**< UID information length, shall not be greater
                                                         than PHHAL_MAX_UID_LENGTH i.e., 10 */
    uint8_t         AppData[PHNFC_MAX_ATR_LENGTH];  /**< Application data information of the
                                                        tag discovered (= Historical bytes for type A) */
    _Field_range_(<=, PHNFC_MAX_ATR_LENGTH)
    uint8_t         AppDataLength;                  /**< Application data length */
    uint8_t         Sak;                            /**< SAK informationof the TYPE A Tag Discovered */
    uint8_t         AtqA[PHNFC_ATQA_LENGTH];        /**< ATQA informationof the TYPE A Tag Discovered */
    uint8_t         MaxDataRate;                    /**< Maximum data rate supported by the TYPE A
                                                       Tag Discovered */
    uint8_t         Fwi_Sfgt;                       /**< Frame waiting time and start up frame guard
                                                         time as defined in ISO/IEC 14443-4[7] for type A */
}phNfc_sIso14443AInfo_t;

/**
 * The <em> Reader B structure </em> includes the available information
 * related to the discovered ISO14443B remote device. This information
 * is updated for every device discovery.
 */

typedef struct phNfc_sIso14443BInfo
{
    union phNfc_uAtqBInfo   /**< AtqB information */
    {
        struct phNfc_sAtqBInfo  /**< AtqB information internal fields */
        {
            uint8_t         Pupi[PHNFC_PUPI_LENGTH];            /**< PUPI information  of the TYPE B
                                                                    Tag Discovered */
            uint8_t         AppData[PHNFC_APP_DATA_B_LENGTH];   /**< Application Data  of the TYPE B
                                                                    Tag Discovered */
            uint8_t         ProtInfo[PHNFC_PROT_INFO_B_LENGTH]; /**< Protocol Information  of the TYPE B
                                                                    Tag Discovered */
        }AtqResInfo;

        uint8_t         AtqRes[PHNFC_ATQB_LENGTH];              /**< ATQB Response Information of TYPE B Tag Discovered */
    }AtqB;
    uint8_t         HiLayerResp[PHNFC_MAX_ATR_LENGTH];  /**< Higher Layer Response information
                                                             in answer to ATRRIB Command for Type B */
    uint8_t         HiLayerRespLength;                  /**< Higher Layer Response length */
    uint8_t         Afi;                                /**< Application Family Identifier of TYPE B
                                                                Tag Discovered */
    uint8_t         MaxDataRate;                        /**< Maximum data rate supported by the TYPE B Tag Discovered */
}phNfc_sIso14443BInfo_t;

typedef struct phNfc_sIso14443BPrimeInfo
{
    void *BPrimeCtxt;           /**< BPrime context */
}phNfc_sIso14443BPrimeInfo_t;


/** \ingroup grp_hal_nfci
 *
 * The <em> Jewel Reader structure </em> includes the available information
 * related to the discovered Jewel remote device. This information
 * is updated for every device discovery.
 */

typedef struct phNfc_sJewelInfo
{
    uint8_t         Uid[PHNFC_MAX_UID_LENGTH];      /**< UID information of the TYPE A
                                                         Tag Discovered */
    _Field_range_(<=, PHNFC_MAX_UID_LENGTH)
    uint8_t         UidLength;                      /**< UID information length */
    uint8_t         HeaderRom0;                     /**< Header Rom byte zero */
    uint8_t         HeaderRom1;                     /**< Header Rom byte one */

}phNfc_sJewelInfo_t;

/** \ingroup grp_hal_nfci
 *
 * The <em> Felica Reader structure </em> includes the available information
 * related to the discovered Felica remote device. This information
 * is updated for every device discovery.
 */

typedef struct phNfc_sFelicaInfo
{
    uint8_t     IDm[(PHNFC_FEL_ID_LEN + 2)];        /**< Current ID of Felica tag */
    _Field_range_(<=, (PHNFC_FEL_ID_LEN + 2))
    uint8_t     IDmLength;                          /**< IDm length, shall not be greater
                                                         than PHHAL_FEL_ID_LEN i.e., 8 */
    uint8_t     PMm[PHNFC_FEL_PM_LEN];              /**< Current PM of Felica tag */
    uint8_t     SystemCode[PHNFC_FEL_SYS_CODE_LEN]; /**< System code of Felica tag */
}phNfc_sFelicaInfo_t;


/** \ingroup grp_hal_nfci
 *
 *  The <em> Reader A structure </em> includes the available information
 *  related to the discovered ISO15693 remote device. This information
 *  is updated for every device discovery.
 */

typedef struct phNfc_sIso15693Info
{
    uint8_t         Uid[PHNFC_15693_UID_LENGTH];    /**< UID information of the 15693 Tag Discovered */
    uint8_t         UidLength;                      /**< UID information length, shall not be greater
                                                         than PHHAL_15693_UID_LENGTH i.e., 8 */
    uint8_t         Dsfid;                          /**< DSF information of the 15693 Tag Discovered */
    uint8_t         Flags;                          /**< Information about the Flags in the 15693 Tag Discovered */
    uint8_t         Afi;                            /**< Application Family Identifier of 15693 Tag Discovered */
}phNfc_sIso15693Info_t;

typedef struct phNfc_sKovioInfo
{
    uint8_t TagIdLength;
    uint8_t TagId[16];
}phNfc_sKovioInfo_t;

/** \ingroup grp_hal_nfci
 *
 *  The <em> \ref phHalNfc_eDataRate enum </em> lists all the Data Rate
 *  values to be used to determine the rate at which the data is transmitted
 *  to the target.
 */

typedef enum phNfc_eDataRate
{
    phNfc_eDataRate_106    = 0x00U, /**< 106 Kbps */
    phNfc_eDataRate_212,            /**< 212 Kbps */
    phNfc_eDataRate_424,            /**< 424 Kbps */
    phNfc_eDataRate_848,            /**< 848 Kbps */
    phNfc_eDataRate_1696,           /**< 1696 Kbps */
    phNfc_eDataRate_3392,           /**< 3392 Kbps */
    phNfc_eDataRate_6784,           /**< 6784 Kbps */
    phNfc_eDataRate_26,             /**< 26 Kbps */
    phNfc_eDataRate_RFU             /**< RFU */
}phNfc_eDataRate_t;

#define phHal_eDataRate_106 phNfc_eDataRate_106 /**< \copybrief phNfc_eDataRate_106 \sa phNfc_eDataRate_106  */
#define phHal_eDataRate_212 phNfc_eDataRate_212 /**< \copybrief phNfc_eDataRate_212 \sa phNfc_eDataRate_212  */
#define phHal_eDataRate_424 phNfc_eDataRate_424 /**< \copybrief phNfc_eDataRate_424 \sa phNfc_eDataRate_424  */
#define phHal_eDataRate_848 phNfc_eDataRate_848 /**< \copybrief phNfc_eDataRate_848 \sa phNfc_eDataRate_848  */
#define phHal_eDataRate_1696 phNfc_eDataRate_1696 /**< \copybrief phNfc_eDataRate_1696 \sa phNfc_eDataRate_1696  */
#define phHal_eDataRate_3392 phNfc_eDataRate_3392 /**< \copybrief phNfc_eDataRate_3392 \sa phNfc_eDataRate_3392  */
#define phHal_eDataRate_6784 phNfc_eDataRate_6784 /**< \copybrief phNfc_eDataRate_6784 \sa phNfc_eDataRate_6784  */
#define phHal_eDataRate_26 phNfc_eDataRate_26 /**< \copybrief phNfc_eDataRate_20 \sa phNfc_eDataRate_20  */
#define phHal_eDataRate_RFU phNfc_eDataRate_RFU /**< \copybrief phNfc_eDataRate_RFU \sa phNfc_eDataRate_RFU  */

typedef phNfc_eDataRate_t phHalNfc_eDataRate_t;


/** \ingroup grp_hal_nfci
 *
 *  The <em> NFCIP1 structure </em> includes the available information
 *  related to the discovered NFCIP1 remote device. This information
 *  is updated for every device discovery.
 */

typedef struct phNfc_sNfcIPInfo
{
    /* Contains the random NFCID3I conveyed with the ATR_REQ.
        always 10 bytes length
        or contains the random NFCID3T conveyed with the ATR_RES.
        always 10 bytes length */
    uint8_t             NFCID[PHNFC_MAX_UID_LENGTH];    /**<NfcID*/
    uint8_t             NFCID_Length;                   /**<NfcID length*/
    /* ATR_RES = General bytes length, Max length = 48 bytes */
    uint8_t             ATRInfo[PHNFC_MAX_ATR_LENGTH];  /**<Contains general bytes of ATR_REQ or ATR_RES*/
    uint8_t             ATRInfo_Length;                 /**<ATR Info length*/
    uint8_t             SelRes;                         /**< SAK information of the tag discovered */
    uint8_t             SenseRes[PHNFC_ATQA_LENGTH];    /**< ATQA information of the tag discovered */
    uint8_t             Nfcip_Active;                   /**< Is Detection Mode of the NFCIP Target Active */
    uint16_t            MaxFrameLength;                 /**< Maximum frame length supported by the NFCIP device */
    phNfc_eDataRate_t   Nfcip_Datarate;                 /**< Data rate supported by the NFCIP device */
}phNfc_sNfcIPInfo_t;

/** \ingroup grp_hal_nfci
*
*  The <em> EpcGen structure </em> includes the available information
*  related to the discovered EpcGen remote device. This information
*  is updated for every device discovery.
*/

typedef struct phNfc_sEpcGenInfo
{
    uint16_t hAccessHandle; /* Access handle used for current session if firmware sends during Intf Activation.
                               Else user has to request it by sending Req_RN command with valid StoredCrc as input */
    uint16_t wStoredCrc;    /**< Used along with Req_RN command for getting AccessHandle
                                 (if AccessHandle is not sent by firmware during Interface activation) */
    uint16_t wXpc_W1;       /**< Store it if firmware sends during Intf Activation */
    struct
    {
        uint8_t  bEpcLength;        /**<Length of Epc in EPC memory*/
        uint8_t  bUmi;              /**<User memory indicator (indicates whether user memory is present in
                                        the tag or user memory does not contain any information incase if
                                        it is present)*/
        uint8_t  bRecomissionStatus;/**<Tag recomission status*/
        uint16_t wNsi;              /**<Numbering system identifier (Identifies the type of app present
                                        in the tag (EpcGlobal app or non-EpcGlobal app) */
    }ProtocolControl;               /**<Protocol control word present in the detected tag*/
}phNfc_sEpcGenInfo_t;

/** \ingroup grp_hal_nfci
 *
 *  The <em> Remote Device Information Union </em> includes the available Remote Device Information
 *  structures. Following the device detected, the corresponding data structure is used.
 */

typedef union phNfc_uRemoteDevInfo
{
    phNfc_sIso14443AInfo_t          Iso14443A_Info;
    phNfc_sIso14443BInfo_t          Iso14443B_Info;
    phNfc_sIso14443BPrimeInfo_t     Iso14443BPrime_Info;
    phNfc_sNfcIPInfo_t              NfcIP_Info;
    phNfc_sEpcGenInfo_t             EpcGen_Info;
    phNfc_sFelicaInfo_t             Felica_Info;
    phNfc_sJewelInfo_t              Jewel_Info;
    phNfc_sIso15693Info_t           Iso15693_Info;
    phNfc_sKovioInfo_t              Kovio_Info;
}phNfc_uRemoteDevInfo_t;

/** \ingroup grp_hal_nfci
*
*  The <em> RF Device Type List </em> is used to identify the type of
*  remote device that is discovered/connected. There seperate
*  types to identify a Remote Reader (denoted by _PCD) and
*  Remote Tag (denoted by _PICC)
*/

typedef enum phNfc_eRFDevType
{
    phNfc_eUnknown_DevType        = 0x00U,

    /* Specific PCD Devices */
    phNfc_eISO14443_A_PCD,
    phNfc_eISO14443_B_PCD,
    phNfc_eISO14443_BPrime_PCD,
    phNfc_eFelica_PCD,
    phNfc_eJewel_PCD,
    phNfc_eISO15693_PCD,
    phNfc_eEpcGen2_PCD,
    /* Generic PCD Type */
    phNfc_ePCD_DevType,
    /* Generic PICC Type */
    phNfc_ePICC_DevType,
    /* Specific PICC Devices */
    /* This PICC type explains that the card is compliant to the
        ISO 14443-1 and 2A specification. This type can be used for the
        cards that is supporting these specifications */
    phNfc_eISO14443_A_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-4A specification */
    phNfc_eISO14443_4A_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-3A specification */
    phNfc_eISO14443_3A_PICC,
    /* This PICC type explains that the card is Mifare UL/1k/4k and
    also it is compliant to ISO 14443-3A. There can also be other
    ISO 14443-3A cards, so the phHal_eISO14443_3A_PICC is also used for
    PICC detection */
    phNfc_eMifare_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-1, 2 and 3B specification */
    phNfc_eISO14443_B_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-4B specification */
    phNfc_eISO14443_4B_PICC,
    /* This PICC type explains that the card is B-Prime type */
    phNfc_eISO14443_BPrime_PICC,
    phNfc_eFelica_PICC,
    phNfc_eJewel_PICC,
    phNfc_eISO15693_PICC,
    phNfc_eEpcGen2_PICC,
    /* This PICC type explains that the card is Kovio type */
    phNfc_eKovio_PICC,

    /* NFC-IP1 Device Types */
    phNfc_eNfcIP1_Target,
    phNfc_eNfcIP1_Initiator,

    /* Other Sources */
    phNfc_eInvalid_DevType

}phNfc_eRFDevType_t;

#define phHal_eUnknown_DevType phNfc_eUnknown_DevType /**< \copybrief phNfc_eUnknown_DevType \sa phNfc_eUnknown_DevType  */
#define phHal_eISO14443_A_PCD phNfc_eISO14443_A_PCD /**< \copybrief phNfc_eISO14443_A_PCD \sa phNfc_eISO14443_A_PCD  */
#define phHal_eISO14443_B_PCD phNfc_eISO14443_B_PCD /**< \copybrief phNfc_eISO14443_B_PCD \sa phNfc_eISO14443_B_PCD  */
#define phHal_eISO14443_BPrime_PCD phNfc_eISO14443_BPrime_PCD /**< \copybrief phNfc_eISO14443_BPrime_PCD \sa phNfc_eISO14443_BPrime_PCD  */
#define phHal_eFelica_PCD phNfc_eFelica_PCD /**< \copybrief phNfc_eFelica_PCD \sa phNfc_eFelica_PCD  */
#define phHal_eJewel_PCD phNfc_eJewel_PCD /**< \copybrief phNfc_eJewel_PCD \sa phNfc_eJewel_PCD  */
#define phHal_eISO15693_PCD phNfc_eISO15693_PCD /**< \copybrief phNfc_eISO15693_PCD \sa phNfc_eISO15693_PCD  */
#define phHal_ePCD_DevType phNfc_ePCD_DevType /**< \copybrief phNfc_ePCD_DevType \sa phNfc_ePCD_DevType  */
#define phHal_ePICC_DevType phNfc_ePICC_DevType /**< \copybrief phNfc_ePICC_DevType \sa phNfc_ePICC_DevType  */
#define phHal_eISO14443_A_PICC phNfc_eISO14443_A_PICC /**< \copybrief phNfc_eISO14443_A_PICC \sa phNfc_eISO14443_A_PICC  */
#define phHal_eISO14443_4A_PICC phNfc_eISO14443_4A_PICC /**< \copybrief phNfc_eISO14443_4A_PICC \sa phNfc_eISO14443_4A_PICC  */
#define phHal_eISO14443_3A_PICC phNfc_eISO14443_3A_PICC /**< \copybrief phNfc_eISO14443_3A_PICC \sa phNfc_eISO14443_3A_PICC  */
#define phHal_eMifare_PICC phNfc_eMifare_PICC /**< \copybrief phNfc_eMifare_PICC \sa phNfc_eMifare_PICC  */
#define phHal_eISO14443_B_PICC phNfc_eISO14443_B_PICC /**< \copybrief phNfc_eISO14443_B_PICC \sa phNfc_eISO14443_B_PICC  */
#define phHal_eISO14443_4B_PICC phNfc_eISO14443_4B_PICC /**< \copybrief phNfc_eISO14443_4B_PICC \sa phNfc_eISO14443_4B_PICC  */
#define phHal_eISO14443_BPrime_PICC phNfc_eISO14443_BPrime_PICC /**< \copybrief phNfc_eISO14443_BPrime_PICC \sa phNfc_eISO14443_BPrime_PICC  */
#define phHal_eFelica_PICC phNfc_eFelica_PICC /**< \copybrief phNfc_eFelica_PICC \sa phNfc_eFelica_PICC  */
#define phHal_eJewel_PICC phNfc_eJewel_PICC /**< \copybrief phNfc_eJewel_PICC \sa phNfc_eJewel_PICC  */
#define phHal_eISO15693_PICC phNfc_eISO15693_PICC /**< \copybrief phNfc_eISO15693_PICC \sa phNfc_eISO15693_PICC  */
#define phHal_eNfcIP1_Target phNfc_eNfcIP1_Target /**< \copybrief phNfc_eNfcIP1_Target \sa phNfc_eNfcIP1_Target  */
#define phHal_eNfcIP1_Initiator phNfc_eNfcIP1_Initiator /**< \copybrief phNfc_eNfcIP1_Initiator \sa phNfc_eNfcIP1_Initiator  */
#define phHal_eInvalid_DevType phNfc_eInvalid_DevType /**< \copybrief phNfc_eInvalid_DevType \sa phNfc_eInvalid_DevType  */


/** \ingroup grp_hal_nfci
 *
 *  The <em> Remote Device Type List </em> is used to identify the type of
 *  remote device that is discovered/connected
 */

typedef phNfc_eRFDevType_t phNfc_eRemDevType_t;
typedef phNfc_eRemDevType_t phHal_eRemDevType_t;

/** \ingroup grp_hal_common
 *  The <em> Hal Command Union </em> includes each available type of Commands.
 */

typedef union phNfc_uCommand
{
  phNfc_eMifareCmdList_t         MfCmd;         /**< Mifare command structure.  */
  phNfc_eIso14443_4_CmdList_t    Iso144434Cmd;  /**< ISO 14443-4 command structure.  */
  phNfc_eFelicaCmdList_t         FelCmd;        /**< Felica command structure.  */
  phNfc_eJewelCmdList_t          JewelCmd;      /**< Jewel command structure.  */
  phNfc_eIso15693_CmdList_t      Iso15693Cmd;   /**< ISO 15693 command structure.  */
  phNfc_eNfcIP1CmdList_t         NfcIP1Cmd;     /**< ISO 18092 (NFCIP1) command structure */
  phNfc_eHid_CmdList_t           HidCmd;        
  phNfc_eHid_CmdList_t           BPrimeCmd;     /**<Could be same is ISO-4*/
  phNfc_eEpcGenCmdList_t         EpcGenCmd;     /**< EpcGen command structure*/
}phNfc_uCmdList_t;



/** \ingroup grp_hal_nfci
 *
 *  The <em> Remote Device Information Structure </em> holds information about one single Remote
 *  Device detected by the polling function .\n
 *  It lists parameters common to all supported remote devices.
 */

typedef struct phNfc_sRemoteDevInformation
{
    uint8_t                    SessionOpened;       /**< [out] Boolean Flag indicating the validity of
                                                         the handle of the remote device.
                                                     * 1 = Device is not actived (Only discovered), 2 = Device is active and ready for use*/
    phNfc_eRemDevType_t        RemDevType;          /**< [out] Remote device type which says that remote
                                                               is Reader A or Reader B or NFCIP or Felica or
                                                               Reader B Prime or Jewel*/
    phNfc_uRemoteDevInfo_t     RemoteDevInfo;       /**< Union of available Remote Device */

}phNfc_sRemoteDevInformation_t;

typedef struct phNfc_sDevInputParam
{
    uint8_t FelicaPollPayload[5];           
    uint8_t NfcPollPayload[5];              
    uint8_t NFCIDAuto;                      
    uint8_t NFCID3i[PHNFC_NFCID_LENGTH];    
    uint8_t DIDiUsed;                       
    uint8_t CIDiUsed;                       
    uint8_t NfcNADiUsed;                    
    uint8_t GeneralByte[48];                
    uint8_t GeneralByteLength;              
    uint8_t ISO14443_4B_AFI;                

} phNfc_sDevInputParam_t;

typedef struct phNfc_sTransceiveInfo
{
    phNfc_uCmdList_t                cmd;                
    uint8_t                         addr;               /**< Start Block Number for T1T and T2T*/
    uint8_t                         bKeyNum;            /**< Key number for Mifare Cards for authentication */
    uint16_t                        timeout;            /**< Timeout value to be used during transceive */
    uint8_t                         NumBlock;           /**< Number of Blocks to perform operation T1T and T2T*/
    /*For Felica Check and Update Service Code List*/
    uint16_t                        *ServiceCodeList;   /**< 2 Byte service Code List, Can be 2 Byte value to be used for all the blocks */
    uint16_t                        *Blocklist;         /**< 2 Byte Block list could be continuous num of blocks so NumBlock is enough*/
    phNfc_sData_t                   sSendData;          
    phNfc_sData_t                   sRecvData;          
    /* For EpcGen Transceive support */
    uint32_t                        dwWordPtr;          /**< Specifies the word address for the memory write */
    uint8_t                         bWordPtrLen;        /**< Specifies the length of word pointer
                                                             00: 8  bits
                                                             01: 16 bits
                                                             10: 24 bits
                                                             11: 32 bits */
    uint8_t                        bWordCount;          /**< Specifies the number of words to be read during EpcGen Read request or
                                                             Specifies the number of words to be written during EpcGen BlockWrite request*/
}phNfc_sTransceiveInfo_t;


/** \ingroup grp_hal_nfci
 *
 *  The <em> \ref phNfc_sIso14443ACfg structure </em> holds the information
 *  required for the NFC device to be used during ISO14443A target discovery
 */
typedef struct phNfc_sIso14443ACfg
{
    uint8_t     Auto_Activation;       /**< Enable Auto Activation for
                                            Technology A \n
                                            If set to 0, the activation procedure will stop
                                            after Select (SAK has been received).
                                            The host could evaluate SAK value and then decide:
                                                - to start communicating with the remote card
                                                  using proprietary commands (see NXP_MIFARE_RAW
                                                  and NXP_MIFARE_CMD)
                                            or
                                                - to activate the remote card up to ISO14443-4
                                                  level (RATS and PPS) using
                                                  CONTINUE ACTIVATION command
                                            If set to 1, activation follows the flow described in
                                            ETSI HCI specification (restrict detection to
                                            ISO14443-4 compliant cards).
                                    */
}phNfc_sIso14443ACfg_t;


/** \ingroup grp_hal_nfci
*
*  The <em> \ref phNfc_sIso14443BCfg structure </em> holds the information
*  required for the NFC device to be used during ISO14443B target discovery
*/
typedef struct phNfc_sIso14443BCfg
{
    uint8_t     AppFamily_ID;       /**< Application Family Identifier for
                                         Technology B, 0x00 means all application */
}phNfc_sIso14443BCfg_t;

/** \ingroup grp_hal_nfci
 *
 *  The structure holds the information
 *  required for the NFC device to be used during Felica target discovery
 */

typedef struct phNfc_sFelicaCfg
{
    uint8_t     SystemCode[PHNFC_FEL_SYS_CODE_LEN]; /**< System code for Felica tags */
    uint8_t     ReqCode;                            /**< Request Code for Felica tags */
    uint8_t     TimeSlotNum;                        /**< Time Slot Number - Used for collision resolutions */
}phNfc_sFelicaCfg_t;


/** \ingroup grp_hal_nfci
 *
 *  This enum is used to enable/disable
 *  phases of the discovery wheel related to specific reader types and
 *  card emulation phase
 *  \note Enabling specific Reader technology when NFCIP1 speed is set in the
 *        phHal_ADD_Cfg is implicitly done in HAL. Use this structure to only
 *        enable/disable Card Reader Functionality
 */
typedef struct phNfc_sPollDevInfo
{
    unsigned int    EnableIso14443A : 1;        /**< Flag to enable Reader A discovery */
    unsigned int    EnableIso14443B : 1;        /**< Flag to enable Reader B discovery */
    unsigned int    EnableFelica212 : 1;        /**< Flag to enable Felica 212 discovery */
    unsigned int    EnableFelica424 : 1;        /**< Flag to enable Felica 424 discovery */
    unsigned int    EnableIso15693 : 1;         /**< Flag to enable ISO 15693 discovery */
    unsigned int    EnableNfcActive : 1;        /**< Flag to enable Active Mode of NFC-IP discovery.
                                                     This is updated internally based on the NFC-IP speed. */
    unsigned int    EnableIso18000 : 1;         /**< EPCGen2 Discovery*/
    unsigned int    EnableHid : 1;              /**< Enable PICO15693 and PICO 14443-B Discovery*/
    unsigned int    EnableBPrime : 1;           /**< Enable BPrime Discovery*/
    unsigned int    EnableKovio : 1;            /**< Enable Kovio Discovery*/
    unsigned int    EnableEmvCoProfile: 1;      /**< Enable EmvCo Profile*/
    unsigned int    RFU : 1;                    /**< Reserved for future use */
    unsigned int    DisableCardEmulation : 1;   /**< Flag to disable the card emulation */
}phNfc_sPollDevInfo_t;


/** \ingroup grp_hal_common
 *
 *  This enumeration is used to identify the type of the host providing the
 *  information or the notification to the Terminal host.
 */

typedef enum phNfc_HostType
{
    /* This type identifies the host controller in the NFC device */
    phNfc_eHostController       = 0x00U,
    /* This type identifies the Host Device controlling the NFC device */
    phNfc_eTerminalHost         = 0x01U,
    /* This type identifies the uicc host connnected to the NFC device */
    phNfc_eUICCHost             = 0x02U,
    /* Host type is unknown */
    phNfc_eUnknownHost          = 0xFFU
}phNfc_HostType_t;


/** \ingroup grp_hal_nfci
*
*  The <em> \ref phHal_eP2PMode enum </em> lists all the NFCIP1 speeds
*  to be used for configuring the NFCIP1 discovery
*/

typedef enum phNfc_eP2PMode
{
    phNfc_eDefaultP2PMode  = 0x00U,
    phNfc_ePassive106 = 0x01U,
    phNfc_ePassive212 = 0x02U,
    phNfc_ePassive424 = 0x04U,
    phNfc_eActive     = 0x08U,
    phNfc_eActive106  = 0x10U,
    phNfc_eActive212  = 0x20U,
    phNfc_eActive424  = 0x40U,
    phNfc_eP2P_ALL    = 0x0FU,
    phNfc_eInvalidP2PMode = 0xFFU
}phNfc_eP2PMode_t;

typedef enum phNfc_eP2PTargetTech
{
    phNfc_eP2PTargetNfcInvalidTech = 0x00,
    phNfc_eP2PTargetNfcATech = 0x01U,
    phNfc_eP2PTargetNfcFTech = 0x02U,
    phNfc_eP2PTargetNfcActiveATech = 0x04U,
    phNfc_eP2PTargetNfcActiveFTech = 0x8U,
    phNfc_eP2PTargetEnableAllTech = 0x0FU
}phNfc_eP2PTargetTech_t;

typedef enum phNfc_eCETech
{
    phNfc_eCENfcInvalidTech = 0x00,
    phNfc_eCENfcATech = 0x01U,
    phNfc_eCENfcBTech = 0x02U,
    phNfc_eCENfcFTech = 0x04U,
    phNfc_eCEEnableAllTech = 0x0FU
}phNfc_eCETech_t;

#define phHal_eP2PTargetNfcATech phNfc_eP2PTargetNfcATech
#define phHal_eP2PTargetNfcFTech phNfc_eP2PTargetNfcFTech
#define phHal_eP2PTargetNfcActiveATech phNfc_eP2PTargetNfcActiveATech
#define phHal_eP2PTargetNfcActiveFTech phNfc_eP2PTargetNfcActiveFTech
#define phHal_eP2PTargetEnableAllTech phNfc_eP2PTargetEnableAllTech

#define phHal_eCENfcATech phNfc_eCENfcATech
#define phHal_eCENfcBTech phNfc_eCENfcBTech
#define phHal_eCENfcFTech phNfc_eCENfcFTech
#define phHal_eCEEnableAllTech phNfc_eCEEnableAllTech

#define phHal_eDefaultP2PMode   phNfc_eDefaultP2PMode   /**< \copybrief phNfc_eDefaultP2PMode   \sa phNfc_eDefaultP2PMode    */
#define phHal_ePassive106       phNfc_ePassive106       /**< \copybrief phNfc_ePassive106  \sa phNfc_ePassive106   */
#define phHal_ePassive212       phNfc_ePassive212       /**< \copybrief phNfc_ePassive212  \sa phNfc_ePassive212   */
#define phHal_ePassive424       phNfc_ePassive424       /**< \copybrief phNfc_ePassive424  \sa phNfc_ePassive424   */
#define phHal_eActive106        phNfc_eActive106        /**< \copybrief phNfc_ePassive106  \sa phNfc_eActive106   */
#define phHal_eActive212        phNfc_eActive212        /**< \copybrief phNfc_ePassive212  \sa phNfc_eActive212   */
#define phHal_eActive424        phNfc_eActive424        /**< \copybrief phNfc_ePassive424  \sa phNfc_eActive424   */
#define phHal_eActive           phNfc_eActive           /**< \copybrief phNfc_eActive      \sa phNfc_eActive       */
#define phHal_eP2P_ALL          phNfc_eP2P_ALL          /**< \copybrief phNfc_eP2P_ALL     \sa phNfc_eP2P_ALL      */
#define phHal_eInvalidP2PMode   phNfc_eInvalidP2PMode   /**< \copybrief phNfc_eInvalidP2PMode  \sa phNfc_eInvalidP2PMode   */


/** \ingroup grp_hal_common
*
*  This enumeration is used to specify the type of notification notified
*  to the upper layer. This classifies the notification into two types
*  one for the discovery notifications and the other for all the remaining
*  event notifications
*/

typedef enum phNfc_eNotificationType
{
    INVALID_NFC_NOTIFICATION    = 0x00U, /* Invalid Notification */
    NFC_DISCOVERY_NOTIFICATION,          /* Remote Device Discovery Notification */
    NFC_EVENT_NOTIFICATION               /* Event Notification from the other hosts */
}phNfc_eNotificationType_t;

typedef struct phNfc_sUiccInfo
{
    /* AID and Parameter Information is obtained if the
     * eventType is NFC_EVT_TRANSACTION.
     */
    phNfc_sData_t           aid;
    phNfc_sData_t           param;

}phNfc_sUiccInfo_t;

/** \ingroup grp_hal_common
*
*  The <em> \ref phNfc_sEmuSupport structure </em> holds the type
*   of the target emulation supported.
*/

typedef struct phNfc_sEmuSupport
{
    unsigned int TypeA:1;
    unsigned int TypeB:1;
    unsigned int TypeBPrime:1;
    unsigned int TypeFelica:1;
    unsigned int TypeMifare:1;
    unsigned int TypeNfcIP1:1;
    unsigned int RFU:2;
}phNfc_sEmuSupport_t;

/** \ingroup grp_hal_common
 *
 *  The <em> \ref phNfc_eConfigP2PMode </em> holds the P2P mode related configuration
 *  for setting the general bytes.
 */

typedef enum phNfc_eConfigP2PMode
{
    NFC_DEP_DEFAULT =   0x00U, /**< Both P2P Initiator and Target configuration */
    NFC_DEP_POLL,              /**< P2P Initiator configurations*/
    NFC_DEP_LISTEN             /**< P2P Target configuration*/
}phNfc_eConfigP2PMode_t;

/** \ingroup grp_hal_nfci
 *
 *  The <em> \ref phNfc_sNfcIPCfg </em> holds the P2P related information
 *  use by the NFC Device during P2P Discovery and connection
 */
typedef struct phNfc_sNfcIPCfg
{
    uint8_t generalBytesLength;/**< ATR_RES = General bytes length, Max length = 48 bytes */
    uint8_t generalBytes[PHNFC_MAX_ATR_LENGTH];/**< General bytes length*/
    phNfc_eConfigP2PMode_t p2pMode; /*Mode for P2P Initiator/Target*/
}phNfc_sNfcIPCfg_t;

/** \ingroup grp_hal_common
*
*  \brief Enumeration used to choose which type of parameters
*         are to be configured
*/
typedef enum phNfc_eConfigType
{
    NFC_INVALID_CONFIG = 0x00U, /**< Invalid Configuration */
    NFC_P2P_CONFIG,             /**< NFCIP1 Parameters */
    NFC_SE_PROTECTION_CONFIG,   /**< Secure Element Protection Cofiguration */
    NFC_EMULATION_CONFIG        /**< Emulation Parameters */
}phNfc_eConfigType_t;

/** \ingroup grp_hal_common
*
*  This enumeration is used to choose the Discovery Configuration
*  Mode :- Configure and Start, Stop or Start with last set
*  configuration
*/

typedef enum phNfc_eDiscoveryConfigMode
{
    NFC_DISCOVERY_CONFIG  = 0x00U,  /**< Configure discovery with values in phHal_sADD_Cfg_t and start discovery */
    NFC_DISCOVERY_START,            /**< Start Discovery with previously set configuration */
    NFC_DISCOVERY_RESUME            /**< Resume the Discovery with previously set configuration.
                                         This is valid only when the Target is not connected. */
}phNfc_eDiscoveryConfigMode_t;


/** \ingroup grp_hal_common
*
*  This enumeration defines various modes of releasing an acquired target
*  or tag.
*/

typedef enum phNfc_eReleaseType
{
    NFC_INVALID_RELEASE_TYPE = 0x00U,   /**<Invalid release type */
    NFC_DISCOVERY_RESTART,              /**< Release current target and
                                             restart discovery within same technology*/
    NFC_DISCOVERY_CONTINUE,             /**< Release current target and continue
                                             discovery with next technology in the wheel */
    NFC_DISCOVERY_STOP,                 /**< Release the current target and stop the Discovery */
    NFC_DEVICE_SLEEP                    /**< Put remote device in sleep mode*/
}phNfc_eReleaseType_t;

/** \ingroup grp_hal_common
*
*  This enumeration defines various modes of Discovery configuration modes and Release types
*  or tag.
*/

typedef enum phNfc_eDiscAndDisconnMode
{
    NFC_DISC_CONFIG_DISCOVERY = 0x00U,  /**< Configure discovery with values in phHal_sADD_Cfg_t and start
                                             discovery */
    NFC_DISC_START_DISCOVERY,           /**< Start Discovery with previously set configuration */
    NFC_DISC_RESUME_DISCOVERY,          /**< Resume the Discovery with previously set configuration.
                                             This is valid only when the Target is not connected */
    NFC_DISCONN_INVALID_RELEASE_TYPE,   /**<Invalid release type */
    NFC_DISCONN_RESTART_DISCOVERY,      /**< Release current target and
                                             restart discovery within same technology*/
    NFC_DISCONN_CONTINUE_DISCOVERY,     /**< Release current target and continue
                                             discovery with next technology in the wheel */
    NFC_DISCONN_STOP_DISCOVERY,         /**< Release the current target and
                                             Stop the Discovery */
    NFC_INTERNAL_CONTINUE_DISCOVERY,    /**< Received async DeActivate Ntf received with Discovery mode,
                                             Update current state to 'Discover' state */
    NFC_INTERNAL_STOP_DISCOVERY        /**< Received async DeActivate Ntf received with Idle mode,
                                             update current state to 'Init' state */
}phNfc_eDiscAndDisconnMode_t;

/** \ingroup grp_hal_common
*
*  This enumeration is used to choose configuration for a specific
*  emulation feature.
*/

typedef enum phNfc_eEmulationType
{
    NFC_UNKNOWN_EMULATION       = 0x00U, /**< Invalid Configuration */
    NFC_HOST_CE_A_EMULATION     = 0x01U, /**< Configure parameters for Type A
                                              card emulation from host */
    NFC_HOST_CE_B_EMULATION     = 0x02U, /**< Configure parameters for Type B
                                              card emulation from host */
    NFC_B_PRIME_EMULATION       = 0x03U, /**< Configure parameters for Type B'
                                              card emulation from host */
    NFC_FELICA_EMULATION        = 0x04U, /**< Configure parameters for Type F
                                              card emulation from host */
    NFC_MIFARE_EMULATION        = 0x06U, /**< Configure parameters for MIFARE
                                              card emulation - For Future Use */
    NFC_SMARTMX_EMULATION       = 0x07U, /**< Configure parameters for SmartMX */
    NFC_UICC_EMULATION          = 0x08U  /**< Configure parameters for UICC emulation */
}phNfc_eEmulationType_t;


/** \ingroup grp_hal_nfct
 *
 * \if hal
 *  \brief Information for Target Mode Start-Up
 * \else
 *  \brief HAL-Specific
 * \endif
 *
 *  The <em> Target Information Structure </em> required to start Target mode.
 *  It contains all the information for the Target mode.
 *
 *  \note None.
 *
 */
typedef struct phNfc_sTargetInfo
{
    uint8_t                 enableEmulation;
    phNfc_sNfcIPCfg_t       targetConfig;
} phNfc_sTargetInfo_t;

/** \ingroup grp_hal_common
*
*  This enumeration is used to choose the mode of operation for the SmartMx Module.
*  Default static configuration at initialization time.
*/
typedef enum phNfc_eSmartMX_Mode
{
    eSmartMx_Wired      = 0x00U, /* SmartMX is in Wired Mode */
    eSmartMx_Default,            /* SmartMX is in Default Configuration Mode */
    eSmartMx_Virtual,            /* SmartMx in the Virutal Mode */
    eSmartMx_Off                 /* SmartMx Feature is Switched off */
} phNfc_eSmartMX_Mode_t;


/** \ingroup grp_hal_common
*
*  This enumeration is used to choose the mode of operation for the SWP Link
*  for UICC Module. Default static configuration at initialization time.
*/
typedef enum phNfc_eSWP_Mode
{
    eSWP_Switch_Off      = 0x00U,   /* SWP Link is Switched off */
    eSWP_Switch_Default,            /* SWP is in Default Configuration Mode */
    eSWP_Switch_On                  /* SWP Link is Switched on */
} phNfc_eSWP_Mode_t;


/** \ingroup grp_hal_common
*
*  The <em> \ref phNfc_sSmartMX_Cfg structure </em> holds the information
*   to configure the SmartMX Module in the NFC Device.
*/
typedef struct phNfc_sSmartMX_Cfg
{
    uint8_t                 enableEmulation;
    uint8_t                 lowPowerMode;
    phNfc_eSmartMX_Mode_t   smxMode;
}phNfc_sSmartMX_Cfg_t;

/**
  * \ingroup grp_hal_common
  *
  * \brief Enum definition contains the type filed of 'TLV' Coding for Listen Mode Routing.
  */
typedef enum phNfc_eLstnModeRtngType
 {
     phNfc_LstnModeRtngTechBased = 0,    /**< Technology-based routing entry */
     phNfc_LstnModeRtngProtocolBased = 1,/**< Protocol-based routing entry */
     phNfc_LstnModeRtngAidBased = 2      /**< AID-based routing entry */
}phNfc_eLstnModeRtngType_t;

/**
 * \ingroup grp_hal_common
 * \brief Enum definition contains supported Power states
 */
typedef struct phNfc_PowerState
{
    BitField_t bSwitchedOn:1;          /**< Switched ON state */
    BitField_t bSwitchedOff:1;         /**< Switched Off state */
    BitField_t bBatteryOff:1;          /**< Battery Off state */
}phNfc_PowerState_t, *pphNfc_PowerState_t;/**< pointer to #phNfc_PowerState_t */

/**
 * \ingroup grp_hal_common
 *
 * \brief Enum definition contains supported RF Technologies
 */
typedef enum phNfc_eRfTechnologies
{
    phNfc_RfTechnologiesNfc_A = 0x00,        /**<NFC A technology */
    phNfc_RfTechnologiesNfc_B = 0x01,        /**<NFC B technology */
    phNfc_RfTechnologiesNfc_F = 0x02         /**<NFC F technology */
} phNfc_eRfTechnologies_t;

/**
 * \ingroup grp_hal_common
 *
 * \brief Enum definition contains supported RF Protocols
 */
typedef enum phNfc_eRfProtocols
{
    phNfc_RfProtocolsUnknownProtocol    = 0x00, /**<Protocol is not known */
    phNfc_RfProtocolsT1tProtocol        = 0x01, /**<Type 1 Tag protocol */
    phNfc_RfProtocolsT2tProtocol        = 0x02, /**<Type 2 Tag protocol */
    phNfc_RfProtocolsT3tProtocol        = 0x03, /**<Type 3 Tag protocol */
    phNfc_RfProtocolsIsoDepProtocol     = 0x04, /**<ISO DEP protocol */
    phNfc_RfProtocolsNfcDepProtocol     = 0x05  /**<NFC DEP protocol (already in use incase of DH-NFCEE)*/
} phNfc_eRfProtocols_t;

/**
 * \ingroup grp_hal_common
 * \brief Technology based listen mode routing
 */

typedef struct phNfc_TechBasedRtngValue
{
    phNfc_PowerState_t tPowerState;          /**< Power state */
    phNfc_eRfTechnologies_t tRfTechnology;   /**< A valid RF Technology */
}phNfc_TechnBasedRtngValue_t, *pphNfc_TechnBasedRtngValue_t;/**< pointer to #phNfc_TechnBasedRtngValue_t */

/**
 * \ingroup grp_hal_common
 * \brief Protocol based listen mode routing
 */

typedef struct phNfc_ProtoBasedRtngValue
{
    phNfc_PowerState_t tPowerState;       /**< Power state */
    phNfc_eRfProtocols_t tRfProtocol;      /**< A valid RF Protocol */
}phNfc_ProtoBasedRtngValue_t, *pphNfc_ProtoBasedRtngValue_t;/**< pointer to #phNfc_ProtoBasedRtngValue_t */

/**
 * \ingroup grp_hal_common
 * \brief AID (Application Identifier) based listen mode routing
 */

typedef struct phNfc_AidBasedRtngValue
{
    phNfc_PowerState_t tPowerState;     /**< Power state */
    uint8_t aAid[MAX_AID_LEN];          /**< A buffer containing AID (5-16 bytes) */
    uint8_t bAidSize;                   /**< Size of AID stored in 'aAid' */
}phNfc_AidBasedRtngValue_t, *pphNfc_AidBasedRtngValue_t;/**< pointer to #phNfc_AidBasedRtngValue_t */

/**
 * \ingroup grp_hal_common
 * \brief Listen mode routing entry
 */

typedef struct phNfc_RtngConfig
{
    void                      *hSecureElement;  /**< Handle to Secure Element */
    phNfc_eLstnModeRtngType_t Type;             /**< The type filed of 'TLV' coding for Listen Mode Routing */
    union
    {
        phNfc_TechnBasedRtngValue_t tTechBasedRtngValue;  /**< Technology based routing value */
        phNfc_ProtoBasedRtngValue_t tProtoBasedRtngValue; /**< Protocol based routing value */
        phNfc_AidBasedRtngValue_t   tAidBasedRtngValue;   /**< Aid based routing value */
    }LstnModeRtngValue;                                   /**< Value filed of Listen mode routing entry */
}phNfc_RtngConfig_t, *pphNfc_RtngConfig_t;/**< pointer to #phNfc_RtngConfig_t*/

/** \ingroup grp_hal_common
*
*  \brief Type of the Protocol Supported
*
*  This enumeration is used to Specify the Type of the Protocol supported
*  in the NFC Controller
*  \note None.
*/
typedef enum phNfc_eEmuProtocol
{
    NFC_EMU_PROTOCOL_T1T = 0x01U,
    NFC_EMU_PROTOCOL_T2T = 0x02U,
    NFC_EMU_PROTOCOL_T3T = 0x03U,
    NFC_EMU_PROTOCOL_T4T = 0x04U,
    NFC_EMU_PROTOCOL_MIFARE_PROPRIETARY = 0x05U,
    NFC_EMU_PROTOCOL_UNKNOWN = 0xFFU
} phNfc_eEmuProtocol_t;

/** \ingroup grp_hal_common
*
*  \brief Type of the Technology Supported
*
*  This enumeration is used to Specify the Type of the Protocol supported
*  in the NFC Controller
*  \note None.
*/
typedef enum phNfc_eEmuTechnology
{
    NFC_EMU_TECH_A = 0x00U,
    NFC_EMU_TECH_B  = 0x01U,
    NFC_EMU_TECH_F = 0x02U,
    NFC_EMU_TECH_15693 = 0x03U,
    NFC_EMU_TECH_UNKNOWN = 0xFFU
} phNfc_eEmuTechnology_t;

/** \ingroup grp_hal_common
*
*  \brief State of the Power Mode
*
*  This enumeration is used to Specify the State of the Power Mode
*  \note None.
*/
typedef enum phNfc_ePowerState
{
    NFC_POWER_NONE = 0x00U,
    NFC_POWER_FULL = 0x01U,
    NFC_POWER_LOW  = 0x02U,
    NFC_POWER_BOTH = 0x03U
} phNfc_ePowerState_t;

/** \ingroup grp_hal_common
*
*  \brief Location of the Emulation Support
*
*  This enumeration is used to Specify the Location of the Emulation supported
*  in the NFC Controller
*  \note None.
*/
typedef enum phNfc_eEmuLocation
{
    NFC_EMU_LOCATION_NONE = 0x00U,
    NFC_EMU_LOCATION_UICC = 0x01U,
    NFC_EMU_LOCATION_EMBEDDED_SE = 0x02U,
    NFC_EMU_LOCATION_HOST = 0x04U,
    NFC_EMU_LOCATION_UNKNOWN = 0xFFU
} phNfc_eEmuLocation_t;

/** \ingroup grp_hal_common
*
* \if hal
*  \brief Information for the Configuring the UICC
* \else
*  \brief HAL-Specific
* \endif
*
*  The <em> \ref phNfc_sUiccEmuCfg structure </em> holds the information
*   to configure the UICC Host.
*
*  \note None.
*/
typedef struct phNfc_sUiccEmuCfg
{
    uint8_t             enableUicc;
    uint8_t             uiccEmuSupport;
    uint8_t             uiccReaderSupport;
    uint8_t             lowPowerMode;
}phNfc_sUiccEmuCfg_t;

typedef struct phNfc_sHostEmuCfg_A
{
    uint8_t                 enableEmulation;    
    phNfc_sIso14443AInfo_t  hostEmuCfgInfo;     
    uint8_t                 enableCID;          
}phNfc_sHostEmuCfg_A_t;

typedef struct phNfc_sHostEmuCfg_B
{
    uint8_t                 enableEmulation;    
    phNfc_sIso14443BInfo_t  hostEmuCfgInfo;     
}phNfc_sHostEmuCfg_B_t;

/** \ingroup grp_hal_common
 *
 * The <em> \ref phNfc_sHostEmuCfg_F structure </em> holds the information
 * to configure the Felica Host Emulation.
 */

typedef struct phNfc_sHostEmuCfg_F
{
    uint8_t                 enableEmulation;    
}phNfc_sHostEmuCfg_F_t;


/** \ingroup grp_hal_common
 *
 * The <em> \ref phNfc_sEmulationCfg structure </em> holds the information
 * required for the device to act as a Tag or NFCIP1 Target.
 */

typedef struct phNfc_sEmulationCfg
{
    phNfc_HostType_t        hostType;   
    phNfc_eEmulationType_t  emuType;    
    union phHal_uEmuConfig  /**< Emulation configuration */
    {
        phNfc_sSmartMX_Cfg_t    smartMxCfg;     
        phNfc_sHostEmuCfg_A_t   hostEmuCfg_A;   
        phNfc_sHostEmuCfg_B_t   hostEmuCfg_B;   
        phNfc_sHostEmuCfg_F_t   hostEmuCfg_F;   
        phNfc_sUiccEmuCfg_t     uiccEmuCfg;     
    }config;
}phNfc_sEmulationCfg_t;

/** \ingroup grp_hal_common
*
*  The <em> \ref phNfc_sReaderCfg structure </em> holds the information
*   to configure the Reader A or Reader B parameters.
*/

typedef struct phNfc_sReaderCfg
{
    phNfc_eRFDevType_t    readerType;
    union phNfc_uReaderCfg /**<Reader configuration for 14443A/14443B*/
    {
        phNfc_sIso14443ACfg_t       Iso14443ACfg;
        phNfc_sIso14443BCfg_t       Iso14443BCfg;
    }config;
}phNfc_sReaderCfg_t;


/** \ingroup grp_hal_common
*
*  The <em> \ref phNfc_sSEProtectionCfg structure </em> holds the
*  information to configure the Secure Element Protection configuration.
*/

typedef struct phNfc_sSEProtectionCfg
{
    uint8_t         mode;
}phNfc_sSEProtectionCfg_t;


/** \ingroup  grp_hal_common
*
*  The <em> Poll configuration structure </em> holds information about the
*  enabling the the type of discovery required by the application. This
*  structure is the input parameter for the discovery call
*
*/

typedef struct NfcPollConfig
{
    uint8_t Bailout;  /**<0 = Bailout disable; 1 = Bailout Enable*/
    uint8_t PollFreq; /**<Poll Frequency,  not applicable to Kovio and */
}NfcPollConfig_t;

/** \ingroup  grp_hal_common
 *
 *  \brief Configuration required for discovery
 *  \note All members of this structure are input parameters [out].
 */

typedef struct phNfc_sADD_Cfg
{
    union
    {
        phNfc_sPollDevInfo_t    PollCfgInfo;            /**< Enable/Disable Specific
                                                             Reader Functionality and
                                                             Card Emulation */
        unsigned int            PollEnabled;            /**< Can be used to set polling 'Off'
                                                             by setting PollEnabled to zero */
    }PollDevInfo;

    uint32_t                    Duration;               /**< Duration of listen or idle
                                                             period of the polling loop in microseconds */
    uint8_t                     NfcIP_Mode;             /**< Select the P2P
                                                             speeds using phHal_eP2PMode_t type.
                                                             This is used to enable NFC-IP Discovery
                                                             The related Reader Type will be implicitly
                                                             selected */
    uint8_t                     NfcIP_Tgt_Disable;      /**< Flag to disable the NFCIP1 Target */
    uint8_t                     NfcIP_Tgt_Mode_Config;  /**< Flag to select the P2P target listen mode technology */
    uint8_t                     CE_Mode_Config;         /**< Flag to select the card emulation listen mode technology */

    NfcPollConfig_t             aPollParms[32];         /**< Each field of array is valid only bitfield in PollCfgInfo is valid*/
    phNfc_sFelicaCfg_t          FelicaPollCfg;          /**< Felica poll configuration*/

}phNfc_sADD_Cfg_t;


/** \ingroup grp_hal_common
*
* \brief Configuration information.
*  The <em> \ref phNfc_uConfig_t structure </em> holds the information
*  required for Configuring the Device.
*/

typedef union phNfc_uConfig
{
    phNfc_sEmulationCfg_t   emuConfig;            
    phNfc_sNfcIPCfg_t       nfcIPConfig;          /**< Gives the information about the General Bytes for NFC-IP Communication. */
    phNfc_sReaderCfg_t      readerConfig;      
    phNfc_sSEProtectionCfg_t protectionConfig;  
}phNfc_uConfig_t;

/** MACRO to declare temporarily unused variables */
#define UNUSED(X) UNREFERENCED_PARAMETER(X);
