/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSC.h

Abstract:

    SC Interface declaration
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#include "NfcCxSCCommon.h"

// To be removed once these Attribute IDs are defined in winsmcrd.h
#define SCARD_ATTR_VENDOR_SPECIFIC_BRAND_INFO 0x0180
#define SCARD_ATTR_VENDOR_SPECIFIC_DEVICECAP_INFO 0x0181

#define HINIBBLE(b) ((BYTE)((b >> 4) & 0x0f))
#define LONIBBLE(b) ((BYTE)(b & 0x0f))

#define APDU_STATUS_WARNING_NO_INFO                 "\x63\x00"
#define APDU_STATUS_ERROR_WRONG_APDU_LENGTH         "\x67\x00"
#define APDU_STATUS_ERROR_AUTH_METHOD_BLOCKED       "\x69\x83"
#define APDU_STATUS_ERROR_CONDITIONS_OF_USE_NOT_SATISFIED       "\x69\x85"
#define APDU_STATUS_ERROR_DATA_OBJECT_MISSING       "\x69\x87"
#define APDU_STATUS_ERROR_INCORRECT_DATA_OBJECT     "\x69\x88"
#define APDU_STATUS_ERROR_INVALID_OBJECT_LENGTH     "\x69\x89"
#define APDU_STATUS_ERROR_COMMAND_ABORTED           "\x6F\x00"
#define APDU_STATUS_SUCCESS                         "\x90\x00"

#define SAK_MIFARE_UL               0x00
#define SAK_MIFARE_CLASSIC_1K       0x01
#define SAK_MIFARE_STD_1K           0x08
#define SAK_MIFARE_MINI             0x09
#define SAK_MIFARE_PLUS_SL2_2K      0x10
#define SAK_MIFARE_PLUS_SL2_4K      0x11
#define SAK_MIFARE_STD_4K           0x18
#define SAK_JCOP_MULTI_PROTOCOL     0x28
#define SAK_SMARTMX_MULTI_PROTOCOL  0x38
#define SAK_MIFARE_INFINEON_1K      0x88

#define SAK_CONFIG_MASK             0x64U
#define SAK_CONFIG_MASKMFC          0x18U

#define ICC_TYPE_UNKNOWN            0
#define ICC_TYPE_14443_TYPE_A       5
#define ICC_TYPE_14443_TYPE_B       6
#define ICC_TYPE_ISO_15693          7

#define PCSC_ATR_SS_NO_INFO         0x00
#define PSCS_ATR_SS_14443A_3        0x03
#define PSCS_ATR_SS_ISO15693_4      0x0C
#define PSCS_ATR_SS_FELICA          0x11

#define PCSC_NN_NO_INFO             0x00
#define PCSC_NN_MIFARE_STD_1K       0x01
#define PCSC_NN_MIFARE_STD_4K       0x02
#define PCSC_NN_MIFARE_UL           0x03
#define PCSC_NN_ICODE_SLI           0x14
#define PCSC_NN_MIFARE_MINI         0x26
#define PCSC_NN_JEWEL               0x2F
#define PSCS_NN_TOPAZ               0x30
#define PCSC_NN_MIFARE_PLUS_SL2_2K  0x38
#define PCSC_NN_MIFARE_PLUS_SL2_4K  0x39
#define PCSC_NN_MIFARE_ULC          0x3A
#define PCSC_NN_FELICA              0x3B
#define PCSC_NN_MIFARE_ULEV1        0x3D

#define PCSC_ATR_INIT_HEADER                        0x3B
#define PCSC_ATR_T0                                 0x80
#define PCSC_ATR_T0_MASK                            0x0F
#define PCSC_ATR_TD1                                0x80
#define PCSC_ATR_TD2                                0x01
#define PCSC_ATR_STORAGE_CARD_T1                    0x80
#define PCSC_ATR_STORAGE_CARD_PRESENCE_INDICATOR    0x4F
#define PCSC_ATR_STORAGE_CARD_HIST_BYTES_LENGTH     0x0C
#define PCSC_ATR_STORAGE_CARD_RID0                  0xA0
#define PCSC_ATR_STORAGE_CARD_RID1                  0x00
#define PCSC_ATR_STORAGE_CARD_RID2                  0x00
#define PCSC_ATR_STORAGE_CARD_RID3                  0x03
#define PCSC_ATR_STORAGE_CARD_RID4                  0x06

//
// Macros to support Jewel tag ATR construction
// Header Rom versions based on jewel IC data sheet Revision1.1
// refer IRT5002 to IRT5006
//

#define PCSC_ATR_JEWEL_96BR10_HR0                   0x01
#define PCSC_ATR_JEWEL_96BR11_HR0                   0x04
#define PCSC_ATR_JEWEL_192B_HR0                     0x05
#define PCSC_ATR_JEWEL_384B_HR0                     0x06
#define PCSC_ATR_JEWEL_1024B_HR0                    0x07
#define PCSC_ATR_JEWEL_2048B_HR0                    0x08

//
// Macros to support Topaz tag ATR construction
// Header Rom versions based on Topaz IC data sheet version M2000-1057-02
// refer TPZ-201-series section 6.
//

#define PCSC_ATR_TOPAZ_120B_HR0                     0x11
#define PCSC_ATR_TOPAZ_512B_HR0                     0x12

#define DEFAULT_14443_4_TRANSCEIVE_TIMEOUT  1500

#define MIFARE_UL_AUTHENTICATE_RESPONSE_BUFFER_SIZE 9
#define MIFARE_UL_AUTHENTICATE_RESPONSE_TIMEOUT     100

#define PCSC_SELECT_PROTOCOL_MIFARE                 0x03
#define PCSC_SELECT_PROTOCOL_ISO4A                  0x04
#define PCSC_SWITCH_PROTOCOL_TYPE                   0X8F
#define PCSC_SWITCH_PROTOCOL_LENGTH                 0X02
#define PCSC_SWITCH_PROTOCOL_STD_TYPE               0X00
#define PCSC_SWITCH_PROTOCOL_APDU_SIZE              0x09
#define PCSC_SWITCH_PROTOCOL_INDEX_NOT_FOUND        0xFF

typedef struct _NFCCX_SC_INTERFACE {
    //
    // Back link to the fdo context
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Interface Created
    //
    BOOLEAN InterfaceCreated;

    //
    // Sequential Dispatch IO Queue
    //
    WDFQUEUE SerialIoQueue;

    //
    // Present / Absent
    //
    NFCCX_SC_PRESENT_ABSENT_DISPATCHER PresentDispatcher;
    NFCCX_SC_PRESENT_ABSENT_DISPATCHER AbsentDispatcher;

    //
    // SmartCard Connection state
    //
    WDFWAITLOCK SmartCardLock;
    _Guarded_by_(SmartCardLock)
    BOOLEAN SmartCardConnected;

    //
    // Reference count for exclusive file handle
    //
    _Guarded_by_(SmartCardLock)
    PNFCCX_FILE_CONTEXT CurrentClient;
    _Guarded_by_(SmartCardLock)
    BOOLEAN SessionEstablished;

    BOOLEAN ClientPowerReferenceHeld;

    //
    // Copy of the remote device info from the RF Interface
    //
    _Guarded_by_(SmartCardLock)
    phNfc_sRemoteDevInformation_t RemoteDeviceInfo;

    //
    // Selected DeviceInfo Number from DeviceList
    //
    DWORD  SelectedProtocolIndex;
    //
    // SmartCard storage card reference counted pointer
    //
    StorageCardManager* StorageCard;

    _Guarded_by_(SmartCardLock)
    LoadKey* StorageCardKey;

} NFCCX_SC_INTERFACE, *PNFCCX_SC_INTERFACE;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL
NfcCxSCInterfaceSequentialIoDispatch;

NTSTATUS
NfcCxSCInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Out_ PNFCCX_SC_INTERFACE * PPSCInterface
    );

VOID
NfcCxSCInterfaceDestroy(
    _In_ PNFCCX_SC_INTERFACE SCInterface
    );

NTSTATUS
NfcCxSCInterfaceStart(
    _In_ PNFCCX_SC_INTERFACE SCInterface
    );

VOID
NfcCxSCInterfaceStop(
    _In_ PNFCCX_SC_INTERFACE SCInterface
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED 
NfcCxSCInterfaceIsIoctlSupported;


FN_NFCCX_DDI_MODULE_IODISPATCH 
NfcCxSCInterfaceIoDispatch;

//
// Reference count in the fileObject providing exclusive file handling
//

NTSTATUS
NfcCxSCInterfaceAddClient(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxSCInterfaceRemoveClient(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

//
// Handling methods below
//

VOID
NfcCxSCInterfaceHandleSmartCardConnectionEstablished(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_ PNFCCX_RF_INTERFACE RFInterface
    );

VOID
NfcCxSCInterfaceHandleSmartCardConnectionLost(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

NTSTATUS
NfcCxSCInterfaceValidateRequest(
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxSCInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

//
// Dispatch methods below
//
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchGetAttribute;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchSetAttribute;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchGetState;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchSetPower;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchSetProtocol;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchIsAbsent;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchIsPresent;
NFCCX_SC_DISPATCH_HANDLER NfcCxSCInterfaceDispatchTransmit;

//
// Dispatched attribute methods below
//
NTSTATUS
NfcCxSCInterfaceDispatchAttributePresent(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxSCInterfaceDispatchAttributeCurrentProtocolType(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceDispatchAttributeAtr(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxSCInterfaceDispatchAttributeIccType(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

//
// Helper methods below don't have locking constraints
//
BOOL
NfcCxSCInterfaceValidateLoadKeyCommand(
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength
    );

BOOL
NfcCxSCInterfaceValidateSwitchProtocolCommand(
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength
    );

NTSTATUS
NfcCxSCInterfaceValidateMifareLoadKeyParameters(
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength,
    _Out_bytecap_(Sw1Sw2Length) PBYTE Sw1Sw2,
    _In_ DWORD Sw1Sw2Length
    );

//
// Helper methods below require a lock
//
_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceLoadStorageClassFromAtrLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceLoadKeyLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength,
    _Out_bytecap_(Sw1Sw2Length) PBYTE Sw1Sw2,
    _In_ DWORD Sw1Sw2Length
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceKeyDataBaseLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength,
    _Out_opt_bytecap_(Sw1Sw2Length) PBYTE Sw1Sw2,
    _In_ DWORD Sw1Sw2Length
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceGetAtrLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesCopied) PBYTE OutputBuffer,
    _In_ DWORD OutputBufferLength,
    _Out_ DWORD* BytesCopied
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceGetDeviceUidLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Outptr_result_bytebuffer_(*pUidLength) BYTE **ppUid,
    _Out_ BYTE *pUidLength
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
VOID
NfcCxSCInterfaceGetDeviceTypeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_ phNfc_eRemDevType_t* pDevType,
    _Out_ DWORD* pSak
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
BOOLEAN
NfcCxSCInterfaceIsStorageCardConnected(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceGetIccTypePerAtrLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _Out_ BYTE* pIccTypePerAtr
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCEnsureCardConnectedPowerReferenceIsHeld(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
VOID
NfcCxSCEnsureCardConnectedPowerReferenceIsReleased(
    _In_ PNFCCX_SC_INTERFACE ScInterface);

//
// Helper methods below requires no lock held
//
_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceTransmitRequest(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PBYTE OutputBuffer,
    _In_ DWORD OutputBufferLength,
    _Out_ DWORD* BytesTransferred
    );

_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceTransmitRawData(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesTransferred) PBYTE OutputBuffer,
    _In_ DWORD OutputBufferLength,
    _Out_ DWORD* BytesTransferred,
    _In_ USHORT Timeout = 0 /* Default Timeout */
    );

_Requires_lock_not_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceResetCard(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

_Requires_lock_not_held_(ScInterface->SmartCardLock)
BOOLEAN
NfcCxSCInterfaceDetectMifareULC(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

BYTE
NfcCxSCInterfaceComputeChecksum(
    _In_reads_bytes_(cbAtr) BYTE* pAtr,
    _In_ DWORD cbAtr
    );

_Requires_lock_held_(ScInterface->SmartCardLock)
NTSTATUS
NfcCxSCInterfaceLoadNewSelectedProtocol(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_bytecount_(InputBufferLength) PBYTE InputBuffer,
    _In_ DWORD InputBufferLength
);

DWORD
NfcCxSCInterfaceGetIndexOfProtocolType(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phNfc_eRemDevType_t RemDevType
);

//
// Inline helper functions
//
_Requires_lock_not_held_(ScInterface->SmartCardLock)
FORCEINLINE StorageCardManager*
NfcCxAcquireStorageCardManagerReference(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    )
{
    StorageCardManager* pStorageCard = NULL;

    WdfWaitLockAcquire(ScInterface->SmartCardLock, NULL);

    if (ScInterface->StorageCard != NULL) {
        pStorageCard = ScInterface->StorageCard;
        pStorageCard->AddRef();
    }

    WdfWaitLockRelease(ScInterface->SmartCardLock);
    return pStorageCard;
}

FORCEINLINE VOID
NfcCxReleaseStorageCardManagerReference(
    _In_ StorageCardManager* StorageCard
    )
{
    if (StorageCard != NULL) {
        StorageCard->Release();
    }
}
