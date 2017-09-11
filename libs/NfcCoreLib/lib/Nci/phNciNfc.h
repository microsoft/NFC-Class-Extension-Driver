/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNciNfcTypes.h>

/*
 ################################################################################
 ****************************** Macro Definitions *******************************
 ################################################################################
 */

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of Higher layer inf of ATTRIB command
 */
#define PH_NCINFC_MAX_HIGHER_LAYER_INF_LEN           (0x30U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of General bytes of ATR REQ command
 */
#define PH_NCINFC_MAX_ATR_REQ_GEN_BYTES_LEN          (0x30U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length NFCID2
 */
#define PH_NCINFC_MAX_NFCID2_LEN                     (0x08U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length NFCID1
 */
#define PH_NCINFC_MAX_NFCID1_LEN                     (0x0AU)

/**
 * \ingroup grp_nci_nfc
 * \brief Length of NFCID0
 */
#define PH_NCINFC_NFCID0_LEN                         (0x04U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max size of Application data of SENSB_RES
 */
#define PH_NCINFC_APP_DATA_LEN                       (0x04U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of Type-A Historical bytes
 */
#define PH_NCINFC_MAX_HIST_BYTES_LEN                 (0x30U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of ATTRIB response Higher layer response field
 */
#define PH_NCINFC_MAX_HIGHER_LAYER_RES_LEN           (0x30U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of AID value in an AID based routing
 */
#define PH_NCINFC_MAX_AID_LEN                        (16U)

/**
 * \ingroup grp_nci_nfc
 * \brief Max length of ATR_RES General bytes
 */
#define PH_NCINFC_MAX_ATR_RES_GEN_BYTES_LEN          (0x30U)

/**
 * \ingroup grp_nci_nfc
 * \brief Maximum number of Discovery Configurations
 */
#define PH_NCINFC_MAXCONFIGPARAMS                    (0x06)

/**
 * \ingroup grp_nci_nfc
 * \brief Maximum number of Remote Devices which can be discovered
 */
#define PH_NCINFC_MAX_REMOTE_DEVICES                  (10U)

/**
 * \ingroup grp_nci_nfc
 * \brief Maximum number of T3T Identifiers
 */
#define PH_NCINFC_MAX_NUM_T3T_IDS                    (16U)

/**
 * \ingroup grp_nci_nfc
 * \brief Listen Nfc-F T3T Identifier length
 */
#define PH_NCINFC_T3TID_LEN                          (10U)

/**
 * \ingroup grp_nci_nfc
 * \brief Length of Listen mode Nfc-F T3T PMM
 */
#define PH_NCINFC_T3TPMM_LEN                         (0x08U)

/**
 * \ingroup grp_nci_nfc
 * Max length of NFCEE T3T Command set System code
 */
#define PH_NCINFC_NFCEE_T3T_MAX_SYSCODE_LEN             (2U)


/**
 * \ingroup grp_nci_nfc
 * Max length of NFCEE T3T Command set PMM information.
 */
#define PH_NCINFC_NFCEE_T3T_MAX_PMM_LEN                 (8U)

/**
 * \ingroup grp_nci_nfc
 * Max length of NFCEE T3T Command set Max T3t Entries.
 */
#define PH_NCINFC_NFCEE_T3T_MAX_ENTRIES                 (16U)

/**
 * \ingroup grp_nci_nfc
 * Max length of NFCEE T3T Command set Idm Parameter
 */
#define PH_NCINFC_NFCEE_T3T_MAX_IDM_LEN                 (8U)

/**
 * \ingroup grp_nci_nfc
 * Max number of NFCEE TLV information
 */
#define PH_NCINFC_NFCEE_INFO_TLV_MAX                    (0x03)

/**
 * \ingroup grp_nci_nfc
 * Max number of protocol supported by NFCEE(for the given NFCEE)
 */
#define PH_NCINFC_NFCEE_SUPPORTEDPROTO_MAX              (0x05)

/**
 * \ingroup grp_nci_nfc
 * Length of Supporting data for RF_ACTION_NTF.
 */
#define PH_NCINFC_NFCEE_MAX_SUPPDATA_LEN                (255U)

/**
 * \ingroup grp_nci_nfc
 * Max number of NFCEE devices supported by the SW Stack
 */
#define PH_NCINFC_NFCEE_DEVICE_MAX                      (3u)

/*
 ################################################################################
 ******************** Enumeration and Structure Definition **********************
 ################################################################################
*/

/**
 * \ingroup grp_nci_nfc
 * \brief CORE_RESET_NTF Reset Trigger, ref NCI2.0 section 4.1
 */
typedef enum phNciNfc_ResetTrigger
{
    PH_NCINFC_RESETTRIGGER_NFCC_INTERNAL_ERROR = 0x00,  /**< Unrecoverable error occurred within the NFCC */
    PH_NCINFC_RESETTRIGGER_NFCC_POWER_ON = 0x01,        /**< NFCC was powered on */
    PH_NCINFC_RESETTRIGGER_CMD_RESET_RECEIVED = 0x02    /**< CORE_RESET_CMD was received */
} phNciNfc_ResetTrigger_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Reset Types
 */
typedef enum phNciNfc_ResetType
{
    phNciNfc_ResetType_KeepConfig = 0x00,   /**< Keep configuration data */
    phNciNfc_ResetType_ResetConfig = 0x01   /**< Reset the configuration */
} phNciNfc_ResetType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Nci reset types
 */
typedef enum phNciNfc_NciReset
{
    phNciNfc_NciReset_DeInit_KeepConfig = 0x00,   /**< Sends reset command to NFCC and releases Nci Handle */
    phNciNfc_NciReset_DeInit_ResetConfig = 0x01,  /**< Sends reset command to NFCC and releases Nci Handle */
    phNciNfc_NciReset_Mgt_Reset = 0x02,           /**< Releases Nci handle without sending reset command to NFCC */
    phNciNfc_NciReset_ClearNci = 0x03,            /**< Resets the content of Nci Context strucutre (for internal use only)*/
    phNciNfc_NciReset_ResetNfcc_KeepConfig = 0x04,    /**< Sends a reset command to NFCC (for internal use only) */
    phNciNfc_NciReset_ResetNfcc_ResetConfig = 0x05/**< Sends a reset command to NFCC (for internal use only) */
} phNciNfc_NciReset_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Type of notification registration to select.
 */
typedef enum phNciNfc_RegisterType
{
    phNciNfc_e_RegisterDefault = 0x00,      /**<For All other notifications of remote devices */
    phNciNfc_e_RegisterTagDiscovery = 0x01, /**<For Tag Discovery notification*/
    phNciNfc_e_RegisterSecureElement = 0x02,/**<For Secure Element notification*/
    phNciNfc_e_RegisterRfDeActivate = 0x03, /**<For Rf Deactivated notification*/
    phNciNfc_e_RegisterGenericError = 0x04, /**<For Generic error notification*/
    phNciNfc_e_RegisterReset        = 0x05  /**<For Reset notification*/
} phNciNfc_RegisterType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Type of notification received by NCI layer
 */
typedef enum phNciNfc_NotificationType
{
    eNciNfc_DiscoverNtf,        /**< Tag Discovered Notification */
    eNciNfc_NfceeDiscoverNtf,   /**< Nfcee Discovered Notification */
    eNciNfc_NfceeStatusNtf,     /**< Nfcee Status Notification */
    eNciNfc_NfceeDiscReqNtf,    /**< Nfcee Discovery Request notification */
    eNciNfc_NfceeActionNtf,     /**< Nfcee Action Notification */
    eNciNfc_NciResetNtf,        /**< Nci Reset Notification */
    eNciNfc_NciActivateNfceeNtf,/**< Activation notification for NFCEE*/
    eNciNfc_NciRfFieldInfoNtf,  /**< Rf field info notification*/
    eNciNfc_NciRfDeActvNtf,     /**< Rf Deactivate notification*/
    eNciNfc_NciInvalidEvt       /**< Invalid*/
} phNciNfc_NotificationType_t;  /** Type of Notification received by Nci Module */

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains supported RF Technologies
 */
typedef enum phNciNfc_RfTechnologies
{
    phNciNfc_e_RfTechnologiesNfc_A = 0x00,         /**<NFC A technology */
    phNciNfc_e_RfTechnologiesNfc_B = 0x01,         /**<NFC B technology */
    phNciNfc_e_RfTechnologiesNfc_F = 0x02,         /**<NFC F technology */
    phNciNfc_e_RfTechnologiesNfc_15693 = 0x03,      /**<ISO 15693 technology */
} phNciNfc_RfTechnologies_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains the type filed of 'TLV' Coding for Rf parameter update.
 */
 typedef enum phNciNfc_RfParamType
 {
     phNciNfc_e_RfParamRfTechAndMode = 0,       /**< Update Rf technolofy and mode */
     phNciNfc_e_RfParamTransmitBitrate = 1,     /**< Update Transmit bit rate */
     phNciNfc_e_RfParamReceiveBitrate = 2 ,      /**< Update Receive bit rate */
     phNciNfc_e_RfParamNfcBDataExchgConf = 3    /**< Update Nfc-B data exchange configuration */
}phNciNfc_RfParamType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains Rf techology and mode supported by NFCC
 */
 typedef enum phNciNfc_RfTechAndMode
 {
     phNciNfc_e_NfcAPassivePoll = 0x00,     /**< Nfc-A passive poll mode */
     phNciNfc_e_NfcBPassivePoll = 0x01,     /**< Nfc-B passive poll mode */
     phNciNfc_e_NfcFPassivePoll = 0x02,     /**< Nfc-F passive poll mode */
     phNciNfc_e_NfcAActivePoll = 0x03,      /**< Nfc-A active poll mode */
     phNciNfc_e_NfcFActivePoll = 0x05,      /**< Nfc-F active poll mode */
     phNciNfc_e_NfcAPassiveListen = 0x80,   /**< Nfc-A passive listen mode */
     phNciNfc_e_NfcBPassiveListen = 0x81,   /**< Nfc-B passive listen mode */
     phNciNfc_e_NfcFPassiveListen = 0x82,   /**< Nfc-F passive listen mode */
     phNciNfc_e_NfcAActiveListen = 0x83,    /**< Nfc-A active listen mode */
     phNciNfc_e_NfcFActiveListen = 0x85,    /**< Nfc-F active listen mode */
     phNciNfc_e_NfcAKovioPoll = 0x77        /**< Kovio-specific Nfc-A poll mode */
}phNciNfc_RfTechAndMode_t;

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE Mode Info contains Nfcee related modes identifier.
 */
typedef enum phNciNfc_NfceeModes
{
  /** NFCEE  enable identifier*/
   PH_NCINFC_EXT_NFCEEMODE_DISABLE = 0x00,
  /** NFCEE  disable identifier*/
   PH_NCINFC_EXT_NFCEEMODE_ENABLE = 0x01,
   /** NFCEE  removed identifier*/
   PH_NCINFC_NFCEE_REMOVED = 0x02,
   /** Future or unknown identifier */
   PH_NCINFC_NFCEEDISC_UNKNOWN
}phNciNfc_NfceeModes_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief NFCEE Information TLV types(NCI Spec Table 84)
 */
typedef enum phNciNfc_NfceeTlvTypes
{
    phNciNfc_HwRegIdType = 0x00, /**< Hardware registration ID*/
    phNciNfc_AtrBytesType = 0x01, /**< ATR Bytes for NFCEE*/
    phNciNfc_T3TCmdSetType = 0x02 /**< T3T Command set information*/
}phNciNfc_NfceeTlvTypes_t;

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE interface type
 */
typedef enum phNciNfc_NfceeIfType
{
   /** NFCEE APDU Data exchange Interface */
   phNciNfc_e_NfceeApduIf,
   /** NFCEE HCI Access Data exchange Interface */
   phNciNfc_e_NfceeHciAccessIf,
   /** NFCEE Type 3 tag Data exchange Interface */
   phNciNfc_e_NfceeT3tIf,
   /** NFCEE Transparent Data exchange Interface */
   phNciNfc_e_NfceeTransIf,
}phNciNfc_NfceeIfType_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief This contains RF Action occured on NFCEE in emulation.
 */
typedef enum phNciNfc_RfNfceeTriggers
{
  /** Identifier of Rf trigger for Application initiated trans*/
  PH_NCINFC_TRIG_APP_INITIATION = 0x10,
  /** Idenitifier of Rf trigger of ISO select of application in Nfcee*/
  PH_NCINFC_TRIG_ISO7816_SELECT = 0x00,
  /** Identifier of Rf protocol changed*/
  PH_NCINFC_TRIG_RFPROTOCOL_ROUTING = 0x01,
  /** Identifier of Rf technology changed*/
  PH_NCINFC_TRIG_RFTECHNOLOGY_ROUTING = 0x02,
  /** RFU or unknown event trigger*/
  PH_NCINFC_TRIG_UNKNOWN
}phNciNfc_RfNfceeTriggers_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief This contains information about generic error received
 */
typedef enum phNciNfc_GenericErrCode
{
    phNciNfc_e_ErrorNotDefined = 0x01,      /**< NCI error code not defined*/
    phNciNfc_e_Rejected = 0x01,             /**< NCI 'STATUS_REJECTED' received*/
    phNciNfc_e_RfFrameCorrupted = 0x02,     /**< NCI 'PH_NCINFC_STATUS_RF_FRAME_CORRUPTED' received*/
    phNciNfc_e_Failed = 0x03,               /**< NCI 'PH_NCINFC_STATUS_FAILED' received*/
    phNciNfc_e_NotInitiatlized = 0x04,      /**< NCI 'PH_NCINFC_STATUS_NOT_INITIALIZED' received*/
    phNciNfc_e_SyntaxErr = 0x05,            /**< NCI 'PH_NCINFC_STATUS_SYNTAX_ERROR' received*/
    phNciNfc_e_SemanticErr = 0x06,          /**< NCI 'PH_NCINFC_STATUS_SEMANTIC_ERROR' received*/
    phNciNfc_e_InvalidParam = 0x07,         /**< NCI 'PH_NCINFC_STATUS_INVALID_PARAM' received*/
    phNciNfc_e_MsgSizeExceeded = 0x08,      /**< NCI 'PH_NCINFC_STATUS_MESSAGE_SIZE_EXCEEDED' received*/
    phNciNfc_e_DiscAlreadyStarted = 0x09,   /**< NCI 'PH_NCINFC_STATUS_DISCOVERY_ALREADY_STARTED' received*/
    phNciNfc_e_DiscTgtActvnFailed = 0x0A,   /**< NCI 'PH_NCINFC_STATUS_DISCOVERY_TARGET_ACTIVATION_FAILED' received*/
    phNciNfc_e_DiscTearDown = 0x0B,         /**< NCI 'PH_NCINFC_STATUS_DISCOVERY_TEAR_DOWN' received*/
}phNciNfc_GenericErrCode_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition contains the type filed of 'TLV' Coding for Listen Mode Routing.
 */
 typedef enum phNciNfc_LstnModeRtngType
 {
     phNciNfc_e_LstnModeRtngTechBased = 0,    /**< Technology-based routing entry */
     phNciNfc_e_LstnModeRtngProtocolBased = 1,/**< Protocol-based routing entry */
     phNciNfc_e_LstnModeRtngAidBased = 2      /**< AID-based routing entry */
}phNciNfc_LstnModeRtngType_t;

/*!
 * \ingroup grp_nci_nfc
 * \brief This contains RF Field information.
 */
typedef enum phNciNfc_RfFieldInfo
{
    phNciNfc_e_RfFieldOff = 0x00,  /**< Rf Field Off*/
    phNciNfc_e_RfFieldOn = 0x01    /**< Rf Field ON*/
}phNciNfc_RfFieldInfo_t;

/**
 * \ingroup grp_nci_nfc
 * \brief RF_NFCEE_DISCOVERY_REQ_NTF information Params
 */
typedef enum phNciNfc_RfNfceeDiscReqType
{
    phNciNfc_e_RfNfceeDiscReqAdd = 0x00,
    phNciNfc_e_RfNfceeDiscReqRemove = 0x01,
}phNciNfc_RfNfceeDiscReqType_t;

/**
 * \ingroup grp_nci_nfc
 * \brief RF_NFCEE_DISCOVERY_REQ_NTF information Params
 */
typedef struct phNciNfc_NfceeDiscReqNtfParams
{
    phNciNfc_RfNfceeDiscReqType_t bType;
    uint8_t bNfceeId;
    phNciNfc_RfTechMode_t eTechMode;
    phNciNfc_RfProtocols_t eProtocol;
}phNciNfc_NfceeDiscReqNtfParams_t, *pphNciNfc_NfceeDiscReqNtfParams_t;

/**
 * \ingroup grp_nci_nfc
 * \brief RF_NFCEE_DISCOVERY_REQ_NTF information
 */
typedef struct phNciNfc_NfceeDiscReqNtfInfo
{
    uint8_t bCount;
    pphNciNfc_NfceeDiscReqNtfParams_t pParams;
}phNciNfc_NfceeDiscReqNtfInfo_t, *pphNciNfc_NfceeDiscReqNtfInfo_t;

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE additional info for T3T Entries.
 */
typedef struct phNciNfc_Nfcee_T3tParams
{
    uint8_t aSysCode[PH_NCINFC_NFCEE_T3T_MAX_SYSCODE_LEN];/**< System Code parameters*/
    uint8_t *pSysCode;/**< System Code parameters*/
    uint8_t aIdm[PH_NCINFC_NFCEE_T3T_MAX_IDM_LEN];/**< Idm Parameters*/
    uint8_t *pIdm;/**< Idm Parameters*/
}phNciNfc_Nfcee_T3tParams_t;/**<Nfcee Additional Info details */

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE T3T Command set Interface supplementary Information.
 */
typedef struct phNciNfc_NfceeT3tInfo
{
    uint8_t *pPmm;/**< PMM Info */
    uint8_t bNumOfEntries;/**< Number of System code & Idm Params entries */
    phNciNfc_Nfcee_T3tParams_t *pT3tParams;
}phNciNfc_NfceeT3tInfo_t;

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE additional info such as ATR, HW REG INFO or T3T Info
 */
typedef union phNciNfc_NfceeVal
{
    uint8_t *pHwId;/**< Hardware Identification info */
    uint8_t *pAtrBytes;/**< ATR bytes */
    phNciNfc_NfceeT3tInfo_t *pT3tCmdSetInfo;/**< T3T command set If Supplementary info */
}phNciNfc_NfceeVal_t;/**< Addition Nfcee Discovery Information */

/**
 * \ingroup grp_nci_nfc
 * \brief NFCEE Tlv info for different Discovery parameters.
 */
typedef struct phNciNfc_NfceeDiscTlvInfo
{
   /**type of nfcee credential viz ATR, T3T, Hardware info */
   phNciNfc_NfceeTlvTypes_t Type;
   /**length of additional info, for T3T*/
   uint8_t bInfoLength;
   /**additional info*/
   phNciNfc_NfceeVal_t tNfceeValue;

}phNciNfc_NfceeDiscTlvInfo_t, *pphNciNfc_NfceeDiscTlvInfo_t;/**< NFCEE Discovery Tlv params Information */

/**
 * \ingroup grp_nci_nfc
 * \brief Contains Nfcee related Info received after discovery.
 */
typedef struct phNciNfc_NfceeDeviceInfo
{
    /*This discovery id Applicable only if it is active*/
    uint8_t bRfDiscId;
    /** NFCEE ID allocated and discovered and required to be communicated */
    uint8_t bNfceeID;
    /** NFCEE status got from discovery ntf */
    phNciNfc_NfceeModes_t eNfceeStatus;
    /** NFCEE number of protocols supported */
    uint8_t bNumSuppProtocols;
    /** NFCEE all supported protocol by an nfcee id */
    uint8_t aSuppProtocols[PH_NCINFC_NFCEE_SUPPORTEDPROTO_MAX];
    /** Number of NFCEE TLV present into the information field*/
    uint8_t bNumTypeInfo;
    uint8_t *pTlvInfo;
    uint32_t TlvInfoLen;
    /** HW ID and ATR(looks will be present for almost all the NFCEEs) so kept outside
        of this discovery information Other are part of the */
    phNciNfc_NfceeDiscTlvInfo_t aAdditionalInfo[PH_NCINFC_NFCEE_INFO_TLV_MAX];
}phNciNfc_NfceeDevDiscInfo_t, *pphNciNfc_NfceeDevDiscInfo_t;/**< Nfcee Discovery Information */

typedef struct phNciNfc_NfceeDeviceHandle
{
    phNciNfc_NfceeDevDiscInfo_t tDevInfo;
}phNciNfc_NfceeDeviceHandle_t, *pphNciNfc_NfceeDeviceHandle_t;

typedef struct phNciNfc_NfceeList
{
    uint8_t bNfceeCount;
    pphNciNfc_NfceeDeviceHandle_t apNfceeList[PH_NCINFC_NFCEE_DEVICE_MAX];
}phNciNfc_NfceeList_t,*pphNciNfc_NfceeList_t;

typedef struct phNciNfc_NfceeInfo
{
    uint8_t bNfceeStatus;/**< TRUE = Add/Update Nfcee into the list, FALSE = Remove Nfcee from the list */
    uint8_t bNfceeId; /**< Nfcee Id */
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;
}phNciNfc_NfceeInfo_t,*pphNciNfc_NfceeInfo_t;

typedef enum phNciNfc_NfceeInitSequenceStatus
{
    phNciNfc_NfceeInitSequenceError = 0x00,   /**< Unrecoverable error */
    phNciNfc_NfceeInitSequenceStarted = 0x01,   /**< NFCEE Initialization sequence started */
    phNciNfc_NfceeInitSequenceCompleted = 0x02   /**< NFCEE Initialization sequence completed */
} phNciNfc_NfceeInitSequenceStatus_t; /**< NFCEE Status field in NFCEE_STATUS_NTF, Table 122, NCI2.0 */

typedef struct phNciNfc_NfceeStatusInfo
{
    phNciNfc_NfceeInitSequenceStatus_t bNfceeStatus;
    uint8_t bNfceeId;
}phNciNfc_NfceeStatusInfo_t, *pphNciNfc_NfceeStatusInfo_t; /**< NFCEE_STATUS_NTF, NCI2.0 */

/**
 * \ingroup grp_nci_nfc
 * \brief Nfcee action Notification Info received from lower layer.
 */
typedef struct phNfc_tNfceeActionInfo
{
    /** NFCEE ID for which Action Ntf is encountered */
    uint8_t bNfceeId;
    /** Trigger that caused this notification to be sent */
    phNciNfc_RfNfceeTriggers_t eTriggerType;
    /** Length of supporting data for the trigger */
    uint8_t bSupDataLen;
    /** Supporting data for the trigger */
    union phNciNfc_SupportData
    {
        uint8_t aSupData[PH_NCINFC_NFCEE_MAX_SUPPDATA_LEN];/**<AID of Application */
        phNciNfc_RfTechMode_t eRfTechMode;/**< RF protocol based routing decision */
        phNciNfc_RfProtocols_t eRfProtocol;/**< RF tech mode based routing decision */
    }phNciNfc_SupportData_t;/* Data can be AID or Rf protocol or Rf tech mode */
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;
}phNfc_tNfceeActionInfo_t,*pphNfc_tNfceeActionInfo_t;

/**
 * \ingroup grp_nci_nfc
 * \brief Configuration
 */
typedef struct phNciNfc_Config
{
    uint8_t bConfigOpt;
    uint8_t bLogDataMessages;
}phNciNfc_Config_t, *pphNciNfc_Config_t; /**< pointer to #phNciNfc_Config_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Poll mode Nfc-A discovery configuration parameters
 */
typedef struct phNciNfc_PollNfcADiscParams
{
    union
    {
        struct
        {
            BitField_t SetBailOut:1;/**< 1:Set 'bBailOut'; 0:Do not set 'bBailOut' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }PollNfcAConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bBailOut;           /**< If set to '1', bail out when NFC-A Technology has been detected
                                     during Poll Mode in Discovery activity */
}phNciNfc_PollNfcADiscParams_t, *pphNciNfc_PollNfcADiscParams_t; /**< pointer to #phNciNfc_PollNfcADiscParams_t */

/**
* \ingroup grp_nci_nfc
* \brief Kovio specific poll mode Nfc-A discovery configuration parameters
*/
typedef struct phNciNfc_PollNfcAKovioDiscParams
{
    union
    {
        struct
        {
            BitField_t SetBailOut:1;/**< 1:Set 'bBailOut'; 0:Do not set 'bBailOut' parameter. Most likely has no affect for Kovio */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }PollNfcAKovioConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bBailOut;           /**< If set to '1', bail out when NFC-A Technology has been detected
                                     during Poll Mode in Discovery activity */
}phNciNfc_PollNfcAKovioDiscParams_t, *pphNciNfc_PollNfcAKovioDiscParams_t; /**< pointer to #phNciNfc_PollNfcAKovioDiscParams_t */


/**
 * \ingroup grp_nci_nfc
 * \brief Poll mode Nfc-B discovery configuration parameters
 */
typedef struct phNciNfc_PollNfcBDiscParams
{
    union
    {
        struct
        {
            BitField_t SetAfi:1;    /**< 1:Set 'bAfi'; 0:Do not set 'bAfi' parameter */
            BitField_t SetBailOut:1; /**< 1:Set 'bBailOut'; 0:Do not set 'bBailOut' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }PollNfcBConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bAfi;                   /**< Application family identifier */
    uint8_t bBailOut;               /**< If set to '1', bail out when NFC-B Technology has been detected
                                         during Poll Mode in Discovery activity */
    uint8_t bAttriB_Param1;         /**< Param 1 of the ATTRIB command (Read-Only parameter) */
}phNciNfc_PollNfcBDiscParams_t, *pphNciNfc_PollNfcBDiscParams_t;  /**< pointer to #phNciNfc_PollNfcBDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Poll mode Nfc-F discovery configuration parameters
 */
typedef struct phNciNfc_PollNfcFDiscParams
{
    union
    {
        struct
        {
            BitField_t SetBitRate:1;/**< 1:Set 'bBitRate'; 0:Do not set 'bBitRate' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }PollNfcFConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bBitRate;               /**< Initial Bit rate (#phNciNfc_BitRates_t)*/
}phNciNfc_PollNfcFDiscParams_t, *pphNciNfc_PollNfcFDiscParams_t;/**< pointer to #phNciNfc_PollNfcFDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Poll mode ISO-DEP discovery configuration parameters
 */
typedef struct phNciNfc_PollIsoDepDiscParams
{
    union
    {
        struct
        {
            BitField_t SetHigherLayerInfo:1;/**< 1:Set 'aHigherLayerInfo'; 0:Do not set 'aHigherLayerInfo' parameter */
            BitField_t SetBitRate:1;/**< 1:Set 'bBitRate'; 0:Do not set 'bBitRate' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }PollIsoDepConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t aHigherLayerInfo[PH_NCINFC_MAX_HIGHER_LAYER_INF_LEN];/**< Higher layer INF field of the ATTRIB command */
    uint8_t bHigherLayerInfoSize;   /**< Size of 'HigherLayerInfo' (can be max 48 bytes) */
    uint8_t bBitRate;               /**< Maximum allowed bit rate, default: 0x00 (106 Kbit/s) (#phNciNfc_BitRates_t) */
}phNciNfc_PollIsoDepDiscParams_t, *pphNciNfc_PollIsoDepDiscParams_t;/**< pointer to #phNciNfc_PollIsoDepDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Poll mode NFC-DEP discovery configuration parameters
 */
typedef struct phNciNfc_PollNfcDepDiscParams
{
    union
    {
        struct
        {
            BitField_t bSetSpeed:1;    /**< 1:Set 'bNfcDepSpeed'; 0:Do not set 'bNfcDepSpeed' parameter */
            BitField_t bSetGenBytes:1; /**< 1:Set 'aAtrReqGenBytes'; 0:Do not set 'aAtrReqGenBytes' parameter */
            BitField_t bSetAtrConfig:1;/**< 1:Set 'AtrReqConfig'; 0:Do not set 'AtrReqConfig' parameter */
         }Config;                   /**< Structure holds list of paramters to be set */
         uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
     }PollNfcDepConfig;                 /**< This union shall give information on which parameter has to be set */
    uint8_t bNfcDepSpeed;           /**< 0 - Highest Available Bit Rates, 1 - Maintain the Bit Rates */
    uint8_t aAtrReqGenBytes[PH_NCINFC_MAX_ATR_REQ_GEN_BYTES_LEN];/**< General Bytes for ATR_REQ */
    uint8_t bAtrReqGeneBytesSize;   /**< Size of buffer pointed by 'pAtrReqGeneralBytes' */
    struct
    {
        BitField_t bDid:1;            /**< If set to 1, the DID MAY be used, otherwise DID shall not be used */
        BitField_t bLr:2;             /**< Value for LR (Length Reduction) */
    }AtrReqConfig;         /**< Configuration to be used in the Optional Parameters (PP) within ATR_REQ */
}phNciNfc_PollNfcDepDiscParams_t, *pphNciNfc_PollNfcDepDiscParams_t;/**< pointer to #phNciNfc_PollNfcDepDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode Nfc-A discovery configuration parameters
 */
typedef struct phNciNfc_LstnNfcADiscParams
{
    union
    {
        struct
        {
            BitField_t SetBitFrameSdd:1;   /**< 1:Set 'bBitFrameSDD'; 0:Do not set 'bBitFrameSDD' parameter */
            BitField_t SetPlatformConfig:1;/**< 1:Set 'bPlatformConfig'; 0:Do not set 'bPlatformConfig' parameter */
            BitField_t SetSelInfo:1;       /**< 1:Set 'SelInfo'; 0:Do not set 'SelInfo' parameter */
            BitField_t SetNfcID1:1;        /**< 1:Set 'NfcID1'; 0:Do not set 'NfcID1' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }LstnNfcAConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bBitFrameSDD;          /**< Bit Frame SDD (Single Device Detection) value to be sent in Byte 1 of SENS_RES */
    uint8_t bPlatformConfig;       /**< Platform Configuration value to be sent in Byte 2 of SENS_RES */
    struct
    {
        BitField_t bIsoDepProtoSupport:1;  /**< ISO-DEP Protocol is supported by the NFC Forum Device in Listen Mode */
        BitField_t bNfcDepProtoSupport:1;  /**< NFC-DEP Protocol is supported by the NFC Forum Device in Listen Mode */
    }SelInfo;                              /**< This value is used to generate SEL_RES.
                                                Bits set in this field shall be set in the SEL_RES sent by the NFCC */
    uint8_t aNfcID1[PH_NCINFC_MAX_NFCID1_LEN];  /**< NFCID1 (as defined in Digital Spec) */
    uint8_t bNfcID1Size;                        /**< Size of NFC ID1 (4, 7 or 10 bytes) */
}phNciNfc_LstnNfcADiscParams_t, *pphNciNfc_LstnNfcADiscParams_t;/**< pointer to #phNciNfc_LstnNfcADiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode Nfc-B discovery configuration parameters
 */
typedef struct phNciNfc_LstnNfcBDiscParams
{
    union
    {
        struct
        {
            BitField_t SetSensBInfo:1;  /**< 1:Set 'SensBInfo'; 0:Do not set 'SensBInfo' parameter */
            BitField_t SetNfcID0:1;     /**< 1:Set 'aNfcID0'; 0:Do not set 'aNfcID0' parameter */
            BitField_t SetAppData:1;    /**< 1:Set 'aAppData'; 0:Do not set 'aAppData' parameter */
            BitField_t SetSfgi:1;       /**< 1:Set 'bSfgi'; 0:Do not set 'bSfgi' parameter */
            BitField_t SetAdcFo:1;      /**< 1:Set 'AdcFo'; 0:Do not set 'AdcFo' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }LstnNfcBConfig;               /**< This union shall give information on which parameter has to be set */
    struct
    {
        BitField_t bIsoDepProtocolSupport:1;/**< ISO-DEP Protocol is supported by the NFC Forum Device in Listen Mode */
        BitField_t bProprietaryUse:2;  /**< For proprietary use */
    }SensBInfo;                     /**< This value is Used to generate Byte 2 of Protocol Info within SENSB_RES */
    uint8_t aNfcID0[PH_NCINFC_NFCID0_LEN];/**< NFCID0 (as defined in Digital Spec - 4 bytes) */
    uint8_t aAppData[PH_NCINFC_APP_DATA_LEN];/**< Application Data (Bytes 6-9) of SENSB_RES */
    uint8_t bSfgi;                  /**< Start-Up Frame Guard Time */
    struct
    {
        BitField_t bDid:1;            /**< If set to 1, the DID MAY be used, otherwise DID shall not be used */
        BitField_t bAdcCodingField:2; /**< b3 and b4 of ADC Coding field of SENSB_RES (Byte 12) */
    }AdcFo;                /**< Byte 3 of Protocol Info within SENSB_RES */
}phNciNfc_LstnNfcBDiscParams_t, *pphNciNfc_LstnNfcBDiscParams_t;/**< pointer to #phNciNfc_LstnNfcBDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode Nfc-F discovery configuration parameters
 */
typedef struct phNciNfc_LstnNfcFDiscParams
{
    union
    {
        struct
        {
            BitField_t SetConfigBitRate:1;  /**< 1:Set 'ConfigBitRate'; 0:Do not set 'ConfigBitRate' parameter */
            BitField_t SetProtocolType:1;   /**< 1:Set 'ProtocolType'; 0:Do not set 'ProtocolType' parameter */
            BitField_t SetT3tId:1;          /**< 1:Set 'aT3tId'; 0:Do not set 'aT3tId' parameter */
            BitField_t SetT3tPmm:1;         /**< 1:Set 'aT3tPmm'; 0:Do not set 'aT3tPmm' parameter */
            BitField_t SetT3tFlags:1;       /**< 1:Set 'wT3tFlags'; 0:Do not set 'wT3tFlags' parameter */
            BitField_t SetbT3tMax:1;
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }LstnNfcFConfig;               /**< This union shall give information on which parameter has to be set */
    struct
    {
        BitField_t bLstn212kbps:1;  /**< If set to 1, listen for 212 kbps */
        BitField_t bLstn424kbps:1;  /**< If set to 1, listen for 424 kbps */
    }ConfigBitRate;                 /**< Configures the bit rates to listen for */
    struct
    {
        BitField_t bNfcDepProtocolSupport:1;/**< NFC-DEP Protocol is supported by the NFC Forum Device in Listen Mode */
        BitField_t bProprietaryUse:2; /**< For proprietary use */
    }ProtocolType; /**< Configures the bit rates to listen for */
    uint8_t bT3tMax; /**< The maximum index of LF_T3T_IDENTIFIERS supported by the NFCC. This is
                           a read only parameter, DH SHALL NOT attempt to write this parameter*/
    uint8_t  aT3tId[PH_NCINFC_MAX_NUM_T3T_IDS][PH_NCINFC_T3TID_LEN];/**< Type 3 Tag identifiers. Each Identifier shall
                              be of 10 bytes. Octet 0 to Octet 1 indicate the System Code of a Type 3 Tag Emulation
                              occurring on the DH. Octet 2 to Octet 9 indicates NFCID2 for the Type 3 Tag Platform */
    uint8_t  aT3tPmm[PH_NCINFC_T3TPMM_LEN];/**< PAD0, PAD1, MRTI_check, MRTI_update and PAD2 of SENSF_RES (8 bytes) */
    uint16_t wT3tFlags;                   /**< A bit field indicating which T3T identifiers are enabled in the
                                               process to create responses to a SENSF_REQ (1-Ignored, 0-used) */
}phNciNfc_LstnNfcFDiscParams_t, *pphNciNfc_LstnNfcFDiscParams_t;/**< pointer to #phNciNfc_LstnNfcFDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode ISO-DEP discovery configuration parameters
 */
typedef struct phNciNfc_LstnIsoDepDiscParams
{
    union
    {
        struct
        {
            BitField_t SetFwt:1;   /**< 1:Set 'bFrameWaitingTime'; 0:Do not set 'bFrameWaitingTime' parameter */
            BitField_t SetLA_HistBytes:1;     /**< 1:Set 'aLA_HistBytes'; 0:Do not set 'aLA_HistBytes' parameter */
            BitField_t SetLB_HigherLayerResp:1;    /**< 1:Set 'aLB_HigherLayerResp'; 0:Do not set 'aLB_HigherLayerResp' parameter */
            BitField_t SetbBitRate:1;       /**< 1:Set 'bBitRate'; 0:Do not set 'bBitRate' parameter */
        }Config;                   /**< Structure holds list of paramters to be set */
        uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }LstnIsoDepConfig;               /**< This union shall give information on which parameter has to be set */
    uint8_t bFrameWaitingTime;        /**< Frame Waiting time Integer */
    uint8_t aLA_HistBytes[PH_NCINFC_MAX_HIST_BYTES_LEN];/**< Historical Bytes (only applicable for Type 4A Tag) */
    uint8_t bHistBytesSize;           /**< Size of buffer pointed by 'pLA_HistBytes' */
    uint8_t aLB_HigherLayerResp[PH_NCINFC_MAX_HIGHER_LAYER_RES_LEN];/**< Higher Layer - Response field of the ATTRIB response */
    uint8_t bHigherLayerRespInfoSize; /**< Size of buffer pointed by 'pLB_HigherLayerRespInfo' */
    uint8_t bBitRate;                 /**< Maximum supported bit rate. Default: 0x00 (106 Kbit/s) */
}phNciNfc_LstnIsoDepDiscParams_t, *pphNciNfc_LstnIsoDepDiscParams_t;/**< pointer to
                                                                         #phNciNfc_LstnIsoDepDiscParams_t*/

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode NFC-DEP discovery configuration parameters
 */
typedef struct phNciNfc_LstnNfcDepDiscParams
{
    union
    {
        struct
        {
            BitField_t bSetWT:1;    /**< 1:Set 'bNfcDepSpeed'; 0:Do not set 'bNfcDepSpeed' parameter */
            BitField_t bSetGenBytes:1; /**< 1:Set 'aAtrReqGenBytes'; 0:Do not set 'aAtrReqGenBytes' parameter */
            BitField_t bSetAtrRespConfig:1;/**< 1:Set 'AtrReqConfig'; 0:Do not set 'AtrReqConfig' parameter */
         }Config;                   /**< Structure holds list of paramters to be set */
         uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
     }LstnNfcDepConfig;                 /**< This union shall give information on which parameter has to be set */
    uint8_t bWaitingTime;           /**< Waiting Time */
    uint8_t aAtrResGenBytes[PH_NCINFC_MAX_ATR_RES_GEN_BYTES_LEN];/**< General Bytes in ATR_RES */
    uint8_t bAtrResGenBytesSize;    /**< Size of buffer pointed by 'pLA_HistBytes' */
    struct
    {
        BitField_t bLengthReduction:2; /**< Value for LR */
    }AtrRespConfig;                 /**< Used to generate the Optional parameters (PP) in ATR_RES */
}phNciNfc_LstnNfcDepDiscParams_t, *pphNciNfc_LstnNfcDepDiscParams_t;/**< pointer to
                                                                         #phNciNfc_LstnNfcDepDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Common parameters for Discovery configuration
 */
typedef struct phNciNfc_CommonDiscParams
{
    union
    {
        struct
        {
            BitField_t SetTotalDuration:1;    /**< 1:Set 'wTotalDuration'; 0:Do not set 'wTotalDuration' parameter */
            BitField_t SetConDevLimit:1; /**< 1:Set 'bConDevLimit'; 0:Do not set 'bConDevLimit' parameter */
            BitField_t SetRfFieldInfo:1;/**< 1:Set 'bRfFieldInfo'; 0:Do not set 'bRfFieldInfo' parameter */
            BitField_t SetRfNfceeAction:1;/**< 1:Set 'bRfNfceeAction'; 0:Do not set 'bRfNfceeAction' parameter */
            BitField_t SetNfcDepOperationParam:1;/**< 1:Set 'NfcDepOperationParam'; 0:Do not set 'NfcDepOperationParam' parameter */
         }Config;                   /**< Structure holds list of paramters to be set */
         uint32_t EnableConfig;     /**< Variable used to know whether ther are any parameters to be set */
    }ComnParamsConfig;                 /**< This union shall give information on which parameter has to be set */
    uint16_t wTotalDuration;        /**< Total Duration of single discovery period in msec (0x0000 - 0xFFFF) */
    uint8_t  bConDevLimit;          /**< Number of identifiers that can be resolved for collision resolution */

    uint8_t bRfFieldInfo;           /**< If set to '1', NFCC is allowed to send RF Field Info Notif's to the DH.
                                         If set to '0', NFCC is not allowed to send RF Field Info Notif's to the DH */
    uint8_t bRfNfceeAction;         /**< If set to '1', NFCC SHALL send RF NFCEE Action notificaiotn to the
                                         DH upon the triggers.
                                         If set to '0', NFCC SHALL NOT send RF NFCEE Action notifications to the DH */
    struct
    {
        BitField_t bRtoxReq:1;         /**< NFC-DEP Target SHALL NOT send RTOX (Response Timeout Extension) requests */
        BitField_t bAttentionCommand:1;/**< If set to 1, the NFC-DEP Initiator SHALL use the ATTENTION command only
                                         as part of the error recovery procedure */
        BitField_t bInformationPdu:1;  /**< If set to 1, Information PDU with no transport data bytes SHALL NOT be sent */
        BitField_t bUseMaxTxLen:1;     /**< If set to 1, all PDUs indicating chaining SHALL use the maximum number of transport data bytes */
    }NfcDepOperationParam;  /**< NFC-DEP Operation Parameter */
}phNciNfc_CommonDiscParams_t, *pphNciNfc_CommonDiscParams_t;/**< pointer to #phNciNfc_CommonDiscParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Supported power states
 */
typedef struct phNciNfc_PowerState
{
    BitField_t bSwitchedOn:1;          /**< Switched ON state */
    BitField_t bSwitchedOff:1;         /**< Switched Off state */
    BitField_t bBatteryOff:1;          /**< Battery Off state */
    BitField_t bSwitchedOnSub1:1;      /**< Switched ON sub-state 1 */
    BitField_t bSwitchedOnSub2:1;      /**< Switched ON sub-state 2 */
    BitField_t bSwitchedOnSub3:1;      /**< Switched ON sub-state 3 */
}phNciNfc_PowerState_t, *pphNciNfc_PowerState_t;/**< pointer to #phNciNfc_PowerState_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Technology based listen mode routing
 */
typedef struct phNciNfc_TechBasedRtngValue
{
    uint8_t bRoute;                             /**< An Nfcee Id(Nfcee id: 1-254) or Dh-Nfcee Id(Nfcee id: 0) */
    phNciNfc_PowerState_t tPowerState;          /**< Power state */
    phNciNfc_RfTechnologies_t tRfTechnology;    /**< A valid RF Technology */
}phNciNfc_TechnBasedRtngValue_t, *pphNciNfc_TechnBasedRtngValue_t;/**< pointer to
                                                                             #phNciNfc_TechnBasedRtngValue_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Protocol based listen mode routing
 */
typedef struct phNciNfc_ProtoBasedRtngValue
{
    uint8_t bRoute;                          /**< An Nfcee Id(Nfcee id: 1-15) or Dh-Nfcee Id(Nfcee id: 0) */
    phNciNfc_PowerState_t tPowerState;       /**< Power state */
    phNciNfc_RfProtocols_t tRfProtocol;      /**< A valid RF Protocol */
}phNciNfc_ProtoBasedRtngValue_t, *pphNciNfc_ProtoBasedRtngValue_t;/**< pointer to
                                                                             #phNciNfc_ProtoBasedRtngValue_t */

/**
 * \ingroup grp_nci_nfc
 * \brief AID (Application Identifier) based listen mode routing
 */
typedef struct phNciNfc_AidBasedRtngValue
{
    uint8_t bRoute;                          /**< An Nfcee Id(Nfcee id: 1-15) or Dh-Nfcee Id(Nfcee id: 0) */
    phNciNfc_PowerState_t tPowerState;       /**< Power state */
    uint8_t aAid[PH_NCINFC_MAX_AID_LEN];      /**< A buffer containing AID (5-16 bytes) */
    uint8_t bAidSize;                        /**< Size of AID stored in 'aAid' */
}phNciNfc_AidBasedRtngValue_t, *pphNciNfc_AidBasedRtngValue_t;/**< pointer to #phNciNfc_AidBasedRtngValue_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Listen mode routing entry
 */
typedef struct phNciNfc_RtngConfig
{
    phNciNfc_LstnModeRtngType_t Type;   /**< The type filed of 'TLV' coding for Listen Mode Routing */
    union
    {
        phNciNfc_TechnBasedRtngValue_t tTechBasedRtngValue;  /**< Technology based routing value */
        phNciNfc_ProtoBasedRtngValue_t tProtoBasedRtngValue; /**< Protocol based routing value */
        phNciNfc_AidBasedRtngValue_t   tAidBasedRtngValue;   /**< Aid based routing value */
    }LstnModeRtngValue;                                           /**< Value filed of Listen mode routing entry */
}phNciNfc_RtngConfig_t, *pphNciNfc_RtngConfig_t;/**< pointer to #phNciNfc_RtngConfig_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Rf protocol to Rf interface mapping entry
 */
typedef struct phNciNfc_MapMode
{
    BitField_t bPollMode:1;   /**< RF Interface is mapped to the RF Protocol in Poll Mode */
    BitField_t bLstnMode:1;   /**< RF Interface is mapped to the RF Protocol in Listen Mode */
}phNciNfc_MapMode_t, *pphNciNfc_MapMode_t; /**< pointer to #phNciNfc_MapMode_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Rf protocol to Rf interface mapping entry
 */
typedef struct phNciNfc_MappingConfig
{
    phNciNfc_RfProtocols_t tRfProtocol;         /**< A valid RF Protocol */
    phNciNfc_MapMode_t Mode;                    /**< Rf protocol to Rf interface mapping mode */
    phNciNfc_RfInterfaces_t tRfInterface;       /**< A valid RF Interface */
}phNciNfc_MappingConfig_t, *pphNciNfc_MappingConfig_t;/**< pointer to #phNciNfc_MappingConfig_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Rf Discovery configuration parameters to be set/get.
 */
typedef struct phNciNfc_RfConfigInfo
{
    BitField_t PollNfcAConfig:1;      /**< Poll Nfc-A Discovery configuration parameters */
    BitField_t PollNfcAKovioConfig:1; /**< Poll Nfc-A Kovio Discovery configuration parameters */
    BitField_t PollNfcBConfig:1;      /**< Poll Nfc-B Discovery configuration parameters */
    BitField_t PollNfcFConfig:1;      /**< Poll Nfc-F Discovery configuration parameters */
    BitField_t PollIsoDepConfig:1;    /**< Poll Iso-Dep Discovery configuration parameters */
    BitField_t PollNfcDepConfig:1;    /**< Poll Nfc-Dep Discovery configuration parameters */
    BitField_t LstnNfcAConfig:1;      /**< Listen Nfc-A Discovery configuration parameters */
    BitField_t LstnNfcBConfig:1;      /**< Listen Nfc-B Discovery configuration parameters */
    BitField_t LstnNfcFConfig:1;      /**< Listen Nfc-F Discovery configuration parameters */
    BitField_t LstnIsoDepConfig:1;    /**< Listen Iso-Dep Discovery configuration parameters */
    BitField_t LstnNfcDepConfig:1;    /**< Listen Nfc-Dep Discovery configuration parameters */
    BitField_t CommonConfig:1;        /**< Common Discovery configuration parameters */
}phNciNfc_RfConfigInfo_t;             /**< Enable/Disable config */

/**
 * \ingroup grp_nci_nfc
 * \brief Rf discovery configuration parameters (both poll and listen modes)
 */
typedef struct phNciNfc_RfDiscConfigParams
{
    phNciNfc_RfConfigInfo_t         tConfigInfo;          /**< Instance of #phNciNfc_RfConfigInfo_t */
    phNciNfc_PollNfcADiscParams_t   tPollNfcADiscParams;  /**< pointer to #phNciNfc_PollNfcADiscParams_t */
    phNciNfc_PollNfcBDiscParams_t   tPollNfcBDiscParams;  /**< pointer to #phNciNfc_PollNfcBDiscParams_t */
    phNciNfc_PollNfcFDiscParams_t   tPollNfcFDiscParams;  /**< pointer to #phNciNfc_PollNfcFDiscParams_t */
    phNciNfc_PollIsoDepDiscParams_t tPollIsoDepDiscParams;/**< pointer to #phNciNfc_PollIsoDepDiscParams_t */
    phNciNfc_PollNfcDepDiscParams_t tPollNfcDepDiscParams;/**< pointer to #phNciNfc_PollNfcDepDiscParams_t */
    phNciNfc_LstnNfcADiscParams_t   tLstnNfcADiscParams;  /**< Lstn mode Nfc-A discovery configuration parameters */
    phNciNfc_LstnNfcBDiscParams_t   tLstnNfcBDiscParams;  /**< Lstn mode Nfc-B discovery configuration parameters */
    phNciNfc_LstnNfcFDiscParams_t   tLstnNfcFDiscParams;  /**< Lstn mode Nfc-F discovery configuration parameters */
    phNciNfc_LstnIsoDepDiscParams_t tLstnIsoDepDiscParams;/**< Lstn mode ISO-DEP discovery configuration parameters */
    phNciNfc_LstnNfcDepDiscParams_t tLstnNfcDepDiscParams;/**< Lstn mode NFC-DEP discovery configuration parameters */
    phNciNfc_CommonDiscParams_t     tCommonDiscParams;    /**< pointer to #phNciNfc_CommonDiscParams_t */
}phNciNfc_RfDiscConfigParams_t,*pphNciNfc_RfDiscConfigParams_t; /**< pointer to #phNciNfc_RfDiscConfigParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Contains Type of devices to be discovered.
 * This structure needs to passed as part of Discover API. This is used to send RF_DISCOVER_CMD
 */
typedef struct phNciNfc_ADD_Cfg
{
    BitField_t EnableIso14443A : 1; /**< Flag to enable Reader A discovery */
    uint8_t    bIso14443A_PollFreq; /**< Discover freq for TYPE-A technology and mode */
    BitField_t EnableIso14443B : 1; /**< Flag to enable Reader A discovery */
    uint8_t    bIso14443B_PollFreq; /**< Discover freq for type-B technology and mode */
    BitField_t EnableFelica : 1;    /**< Flag to enableReader F discovery */
    uint8_t    bFelica_PollFreq;    /**< Discover freq for type-F technology and mode */
    BitField_t EnableEpcGen : 1;    /**< Flag to enable Reader F discovery */
    uint8_t    bEpcGen_PollFreq;    /**< Discover freq for type-F technology and mode */
    BitField_t EnableIso15693 : 1;  /**< Flag to enable Reader I discovery */
    uint8_t    bIso15693_PollFreq;  /**< Discover freq for type-I technology and mode */
    BitField_t EnableKovio : 1;     /**< Flag to enable Kovio Reader A discovery */
    uint8_t    bKovio_PollFreq;     /**< Discover freq for Kovio type-A technology and mode */
    BitField_t ListenNfcA : 1;      /**< Flag to enable Listen Mode for Nfc-A Technology */
    BitField_t ListenNfcB : 1;      /**< Flag to enable Listen Mode for Nfc-B Technology */
    BitField_t ListenNfcF : 1;      /**< Flag to enable Listen Mode for Nfc-F Technology */
    BitField_t ListenNfcAActive : 1;      /**< Flag to enable Active  Listen Mode for Nfc-A Technology */
    BitField_t ListenNfcFActive : 1;      /**< Flag to enable  Active Listen Mode for Nfc-F Technology */
    BitField_t PollNfcAActive : 1;      /**< Flag to enable  Active Poll Mode for Nfc-A Technology */
    BitField_t PollNfcFActive : 1;      /**< Flag to enable  Active Poll Mode for Nfc-F Technology */
}phNciNfc_ADD_Cfg_t,*pphNciNfc_ADD_Cfg_t; /**< pointer to #phNciNfc_ADD_Cfg_t */

/** This structure contains numbers of devices discovered and pointer to
    Remote device info structures.*/
typedef struct phNciNfc_DeviceInfo
{
    uint32_t dwNumberOfDevices;/**< Number of Devices Discovered */
    /** Pointer to Remote device info list*/
    phNciNfc_RemoteDevInformation_t *pRemDevList[PH_NCINFC_MAX_REMOTE_DEVICES];
}phNciNfc_DeviceInfo_t,*pphNciNfc_DeviceInfo_t;/**< Pointer to Structure #phNciNfc_DeviceInfo_t*/

/**
 * \ingroup grp_nci_nfc
 * \brief This structure contains Details of RF field info whether it is ON/OFF
 */
typedef struct phNciNfc_GenericErrInfo
{
    phNciNfc_GenericErrCode_t eGenericErrInfo;/**< Information about generic error*/
}phNciNfc_GenericErrInfo_t,*pphNciNfc_GenericErrInfo;/**< Pointer to structure #phNciNfc_GenericErrorInfo_t*/

/**
 * \ingroup grp_nci_nfc
 * \brief This structure contains Details of RF field info whether it is ON/OFF
 */
typedef struct phNciNfc_RfFieldNtfInfo
{
    phNciNfc_RfFieldInfo_t eRfFieldInfo;/**< RF Field Info whether ON/OFF*/
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;/**< Handle of Nfcee Device */
}phNciNfc_RfFieldNtfInfo_t,*pphNciNfc_RfFieldNtfInfo_t;/**< Pointer to structure #phNciNfc_RfFieldInfo_t*/

/**
 * \ingroup grp_nci_nfc
 * \brief This structure contains Details of RF Deactivation type info
 */
typedef struct phNciNfc_RfDeactvNtfInfo
{
    phNciNfc_DeActivateType_t eRfDeactvType;       /**< RF Deactivation type info */
    phNciNfc_DeActivateReason_t eRfDeactvReason;   /**< RF Deactivation reason */
}phNciNfc_RfDeactvNtfInfo_t,*pphNciNfc_RfDeactvNtfInfo_t;/**< Pointer to structure #phNciNfc_RfDeactvNtfInfo_t*/

/**
 * \ingroup grp_nci_nfc
 * \brief This structure contains Details of RF Deactivation type info
 */
typedef struct phNciNfc_NfceeHciEventInfo
{
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;/**< Handle of Nfcee Device */
    uint8_t *pTransBuff;
    uint16_t wLength;
}phNciNfc_NfceeHciEventInfo_t,*pphNciNfc_NfceeHciEventInfo_t;/**< Pointer to structure #phNciNfc_RfDeactvNtfInfo_t*/

/**
 * \ingroup grp_nci_nfc
 *
 *  This is a union returned as part of the \ref phNciNfc_NotificationInfo_t
 *  callback. It contains one of the following info
 *  1. Remote device Discovered
 *  2. Nfcee Discovery Request Notification
 *  3. Nfcee Action Notification
 *  These notification shall be sent if client has registered using the
 *  \ref phNciNfc_RegisterNotification.
 */
typedef union phNciNfc_NotificationInfo
{
    pphNciNfc_DeviceInfo_t pDiscoveryInfo;/**< Remote Devices Information*/
    phNciNfc_NfceeDiscReqNtfInfo_t tNfceeDiscReqInfo;/**< Nfcee Requested Discovery Info */
    pphNfc_tNfceeActionInfo_t pActionInfo;      /**< Action taken on Nfcee */
    pphNciNfc_NfceeDeviceHandle_t pNfceeHandle;     /**< This handle is shared if NTF is NFCEE device specific */
    phNciNfc_RfFieldNtfInfo_t tRfFieldInfo;       /**< RF Field Information */
    phNciNfc_RfDeactvNtfInfo_t  tRfDeactvInfo;      /**< RF Deactivate info */
    phNciNfc_NfceeHciEventInfo_t  tEventInfo;      /**< Hci events info */
    phNciNfc_GenericErrInfo_t tGenericErrInfo;     /**< Generic error information */
    phNciNfc_NfceeInfo_t tNfceeInfo;        /**< Nfcee notification */
    phNciNfc_NfceeStatusInfo_t tNfceeStatusInfo; /**< Nfcee status info (NCI2.0 specific) */
}phNciNfc_NotificationInfo_t,*pphNciNfc_NotificationInfo_t; /**< Notification info received by Nci module */

/**
 * \ingroup grp_nci_nfc
 * \brief After reading listen mode routing table configuraiton from NFCC, this structure shall be
 * updated for the upper layer.
 */
typedef struct phNciNfc_GetRtngConfig
{
    uint8_t bNumRtngConfigs;                    /**< Number of routing entries */
    pphNciNfc_RtngConfig_t pRtngConfig;         /**< Array of routing entries */
}phNciNfc_GetRtngConfig_t,*pphNciNfc_GetRtngConfig_t; /**< Pointer to #phNciNfc_GetRtngConfig_t structure */

/**
 * \ingroup grp_nci_nfc
 * \brief Nfc B data exchange configuration parameters
 */
typedef struct phNciNfc_NfcBDataExchgConf
{
    BitField_t MinTR0 : 2;              /**< Minimum TR0 */
    BitField_t MinTR1 : 2;              /**< Minimum TR1 */
    BitField_t SupressEoS : 1;          /**< Supression of EoS */
    BitField_t SupressSoS : 1;          /**< Supression of SoS */
    BitField_t MinTR2 : 2;              /**< Minimum TR2 */
}phNciNfc_NfcBDataExchgConf_t, *pphNciNfc_NfcBDataExchgConf_t; /**< pointer to #phNciNfc_NfcBDataExchgConf_t structure */

/**
 * \ingroup grp_nci_nfc
 * \brief This structure holds Rf parameter update information
 */
typedef struct phNciNfc_RfParamUpdate
{
    phNciNfc_RfParamType_t Type;    /**< The type filed of 'TLV' coding for Rf parameter to be updated */
    union
    {
        phNciNfc_RfTechAndMode_t eTechAndMode; /**< Technology and mode */
        phNciNfc_BitRates_t eBitRate;          /**< Transmit or Receive bit rate */
        phNciNfc_NfcBDataExchgConf_t tNfcBDataExchgConf;/**< Structure holding bit fields of Nfc-B Data
                                                             exchange configuration parameter */
    }RfParamValue;                                           /**< Value filed of Rf parameter update */
}phNciNfc_RfParamUpdate_t, *pphNciNfc_RfParamUpdate_t;/**< pointer to #phNciNfc_RfParamUpdate_t */

/**
 * \ingroup grp_nci_nfc
 * \brief This structure information of NFCC supported features
 */
typedef struct phNciNfc_NfccFeatures
{
    uint8_t NciVer;                         /**< NCI Version supported by NFCC */
    struct
    {
        BitField_t DiscConfigMode:2;        /**<0x00: DH is the only entity that configures the NFCC;
                                                0x01: NFCC can receive configurations from the DH and other NFCEEs*/
        BitField_t DiscFreqConfig:1;        /**<If set to 0b the Discovery Frequency value is ignored and the
                                                value of 0x01 SHALL be used by the NFCC*/
    }DiscConfigInfo;                        /**<Discovery configuration information*/
    struct
    {
        BitField_t AidBasedRouting:1;       /**<Supported if the bit is set to 1b */
        BitField_t ProtocolBasedRouting:1;  /**<Supported if bit is set to 1b */
        BitField_t TechnBasedRouting:1;     /**<Supported if bit is set to 1b */
    }RoutingInfo;                           /**<Types of routing suported by NFCC*/
    uint16_t RoutingTableSize;              /**< Maximum Routing table size*/
    struct
    {
        BitField_t SwitchOffState:1;        /**<Supported if the bit is set to 1b*/
        BitField_t BatteryOffState:1;       /**<Supported if the bit is set to 1b*/
    }PowerStateInfo;                        /**<Power states information*/
    uint8_t ManufacturerId;
    struct
    {
        uint8_t Length;
        uint8_t *Buffer;                    /**<Manufacturer information NCI*/
    }ManufactureInfo;
}phNciNfc_NfccFeatures_t, *pphNciNfc_NfccFeatures_t;/**< pointer to #phNciNfc_NfccFeatures_t */

/*!
 * \ingroup grp_nci_nfc
 * \brief Type 1 tag command list supported by NCI stack
 */
typedef enum phNciNfc_T1TCmdList
{
    phNciNfc_eT1TRaw    = 0x00,   /**< Performs Raw communication over T1T Tag*/
    phNciNfc_eT1TWriteN = 0x01,   /**< Write Multiple blocks to T1T tag*/
    phNciNfc_eT1TInvalidCmd /**< Invalid Command*/
}phNciNfc_T1TCmdList_t;    /**< Type1 Tag specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief Type 2 tag command list supported by NCI stack
 */
typedef enum phNciNfc_T2TCmdList
{
    phNciNfc_eT2TRaw    = 0x00,   /**< Performs Raw communication over T2T Tag*/
    phNciNfc_eT2TWriteN,   /**< Write Multiple blocks to T2T tag*/
    phNciNfc_eT2TreadN,   /**< Read Multiple blocks to T2T tag*/
    phNciNfc_eT2TSectorSel,   /**< Sector Select for MifareStd Cards*/
    phNciNfc_eT2TAuth,   /**< Sector Select for MifareStd Cards*/
    phNciNfc_eT2TProxCheck,/**< Proxy Check command for MF+*/
    phNciNfc_eT2TInvalidCmd /**< Invalid Command*/
}phNciNfc_T2TCmdList_t;    /**< Type2 Tag and Mifare specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief Type 3 tag command list supported by NCI stack
 */
typedef enum phNciNfc_T3TCmdList
{
    phNciNfc_eT3TRaw    = 0x00,   /**< Performs Raw communication over T3T Tag*/
    phNciNfc_eT3TInvalidCmd /**< Invalid Command*/
}phNciNfc_T3TCmdList_t;    /**< Type3 Tag specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief Type 4 tag command list supported by NCI stack
 */
typedef enum phNciNfc_T4TCmdList
{
    phNciNfc_eT4TRaw    = 0x00,   /**< Performs Raw communication over T4T Tag*/
    phNciNfc_eT4TInvalidCmd /**< Invalid Command*/
}phNciNfc_T4TCmdList_t;    /**< Type3 Tag specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief EpcGen tag command list supported by NCI stack
 */
typedef enum phNciNfc_EpcGenCmdList
{
    phNciNfc_eEpcGenRaw    = 0x00,  /**< Performs Raw communication over EpcGen Tag*/
    phNciNfc_eEpcGenInvalidCmd      /**< Invalid Command*/
}phNciNfc_EpcGenCmdList_t;    /**< Type3 Tag specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief ISO15693 tag command list supported by NCI stack
 */
typedef enum phNciNfc_Iso15693CmdList
{
    phNciNfc_eIso15693Raw    = 0x00,  /**< Performs Raw communication over ISO15693 Tag*/
    phNciNfc_eIso15693InvalidCmd      /**< Invalid Command*/
}phNciNfc_Iso15693CmdList_t;    /**< Type3 Tag specicific command list*/

/*!
 * \ingroup grp_nci_nfc
 * \brief All command list for tag operation supported by NCI stack
 */
typedef union phNciNfc_TagCmdList
{
    phNciNfc_T1TCmdList_t T1TCmd; /**< T1T Specific command*/
    phNciNfc_T2TCmdList_t T2TCmd; /**< T2T Specific command*/
    phNciNfc_T3TCmdList_t T3TCmd; /**< T3T Specific command*/
    phNciNfc_T4TCmdList_t T4TCmd; /**< T4T Specific command*/
    phNciNfc_EpcGenCmdList_t EpcGenCmd; /**< EpcGen Specific command*/
    phNciNfc_Iso15693CmdList_t Iso15693Cmd; /**< ISO15693 Specific command*/
}phNciNfc_TagCmdList_t; /**< Tag specific command*/

/*!
 * \ingroup grp_nci_nfc
 * \brief Transceive info
 */
typedef struct phNciNfc_TransceiveInfo
{
    phNciNfc_TagCmdList_t   uCmd;     /**< Technology Specific commands*/
    uint8_t                 bAddr;    /**< Start address to perform operation,Valid for T1T T2T T3T and some Propriatery tags*/
    uint8_t                 bNumBlock;/**< Number of blocks */
    uint16_t                wTimeout; /**< Timeout value to be used during transceive */
    phNciNfc_Data_t         tSendData;/**< Buffer information for sending data*/
    phNciNfc_Data_t         tRecvData;/**< Buffer information for receiving data*/
}phNciNfc_TransceiveInfo_t, *pphNciNfc_TransceiveInfo_t;/**< pointer to struct #phNciNfc_TransceiveInfo_t*/

/*!
 * \ingroup grp_nci_nfc
 * \brief Struct holds SENSF_REQ params info as in [DIGITAL]
 */
typedef struct phNciNfc_SensFReqParams
{
    uint8_t   bSysCode[2];     /**< System Code - Info of NFC device being polled for */
    uint8_t   bReqCode;        /**< Request Code - additional info in the SENSF_RES reponse */
    uint8_t   bTimeSlotNum;    /**< Time Slot Number - Used for collision resolutions */
} phNciNfc_SensFReqParams_t,*pphNciNfc_SensFReqParams_t;/**< pointer to #phNciNfc_SensFReqParams_t */

/**
 * \ingroup grp_nci_nfc
 * \brief Enum definition of the route selection power state
 */
 typedef enum phNciNfc_PowerSubState
 {
    phNciNfc_e_SwitchedOnState = 0x00,  /**< Switched On state */
    phNciNfc_e_SwitchedOnSubState1,     /**< Switched On sub-state 1 */
    phNciNfc_e_SwitchedOnSubState2,     /**< Switched On sub-state 2 */
    phNciNfc_e_SwitchedOnSubState3,     /**< Switched On sub-state 3 */
} phNciNfc_PowerSubState_t;

typedef void (*pphNciNfc_Notification_t) (
        void                         *pContext,
        phNciNfc_NotificationType_t eNtfType,
        pphNciNfc_NotificationInfo_t  pInfo,
        NFCSTATUS                    wStatus
        );

typedef void (*pphNciNfc_RegListNotifyCb_t)(
        void* pContext,
        NFCSTATUS status,
        pphNciNfc_DeviceInfo_t pInfo
        );

typedef void (*pphNciNfc_TransreceiveCallback_t)(
        void* pContext,
        NFCSTATUS status,
        pphNciNfc_Data_t pRecvData
        );

typedef void (*pphNciNfc_Receive_RspCb_t)(
        void* pContext,
        NFCSTATUS status,
        pphNciNfc_Data_t pRecvData
        );

typedef NFCSTATUS (*pphNciNfc_RegDataCb_t)(
        void* pContext,
        void* pInfo,
        NFCSTATUS wStatus
        );

/*
  ################################################################################
  **************** NCI Standard Function Prototype Declaration *******************
  ################################################################################
*/

typedef struct phNciNfc_Context* pphNciNfc_Context_t;

extern bool_t
phNciNfc_IsVersion1x(_In_ pphNciNfc_Context_t pNciContext);

extern bool_t
phNciNfc_IsVersion2x(_In_ pphNciNfc_Context_t pNciContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_Initialise function initialises the NCI context and all other
 * resources used in the NCI Layer for the corresponding interface link.
 * This function should be called before calling any other NCI APIs.
 * This initialises NCI SW stack and NFCC in default mode
 *
 * \param[in] pHwRef pHwRef is the Information of the Device Interface Link .
 * \param[in] pConfig pConfig is the Information required to configure the parameters
 * \param[in] pInitNotifyCb Upper layer Notification function pointer.
 * \param[in] pContext Upper layer Context
 * \param[in] eResetType Type of reset to be performed before Init(#phNciNfc_ResetType_t)
 *
 * \retval #NFCSTATUS_PENDING Initialisation of NCI Layer is in Progress.
 * \retval #NFCSTATUS_FAILED Initialisation of NCI Layer failed.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_ALREADY_INITIALISED Stack already initialized
 * \retval Other errors Errors related to the other layers
 */
extern NFCSTATUS
phNciNfc_Initialise(void*                        pHwRef,
                    phNciNfc_Config_t*           pConfig,
                    pphNciNfc_IfNotificationCb_t pInitNotifyCb,
                    void*                        pContext,
                    phNciNfc_ResetType_t         eResetType);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_ReInitialise function re-initialises the NCI context by sending RESET
 * and INIT commands to NFCC.
 *
 * \param[in] pNciHandle handle to the context of the NCI Layer.
 * \param[in] pReInitNotifyCb Upper layer Notification function pointer.
 * \param[in] pContext Upper layer Context
 *
 * \retval #NFCSTATUS_PENDING Initialisation of NCI Layer is in Progress.
 * \retval #NFCSTATUS_FAILED Initialisation of NCI Layer failed.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 */
extern NFCSTATUS
phNciNfc_ReInitialise(void* pNciHandle,
                      pphNciNfc_IfNotificationCb_t pReInitNotifyCb,
                      void*                        pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This is a Nfcee Discovery Enable or Nfcee Discovery Diable Api.
 *
 * \param[in]  pNciHandle      NCI context, NCI layer specific context.
 *
 * \retval NFCSTATUS_PENDING                If the command is yet to be processed.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_DEVICE         The device has not been opened or has
 *                                          been disconnected meanwhile
 */
extern NFCSTATUS
phNciNfc_Nfcee_StartDiscovery(void * pNciHandle,
                              pphNciNfc_IfNotificationCb_t  pNfceeDiscCb,
                              void *pContext);


/**
 * \ingroup grp_nci_nfc
 *
 * \brief This is a Nfcee communication Api, to establish a connection point with
 * a SE or NFCEE.(establihsing connection is prereqisite of Nfcee communication)
 *
 * \param[in]  pNciNfceeHandle      NCI NFCEE specific context, Module specific NCI Handle.
 * \param[in]  bProtocolType   NFCEE Protocol type to Connect the Interface of communication
 *
 *
 * \retval NFCSTATUS_PENDING                If the command is yet to be processed.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function
 *                                          is invalid.
 * \retval NFCSTATUS_INVALID_DEVICE         The device has not been opened or has
 *                                          been disconnected meanwhile
 */
extern NFCSTATUS
phNciNfc_Nfcee_Connect(void * pNciHandle,\
                       void * pNfceeHandle,\
                       pphNciNfc_IfNotificationCb_t pNfceeConnectCb,\
                       void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This is a Nfcee communication Api, to disconnect it to
 * a SE or NFCEE (established connection is prereqisite)
 *
 * \param[in]  pNfceeHandle        NCI NFCEE specific ccontext, Module specific NCI Handle.
 *
 * \retval NFCSTATUS_PENDING                If the command is yet to be processed.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_DEVICE         The device has not been opened or has
 *                                          been disconnected meanwhile
 */
extern NFCSTATUS
phNciNfc_Nfcee_Disconnect(void * pNciHandle,\
                          void * pNfceeHandle,
                          pphNciNfc_IfNotificationCb_t pNfceeDisconnectCb,\
                          void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This is a Nfcee Discovery Disable.
 *
 * \param[in]  pNciHandle        NCI layer speicific context.
 *
 * \retval NFCSTATUS_PENDING                If the command is yet to be processed.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_DEVICE         The device has not been opened or has
 *                                          been disconnected meanwhile
 */
extern NFCSTATUS
phNciNfc_Nfcee_StopDiscovery(void * pNciHandle,
                             pphNciNfc_IfNotificationCb_t pNfceeDiscCb,
                             void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This Synchronous API is used to Get the list of Nfcee Connected to the NFCC.
 *
 * \param[in]  pNciHandle        NCI layer speicific context.
 * \param[in]  bNfceeCount       Number of NFCEEs connected
 * \param[in]  pNfceeList        Pointer containing the Nfcee Handles.
 *
 * \retval NFCSTATUS_NOT_INITIALISED        Nci stack is not initialzed
 * \retval NFCSTATUS_SUCCESS                If retrieving list is successful
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 * \retval NFCSTATUS_FAILED                 If the Nfcee list is not proper.
 */
 NFCSTATUS phNciNfc_GetNfceeList(void *pNciHandle,
                                pphNciNfc_NfceeList_t pNfceeList);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This is a Nfcee mode set Api, to establish a connection point with
 * a SE or NFCEE.(establishing connection is prereqisite of Nfcee communication)
 *
 * \param[in]  pNciHandle        NCI layer specific context.
 * \param[in]  pNfceeHandle      Nfcee Handle for which Mode set is executed.
 * \param[in]  NfceeMode         NFCEE mode to enable disable the element
 * \param[in]  pNotifyCb         Upper layer call back function
 * \param[in]  pContext          Context of the Upper Layer.
 *
 * \retval NFCSTATUS_PENDING                If the command is yet to be processed.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_DEVICE         The device has not been opened or has
 *                                          been disconnected meanwhile
 */
extern NFCSTATUS
phNciNfc_Nfcee_ModeSet(void * pNciHandle,
                       void * pNfceeHandle,
                       phNciNfc_NfceeModes_t eNfceeMode,
                       pphNciNfc_IfNotificationCb_t pNotifyCb,
                       void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_Start_Discovery starts the discovery wheel from the begining.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] pPollConfig pointer to #phNciNfc_ADD_Cfg_t structure
 * \param[in] pDiscoveryCb Upper layer call back function
 * \param[in] pContext pContext is the context of the Upper Layer.
 *
 * \retval #NFCSTATUS_PENDING Discovery wheel start is in Progress.
 * \retval #NFCSTATUS_NOT_INITIALISED NCI layer not Initialized.
 * \retval #NFCSTATUS_BUSY Discovery already in Progress.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Request failed.
 */
extern NFCSTATUS
phNciNfc_StartDiscovery(void* pNciHandle,
                        phNciNfc_ADD_Cfg_t *pPollConfig,
                        pphNciNfc_IfNotificationCb_t pDiscoveryCb,
                        void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This API is a synchronous call used to register a listener for discover
 *  notifications received by lower layer.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] eRegisterType Type of Registration [Only default is supported for now].
 * \param[in] pRegNotifyCb Upper layer call back function to be invoked.
 * \param[in] pContext Context of the Upper Layer.
 *
 * \retval #NFCSTATUS_SUCCESS Notification registration successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not  be interpreted properly.
 * \retval #NFCSTATUS_NOT_INITIALISED Nci layer not initialized.
 */
extern NFCSTATUS
phNciNfc_RegisterNotification(
                        void                        *pNciHandle,
                        phNciNfc_RegisterType_t     eRegisterType,
                        pphNciNfc_Notification_t    pRegNotifyCb,
                        void                        *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This API is a synchronous call used to De-register a listener for discover
 *  notifications. De-Register causes lower layer not to invoke any upper layer
 *  callback function upon receiving notifications.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] eRegisterType Type of Registration [Only default is supported for now].
 *
 * \retval #NFCSTATUS_SUCCESS Notification De-Registration successful.
 * \retval #NFCSTATUS_NOT_INITIALISED Nci layer not initialized.
 */
extern NFCSTATUS
phNciNfc_DeregisterNotification(
                        void                        *pNciHandle,
                        phNciNfc_RegisterType_t     eRegisterType);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This function selects One of the Targets discovered by NFCC.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] pRemoteDevInfo Device information of the target to be selected.
 * \param[in] eRfInterface RF interface supported by the target.
 * \param[in] pDiscSelCb Callback function to be invoked after target selection.
 * \param[in] pContext Context of the upper module.
 *
 * \retval #NFCSTATUS_PENDING Selection of Target is success.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 */
extern NFCSTATUS
phNciNfc_Connect(void *pNciHandle,
                 pphNciNfc_RemoteDevInformation_t pRemoteDevInfo,
                 phNciNfc_RfInterfaces_t eRfInterface,
                 pphNciNfc_IfNotificationCb_t pDiscSelCb,
                 void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This function Stops discovery or Disconnects the active target.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] eDeActivateType De-Activation type
 * \param[in] pDeActivateCb Callback function to be invoked after deactivation.
 * \param[in] pContext Context of the upper module.
 *
 * \retval #NFCSTATUS_PENDING Deactivation of Target is success.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 */
extern NFCSTATUS
phNciNfc_Deactivate(void *pNciHandle,
                    phNciNfc_DeActivateType_t eDeActivateType,
                    pphNciNfc_IfNotificationCb_t pDeActivateCb,
                    void *pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This phNciNfc_Transreceive function allows to send data to and receive data
 *  from the Remote Device selected by the caller.It is also used by the
 *  NFCIP1 Initiator while performing a transaction with the NFCIP1 target.
 *  The caller has to provide the Device Context handle and command in order to communicate
 *  with the selected remote device.For P2P transactions the command type will not be used.
 *
 * \param[in] pNciHandle pNciHandle is the handle or the context of the NCI Layer.
 * \param[in] pDevicehandle Active device context handle passed during Target Selection.
 * \param[in] psTransceiveInfo Information required by transceive is
 *                             concealed in this structure. It contains
 *                             the send,receive buffers and their lengths.
 * \param[in] pTrcvCallback Callback function to return response or error.
 * \param[in] pContext Upper layer context to be returned in the callback.
 *
 * \retval #NFCSTATUS_PENDING Transceive initiated, Callback shall return Response or error.
 * \retval #NFCSTATUS_NOT_INITIALIZED NCI layer is not initialized.
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval NFCSTATUS_FAILED Data exchange request failed
 */
extern NFCSTATUS
phNciNfc_Transceive( void*                               pNciHandle,
                     void*                               pDevicehandle,
                     pphNciNfc_TransceiveInfo_t         psTransceiveInfo,
                     pphNciNfc_TransreceiveCallback_t    pTrcvCallback,
                     void*                               pContext);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall be invoked as part of P2P remote device receive in P2P Target mode.
 *  It shall register for data message which shall be sent from remote P2P Initiator device.
 *  Upon receiving data message, upper layer call back function shall be invoked and received data shall be shared.
 *
 *  \param[in] pNciCtx pointer to the NCI context structure
 *  \param[in] pReceiveCb pointer to call back function of type #pphNciNfc_IfNotificationCb_t
 *  \param[in] pContext upper layer context
 *
 *  \return NFCSTATUS_PENDING Receive in progress
 *  \return NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 *  \return NFCSTATUS_FEATURE_NOT_SUPPORTED P2P not active
 *  \return NFCSTATUS_FAILED P2P receive failed
 *  \return NFCSTATUS_INSUFFICIENT_RESOURCES Failed to allocate memory
 */
extern NFCSTATUS
phNciNfc_ReceiveData(void *pNciCtx,
                     pphNciNfc_IfNotificationCb_t pReceiveCb,
                     void* pContext);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall be invoked as part of P2P remote device send in P2P Target mode.
 *   It shall send the data received from upper layer to the remote P2P Initiator device.
 *
 *  \param[in] pNciCtx pointer to the NCI context structure
 *  \param[in] pSendCb pointer to call back function of type #pphNciNfc_IfNotificationCb_t
 *  \param[in] pContext upper layer context
 *  \param[in] pSendData Data to be sent to the remote P2P Initiator
 *
 *  \return NFCSTATUS_PENDING Sending in progress
 *  \return NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 *  \return NFCSTATUS_FEATURE_NOT_SUPPORTED P2P not active
 *  \return NFCSTATUS_FAILED P2P send failed
 *  \return NFCSTATUS_INSUFFICIENT_RESOURCES Failed to allocate memory
 */
extern NFCSTATUS
phNciNfc_SendData(void *pNciCtx,
                  pphNciNfc_IfNotificationCb_t pSendCb,
                  void* pContext,
                  phNfc_sData_t *pSendData);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function aborts data transfer
 *  \param[in] pNciHandle pointer to the NCI context structure
 */
extern void
phNciNfc_AbortDataTransfer(void *pNciHandle);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief This function allows the upper layer to register events with a secure element.
 *
 * \param[in] pNciCtx Nci Handle or the context of the NCI Layer.
 * \param[in] pSeHandle Secure element handle for which event shall be registered or unregistered.
 * \param[in] pSeEventCb upper layer call back function pointer which shall be invoked upon receiving event.
 * \param[in] pContext Upper layer HCI context to be returned in the callback.
 *
 * \retval #NFCSTATUS_SUCCESS Se event successfulyl registered
 * \retval #NFCSTATUS_FAILED Registration failed as there are no slots available/logical conn not found
 * \retval #NFCSTATUS_NOT_INITIALIZED NCI layer is not initialized
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 */
extern NFCSTATUS
phNciNfc_RegisterHciSeEvent(void *pNciCtx,
                            void* pSeHandle,
                            pphNciNfc_RegDataCb_t pSeEventCb,
                            void* pContext);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall be invoked as part of SE send operation.
 *  It shall send the data received from upper layer to the NFCEE.
 *
 *  \param[in] pNciCtx pointer to the NCI context structure
 *  \param[in] pSeHandle SE handle
 *  \param[in] pSendCb pointer to call back function of type #pphNciNfc_IfNotificationCb_t
 *  \param[in] pContext upper layer context
 *  \param[in] pSendData Data to be sent to the SE
 *
 *  \return NFCSTATUS_PENDING Sending in progress
 *  \return NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 *  \return NFCSTATUS_FAILED send failed
 *  \return NFCSTATUS_INSUFFICIENT_RESOURCES Failed to allocate memory
 */
extern NFCSTATUS
phNciNfc_SeSendData(void* pNciCtx,
                    void* pSeHandle,
                    pphNciNfc_IfNotificationCb_t pSendCb,
                    void* pContext,
                    phNfc_sData_t *pSendData);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_SetConfigRfParameters function configures the poll and listen mode
 * configuration parameters. If also validates the input configuration parameter.
 *
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 * \param[in] pDiscConfigParams Pointer to #phNciNfc_RfDiscConfigParams_t structure.
 * \param[in] pConfigRfNotifyCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Rf configuration in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Rf configuration failed.
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized
 */
extern NFCSTATUS
phNciNfc_SetConfigRfParameters(
                            void*                          pNciHandle,
                            pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                            pphNciNfc_IfNotificationCb_t   pConfigRfNotifyCb,
                            void*                          pContext);

extern NFCSTATUS
phNciNfc_SetConfigRaw(
                       void*                      pNciHandle,
                       uint8_t *pBuff, uint16_t Length,
                       pphNciNfc_IfNotificationCb_t pConfigRfNotifyCb,
                       void*                      pContext);


/**
 * \ingroup grp_nci_nfc
 *
 * \brief This function gets the poll and listen mode configuration
 * parameters from NFCC. A chain of commands shall be sent to NFCC if the
 *  parameters length in response exceeds max packet payload size.
 *
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 *
 * \param[in] tConfigInfo Strucure containing the parameters to retrieve from NFCC.
 * \param[in] pDiscConfigParams Pointer to #phNciNfc_RfDiscConfigParams_t structure.
 * \param[in] pConfigRfNotifyCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Rf configuration in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Rf configuration failed.
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized
 */
extern NFCSTATUS
phNciNfc_GetConfigRfParameters(
                            void*                          pNciHandle,
                            pphNciNfc_RfDiscConfigParams_t pDiscConfigParams,
                            pphNciNfc_IfNotificationCb_t   pConfigRfNotifyCb,
                            void*                          pContext);

extern NFCSTATUS
phNciNfc_GetConfigRaw(
                      void*                      pNciHandle,
                      uint8_t *pBuff, uint16_t Length,
                      pphNciNfc_IfNotificationCb_t pConfigRfNotifyCb,
                      void*                      pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_ConfigMapping function mapps Rf protocols to Rf interfaces  which
 * shall be used for the communication from the DH to a Remote NFC Endpoint using a specific RF Protocol.
 * The function also validates the input configuration parameters.
 *
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 * \param[in] bNumMapEntries Number of Rf protocol to Rf interface entries
 * \param[in] pProtoIfMapping Pointer to #phNciNfc_MappingConfig_t structure.
 * \param[in] pConfigMappingNotifyCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Protocol interface mapping in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Protocol interface mapping failed.
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized
 */
extern NFCSTATUS
phNciNfc_ConfigMapping(
                        void*                        pNciHandle,
                        uint8_t                      bNumMapEntries,
                        pphNciNfc_MappingConfig_t    pProtoIfMapping,
                        pphNciNfc_IfNotificationCb_t pConfigMappingNotifyCb,
                        void*                        pContext);

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_SetRtngTableConfig function configures listen mode routing table which
 * is required to provide the NFCC the information on where to route received data when
 * activated in Listen Mode. The function also validates the input routing parameters.
 *
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 * \param[in] bNumRtngEntries Number of listen mode routing table entries
 * \param[in] pRtngConfig Pointer to #phNciNfc_RtngConfig_t structure.
 * \param[in] pSetRtngTableNotifyCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Listen mode routing table configuration in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Listen mode routing table configuration failed.
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized.
 */
extern NFCSTATUS
phNciNfc_SetRtngTableConfig(
                            void*                        pNciHandle,
                            uint8_t                      bNumRtngEntries,
                            pphNciNfc_RtngConfig_t       pRtngConfig,
                            pphNciNfc_IfNotificationCb_t pSetRtngTableNotifyCb,
                            void*                        pContext
                           );

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_GetRtngTableConfig function reads listen mode routing table configuration from NFCC.
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 * \param[in] pGetRtngTableNotifyCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Reading listen mode routing table configuration in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not be interpreted properly.
 * \retval #NFCSTATUS_FAILED Failed to read listen mode routing table configuration.
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized.
 */
extern NFCSTATUS
phNciNfc_GetRtngTableConfig(
                            void*                        pNciHandle,
                            pphNciNfc_IfNotificationCb_t pGetRtngTableNotifyCb,
                            void*                        pContext
                           );

/**
 * \ingroup grp_nci_nfc
 *
 * \brief The phNciNfc_RfParameterUpdate function shall update the Rf communication parameters.
 * (once the Frame RF Interface has been activated).
 *
 * \param[in] pNciHandle Handle or the context of the NCI Layer.
 * \param[in] bNumOfParams Number of listen mode routing table entries
 * \param[in] pRfParamUpdate Pointer to #phNciNfc_RfParamUpdate_t structure.
 * \param[in] pRfParamUpdateCb Upper layer release callback function pointer.
 * \param[in] pContext Upper layer context.
 *
 * \retval #NFCSTATUS_PENDING Rf parameter update in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER One or more of the supplied parameters could not
 * be interpreted properly.
 * \retval #NFCSTATUS_FAILED Rf parameter update failed
 * \retval #NFCSTATUS_NOT_INITIALISED Stack not initialized.
 */
extern NFCSTATUS
phNciNfc_RfParameterUpdate(
                           void*                        pNciHandle,
                           uint8_t                      bNumOfParams,
                           pphNciNfc_RfParamUpdate_t    pRfParamUpdate,
                           pphNciNfc_IfNotificationCb_t pRfParamUpdateCb,
                           void*                        pContext
                          );

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall reset the NCI and NFCC based on the input 'eNciResetType' type.
 *  If eNciResetType is:
 *  phNciNfc_NciReset_DeInit_KeepConfig - Resets NFCC by sending RESET command (keeping config) and releases Nci Handle
 *  phNciNfc_NciReset_DeInit_ResetConfig - Resets NFCC by sending RESET command (reseting config) and releases Nci Handle
 *  phNciNfc_NciReset_Mgt_Reset - Releases Nci handle without sending reset command to NFCC
 *  phNciNfc_NciReset_ClearNci - Resets the content of Nci Context strucutre (for internal use only)
 *  phNciNfc_NciReset_ResetNfcc_KeepConfig - Resets NFCC by sending a RESET command (keeping config) (for internal use only)
 *  phNciNfc_NciReset_ResetNfcc_ResetConfig - Resets NFCC by sending a RESET command (reseting config) (for internal use only)

 *  \param[in] pNciHandle pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] eNciResetType Nci reset type (#phNciNfc_NciReset_t)
 *  \param[in] pResetCb upper layer call back function (only used if 'eNciResetType' is 'phNciNfc_NciReset_DeInit' or 'phNciNfc_NciReset_ResetNfcc')
 *  \param[in] pContext upper layer context (only used if 'eNciResetType' is 'phNciNfc_NciReset_DeInit' or 'phNciNfc_NciReset_ResetNfcc')
 *
 *  \return #NFCSTATUS_NOT_INITIALISED Stack not initialized
 *  \return #NFCSTATUS_SUCCESS Reset success
 *  \return #NFCSTATUS_FAILED Reset failed
 *  \return #NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 */
extern NFCSTATUS
phNciNfc_Reset(void*                        pNciHandle,
               phNciNfc_NciReset_t          eNciResetType,
               pphNciNfc_IfNotificationCb_t pResetCb,
               void*                        pContext);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall return the NFCC features supported to the caller
 *
 *  \param[in] pNciCtx pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] pNfccFeatures pointer to structure of type #pphNciNfc_NfccFeatures_t
 *
 *  \return #NFCSTATUS_NOT_INITIALISED Stack not initialized
 *  \return #NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 *  \return #NFCSTATUS_SUCCESS Input Data has been sent to NFCC
 */
extern NFCSTATUS
phNciNfc_GetNfccFeatures(
                       void *pNciCtx,
                       pphNciNfc_NfccFeatures_t pNfccFeatures
                      );

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall store the Header rom and UID information of the Jewel tag.
 *
 *  \param[in] pContext pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] pRemDev Pointer to the remote device structure
 *  \param[in] pBuff Pointer to the buffer where RID response is received.
 *
 *  \return #NFCSTATUS_FAILED Failed to store response.
 *  \return #NFCSTATUS_SUCCESS Stored the response successfully.
 */
extern NFCSTATUS
phNciNfc_UpdateJewelInfo(void *pContext,\
                         pphNciNfc_RemoteDevInformation_t pRemDev,\
                         uint8_t *pBuff);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall store the system information of the ISO15693 tag.
 *
 *  \param[in] pContext pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] pRemDev Pointer to the remote device structure
 *  \param[in] pBuff Pointer to the buffer where GetSysInfo response is received.
 *
 *  \return #NFCSTATUS_FAILED Failed to store response.
 *  \return #NFCSTATUS_SUCCESS Stored the response successfully.
 */
extern NFCSTATUS
phNciNfc_Update15693SysInfo(void *pContext,\
                            pphNciNfc_RemoteDevInformation_t pRemDev,\
                            uint8_t *pBuff);

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function sets up the required params for SENSF_REQ to be sent to Type 3
 *         Tag through NFCC
 *
 *  \param[in] psContext        pointer to the NCI context structure
 *  \param[in] pPollReqParams   holds the SystemCode,ReqCode and TimeSlotNum to be included in SENSF_REQ cmd
 *  \param[in] pNotify          Notify to caller after T3T response/Ntf.
 *  \param[in] pContext         Caller context
 *
 *  \return NFCSTATUS_INVALID_PARAMETER     - Invalid input parameters
 *  \return NFCSTATUS_FEATURE_NOT_SUPPORTED - Felica feature is not supported
 *  \return NFCSTATUS_PENDING               - Sent request is Pending
 */
extern NFCSTATUS
phNciNfc_T3TPollReq(
                    void   *psContext,
                    pphNciNfc_SensFReqParams_t  pPollReqParams,
                    pphNciNfc_IfNotificationCb_t pNotify,
                    void *pContext
                    );

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall initiate the Iso-Dep presence check procedure
 *
 *  \param[in] pNciHandle pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] pIsoDepPresChkCb Upper layer call back function pointer
 *  \param[in] pContext Upper layer context
 *
 *  \return #NFCSTATUS_NOT_INITIALISED Stack not initialized
 *  \return #NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 *  \return #NFCSTATUS_PENDING Presence has been initiated
 */
extern NFCSTATUS
phNciNfc_IsoDepPresenceChk(void* pNciHandle,
                           pphNciNfc_IfNotificationCb_t pIsoDepPresChkCb,
                           void* pContext
                           );

/**
 *  \ingroup grp_nci_nfc
 *
 *  \brief This function shall set the current power sub state
 *
 *  \param[in] pNciHandle pointer to the NCI context structure (#pphNciNfc_Context_t)
 *  \param[in] bSubState The current power sub state
 *  \param[in] pSetPowerSubStateCb Upper layer call back function pointer
 *  \param[in] pContext Upper layer context
 *
 *  \return #NFCSTATUS_NOT_INITIALISED Stack not initialized
 *  \return #NFCSTATUS_INVALID_PARAMETER Invalid input parameter
 */
extern NFCSTATUS
phNciNfc_SetPowerSubState(void* pNciHandle,
                          phNciNfc_PowerSubState_t bSubState,
                          pphNciNfc_IfNotificationCb_t pSetPowerSubStateCb,
                          void* pContext
                          );
