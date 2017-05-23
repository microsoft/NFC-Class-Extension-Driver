/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxESE.h

Abstract:

    Embedded SE Interface declaration

Environment:

    User-mode Driver Framework

--*/

#define DEFAULT_HCI_TX_RX_TIME_OUT                 5000U /* 5 Secs */
#define SET_APDU_MODE_VALUE                        2U
#define DEFAULT_SCARD_IO_HEADER_SIZE               8U

FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED
NfcCxEmbeddedSEInterfaceIsIoctlSupported;

FN_NFCCX_DDI_MODULE_IODISPATCH
NfcCxEmbeddedSEInterfaceIoDispatch;

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchGetAttribute(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchGetState(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchSetAttribute(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeCurrentProtocolTypeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributePresentLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeIccTypeLocked(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceVerifyAndAddIsPresent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ WDFREQUEST Request
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchIsPresent(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceVerifyAndAddIsAbsent(
    _In_ PNFCCX_SE_INTERFACE SEInterface,
    _In_ WDFREQUEST Request
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchIsAbsent(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchSetProtocol(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchAttributeAtr(
    _In_ PNFCCX_SC_INTERFACE ScInterface,
    _In_opt_bytecount_(cbResultBuffer) PBYTE pbResultBuffer,
    _In_ size_t cbResultBuffer,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceSetEmulationModeForESE(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ CARD_EMULATION_MODE_INTERNAL eMode
    );

static VOID CALLBACK
NfcCxEmbeddedSEAPDUThread(
    _In_ PTP_CALLBACK_INSTANCE pInstance,
    _In_ PVOID pContext,
    _In_ PTP_WORK /*pWork*/
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceStart(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

VOID
NfcCxEmbeddedSEInterfaceDestroy(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceStop(
    _In_ PNFCCX_SC_INTERFACE ScInterface
    );

NTSTATUS NfcCxEmbeddedSEInterfaceCopyResponseData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    );

NTSTATUS NfcCxEmbeddedSEInterfaceCreate(_In_ PNFCCX_RF_INTERFACE RFInterface);

//
// Reference count in the fileObject providing exclusive file handling
//

NTSTATUS
NfcCxEmbeddedSEInterfaceAddClient(
    _In_ PNFCCX_SE_INTERFACE SeInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID
NfcCxEmbeddedSEInterfaceRemoveClient(
    _In_ PNFCCX_SE_INTERFACE SeInterface,
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxEmbeddedSEInterfaceDispatchSetPower(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
);