/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>

/** Maximum size of NFCID1 - Mapped to UID value for type-A target */
#define PH_NCINFCTYPES_MAX_UID_LENGTH           (0x0AU)
#define PH_NCINFCTYPES_KOVIO_TAG_ID_LENGTH      (0x20)  /**< Max length of the Kovio tag barcode */
#define PH_NCINFCTYPES_15693_UID_LENGTH         (0x08U) /**< Length of the Inventory bytes for ISO15693 Tag */

/** Maximum length of ATR_RES (General Bytes) length expected */
#define PH_NCINFCTYPES_MAX_ATR_LENGTH           (0x30U)
#define PH_NCINFCTYPES_MAX_HIST_BYTES           (0x0FU)       /**< Max Historical bytes returned by Type A tag */
#define PH_NCINFCTYPES_ATQA_LENGTH              (0x02U)       /**< ATQA length */
#define PH_NCINFCTYPES_PUPI_LENGTH              (0x04U)       /**< PUPI length */
#define PH_NCINFCTYPES_APP_DATA_B_LENGTH        (0x04U)       /**< Application Data length for Type B */
#define PH_NCINFCTYPES_PROT_INFO_B_LENGTH       (0x03U)       /**< Protocol info length for Type B */
#define PH_NCINFCTYPES_ATQB_LENGTH              (0x0BU)       /**< ATQB length */
#define PH_NCINFCTYPES_MAX_SENSB_LEN            (0x0CU)       /**< Length of sensb response */
#define PH_NCINFCTYPES_ATTRIBRESP_HLR_LEN       (0x20U)       /**< Length of attrib response high layer */

/** Max Length of SENSF response */
#define PH_NCINFCTYPES_SENS_RES_LEN              (0x12U)       /**< This length is valid for SENS_RES and SENSF_RES */
#define PH_NCINFCTYPES_MIN_SENF_LEN              (0X10U)       /**< Min length of SensF parameter */
#define PH_NCINFCTYPES_MAX_SENSF_LEN             (0x12U)       /**< Max length of SensF parameter */
#define PH_NCINFCTYPES_NFCID2_LEN                (0X08U)       /**< NfcId2 length */

#define PH_NCINFCTYPES_FEL_SYS_CODE_LEN          (0x02U)       /**< Felica System Code Length */
#define PH_NCINFCTYPES_FEL_ID_LEN                (0x08U)       /**< Felica current ID Length */
#define PH_NCINFCTYPES_FEL_PM_LEN                (0x08U)       /**< Felica current PM Length */
#define PH_NCINFCTYPES_NFCID1_4BYTES             (0x04)        /**< 4 bytes NFCID1 */
#define PH_NCINFCTYPES_NFCID1_7BYTES             (0x07)        /**< 7 bytes NFCID1 */
#define PH_NCINFCTYPES_MAX_NFCID1_SIZE           (0x0AU)       /**< Max NFCID1 size */
#define PH_NCINFCTYPES_MAX_NFCID3_SIZE           (0x0AU)       /**< Max NFCID3 size */
#define PH_NCINFCTYPES_MAX_NFCID2_SIZE           (0x08U)       /**< NFCID2 size */
#define PH_NCINFCTYPES_DATA_XCHG_PARAMS_LEN      (0x03U)       /**< Length of Data Exchange Params */

#define PH_NCINFCTYPES_ISO15693_ACTV_PARAMS_LEN  (0x0AU)       /**< Length of ISO15693 Activation Params */

#define PH_NCINFCTYPES_DID_LEN                  (0x01U)        /**< Length of DID (Device Identification number) support */
#define PH_NCINFCTYPES_SEND_BIT_RATE_LEN        (0x01U)        /**< Length of Send Bit rate supported by Initiator/target */
#define PH_NCINFCTYPES_RECV_BIT_RATE_LEN        (0x01U)        /**< Length of Receive Bit rate supported by Initiator/target */
#define PH_NCINFCTYPES_TIMEOUT_LEN              (0x01U)        /**< Time out (valid only for ATR_RES sent from remote P2P Target device) */
#define PH_NCINFCTYPES_PPT_LEN                  (0x01U)        /**< Length of Indicates Length Reduction field and the
                                                                  presence of optional parameters */

#define PH_NCINFCTYPES_MAXFRAMELEN              (0x04U)         /**< Max frame length bit location */
#define PH_NCINFCTYPES_MAXFRAMELEN_BITMASK      (0x03U)         /**< Max frame length bit mask */
#define PH_NCINFCTYPES_MAXFRAMELEN_MAX          (0x256U)        /**< Max frame length */
#define PH_NCINFCTYPES_MAXFRAMELEN_LIMITER      (0x02U)         /**< Max frame length as per [DIGITAL] is 254 */
#define PH_NCINFCTYPES_GENERALBYTES_SUPP        (0x02)          /**< General bytes bitmask */
#define PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET  (0x0FU)         /**< ATR_RES general bytes length offset */
#define PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET  (0x0EU)         /**< ATR_REQ general bytes length offset */

#define PH_NCINFCTYPES_RF_INTFS_PROP_MIN        (0x80)
#define PH_NCINFCTYPES_RF_INTFS_PROP_MAX        (0xFE)
#define PH_NCINFCTYPES_RF_PROTOS_PROP_MIN       (0x80)
#define PH_NCINFCTYPES_RF_PROTOS_PROP_MAX       (0xFE)
#define PH_NCINFCTYPES_RF_TECH_POLL_PROP_MIN    (0x70)
#define PH_NCINFCTYPES_RF_TECH_POLL_PROP_MAX    (0x7F)
#define PH_NCINFCTYPES_RF_TECH_LISTEN_PROP_MIN  (0xF0)
#define PH_NCINFCTYPES_RF_TECH_LISTEN_PROP_MAX  (0xFF)
#define PH_NCINFCTYPES_BIT_RATE_PROP_MIN        (0x80)
#define PH_NCINFCTYPES_BIT_RATE_PROP_MAX        (0xFE)

/**< Max ATR length */
#define PH_NCINFCTYPES_ATR_MAX_LEN              (PH_NCINFCTYPES_MAX_NFCID3_SIZE+    \
                                                 PH_NCINFCTYPES_DID_LEN+            \
                                                 PH_NCINFCTYPES_SEND_BIT_RATE_LEN+  \
                                                 PH_NCINFCTYPES_RECV_BIT_RATE_LEN+  \
                                                 PH_NCINFCTYPES_TIMEOUT_LEN+        \
                                                 PH_NCINFCTYPES_PPT_LEN+            \
                                                 PH_NCINFCTYPES_MAX_ATR_LENGTH)

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains supported RF Protocols
 */
typedef enum phNciNfc_RfProtocols
{
    phNciNfc_e_RfProtocolsUnknownProtocol = 0x00,/**<Protocol is not known */
    phNciNfc_e_RfProtocolsT1tProtocol = 0x01,    /**<Type 1 Tag protocol */
    phNciNfc_e_RfProtocolsT2tProtocol = 0x02,    /**<Type 2 Tag protocol */
    phNciNfc_e_RfProtocolsT3tProtocol = 0x03,    /**<Type 3 Tag protocol */
    phNciNfc_e_RfProtocolsIsoDepProtocol = 0x04, /**<ISO DEP protocol */
    phNciNfc_e_RfProtocolsNfcDepProtocol = 0x05, /**<NFC DEP protocol */
    phNciNfc_e_RfProtocols15693Protocol = 0x06,  /**<15693 protocol */
    phNciNfc_e_RfProtocolsMifCProtocol = 0x80,   /**<Mifare Classic protocol */
    phNciNfc_e_RfProtocolsKovioProtocol = 0x8A,  /**<Kovio protocol */
} phNciNfc_RfProtocols_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Supported RF Interfaces
 */
typedef enum phNciNfc_RfInterfaces
{
    phNciNfc_e_RfInterfacesNfceeDirect_RF = 0x00,   /**<Nfcee Direct RF Interface */
    phNciNfc_e_RfInterfacesFrame_RF = 0x01,         /**<Frame RF Interface */
    phNciNfc_e_RfInterfacesISODEP_RF = 0x02,        /**<ISO DEP RF Interface */
    phNciNfc_e_RfInterfacesNFCDEP_RF = 0x03,        /**<NFC DEP RF Interface */
    phNciNfc_e_RfInterfacesTagCmd_RF = 0x80,        /**<Tag-Cmd RF Interface (Nxp prop) */
} phNciNfc_RfInterfaces_t;

/**
 * \brief Enum definition contains RF technology modes supported.
 * This information is a part of RF_DISCOVER_NTF or RF_INTF_ACTIVATED_NTF.
 */
typedef enum phNciNfc_RfTechMode
{
    phNciNfc_NFCA_Poll = 0x00,          /**<Nfc A Technology in Poll Mode */
    phNciNfc_NFCB_Poll = 0x01,          /**<Nfc B Technology in Poll Mode */
    phNciNfc_NFCF_Poll = 0x02,          /**<Nfc F Technology in Poll Mode */
    phNciNfc_NFCA_Active_Poll = 0x03,   /**<Nfc A Technology in Active Poll Mode */
    phNciNfc_NFCF_Active_Poll = 0x05,   /**<Nfc F Technology in Active Poll Mode */
    phNciNfc_NFCISO15693_Poll = 0x06,   /**<Nfc ISO15693 Technology in Poll Mode */
    phNciNfc_NFCA_Listen = 0x80,        /**<Nfc A Technology in Listen Mode */
    phNciNfc_NFCB_Listen = 0x81,        /**<Nfc B Technology in Listen Mode */
    phNciNfc_NFCF_Listen = 0x82,        /**<Nfc F Technology in Listen Mode */
    phNciNfc_NFCA_Active_Listen = 0x83, /**<Nfc A Technology in Active Listen Mode */
    phNciNfc_NFCF_Active_Listen = 0x85, /**<Nfc F Technology in Active Listen Mode */
    phNciNfc_NFCISO15693_Active_Listen = 0x86,  /**<Nfc ISO15693 Technology in Listen Mode */
    phNciNfc_NFCA_Kovio_Poll = 0x77,    /**<Nfc Kovio Technology in Poll Mode */
} phNciNfc_RfTechMode_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains supported Bitrates
 */
typedef enum phNciNfc_BitRates
{
    phNciNfc_e_BitRate106 = 0x00,            /**<Bit rate at 106 Kbit/Sec */
    phNciNfc_e_BitRate212 = 0x01,            /**<Bit rate at 212 Kbit/Sec*/
    phNciNfc_e_BitRate424 = 0x02,            /**<Bit rate at 424 Kbit/Sec*/
    phNciNfc_e_BitRate848 = 0x03,            /**<Bit rate at 848 Kbit/Sec*/
    phNciNfc_e_BitRate1696 = 0x04,           /**<Bit rate at 1696 Kbit/Sec*/
    phNciNfc_e_BitRate3392 = 0x05,           /**<Bit rate at 3392 Kbit/Sec*/
    phNciNfc_e_BitRate6784 = 0x06,           /**<Bit rate at 6784 Kbit/Sec*/
    phNciNfc_e_BitRate26 = 0x20,             /**<Bit rate at 26 Kbit/Sec*/
}phNciNfc_BitRates_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief This is used to identify the exact device type
 */
typedef enum phNciNfc_RFDevType
{
    phNciNfc_eUnknown_DevType = 0x00U,

    /* Generic PICC Type */
    phNciNfc_ePICC_DevType,
    /* Specific PICC Devices */
    /* This PICC type explains that the card is compliant to the
        ISO 14443-1 and 2A specification. This type can be used for the
        cards that is supporting these specifications */
    phNciNfc_eISO14443_A_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-4A specification */
    phNciNfc_eISO14443_4A_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-3A specification */
    phNciNfc_eISO14443_3A_PICC,
    /* This PICC type explains that the card is Mifare UL/1k/4k and
    also it is compliant to ISO 14443-3A. There can also be other
    ISO 14443-3A cards, so the phNciNfc_eISO14443_3A_PICC is also used for
    PICC detection */
    phNciNfc_eMifareUL_PICC,
    phNciNfc_eMifare1k_PICC,
    phNciNfc_eMifare4k_PICC,
    phNciNfc_eMifareMini_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-1, 2 and 3B specification */
    phNciNfc_eISO14443_B_PICC,
    /* This PICC type explains that the card is compliant to the
        ISO 14443-4B specification */
    phNciNfc_eISO14443_4B_PICC,
    /* This PICC type explains that the card is B-Prime type */
    phNciNfc_eISO14443_BPrime_PICC,
    phNciNfc_eFelica_PICC,
    phNciNfc_eJewel_PICC,
    /* This PICC type explains that the card is ISO15693 type */
    phNciNfc_eISO15693_PICC,
    /* This PICC type explains that the card is EpcGen2 type */
    phNciNfc_eEpcGen2_PICC,
    /* This PICC type explains that the card is Kovio type */
    phNciNfc_eKovio_PICC,

    /* NFC-IP1 Device Types */
    phNciNfc_eNfcIP1_Target,
    phNciNfc_eNfcIP1_Initiator,

    /* Other Sources */
    phNciNfc_eInvalid_DevType

}phNciNfc_RFDevType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains Deactivation types
 */
typedef enum phNciNfc_DeActivateType
{
    phNciNfc_e_IdleMode = 0,    /**<Deactivate to Idle Mode */
    phNciNfc_e_SleepMode = 1,   /**<Deactivate to Sleep Mode */
    phNciNfc_e_SleepAfMode = 2,/**<Deactivate to SleepAF Mode */
    phNciNfc_e_DiscMode = 3   /**<Deactivate to Discovery Mode */
}phNciNfc_DeActivateType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains Deactivation Reason
 */
typedef enum phNciNfc_DeActivateReason
{
    phNciNfc_e_DhRequest = 0,    /**<Deactivate to Idle Mode */
    phNciNfc_e_EndPoint = 1,   /**<Deactivate to Sleep Mode */
    phNciNfc_e_RfLinkLoss = 2,/**<Deactivate to SleepAF Mode */
    phNciNfc_e_NfcB_BadAfi = 3,   /**<Deactivate to Discovery Mode */
    phNciNfc_e_DhRequestFailed = 4, /**<Deactivate to Idle Mode or Poll Active */
}phNciNfc_DeActivateReason_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief This contains the destination type for logical connection
 */
typedef enum phNciNfc_DestType
{
    phNciNfc_e_NFCC_LOOPBACK        = 0x01U,    /**<connection between DH & NFCC for loop-back mode */
    phNciNfc_e_REMOTE_NFC_ENDPOINT  = 0x02U,    /**<for Remote NFC Endpoint communication */
    phNciNfc_e_NFCEE                = 0x03U,    /**<for NFCEE communication */
    phNciNfc_e_RESERVED_DEST_TYPE1  = 0xC1U,    /**<for proprietary use */
    phNciNfc_e_UNKNOWN_DEST_TYPE    = 0XFFU     /**<invalid DestinationType */
}phNciNfc_DestType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Structure contains buffer where payload of the received data packet
 *        shall be stored and length of the payload stored in the buffer.
 */
typedef struct phNciNfc_Data
{
    uint8_t *pBuff;     /**< Buffer to store received data packet's payload */
    uint16_t wLen;      /**< Length of the payload */
}phNciNfc_Data_t, *pphNciNfc_Data_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Transaction Completion Information Structure of TML.
 */
typedef struct phNciNfc_TransactInfo
{
    void*    pContext;  /**<Holds Nci Context (Nci Handle) incase of Init upper layer call back*/
    uint8_t* pbuffer;   /**<Buffer containing transaction information*/
    uint16_t wLength;   /**<Size of pbuffer*/
}phNciNfc_TransactInfo_t,*pphNciNfc_TransactInfo_t; /**< Instance of Transaction structure */

/*!
 * \ingroup grp_nci_nfc
 *
 * \brief RATS Response Params structure
 */
typedef struct phNciNfc_RATSResp
{
    uint8_t   bFormatByte;                 /**< Format Byte */
    uint8_t   bIByteTA;                    /**< Interface Byte TA(1) */
    uint8_t   bIByteTB;                    /**< Interface Byte TB(1) */
    uint8_t   bIByteTC;                    /**< Interface Byte TC(1) */
    uint8_t   bHistByte[PH_NCINFCTYPES_MAX_HIST_BYTES];   /**< Historical Bytes - Max size 15 */
} phNciNfc_RATSResp_t;

/** The <em> Reader A structure </em> includes the available information
*  related to the discovered ISO14443A remote device. This information
*  is updated for every device discovery.
*/
typedef struct phNciNfc_Iso14443AInfo
{
    uint8_t         Uid[PH_NCINFCTYPES_MAX_UID_LENGTH]; /**< UID information of the TYPE A
                                                        Tag Discovered NFCID1 -
                                                        Considering max size of NFCID1*/
    _Field_range_(<=, PH_NCINFCTYPES_MAX_UID_LENGTH)
    uint8_t         UidLength;                          /**< UID information length, shall not be greater
                                                        than PHNCINFC_MAX_UID_LENGTH i.e., 10 */
    uint8_t         AppData[PH_NCINFCTYPES_MAX_ATR_LENGTH]; /**< Application data information of the
                                                        tag discovered (= Historical bytes for
                                                        type A) */
    uint8_t         AppDataLength;                      /**< Application data length */
    uint8_t         Sak;                                /**< SAK informationof the TYPE ATag Discovered
                                                        Mapped to SEL_RES Response*/
    uint8_t         AtqA[PH_NCINFCTYPES_ATQA_LENGTH];        /**< ATQA informationof the TYPE A
                                                        Tag Discovered */
    uint8_t         MaxDataRate;                        /**< Maximum data rate supported by the TYPE A
                                                        Tag Discovered */
    uint8_t         Fwi_Sfgt;                           /**< Frame waiting time and start up frame guard
                                                        time as defined in ISO/IEC 14443-4[7] for type A */
    uint8_t         bSensResResp[2];                    /**< SENS_RES Response */
    uint8_t         bSelResRespLen;                     /**< SEL_RES Response Length */
    uint8_t         bHRx[2];                            /**< HRx Response */
    uint8_t         bHRxLen;                            /**< HRx Response Length */
    uint8_t         bRatsRespLen;                       /**< Length of RATS Response */
    phNciNfc_RATSResp_t   tRatsResp;                    /**< RATS Response Info */
    phNfc_eMifareULType_t ULType;                       /**< [Mifare Ultralight only (SAK == 0x00)] The type of
                                                        Mifare Ultralight card */
    uint8_t         DataAreaSize;                       /**< [Mifare Ultralight EV1 only] The size of the
                                                        data area in bytes, obtained from the GET_VERSION
                                                        command */
}phNciNfc_Iso14443AInfo_t;

typedef struct phNciNfc_KovioInfo
{
    uint8_t         TagIdLength;
    uint8_t         TagId[PH_NCINFCTYPES_KOVIO_TAG_ID_LENGTH];
}phNciNfc_KovioInfo_t;

typedef struct phNciNfc_ListenNfcAInfo
{
    uint8_t bDummy; /*Nothing is defined in NCI*/
}phNciNfc_ListenNfcAInfo_t;

typedef struct phNciNfc_ListenNfcBInfo
{
    uint8_t bDummy; /*Nothing is defined in NCI*/
}phNciNfc_ListenNfcBInfo_t;
/*!
 * \ingroup grp_nci_nfc
 *
 * \brief ATTRIB Response Params structure
 */
typedef struct phNciNfc_ATTRIBResp
{
    uint8_t   bMbliDidByte;      /**< b4-b1 nibble = DID, b8-b5 nibble = MBLI */
    uint8_t   pHighLayerResp[PH_NCINFCTYPES_ATTRIBRESP_HLR_LEN];   /**< Higher layer Rsp for ATTRIB Cmd */
} phNciNfc_ATTRIBResp_t;

/** The <em> Reader B structure </em> includes the available information
*  related to the discovered ISO14443B remote device. This information
*  is updated for every device discovery.
*/
typedef struct phNciNfc_Iso14443BInfo
{
    union phNciNfc_AtqBInfo
    {
        struct phNciNfc_AtqBResInfo
        {
            uint8_t         Pupi[PH_NCINFCTYPES_PUPI_LENGTH];            /**< PUPI information  of the TYPE B
                                                                    Tag Discovered */
            uint8_t         AppData[PH_NCINFCTYPES_APP_DATA_B_LENGTH];   /**< Application Data  of the TYPE B
                                                                    Tag Discovered */
            uint8_t         ProtInfo[PH_NCINFCTYPES_PROT_INFO_B_LENGTH]; /**< Protocol Information  of the TYPE B
                                                                    Tag Discovered */
        }AtqResInfo;
        uint8_t         AtqRes[PH_NCINFCTYPES_ATQB_LENGTH];              /**< ATQB Response Information of TYPE B
                                                                    Tag Discovered */
    }AtqB;
    uint8_t         HiLayerResp[PH_NCINFCTYPES_MAX_ATR_LENGTH];          /**< Higher Layer Response information
                                                                    in answer to ATRRIB Command for Type B */
    uint8_t         HiLayerRespLength;                              /**< Higher Layer Response length */
    uint8_t         Afi;                                            /**< Application Family Identifier of TYPE B
                                                                    Tag Discovered */
    uint8_t         MaxDataRate;                                    /**< Maximum data rate supported by the TYPE B
                                                                    Tag Discovered */
    uint8_t         bSensBRespLen;/**< SensB-Response length */
    /** SENSB_RES can be maximum of 12 bytes */
    uint8_t         aSensBResp[PH_NCINFCTYPES_MAX_SENSB_LEN];/**< SensB-Response */
    uint8_t         bAttribRespLen;                       /**< Length of ATTRIB Response */
    phNciNfc_ATTRIBResp_t  tAttribResp;                   /**< ATTRIB Response Info */
}phNciNfc_Iso14443BInfo_t;

/*!
 * \ingroup grp_nci_nfc
 *
 *  The <em> NFCIP1 structure </em> includes the available information
 *  related to the discovered NFCIP1 remote device. This information
 *  is updated for every device discovery.
 */
typedef struct phNciNfc_NfcIPInfo
{
    /* Contains the random NFCID3I conveyed with the ATR_REQ.
        always 10 bytes length
        or contains the random NFCID3T conveyed with the ATR_RES.
        always 10 bytes length */
    uint8_t         NFCID[PH_NCINFCTYPES_MAX_NFCID3_SIZE];
    uint8_t         NFCID_Length;
    /**< SAK information of the tag discovered */
    uint8_t         SelRes;
    uint8_t         SelResLen;
    /**< ATQA information of the tag discovered */
    uint8_t         SensRes[PH_NCINFCTYPES_SENS_RES_LEN];
    uint8_t         SensResLength;
    /**< Is Detection Mode of the NFCIP Target Active */
    uint8_t         Nfcip_Active; /* 0: passive; 1: active */

    /**< Data rate supported by the NFCIP device (Initiator to target speed if remote device is initiator;
                                                  Target to Initiator speed if remote device is Target)*/
    phNciNfc_BitRates_t         Nfcip_Datarate;

    /* Holds ATR_RES if remote device is a P2P target and ATR_REQ if remote device is a P2P initiator */
    uint8_t         aAtrInfo[PH_NCINFCTYPES_ATR_MAX_LEN];
    uint8_t         bATRInfo_Length;
}phNciNfc_NfcIPInfo_t;

/*!
 * \ingroup grp_nci_nfc
 *
 *  The <em> EpcGen structure </em> includes the available information
 *  related to the discovered EpcGen remote device. This information
 *  is updated for every device discovery.
 */

typedef struct phNciNfc_EpcGen
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
}phNciNfc_EpcGen_t;

/**
* This structure shall holds NfcIP configuration to be set
*/
typedef struct phNciNfc_NfcIPCfg
{
    /* ATR_RES = General bytes length, Max length = 48 bytes */
    uint8_t aGeneralBytes[PH_NCINFCTYPES_MAX_ATR_LENGTH];
    uint8_t bGeneralBytesLength;
}phNciNfc_NfcIPCfg_t,*pphNciNfc_NfcIPCfg_t; /**< pointer to #phNciNfc_sNfcIPCfg_t */

/** The <em> Felica Reader structure </em> includes the available information
*  related to the discovered Felica remote device. This information
*  is updated for every device discovery.
*/
typedef struct phNciNfc_FelicaInfo
{
    union{
        uint8_t   bNfcId2[PH_NCINFCTYPES_FEL_ID_LEN];     /**< NFCID2 of NFC Forum Device (for NFC-DEP ? )*/
        uint8_t   bIDm[PH_NCINFCTYPES_FEL_ID_LEN];        /**< ManufacturerId of Type 3 Tag (for T3T Protocol) */
    }DevIdType;                                            /**< One of the above 2 ids,based on currently active protocol */
    uint8_t     IDmLength;                              /**< IDm length, shall not be greater
                                                        than PHHAL_FEL_ID_LEN i.e., 8 */
    uint8_t     PMm[PH_NCINFCTYPES_FEL_PM_LEN];              /**< Current PM of Felica tag */
    uint8_t     SystemCode[PH_NCINFCTYPES_FEL_SYS_CODE_LEN]; /**< System code of Felica tag */
    uint8_t   bBitRate;                               /**< Bit Rate supported */
    uint8_t   bSensFRespLen;                          /**< Length of SENSF-RES */
    /** SENSF_RES can be maximum of 18 bytes */
    uint8_t     aSensFResp[PH_NCINFCTYPES_MAX_SENSF_LEN];    /**< SENSF-RES parameter */
}phNciNfc_FelicaInfo_t;

/** \ingroup grp_nci_nfc
*
*  \brief Remote Device Jewel Reader RF Gate Information Container
*
*  The <em> Jewel Reader structure </em> includes the available information
*  related to the discovered Jewel remote device. This information
*  is updated for every device discovery.
*/
typedef struct phNciNfc_JewelInfo
{
    uint8_t         Uid[PH_NCINFCTYPES_MAX_UID_LENGTH];  /**< UID information of the TYPE A
                                                         Tag Discovered */
    uint8_t         UidLength;                      /**< UID information length, shall not be greater
                                                    than PH_NCINFCTYPES_MAX_UID_LENGTH i.e., 10 */
    uint8_t         bSensResResp[2];                    /**< SENS_RES Response */
    uint8_t         HeaderRom0; /**< Header Rom byte zero */
    uint8_t         HeaderRom1; /**< Header Rom byte one */

}phNciNfc_JewelInfo_t;

/** \ingroup grp_hal_nfci
*
*  \brief Remote Device Reader 15693 RF Gate Information Container
*
*  The <em> Reader A structure </em> includes the available information
*  related to the discovered ISO15693 remote device. This information
*  is updated for every device discovery.
*  \note None.
*/
typedef struct phNciNfc_Iso15693Info
{
    uint8_t         Uid[PH_NCINFCTYPES_15693_UID_LENGTH];/**< UID information of the 15693
                                                              Tag Discovered */
    uint8_t         UidLength;                      /**< UID information length, shall not be greater
                                                    than PHHAL_15693_UID_LENGTH i.e., 8 */
    uint8_t         Dsfid;                          /**< DSF information of the 15693
                                                       Tag Discovered */
    uint8_t         Flags;                          /**< Information about the Flags
                                                        in the 15693 Tag Discovered */
    uint8_t         Afi;                            /**< Application Family Identifier of
                                                          15693 Tag Discovered */
}phNciNfc_Iso15693Info_t;

/** The Remote Device Information Union includes the available Remote Device Information
*  structures. Following the device detected, the corresponding data structure is used.
*/
typedef union phNciNfc_RemoteDevInfo
{
    phNciNfc_Iso14443AInfo_t          Iso14443A_Info;/**< Type A tag Info */
    phNciNfc_Iso14443BInfo_t          Iso14443B_Info;/**< Type B tag Info */
    phNciNfc_NfcIPInfo_t              NfcIP_Info;    /**< NfcIP Info */
    phNciNfc_FelicaInfo_t             Felica_Info;   /**< Type F tag Info */
    phNciNfc_Iso15693Info_t           Iso15693_Info; /**< Type ISO15693 tag Info */
    phNciNfc_EpcGen_t                 EpcGen_Info;   /**< Type E tag Info */
    phNciNfc_KovioInfo_t              Kovio_Info;   /**< Type Kovio tag Info */
    phNciNfc_JewelInfo_t              Jewel_Info;      /**< Jewel - Type 1 Tag Info */
    phNciNfc_ListenNfcAInfo_t         ListenNfcAInfo;  /*Type A Listen mode info*/
    phNciNfc_ListenNfcBInfo_t         ListenNfcBInfo;  /*Type A Listen mode info*/
}phNciNfc_RemoteDevInfo_t;

typedef struct phNciNfc_RemoteDevInformation
{
    uint8_t                    SessionOpened;       /**< Flag indicating the validity of
                                                    *   the handle of the remote device. */
    phNciNfc_RFDevType_t       RemDevType;         /**< Remote device type which says that remote
                                                    is Reader A or Reader B or NFCIP or Felica or
                                                    Reader B Prime or Jewel*/
    uint8_t bRfDiscId;                              /**< ID of the Tag */
    phNciNfc_RfInterfaces_t    eRfIf;               /**< RF Interface */
    phNciNfc_RfProtocols_t eRFProtocol;             /**< RF protocol of the target */
    phNciNfc_RfTechMode_t eRFTechMode;              /**< RF Technology mode of the discovered/activated target */
    uint8_t bMaxPayLoadSize;                        /**< Max data payload size*/
    uint8_t bInitialCredit;                         /**< Initial credit*/
    uint8_t bTechSpecificParamLen;                  /**< Technology Specific parameter length, for Debugging purpose only*/
    phNciNfc_RfTechMode_t eDataXchgRFTechMode;      /**< Data Exchange RF Technology mode of the activated target */
    uint8_t   bTransBitRate;                        /**< Transmit Bit Rate */
    uint8_t   bRecvBitRate;                         /**< Receive Bit Rate */
    phNciNfc_RemoteDevInfo_t tRemoteDevInfo;        /**<Structure object to #phNciNfc_RemoteDevInfo_t*/
}phNciNfc_RemoteDevInformation_t,*pphNciNfc_RemoteDevInformation_t;/**< Pointer to Remote Dev Info*/

/*!
 * \ingroup grp_nci_nfc
 *
 * \brief Generic Response Callback definition.
 * Generic callback definition used as callback type in few APIs below.
 * Note:
 * Status and error codes for this type of callback are documented in respective APIs wherever it is used.
 *
 *   param[in] pContext  Nci client context passed in the corresponding request before.
 *   param[in] wStatus - Status of the transaction
 */
typedef void (*pphNciNfc_IfNotificationCb_t)(
        void* pContext,
        NFCSTATUS status,
        void *pInfo
        );
