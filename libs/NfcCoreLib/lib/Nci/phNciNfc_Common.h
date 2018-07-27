/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include "phNciNfc.h"

/* Time out between command and response or send and receive data */
#define PHNCINFC_NCI_CMD_RSP_TIMEOUT    (2000)
/* Time out between init command and response */
#define PHNCINFC_NCI_INIT_RSP_TIMEOUT   (5000)
/* Default time out value to be used incase of transceive */
#define PHNCINFC_NCI_TRANSCEIVE_TIMEOUT (500)

/* Value indicates a invalid Discover ID */
#define PHNCINFC_INVALID_DISCID                 (0xFF)

/* Indicates the discovered NFCEE is not yet activated */
#define PH_NCINFC_NFCEE_NOT_ACTIVATED           (0xFF)

/* Value indicates Minimum Length of Activation Notification */
#define PHNCINFC_RFDISC_ACTVNTFMINLEN           (11U)
/* Length of 'Rf Disc ID' parameter of Intf actvd ntf */
#define PHNCINFC_RF_DISCOVERY_ID_LEN            (1U)
/** Length of 'Rf interface' parameter of Intf actvd ntf */
#define PHNCINFC_RF_INTFERFACE_LEN              (1U)
/** Length of 'Rf Procotol' parameter of Intf actvd ntf */
#define PHNCINFC_RF_PROTOCOL_LEN                (1U)
/** Length of 'Activated Rf Tech and mode' parameter of Intf actvd ntf */
#define PHNCINFC_ACTVD_RF_TECH_MODE_LEN         (1U)
/** Length of 'Max Data pkt payload size' parameter of Intf actvd ntf */
#define PHNCINFC_MAX_DATA_PKT_PAYLOAD_LEN       (1U)
/** Length of 'Initial No credits' parameter of Intf actvd ntf */
#define PHNCINFC_INITIAL_NO_CREDITS_LEN         (1U)
/** Length of 'Length of Rf Tech specific params' parameter of Intf actvd ntf */
#define PHNCINFC_RF_TECH_SPECIFIC_PARAMS_LEN    (1U)
/** Length of 'Data exchange Rf tech and mode' parameter of Intf actvd ntf */
#define PHNCINFC_DATA_XCNG_RF_TECH_MODE_LEN     (1U)
/** Length of 'Data exchange transmit bit rate' parameter of Intf actvd ntf */
#define PHNCINFC_DATA_XCNG_TX_BIT_RATE_LEN      (1U)
/** Length of 'Data exchange receive bit rate' parameter of Intf actvd ntf */
#define PHNCINFC_DATA_XCNG_RX_BIT_RATE_LEN      (1U)
/** Length of 'Length of Activation params' parameter of Intf actvd ntf */
#define PHNCINFC_ACTIVATION_PARAMS_LEN          (1U)

/** Max number of Rf Discovery Id's that can be present */
#define PHNCINFC_RF_DISCOVERY_ID_MAX            (254U)

/** Min Data packet payload size */
#define PHNCINFC_MIN_DATA_PKT_PAYLOAD_SIZE      (1U)
/** Data packet payload offset in Intf Actvd Ntf */
#define PHNCINFC_DATA_PKT_PAYLOAD_OFFSET        (4U)
/** Default Data packet payload size */
#define PHNCINFC_DATA_PKT_PAYLOAD_SIZE_DEFAULT  (0xFC)

/** Timer value used for invkoing temporary Ntf call back */
#ifdef DISABLE_NFCCX_DTA
#define PH_NCINFC_NTF_TIMEROUT                  (1000)
#else
#define PH_NCINFC_NTF_TIMEROUT                  (5000)
#endif

/* Dummy Destination ID for NFCC Loopback Mode */
#define PH_NCINFC_LOOPBACK_MODE_DESTID          (0xFE)

/* Max number of SE event registrations */
#define PHNCINFC_MAX_SE_EVENT_REGS               (2U)

typedef struct phNciNfc_ResetInfo
{
    uint8_t NciVer;                 /**< NCI Version supported by NFCC */
    phNciNfc_ResetType_t ResetTypeRsp; /**< Type of reset response */
    phNciNfc_ResetType_t ResetTypeReq; /**< Reset type requested */
    /*Add Last Reset Initiated by and Reason Code*/
}phNciNfc_ResetInfo_t, *pphNciNfc_ResetInfo_t; /**< pointer to #phNciNfc_ResetInfo_t */

typedef struct phNciNfc_InitInfo
{
    uint8_t bExtension[2];              /**< "Feature Enable" NCI2.0, 4.2, Table 8: Control Messages to Initialize the NFCC*/
    uint8_t bSkipRegisterAllNtfs;       /**< Skip registration of all ntfs as they are already registered */
}phNciNfc_InitInfo_t, *pphNciNfc_InitInfo_t; /**< pointer to #phNciNfc_InitInfo_t */

typedef struct phNciNfc_SendPayload
{
    void *pBuff;                    /**< Pointer to buffer where payload shall be stored */
    uint16_t wPayloadSize;          /**< Size of payload stored in the payload buffer */
}phNciNfc_SendPayload_t,*pphNciNfc_SendPayload_t; /**< pointer to #phNciNfc_SendPayload_t */

typedef struct phNciNfc_SetConfigOptInfo
{
    pphNciNfc_RfDiscConfigParams_t pSetConfParams;   /**< Upper layer set cfg param */
    pphNciNfc_RfDiscConfigParams_t pGetCfgParams;    /**< Get config param info */
}phNciNfc_SetConfigOptInfo_t;

typedef struct phNciNfc_RfConfigCtx
{
    uint8_t bReqParamNum;                           /**< Number of Parameters requested to NFCC */
    uint8_t bReqParamLen;                           /**< Length of Parameters requested to NFCC */
    _Field_size_bytes_(bReqParamLen + 1)
    uint8_t *pReqParamList;                         /**< Buffer is pre-pended with 1 byte that contains bReqParamNum */
}phNciNfc_RfConfigCtx_t,*pphNciNfc_RfConfigCtx_t; /**< pointer to #phNciNfc_RfConfigCtx_t*/

typedef struct phNciNfc_RtngConfInfo
{
    uint8_t  bMore;             /**< More number of notification are followed while reading routing table or
                                     more commands need to be sent while setting routing table */
    uint8_t  bRtngEntryOffset;  /**< Number of entries which have already been set */
    uint8_t  bTotalNumEntries;  /**< Total number of routing entries (applies for both set and get) */
    uint8_t* pRtngNtfPayload;   /**< Pointer to a buffer of size maximum routing table size for storing received
                                     routing table notification messages payload */
    uint16_t wPayloadSize;      /**< Total size of the routing table notification messages payload stored */
}phNciNfc_RtngConfInfo_t,*pphNciNfc_RtngConfInfo_t; /**< pointer to #phNciNfc_RtngConfInfo_t*/

typedef struct phNciNfc_RspBuffInfo
{
    uint8_t aBuff[260];         /**< Buffer to store received response payload */
    uint16_t wLen;              /**< Length of the payload */
}phNciNfc_RspBuffInfo_t, *pphNciNfc_RspBuffInfo_t; /**< pointer to #phNciNfc_RspBuffInfo_t */

typedef struct phNciNfc_ActiveDeviceInfo
{
    pphNciNfc_RemoteDevInformation_t pDevInfo;        /**< Active Device info as defined in specification*/
}phNciNfc_ActiveDeviceInfo_t, *pphNciNfc_ActiveDeviceInfo_t; /**< pointer to #phNciNfc_ActiveDeviceContext_t */

typedef enum phNciNfc_ExtnReqId
{
    phNciNfc_e_T1TXchgDataReq = 0x10,                   /**<T1T Raw Data Request from DH */
    phNciNfc_e_T1TWriteNReq = 0x20,                     /**<T1T N bytes write request from DH */
    phNciNfc_e_MfRawDataXchgHdr = 0x10,                 /**<MF Raw Data Request from DH */
    phNciNfc_e_MfWriteNReq = 0x31,                      /**<MF N bytes write request from DH */
    phNciNfc_e_MfReadNReq = 0x32,                       /**<MF N bytes read request from DH */
    phNciNfc_e_MfSectorSelReq = 0x33,                   /**<MF Block select request from DH */
    phNciNfc_e_MfPlusProxCheckReq = 0x28,               /**<MF+ Prox check request for NFCC from DH */
    phNciNfc_e_MfcAuthReq = 0x40,                       /**<MFC Authentication request for NFCC from DH */
    phNciNfc_e_InvalidReq                               /**<Invalid ReqId */
} phNciNfc_ExtnReqId_t ;

typedef enum phNciNfc_ExtnRespId
{
    phNciNfc_e_T1TXchgDataRsp = 0x10,                   /**<DH gets Raw data from T1T on successful req */
    phNciNfc_e_T1TWriteNRsp = 0x20,                     /**<DH gets write status */
    phNciNfc_e_MfXchgDataRsp = 0x10,                    /**<DH gets Raw data from MF on successful req */
    phNciNfc_e_MfWriteNRsp = 0x31,                      /**<DH gets write status */
    phNciNfc_e_MfReadNRsp = 0x32,                       /**<DH gets N Bytes read from MF, if successful */
    phNciNfc_e_MfSectorSelRsp = 0x33,                   /**<DH gets the “Sector Select” cmd status */
    phNciNfc_e_MfPlusProxCheckRsp = 0x29,               /**<DH gets the MF+ “Prox. Check” cmd status */
    phNciNfc_e_MfcAuthRsp = 0x40,                       /**<DH gets the “authenticate” cmd status */
    phNciNfc_e_InvalidRsp                               /**<Invalid RspId */
}phNciNfc_ExtnRespId_t ;

typedef struct phNciNfc_ExtnInfo
{
    union{
        phNciNfc_ExtnReqId_t      ExtnReqId;      /**< NCI extension reqid sent */
        phNciNfc_ExtnRespId_t     ExtnRspId;      /**< NCI extension respid received */
    }ActivExtnId;                                 /**< Active Req/Rsp Id */
    uint8_t              bParamsNumsPresent;      /**< Holds number of params available for this Req/Rsp Id */
    uint8_t              bParam[8];               /**< Req/Res: Param[0] = Param1, Param[1] = Param2 */
}phNciNfc_ExtnInfo_t;

typedef struct phNciNfc_TranscvContext
{
    phNciNfc_TransceiveInfo_t    tTranscvInfo;       /**<Remote device transaction info */
    pphNciNfc_TransreceiveCallback_t  pNotify;       /**< Upper layer CB */
    void                        *pContext;           /**< Caller's context */
    NFCSTATUS                   wPrevStatus;         /**< status of previously sent data */
    phNciNfc_Data_t             tSendPld;            /**< Payload (Header + Data)to be sent to NFCC */
    phNciNfc_Data_t             tRecvPld;            /**< Payload (Header + Data)received from NFCC */
    phNciNfc_ExtnInfo_t         tActiveExtn;         /**< NCI Extension corresponding to activated target */
    uint8_t                     bConnId;             /**< Logical connection ID*/
}phNciNfc_TranscvContext_t, *pphNciNfc_TranscvContext_t;/**< pointer to struct #phNciNfc_TranscvContext_t*/

typedef struct phNciNfc_LstnModeRecvInfo
{
    uint8_t bDataBuffEnable;   /**< Flag to identify whether any data is received and stored in the below
                                    buffer. (0 : No data received; 1 : Data received) */
    uint8_t *pBuff;            /**< Received data message is stored in this buffer. After extracting the buffer
                                    content, this buffer has to be de-allocated */
    uint16_t wBuffSize;       /**< Specifies size of content present in the buffer */
    NFCSTATUS wLstnCbStatus;  /**< Status returned by the call back function */
}phNciNfc_LstnModeRecvInfo_t;

typedef struct phNciNfc_RfDeActvInfo
{
    /*Upper layer Context for deactivate notif received call*/
    void                        *DeActvCtxt;
    pphNciNfc_Notification_t     pDeActvNotif;

}phNciNfc_RfDeActvInfo_t;

typedef struct phNciNfc_RegListenerInfo
{
    /*Upper layer Context for discovery call*/
    void                            *DiscoveryCtxt;
    pphNciNfc_Notification_t        pDiscoveryNotification;

    /*Upper layer Context for Nfcee call*/
    void                            *NfceeCtxt;
    pphNciNfc_Notification_t        pNfceeNotification;

    /*Upper layer Context for Generic error call*/
    void                            *GenericErrNtfCtxt;
    pphNciNfc_Notification_t        pGenericErrNtfCb;

    /*Upper layer Context for Reset Ntf call*/
    void                            *ResetNtfCtxt;
    pphNciNfc_Notification_t        pResetNtfCb;

}phNciNfc_RegListenerInfo_t;

typedef struct phNciNfc_RegForSyncNtf
{
    /**NCI discovery module shall register if Activated notification is expected */
    pphNciNfc_IfNotificationCb_t        pActvNtfCb;
    void                                *ActvNtfCtxt;

    /**NCI discovery module shall register if De-Activated notification is expected */
    pphNciNfc_IfNotificationCb_t        pDeActvNtfCb;
    void                                *DeActvNtfCtxt;

    /**NCI discovery module shall register if De-Activated notification is expected */
    pphNciNfc_IfNotificationCb_t        pT3tNtfCb;
    void                                *T3tNtfCtxt;

}phNciNfc_RegForSyncNtf_t;

typedef struct phNciNfc_Dest_Params
{
    uint8_t bDestParamType;    /**< Type of destination specific param */
    uint8_t bDestParamLen;     /**< destination specific param length */
    uint8_t bDestParamVal[2];  /**< destination specific param value */
}phNciNfc_Dest_Params_t, *pphNciNfc_Dest_Params_t;

#define PHNCINFC_DEST_PARAMS_INIT {0,0,{0,0}} /** Init value of \ref phNciNfc_Dest_Params_t */

typedef struct phNciNfc_Dest_Info
{
    phNciNfc_DestType_t     tDest;          /**< Destination type */
    uint8_t                 bNumDestParams; /**< Number of destination specific params */
    phNciNfc_Dest_Params_t  tDestParams;    /**< Destination specific params */
}phNciNfc_Dest_Info_t;

#define PHNCINFC_DEST_INFO_INIT {phNciNfc_e_UNKNOWN_DEST_TYPE,0,PHNCINFC_DEST_PARAMS_INIT}

typedef struct phNciNfc_LogConnContext
{
    pphNciNfc_IfNotificationCb_t IfLogConnNtf;  /**<Pointer to upper layer call back function*/
    void *IfLogConnNtfCtx;                      /**<Pointer to upper layer context*/
    uint8_t bConnId;                            /**<The connection id that is being used*/
    uint8_t bDestId;                            /**<The destination id for the connection id*/
    phNciNfc_DestType_t tDestType;              /**<The destination type for the connection id*/
}phNciNfc_LogConnContext_t;

typedef struct phNciNfc_SeEventInfo
{
    uint8_t bEnable;                                /**< Parameter used to identify if the entry is already occupied
                                                        (0: Not used; 1 used)*/
    void *pSeHandle;                                /**< Se handle passed by upper layer */
    void *pUpperLayerCtx;                           /**< Upper layer context */
    pphNciNfc_RegDataCb_t pUpperLayerCb;           /**< Upper layer call back function pointer */
}phNciNfc_SeEventInfo_t, *pphNciNfc_SeEventInfo_t;  /**< pointer to #phNciNfc_SeEventInfo_t*/

typedef struct phNciNfc_SeEventList
{
    phNciNfc_SeEventInfo_t aSeEventList[PHNCINFC_MAX_SE_EVENT_REGS];/**< Array of Se event information structures */
}phNciNfc_SeEventList_t, *pphNciNfc_SeEventList_t;  /**< pointer to #phNciNfc_SeEventList_t*/

extern NFCSTATUS phNciNfc_SetConnCredentials(void *psNciContext);

extern NFCSTATUS phNciNfc_ProcessIntfErrNtf(void *pContext, void *pInfo, NFCSTATUS wStatus);
extern NFCSTATUS phNciNfc_ProcessGenericErrNtf(void *pContext, void *pInfo, NFCSTATUS wStatus);
extern NFCSTATUS phNciNfc_ProcessGenericErrNtfMFC(void *pContext, void *pInfo, NFCSTATUS wStatus);

extern NFCSTATUS phNciNfc_DummyReadReq(void *pNciContext);
extern NFCSTATUS phNciNfc_ReceiveDataBufferCb(void* pContext, void *pInfo, NFCSTATUS wStatus);

extern NFCSTATUS phNciNfc_UpdateNtfList(pphNciNfc_DeviceInfo_t pDevInfo,
                                        pphNciNfc_RemoteDevInformation_t pRemoteDevInfo,
                                        uint8_t bUpdateList);

extern NFCSTATUS phNciNfc_GenericSequence(void *pNciCtx, void *pInfo, NFCSTATUS Status);
extern void phNciNfc_FreeSendPayloadBuff(void* pContext);
extern void phNciNfc_Notify(void* pContext, NFCSTATUS wStatus,void* pBuff);

extern NFCSTATUS phNciNfc_ReleaseNciHandle(void);
