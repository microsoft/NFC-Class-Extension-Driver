/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSE.h

Abstract:

    SE Interface declaration
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#include "NfcCxRF.h"

// {97D3609E-A9A3-48ca-9EA6-79A48713E2A0}
EXTERN_C __declspec(selectany) const GUID UICC_SECUREELEMENT_ID =
{ 0x97d3609e, 0xa9a3, 0x48ca, { 0x9e, 0xa6, 0x79, 0xa4, 0x87, 0x13, 0xe2, 0xa0 } };

// {7473C9E7-06F2-48fa-8245-524E7053AE02}
EXTERN_C __declspec(selectany) const GUID ESE_SECUREELEMENT_ID =
{ 0x7473c9e7, 0x6f2, 0x48fa, { 0x82, 0x45, 0x52, 0x4e, 0x70, 0x53, 0xae, 0x2 } };

// {7d534654-4e46-4343-5844-484e46434545}
EXTERN_C __declspec(selectany) const GUID DH_SECUREELEMENT_ID =
{ 0x7d534654, 0x4e46, 0x4343, { 0x58, 0x44, 0x48, 0x4e, 0x46, 0x43, 0x45, 0x45 } };

#define NFCCX_MAX_HCE_PACKET_QUEUE_LENGTH (5)

typedef
NTSTATUS
NFCCX_SE_DISPATCH_HANDLER(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

typedef NFCCX_SE_DISPATCH_HANDLER *PFN_NFCCX_SE_DISPATCH_HANDLER;

typedef struct _NFCCX_SE_POWER_SETTINGS
{
    SECURE_ELEMENT_CARD_EMULATION_MODE EmulationMode;
    BOOLEAN WiredMode;
    BOOLEAN ForcePowerOn;
} NFCCX_SE_POWER_SETTINGS;

typedef struct _NFCCX_SE_POWER_SETTINGS_LIST_ITEM
{
    GUID SecureElementId;
    NFCCX_SE_POWER_SETTINGS Settings;
} NFCCX_SE_POWER_SETTINGS_LIST_ITEM;

typedef struct _NFCCX_SE_INTERFACE {

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
    // SEManager Exclusive File Handle
    //
    WDFWAITLOCK SEManagerLock;
    _Guarded_by_(SEManagerLock)
    PNFCCX_FILE_CONTEXT SEManager;
    
    //
    // Secure Element Events
    //
    WDFWAITLOCK SEEventsLock;
    _Guarded_by_(SEEventsLock)
    LIST_ENTRY SEEventsList;

    //
    // Secure Element Emulation Mode
    //
    WDFWAITLOCK SEPowerSettingsLock;
    NFCCX_SE_POWER_SETTINGS_LIST_ITEM SEPowerSettings[MAX_NUMBER_OF_SE];
    DWORD SEPowerSettingsCount;

} NFCCX_SE_INTERFACE, *PNFCCX_SE_INTERFACE;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL
NfcCxSEInterfaceSequentialIoDispatch;

NTSTATUS
NfcCxSEInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT DeviceContext,
    _Out_ PNFCCX_SE_INTERFACE * PPSEInterface
    );

VOID
NfcCxSEInterfaceDestroy(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    );

NTSTATUS
NfcCxSEInterfaceStart(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    );

NTSTATUS
NfcCxSEInterfaceStop(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    );

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED 
NfcCxSEInterfaceIsIoctlSupported;


FN_NFCCX_DDI_MODULE_IODISPATCH 
NfcCxSEInterfaceIoDispatch;

NTSTATUS
NfcCxSEInterfaceAddClient(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxSEInterfaceRemoveClient(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxSEInterfaceHandleEvent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_  SECURE_ELEMENT_EVENT_TYPE EventType,
    _In_ phLibNfc_SE_List_t *pSEListEntry,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo
    );

VOID
NfcCxSEInterfacePurgeHCERecvQueue(
    _In_ PNFCCX_SE_INTERFACE SEInterface
    );

VOID
NfcCxSEInterfaceHandleHCEPacket(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ USHORT ConnectionId,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ USHORT DataLength
    );

NTSTATUS
NfcCxSEInterfaceValidateRequest(
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxSEInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchEnumEndpoints;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchGetNextEvent;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchSubscribeForEvent;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchSetCardEmulationMode;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchGetNfccCapabilities;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchGetRoutingTable;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchSetRoutingTable;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchHCERemoteRecv;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchHCERemoteSend;
NFCCX_SE_DISPATCH_HANDLER NfcCxSEInterfaceDispatchSetPowerMode;

NTSTATUS
NfcCxSEInterfaceCopyEventData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    );

BOOLEAN
NfcCxSEInterfaceMatchesEvent(
    _In_ CNFCPayload * EventPayload,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxSEInterfaceGetEventPayload(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_SE_List_t *pSEListEntry,
    _In_ SECURE_ELEMENT_EVENT_TYPE EventType,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _Outptr_ CNFCPayload **EventPayload
    );

BOOLEAN
NfcCxSEInterfaceCompleteRequestLocked(
    _In_ WDFQUEUE EventQueue,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength
    );

GUID
NfcCxSEInterfaceGetSecureElementId(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement
    );

BOOLEAN
NfcCxSEInterfaceGetSecureElementHandle(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ const GUID& guidSecureElementId,
    _Outptr_result_maybenull_ phLibNfc_Handle *phSecureElement
    );

#define NFCCX_SE_FLAG_SET_ROUTING_TABLE 0x2

NTSTATUS
NfcCxSEInterfaceValidateCardEmulationState(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO pMode,
    _Inout_ uint8_t& RtngTableCount,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable,
    _Out_ DWORD *pdwFlags
    );

NTSTATUS
NfcCxSEInterfaceSetCardEmulationMode(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ PSECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO pMode
    );

NTSTATUS
NfcCxSEInterfaceSetCardWiredMode(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId,
    _In_ BOOLEAN WiredMode
    );

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NTSTATUS
NfcCxSEInterfaceUpdateSEActivationMode(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId,
    _In_ const NFCCX_SE_POWER_SETTINGS& PowerSettings
    );

NTSTATUS
NfcCxSEInterfaceResetCard(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId
    );

NTSTATUS
NfcCxSEInterfaceGetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(uiRoutingTableSize, uiRoutingTableSize) PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable,
    _Inout_ uint32_t& uiRoutingTableSize
    );

NTSTATUS
NfcCxSEInterfaceVerifyRoutingTablePower(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phNfc_PowerState_t eRequestedPowerState,
    _Inout_ uint8_t& RtngTableCount,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable,
    _Out_ BOOLEAN* pbIsRoutingTableChanged
    );

NTSTATUS
NfcCxSEInterfaceGetActivationMode(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _Out_ phLibNfc_eSE_ActivationMode& eActivationMode
    );

NTSTATUS
NfcCxSEInterfaceValidateRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable
    );

NTSTATUS
NfcCxSEInterfaceConvertRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable
    );

NTSTATUS
NfcCxSEInterfaceGetSEInfo(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ const GUID& SecureElementId,
    _Out_ phLibNfc_SE_List_t* pSEInfo
    );

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NFCCX_SE_POWER_SETTINGS_LIST_ITEM*
NfcCxSEInterfaceGetPowerSettingsListItem(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId
    );

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NFCCX_SE_POWER_SETTINGS
NfcCxSEInterfaceGetPowerSettings(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId
    );

_Requires_lock_held_(SEInterface->SEPowerSettingsLock)
NTSTATUS
NfcCxSEInterfaceSetPowerSettings(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ const GUID& SecureElementId,
    _In_ const NFCCX_SE_POWER_SETTINGS& PowerSettings
    );

BOOLEAN FORCEINLINE
NfcCxSEInterfaceValidateEventType(
    _In_ SECURE_ELEMENT_EVENT_TYPE eEventType
    )
{
    return (eEventType == ExternalReaderArrival ||
            eEventType == ExternalReaderDeparture ||
            eEventType == ApplicationSelected ||
            eEventType == Transaction ||
            eEventType == HceActivated ||
            eEventType == HceDeactivated);
}

BOOLEAN FORCEINLINE
NfcCxSEInterfaceValidateEmulationMode(
    _In_ SECURE_ELEMENT_CARD_EMULATION_MODE eMode
    )
{
    return (eMode == EmulationOff ||
            eMode == EmulationOnPowerIndependent ||
            eMode == EmulationOnPowerDependent);
}

phLibNfc_eSE_ActivationMode FORCEINLINE
NfcCxSEInterfaceGetActivationMode(
    _In_ SECURE_ELEMENT_CARD_EMULATION_MODE eMode
    )
{
    switch (eMode) {
    case EmulationOnPowerDependent:
    case EmulationOnPowerIndependent:
        return phLibNfc_SE_ActModeOn;
    default:
        return phLibNfc_SE_ActModeOff;
    }
}

BOOLEAN FORCEINLINE
NfcCxSEInterfaceGetPowerState(
    _In_ SECURE_ELEMENT_CARD_EMULATION_MODE eMode,
    _In_ const phNfc_sDeviceCapabilities_t& Caps,
    _Out_ phNfc_PowerState_t *pPowerState
    )
{
    switch (eMode) {
    case EmulationOnPowerDependent:
    case EmulationOnPowerIndependent:
        pPowerState->bSwitchedOn = 0x1;
        pPowerState->bSwitchedOff = (eMode == EmulationOnPowerIndependent) && Caps.PowerStateInfo.SwitchOffState;
        pPowerState->bBatteryOff = (eMode == EmulationOnPowerIndependent) && Caps.PowerStateInfo.BatteryOffState;
        return TRUE;
    case EmulationOff:
        pPowerState->bSwitchedOn = 0x1;
        pPowerState->bSwitchedOff = pPowerState->bBatteryOff = 0x0;
        return TRUE;
    default:
        pPowerState->bSwitchedOn = 0x0;
        pPowerState->bSwitchedOff = pPowerState->bBatteryOff = 0x0;
        return FALSE;
    }
}

SECURE_ELEMENT_TYPE FORCEINLINE
NfcCxSEInterfaceGetSecureElementType(
    _In_ phLibNfc_SE_Type_t eType
    )
{
    switch (eType) {
    case phLibNfc_SE_Type_UICC:
        return External;
    case phLibNfc_SE_Type_eSE:
        return Integrated;
    case phLibNfc_SE_Type_DeviceHost:
        return DeviceHost;
    default:
        return Integrated;
    }
}

BOOLEAN FORCEINLINE
NfcCxSEInterfaceIsPowerStateEqual(
    _In_ phNfc_PowerState_t& Current,
    _In_ phNfc_PowerState_t& Next
    )
{
    return ((Current.bSwitchedOn == Next.bSwitchedOn) &&
            (Current.bSwitchedOff == Next.bSwitchedOff) &&
            (Current.bBatteryOff == Next.bBatteryOff));
}

GUID FORCEINLINE
NfcCxSEInterfaceGetSecureElementId(
    _In_ const phLibNfc_SE_List_t *pSEInfo
    )
{
    GUID secureElementId = GUID_NULL;

    switch (pSEInfo->eSE_Type) {
    case phLibNfc_SE_Type_UICC:
        secureElementId = UICC_SECUREELEMENT_ID;
        break;
    case phLibNfc_SE_Type_eSE:
        secureElementId = ESE_SECUREELEMENT_ID;
        break;
    case phLibNfc_SE_Type_DeviceHost:
        secureElementId = DH_SECUREELEMENT_ID;
        break;
    }

    return secureElementId;
}
