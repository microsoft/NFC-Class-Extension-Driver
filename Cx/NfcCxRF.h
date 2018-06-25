/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxRF.h

Abstract:

    RFInterface declarations
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#define MAX_MESSAGE_SIZE                (NFP_MAXIMUM_MESSAGE_PAYLOAD_SIZE + 512)
#define MAX_INBOX_SIZE                  MAX_MESSAGE_SIZE
#define MAX_ROUTING_TABLE_SIZE          32
#define MAX_NUMBER_OF_SE                (PHLIBNFC_MAXNO_OF_SE + 1)
#define MAX_CALLBACK_TIMEOUT            8000
#define MAX_WATCHDOG_TIMEOUT            10000
#define MAX_DEINIT_TIMEOUT              8000
#define PRESENCE_CHECK_INTERVAL         200
#define MAX_TRANSCEIVE_TIMEOUT          5000

#define DEVICE_ARRIVAL_BIT_BIDIRECTION  0x1
#define DEVICE_ARRIVAL_BIT_READONLY     0x2

typedef enum _NFCCX_LIBNFC_MESSAGE {
    LIBNFC_INIT,
    LIBNFC_DEINIT,
    LIBNFC_DISCOVER_CONFIG,
    LIBNFC_DISCOVER_STOP,
    LIBNFC_TAG_WRITE,
    LIBNFC_TAG_CONVERT_READONLY,
    LIBNFC_TARGET_ACTIVATE,
    LIBNFC_TARGET_DEACTIVATE_SLEEP,
    LIBNFC_TARGET_PRESENCE_CHECK,
    LIBNFC_TARGET_TRANSCEIVE,
    LIBNFC_TARGET_SEND,
    LIBNFC_SE_ENUMERATE,
    LIBNFC_SE_SET_MODE,
    LIBNFC_SE_SET_ROUTING_TABLE,
    LIBNFC_SNEP_CLIENT_PUT,
    LIBNFC_STATE_HANDLER,
    LIBNFC_SEQUENCE_HANDLER,
    LIBNFC_DTA_MESSAGE,
    LIBNFC_EMEBEDDED_SE_TRANSCEIVE,
    LIBNFC_EMBEDDED_SE_GET_ATR_STRING,
    LIBNFC_MESSAGE_MAX,
} NFCCX_LIBNFC_MESSAGE, *PNFCCX_LIBNFC_MESSAGE;

C_ASSERT(LIBNFC_MESSAGE_MAX <= PH_OSALNFC_MESSAGE_BASE);

typedef struct _NFCCX_LIBNFC_CONTEXT {
    DWORD                         LibNfcThreadId;
    NTSTATUS                      Status;
    phLibNfc_InitType_t           eInitType;
    uint8_t                       ConfigStatus;
    HANDLE                        hNotifyCompleteEvent;
    phLibNfc_RemoteDevList_t*     pRemDevList;
    BOOLEAN                       bIsTagPresent;
    BOOLEAN                       bIsTagConnected;
    BOOLEAN                       bIsTagNdefFormatted;
    BOOLEAN                       bIsTagWriteAttempted;
    BOOLEAN                       bIsTagReadOnlyAttempted;
    BOOLEAN                       bIsP2PConnected;
    uint8_t                       uNoRemoteDevices;
    DWORD                         SelectedProtocolIndex;
    phLibNfc_eReleaseType_t       eReleaseType;
    PNFCCX_STATE_INTERFACE        StateInterface;
    PNFCCX_LLCP_INTERFACE         LLCPInterface;
    PNFCCX_SNEP_INTERFACE         SNEPInterface;
    phLibNfc_StackCapabilities_t  sStackCapabilities;
    BOOLEAN                       EnableSEDiscovery;
    phLibNfc_SE_List_t            SEList[MAX_NUMBER_OF_SE];
    _Field_range_(<= , MAX_NUMBER_OF_SE)
    uint8_t                       SECount;
    BOOLEAN                       bIsDefaultRtngConfig;
    phLibNfc_RtngConfig_t         RtngTable[MAX_ROUTING_TABLE_SIZE];
    _Field_range_(<=, MAX_ROUTING_TABLE_SIZE)
    uint8_t                       RtngTableCount;
    uint16_t                      uHCEConnectionId;
    BOOLEAN                       bIsHCEConnected;
    BOOLEAN                       bIsHCEActivated;
    BOOLEAN                       bIsHCERecv;
} NFCCX_LIBNFC_CONTEXT, *PNFCCX_LIBNFC_CONTEXT;

typedef struct _NFCCX_RF_INTERFACE {
    //
    // Back link to the FDO
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Device operation Lock
    // Used to serialize all the user calls.
    //
    WDFWAITLOCK DeviceLock;

    //
    // Sequence related
    //
    PNFCCX_CX_SEQUENCE pSeqHandler;
    BOOLEAN bSeqHandler;
    UCHAR SeqNext;
    UCHAR SeqMax;
    _Guarded_by_(SequenceLock)
    PFN_NFC_CX_SEQUENCE_HANDLER SeqHandlers[SequenceMaximum];
    WDFWAITLOCK SequenceLock;

    //
    // LibNfc references
    //
    phLibNfc_sConfig_t LibConfig;
    PNFCCX_LIBNFC_CONTEXT pLibNfcContext;
    phLibNfc_sADD_Cfg_t DiscoveryConfig;
    phLibNfc_Registry_Info_t RegInfo;

    //
    // Configuration
    //
    GUID SessionId;
    BOOLEAN bIsDiscoveryStarted;
    ULONG uiPollDevInfo;
    UCHAR uiNfcIP_Mode;
    UCHAR uiNfcIP_Tgt_Mode;
    UCHAR uiNfcCE_Mode;
    ULONG uiDuration;
    UCHAR uiBailout; 
    // Store timetamp for Kovio last detection time to detect the tag being read again too quickly
    ULONGLONG bKovioDetected;

    //
    // Dynamic Configuration. May have different state based on initial initialization or recovery initialization.
    //
    phLibNfc_eSE_ActivationMode eDeviceHostInitializationState;

    //
    // Send/Receive buffers
    //
    phNfc_sTransceiveInfo_t sTransceiveBuffer;
    phNfc_sData_t sReceiveBuffer;
    phNfc_sData_t sSendBuffer;

    //
    // Buffers for eSE
    //
    phNfc_sSeTransceiveInfo_t SeTransceiveInfo;
    phNfc_SeAtr_Info_t SeATRInfo;

    //
    // Background workers
    //
    PTP_WORK tpTagPrescenceWork;
    HANDLE hStartPresenceCheck;
    WDFWAITLOCK PresenceCheckLock;

    //
    // Ndef Message buffer
    //
    ULONG uiActualNdefMsgLength;   // Indicates Actual length of NDEF Message in Tag.
    ULONG uiMaxNdefMsgLength;      // Indicates Maximum Ndef Message length that Tag can hold.
    phLibNfc_Data_t sNdefMsg;

    // Store barcode in a separate buffer, to not mix with ndef
    phLibNfc_Data_t sBarcodeMsg;

    //
    // SE Mode data
    //
    phLibNfc_eSE_ActivationMode SEActivationMode;
    phLibNfc_PowerLinkModes_t SEPowerAndLinkControl;

    //
    // Watchdog timer
    //
    PTP_TIMER tpWatchdogTimer;

    //
    // Power
    //
    NFC_CX_POWER_RF_STATE RFPowerState;

} NFCCX_RF_INTERFACE, *PNFCCX_RF_INTERFACE;

typedef struct _NFCCX_RF_LIBNFC_REQUEST_CONTEXT {
    PNFCCX_RF_INTERFACE RFInterface;
    PNFCCX_CX_SEQUENCE Sequence; 
} NFCCX_RF_LIBNFC_REQUEST_CONTEXT, *PNFCCX_RF_LIBNFC_REQUEST_CONTEXT;

//
// Helpers
//
FORCEINLINE PNFCCX_LLCP_INTERFACE
NfcCxRFInterfaceGetLLCPInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->pLibNfcContext->LLCPInterface;
}

FORCEINLINE PNFCCX_SNEP_INTERFACE
NfcCxRFInterfaceGetSNEPInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->pLibNfcContext->SNEPInterface;
}

//
// Interface
//
NTSTATUS
NfcCxRFInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_RF_INTERFACE * PPRFInterface
    );

VOID
NfcCxRFInterfaceDestroy(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceStop(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceSetDiscoveryConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PCNFC_CX_RF_DISCOVERY_CONFIG Config
    );

NTSTATUS
NfcCxRFInterfaceSetLLCPConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PCNFC_CX_LLCP_CONFIG Config
    );

NTSTATUS
NfcCxRFInterfaceRegisterSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_HANDLER EvtNfcCxSequenceHandler
    );

NTSTATUS
NfcCxRFInterfaceUnregisterSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_SEQUENCE Sequence
    );

NTSTATUS
NfcCxRFInterfaceUpdateDiscoveryState(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_POWER_RF_STATE RFPowerState
    );

NTSTATUS
NfcCxRFInterfaceWriteTag(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ CNFCProximityBuffer * SendBuffer
    );

NTSTATUS
NfcCxRFInterfaceWriteP2P(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ CNFCProximityBuffer * SendBuffer
    );

NTSTATUS
NfcCxRFInterfaceTransmit(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed,
    _In_ USHORT Timeout
    );

NTSTATUS
NfcCxRFInterfaceTargetSend(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength
    );

NTSTATUS
NfcCxRFInterfaceConvertToReadOnlyTag(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceTargetReactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceTargetDeactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
);

NTSTATUS
NfcCxRFInterfaceTargetActivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceGetSecureElementList(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(MAX_NUMBER_OF_SE, *SECount) phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE],
    _Out_range_(<= , MAX_NUMBER_OF_SE) uint8_t *SECount,
    _In_ BOOLEAN EnableSEDiscovery = FALSE
    );

NTSTATUS
NfcCxRFInterfaceSetCardActivationMode(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phLibNfc_eSE_ActivationMode eActivationMode,
    _In_ phLibNfc_PowerLinkModes_t ePowerAndLinkControl
    );

NTSTATUS
NfcCxRFInterfaceResetCard(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement
    );

NTSTATUS
NfcCxRFInterfaceSetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_(RtngTableCount) phLibNfc_RtngConfig_t* pRtngTable,
    _In_ uint8_t RtngTableCount
    );

NTSTATUS
NfcCxRFInterfaceGetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(RtngTableCount, *pRtngTableCount) phLibNfc_RtngConfig_t* pRtngTable,
    _In_ uint8_t RtngTableCount,
    _Out_ uint8_t *pRtngTableCount
    );

NTSTATUS
NfcCxRFInterfaceGetNfccCapabilities(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_ PSECURE_ELEMENT_NFCC_CAPABILITIES pCapabilities
    );

VOID
NfcCxPostLibNfcThreadMessage(
    _Inout_ PVOID RFInterface,
    _In_ DWORD Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    );

BOOLEAN
NfcCxRFInterfaceIsHCESupported(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

NTSTATUS
NfcCxRFInterfaceESETransmit(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed,
    _In_ USHORT Timeout
    );

NTSTATUS
NfcCxRFInterfaceESEGetATRString(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed
    );

//
// Callback Handlers
//
VOID
NfcCxRFInterfaceTagConnectionEstablished(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ DWORD ArrivalBitMask
    );

VOID
NfcCxRFInterfaceTagConnectionLost(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

BOOLEAN
NfcCxRFInterfaceCheckIfWriteTagPublicationsExist(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

VOID
NfcCxRFInterfaceP2pConnectionEstablished(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

VOID
NfcCxRFInterfaceP2pConnectionLost(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

typedef enum _RECEIVED_NDEF_MESSAGE_SOURCE {
    ReceivedNdefFromTag,
    ReceivedNdefFromPeer
} RECEIVED_NDEF_MESSAGE_SOURCE;

VOID
NfcCxRFInterfaceHandleReceivedNdefMessage(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ RECEIVED_NDEF_MESSAGE_SOURCE eSource,
    _In_ uint16_t cbRawNdef,
    _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
    );

VOID
NfcCxRFInterfaceHandleSecureElementEvent(
    _In_ PVOID pContext,
    _In_ phLibNfc_eSE_EvtType_t EventType,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _In_ NFCSTATUS Status
    );

//
// State Handlers
//
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateInitialize;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateShutdown;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRecovery;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfIdle2RfDiscovery;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDiscovery2RfIdle;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfActive2RfIdle;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfActive2RfDiscovery;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDiscovery2RfDiscovered;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDiscovered;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDiscovered2RfDataXchg;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfIdle;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDiscovery;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDataXchg2RfDiscovered;
FN_NFCCX_STATE_HANDLER NfcCxRFInterfaceStateRfDataXchg;

//
// State Connectors
//
FN_NFCCX_STATE_CONNECTOR NfcCxRFInterfaceConnChkDiscMode;
FN_NFCCX_STATE_CONNECTOR NfcCxRFInterfaceConnChkDeviceType;

//
// Sequence Handlers
//
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreInitialize;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceInitialize;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostInitialize;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreDeinitialize;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceDeinitialize;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostDeinitialize;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreRfDiscoveryStart;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceRfDiscoveryConfig;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostRfDiscoveryStart;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreRfDiscoveryStop;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceRfDiscoveryStop;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostRfDiscoveryStop;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceRemoteDevNtfRegister;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceRemoteDevConnect;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceRemoteDevDisconnect;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagCheckNdef;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagReadNdef;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagFormatNdef;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagWriteNdef;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagConvertReadOnly;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTargetTransceive;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTargetPresenceCheck;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceTagReadBarcode;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreSEEnumerate;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceSEEnumerate;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostSEEnumerate;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceSENtfRegister;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceSetDefaultRoutingTable;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceDisableSecureElements;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceSESetModeConfig;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceSESetPowerAndLinkControl;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceConfigureRoutingTable;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePreRecovery;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfacePostRecovery;

FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceESETransceiveSeq;
FN_NFCCX_CX_SEQUENCE_ENTRY NfcCxRFInterfaceESEGetATRStringSeq;

#define RF_INTERFACE_INITIALIZE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreInitialize) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceInitialize) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostInitialize)

#define RF_INTERFACE_DEINITIALIZE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreDeinitialize) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceDeinitialize) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostDeinitialize)

#define RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreRfDiscoveryStart) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRfDiscoveryConfig) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostRfDiscoveryStart)

#define RF_INTERFACE_RF_DISCOVERY_STOP_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreRfDiscoveryStop) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRfDiscoveryStop) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostRfDiscoveryStop)

#define RF_INTERFACE_REMOTEDEV_NTF_REGISTER_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRemoteDevNtfRegister)
#define RF_INTERFACE_REMOTEDEV_CONNECT_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRemoteDevConnect)

#define RF_INTERFACE_REMOTEDEV_DISCONNECT_STOP_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreRfDiscoveryStop) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRemoteDevDisconnect) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostRfDiscoveryStop)

#define RF_INTERFACE_REMOTEDEV_DISCONNECT_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceRemoteDevDisconnect)

#define RF_INTERFACE_TAG_READ_NDEF_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagCheckNdef) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagReadNdef)

#define RF_INTERFACE_TAG_READ_BARCODE_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagReadBarcode)

#define RF_INTERFACE_TAG_WRITE_NDEF_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagFormatNdef) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagCheckNdef) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagWriteNdef)

#define RF_INTERFACE_TAG_CONVERT_READONLY_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTagConvertReadOnly)

#define RF_INTERFACE_TARGET_TRANSCEIVE_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTargetTransceive)
#define RF_INTERFACE_TARGET_PRESENCE_CHECK_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceTargetPresenceCheck)

#define RF_INTERFACE_EMBEDDEDSE_TRANSCEIVE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceESETransceiveSeq)

#define RF_INTERFACE_EMBEDDEDSE_GET_ATR_STRING_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceESEGetATRStringSeq)

#define RF_INTERFACE_SE_NTF_REGISTER_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceSENtfRegister)

#define RF_INTERFACE_SE_ENUMERATE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreSEEnumerate) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceSEEnumerate) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostSEEnumerate) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceSetDefaultRoutingTable)

#define RF_INTERFACE_SE_DISABLE_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceDisableSecureElements)

#define RF_INTERFACE_SE_SET_MODE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceSESetPowerAndLinkControl) \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceSESetModeConfig) \

#define RF_INTERFACE_SE_SET_ROUTING_MODE_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfaceConfigureRoutingTable)

#define RF_INTERFACE_PRE_RECOVERY_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePreRecovery)
#define RF_INTERFACE_POST_RECOVERY_SEQUENCE NFCCX_CX_SEQUENCE_ENTRY(NfcCxRFInterfacePostRecovery)
