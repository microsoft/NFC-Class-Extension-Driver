/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>
#include <phNfcCompId.h>
#include <phNfcTypes.h>
#include <phFriNfc_NdefMap.h>
#include <phFriNfc_OvrHal.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phOsalNfc_Timer.h>
#include <phFriNfc_NdefReg.h>
#include <phLibNfc.h>
#include "phLibNfc_ndef_raw.h"

#include <phNfcHalTypes2.h>
#include "phLibNfc_CeHost.h"
#include "phLibNfc_State.h"
#include "phLibNfc_Sequence.h"
#include "phLibNfc_Llcp.h"
#include "phLibNfc_Discovery.h"
#include "phLibNfc_Ioctl.h"
#include "phLibNfc_IoctlUtils.h"

/**Maximum number of Records.Presently set to a realistic value of 128 Configurable upto 1K*/
#define PH_LIBNFC_INTERNAL_MAX_NO_OF_RECORDS        128U

#define PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH           PHHAL_MAX_ATR_LENGTH

/**Map LibNfc Remote device handle to Nci RemoteDevice Handle*/
#define PH_LIBNFC_INTERNAL_LIBTONCI_MAP             0x01
/**Map Nci Remote Device Handle to LibNfc Remote Device Handle*/
#define PH_LIBNFC_INTERNAL_NCITOLIB_MAP             0x00

/*Register of IC manufacturers*/
#define PH_LIBNFC_MANUFACTURER_UNKNOWN              (0x00)
#define PH_LIBNFC_MANUFACTURER_NXP                  (0x04)
#define PH_LIBNFC_MANUFACTURER_QC                   (0x57)
#define PH_LIBNFC_MANUFACTURER_MARVELL              (0x88)

#define PH_LIBNFC_INTERNAL_SEND_BUFF_SIZE           (0x212) /*Buffer to be used for mapping (530 bytes) */
#define PH_LIBNFC_INTERNAL_RECV_BUFF_SIZE           (0x212) /*Buffer for Receiving Data from the card */

#define PH_LIBNFC_INTERNAL_INPROGRESS               (0x01) /*Deactivate operation in progress*/
#define PH_LIBNFC_INTERNAL_COMPLETE                 (0x00) /*Deacivate operation not in progrss*/
#define PH_LIBNFC_INTERNAL_CALL_CB_TRUE             (0x10) /*In notification handler call client callback*/
#define PH_LIBNFC_INTERNAL_CALL_CB_FALSE            (0x11) /*In notification handler do not call client callback*/

#define PH_LIBNFC_INTERNAL_CHK_NDEF_NOT_DONE        0x02U

#define PH_LIBNFC_INTERNAL_HCI_CONNECTIVITY_EVENT   (0x10)
#define PH_LIBNFC_INTERNAL_HCI_TRANSACTION_EVENT    (0x12)

// ETSI TS 102 622, v12.1.0, Section 12.3.2
enum PHHCINFC_APDU_APPLICATION_GATE_EVENTS
{
    PHHCINFC_PROP_DATA_EVENT = 0x10, // aka EVT_R-APDU
    PHHCINFC_PROP_WTX_EVENT = 0x11, // aka EVT_WTX
};

#define PHHCINFC_NO_PIPE_DATA                       0xFF
/* HCI EVT_ABORT and EVT_ATR as per ETSI12*/
#define PHHCINFC_EVENT_ABORT                        0x11

#define PHHCINFC_EVENT_ATR_RECV                     0x12

#define PHLIBNFC_TRANSACTION_AID                    0x81
#define PHLIBNFC_TRANSACTION_PARAM                  0x82

#define PHHCINFC_TOTAL_NFCEES                       0x03

/*If Reactivate sequnce failed to reactivate Tag then change reactivation sequence
    Before: Deactivate with sleep + select command
    After : Select command*/
#define PH_LIBNFC_REACT_DEACTSELECT                 (0x00)
#define PH_LIBNFC_REACT_ONLYSELECT                  (0x01)

/* Enable high data rate request flag for all transceive commands */
#define PH_LIBNFC_15693_ENABLE_HIGH_DATARATE        (0x02)

#define PH_LIBNFC_AUTHENTICATION_KEY                (0x00U)
#define PH_LIBNFC_ENABLE_KEY_B                      (0x80U)

#define PHLIBNFC_MFCUIDLEN_INAUTHCMD        0x04
#define PHLIBNFC_MFC_AUTHKEYLEN             0x06
#define PHLIBNFC_MFCWITH_7BYTEUID           0x07
#define PHLIBNFC_MFCUIDINDEX_7BYTEUID       0x03

#define PHLIBNFC_MIFAREUL_SAK               0x00 /* MIFARE UL and ULC SAK */
#define PHLIBNFC_MFC1K_SAK                  0x08 /* Classic 1K Tag*/
#define PHLIBNFC_MFC1K_WITHSAK1             0x01 /* Old Classic 1K Tag*/
#define PHLIBNFC_MFC4K_SAK                  0x18 /* Classic 4K Tag*/
#define PHLIBNFC_MFCMINI_SAK                0x09 /* Classic Mini Tag*/
#define PHLIBNFC_MFC1K_WITHSAK88            0x88 /* Classic 1K - Infineon card*/
#define PHLIBNFC_MFC4K_WITHSAK98            0x98 /* Classic 4K - Pro card*/
#define PHLIBNFC_MFC4K_WITHSAKB8            0xB8 /* Classic 4K - Pro card*/
#define PHLIBNFC_MFC1K_WITHSAK28            0x28 /* Classic 1K - Emulation*/
#define PHLIBNFC_MFC4K_WITHSAK38            0x38 /* Classic 4K - Emulation*/

#define PHLIBNFC_MIFARESTD4K_BLK128         128  /* Block number 128 for Mifare 4k */
#define PHLIBNFC_MIFARESTD_SECTOR_NO32      32   /* Sector 32 for Mifare 4K*/
#define PHLIBNFC_MIFARESTD_BLOCK_BYTES      16   /* Bytes per block after block 32 for Mifare 4K*/
#define PHLIBNFC_MFC_EMBEDDED_KEY           0x10 /* Mifare classic use Embedded Key */

extern phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDeact[];
extern phLibNfc_Sequence_t gphLibNfc_DiscSeqWithDeactSleep[];
extern phLibNfc_Sequence_t gphLibNfc_MFCSendAuthCmdForPresChk[];
extern phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeq2[];
extern phLibNfc_Sequence_t gphLibNfc_Felica_CheckPresSeq[];
extern phLibNfc_Sequence_t gphLibNfc_IsoDep_CheckPresSeq[];
extern phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeq2Select [];

typedef struct phLibNfc_NfcIpInfo
{
    phNfc_sData_t                *p_recv_data;
    uint32_t                     recv_index;
    /*NFC IP remote initator handle */
    uint32_t                     Rem_Initiator_Handle;
}phLibNfc_NfcIpInfo_t;

typedef struct phLibNfc_RemoteDev_Map
{
    void *pLibNfc_RemoteDev_List;       /**< discovered Target NCI handle */
    void *pNci_RemoteDev_List;          /**< discoverd Target LIBNFC handle */
}phLibNfc_RemoteDevList_Map_t;

typedef struct phLibNfc_Status
{
    BitField_t                    RlsCb_status : 1;
    BitField_t                    DiscEnbl_status : 1;
    BitField_t                    Connect_status : 1;
    BitField_t                    TransProg_status : 1;
    BitField_t                    RelsProg_status : 1;
    BitField_t                    GenCb_pending_status : 1;
    BitField_t                    Shutdown_pending_status : 1;
    BitField_t                    Discovery_pending_status : 1;
}phLibNfc_Status_t;

typedef struct phLibNfc_CB_Info
{
    pphLibNfc_InitCallback_t       pClientInitCb;
    void                           *pClientInitCntx;

    pphLibNfc_RspCb_t              pClientShutdownCb;
    void                           *pClientShtdwnCntx;

    pphLibNfc_ConnectCallback_t    pClientConnectCb;
    void                           *pClientConCntx;

    pphLibNfc_TransceiveCallback_t  pClientTranscvCb;
    void                           *pClientTranscvCntx;

    pphLibNfc_DisconnectCallback_t pClientDisConnectCb;
    void                           *pClientDConCntx;

    phLibNfc_NtfRegister_RspCb_t   pClientNtfRegRespCB;
    void                           *pClientNtfRegRespCntx;

    pphLibNfc_RspCb_t              pClientDisConfigCb;
    void                           *pClientDisCfgCntx;

    pphLibNfc_ChkNdefRspCb_t       pClientCkNdefCb;
    void                           *pClientCkNdefCntx;

    pphLibNfc_TransceiveCallback_t pClientTransceiveCb;
    void                           *pClientTranseCntx;

    pphLibNfc_TransceiveCallback_t pSeClientTransCb;
    void                           *pSeClientTransCntx;

    /* Se WTX event callback and its context */
    pphLibNfc_SE_WtxCallback_t     pSeClientEvtWtxCb;
    void                           *pSeClientEvtWtxCntx;

    pphLibNfc_RspCb_t              pClientRdNdefCb;
    void                           *pClientRdNdefCntx;

    pphLibNfc_RspCb_t              pClientWrNdefCb;
    void                           *pClientWrNdefCntx;

    pphLibNfc_Ndef_Search_RspCb_t   pClientNdefNtfRespCb;
    void                           *pClientNdefNtfRespCntx;

    pphLibNfc_RspCb_t              pClientPresChkCb;
    void                           *pClientPresChkCntx;

    pphLibNfc_RspCb_t              pClientRoutingCfgCb;
    void                           *pClientRoutingCfgCntx;

    pphLibNfc_ChkLlcpRspCb_t       pClientLlcpCheckRespCb;
    void                           *pClientLlcpCheckRespCntx;

    pphLibNfc_LlcpLinkStatusCb_t   pClientLlcpLinkCb;
    void                           *pClientLlcpLinkCntx;

    pphLibNfc_RspCb_t              pClientLlcpDiscoveryCb;
    void                           *pClientLlcpDiscoveryCntx;

    pphLibNfc_RspCb_t              pClientNfcIpCfgCb;
    void                           *pClientNfcIpCfgCntx;

    pphLibNfc_RspCb_t              pClientNfcIpTxCb;
    void                           *pClientNfcIpTxCntx;

    pphLibNfc_Receive_RspCb_t      pClientNfcIpRxCb;
    void                           *pClientNfcIpRxCntx;

    pphLibNfc_RspCb_t              pNFCEEDiscoveryCb;
    void                           *pNFCEEDiscoveryCntx;

    pphLibNfc_SE_SetModeRspCb_t    pSeSetModeCb;
    void                           *pSeSetModeCtxt;

    pphLibNfc_SE_NotificationCb_t  pSeListenerNtfCb;
    void                           *pSeListenerCtxt;

    phLibNfc_NtfRegister_RspCb_t   pCeHostNtfCb;
    void                           *pCeHostNtfCntx;

    pphLibNfc_SE_SetModeRspCb_t     pPowerCtrlLinkCb;
    void                            *pPowerCtrlLinkCntx;

    /* Se Get Atr Call back & it's context */
    pphLibNfc_GetAtrCallback_t     pSeClientGetAtrCb;
    void                           *pSeClientGetAtrCntx;

}phLibNfc_CB_Info_t;

typedef struct phLibNfc_NdefInfo
{
    bool_t                       NdefContinueRead;
    uint32_t                     NdefActualSize,
                                 AppWrLength;
    phFriNfc_NdefMap_t           *psNdefMap;
    uint16_t                     NdefSendRecvLen;
    uint16_t                     NdefDataCount;
    phNfc_sData_t                *psUpperNdefMsg;
    uint32_t                     dwWrLength;
    uint32_t                     NdefReadTimerId,
                                 NdefLength;
    uint8_t                      is_ndef ;
    phFriNfc_sNdefSmtCrdFmt_t    *ndef_fmt ;
    phLibNfc_Last_Call_t         eLast_Call;
    uint32_t                     Chk_Ndef_Timer_Id;
    pphLibNfc_RspCb_t            pClientNdefFmtCb;
    void                        *pClientNdefFmtCntx;
    phLibNfc_Ndef_SrchType_t    *pNdef_NtfSrch_Type;

}phLibNfc_NdefInfo_t;

typedef struct phLibNfc_NdefRecInfo
{
    phFriNfc_NdefReg_CbParam_t  CbParam;
    phFriNfc_NdefReg_t          NdefReg;
    uint8_t                     *NdefTypes_array[100];
    phFriNfc_NdefRecord_t       RecordsExtracted;
    uint8_t                     ChunkedRecordsarray[PH_LIBNFC_INTERNAL_MAX_NO_OF_RECORDS];
    uint32_t                    NumberOfRecords;
    uint8_t                     IsChunked[PH_LIBNFC_INTERNAL_MAX_NO_OF_RECORDS];
    uint32_t                    NumberOfRawRecords;
    uint8_t                     *RawRecords[PH_LIBNFC_INTERNAL_MAX_NO_OF_RECORDS];
    phFriNfc_NdefReg_Cb_t       *NdefCb;
    phNfc_sData_t               ndef_message;
}phLibNfc_NdefRecInfo_t;

typedef enum phLibNfc_DummyEventType
{
    phLibNfc_DummyEventSetMode = 0x01,      /**<Set SE Set Mode*/
    phLibNfc_DummyEventSetP2PConfigs,       /**<Allow to set P2P Config parameters*/
    phLibNfc_DummyEventFelicaChkPresExtn,   /**<Check presence Felica event*/
    phLibNfc_DummyEventIsoDepChkPresExtn,   /**<Check presence IsoDep event*/
    phLibNfc_DummyEventChkPresMFC,          /**<Check presence extension event*/
    phLibNfc_DummyEventSetRtngCfg,          /**<Set routing table configuration event*/
    phLibNfc_DummyEventPowerAndLinkCtrl,    /**<Set SE Power And Link Control*/
    phLibNfc_DummyEventInvalid              /**<Invalid Dummy event*/
}phLibNfc_DummyEventType_t; /**< Dummy event enum type*/

typedef struct phLibNfc_DummyInfo
{
    phLibNfc_DummyEventType_t Evt; /**< Event type*/
    void *Params;                 /**< Pointer to structure corresponding to event*/
}phLibNfc_DummyInfo_t; /**< Structure for dummy events*/

typedef struct phLibNfc_SeCtxt
{
    pphLibNfc_SE_List_t             pActiveSeInfo;
    phNciNfc_NfceeModes_t           eNfceeMode;
    phNciNfc_PowerLinkModes_t       ePowerLinkMode;
    pphNciNfc_RtngConfig_t          pRoutingCfgBuffer;
    phLibNfc_eSE_ActivationMode     eActivationMode;    /* User rquested SE activation mode */
    uint32_t                        nNfceeDiscNtf;      /* Number of NFCEE discovery NTF to process */
}phLibNfc_SeCtxt_t, *pphLibNfc_SeCtxt_t;

typedef struct phLibNfc_SelectInfo
{
    bool_t                   bSelectInpAvail;  /**< flag to indicate if any input is provided */
    phNciNfc_RfInterfaces_t  eRfIf;        /**< interface to switch to during SELECT */
}phLibNfc_SelectInfo_t;

typedef struct phLibNfc_MifareCInfo
{
    phNfc_eMifareCmdList_t  cmd;        /**< Technology Specific commands*/
    uint8_t                 addr;       /**< Internal Address Field required for only Mifare
                                             Family Proprietary Cards*/
    uint8_t                 key;        /**< The Authentication key>*/
    uint8_t                 MFCKey[8];  /**< The user key is stored here>*/
}phLibNfc_MifareCInfo_t;

typedef struct phLibNfc_SE_Info
{
    uint8_t             bSeCount; /**< No of Se discovered during init*/
    phLibNfc_SE_List_t  tSeList[PHHCINFC_TOTAL_NFCEES]; /**< Structure object to #phLibNfc_SE_List_t*/
    uint8_t             bSeState[PHHCINFC_TOTAL_NFCEES]; /**< #phLibNfc_SE_Status_t*/
    uint8_t             bNfceeSuppNfcTech;   /**< Bitmask, contains information of which NFC technology is
                                                  being supported on connected NFCEE*/
} phLibNfc_SE_Info_t, *pphLibNfc_SE_Info_t;

typedef enum phLibNfc_SE_Status
{
    phLibNfc_SeStateInvalid = 0x0,  /**< State Invalid*/
    phLibNfc_SeStateNotInitialized, /** <State Not Inialized - Yet to Start Initialization*/
    phLibNfc_SeStateInitializing, /** <State where Initialization is in Progress*/
    phLibNfc_SeStateInitialized,  /**< State where SE is Initialized*/
} phLibNfc_SE_Status_t;

typedef enum phLibNfc_SE_Index
{
    phLibNfc_SE_Index_HciNwk     = 0x00,  /**< Index to Store HCI Network Info in tSeInfo element in LibNfc Context*/
    phLibNfc_SE_Index_UICC       = 0x01,  /**< Index to Store UICC Info in tSeInfo element in LibNfc Context*/
    phLibNfc_SE_Index_eSE        = 0x02,  /**< Index to Store eSE Info in tSeInfo element in LibNfc Context*/
    phLibNfc_SE_Index_MaxCount   = 0x03   /**< Maximum number of SEs supported by LibNfc Context*/
} phLibNfc_SE_Index_t;

typedef struct phLibNfc_LibContext
{
    phLibNfc_InitType_t eInitType;                       /**<Initialization type*/
    phNciNfc_ResetType_t eConfigStatus;                  /**<Reset Configuration status*/
    phLibNfc_sConfig_t Config;                           /**<Structure object to #phLibNfc_sConfig_t*/
    phNfc_sHwReference_t sHwReference;                   /**<Structure object to #phNfc_sHwReference_t*/
    phNciNfc_NfccFeatures_t tNfccFeatures;               /**<Structure object to #phNciNfc_NfccFeatures_t*/
    phLibNfc_Status_t status;                            /**<Structure object to #phLibNfc_Status_t*/
    phLibNfc_CB_Info_t CBInfo;                           /**<Structure object to #phLibNfc_CB_Info_t*/
    phLibNfc_SeCtxt_t sSeContext;
    phLibNfc_StateContext_t StateContext;                /**<Structure object to #phLibNfc_StateContext_t*/
    void * Connected_handle;                             /**<Handle for target connected*/
    void * DummyConnect_handle;                          /**<Handle stored during dummy connect*/
    void * Disc_handle[MAX_REMOTE_DEVICES];              /**<Discovered handle get stored here*/
    void * pInfo;                                        /**<Pointer to phNciNfc_DeviceInfo_t stored here */
    phLibNfc_Event_t DiscTagTrigEvent;                   /**<Trig event of Discovery from NCI*/
    phLibNfc_RemoteDevList_t psRemoteDevList[MAX_REMOTE_DEVICES]; /**<Structure object to #phLibNfc_RemoteDevList_t*/
    uint8_t bTechMode;                                   /**<Technology mode of the NFCC under which remode device is activated. 1=Poll Mode, 0=Listen Mode*/
    uint8_t bRfInterface;                                /**<RF Interface of the NFCC on which remote device is activated. 1=NFC DEP, 0=ISO DEP*/
    uint8_t dev_cnt;                                     /**<Number of tagets connected*/
    phLibNfc_eDiscAndDisconnMode_t DiscDisconnMode;      /**<Structure object to #phLibNfc_eDiscAndDisconnMode_t*/
    phLibNfc_Registry_Info_t RegNtfType;
    phLibNfc_Discover_Info_t DiscoverdNtfType;
    phLibNfc_NfcIpInfo_t tNfcIp_Context;
    phLibNfc_sADD_Cfg_t tADDconfig;
    phNciNfc_DeviceInfo_t* pDiscInfo;
    phLibNfc_SelectInfo_t tSelInf;                       /**<Discover select input params*/
    uint32_t bTotalNumDev;                               /**<Number of discovery notifications*/
    phLibNfc_RemoteDevList_Map_t Map_Handle[MAX_REMOTE_DEVICES];
    phLibNfc_sRemoteDevInformation_t* psRemoteDevInfo;
    phLibNfc_NdefInfo_t ndef_cntx;
    phNfc_sDevInputParam_t* psDevInputParam;
    phFriNfc_OvrHal_t* psOverHalCtxt;
    uint8_t bDiscoverInProgress;                            /**<Flag to track whether discovery is in progress or not */
    phLibNfc_sTransceiveInfo_t* psBufferedAuth;
    phLibNfc_NdefRecInfo_t phLib_NdefRecCntx;
    uint8_t aSendBuff[PH_LIBNFC_INTERNAL_SEND_BUFF_SIZE];   /**< buffer used for sending mapped command to the lower layer*/
    uint8_t aRecvBuff[PH_LIBNFC_INTERNAL_RECV_BUFF_SIZE];   /**< buffer used for receiving response from lower layer*/
    uint8_t bLastCmdSent;                                   /**< Last command sent during transceive */
    uint8_t bSkipTransceive;                                /**< Flag specifies whether to skip transceive or not. Since
                                                                 'phLibNfc_TranscvExit' is part of 'phLibNfc_StateFptr' exit function,
                                                                 phLibNfc_TranscvExit function gets invoked during RemoteDev_Disconnect
                                                                 in which case transceive is not required. Inorder to skip the same,
                                                                 this flag has been used */

    uint8_t SeqNext;                         /**<Next sequence*/
    uint8_t SeqMax;                          /**<Maximum number of sequences*/
    phLibNfc_Sequence_t *pSeqHandler;        /**<Pointer to #phLibNfcNfc_Sequence_t*/

    uint8_t bDiscovery_Notify_Enable;        /**<This flag is used for rediscovery purpose*/
    uint8_t bCall_RegisterListner_Cb;        /**<This flag is used to convey register listner that do not call callback in case of rediscovery*/
    uint8_t bHceSak;                         /**<This flag is used to indicate that Host card emulation Sak Value is added during discovery */
    phLibNfc_LlcpInfo_t llcp_cntx;           /**<LLCP Info*/

    phLibNfc_sTransceiveInfo_t *psTransceiveInfo; /**< Transceive Info stored in case of Mifare Classic tag and Write16 command*/
    phLibNfc_sTransceiveInfo_t *psDummyTransceiveInfo; /**< For workaround in MFC after write16 command authentication for other sector is failing
                                                            but after authetication if read is executed then authentication for other sector is passing
                                                            below transceive is used for that read purpose*/

    phLibNfc_sTransceiveInfo_t *psTransceiveInfo1; /**< This buffer is used for authentication command sent in form of RAW command*/
    pphNfc_SeAtr_Info_t     pAtrInfo; /**<Structure pointer to store Hci Get Atr receive buffers */

    phNfc_sData_t tTranscvBuff; /**< Holds the Transceive buffer passed by caller */
    uint8_t bT3tMax;            /**< The maximum index of LF_T3T_IDENTIFIERS supported by the NFCC */
    uint8_t bReactivation_Flag; /**< Reactivation status which is used to select reactivation sequence
                                     for MFC and in case of connect twice,check presence and in transcvCb.
                                     This flag is set if in transceive Mifare classic related command sent in command format
                                     or in raw format read16(0x30),write16(0xA0),autheticate key A(0x60),authenticate key B(0x61)
                                     This flag is used in case of mifare classic reactivation*/
    uint8_t bPcdConnected;
    uint8_t bDiscDelayFlag;    /**< Flag to decide whether or not to introduce a delay before config discovery */
    void *pHciContext;         /**< Hci context */
    uint32_t dwHciTimerId;     /**< Hci timer ID */
    pphNfc_sSeTransceiveInfo_t pSeTransInfo; /**< Structure pointer to store Hci Transceive send and receive buffers */
    uint32_t WdTimerId;                 /**< Watchdog timer ID */
    phLibNfc_MifareCInfo_t tMfcInfo;    /**< Member to store the MFC Auth information*/
    uint8_t bDtaFlag;                   /**< This variable is used for DTA */
    phLibNfc_SE_Info_t tSeInfo;
    uint32_t dwHciInitDelay;            /**< HCI Init Delay */
    uint8_t HCE_FirstBuf;               /**< Flag to identify if the pkt is 1st data pkt or not */
    phNfc_sData_t HCE_FirstBuffer;      /**< Stored 1st HCE data Pkt */
    phNfc_sData_t tRfRawConfig;         /**< Structure object containing buffer of Rf raw config */
    phNciNfc_PowerSubState_t PwrSubState;      /**< The current switched on sub state */
    phNciNfc_PowerSubState_t TgtPwrSubState;   /**< The targetted switched on sub state */
}phLibNfc_LibContext_t,*pphLibNfc_LibContext_t, *pphLibNfc_Context_t; /**< pointer to #phLibNfc_LibContext_t structure */

extern pphLibNfc_LibContext_t phLibNfc_GetContext();

extern NFCSTATUS phLibNfc_MapCmds(phNciNfc_RFDevType_t RemDevType, phLibNfc_sTransceiveInfo_t* pTransceiveInfo, pphNciNfc_TransceiveInfo_t pMappedTranscvIf);
extern NFCSTATUS phLibNfc_NciTranscv(void* pNciHandle, void* pDevicehandle, pphNciNfc_TransceiveInfo_t psTransceiveInfo, void* pContext);
extern NFCSTATUS phLibNfc_MapRemoteDevHandle(phLibNfc_sRemoteDevInformation_t **Libnfc_RemoteDevHandle, phNciNfc_RemoteDevInformation_t **Nci_RemoteDevHandle, uint8_t Flag);

extern NFCSTATUS phLibNfc_ValidateNciHandle(pphNciNfc_RemoteDevInformation_t psRemoteDevHandle);
extern NFCSTATUS phLibNfc_ValidateDevHandle(phLibNfc_Handle hRemoteDevice);

extern void phLibNfc_MfcAuthInfo_Clear(pphLibNfc_LibContext_t pLibContext);
extern NFCSTATUS phLibNfc_MapBitRate(_In_ phNciNfc_BitRates_t NciBitRate, _Out_ phNfc_eDataRate_t *pLibNfcBitRate);

extern NFCSTATUS phLibNfc_StateDiscoveredEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3);
extern NFCSTATUS phLibNfc_StateDiscoveredExit(void *pContext, void *pParam1, void *pParam2, void *pParam3);
extern NFCSTATUS phLibNfc_StateDiscoveryEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3);
extern NFCSTATUS phLibNfc_StateSEListenEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3);

extern void phLibNfc_NotificationRegister_Resp_Cb(void* pContext, phNciNfc_NotificationType_t eNtfType, pphNciNfc_NotificationInfo_t pDevInfo, NFCSTATUS status);
extern void phLibNfc_GenericErrorHandler(void* pContext, phNciNfc_NotificationType_t eNtfType, pphNciNfc_NotificationInfo_t pGenericErrInfo, NFCSTATUS status);
extern void phLibNfc_ResetNtfHandler(void* pContext, phNciNfc_NotificationType_t eNtfType, pphNciNfc_NotificationInfo_t pGenericErrInfo, NFCSTATUS status);
extern void phLibNfc_DeActvNtfRegister_Resp_Cb(void* pContext, phNciNfc_NotificationType_t eNtfType, pphNciNfc_NotificationInfo_t pDevInfo, NFCSTATUS status);

extern NFCSTATUS phLibNfc_VerifyResponse(pphNciNfc_Data_t pTransactInfo, pphNciNfc_RemoteDevInformation_t NciRemoteDevHandle);
extern NFCSTATUS phLibNfc_InternConnect(void *pContext,void *pNciRemoteDevhandle,void *pLibRemotDevhandle);

extern NFCSTATUS phLibNfc_ChkMfCTag(pphNciNfc_RemoteDevInformation_t RemoteDevInfo);
extern void phLibNfc_ProcessDevInfo(void* pContext, phLibNfc_Event_t TrigEvent, pphNciNfc_DeviceInfo_t pDevInfo,NFCSTATUS wStatus);

extern void phLibNfc_RemoteDev_ConnectTimer_Cb(_In_ void *pLibContext);
extern void phLibNfc_TranscvCb(void* pContext, NFCSTATUS status, pphNciNfc_Data_t  pInfo);
extern void phLibNfc_RemoteDev_ChkPresence_Cb(void *pContext, phNfc_sData_t* pResBuffer, NFCSTATUS wStatus);

extern void phLibNfc_InternalSeq(void *pCtx, NFCSTATUS Status, pphNciNfc_Data_t pInfo);
extern NFCSTATUS phLibNfc_AddDeferredEvent(pphLibNfc_Context_t pCtx,phLibNfc_Event_t TrigEvent, void * Param1, void * Param2, void * Param3);
extern void phLibNfc_ClearAllDeferredEvents(pphLibNfc_Context_t pCtx);
extern void phLibNfc_DeferredEventHandler(_In_ void* pContext);

extern void phLibNfc_UpdateEvent(NFCSTATUS wStatus,phLibNfc_Event_t *pTrigEvent);
extern NFCSTATUS phLibNfc_GetConnectedHandle(phLibNfc_Handle *pHandle);
extern void phLibNfc_ClearLibContext(pphLibNfc_LibContext_t pLibContext);

extern void phHciNfc_Process_eSE_ClearALLPipes(void);
extern NFCSTATUS phLibNfc_DummyDisc(void *pContext, pphOsalNfc_TimerCallbck_t pTimerCb, uint32_t dwTimeOut);
