/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcStatus.h>

#include <phNfcConfig.h>
#include "phNfcHalTypes2.h"
#include <phFriNfc_NdefRecord.h>
#include <phNfcLlcpTypes.h>
#include <phNfcConfig.h>

typedef void * phLibNfc_Handle;

/** \ingroup grp_lib_nfc
    Enum of initialization types */
typedef enum phLibNfc_InitType
{
    phLibNfc_InitType_Default = 0x00,  /**<Default initialization */
    phLibNfc_InitType_SkipConfig,      /**<Skip configuration after initialization */
    phLibNfc_InitType_ForceConfig,     /**<Force configuration after initialization */
} phLibNfc_InitType_t;

/** \ingroup grp_lib_nfc
    Ndef data offset in NDEF card */
typedef enum phLibNfc_Ndef_EOffset
{
    phLibNfc_Ndef_EBegin         = 0x01, /**< Start from the beginning position */
    phLibNfc_Ndef_ECurrent               /**< Start from the current position */
} phLibNfc_Ndef_EOffset_t;

/** \ingroup grp_lib_nfc
    Enum of secure element types */
typedef enum phLibNfc_SE_Type
{
    phLibNfc_SE_Type_Invalid     = 0x00, /**< SE type Invalid */
    phLibNfc_SE_Type_eSE         = 0x01, /**< SE type - eSE - connected with Wired Mode */
    phLibNfc_SE_Type_UICC        = 0x02, /**< SE type - UICC */
    phLibNfc_SE_Type_DeviceHost  = 0x03, /**< SE type - DeviceHost */
    phLibNfc_SE_Type_HciNwk      = 0x04  /**< Indicates HCI Network */
}phLibNfc_SE_Type_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Different Secure Element modes of operation
 *
 * \b On Mode:      Secure Element is turned on and can communicate with both the host and (if the
 *                  the routing table allows) the external reader.
 *
 * \b Off Mode:     Off mode is used when no communication with the Secure Element is needed.
 *                  Secure element does not communicate to either the host or an external reader
 *                  in this mode.
 */
typedef enum
{
    phLibNfc_SE_ActModeOn      = 0x00,
    phLibNfc_SE_ActModeOff     = 0x01,
}phLibNfc_eSE_ActivationMode;

/**
* \ingroup grp_nci_nfc
* \brief NFCEE Power and Link Control Info contains power related identifier..
*/
typedef enum phLibNfc_PowerLinkMode
{
    phLibNfc_PLM_NfccDecides = 0x00,                /** NFCEE  NFCC decides identifier*/
    phLibNfc_PLM_PowerSupplyAlwaysOn = 0x01,        /** NFCEE  power supply always on*/
    phLibNfc_PLM_ComLinkActiveWhenPowerOn = 0x02,   /** NFCEE  NFCC to NFCEE Communication link always active when the NFCEE is powered on*/
    phLibNfc_PLM_PowerAndComLinkAlwaysOn = 0x03,    /** NFCEE Power supply and NFCC to NFCEE communication link are always on*/
    phLibNfc_PLM_PowerLinkUnknown,                  /** Future or unknown identifier */
}phLibNfc_PowerLinkModes_t;

/** \ingroup grp_lib_nfc
    SE event information */
typedef union phLibNfc_uSeEvtInfo
{
    phNfc_sUiccInfo_t UiccEvtInfo; /**< Indicates UICC event information */
}phLibNfc_uSeEvtInfo_t;

/** \ingroup grp_lib_nfc
    SE transaction events sent to host */
typedef enum phLibNfc_eSE_EvtType
{
    phLibNfc_eSE_EvtTypeTransaction,           /**<Indication for external reader's attempt to
                                                   access secure element */
    phLibNfc_eSE_EvtConnectivity,              /**<Notifies the host that it shall
                                                   send a connectivity event from UICC as defined in
                                                   ETSI TS 102 622 V7.4.0 */
    phLibNfc_eSE_EvtReaderArrival,                   /**<External reader is detected and initial protocol handshake has occured */
    phLibNfc_eSE_EvtReaderDeparture,                  /**<External reader is no longer detected */
    phLibNfc_eSE_EvtRfFieldEnter,                  /**<External RF field is detected */
    phLibNfc_eSE_EvtRfFieldExit,                 /**<External RF field is no longer detected */
} phLibNfc_eSE_EvtType_t;

typedef phNfc_sSupProtocol_t    phLibNfc_Registry_Info_t;
typedef phNfc_sSupProtocol_t    phLibNfc_Discover_Info_t;

typedef phNfc_sData_t           phLibNfc_Data_t;
typedef phNfc_RtngConfig_t      phLibNfc_RtngConfig_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Configuration structure
 */
typedef struct phLibNfc_sConfig
{
    BitField_t bConfigOpt:1;
    BitField_t bHciNwkPerNfcee:1;
    BitField_t bNfceeActionNtf:1;
    BitField_t bIsoDepPresChkCmd:1;
    BitField_t bSwitchedOnSubState:1;
    BitField_t bLogNciDataMessages:1;
} phLibNfc_sConfig_t, *pphLibNfc_sConfig_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Defines supported tag types for NDEF mapping and formatting feature
 */
typedef struct SupportedTagInfo
{
    unsigned MifareUL:1;        /**<TRUE indicates specified feature (mapping
                                     or formatting)for MIFARE UL tag supported else not supported.*/
    unsigned MifareStd:1;       /**<TRUE indicates specified feature (mapping
                                     or formatting)for Mifare Std tag supported else not supported.*/
    unsigned MifareULC:1;       /**<TRUE indicates specified feature (mapping
                                     or formatting)for MIFARE UL2 tag supported else not supported.*/
    unsigned ISO14443_4A:1;     /**<TRUE indicates specified feature (mapping
                                     or formatting)for ISO14443_4A tag supported else not supported.*/
    unsigned ISO14443_4B:1;     /**<TRUE indicates specified feature (mapping
                                     or formatting)for ISO14443_4B tag supported else not supported.*/
    unsigned ISO15693:1;        /**<TRUE indicates specified feature (mapping
                                     or formatting)for ISO15693 tag supported else not supported.*/
    unsigned FeliCa:1;          /**<TRUE indicates specified feature (mapping
                                     or formatting)for FeliCa tag  supported else not supported.*/
    unsigned Jewel:1;           /**<TRUE indicates specified feature (mapping
                                     or formatting)for JEWEL tag supported else not supported.*/
    unsigned Desfire:1;         /**<TRUE indicates specified feature (mapping
                                     or formatting)for DESFire tag supported else not supported.*/
    unsigned Kovio : 1;         /**<TRUE indicates specified feature (mapping
                                     or formatting) for Kovio barcode tag supported else not supported.*/
}phLibNfc_sSupportedTagInfo_t;

typedef phLibNfc_sSupportedTagInfo_t  phLibNfc_sNDEFMappingInfo_t;
typedef phLibNfc_sSupportedTagInfo_t  phLibNfc_sTagFormattingInfo_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Stack capabilities details contains device capabilities and
 * supported tags for NDEF mapping and formatting feature
 */
typedef struct StackCapabilities
{
    phLibNfc_sDeviceCapabilities_t      psDevCapabilities;      /**<Device capabilities*/
    phLibNfc_sNDEFMappingInfo_t         psMappingCapabilities;  /**<NDEF support*/
    phLibNfc_sTagFormattingInfo_t       psFormatCapabilities;   /**<Tag formatting support*/
}phLibNfc_StackCapabilities_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Maximum number of secured elements
 */
#define PHLIBNFC_MAXNO_OF_SE 0x02

/**
 * \ingroup grp_lib_nfc
 * \brief Strucuture for secure element details
 */
typedef struct phLibNfc_SE_List
{
    phLibNfc_Handle             hSecureElement;     /**< handle to Secure Element */
    uint8_t                     hciHostId;          /**< Host ID in HCI network. */
    phLibNfc_SE_Type_t          eSE_Type;           /**< type of Secure Element(SE)*/
    phLibNfc_eSE_ActivationMode eSE_ActivationMode; /**< state of the secure element */
    phLibNfc_PowerLinkModes_t   eSE_PowerLinkMode;  /**< Power And Link mode of the secure element */
} phLibNfc_SE_List_t, *pphLibNfc_SE_List_t;

/**
 * \ingroup grp_lib_nfc
 * \brief Strucuture for secure element transceive
 */
typedef struct phNfc_sSeTransceiveInfo
{
    phNfc_sData_t               sSendData; /**Data buffer and data length to be sent*/
    phNfc_sData_t               sRecvData; /**Data buffer pointer and buffer lenght to receive data*/
    uint32_t                    timeout;   /**< specifies the timeout value (in msec) for SE transceive
                                                (use 0 to disable timer) */
}phNfc_sSeTransceiveInfo_t,*pphNfc_sSeTransceiveInfo_t;/**< Refer #phNfc_sSeTransceiveInfo_t */

/**
 * \ingroup grp_lib_nfc
 * \brief Target specific information obtained during the device discovery
 */
typedef struct phLibNfc_RemoteDev
{
    phLibNfc_Handle                   hTargetDev;       /**< discovered Target handle */
    phLibNfc_sRemoteDevInformation_t* psRemoteDevInfo;  /**< discovered Target details */
}phLibNfc_RemoteDevList_t;

/**
 * \ingroup grp_lib_nfc
 * \brief NDEF registration structure definition
 */
typedef struct phLibNfc_Ndef_SrchType
{
    uint8_t Tnf;        /**<  Type Name Format of this NDEF record */
    uint8_t *Type;      /**<  Type field of this NDEF record */
    uint8_t TypeLength; /**<  Length of the Type field of this NDEF record */
} phLibNfc_Ndef_SrchType_t;

/** \ingroup grp_lib_nfc
    NDEF information structure definition */
typedef struct phLibNfc_Ndef_Info
{
    uint32_t                NdefMessageLengthActual;   /**<Actual length of the NDEF message  */
    uint32_t                NdefMessageLengthMaximum;  /**<Maximum length of the NDEF message */
    uint8_t                 *pNdefMessage;             /**<Pointer to raw NDEF Data buffer    */
    uint32_t                NdefRecordCount;           /**<Number of NDEF records pointed by pNdefRecord */
    phFriNfc_NdefRecord_t   *pNdefRecord;              /**<Pointer to the NDEF Records contained
                                                          within the NDEF message */
} phLibNfc_Ndef_Info_t;

/**
 * \ingroup grp_lib_nfc
 * Structure for secure element Get ATR API
 */
typedef struct phNfc_SeAtr_Info
{
    uint8_t* pBuff; /**<To Store ATR Received from eSE*/
    uint32_t dwLength; /**<Actual Lenght of the ATR received:*/
} phNfc_SeAtr_Info_t, *pphNfc_SeAtr_Info_t; /**< Get ATR info */


/**
 *\ingroup grp_lib_nfc
 * \brief NDEF States
   As per NFC forum specification, the card can be in one of the below mentioned states
   INVALID - Card is NOT NFC forum specified tag. NDEF Format can be performed for
             the factory frash cards, which is in INVALID NDEF state, other cards
             may or may not be formatted
   INITIALISED - Card is NFC formatted tag, but it does not have any NDEF data. NDEF write
                 can be performed on this tag to write the NDEF data. After write tag is
                 in READ WRITE state
   READ WRITE - Card is NFC forum tag. READ and WRITE operations are permitted on this.
   READ ONLY - Card is NFC forum tag. Only READ operation is permitted, Write opreation is
               not possible
*/
#define PHLIBNFC_NDEF_CARD_INVALID                      0x00U   /**<card is NOT NFC forum specified tag*/
#define PHLIBNFC_NDEF_CARD_INITIALISED                  0x01U   /**<Ndef card initialised*/
#define PHLIBNFC_NDEF_CARD_READ_WRITE                   0x02U   /**<card is NFC forum specified tag. User can use both*/
#define PHLIBNFC_NDEF_CARD_READ_ONLY                    0x03U   /**<card is NFC forum specified tag. User can only use*/

/** \ingroup grp_lib_nfc
    NDEF Information structure */
typedef struct phLibNfc_ChkNdef_Info
{
    uint8_t   NdefCardState;                    /**< Card state information */
    uint32_t  ActualNdefMsgLength;              /**< Actual length of NDEF Message in the tag */
    uint32_t  MaxNdefMsgLength;                 /**< Maximum NDEF Message length that tag can hold*/
} phLibNfc_ChkNdef_Info_t;


typedef phFriNfc_LlcpMac_eLinkStatus_t phLibNfc_Llcp_eLinkStatus_t;
typedef  phFriNfc_Llcp_sLinkParameters_t  phLibNfc_Llcp_sLinkParameters_t;
typedef phFriNfc_LlcpTransport_eSocketType_t phLibNfc_Llcp_eSocketType_t;
typedef phFriNfc_LlcpTransport_sSocketOptions_t phLibNfc_Llcp_sSocketOptions_t;

/**
/**
 * \ingroup grp_lib_nfc
 * \brief Response callback for initialization
 *
 * This callback is used to indicate the LibNfc client, if initialization request is successful or failed
 *
 * \param[in] pContext           Context passed in the initialization request
 * \param[in] ConfigStatus       The reset configuration status from the controller
 * \param[in] status             Status of the response callback
 */
typedef void (*pphLibNfc_InitCallback_t)(
                void            *pContext,
                uint8_t         ConfigStatus,
                NFCSTATUS       Status
    );

/**
/**
 * \ingroup grp_lib_nfc
 * \brief Response callback for connect request
 *
 * This callback is used to indicate the LibNfc client, if connect request is successful or failed
 *
 * \param[in] pContext           Context passed in the connect request
 * \param[in] hRemoteDev         Handle to remote device with which the connect was requested
 * \param[in] psRemoteDevInfo    Contains updated remote device information (For few tags
 *                               some details like historical bytes are only updated after connect)
 * \param[in] status             Status of the response callback
 *                               - #NFCSTATUS_SUCCESS - Connect operation successful
 *                               - #NFCSTATUS_TARGET_LOST - Connect operation failed because target is lost
 *                               - #NFCSTATUS_SHUTDOWN - Shutdown in progress
 */
typedef void (*pphLibNfc_ConnectCallback_t)(
                void                                *pContext,
                phLibNfc_Handle                     hRemoteDev,
                phLibNfc_sRemoteDevInformation_t    *psRemoteDevInfo,
                NFCSTATUS                           Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Response callback for disconnect request
 *
 * This callback is used to indicate the LibNfc client, if disconnect request is successful or failed
 *
 * \param[in] pContext       Context passed in the disconnect request
 * \param[in] hRemoteDev     Handle to remote device with which disconnect was requested
 * \param[in] status         Status of the response callback
 *                           - #NFCSTATUS_SUCCESS - Disconnect operation successful
 *                           - #NFCSTATUS_SHUTDOWN - Shutdown in progress
 */
typedef void (*pphLibNfc_DisconnectCallback_t)(
                void*                pContext,
                phLibNfc_Handle      hRemoteDev,
                NFCSTATUS            Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Response callback for IOCTL request
 *
 * This callback is used to indicate the LibNfc client, if IOCTL request is successful or failed.
 * This callback may contain response data depending on the IOCTL command type issued.
 *
 * \param[in] pContext           Context passed in the connect request
 * \param[in,out] pOutParam      Command specific response buffer and size of the buffer.
 *                               This buffer address is same as pOutParam sent in #phLibNfc_Mgt_IoCtl
 * \param[in] status             Status of the response callback
 *                               - #NFCSTATUS_SUCCESS - IOCTL operation successful
 *                               - #NFCSTATUS_TARGET_LOST - IOCTL operation failed because target is lost
 *                               - #NFCSTATUS_SHUTDOWN - IOCTL operation failed because shutdown in progress
 */
typedef void (*pphLibNfc_IoctlCallback_t) (
                void*          pContext,
                phNfc_sData_t* pOutParam,
                NFCSTATUS      Status
    );

/**
 *\ingroup grp_lib_nfc
 * \brief Response callback for Transceive request
 *
 * This callback is used to provide received data to the LibNfc client in #phNfc_sData_t format.
 * This callback is called when LibNfc client has performed a Transceive operation on a tag
 * or the device acts as an Initiator during a P2P transactions
 *
 * \param[in] pContext       LibNfc client context passed in the corresponding request
 * \param[in] hRemoteDev     Handle to remote device which was used for transceive
 * \param[in] pResBuffer     Response buffer of type #phNfc_sData_t
 * \param[in] status         Status of the response callback
 *                - #NFCSTATUS_SUCCESS - Transceive operation  successful
 *                - #NFCSTATUS_TARGET_LOST - Transceive operation failed because target is lost
 *                - #NFCSTATUS_SHUTDOWN - Transceive operation failed because Shutdown in progress
 *                - #NFCSTATUS_ABORTED - Transceive aborted due to disconnect request in between
 *                -#NFCSTATUS_INSUFFICIENT_RESOURCES - Transceive operation failed because Insufficient resources
 */
typedef void (*pphLibNfc_TransceiveCallback_t)(
                void*               pContext,
                phLibNfc_Handle     hRemoteDev,
                phNfc_sData_t*      pResBuffer,
                NFCSTATUS           Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief SE Wait Time Extension callback.
 *
 * This callback is used to notifiy that the eSE has asked for a timeout extension, during a transceive call.
 *
 * \param[in] pContext           LibNfc client context passed in the SE notification register request
 * \param[in] hSecureElement     Handle to Secures Element
 * \param[in] pSeEvtInfo         The event arguments
 * \param[in] Status             Status of the response callback
 *              - #NFCSTATUS_SUCCESS - SE event is received
 *              - #NFCSTATUS_FAILED - Failed
 */
typedef void(*pphLibNfc_SE_WtxCallback_t)(
    void*                           pContext,
    phLibNfc_Handle                 hSecureElement,
    phLibNfc_sSeWtxEventInfo_t*     pSeEvtInfo,
    NFCSTATUS                       Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Generic response Callback definition
 *
 * Generic callback used as a callback for multiple APIs
 *
 * \note : Status and error codes for this type of callback are documented in respective APIs
 * wherever it is used
 *
 * \param[in] pContext LibNfc client context passed in the corresponding request
 * \param[in] status Status of the response callback
 */
typedef void(*pphLibNfc_RspCb_t) (void* pContext,NFCSTATUS  Status);

/**
 * \ingroup grp_lib_nfc
 * \brief Check NDEF callback definition
 *
 * This call back is used to provide the result of check NDEF request
 *
 * \note : Status and error codes for this type of callback are documented in API
 *
 * \param[in] pContext       LibNfc client context passed in the corresponding request
 * \param[in] Ndef_Info      Ndef message information
 * \param[in] status         Status of the response callback
 */
typedef void(*pphLibNfc_ChkNdefRspCb_t)(void*           pContext, \
                                        phLibNfc_ChkNdef_Info_t Ndef_Info,
                                        NFCSTATUS               Status);


/**
 * \ingroup grp_lib_nfc
 * \brief Notification handler callback
 *
 * This callback type is used to provide information on discovered targets to
 * LibNfcClient.  Discovered targets will be notified in #phLibNfc_RemoteDevList_t format.
 * In case multiple targets discovered, remote device list contains more than one targets
 *
 * \note List will be exported as memory block, based on \b uNofRemoteDev
 *      parameter
 *
 * \b E.g. Multiple targets discovered can be referred as phLibNfc_RemoteDevList_t[0] and
 * phLibNfc_RemoteDevList_t[1]
 *
 * Subsequent operations on discovered target shall be performed using target specific
 * handle \b hTargetDev
 *
 * \param[in] pContext        Client context passed in the corresponding request
 * \param[in] psRemoteDevList Remote Device list contains discovered target details
 *                            For details refer to #phLibNfc_RemoteDevList_t
 *                            List size depends on no of remote devices discovered
 * \param[in] uNofRemoteDev   Indicates number of remote devices discovered
 * \param[in] Status          Status of the response callback
 *                          - #NFCSTATUS_SUCCESS - Discovered single target successfully
 *                          - #NFCSTATUS_MULTIPLE_TAGS - Multiple targets found
 *                          - #NFCSTATUS_MULTIPLE_PROTOCOLS - Target found supports multiple protocols
 *                          - #NFCSTATUS_SHUTDOWN  - Failed because shutdown in progress
 *                          - #NFCSTATUS_DESELECTED - Target deselected
 */
typedef void (*phLibNfc_NtfRegister_RspCb_t)(
    void*                           pContext,
    phLibNfc_RemoteDevList_t*       psRemoteDevList,
    uint8_t                         uNofRemoteDev,
    NFCSTATUS                       Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Response Callback for secure element mode settings
 *
 * This callback type is used to provide information if request SE mode set is successful or not
 *
 * \param[in] pContext LibNfc client context     passed in the activation request.
 * \param[in] hSecureElement     Handle to secure element.
 * \param[in] Status Indicates API status.
 *                   - #NFCSTATUS_SUCCESS    Secure element set mode is successful
 *                   - #NFCSTATUS_SHUTDOWN   SE set mode failed because shutdown in progress
 *                   - #NFCSTATUS_FAILED     SE set mode failed
 */
typedef void(*pphLibNfc_SE_SetModeRspCb_t)(
    void*            pContext,
    phLibNfc_Handle  hSecureElement,
    NFCSTATUS        Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Notification callback for #phLibNfc_SE_NtfRegister().
 *
 * This callback is called when external reader access the SE.
 *
 * \param[in] pContext           LibNfc client context passed in the SE notification register request
 * \param[in] EventType          Event type of secure element transaction
 * \param[in] hSecureElement     Handle to Secures Element
 * \param[in] pAppID             Application identifier to be accessed on SE.
 *                               Sent when available from SE otherwise empty.
 * \param[in] Status             Indicates API status
 *              - #NFCSTATUS_SUCCESS - SE event is received
 *              - #NFCSTATUS_FAILED - Failed
 *
 *
 */
typedef void (*pphLibNfc_SE_NotificationCb_t) (void*                        pContext,
                                               phLibNfc_eSE_EvtType_t       EventType,
                                               phLibNfc_Handle              hSecureElement,
                                               phLibNfc_uSeEvtInfo_t*       pSeEvtInfo,
                                               NFCSTATUS                    Status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Receive response callback
 *
 * This callback type is used to provide received data to the
 * LibNfc client in #phNfc_sData_t format, when LibNfc client has performed
 * receive operation and the device acts as a Target during P2P communication
 *
 * \param[in] pContext                   LibNfc client context passed in the receive request
 * \param[in] pRecvBufferInfo            Response buffer of type #phNfc_sData_t
 * \param[in] status                     Status of the response callback
 *                  - #NFCSTATUS_SUCCESS - Receive operation successful
 *                  - #NFCSTATUS_SHUTDOWN - Receive operation failed because shutdown in progress
 *                  - #NFCSTATUS_ABORTED - Aborted due to initiator issued disconnect request
 */
typedef void (*pphLibNfc_Receive_RspCb_t)(void*              pContext,
                                          phNfc_sData_t*     pRecvBufferInfo,
                                          NFCSTATUS          status
    );

/**
 * \ingroup grp_lib_nfc
 * \brief NDEF Search Response callback
 *
 *  A function of this type is notified when registered NDEF type detected
 *
 * \param[in] pContext                    Pointer to context previously provided during the request
 * \param[in] psNdefInfo                  Ndef specific details of the remote device discovered
 * \param[in] hRemoteDevice               Handle to remote device on which NDEF detection is done
 * \param[in] Status                      Indicates callback status.
 *                  - #NFCSTATUS_SUCCESS - Indicates registered type detected            .
 *                  - #NFCSTATUS_SHUTDOWN - Indicates shutdown in progress
 *                  - #NFCSTATUS_FAILED - status failed
 *                  - #NFCSTATUS_ABORTED - Aborted due to disconnect operation in between
 */
typedef void (*pphLibNfc_Ndef_Search_RspCb_t)   ( void*                    pContext,
                                                  phLibNfc_Ndef_Info_t*    psNdefInfo,
                                                  phLibNfc_Handle          hRemoteDevice,
                                                  NFCSTATUS                Status
    );

/** \ingroup grp_lib_nfc
    LLCP check response callback definition */
typedef void (*pphLibNfc_ChkLlcpRspCb_t) ( void*      pContext,
                                           NFCSTATUS  status
                                           );

/** \ingroup grp_lib_nfc
    LLCP link status callback definition */
typedef void (*pphLibNfc_LlcpLinkStatusCb_t) ( void*                         pContext,
                                               phLibNfc_Llcp_eLinkStatus_t   eLinkStatus
                                               );


/** \ingroup grp_lib_nfc
    LLCP socket error notification callback definition */
typedef void (*pphLibNfc_LlcpSocketErrCb_t) ( void*      pContext,
                                              uint8_t    nErrCode
                                              );

/** \ingroup grp_lib_nfc
    Incoming connection on a listening LLCP socket callback definition */
typedef void (*pphLibNfc_LlcpSocketListenCb_t) ( void*            pContext,
                                                 phLibNfc_Handle  hIncomingSocket
                                                 );

/** \ingroup grp_lib_nfc
    LLCP socket connect callback definition */
typedef void (*pphLibNfc_LlcpSocketConnectCb_t) ( void*        pContext,
                                                  uint8_t      nErrCode,
                                                  NFCSTATUS    status
                                                  );

/** \ingroup grp_lib_nfc
    LLCP socket disconnect callback definition */
typedef void (*pphLibNfc_LlcpSocketDisconnectCb_t) ( void*        pContext,
                                                     NFCSTATUS    status
                                                     );

/** \ingroup grp_lib_nfc
    LLCP socket accept callback definition */
typedef void (*pphLibNfc_LlcpSocketAcceptCb_t) ( void*        pContext,
                                                 NFCSTATUS    status
                                                 );

/** \ingroup grp_lib_nfc
    LLCP socket reject callback definition */
typedef void (*pphLibNfc_LlcpSocketRejectCb_t) ( void*        pContext,
                                                 NFCSTATUS    status
                                                 );

/** \ingroup grp_lib_nfc
    LLCP socket receiption callback definition */
typedef void (*pphLibNfc_LlcpSocketRecvCb_t) ( void*     pContext,
                                               NFCSTATUS status
                                               );

/** \ingroup grp_lib_nfc
    LLCP socket reception with SSAP callback definition */
typedef void (*pphLibNfc_LlcpSocketRecvFromCb_t) ( void*       pContext,
                                                   uint8_t     ssap,
                                                   NFCSTATUS   status
                                                   );

/** \ingroup grp_lib_nfc
    LLCP socket emission callback definition */
typedef void (*pphLibNfc_LlcpSocketSendCb_t) ( void*        pContext,
                                               NFCSTATUS    status
                                               );

/**
 * \ingroup grp_lib_nfc
 * \brief Initializes the NFC library
 *
 * This function initializes LibNfc including underlying layers.
 * A session with NFC hardware will be established.
 *
 * \note Before successful initialization other NFC functionalities are not enabled.
 *
 * \param[in] pDriverHandle      The Driver Handle
 * \param[in] eInitType          The LIBNFC initialization type
 * \param[in] psConfig           Configuration parameters
 * \param[in] pInitCb            The init callback is called when initialization is
 *                               completed or an error occured in initialization
 * \param[in] pContext           Client context which will be included in
 *                               callback when the request is completed
 *
 * \retval #NFCSTATUS_ALREADY_INITIALISED     LibNfc is already initialized
 * \retval #NFCSTATUS_PENDING                 Init sequence has been successfully
 *                                            started and result will be conveyed via
 *                                            callback notification
 * \retval #NFCSTATUS_INVALID_PARAMETER       The parameter could not be properly
 *                                            interpreted
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES  Insufficient resource
 */
NFCSTATUS
phLibNfc_Mgt_Initialize (_In_ void *                    pDriverHandle,
                         _In_ phLibNfc_InitType_t       eInitType,
                         _In_ pphLibNfc_sConfig_t       psConfig,
                         _In_ pphLibNfc_InitCallback_t  pInitCb,
                         _In_ void *                    pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief De-initializes NFC library
 *
 * This function de-initializes and closes the current session with NFC hardware.
 * All configurations and setups done are invalidated to restart communication.
 * Resources currently used by LibNfc are released during De-initialization.
 *
 * \param[in]    pDriverHandle   The Driver Handle
 * \param[in]    pDeInitCb       De-initialization callback  is called by the LibNfc when init
 *                               completed or there is an error in de-initialization
 * \param[in]    pContext        Client context which will be included in
 *                               callback when the request is completed
 *
 * \retval #NFCSTATUS_SUCCESS     Device stack is already De-initialized
 * \retval #NFCSTATUS_PENDING     De-initialization sequence has been successfully
 *                                started and result will be conveyed via callback
 *                                notification.
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED     Indicates LibNfc is not yet initialized
 * \retval #NFCSTATUS_BUSY                Previous De-Init request in progress
 * \retval #NFCSTATUS_FAILED              Request failed
 */
NFCSTATUS
phLibNfc_Mgt_DeInitialize (_In_ void *                  pDriverHandle,
                           _In_ pphLibNfc_RspCb_t       pDeInitCb,
                           _In_ void *                  pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Function to Get list of available Secure Elements
 *
 * This function retrieves list of secure elements locally connected.
 *
 * LibNfc client shall pass empty list of size phLIBNFC_MAXNO_OF_SE.  On availability
 * of SE list, libNfc client can perform operation on specific SE using SE handle.
 *
 * The handle given in the #phLibNfc_SE_List_t structure stays valid until shutdown is
 * called.
 *
 * \note In case no SE's found, API still returns \ref #NFCSTATUS_SUCCESS with \b
 * uSE_count set to zero. Value zero indicates none of the SE's connected to NFC
 * hardware.
 *
 * \param[in,out] pSE_List       List of SEs with SE details in #phLibNfc_SE_List_t format
 * \param[in,out] uSE_count      Number of SEs in the list
 *
 * \note LibNfc client has to interpret number of secure elements in \b pSE_List based on this
 *       count.
 *
 * \retval    #NFCSTATUS_SUCCESS                  Operation is sucessful
 * \retval    #NFCSTATUS_SHUTDOWN                 Operation failed because shutdown in progress
 * \retval    #NFCSTATUS_NOT_INITIALISED          Operation failed because stack is not yet initialized
 * \retval    #NFCSTATUS_INVALID_PARAMETER        Invalid parameter
 */
NFCSTATUS phLibNfc_SE_GetSecureElementList(_Out_writes_to_(PHLIBNFC_MAXNO_OF_SE, *uSE_count) phLibNfc_SE_List_t*  pSE_List,
                                           _Out_range_(0, PHLIBNFC_MAXNO_OF_SE) uint8_t* uSE_count
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Sets secure element mode
 *
 * This function configures SE to different mode.
 *
 * -# If mode is #phLibNfc_SE_ActModeOn then external reader and LibNfc client can
 *    communicate with this SE.
 *
 * -# If mode is #phLibNfc_SE_ActModeOff This means SE is in off mode.
 *      \note This mode is applicable both UICC and SmartMX
 *
 * \param[in]  hSE_Handle           Secure element handle
 * \param[in]  eActivation_mode     SE mode to be configured
 * \param[in]  pSE_SetMode_Rsp_cb   Pointer to response callback
 * \param[in]  pContext             Client context which will be included in
 *                                  callback when the request is completed.
 *
 * \retval   #NFCSTATUS_PENDING             SE set mode transaction started
 * \retval   #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval   #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
 * \retval   #NFCSTATUS_INVALID_HANDLE      Invalid handle
 * \retval   #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval   #NFCSTATUS_REJECTED            Invalid request
 * \retval   #NFCSTATUS_FAILED              Request failed
 */
NFCSTATUS phLibNfc_SE_SetMode ( phLibNfc_Handle              hSE_Handle,
                                phLibNfc_eSE_ActivationMode  eActivation_mode,
                                pphLibNfc_SE_SetModeRspCb_t  pSE_SetMode_Rsp_cb,
                                void *                       pContext
    );

/**
* \ingroup grp_lib_nfc
* \brief Sets power and link control configuration for the eSE secure element type.
*
* This function configures SE to different power mode.
*
* -# If mode is set to #phLibNfc_SE_NfccDecides then the NFCC will be responsible for powering
* the eSE.
* -# If mode is set to #phLibNfc_SE_PowerSupplyAlwaysOn then the NFCC will keep the eSE power
* supply always on.
* -# If mode is set to #phLibNfc_SE_NfccComLinkActiveOnPowerOn then the NFCC will keep the eSE
* link always on.
* -# If mode is set to #phLibNfc_SE_PowerNfccLinkAlwaysOn then the NFCC will keep the eSE
* power supply and the link always on.
*
* \param[in]  hSE_Handle           Secure element handle
* \param[in]  ePowerAndLinkModes   eSE power and link modes
* \param[in]  pSE_PowerAndLinkControl_Rsp_cb   Pointer to response callback
* \param[in]  pContext             Client context which will be included in
*                                  callback when the request is completed.
*
* \retval   #NFCSTATUS_PENDING             SE power and link control transaction started
* \retval   #NFCSTATUS_SHUTDOWN            Shutdown in progress
* \retval   #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
* \retval   #NFCSTATUS_INVALID_HANDLE      Invalid handle
* \retval   #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
* \retval   #NFCSTATUS_REJECTED            Invalid request
* \retval   #NFCSTATUS_FAILED              Request failed
*/
NFCSTATUS phLibNfc_SE_PowerAndLinkControl(phLibNfc_Handle              hSE_Handle,
                                          phLibNfc_PowerLinkModes_t    ePowerAndLinkModes,
                                          pphLibNfc_RspCb_t            pSE_PowerAndLinkControl_Rsp_cb,
                                          void *                       pContext
    );

/**
* \ingroup grp_lib_nfc
* \brief This function shall be used to send and receive data from Secure element.
*
* \param[in]  hSE_Handle        Handle to secure element
* \param[in]  pSeTransceiveInfo pointer to #phNfc_sSeTransceiveInfo_t structure
* \param[in]  pTransceive_RspCb  The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_INVALID_PARAMETER    One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval #NFCSTATUS_PENDING              Reception operation is in progress,
*                                            pTransceive_RspCb will be called upon completion.
* \retval #NFCSTATUS_NOT_INITIALISED      Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN             Shutdown in progress.
* \retval #NFCSTATUS_FAILED               Operation failed.
* \retval #NFCSTATUS_MORE_INFORMATION     User buffer not sufficient to store received data.
*/
extern
NFCSTATUS phLibNfc_eSE_Transceive (phLibNfc_Handle                 hSE_Handle,
                                   phNfc_sSeTransceiveInfo_t       *pSeTransceiveInfo,
                                   pphLibNfc_TransceiveCallback_t  pTransceive_RspCb,
                                   void*                           pContext);

/**
* \ingroup grp_lib_nfc
* \brief This function shall be used to enumerate the SEs attached to controller.
*
* \param[in]  pSEDiscoveryCb     The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_INVALID_PARAMETER    One or more of the supplied parameters
*                                         could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE        Indicates stack is in a state other than initialized
 */
extern
NFCSTATUS phLibNfc_SE_Enumerate(pphLibNfc_RspCb_t pSEDiscoveryCb,
                                void* pContext
                                );

/**
 * \ingroup grp_lib_nfc
 *  \brief Registers notification handler to handle secure element specific events
 *
 *  This function registers handler to report SE specific transaction events.
 *  Possible different types of events are as defined in #phLibNfc_eSE_EvtType_t.

 * \param[in]  pSE_NotificationCb    Pointer to notification callback
 * \param[in]  pContext              Client context which will be included in
 *                                   callback when the request is completed.
 *
 * \retval  #NFCSTATUS_SUCCESS                 Registration sucessful
 * \retval  #NFCSTATUS_SHUTDOWN                Shutdown in progress
 * \retval  #NFCSTATUS_NOT_INITIALISED         LibNfc is not yet initialized
 * \retval  #NFCSTATUS_INVALID_PARAMETER       Invalid parameter
 * \retval  #NFCSTATUS_FAILED                  Request failed
 */
NFCSTATUS phLibNfc_SE_NtfRegister   (pphLibNfc_SE_NotificationCb_t  pSE_NotificationCb,
                                     void   *                       pContext
    );

/**
 * \ingroup grp_lib_nfc
 *  \brief Unregister the registered listener for SE event
 *
 * This function unregisters the listener which  has been registered with \ref
 * phLibNfc_SE_NtfRegister.
 *
 * \retval  #NFCSTATUS_SUCCESS                 Unregistration successful
 * \retval  #NFCSTATUS_SHUTDOWN                Shutdown in progress
 * \retval  #NFCSTATUS_NOT_INITIALISED         LibNfc is not yet initialized
 * \retval  #NFCSTATUS_FAILED                  Request failed
 */
NFCSTATUS phLibNfc_SE_NtfUnregister(void);

/**
 * \ingroup grp_lib_nfc
 *  \brief IOCTL interface
 *
 * The I/O Control function allows the caller to configure specific
 * functionality provided by the lower layer.
 *
 * \param[in] pDriverHandle Driver handle (This parameter is valid only for firmware
 *                          download feature. This parameter is not relevent for
 *                          other IOCTL features
 * \param[in] IoctlCode     Control code for the operation. This value identifies the
 *                          specific operation to be performed. For more details on supported
 *                          IOCTL codes refer to grp_lib_ioctl.
 * \param[in,out] pInParam  Pointer to any input data structure containing data which is
 *                          interpreted based on IOCTL code and the length of the data.
 * \param[in,out] pOutParam Pointer to output buffer details to hold IOCTL specific
 *                          response buffer. This buffer will be updated and sent back
 *                          as part of callback.
 * \param[in] pIoCtl_Rsp_cb Response callback registered by the LibNfc Client
 * \param[in] pContext      Client context which will be included in callback when the request
 *                          is completed
 *
 * \retval #NFCSTATUS_PENDING           Request is in pending state, callback will be called later
 * \retval #NFCSTATUS_INVALID_PARAMETER Invalid parameter
 * \retval #NFCSTATUS_BUFFER_TOO_SMALL  The buffer provided by the caller is to small
 * \retval #NFCSTATUS_SHUTDOWN          Shutdown in progress
 * \retval #NFCSTATUS_NOT_INITIALISED   LibNfc is not yet initialized.
 * \retval #NFCSTATUS_NOT_ALLOWED       Requested operation is not allowed
 */
NFCSTATUS phLibNfc_Mgt_IoCtl    (void*                      pDriverHandle,
                                 uint16_t                   IoctlCode,
                                 phNfc_sData_t*             pInParam,
                                 phNfc_sData_t*             pOutParam,
                                 pphLibNfc_IoctlCallback_t  pIoCtl_Rsp_cb,
                                 void*                      pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Function to register notification handler for target discovery
 *
 * This function allows libNfc client to register for notifications based technology type
 * it is interested to discover. In case LibNfc client is interested in multiples technology
 * discovery, it can enable respective bits in \b pRegistryInfo. when Registered type
 * target is discovered in RF field, LibNfc notifies registered notification callback.
 *
 * \note If this API is called multiple times, most recent request registry details
 *       is used for registration.
 *
 * \param[in] pRegistryInfo        Structure contains bitwise registry information. Specific
 *                                 technology type discovery can be registered if
 *                                 corresponding bit is enabled.
 * \param[in] pNotificationHandler Pointer to notification callback. This callback will be notified
 *                                 on registered target discovered.
 * \param[in] pContext             Client context which will be included in callback when the request
 *                                 is completed.
 *
 * \retval #NFCSTATUS_SUCCESS           Registration successful
 * \retval #NFCSTATUS_INVALID_PARAMETER Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED   LibNfc is not yet initialized.
 * \retval #NFCSTATUS_SHUTDOWN          Shutdown in progress
 */
NFCSTATUS   phLibNfc_RemoteDev_NtfRegister(
    phLibNfc_Registry_Info_t*       pRegistryInfo,
    phLibNfc_NtfRegister_RspCb_t    pNotificationHandler,
    void*                           pContext
    );

/**
 * \ingroup grp_lib_nfc
 *  \brief Configure Discovery
 *
 * This function is used to configure and start the discovery (Polling loop configuration and
 * starting). It includes
 *   -# Enabling/disabling of Reader phases for all supported card technologies
 *   -# Configuring NFC-IP1 Initiator
 *   -# Duration of the Card Emulation phase
 *
 * Polling loop configuration based on discovery mode selected is as below
 *
 *   -# If discovery Mode is set as #NFC_DISCOVERY_CONFIG then previous configurations
 *      over written by new configurations passed in #phNfc_sADD_Cfg_t and
 *      Polling loop restarts with new configurations
 *   -# If discovery Mode is set as #NFC_DISCOVERY_START,
 *      then discovery parameters passed in #phNfc_sADD_Cfg_t is not be
 *      considered and old configurations are used
 *   -# If discovery Mode is set as #NFC_DISCOVERY_STOP discovery mode stops the
 *      polling loop even though the discovery is ongoing
 *
 * \param[in]    DiscoveryMode           Discovery Mode allows to choose between
 *                                       discovery configuration, start or stop
 * \param[in]    sADDSetup               Includes Enable/Disable discovery for
 *                                       each supported protocol
 * \param[in]    pConfigDiscovery_RspCb  Callback is called when the discovery wheel
 *                                       configuration is complete
 * \param[in]    pContext                Client context which will be included in
 *                                       callback when the request is completed
 *
 * \retval #NFCSTATUS_PENDING                  Discovery request is in progress and result
 *                                             will be notified through callback
 * \retval #NFCSTATUS_INVALID_PARAMETER        Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED          LibNfc is not initialized
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES   Insufficient resource
 * \retval #NFCSTATUS_BUSY                     discovery is in progress or already
 *                                             discovered a Target and it is connected
 * \retval #NFCSTATUS_FAILED                   Request failed
 */
NFCSTATUS phLibNfc_Mgt_ConfigureDiscovery (phLibNfc_eDiscoveryConfigMode_t  DiscoveryMode,
                                           phLibNfc_sADD_Cfg_t              sADDSetup,
                                           pphLibNfc_RspCb_t                pConfigDiscovery_RspCb,
                                           void*                            pContext
    );


/**
 * \ingroup grp_lib_nfc
 * \brief Connect to Remote device
 *
 * This function is called to connect to discovered target.
 * Once notification handler notified successfully discovered targets, those will be available in
 * #phLibNfc_RemoteDevList_t. Remote device list contains valid handles for discovered
 * targets. LibNfc client can connect to one out of the discovered targets using this function.
 *
 * A new session is started after connect operation is successful. The session ends with a
 * successful disconnect operation. Connect operation on an already connected tag reactivates
 * the Tag. This feature is not valid for Jewel/Topaz Tags, and hence a second connect if issued
 * without disconnecting a Jewel/Topaz tag always fails.
 *
 * \note In case multiple targets discovered, LibNfc client can connect to only one target.
 *
 * \param[in]     hRemoteDevice        Handle of the target device obtained in discovery
 * \param[in]     pNotifyConnect_RspCb Client response callback which will be
 *                                     notified to indicate status of the request
 * \param[in]     pContext             Client context which will be included in
 *                                     callback when the request is completed
 *
 * \retval #NFCSTATUS_PENDING           Request initiated, result will be informed in callback
 * \retval #NFCSTATUS_INVALID_PARAMETER Invalid parameter
 * \retval #NFCSTATUS_TARGET_LOST       Indicates target is lost
 * \retval #NFCSTATUS_SHUTDOWN          Shutdown in progress
 * \retval #NFCSTATUS_NOT_INITIALISED   LibNfc is not yet initialized
 * \retval #NFCSTATUS_INVALID_HANDLE    Target handle is invalid
 * \retval #NFCSTATUS_FAILED            Request failed
 */
NFCSTATUS phLibNfc_RemoteDev_Connect(phLibNfc_Handle                hRemoteDevice,
                                     pphLibNfc_ConnectCallback_t    pNotifyConnect_RspCb,
                                     void*                          pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Communication with remove device
 *
 * This function allows to send data to and receive data
 * from the target selected by LibNfc client while acting as Reader.
 * It is also used by the NFCIP1 Initiator while performing a transaction with
 * the NFCIP1 target. The LibNfc client has to provide the handle of the
 * target and the command in order to communicate with the selected remote device.
 *
 * \param[in] hRemoteDevice      Handle of the connected remote device.
 * \param[in] psTransceiveInfo   Structure of the information required by transceive.
 *                               It contains send,receive buffers and command specific details
 * \param[in] pTransceive_RspCb  Callback function for the received response or error
 * \param[in] pContext           Client context which will be included in
 *                               callback when the request is completed
 *
 * \retval #NFCSTATUS_PENDING               Request initiated, result will be informed through
 *                                          the callback
 * \retval #NFCSTATUS_INVALID_PARAMETER     Invalid parameter
 * \retval #NFCSTATUS_COMMAND_NOT_SUPPORTED The command is not supported
 * \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress
 * \retval #NFCSTATUS_TARGET_LOST           Target is lost
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED  The Remote Device is not connected
 * \retval #NFCSTATUS_INVALID_HANDLE        Target handle is invalid
 * \retval #NFCSTATUS_NOT_INITIALISED       LibNfc is not yet initialized
 * \retval #NFCSTATUS_REJECTED              Invalid request
 * \retval #NFCSTATUS_FAILED                Request failed
 */
NFCSTATUS phLibNfc_RemoteDev_Transceive(phLibNfc_Handle                 hRemoteDevice,
                                        phLibNfc_sTransceiveInfo_t*     psTransceiveInfo,
                                        pphLibNfc_TransceiveCallback_t  pTransceive_RspCb,
                                        void*                           pContext
    );

/**
 * \ingroup grp_lib_nfc
 *  \brief Disconnect from already connected target
 *
 *  The function allows to disconnect from already connected target. This
 *  function closes the session opened during connect operation. The status of the polling
 *  loop after disconnection is determined by the #phNfc_eReleaseType_t parameter.
 *  It is also used to switch from wired to virtual mode in case the discovered
 *  device is SmartMX in wired mode.
 *
 * \param[in] hRemoteDevice    Handle of the target device
 * \param[in] ReleaseType      Release mode to be used while disconnecting from target.
 * \param[in] pDscntCallback   Client response callback which will be notified to indicate
 *                             status of the request
 * \param[in] pContext         Client context which will be included in callback when the request
 *                             is completed
 *
 * \retval #NFCSTATUS_PENDING                Request initiated, result will be informed
 *                                           through the callback
 * \retval #NFCSTATUS_INVALID_PARAMETER      Invalid parameter
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED   Remote Device is not connected
 * \retval #NFCSTATUS_NOT_INITIALISED        LibNfc is not yet initialized
 * \retval #NFCSTATUS_INVALID_HANDLE         Target handle is invalid
 * \retval #NFCSTATUS_SHUTDOWN               Shutdown in progress
 * \retval #NFCSTATUS_REJECTED               Request rejected
 * \retval #NFCSTATUS_BUSY                   Cannot disconnect due to outstanding transaction in progress
 * \retval #NFCSTATUS_FAILED                 Request failed
 */
NFCSTATUS phLibNfc_RemoteDev_Disconnect( phLibNfc_Handle                 hRemoteDevice,
                                         phLibNfc_eReleaseType_t         ReleaseType,
                                         pphLibNfc_DisconnectCallback_t  pDscntCallback,
                                         void*                           pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Unregister notification handler
 *
 * This  function unregisters the listener which has been registered with
 * #phLibNfc_RemoteDev_NtfUnregister. After this function call the callback
 * function for target discovery notification will not be called.
 * If no callback is registered then also call to this function succeeds.

 * \retval #NFCSTATUS_SUCCESS          Callback unregistered
 * \retval #NFCSTATUS_SHUTDOWN         Shutdown in progress
 * \retval #NFCSTATUS_NOT_INITIALISED  LibNfc is not yet initialized
 */
NFCSTATUS phLibNfc_RemoteDev_NtfUnregister(void);

/**
 * \ingroup grp_lib_nfc
 *  \brief Check for target presence
 *
 * This  function checks, if connected target is present in RF filed or not.
 * Client can make use of this API to check periodically check if target is
 * still in the field or not.
 *
 * \param[in] hRemoteDevice         Handle of the connected target device.
 * \param[in] pPresenceChk_RspCb    Callback function called on completion of the
 *                                  presence check or in case an error
 * \param[in] pContext              Client context which will be included in
 *                                  callback when the request is completed
 *
 * \retval  #NFCSTATUS_PENDING              Presence check  started, status will be notified
 *                                          through callback
 * \retval  #NFCSTATUS_NOT_INITIALISED      LibNfc is not initialized
 * \retval  #NFCSTATUS_INVALID_PARAMETER    Invalid parameter
 * \retval  #NFCSTATUS_TARGET_NOT_CONNECTED Remote Device is not connected
 * \retval  #NFCSTATUS_INVALID_HANDLE       Target handle is invalid
 * \retval  #NFCSTATUS_SHUTDOWN             Shutdown in progress
 * \retval  #NFCSTATUS_FAILED               Request failed
 *
 */
NFCSTATUS phLibNfc_RemoteDev_CheckPresence( phLibNfc_Handle     hRemoteDevice,
                                            pphLibNfc_RspCb_t    pPresenceChk_RspCb,
                                            void*                pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Check NDEF
 *
 * This function checks if connected tag is NDEF compliant or not.
 *
 * \param[in] hRemoteDevice      Handle of the connected remote device
 * \param[in] pCheckNdef_RspCb   Response callback. This will be called when check NDEF
 *                               process completes
 * \param[in] pContext           Client context which will be included in
 *                               callback when the request is completed
 *
 * \retval #NFCSTATUS_PENDING                   Check NDEF started, status will be notified
 *                                              through callback
 * \retval #NFCSTATUS_INVALID_PARAMETER         Invalid parameter
 * \retval #NFCSTATUS_TARGET_LOST               Target is lost
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED      Remote Device is not connected
 * \retval #NFCSTATUS_INVALID_HANDLE            Target handle is invalid
 * \retval #NFCSTATUS_SHUTDOWN                  Shutdown in progress
 * \retval  #NFCSTATUS_FAILED                   Request failed
 */
NFCSTATUS phLibNfc_Ndef_CheckNdef(phLibNfc_Handle              hRemoteDevice,
                                  pphLibNfc_ChkNdefRspCb_t     pCheckNdef_RspCb,
                                  void*                        pContext);

/**
 * \ingroup grp_lib_nfc
 * \brief  Read NDEF  message from an NDEF compliant Tag
 *
 * This  function reads an NDEF message from connected tag. This function can read the NDEF
 * message from start or can continue from previous read.
 * If the call returns with #NFCSTATUS_PENDING, a response callback pNdefRead_RspCb is
 * called, when the read operation is complete.
 *
 * \note Before issuing NDEF read operation LibNfc client should perform NDEF check operation
 * using #phLibNfc_Ndef_CheckNdef function. If the call back error code is #NFCSTATUS_FAILED
 * then the LIBNFC client has to do the phLibNfc_RemoteDev_CheckPresence to find, it is a
 * communication error or target lost.
 *
 * \param[in]  hRemoteDevice         Handle of the connected remote device
 * \param[in]  psRd                  Pointer to  the read buffer info
 * \param[in]  Offset                Reading Offset * phLibNfc_Ndef_EBegin - Read from
 *                                   beginning phLibNfc_Ndef_ECurrent - Read from current offset
 * \param[in]  pNdefRead_RspCb       Response callback defined by the caller
 * \param[in]  pContext              Client context which will be included in
 *                                   callback when the request is completed
 *
 * \retval #NFCSTATUS_SUCCESS             NDEF read operation successful
 * \retval #NFCSTATUS_PENDING             Request accepted and started operation
 * \retval #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval #NFCSTATUS_INVALID_HANDLE      Target handle is invalid
 * \retval #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized.
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED   Remote Device is not connected
 * \retval #NFCSTATUS_FAILED              Read operation failed since tag does not contain NDEF data
 * \retval #NFCSTATUS_NON_NDEF_COMPLIANT  Tag is not NDEF Compliant
 * \retval #NFCSTATUS_REJECTED            Request rejected
 */
NFCSTATUS phLibNfc_Ndef_Read(phLibNfc_Handle                   hRemoteDevice,
                             phNfc_sData_t*                     psRd,
                             phLibNfc_Ndef_EOffset_t            Offset,
                             pphLibNfc_RspCb_t                  pNdefRead_RspCb,
                             void*                              pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Write  NDEF data to NDEF tag
 *
 * This function allows the LibNfc client to write a NDEF data to already connected NFC tag.
 * Function writes a complete NDEF  message to a tag. If a NDEF message already
 * exists in the tag, it will be overwritten. When the transaction is complete,
 * a notification callback is notified.
 *
 * \note Before issuing NDEF write operation LibNfc client should perform NDEF check operation
 * using #phLibNfc_Ndef_CheckNdef interface.
 *
 * \param[in] hRemoteDevice          Handle of the connected remote device
 * \param[in] psWr                   NDEF Buffer to write. If NdefMessageLen is set to 0
 *                                   and pNdefMessage is NULL, This function will erase
 *                                   tag
 * \param[in] pNdefWrite_RspCb       Response callback
 * \param[in] pContext               Client context which will be included in
 *                                   callback when the request is completed
 *
 * If the call back error code is #NFCSTATUS_FAILED then the LibNfc client has to do the
 * phLibNfc_RemoteDev_CheckPresence to find, its communication error or target lost
 *
 * \retval #NFCSTATUS_PENDING             Request accepted and started
 * \retval #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval #NFCSTATUS_INVALID_HANDLE      Target handle is invalid
 * \retval #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_NON_NDEF_COMPLIANT  Tag is not NDEF Compliant
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED  Remote Device is not connected
 * \retval #NFCSTATUS_REJECTED            Request rejected
 * \retval #NFCSTATUS_FAILED              operation failed
 */
NFCSTATUS phLibNfc_Ndef_Write (phLibNfc_Handle          hRemoteDevice,
                               phNfc_sData_t*           psWr,
                               pphLibNfc_RspCb_t        pNdefWrite_RspCb,
                               void*                    pContext
    );

/**
 * \ingroup grp_lib_nfc
 *  \brief NDEF Format target
 *
 * This function allows the LibNfc client to perform NDEF formating operation on connected target.
 *
 * \note
 *    -# Prior to formating it is recommended to perform NDEF check using #phLibNfc_Ndef_CheckNdef interface
 *    -# Format feature is supported only for MIFARE Std, MIFARE UL/ULC and DESFire tag types
 *    -# If the callback error code is #NFCSTATUS_FAILED then the LIBNFC client has to do the
 *       phLibNfc_RemoteDev_CheckPresence to find its communication error or target lost
 *
 * \param[in] hRemoteDevice          Handle of the connected remote device
 * \param[in] pScrtKey               Information containing the secret key data
 *                                   and Secret key buffer length
 * \param[in] pNdefformat_RspCb      Response callback defined by the caller
 * \param[in] pContext               Client context which will be included in
 *                                   callback when the request is completed
 *
 * \retval #NFCSTATUS_PENDING                 Request accepted and started
 * \retval #NFCSTATUS_SHUTDOWN                Shutdown in progress
 * \retval #NFCSTATUS_INVALID_HANDLE          Target handle is invalid
 * \retval #NFCSTATUS_NOT_INITIALISED         LibNfc is not yet initialized
 * \retval #NFCSTATUS_INVALID_PARAMETER       Invalid parameter
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED    Remote Device is not connected
 * \retval #NFCSTATUS_FAILED                  operation failed
 * \retval #NFCSTATUS_REJECTED                Tag is already formatted
 */
NFCSTATUS phLibNfc_RemoteDev_FormatNdef(phLibNfc_Handle         hRemoteDevice,
                                        phNfc_sData_t*          pScrtKey,
                                        pphLibNfc_RspCb_t       pNdefformat_RspCb,
                                        void*                   pContext
    );

/**
* \ingroup grp_lib_nfc
*
* \brief To convert a already formatted NDEF READ WRITE tag to READ ONLY.
*
* This function allows the LibNfc client to convert a already formatted NDEF READ WRITE
* tag to READ ONLY on discovered target.
*
*\note
* <br>1. Prior to formating it is recommended to perform NDEF check using \ref phLibNfc_Ndef_CheckNdef interface.
* <br>2. READ ONLY feature supported only for MIFARE UL and Desfire tag types.
* If the call back error code is NFCSTATUS_FAILED then the LIBNFC client has to do the
* phLibNfc_RemoteDev_CheckPresence to find, its communication error or target lost.
*
* \param[in] hRemoteDevice          handle of the remote device.This handle to be
*                                   same as as handle obtained for specific remote device
*                                   during device discovery.
* \param[in] pScrtKey               Key to be used for making Mifare read only. This parameter is
*                                   unused in case of readonly for other cards.
* \param[in] pNdefReadOnly_RspCb    Response callback defined by the caller.
* \param[in] pContext               Client context which will be included in
*                                   callback when the request is completed.
*
* \retval NFCSTATUS_PENDING                 Request accepted and started.
* \retval NFCSTATUS_SHUTDOWN                Shutdown in progress.
* \retval NFCSTATUS_INVALID_HANDLE          Target  handle is invalid.
* \retval NFCSTATUS_NOT_INITIALISED         Indicates stack is not yet initialized.
* \retval NFCSTATUS_INVALID_PARAMETER       One or more of the supplied parameters could not
*                                           be  properly interpreted.
* \retval NFCSTATUS_TARGET_NOT_CONNECTED    The Remote Device is not connected.
* \retval NFCSTATUS_FAILED                  operation failed.
* \retval NFCSTATUS_REJECTED                Tag is already  formatted one.
*/
NFCSTATUS phLibNfc_ConvertToReadOnlyNdef(phLibNfc_Handle        hRemoteDevice,
                                         phNfc_sData_t*         pScrtKey,
                                         pphLibNfc_RspCb_t      pNdefReadOnly_RspCb,
                                         void*                  pContext
                                         );

/**
 * \ingroup grp_lib_nfc
 * \brief Search for NDEF Record type
 *
 * This function allows LibNfc client to search NDEF content based on TNF value and type
 *
 * This API allows to find NDEF records based on RTD (Record Type Descriptor) info
 * LibNfc internally parses NDEF content based registration type registered. In case
 * there is match, LibNfc client is notified with NDEF information details.  LibNfc
 * client can search a new NDEF registration type once the previous call is handled.
 *
 * \param[in]     hRemoteDevice       Handle of the connected remote device
 * \param[in]     psSrchTypeList      List of NDEF records to be looked in based on TNF value and type.
 *                                    For NDEF search type refer to #phLibNfc_Ndef_SrchType.
 *                                    If this set to NULL then it means that libNfc client interested in
 *                                    all possible NDEF records
 * \param[in]     uNoSrchRecords      Indicates number of NDEF records in requested list as mentioned
 *                                    in psSrchTypeList
 * \param[in]     pNdefNtfRspCb       Response callback defined by the caller
 * \param[in]     pContext            Client context which will be included in
 *                                    callback when callback is notified
 *
 *
 * \retval #NFCSTATUS_SUCCESS             NDEF record search successful
 * \retval #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
 * \retval #NFCSTATUS_INVALID_HANDLE      Target handle is invalid
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_TARGET_NOT_CONNECTED  Remote Device is not connected
 * \retval #NFCSTATUS_FAILED              operation failed
 * \retval #NFCSTATUS_BUSY                Previous request in progress can not accept new request
 * \retval #NFCSTATUS_ABORTED             Aborted due to disconnect request in between
 */
NFCSTATUS phLibNfc_Ndef_SearchNdefContent(
    phLibNfc_Handle                 hRemoteDevice,
    phLibNfc_Ndef_SrchType_t*       psSrchTypeList,
    uint8_t                         uNoSrchRecords,
    pphLibNfc_Ndef_Search_RspCb_t   pNdefNtfRspCb,
    void *                          pContext
    );

/**
 * \ingroup grp_lib_nfc
 *  \brief Receive data from initiator at target side during P2P communication
 *
 * This function is used by P2P target to receives data coming from the
 * Initiator. Once this function is called by LibNfc client on target side it waits for
 * receiving data from initiator.
 *
 * \note Once this function is called, its mandatory to wait for receive. The data is received through \ref
 *       pphLibNfc_Receive_RspCb_t callback notification. Only function which is allowed
 *       before callback is #phLibNfc_Mgt_DeInitialize.
 *
 * \param[in] hRemoteDevice       Peer handle
 * \param[in] pReceiveRspCb       Callback function called after receiving the data or error
 * \param[in] pContext            Upper layer context to be returned in the callback
 *
 * \retval #NFCSTATUS_PENDING            Receive operation is in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER  Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED    LibNfc is not yet initialized
 * \retval #NFCSTATUS_SHUTDOWN           Shutdown in progress
 * \retval #NFCSTATUS_INVALID_DEVICE     Invalid device
 * \retval #NFCSTATUS_DESELECTED         Receive operation is not possible due to
 *                                       initiator issued disconnect or intiator
 *                                       physically removed from the RF field
 * \retval #NFCSTATUS_REJECTED           Indicates invalid request
 * \retval #NFCSTATUS_FAILED             Request failed
 */
extern
NFCSTATUS
phLibNfc_RemoteDev_Receive( phLibNfc_Handle            hRemoteDevice,
                            pphLibNfc_Receive_RspCb_t   pReceiveRspCb,
                            void*                       pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Send data to initiator from target during P2P communication
 *
 * This function sends data from P2P target to initiator in response to packet
 * received from initiator during P2P communication. This function is used only after
 * receving data from initiator. In other words this function is called only after calling
 * #phLibNfc_RemoteDev_Receive function.
 *
 * \param[in] hRemoteDevice             Peer handle
 * \param[in] pTransferData             Data and the length of the data to be transferred
 * \param[in] pSendRspCb                Callback function called on completion of send or
 *                                      in case of an error
 * \param[in] pContext                  Upper layer context to be returned in the callback
 *
 * \retval #NFCSTATUS_PENDING             Send operation is in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
 * \retval #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval #NFCSTATUS_INVALID_DEVICE      Invalid device
 * \retval #NFCSTATUS_BUSY                Previous request in progress cannot accept new request
 * \retval #NFCSTATUS_DESELECTED          Send operation is not possible due to
 *                                        initiator issued disconnect or intiator
 *                                        physically removed from the RF field
 * \retval   #NFCSTATUS_REJECTED          Invalid request
 * \retval   #NFCSTATUS_FAILED            Request failed
 */
extern
NFCSTATUS
phLibNfc_RemoteDev_Send(phLibNfc_Handle             hRemoteDevice,
                        phNfc_sData_t*              pTransferData,
                        pphLibNfc_RspCb_t           pSendRspCb,
                        void*                       pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Interface to configure P2P
 *
 * The setting will be typically take effect for the next cycle of the
 * discovery. For optional configuration internal defaults will be
 * used in case the configuration is not set.
 *
 * \param[in] pConfigInfo           P2P configuration details
 * \param[in] pConfigRspCb          Callback which is called on completion of the configuration
 * \param[in] pContext              Upper layer context to be returned in the callback
 *
 * \retval #NFCSTATUS_PENDING             Config operation is in progress
 * \retval #NFCSTATUS_INVALID_PARAMETER   Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED     LibNfc is not yet initialized
 * \retval #NFCSTATUS_SHUTDOWN            Shutdown in progress
 * \retval #NFCSTATUS_BUSY                Previous request in progress can not accept new request
 */
extern NFCSTATUS phLibNfc_Mgt_SetP2P_ConfigParams(   phLibNfc_sNfcIPCfg_t*   pConfigInfo,
                                                     pphLibNfc_RspCb_t       pConfigRspCb,
                                                     void*                   pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Stack capabilities
 *
 *  LibNfc client can query to retrieve stack capabilities. It includes
 *
 * -# Device capabilities which contains details like protocols supported,
 *    Hardware,Firmware and Model-id version details. For details refer to
 *    #phNfc_sDeviceCapabilities_t
 * -# NDEF mapping related info. This information helps in identifying supported tags
 *    for NDEF mapping
 * -# NDEF formatting related info. This info helps in identifying supported tags for
 *      NDEF formatting feature.
 *
 * \param[out] phLibNfc_StackCapabilities   Contains device capabilities, NDEF mapping and
 *                                          formatting feature support for different tag types
 * \param[in] pContext                      Upper layer context
 *
 * \retval #NFCSTATUS_SUCCESS               Get stack Capabilities operation successful
 * \retval #NFCSTATUS_INVALID_PARAMETER     Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED       LibNfc is not yet initialized
 * \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress
 * \retval #NFCSTATUS_FAILED                operation failed
 * \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request
 */
extern NFCSTATUS phLibNfc_Mgt_GetstackCapabilities(phLibNfc_StackCapabilities_t* phLibNfc_StackCapabilities,
                                                   void*                         pContext
    );

/**
 * \ingroup grp_lib_nfc
 * \brief Interface to LibNfc Reset
 *
 *  LibNfc client can reset LibNfc through this function.
 *
 *\param[in]  pContext  Upper layer context
 *
 * \retval #NFCSTATUS_SUCCESS               Reset operation successful
 * \retval #NFCSTATUS_INVALID_PARAMETER     Invalid parameter
 * \retval #NFCSTATUS_NOT_INITIALISED       LibNfc is not yet initialized
 * \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress
 */

extern NFCSTATUS phLibNfc_Mgt_Reset(void*  pContext);

/**
* \ingroup grp_lib_nfc
* \brief CE Notification Register
*
* This interface registers notification handler for Card Emulation
* activaction and deactivation.
*
* This  function allows  libNfc client to register for notifications based technology
* type for card emulation. When the registered CE activation is received,
* LibNfc notifies registered notification callback
*
* \note In case this API is called multiple times, most recent request registry details will be used
* for registration.
*
* \param[in] pNotificationHandler   Notification callback. This callback will
*                                   be notified once registered CE activation or
*                                   deactivation is received.
* \param[in] pContext               Client context which will be included in
*                                   callback when the request is completed.
*
* \retval #NFCSTATUS_SUCCESS            Indicates registration successful
* \retval #NFCSTATUS_INVALID_PARAMETER  Invalid parameter
* \retval #NFCSTATUS_NOT_INITIALISED    LibNfc is not yet initialized
* \retval #NFCSTATUS_SHUTDOWN           Shutdown in progress
*/
extern
NFCSTATUS
phLibNfc_CardEmulation_NtfRegister(
                                        phLibNfc_NtfRegister_RspCb_t    pNotificationHandler,
                                        void                            *pContext
                                        );

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface to configure listen mode routing table</b>.
* The routing table shall not be configured before after starting discovery (poll/listen) or
* if any interface is already active.
*
* \param[in]  bNumRtngConfigs      Number of routing configuration structures pointed by 'pRoutingCfg' parameter
* \param[in]  pRoutingCfg          pointer to structures containing multiple routing configurations as detailed
*                                  as in \ref phLibNfc_RtngConfig_t.
* \param[in]  pRoutingCfg_RspCb    This callback will be called once the function completes the Configuration
* \param[in]  pContext             Upper layer context to be returned along with the callback.
*
* \retval NFCSTATUS_PENDING                  Config operation is in progress
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_BUSY                     Previous request in progress cannot accept new request.
* \retval NFCSTATUS_NOT_ALLOWED              No NFCEEs found,can not configure Routing table.
*
* \note Response callback parameters details for this interface are as listed below
*/
extern NFCSTATUS phLibNfc_Mgt_ConfigRoutingTable(uint8_t               bNumRtngConfigs,
                                                 phLibNfc_RtngConfig_t *pRoutingCfg,
                                                 pphLibNfc_RspCb_t     pRoutingCfg_RspCb,
                                                 void*                 pContext);

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface to configure local LLCP peer</b>.
*
* This function configures the parameters of the local LLCP peer. This function must be called
* before any other LLCP-related function from this API.
*
* \param[in] pConfigInfo   Contains local LLCP link parameters to be applied
* \param[in] pConfigRspCb  This callback has to be called once LibNfc
*                          completes the Configuration.
* \param[in] pContext      Upper layer context to be returned in
*                          the callback.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_PENDING               Configuration operation is in progress,
                                           pConfigRspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Mgt_SetLlcp_ConfigParams( phLibNfc_Llcp_sLinkParameters_t* pConfigInfo,
                                                   pphLibNfc_RspCb_t                pConfigRspCb,
                                                   void*                            pContext
                                                   );

/**
* \ingroup grp_lib_nfc
* \brief <b>Checks if a remote peer is LLCP compliant</b>.
*
* This functions allows to check if a previously detected tag is compliant with the
* LLCP protocol. This step is needed before calling any other LLCP-related function on
* this remote peer, except local LLCP peer configurationn, which is more general. Once
* this checking is done, the caller will be able to receive link status notifications
* until the peer is disconnected.
*
* \param[in] hRemoteDevice       Peer handle obtained during device discovery process.
* \param[in] pCheckLlcp_RspCb    The callback to be called once LibNfc
*                                completes the LLCP compliancy check.
* \param[in] pLink_Cb            The callback to be called each time the
*                                LLCP link status changes.
* \param[in] pContext            Upper layer context to be returned in
*                                the callbacks.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_PENDING               Check operation is in progress, pCheckLlcp_RspCb will
*                                          be called upon completion.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_CheckLlcp( phLibNfc_Handle              hRemoteDevice,
                                          pphLibNfc_ChkLlcpRspCb_t     pCheckLlcp_RspCb,
                                          pphLibNfc_LlcpLinkStatusCb_t pLink_Cb,
                                          void*                        pContext
                                          );

/**
* \ingroup grp_lib_nfc
* \brief <b>Activates a LLCP link with a remote device </b>.
*
* This function launches the link activation process on a remote LLCP-compliant peer. The link status
* notification will be sent by the corresponding callback given in the phLibNfc_Llcp_CheckLlcp function.
* If the activation fails, the deactivated status will be notified, even if the link is already in a
* deactivated state.
*
* \param[in] hRemoteDevice       Peer handle obtained during device discovery process.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_PENDING               Activation operation is in progress,
                                           pLink_Cb will be called upon completion.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_Activate( phLibNfc_Handle hRemoteDevice );


/**
* \ingroup grp_lib_nfc
* \brief <b>Deactivate a previously activated LLCP link with a remote device</b>.
*
* This function launches the link deactivation process on a remote LLCP-compliant peer. The link status
* notification will be sent by the corresponding callback given in the phLibNfc_Llcp_CheckLlcp function.
*
* \param[in] hRemoteDevice       Peer handle obtained during device discovery process.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_PENDING               Deactivation operation is in progress,
                                           pLink_Cb will be called upon completion.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_Deactivate( phLibNfc_Handle  hRemoteDevice );


/**
* \ingroup grp_lib_nfc
* \brief <b>Get information on the local LLCP peer</b>.
*
* This function returns the LLCP link parameters of the local peer that were used
* during the link activation.
*
* \param[in]  hRemoteDevice         Peer handle obtained during device discovery process.
* \param[out] pConfigInfo           Pointer on the variable to be filled with the configuration
                                    parameters used during activation.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_GetLocalInfo( phLibNfc_Handle                  hRemoteDevice,
                                             phLibNfc_Llcp_sLinkParameters_t* pConfigInfo
                                             );

/**
* \ingroup grp_lib_nfc
* \brief <b>Get information on the remote LLCP peer</b>.
*
* This function returns the LLCP link parameters of the remote peer that were received
* during the link activation.
*
* \param[in]  hRemoteDevice         Peer handle obtained during device discovery process.
* \param[out] pConfigInfo           Pointer on the variable to be filled with the configuration
                                    parameters used during activation.
*
* \retval #NFCSTATUS_SUCCESS               Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER     One or more of the supplied parameters
*                                          could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED       Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN              Shutdown in progress.
* \retval #NFCSTATUS_FAILED                Operation failed.
* \retval #NFCSTATUS_BUSY                  Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_GetRemoteInfo( phLibNfc_Handle                    hRemoteDevice,
                                              phLibNfc_Llcp_sLinkParameters_t*   pConfigInfo
                                              );


/**
* \ingroup grp_lib_nfc
* \brief <b>Create a socket on a LLCP-connected device</b>.
*
* This function creates a socket for a given LLCP link. Sockets can be of two types :
* connection-oriented and connectionless. If the socket is connection-oriented, the caller
* must provide a working buffer to the socket in order to handle incoming data. This buffer
* must be large enough to fit the receive window (RW * MIU), the remaining space being
* used as a linear buffer to store incoming data as a stream. Data will be readable later
* using the phLibNfc_Llcp_Recv function. If the socket is connectionless, the caller may
* provide a working buffer to the socket in order to bufferize as many packets as the buffer
* can contain (each packet needs MIU + 1 bytes).
* The options and working buffer are not required if the socket is used as a listening socket,
* since it cannot be directly used for communication.
*
* \param[in]  eType                 The socket type.
* \param[in]  psOptions             The options to be used with the socket.
* \param[in]  psWorkingBuffer       A working buffer to be used by the library.
* \param[out] phSocket              A pointer on the variable to be filled with the handle on the created socket.
* \param[in]  pErr_Cb               The callback to be called each time the socket is in error.
* \param[in]  pContext              Upper layer context to be returned in the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                             declared in the options.
* \retval #NFCSTATUS_INSUFFICIENT_RESOURCES   No more socket handle available.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Socket( phLibNfc_Llcp_eSocketType_t      eType,
                                       phLibNfc_Llcp_sSocketOptions_t*  psOptions,
                                       phNfc_sData_t*                   psWorkingBuffer,
                                       phLibNfc_Handle*                 phSocket,
                                       pphLibNfc_LlcpSocketErrCb_t      pErr_Cb,
                                       void*                            pContext
                                       );


/**
* \ingroup grp_lib_nfc
* \brief <b>Get SAP of remote services using their names</b>.
*
* This function sends SDP queries to the remote peer to get the SAP to address for a given
* service name. The queries are aggregated as much as possible for efficiency, but if all
* the queries cannot fit in a single packet, they will be splitted in multiple packets.
* The callback will be called only when all of the requested services names SAP will be
* gathered. As mentionned in LLCP specification, a SAP of 0 means that the service name
* as not been found.
*
* This feature is available only since LLCP v1.1, both devices must be at least v1.1 in
* order to be able to use this function.
*
* \param[in]  hRemoteDevice      Peer handle obtained during device discovery process.
* \param[in]  psServiceNameList  The list of the service names to discover.
* \param[out] pnSapList          The list of the corresponding SAP numbers, in the same
*                                order than the service names list.
* \param[in]  nListSize          The size of both service names and SAP list.
* \param[in]  pDiscover_Cb       The callback to be called once LibNfc matched SAP for
*                                all of the provided service names.
* \param[in]  pContext           Upper layer context to be returned in the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
* \retval #NFCSTATUS_FEATURE_NOT_SUPPORTED    Remote peer does not support this feature (e.g.: is v1.0).
* \retval #NFCSTATUS_BUSY                     Previous request in progress can not accept new request.
*/
extern NFCSTATUS phLibNfc_Llcp_DiscoverServices( phLibNfc_Handle     hRemoteDevice,
                                                 phNfc_sData_t       *psServiceNameList,
                                                 uint8_t             *pnSapList,
                                                 uint8_t             nListSize,
                                                 pphLibNfc_RspCb_t   pDiscover_Cb,
                                                 void                *pContext
                                               );

/**
* \ingroup grp_lib_nfc
* \brief <b>Close a socket on a LLCP-connected device</b>.
*
* This function closes a LLCP socket previously created using phLibNfc_Llcp_Socket.
* If the socket was connected, it is first disconnected, and then closed.
*
* \param[in]  hSocket               Socket handle obtained during socket creation.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Close( phLibNfc_Handle hSocket );


/**
* \ingroup grp_lib_nfc
* \brief <b>Get the local options of a socket</b>.
*
* This function returns the local options (maximum packet size and receive window size) used
* for a given connection-oriented socket. This function shall not be used with connectionless
* sockets.
*
* \param[in]  hSocket               Socket handle obtained during socket creation.
* \param[in]  psLocalOptions        A pointer to be filled with the local options of the socket.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_SocketGetLocalOptions( phLibNfc_Handle                  hSocket,
                                                      phLibNfc_Llcp_sSocketOptions_t*  psLocalOptions
                                                      );


/**
* \ingroup grp_lib_nfc
* \brief <b>Get the local options of a socket</b>.
*
* This function returns the remote options (maximum packet size and receive window size) used
* for a given connection-oriented socket. This function shall not be used with connectionless
* sockets.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  psRemoteOptions    A pointer to be filled with the remote options of the socket.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_SocketGetRemoteOptions( phLibNfc_Handle                  hRemoteDevice,
                                                       phLibNfc_Handle                  hSocket,
                                                       phLibNfc_Llcp_sSocketOptions_t*  psRemoteOptions
                                                       );


/**
* \ingroup grp_lib_nfc
* \brief <b>Bind a socket to a local SAP</b>.
*
* This function binds the socket to a local Service Access Point.
*
* \param[in]  hSocket               Peer handle obtained during device discovery process.
* \param[in]  nSap                  Service access point
* \param[in]  psServiceName         Pointer to a structure for specifying service name and its length
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_ALREADY_REGISTERED       The selected SAP is already bound to another
                                              socket.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Bind( phLibNfc_Handle hSocket,
                                     uint8_t         nSap,
                                     phNfc_sData_t * psServiceName
                                     );


/**
* \ingroup grp_lib_nfc
* \brief <b>Listen for incoming connection requests on a socket</b>.
*
* This function switches a socket into a listening state and registers a callback on
* incoming connection requests. In this state, the socket is not able to communicate
* directly. The listening state is only available for connection-oriented sockets
* which are still not connected. The socket keeps listening until it is closed, and
* thus can trigger several times the pListen_Cb callback. The caller can adverise the
* service through SDP by providing a service name.
*
*
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  pListen_Cb         The callback to be called each time the
*                                socket receive a connection request.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state to switch
*                                             to listening state.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Listen( phLibNfc_Handle                  hSocket,
                                       pphLibNfc_LlcpSocketListenCb_t   pListen_Cb,
                                       void*                            pContext
                                       );


/**
* \ingroup grp_lib_nfc
* \brief <b>Accept an incoming connection request for a socket</b>.
*
* This functions allows the client to accept an incoming connection request.
* It must be used with the socket provided within the listen callback. The socket
* is implicitly switched to the connected state when the function is called.
*
* \param[in]  hSocket               Socket handle obtained in the listening callback.
* \param[in]  psOptions             The options to be used with the socket.
* \param[in]  psWorkingBuffer       A working buffer to be used by the library.
* \param[in]  pErr_Cb               The callback to be called each time the accepted socket
*                                   is in error.
* \param[in]  pAccept_RspCb         The callback to be called when the Accept operation
*                                   is completed.
* \param[in]  pContext              Upper layer context to be returned in the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                             declared in the options.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Accept( phLibNfc_Handle                  hSocket,
                                       phLibNfc_Llcp_sSocketOptions_t*  psOptions,
                                       phNfc_sData_t*                   psWorkingBuffer,
                                       pphLibNfc_LlcpSocketErrCb_t      pErr_Cb,
                                       pphLibNfc_LlcpSocketAcceptCb_t   pAccept_RspCb,
                                       void*                            pContext
                                       );


/**
* \ingroup grp_lib_nfc
* \brief <b>Reject an incoming connection request for a socket</b>.
*
* This functions allows the client to reject an incoming connection request.
* It must be used with the socket provided within the listen callback. The socket
* is implicitly closed when the function is called.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained in the listening callback.
* \param[in]  pReject_RspCb      The callback to be called when the Reject operation
*                                is completed.
* \param[in]  pContext           Upper layer context.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Reject( phLibNfc_Handle                  hRemoteDevice,
                                       phLibNfc_Handle                  hSocket,
                                       pphLibNfc_LlcpSocketAcceptCb_t   pReject_RspCb,
                                       void*                            pContext);


/**
* \ingroup grp_lib_nfc
* \brief <b>Try to establish connection with a socket on a remote SAP</b>.
*
* This function tries to connect to a given SAP on the remote peer. If the
* socket is not bound to a local SAP, it is implicitly bound to a free SAP.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  nSap               The destination SAP to connect to.
* \param[in]  pConnect_RspCb     The callback to be called when the connection
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Connection operation is in progress,
*                                             pConnect_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Connect( phLibNfc_Handle                 hRemoteDevice,
                                        phLibNfc_Handle                 hSocket,
                                        uint8_t                         nSap,
                                        pphLibNfc_LlcpSocketConnectCb_t pConnect_RspCb,
                                        void*                           pContext
                                        );


/**
* \ingroup grp_lib_nfc
* \brief <b>Try to establish connection with a socket on a remote service, given its URI</b>.
*
* This function tries to connect to a SAP designated by an URI. If the
* socket is not bound to a local SAP, it is implicitly bound to a free SAP.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  psUri              The URI corresponding to the destination SAP to connect to.
* \param[in]  pConnect_RspCb     The callback to be called when the connection
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Connection operation is in progress,
*                                             pConnect_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_ConnectByUri( phLibNfc_Handle                 hRemoteDevice,
                                             phLibNfc_Handle                 hSocket,
                                             phNfc_sData_t*                  psUri,
                                             pphLibNfc_LlcpSocketConnectCb_t pConnect_RspCb,
                                             void*                           pContext
                                             );


/**
* \ingroup grp_lib_nfc
* \brief <b>Disconnect a currently connected socket</b>.
*
* This function initiates the disconnection of a previously connected socket.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  pDisconnect_RspCb  The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Disconnection operation is in progress,
*                                             pDisconnect_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Disconnect( phLibNfc_Handle                    hRemoteDevice,
                                           phLibNfc_Handle                    hSocket,
                                           pphLibNfc_LlcpSocketDisconnectCb_t pDisconnect_RspCb,
                                           void*                              pContext
                                           );


/**
* \ingroup grp_lib_nfc
* \brief <b>Read data on a socket</b>.
*
* This function is used to read data from a socket. It reads at most the
* size of the reception buffer, but can also return less bytes if less bytes
* are available. If no data is available, the function will be pending until
* more data comes, and the response will be sent by the callback. This function
* can only be called on a connection-oriented socket.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  psBuffer           The buffer receiving the data.
* \param[in]  pRecv_RspCb        The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Reception operation is in progress,
*                                             pRecv_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Recv( phLibNfc_Handle              hRemoteDevice,
                                     phLibNfc_Handle              hSocket,
                                     phNfc_sData_t*               psBuffer,
                                     pphLibNfc_LlcpSocketRecvCb_t pRecv_RspCb,
                                     void*                        pContext
                                     );


/**
* \ingroup grp_lib_nfc
* \brief <b>Read data on a socket and get the source SAP</b>.
*
* This function is the same as phLibNfc_Llcp_Recv, except that the callback includes
* the source SAP. This functions can only be called on a connectionless socket.
*
* \param[in]  hRemoteDevice   Remote device handle
* \param[in]  hSocket         Socket handle obtained during socket creation.
* \param[in]  psBuffer        The buffer receiving the data.
* \param[in]  pRecv_Cb        The callback to be called when the
*                             operation is completed.
* \param[in]  pContext        Upper layer context to be returned in
*                             the callback.
*
* \retval #NFCSTATUS_SUCCESS              Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER    One or more of the supplied parameters
*                                         could not be properly interpreted.
* \retval #NFCSTATUS_PENDING              Reception operation is in progress,
*                                         pRecv_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE        The socket is not in a valid state, or not of
*                                         a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED      Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN             Shutdown in progress.
* \retval #NFCSTATUS_FAILED               Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_RecvFrom( phLibNfc_Handle                   hRemoteDevice,
                                         phLibNfc_Handle                   hSocket,
                                         phNfc_sData_t*                    psBuffer,
                                         pphLibNfc_LlcpSocketRecvFromCb_t  pRecv_Cb,
                                         void*                             pContext
                                         );


/**
* \ingroup grp_lib_nfc
* \brief <b>Send data on a socket</b>.
*
* This function is used to write data on a socket. This function
* can only be called on a connection-oriented socket which is already
* in a connected state.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  psBuffer           The buffer containing the data to send.
* \param[in]  pSend_RspCb        The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Reception operation is in progress,
*                                             pSend_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_Send( phLibNfc_Handle              hRemoteDevice,
                                     phLibNfc_Handle              hSocket,
                                     phNfc_sData_t*               psBuffer,
                                     pphLibNfc_LlcpSocketSendCb_t pSend_RspCb,
                                     void*                        pContext
                                     );

/**
* \ingroup grp_lib_nfc
* \brief <b>Send data on a socket to a given destination SAP</b>.
*
* This function is used to write data on a socket to a given destination SAP.
* This function can only be called on a connectionless socket.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
* \param[in]  nSap               The destination SAP.
* \param[in]  psBuffer           The buffer containing the data to send.
* \param[in]  pSend_RspCb        The callback to be called when the
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval #NFCSTATUS_SUCCESS                  Operation successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_PENDING                  Reception operation is in progress,
*                                             pSend_RspCb will be called upon completion.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_SendTo( phLibNfc_Handle               hRemoteDevice,
                                       phLibNfc_Handle               hSocket,
                                       uint8_t                       nSap,
                                       phNfc_sData_t*                psBuffer,
                                       pphLibNfc_LlcpSocketSendCb_t  pSend_RspCb,
                                       void*                         pContext
                                       );

/**
* \ingroup grp_lib_nfc
* \brief <b>Cancel the pending Send fragment issued by SNEP in case SNEP has received Invalid Response</b>.
*
* This function is used by SNEP layer only. SNEP calls this function in case of Fragmented PUT/GET request.
* This function is called to cancel the pending send Fragment at Fri layer, in case an Invalid response(2nd CONTINUE Request/Response)
* is received.
*
* \param[in]  hRemoteDevice      Remote device handle
* \param[in]  hSocket            Socket handle obtained during socket creation.
*
* \retval #NFCSTATUS_ABORTED                  Send Operation aborted successful.
* \retval #NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                             could not be properly interpreted.
* \retval #NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                             a valid type to perform the requsted operation.
* \retval #NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval #NFCSTATUS_FAILED                   Operation failed.
*/
extern NFCSTATUS phLibNfc_Llcp_CancelPendingSend( phLibNfc_Handle hRemoteDevice,
                                                  phLibNfc_Handle hSocket
                                                  );

/**
*\ingroup grp_lib_nfc
* \brief Response callback for eSE Get ATR request
*
* This callback is used to provide received data to the LibNfc client in #phNfc_sData_t format.
* This callback is called when LibNfc client has performed a eSE Get ATR operation on a tag
*
* \param[in] pContext       LibNfc client context passed in the corresponding request
* \param[in] pResAtrInfo    Response buffer of type #pphNfc_SeAtr_Info_t
* \param[in] status         Status of the response callback
*                - #NFCSTATUS_SUCCESS - operation  successful
*                - #NFCSTATUS_FAILED  - operation failed because target is lost
*                - #NFCSTATUS_SHUTDOWN -operation failed because Shutdown in progress
*                - #NFCSTATUS_ABORTED - aborted due to disconnect request in between
*                - #NFCSTATUS_INSUFFICIENT_RESOURCES - operation failed because Insufficient resources
*/
typedef void(*pphLibNfc_GetAtrCallback_t)(
    void* pContext,
    pphNfc_SeAtr_Info_t pResAtrInfo,
    NFCSTATUS Status
    );

/**
* \ingroup grp_lib_nfc
* \brief This function shall  retreive the ATR (Answer to Request) from Secure element.
*
* \param[in]  hSE_Handle        Handle to secure element
* \param[in]  pAtrInfo          pointer to #phNfc_SeAtr_Info_t structure
* \param[in]  pGetAtr_RspCb     The callback to be called when the
*                               operation is completed.
* \param[in]  pContext          Upper layer context to be returned in
*                               the callback.
*
* \retval #NFCSTATUS_INVALID_PARAMETER    One or more of the supplied parameters
*                                         could not be properly interpreted.
* \retval #NFCSTATUS_PENDING              Reception operation is in progress,
*                                         pGetAtr_RspCb will be called upon completion.
* \retval #NFCSTATUS_NOT_INITIALISED      Indicates stack is not yet initialized.
* \retval #NFCSTATUS_SHUTDOWN             Shutdown in progress.
* \retval #NFCSTATUS_FAILED               Operation failed.
*/
extern NFCSTATUS phLibNfc_eSE_GetAtr(phLibNfc_Handle                 hSE_Handle,
                                     phNfc_SeAtr_Info_t*             pAtrInfo,
                                     pphLibNfc_GetAtrCallback_t      pGetAtr_RspCb,
                                     void*                           pContext);
