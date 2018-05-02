/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxDTA.cpp

Abstract:

    DTA Interface implementation

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxDTA.tmh"

typedef struct _NFCCX_DTA_DISPATCH_ENTRY {
    ULONG IoControlCode;
    size_t MinimumInputBufferLength;
    size_t MinimumOutputBufferLength;
    PFN_NFCCX_DTA_DISPATCH_HANDLER DispatchHandler;
} NFCCX_DTA_DISPATCH_ENTRY, *PNFCCX_DTA_DISPATCH_ENTRY;

NFCCX_DTA_DISPATCH_ENTRY
g_DTADispatch [] = {
    //
    // NFC DTA DDI
    //
    { IOCTL_NFCDTA_CONFIG_RF_DISCOVERY,                 sizeof(NFC_RF_DISCOVERY_CONFIG),            0,                                      NfcCxDTAInterfaceDispatchConfigRfDiscovery },
    { IOCTL_NFCDTA_REMOTE_DEV_GET_NEXT,                 0,                                          sizeof(NFC_REMOTE_DEV_INFO),            NfcCxDTAInterfaceDispatchRemoteDeviceGetNext },
    { IOCTL_NFCDTA_REMOTE_DEV_CONNECT,                  sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchRemoteDeviceConnect },
    { IOCTL_NFCDTA_REMOTE_DEV_DISCONNECT,               sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchRemoteDeviceDisconnect },
    { IOCTL_NFCDTA_REMOTE_DEV_TRANSCEIVE,               sizeof(NFC_REMOTE_DEV_SEND_INFO),           sizeof(NFC_REMOTE_DEV_RECV_INFO) +
                                                                                                    sizeof(DWORD),                          NfcCxDTAInterfaceDispatchRemoteDeviceTransceive },
    { IOCTL_NFCDTA_REMOTE_DEV_RECV,                     0,                                          sizeof(NFC_REMOTE_DEV_RECV_INFO),       NfcCxDTAInterfaceDispatchRemoteDeviceRecv },
    { IOCTL_NFCDTA_REMOTE_DEV_SEND,                     sizeof(NFC_REMOTE_DEV_SEND_INFO),           0,                                      NfcCxDTAInterfaceDispatchRemoteDeviceSend },
    { IOCTL_NFCDTA_REMOTE_DEV_CHECK_PRESENCE,           sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchRemoteDeviceCheckPresence },
    { IOCTL_NFCDTA_CONFIG_P2P_PARAM,                    sizeof(NFC_P2P_PARAM_CONFIG),               0,                                      NfcCxDTAInterfaceDispatchConfigP2pParam },
    { IOCTL_NFCDTA_SET_RF_CONFIG,                       4,                                          0,                                      NfcCxDTAInterfaceDispatchSetRfConfig },
    //
    // NDEF
    //
    { IOCTL_NFCDTA_REMOTE_DEV_NDEF_WRITE,               sizeof(NFC_REMOTE_DEV_SEND_INFO),           0,                                      NfcCxDTAInterfaceDispatchRemoteDevNdefWrite },
    { IOCTL_NFCDTA_REMOTE_DEV_NDEF_READ,                sizeof(NFC_REMOTE_DEV_HANDLE),              sizeof(NFC_REMOTE_DEV_RECV_INFO),       NfcCxDTAInterfaceDispatchRemoteDevNdefRead },
    { IOCTL_NFCDTA_REMOTE_DEV_NDEF_CONVERT_READ_ONLY,   sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchRemoteDevNdefConvertReadOnly },
    { IOCTL_NFCDTA_REMOTE_DEV_NDEF_CHECK,               sizeof(NFC_REMOTE_DEV_HANDLE),              sizeof(NFC_NDEF_INFO),                  NfcCxDTAInterfaceDispatchRemoteDevNdefCheck },
    //
    // LLCP
    //
    { IOCTL_NFCDTA_LLCP_CONFIG,                         sizeof(NFC_LLCP_CONFIG),                    0,                                      NfcCxDTAInterfaceDispatchLlcpConfig },
    { IOCTL_NFCDTA_LLCP_ACTIVATE,                       sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchLlcpActivate },
    { IOCTL_NFCDTA_LLCP_DEACTIVATE,                     sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchLlcpDeactivate },
    { IOCTL_NFCDTA_LLCP_DISCOVER_SERVICES,              sizeof(NFC_LLCP_SERVICE_DISCOVER_REQUEST),  NFC_LLCP_SERVICE_DISCOVER_SAP_HEADER,   NfcCxDTAInterfaceDispatchLlcpDiscoverServices },
    { IOCTL_NFCDTA_LLCP_LINK_STATUS_CHECK,              sizeof(NFC_REMOTE_DEV_HANDLE),              0,                                      NfcCxDTAInterfaceDispatchLlcpLinkStatusCheck },
    { IOCTL_NFCDTA_LLCP_GET_NEXT_LINK_STATUS,           0,                                          sizeof(NFC_LLCP_LINK_STATUS),           NfcCxDTAInterfaceDispatchLlcpGetNextLinkStatus },
    { IOCTL_NFCDTA_LLCP_SOCKET_CREATE,                  sizeof(NFC_LLCP_SOCKET_INFO),               sizeof(NFC_LLCP_SOCKET_HANDLE),         NfcCxDTAInterfaceDispatchLlcpSocketCreate },
    { IOCTL_NFCDTA_LLCP_SOCKET_CLOSE,                   sizeof(NFC_LLCP_SOCKET_HANDLE),             0,                                      NfcCxDTAInterfaceDispatchLlcpSocketClose },
    { IOCTL_NFCDTA_LLCP_SOCKET_BIND,                    sizeof(NFC_LLCP_SOCKET_SERVICE_INFO),       0,                                      NfcCxDTAInterfaceDispatchLlcpSocketBind },
    { IOCTL_NFCDTA_LLCP_SOCKET_LISTEN,                  sizeof(NFC_LLCP_SOCKET_HANDLE),             sizeof(NFC_LLCP_SOCKET_HANDLE),         NfcCxDTAInterfaceDispatchLlcpSocketListen },
    { IOCTL_NFCDTA_LLCP_SOCKET_ACCEPT,                  sizeof(NFC_LLCP_SOCKET_ACCEPT_INFO),        0,                                      NfcCxDTAInterfaceDispatchLlcpSocketAccept },
    { IOCTL_NFCDTA_LLCP_SOCKET_CONNECT,                 sizeof(NFC_LLCP_SOCKET_CONNECT_INFO),       0,                                      NfcCxDTAInterfaceDispatchLlcpSocketConnect },
    { IOCTL_NFCDTA_LLCP_SOCKET_DISCONNECT,              sizeof(NFC_LLCP_SOCKET_HANDLE),             0,                                      NfcCxDTAInterfaceDispatchLlcpSocketDisconnect },
    { IOCTL_NFCDTA_LLCP_SOCKET_RECV,                    sizeof(NFC_LLCP_SOCKET_HANDLE),             sizeof(NFC_LLCP_SOCKET_PAYLOAD),        NfcCxDTAInterfaceDispatchLlcpRecv },
    { IOCTL_NFCDTA_LLCP_SOCKET_RECV_FROM,               sizeof(NFC_LLCP_SOCKET_HANDLE),             sizeof(NFC_LLCP_SOCKET_PAYLOAD),        NfcCxDTAInterfaceDispatchLlcpRecvFrom },
    { IOCTL_NFCDTA_LLCP_SOCKET_SEND,                    sizeof(NFC_LLCP_SOCKET_PAYLOAD),            0,                                      NfcCxDTAInterfaceDispatchLlcpSocketSend },
    { IOCTL_NFCDTA_LLCP_SOCKET_SNED_TO,                 sizeof(NFC_LLCP_SOCKET_CL_PAYLOAD),         0,                                      NfcCxDTAInterfaceDispatchLlcpSocketSendTo },
    { IOCTL_NFCDTA_LLCP_SOCKET_GET_NEXT_ERROR,          sizeof(NFC_LLCP_SOCKET_HANDLE),             sizeof(NFC_LLCP_SOCKET_ERROR_INFO),     NfcCxDTAInterfaceDispatchLlcpGetNextError },
    //
    // SNEP Server
    //
    { IOCTL_NFCDTA_SNEP_INIT_SERVER,                    sizeof(NFC_SNEP_SERVER_INFO),               sizeof(NFC_SNEP_SERVER_HANDLE),         NfcCxDTAInterfaceDispatchSnepInitServer },
    { IOCTL_NFCDTA_SNEP_DEINIT_SERVER,                  sizeof(NFC_SNEP_SERVER_HANDLE),             0,                                      NfcCxDTAInterfaceDispatchSnepDeinitServer },
    { IOCTL_NFCDTA_SNEP_SERVER_GET_NEXT_CONNECTION,     sizeof(NFC_SNEP_SERVER_HANDLE),             sizeof(NFC_SNEP_SERVER_CONNECTION_HANDLE),
                                                                                                                                            NfcCxDTAInterfaceDispatchSnepServerGetNextConnection },
    { IOCTL_NFCDTA_SNEP_SERVER_ACCEPT,                  sizeof(NFC_SNEP_SERVER_ACCEPT_INFO),        0,                                      NfcCxDTAInterfaceDispatchSnepServerAccept },
    { IOCTL_NFCDTA_SNEP_SERVER_GET_NEXT_REQUEST,        sizeof(NFC_SNEP_SERVER_HANDLE),             sizeof(NFC_SNEP_SERVER_REQUEST),        NfcCxDTAInterfaceDispatchSnepServerGetNextRequest },
    { IOCTL_NFCDTA_SNEP_SERVER_SEND_RESPONSE,           NFC_SNEP_SERVER_RESPONSE_HEADER,            sizeof(NFC_SNEP_SERVER_RESPONSE_INFO),  NfcCxDTAInterfaceDispatchSnepServerSendResponse },
    //
    // SNEP Client
    //
    { IOCTL_NFCDTA_SNEP_INIT_CLIENT,                    sizeof(NFC_SNEP_CLIENT_INFO),               sizeof(NFC_SNEP_CLIENT_HANDLE),         NfcCxDTAInterfaceDispatchSnepInitClient },
    { IOCTL_NFCDTA_SNEP_DEINIT_CLIENT,                  sizeof(NFC_SNEP_CLIENT_HANDLE),             0,                                      NfcCxDTAInterfaceDispatchSnepDeinitClient },
    { IOCTL_NFCDTA_SNEP_CLIENT_PUT,                     NFC_SNEP_CLIENT_PUT_HEADER,                 0,                                      NfcCxDTAInterfaceDispatchSnepClientPut },
    { IOCTL_NFCDTA_SNEP_CLIENT_GET,                     NFC_SNEP_CLIENT_GET_HEADER,                 sizeof(NFC_SNEP_CLIENT_DATA_BUFFER),    NfcCxDTAInterfaceDispatchSnepClientGet },
    //
    // SE
    //
    { IOCTL_NFCDTA_SE_ENUMERATE,                        0,                                          sizeof(NFC_SE_LIST),                    NfcCxDTAInterfaceDispatchSeEnumerate },
    { IOCTL_NFCDTA_SE_SET_EMULATION_MODE,               sizeof(NFC_SE_EMULATION_MODE_INFO),         0,                                      NfcCxDTAInterfaceDispatchSeSetEmulationMode },
    { IOCTL_NFCDTA_SE_SET_ROUTING_TABLE,                sizeof(NFC_SE_ROUTING_TABLE),               0,                                      NfcCxDTAInterfaceDispatchSeSetRoutingTable },
    { IOCTL_NFCDTA_SE_GET_NEXT_EVENT,                   0,                                          sizeof(NFC_SE_EVENT_INFO),              NfcCxDTAInterfaceDispatchSeGetNextEvent },
};

//
// Local forward declarations
//
extern DWORD WINAPI
NfcCxDTAInterfaceLibNfcThread(
    _In_ LPVOID pContext
    );

_Requires_lock_held_(DTAInterface->DeviceLock)
extern NTSTATUS
NfcCxDTAInterfaceExecute(
    _Inout_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    );

NTSTATUS
NfcCxDTAInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_DTA_INTERFACE* PPDTAInterface
    )
/*++

Routine Description:

    This routine creates and initializes the DTA Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    PPDTAInterface - A pointer to a memory location to receive the allocated DTA interface

Return Value:

    NTSTATUS

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(FdoContext);
    UNREFERENCED_PARAMETER(PPDTAInterface);

    return STATUS_NOT_SUPPORTED;
#else
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttrib;
    phOsalNfc_Config_t config = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface = (PNFCCX_DTA_INTERFACE)malloc(sizeof(*dtaInterface));
    if (NULL == dtaInterface) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the DTA interface");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(dtaInterface, sizeof((*dtaInterface)));

    dtaInterface->FdoContext = FdoContext;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = dtaInterface->FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib, &dtaInterface->DeviceLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the device lock, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = dtaInterface->FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib, &dtaInterface->SocketLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the socket lock, %!STATUS!", status);
        goto Done;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = dtaInterface->FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib, &dtaInterface->SnepServerLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the SNEP server lock, %!STATUS!", status);
        goto Done;
    }

    dtaInterface->pLibNfcContext = (PNFCCX_LIBNFC_CONTEXT)malloc(sizeof(*dtaInterface->pLibNfcContext));
    if (NULL == dtaInterface->pLibNfcContext) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the LibNfc context");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(dtaInterface->pLibNfcContext, sizeof(*dtaInterface->pLibNfcContext));

    dtaInterface->pLibNfcContext->hNotifyCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == dtaInterface->pLibNfcContext->hNotifyCompleteEvent) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create notification complete event, %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize OSAL
    //
    config.pfnCallback = NfcCxDTAInterfaceLibNfcMessageHandler;
    config.pCallbackContext = dtaInterface;

    status = NfcCxNtStatusFromNfcStatus(phOsalNfc_Init(&config));
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize OSAL, %!STATUS!", status);
        goto Done;
    }

    dtaInterface->pLibNfcContext->LibNfcThreadId = config.dwCallbackThreadId;

    dtaInterface->LibConfig.bHciNwkPerNfcee = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_HCI_NETWORK_PER_NFCEE) != 0;
    dtaInterface->LibConfig.bNfceeActionNtf = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_NFCEE_ACTION_NTF) == 0;

    //
    // Initialize the pended request queue for remote dev get next
    //
    status = dtaInterface->RemoteDevGetNext.Initialize(dtaInterface->FdoContext, FALSE, NFCCX_MAX_REMOTE_DEV_QUEUE_LENGTH);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "RemoteDevGetNext failed initialization with Status code %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize the pended request queue for remote dev recv
    //
    status = dtaInterface->RemoteDevRecv.Initialize(dtaInterface->FdoContext, TRUE, NFCCX_MAX_REMOTE_DEV_RECV_QUEUE_LENGTH);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "RemoteDevRecv failed initialization with Status code %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize the pended request queue for SE get next event
    //
    status = dtaInterface->SeGetNextEvent.Initialize(dtaInterface->FdoContext, TRUE, NFCCX_MAX_SE_GET_NEXT_EVENT_QUEUE_LENGTH);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "SeGetNextEvent failed initialization with Status code %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize the pended request queue for LLCP link status
    //
    status = dtaInterface->LlcpLinkStatus.Initialize(dtaInterface->FdoContext, FALSE, NFCCX_MAX_LLCP_LINK_STATUS_QUEUE_LENGTH);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "LlcpLinkStatus failed initialization with Status code %!STATUS!", status);
        goto Done;
    }

    dtaInterface->bLlcpAutoActivate = FALSE;

    InitializeListHead(&dtaInterface->SocketContext);
    InitializeListHead(&dtaInterface->SnepServerContext);

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != dtaInterface) {
            NfcCxDTAInterfaceDestroy(dtaInterface);
            dtaInterface = NULL;
        }
    }

    *PPDTAInterface = dtaInterface;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
#endif
}

VOID
NfcCxDTAInterfaceDestroy(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
/*++

Routine Description:

    This routine cleans up the DTA Interface

Arguments:

    DTAInterface - A pointer to the DTAInterface to cleanup.

Return Value:

    None

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(DTAInterface);
#else
    //
    // Perform any cleanup associated with the DTAInterface
    //
    if (NULL != DTAInterface->pLibNfcContext) {

        phOsalNfc_DeInit();

        if (NULL != DTAInterface->pLibNfcContext->hNotifyCompleteEvent) {
            CloseHandle(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);
            DTAInterface->pLibNfcContext->hNotifyCompleteEvent = NULL;
        }

        free(DTAInterface->pLibNfcContext);
        DTAInterface->pLibNfcContext = NULL;
    }

    if (NULL != DTAInterface->DeviceLock) {
        WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

        DTAInterface->RemoteDevGetNext.Deinitialize();
        DTAInterface->RemoteDevRecv.Deinitialize();
        DTAInterface->SeGetNextEvent.Deinitialize();

        WdfWaitLockRelease(DTAInterface->DeviceLock);

        WdfObjectDelete(DTAInterface->DeviceLock);
        DTAInterface->DeviceLock = NULL;
    }

    if (NULL != DTAInterface->SocketLock) {
        WdfWaitLockAcquire(DTAInterface->SocketLock, NULL);
        //
        // Drain any leftover socket handles
        //
        while (!IsListEmpty(&DTAInterface->SocketContext)) {

            PLIST_ENTRY ple = RemoveHeadList(&DTAInterface->SocketContext);
            delete CNFCDtaSocketContext::FromListEntry(ple);
        }
        WdfWaitLockRelease(DTAInterface->SocketLock);

        WdfObjectDelete(DTAInterface->SocketLock);
        DTAInterface->SocketLock = NULL;
    }

    if (NULL != DTAInterface->SnepServerLock) {
        WdfWaitLockAcquire(DTAInterface->SnepServerLock, NULL);
        //
        // Drain any leftover socket handles
        //
        while (!IsListEmpty(&DTAInterface->SnepServerContext)) {

            PLIST_ENTRY ple = RemoveHeadList(&DTAInterface->SnepServerContext);
            delete CNFCDtaSnepServerContext::FromListEntry(ple);
        }
        WdfWaitLockRelease(DTAInterface->SnepServerLock);

        WdfObjectDelete(DTAInterface->SnepServerLock);
        DTAInterface->SnepServerLock = NULL;
    }

    //
    // Since the lock objects are parented to the device,
    // there are no needs to manually delete them here
    //
    if (DTAInterface->InterfaceCreated) {
        //
        // Disable the DTA interface
        //
        WdfDeviceSetDeviceInterfaceState(DTAInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCDTA,
                                         NULL,
                                         FALSE);

        TRACE_LINE(LEVEL_VERBOSE, "DTA interface disabled");
    }

    free(DTAInterface);
#endif
}

NTSTATUS
NfcCxDTAInterfaceStart(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
/*++

Routine Description:

    Start the DTA Interface

Arguments:

    DTAInterface - The DTA Interface

Return Value:

    NTSTATUS

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(DTAInterface);

    return STATUS_NOT_SUPPORTED;
#else
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!DTAInterface->FdoContext->DisablePowerManagerStopIdle)
    {
        //
        // When in test mode, we prevent the hardware from idle stopping
        //
        (void)WdfDeviceStopIdle(DTAInterface->FdoContext->Device, FALSE);
    }


    WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(DTAInterface, LIBNFC_INIT, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize LIBNFC, %!STATUS!", status);
        WdfWaitLockRelease(DTAInterface->DeviceLock);
        goto Done;
    }

    //
    // Enable DTA Mode
    //
    status = NfcCxDTAInterfaceExecute(DTAInterface, LIBNFC_DTA_MESSAGE, DTA_SET_MODE, TRUE, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set the DTA mode, %!STATUS!", status);
        WdfWaitLockRelease(DTAInterface->DeviceLock);
        goto Done;
    }

    WdfWaitLockRelease(DTAInterface->DeviceLock);

    //
    // Publish the DTA interface
    //
    if (!DTAInterface->InterfaceCreated) {
        status = WdfDeviceCreateDeviceInterface(DTAInterface->FdoContext->Device,
                                                (LPGUID) &GUID_DEVINTERFACE_NFCDTA,
                                                NULL);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to create the DTA device interface, %!STATUS!", status);
            goto Done;
        }

        WdfDeviceSetDeviceInterfaceState(DTAInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCDTA,
                                         NULL,
                                         TRUE);

        DTAInterface->InterfaceCreated = TRUE;

    } else {
        WdfDeviceSetDeviceInterfaceState(DTAInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCDTA,
                                         NULL,
                                         TRUE);
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
#endif
}

NTSTATUS
NfcCxDTAInterfaceStop(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
/*++

Routine Description:

    Stop the DTA Interface

Arguments:

    DTAInterface - The DTA Interface

Return Value:

    NTSTATUS

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(DTAInterface);

    return STATUS_NOT_SUPPORTED;
#else
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Disable DTA Mode
    //
    WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

    (void)NfcCxDTAInterfaceExecute(DTAInterface, LIBNFC_DTA_MESSAGE, DTA_SET_MODE, FALSE, NULL, NULL);
    (void)NfcCxDTAInterfaceExecute(DTAInterface, LIBNFC_DEINIT, NULL, NULL, NULL, NULL);

    WdfWaitLockRelease(DTAInterface->DeviceLock);

    if (DTAInterface->InterfaceCreated) {
        WdfDeviceSetDeviceInterfaceState(DTAInterface->FdoContext->Device,
                                         (LPGUID) &GUID_DEVINTERFACE_NFCDTA,
                                         NULL,
                                         FALSE);
    }

    if (!DTAInterface->FdoContext->DisablePowerManagerStopIdle)
    {
        //
        // Resume idling when exiting test mode
        //
        WdfDeviceResumeIdle(DTAInterface->FdoContext->Device);
    }


    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
#endif
}

BOOLEAN
NfcCxDTAInterfaceIsIoctlSupported(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This routine returns true if the provided IOCTL is supported by the
    module.

Arguments:

    FdoContext - The FDO Context
    IoControlCode - The IOCTL code to check

Return Value:

    TRUE - The IOCTL is supported by this module
    FALSE - The IOCTL is not supported by this module

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(FdoContext);
    UNREFERENCED_PARAMETER(IoControlCode);

    return FALSE;
#else
    ULONG i;

    UNREFERENCED_PARAMETER(FdoContext);

    for (i=0; i < ARRAYSIZE(g_DTADispatch); i++) {
        if (g_DTADispatch[i].IoControlCode == IoControlCode) {
            return TRUE;
        }
    }

    return FALSE;
#endif
}

NTSTATUS
NfcCxDTAInterfaceIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++

Routine Description:

    This is the first entry into the DTAInterface.  It validates and dispatches DTA request
    as appropriate.

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    OutputBufferLength - Length of the output buffer associated with the request.
    InputBufferLength - Length of the input buffer associated with the request.
    IoControlCode - IOCTL code.

Return Value:

  NTSTATUS.

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(FileContext);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    return STATUS_NOT_SUPPORTED;
#else
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext;
    WDFMEMORY outMem = {0};
    WDFMEMORY inMem = {0};
    PVOID     inBuffer = NULL;
    PVOID     outBuffer = NULL;
    size_t    sizeInBuffer = 0;
    size_t    sizeOutBuffer = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFileObjectGetFdoContext(FileContext);

    //
    // Verify we are in DTA mode
    //
    if ((NULL == fdoContext->DTAInterface) ||
        (!fdoContext->DTAInterface->InterfaceCreated))
    {
        TRACE_LINE(LEVEL_ERROR, "DTA interface is not created");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Get the request memory and perform the operation here
    //
    if (0 != OutputBufferLength) {
        status = WdfRequestRetrieveOutputMemory(Request, &outMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the output buffer, %!STATUS!", status);
            goto Done;
        }

        outBuffer = WdfMemoryGetBuffer(outMem, &sizeOutBuffer);
        NT_ASSERT(sizeOutBuffer == OutputBufferLength);
        NT_ASSERT(NULL != outBuffer);
    }

    if (0 != InputBufferLength) {
        status = WdfRequestRetrieveInputMemory(Request, &inMem);
        if(!NT_SUCCESS(status) ) {
            TRACE_LINE(LEVEL_ERROR, "Failed to retrieve the input buffer, %!STATUS!", status);
            goto Done;
        }

        inBuffer = WdfMemoryGetBuffer(inMem, &sizeInBuffer);
        NT_ASSERT(sizeInBuffer == InputBufferLength);
        NT_ASSERT(NULL != inBuffer);
    }

    //
    // Validate the request
    //
    status = NfcCxDTAInterfaceValidateRequest(IoControlCode,
                                             InputBufferLength,
                                             OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request validation failed, %!STATUS!", status);
        goto Done;
    }

    //
    // Dispatch the request
    //
    status = NfcCxDTAInterfaceDispatchRequest(FileContext,
                                             Request,
                                             IoControlCode,
                                             inBuffer,
                                             InputBufferLength,
                                             outBuffer,
                                             OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Request dispatch failed, %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
#endif
}

NTSTATUS
NfcCxDTAInterfaceValidateRequest(
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine validates the DTA request.

Arguments:

    IoControlCode - IOCTL code.
    InputBufferLength - Length of the input buffer associated with the request.
    OutputBufferLength - Length of the output buffer associated with the request.

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_DTADispatch); i++) {
        if (g_DTADispatch[i].IoControlCode == IoControlCode) {

            if (g_DTADispatch[i].MinimumInputBufferLength > InputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
                    g_DTADispatch[i].MinimumInputBufferLength,
                    InputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            if (g_DTADispatch[i].MinimumOutputBufferLength > OutputBufferLength) {
                TRACE_LINE(LEVEL_ERROR, "Invalid Output buffer.  Expected %I64x, got %I64x",
                    g_DTADispatch[i].MinimumOutputBufferLength,
                    OutputBufferLength);
                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            status = STATUS_SUCCESS;
            break;
        }
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA request to the appropriate handler.

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    IoControlCode - The Io Control Code of the DTA Request.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

  NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    USHORT i = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (i = 0; i < ARRAYSIZE(g_DTADispatch); i++) {
        if (g_DTADispatch[i].IoControlCode == IoControlCode) {

            status = g_DTADispatch[i].DispatchHandler(FileContext,
                                                      Request,
                                                      InputBuffer,
                                                      InputBufferLength,
                                                      OutputBuffer,
                                                      OutputBufferLength);
            break;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

CNFCDtaSocketContext::CNFCDtaSocketContext() :
    m_hSocket(NULL),
    m_pDTAInterface(NULL),
    m_bListenEnabled(FALSE)
{
    m_sWorkingBuffer.buffer = NULL;
    m_sWorkingBuffer.length = 0;

    InitializeListHead(&m_ListEntry);
}

CNFCDtaSocketContext::CNFCDtaSocketContext(
    _In_ NFC_LLCP_SOCKET_HANDLE hSocket,
    _In_ PNFCCX_DTA_INTERFACE pDTAInterface
    ) :
    m_hSocket(hSocket),
    m_pDTAInterface(pDTAInterface)
{
    m_sWorkingBuffer.buffer = NULL;
    m_sWorkingBuffer.length = 0;
    m_bListenEnabled = FALSE;

    InitializeListHead(&m_ListEntry);
}

CNFCDtaSocketContext::~CNFCDtaSocketContext()
{
    Deinitialize();
}

NTSTATUS CNFCDtaSocketContext::Initialize(
    _In_ NFC_LLCP_SOCKET_HANDLE hSocket,
    _In_ PNFCCX_DTA_INTERFACE pDTAInterface,
    _In_ DWORD WorkingBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (NULL != m_sWorkingBuffer.buffer) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    m_sWorkingBuffer.buffer = new BYTE[WorkingBufferSize];
    if (NULL == m_sWorkingBuffer.buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    m_sWorkingBuffer.length = WorkingBufferSize;
    m_hSocket = hSocket;
    m_pDTAInterface = pDTAInterface;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID CNFCDtaSocketContext::Deinitialize()
{
    m_hSocket = NULL;
    m_pDTAInterface = NULL;
    m_bListenEnabled = FALSE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != m_sWorkingBuffer.buffer) {
        delete m_sWorkingBuffer.buffer;
        m_sWorkingBuffer.buffer = NULL;
        m_sWorkingBuffer.length = 0;
    }

    m_SocketError.Deinitialize();
    m_SocketListen.Deinitialize();
    m_SocketReceive.Deinitialize();

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS CNFCDtaSocketContext::EnableSocketListen()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_SocketListen.Initialize(m_pDTAInterface->FdoContext,
                                       FALSE,
                                       NFCCX_MAX_LLCP_SOCKET_LISTEN_QUEUE_LENGTH);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initilaize socket listen with status %!STATUS!", status);
        goto Done;
    }

    m_bListenEnabled = TRUE;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS CNFCDtaSocketContext::EnableSocketError()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_SocketError.Initialize(m_pDTAInterface->FdoContext,
                                      FALSE,
                                      NFCCX_MAX_LLCP_SOCKET_ERROR_QUEUE_LENGTH);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initilaize socket error with status %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS CNFCDtaSocketContext::EnableSocketReceive()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_SocketReceive.Initialize(m_pDTAInterface->FdoContext,
                                        TRUE,
                                        NFCCX_MAX_REMOTE_DEV_RECV_QUEUE_LENGTH);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initilaize socket receive with status %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

CNFCDtaSnepServerContext::CNFCDtaSnepServerContext() :
    m_hSnepServer(NULL),
    m_pDTAInterface(NULL),
    m_pServerResponseInfo(NULL)
{
    m_sDataInbox.buffer = NULL;
    m_sDataInbox.length = 0;

    InitializeListHead(&m_ListEntry);
}

CNFCDtaSnepServerContext::CNFCDtaSnepServerContext(
    _In_ NFC_SNEP_SERVER_HANDLE hSnepServer,
    _In_ PNFCCX_DTA_INTERFACE pDTAInterface
    ) :
    m_hSnepServer(hSnepServer),
    m_pDTAInterface(pDTAInterface),
    m_pServerResponseInfo(NULL)
{
    m_sDataInbox.buffer = NULL;
    m_sDataInbox.length = 0;

    InitializeListHead(&m_ListEntry);
}

CNFCDtaSnepServerContext::~CNFCDtaSnepServerContext()
{
    Deinitialize();
}

NTSTATUS CNFCDtaSnepServerContext::Initialize(
    _In_ NFC_SNEP_SERVER_HANDLE hSnepServer,
    _In_ PNFCCX_DTA_INTERFACE pDTAInterface,
    _In_ DWORD InboxBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (NULL != m_sDataInbox.buffer) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    m_sDataInbox.buffer = new BYTE[InboxBufferSize];
    if (NULL == m_sDataInbox.buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    m_sDataInbox.length = InboxBufferSize;
    m_hSnepServer = hSnepServer;
    m_pDTAInterface = pDTAInterface;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID CNFCDtaSnepServerContext::Deinitialize()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_hSnepServer = NULL;
    m_pDTAInterface = NULL;

    if (NULL != m_sDataInbox.buffer) {
        delete m_sDataInbox.buffer;
        m_sDataInbox.buffer = NULL;
        m_sDataInbox.length = 0;
    }

    m_Connection.Deinitialize();
    m_Request.Deinitialize();

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS CNFCDtaSnepServerContext::EnableSnepConnection()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_Connection.Initialize(m_pDTAInterface->FdoContext,
                                     FALSE,
                                     NFCCX_MAX_SNEP_CONNECTION_QUEUE_LENGTH);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initilaize SNEP connections with status %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS CNFCDtaSnepServerContext::EnableSnepRequest()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_Request.Initialize(m_pDTAInterface->FdoContext,
                                  TRUE,
                                  NFCCX_MAX_SNEP_REQUEST_QUEUE_LENGTH);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initilaize SNEP request with status %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// NFC DTA DDI
//
NTSTATUS
NfcCxDTAInterfaceDispatchConfigRfDiscovery(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA config RF discovery ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_DISCOVER_CONFIG,
                                      (UINT_PTR)InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceGetNext(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA get next remote dev ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = dtaInterface->RemoteDevGetNext.ProcessRequest(FileContext,
                                                           Request,
                                                           OutputBuffer,
                                                           OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

    WdfWaitLockRelease(dtaInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceConnect(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device connect ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_CONNECT,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceDisconnect(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the remote device disconnect ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_DISCONNECT,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceTransceive(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device transceive ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_REMOTE_DEV_SEND_INFO remoteDevSendInfo = (PNFC_REMOTE_DEV_SEND_INFO) InputBuffer;
    PNFC_REMOTE_DEV_RECV_INFO remoteDevRecvInfo = (PNFC_REMOTE_DEV_RECV_INFO) (((BYTE*) OutputBuffer) + sizeof(DWORD));
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);

    _Analysis_assume_(NULL != OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (InputBufferLength  < offsetof(NFC_REMOTE_DEV_SEND_INFO, sSendBuffer) +
                             offsetof(NFC_DATA_BUFFER, pbBuffer) +
                             remoteDevSendInfo->sSendBuffer.cbBuffer) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    remoteDevRecvInfo->hRemoteDev = remoteDevSendInfo->hRemoteDev;
    remoteDevRecvInfo->sRecvBuffer.cbBuffer = (USHORT) (OutputBufferLength - (sizeof(DWORD) +
                                                                              offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) +
                                                                              offsetof(NFC_DATA_BUFFER, pbBuffer)));

    if (!NT_SUCCESS(NfcCxDTAInterfacePrepareTransceiveBuffer(dtaInterface,
                                                             remoteDevSendInfo,
                                                             remoteDevRecvInfo))) {
        TRACE_LINE(LEVEL_ERROR, "Failed preparing transceive buffer");
        status = STATUS_INVALID_PARAMETER;
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_TRANSCEIVE,
                                      (UINT_PTR) remoteDevSendInfo->hRemoteDev,
                                      NULL,
                                      NULL);

    remoteDevRecvInfo->sRecvBuffer.cbBuffer = (USHORT) dtaInterface->sTransceiveBuffer.sRecvData.length;
    *((DWORD*)OutputBuffer) = sizeof(DWORD)+
                              offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) +
                              offsetof(NFC_DATA_BUFFER, pbBuffer) +
                              remoteDevRecvInfo->sRecvBuffer.cbBuffer;
    if (NT_SUCCESS(status)) {
        usedBufferSize = *((DWORD*)OutputBuffer);
    }
    else {
        usedBufferSize = sizeof(DWORD);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceRecv(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device receive ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = dtaInterface->RemoteDevRecv.ProcessRequest(FileContext,
                                                        Request,
                                                        OutputBuffer,
                                                        OutputBufferLength);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

    WdfWaitLockRelease(dtaInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceSend(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device send ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    PNFC_REMOTE_DEV_SEND_INFO remoteDevSendInfo = (PNFC_REMOTE_DEV_SEND_INFO) InputBuffer;
    DWORD requiredBufferSize = offsetof(NFC_REMOTE_DEV_SEND_INFO, sSendBuffer) +
                               offsetof(NFC_DATA_BUFFER, pbBuffer) +
                               remoteDevSendInfo->sSendBuffer.cbBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (InputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size, actual %d, required %d", (DWORD)InputBufferLength, requiredBufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    dtaInterface->sSendBuffer.buffer = (uint8_t *) remoteDevSendInfo->sSendBuffer.pbBuffer;
    dtaInterface->sSendBuffer.length = (DWORD) remoteDevSendInfo->sSendBuffer.cbBuffer;

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_SEND,
                                      (UINT_PTR) remoteDevSendInfo->hRemoteDev,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDeviceCheckPresence(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device check presence ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(FileContext);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchConfigP2pParam(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA config P2P param ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_P2P_CONFIG,
                                      (UINT_PTR)InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSetRfConfig(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA config RF RAW ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_RF_CONFIG,
                                      (UINT_PTR)InputBuffer,
                                      (UINT_PTR)InputBufferLength,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// NDEF
//
NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDevNdefWrite(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote device NDEF write ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_REMOTE_DEV_SEND_INFO remoteDevSendInfo = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    remoteDevSendInfo = (PNFC_REMOTE_DEV_SEND_INFO) InputBuffer;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (InputBufferLength  < sizeof(*remoteDevSendInfo) + remoteDevSendInfo->sSendBuffer.cbBuffer) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    dtaInterface->sSendBuffer.buffer = remoteDevSendInfo->sSendBuffer.pbBuffer;
    dtaInterface->sSendBuffer.length = remoteDevSendInfo->sSendBuffer.cbBuffer;

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_NDEF_WRITE,
                                      (UINT_PTR) remoteDevSendInfo->hRemoteDev,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDevNdefRead(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote dev NDEF read ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_REMOTE_DEV_RECV_INFO remoteDevRecvInfo = (PNFC_REMOTE_DEV_RECV_INFO) (((BYTE*)OutputBuffer) + sizeof(DWORD));
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);
    _Analysis_assume_(NULL != OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (OutputBufferLength < sizeof(DWORD) +
                             offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) +
                             offsetof(NFC_DATA_BUFFER, pbBuffer) +
                             dtaInterface->sNdefInfo.dwActualMessageLength) {
        TRACE_LINE(LEVEL_ERROR, "Incorrect buffer size");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    dtaInterface->sNdefMsg.buffer = remoteDevRecvInfo->sRecvBuffer.pbBuffer;
    dtaInterface->sNdefMsg.length = (uint32_t) (OutputBufferLength - (sizeof(DWORD) +
                                                                      offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) +
                                                                      offsetof(NFC_DATA_BUFFER, pbBuffer)));

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_NDEF_READ,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    remoteDevRecvInfo->hRemoteDev = *((PNFC_REMOTE_DEV_HANDLE) InputBuffer);
    remoteDevRecvInfo->sRecvBuffer.cbBuffer = (USHORT) dtaInterface->sNdefMsg.length;

    *((DWORD*)OutputBuffer) = sizeof(DWORD)+
                              offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) +
                              offsetof(NFC_DATA_BUFFER, pbBuffer) +
                              remoteDevRecvInfo->sRecvBuffer.cbBuffer;
    if (NT_SUCCESS(status)) {
        usedBufferSize = *((DWORD*)OutputBuffer);
    }
    else {
        usedBufferSize = sizeof(DWORD);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDevNdefConvertReadOnly(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote dev NDEF convert read only ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_NDEF_CONVERT_READ_ONLY,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchRemoteDevNdefCheck(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA remote dev NDEF check ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_NDEF_INFO ndefInfo = (PNFC_NDEF_INFO) OutputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    // Buffer sizes are checked via the IOCTL dispatch table (at the top of the file).
    _Analysis_assume_(NULL != InputBuffer);
    _Analysis_assume_(OutputBufferLength >= sizeof(NFC_NDEF_INFO));

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_REMOTE_DEV_NDEF_CHECK,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    if (NT_SUCCESS(status)) {
        *ndefInfo = dtaInterface->sNdefInfo;
        //
        // Complete the request
        //
        WdfRequestCompleteWithInformation(Request, status, sizeof(*ndefInfo));

        //
        // Now that the request is in the holding queue or that we have completed it
        // return STATUS_PENDING so the request isn't completed by the calling method.
        //
        status = STATUS_PENDING;
    }

    WdfWaitLockRelease(dtaInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// LLCP
//
NTSTATUS
NfcCxDTAInterfaceDispatchLlcpConfig(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA the LLCP config ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_CONFIG,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpActivate(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP activate ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_ACTIVATE,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpDeactivate(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP deactivate ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_DEACTIVATE,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpDiscoverServices(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP discover services ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_LLCP_SERVICE_DISCOVER_REQUEST llcpServiceDiscoverRequest = NULL;
    PNFC_LLCP_SERVICE_DISCOVER_SAP llcpServiceDiscoverSap = NULL;
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);
    _Analysis_assume_(NULL != OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    llcpServiceDiscoverRequest = (PNFC_LLCP_SERVICE_DISCOVER_REQUEST)InputBuffer;
    llcpServiceDiscoverSap = (PNFC_LLCP_SERVICE_DISCOVER_SAP)OutputBuffer;

    if (llcpServiceDiscoverRequest->NumberOfEntries != OutputBufferLength - offsetof(NFC_LLCP_SERVICE_DISCOVER_SAP, SAPEntries)) {
        TRACE_LINE(LEVEL_ERROR, "Incorrect buffer size");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_DISCOVER_SERVICES,
                                      (UINT_PTR) InputBuffer,
                                      (UINT_PTR) OutputBuffer,
                                      NULL);

    if (NT_SUCCESS(status)) {
        usedBufferSize = offsetof(NFC_LLCP_SERVICE_DISCOVER_SAP, SAPEntries) +
                         llcpServiceDiscoverRequest->NumberOfEntries * sizeof(llcpServiceDiscoverSap->SAPEntries);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpLinkStatusCheck(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP link status check ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_LINK_STATUS_CHECK,
                                      (UINT_PTR) (*((PNFC_REMOTE_DEV_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpGetNextLinkStatus(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP get next link status ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = dtaInterface->LlcpLinkStatus.ProcessRequest(FileContext,
                                                         Request,
                                                         OutputBuffer,
                                                         OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

    WdfWaitLockRelease(dtaInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketCreate(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP create socket ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_LLCP_SOCKET_INFO llcpSocket = NULL;
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    llcpSocket = (PNFC_LLCP_SOCKET_INFO)InputBuffer;

    if (ConnectionOriented != llcpSocket->eSocketType && Connectionless != llcpSocket->eSocketType) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket type eSocketType %!NFC_LLCP_SOCKET_TYPE!", llcpSocket->eSocketType);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 == llcpSocket->sSocketOption.uMIUX || 0 == llcpSocket->sSocketOption.bRW) {
        TRACE_LINE(LEVEL_ERROR, "Invalid parameters MIUX %d, RW %d",
            llcpSocket->sSocketOption.uMIUX,
            llcpSocket->sSocketOption.bRW);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_CREATE,
                                      (UINT_PTR) InputBuffer,
                                      (UINT_PTR) OutputBuffer,
                                      NULL);

    if (NT_SUCCESS(status)) {
        usedBufferSize = sizeof(NFC_LLCP_SOCKET_HANDLE);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketClose(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP close socket ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE) InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_CLOSE,
                                      (UINT_PTR) llcpSocketHandle,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketBind(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP bind socket ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_LLCP_SOCKET_SERVICE_INFO llcpSocketServiceInfo = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    llcpSocketServiceInfo = (PNFC_LLCP_SOCKET_SERVICE_INFO)InputBuffer;

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketServiceInfo->hSocket);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketServiceInfo->hSocket);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_BIND,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketListen(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket listen ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE)InputBuffer;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_LISTEN,
                                      (UINT_PTR) llcpSocketHandle,
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketAccept(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket accept ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_LLCP_SOCKET_ACCEPT_INFO llcpSocketAcceptInfo = (PNFC_LLCP_SOCKET_ACCEPT_INFO)InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "socket handle 0x%p, MIUX %d, RW %d",
               llcpSocketAcceptInfo->hSocket,
               llcpSocketAcceptInfo->sSocketOption.uMIUX,
               llcpSocketAcceptInfo->sSocketOption.bRW);

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketAcceptInfo->hSocket);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketAcceptInfo->hSocket);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_ACCEPT,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketConnect(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket connect ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_LLCP_SOCKET_CONNECT_INFO llcpSocketConnectInfo = (PNFC_LLCP_SOCKET_CONNECT_INFO) InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketConnectInfo->hSocket);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketConnectInfo->hSocket);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_CONNECT,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketDisconnect(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket disconnect ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE) InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_DISCONNECT,
                                      (UINT_PTR) (*((PNFC_LLCP_SOCKET_HANDLE) InputBuffer)),
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpRecv(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP recv ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE) InputBuffer;
    CNFCDtaSocketContext* dtaSocketContext = (CNFCDtaSocketContext*) llcpSocketHandle;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (ConnectionOriented != dtaSocketContext->GetSocketType()) {
        TRACE_LINE(LEVEL_ERROR, "Recv is only valid for connection oriented socket type, current socket type %!NFC_LLCP_SOCKET_TYPE!",
                   dtaSocketContext->GetSocketType());
        status = STATUS_INVALID_PARAMETER;
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_RECV,
                                      (UINT_PTR) (*((PNFC_LLCP_SOCKET_HANDLE) InputBuffer)),
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpRecvFrom(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP recv from ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE) InputBuffer;
    CNFCDtaSocketContext* dtaSocketContext = (CNFCDtaSocketContext*)llcpSocketHandle;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (Connectionless != dtaSocketContext->GetSocketType()) {
        TRACE_LINE(LEVEL_ERROR, "RecvFrom is only valid for connectionless socket type, current socket type %!NFC_LLCP_SOCKET_TYPE!",
                   dtaSocketContext->GetSocketType());
        status = STATUS_INVALID_PARAMETER;
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_RECV,
                                      (UINT_PTR) (*((PNFC_LLCP_SOCKET_HANDLE) InputBuffer)),
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketSend(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket send ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    PNFC_LLCP_SOCKET_PAYLOAD llcpSocketPayload = (PNFC_LLCP_SOCKET_PAYLOAD)InputBuffer;
    CNFCDtaSocketContext* dtaSocketContext = (CNFCDtaSocketContext*)llcpSocketPayload->hSocket;
    DWORD requiredBufferSize = offsetof(NFC_LLCP_SOCKET_PAYLOAD, sPayload) +
                               offsetof(NFC_DATA_BUFFER, pbBuffer) +
                               llcpSocketPayload->sPayload.cbBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (InputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size, actual %d, required %d", (DWORD)InputBufferLength, requiredBufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketPayload->hSocket);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketPayload->hSocket);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (ConnectionOriented != dtaSocketContext->GetSocketType()) {
        TRACE_LINE(LEVEL_ERROR, "Send is only valid for connection oriented socket type, current socket type %!NFC_LLCP_SOCKET_TYPE!",
                   dtaSocketContext->GetSocketType());
        status = STATUS_INVALID_PARAMETER;
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_SEND,
                                      (UINT_PTR)InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpSocketSendTo(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP socket send to ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    PNFC_LLCP_SOCKET_CL_PAYLOAD llcpSocketPayload = (PNFC_LLCP_SOCKET_CL_PAYLOAD)InputBuffer;
    CNFCDtaSocketContext* dtaSocketContext = (CNFCDtaSocketContext*)llcpSocketPayload->hSocket;
    DWORD requiredBufferSize = offsetof(NFC_LLCP_SOCKET_CL_PAYLOAD, sPayload) +
                               offsetof(NFC_DATA_BUFFER, pbBuffer) +
                               llcpSocketPayload->sPayload.cbBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (InputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size, actual %d, required %d", (DWORD)InputBufferLength, requiredBufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketPayload->hSocket);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketPayload->hSocket);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (Connectionless != dtaSocketContext->GetSocketType()) {
        TRACE_LINE(LEVEL_ERROR, "SendTo is only valid for connectionless socket type, current socket type %!NFC_LLCP_SOCKET_TYPE!",
                   dtaSocketContext->GetSocketType());
        status = STATUS_INVALID_PARAMETER;
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_SEND,
                                      (UINT_PTR)InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchLlcpGetNextError(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA LLCP get next socket error ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_LLCP_SOCKET_HANDLE llcpSocketHandle = *(PNFC_LLCP_SOCKET_HANDLE) InputBuffer;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySocketHandle(dtaInterface, llcpSocketHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid socket handle 0x%p", llcpSocketHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_LLCP_SOCKET_GET_ERROR,
                                      (UINT_PTR) llcpSocketHandle,
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// SNEP Server
//
NTSTATUS
NfcCxDTAInterfaceDispatchSnepInitServer(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP init server ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_SERVER_INFO snepServerInfo = NULL;
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    snepServerInfo = (PNFC_SNEP_SERVER_INFO)InputBuffer;

    if (DefaultSnepServer != snepServerInfo->eServerType && ExtendedSnepServer != snepServerInfo->eServerType) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server type eServerType %!NFC_SNEP_SERVER_TYPE!", snepServerInfo->eServerType);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 == snepServerInfo->sSocketOption.uMIUX || 0 == snepServerInfo->sSocketOption.bRW) {
        TRACE_LINE(LEVEL_ERROR, "Invalid parameters MIUX %d, RW %d",
                   snepServerInfo->sSocketOption.uMIUX,
                   snepServerInfo->sSocketOption.bRW);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (MIN_INBOX_SIZE > snepServerInfo->usInboxSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid parameter InboxSize %d", snepServerInfo->usInboxSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_INIT,
                                      (UINT_PTR) InputBuffer,
                                      (UINT_PTR) OutputBuffer,
                                      NULL);

    if (NT_SUCCESS(status)) {
        usedBufferSize = sizeof(NFC_SNEP_SERVER_HANDLE);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepDeinitServer(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP deinit server ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    NFC_SNEP_SERVER_HANDLE snepServerHandle = *(PNFC_SNEP_SERVER_HANDLE)InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySnepServerHandle(dtaInterface, snepServerHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server handle 0x%p", snepServerHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_DEINIT,
                                      (UINT_PTR) snepServerHandle,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepServerGetNextConnection(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP server get next connection ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_SNEP_SERVER_HANDLE snepServerHandle = *(PNFC_SNEP_SERVER_HANDLE)InputBuffer;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySnepServerHandle(dtaInterface, snepServerHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server handle 0x%p", snepServerHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_GET_NEXT_CONNECTION,
                                      (UINT_PTR) snepServerHandle,
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepServerAccept(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP server accept ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_SERVER_ACCEPT_INFO snepServerAcceptInfo = (PNFC_SNEP_SERVER_ACCEPT_INFO)InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Snep Server handle 0x%p, MIU %d, RM %d",
               snepServerAcceptInfo->hSnepServer,
               snepServerAcceptInfo->sSocketOption.uMIUX,
               snepServerAcceptInfo->sSocketOption.bRW);

    status = NfcCxDTAInterfaceVerifySnepServerHandle(dtaInterface, snepServerAcceptInfo->hSnepServer);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server handle 0x%p", snepServerAcceptInfo->hSnepServer);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_ACCEPT,
                                      (UINT_PTR) InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepServerGetNextRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP server get next request ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    NFC_SNEP_SERVER_HANDLE snepServerHandle = *(PNFC_SNEP_SERVER_HANDLE)InputBuffer;
    NFCCX_DTA_REQUEST_CONTEXT requestContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    requestContext.FileContext = FileContext;
    requestContext.Request = Request;
    requestContext.OutputBuffer = OutputBuffer;
    requestContext.OutputBufferLength = OutputBufferLength;

    status = NfcCxDTAInterfaceVerifySnepServerHandle(dtaInterface, snepServerHandle);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server handle 0x%p", snepServerHandle);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_GET_NEXT_REQUEST,
                                      (UINT_PTR) snepServerHandle,
                                      (UINT_PTR) &requestContext,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        WdfWaitLockRelease(dtaInterface->DeviceLock);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepServerSendResponse(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP server send response ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_SERVER_RESPONSE_INFO snepServerRespnseEgress = (PNFC_SNEP_SERVER_RESPONSE_INFO)InputBuffer;
    PNFC_SNEP_SERVER_RESPONSE_INFO snepServerRespnseIngress = (PNFC_SNEP_SERVER_RESPONSE_INFO)(((BYTE*)OutputBuffer) + sizeof(DWORD));
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);

    _Analysis_assume_(NULL != OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (InputBufferLength  < offsetof(NFC_SNEP_SERVER_RESPONSE_INFO, sResponsePayload) +
                             offsetof(NFC_DATA_BUFFER, pbBuffer) +
                             snepServerRespnseEgress->sResponsePayload.cbBuffer) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceVerifySnepServerHandle(dtaInterface, snepServerRespnseEgress->hSnepServer);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server handle 0x%p", snepServerRespnseEgress->hSnepServer);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    snepServerRespnseIngress->hConnection = snepServerRespnseEgress->hConnection;
    snepServerRespnseIngress->sResponsePayload.cbBuffer = (USHORT)(OutputBufferLength - (sizeof(DWORD)+
                                                                                         offsetof(NFC_SNEP_SERVER_RESPONSE_INFO, sResponsePayload) +
                                                                                         offsetof(NFC_DATA_BUFFER, pbBuffer)));

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_SERVER_SEND_RESPONSE,
                                      (UINT_PTR)snepServerRespnseEgress,
                                      (UINT_PTR)snepServerRespnseIngress,
                                      NULL);

    *((DWORD*)OutputBuffer) = sizeof(DWORD)+
                              offsetof(NFC_SNEP_SERVER_RESPONSE_INFO, sResponsePayload) +
                              offsetof(NFC_DATA_BUFFER, pbBuffer) +
                              snepServerRespnseEgress->sResponsePayload.cbBuffer;
    if (NT_SUCCESS(status)) {
        usedBufferSize = *((DWORD*)OutputBuffer);
    }
    else {
        usedBufferSize = sizeof(DWORD);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// SNEP Client
//
NTSTATUS
NfcCxDTAInterfaceDispatchSnepInitClient(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP init client ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_CLIENT_INFO snepClientInfo = NULL;
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    snepClientInfo = (PNFC_SNEP_CLIENT_INFO)InputBuffer;

    if (DefaultSnepServer != snepClientInfo->eServerType && ExtendedSnepServer != snepClientInfo->eServerType) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP server type eServerType %!NFC_SNEP_SERVER_TYPE!", snepClientInfo->eServerType);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (0 == snepClientInfo->sSocketOption.uMIUX || 0 == snepClientInfo->sSocketOption.bRW) {
        TRACE_LINE(LEVEL_ERROR, "Invalid parameters MIUX %d, RW %d",
                   snepClientInfo->sSocketOption.uMIUX,
                   snepClientInfo->sSocketOption.bRW);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_CLIENT_INIT,
                                      (UINT_PTR) InputBuffer,
                                      (UINT_PTR) OutputBuffer,
                                      NULL);

    if (NT_SUCCESS(status)) {
        usedBufferSize = sizeof(NFC_SNEP_CLIENT_HANDLE);
    }
    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepDeinitClient(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP deinit client ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    NFC_SNEP_CLIENT_HANDLE snepClientHandle = *(PNFC_SNEP_CLIENT_HANDLE)InputBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_CLIENT_DEINIT,
                                      (UINT_PTR) snepClientHandle,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepClientPut(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP client put ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    PNFC_SNEP_CLIENT_PUT_INFO snepClienPutInfo = (PNFC_SNEP_CLIENT_PUT_INFO)InputBuffer;
    DWORD requiredBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != OutputBuffer);

    requiredBufferSize = offsetof(NFC_SNEP_CLIENT_PUT_INFO, sPutPayload) +
                         offsetof(NFC_DATA_BUFFER, pbBuffer) +
                         snepClienPutInfo->sPutPayload.cbBuffer;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (InputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size, actual %d, required %d", (DWORD)InputBufferLength, requiredBufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_CLIENT_PUT,
                                      (UINT_PTR) snepClienPutInfo,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSnepClientGet(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SNEP client get ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_CLIENT_GET_INFO snepClientGetInfo = (PNFC_SNEP_CLIENT_GET_INFO)InputBuffer;
    PNFC_SNEP_CLIENT_DATA_BUFFER snepClientDataBuffer = (PNFC_SNEP_CLIENT_DATA_BUFFER)(((BYTE*)OutputBuffer) + sizeof(DWORD));
    DWORD requiredBufferSize = 0;
    ULONG usedBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    _Analysis_assume_(NULL != OutputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    requiredBufferSize = offsetof(NFC_SNEP_CLIENT_GET_INFO, sGetPayload) +
                         offsetof(NFC_DATA_BUFFER, pbBuffer) +
                         snepClientGetInfo->sGetPayload.cbBuffer;

    if (InputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Invalid input buffer size, actual %d, required %d", (DWORD)InputBufferLength, requiredBufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    snepClientDataBuffer->cbBuffer = (USHORT)(OutputBufferLength - (sizeof(DWORD) +
                                                                    offsetof(NFC_DATA_BUFFER, pbBuffer)));

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SNEP_CLIENT_GET,
                                      (UINT_PTR) snepClientGetInfo,
                                      (UINT_PTR) snepClientDataBuffer,
                                      NULL);

    *((DWORD*)OutputBuffer) = sizeof(DWORD) +
                              offsetof(NFC_DATA_BUFFER, pbBuffer) +
                              snepClientDataBuffer->cbBuffer;
    if (NT_SUCCESS(status)) {
        usedBufferSize = *((DWORD*)OutputBuffer);
    }
    else {
        usedBufferSize = sizeof(DWORD);
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

//
// SE
//
NTSTATUS
NfcCxDTAInterfaceDispatchSeEnumerate(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SE enumerate ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    ULONG usedBufferSize = sizeof(DWORD);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(NULL != OutputBuffer);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL for EnumEndpoints");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SE_ENUMERATE,
                                      (UINT_PTR)OutputBuffer,
                                      (UINT_PTR)OutputBufferLength,
                                      NULL);

    if (NT_SUCCESS(status)) {
        usedBufferSize = *(DWORD*)OutputBuffer;
    }

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSeSetEmulationMode(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SE set emulation mode ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SE_EMULATION_MODE_INFO seEmulationMode = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);
    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    seEmulationMode = (PNFC_SE_EMULATION_MODE_INFO) InputBuffer;

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (NULL == seEmulationMode->hSecureElement) {
        TRACE_LINE(LEVEL_ERROR, "Cannot change emulation mode for DeviceHost SE");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SE_SET_EMULATION_MODE,
                                      (UINT_PTR)InputBuffer,
                                      NULL,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSeSetRoutingTable(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SE set routing table ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SE_ROUTING_TABLE routingTable = NULL;
    phLibNfc_RtngConfig_t rtngTable[MAX_ROUTING_TABLE_SIZE];
    DWORD expectedBufferLength = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(NULL != InputBuffer);

    dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;
    routingTable = (PNFC_SE_ROUTING_TABLE) InputBuffer;
    expectedBufferLength = offsetof(NFC_SE_ROUTING_TABLE, TableEntries) +
                           routingTable->NumberOfEntries * sizeof(NFC_SE_ROUTING_TABLE_ENTRY);

    if (0 != OutputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Output buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (expectedBufferLength > InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Expected input buffer length (%d) is greater than input buffer length (%d)",
                   expectedBufferLength,
                   (int) InputBufferLength);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = NfcCxDTAInterfaceValidateRoutingTable(routingTable);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxDTAInterfaceValidateRoutingTable Failed with status, %!STATUS!", status);
        goto Done;
    }

    status = NfcCxDTAInterfaceConvertRoutingTable(routingTable, rtngTable);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "NfcCxDTAInterfaceConvertRoutingTable Failed with status, %!STATUS!", status);
        goto Done;
    }

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    status = NfcCxDTAInterfaceExecute(dtaInterface,
                                      LIBNFC_DTA_MESSAGE,
                                      DTA_SE_SET_ROUTING_TABLE,
                                      (UINT_PTR)rtngTable,
                                      (UINT_PTR)routingTable->NumberOfEntries,
                                      NULL);

    WdfWaitLockRelease(dtaInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceDispatchSeGetNextEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

    This routine dispatches the DTA SE get next event ioctl

Arguments:

    FileContext - The File context
    Request - Handle to a framework request object.
    InputBuffer - The input buffer
    InputBufferLength - Length of the input buffer.
    OutputBufferLength - Length of the output buffer associated with the request.
    OuptutBuffer - The output buffer

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = NfcCxFileObjectGetFdoContext(FileContext)->DTAInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(InputBuffer);

    WdfWaitLockAcquire(dtaInterface->DeviceLock, NULL);

    if (0 != InputBufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Input buffer should be NULL");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = dtaInterface->SeGetNextEvent.ProcessRequest(FileContext,
                                                         Request,
                                                         OutputBuffer,
                                                         OutputBufferLength);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

    //
    // Now that the request is in the holding queue or that we have completed it
    // return STATUS_PENDING so the request isn't completed by the calling method.
    //
    status = STATUS_PENDING;

Done:

    WdfWaitLockRelease(dtaInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

FORCEINLINE
static BOOLEAN
NfcCxDTAInterfaceVerifyType2Tag(BYTE Sak)
{
    TRACE_LINE(LEVEL_INFO, "SAK %d", Sak);

    if (Sak == SAK_MIFARE_STD_1K ||
        Sak == SAK_MIFARE_STD_4K ||
        Sak == SAK_MIFARE_MINI) {
        Sak &= (uint8_t)SAK_CONFIG_MASKMFC;
    }
    else {
        Sak &= (uint8_t)SAK_CONFIG_MASK;
    }

    if (Sak == SAK_MIFARE_UL ||
        Sak == SAK_MIFARE_INFINEON_1K) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NfcCxDTAInterfaceMapRemoteDevInfo(
    _In_ phLibNfc_RemoteDevList_t* RemoteDev,
    _Out_ PNFC_REMOTE_DEV_INFO RemoteDevInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    phLibNfc_sRemoteDevInformation_t* remoteDevInfo = RemoteDev->psRemoteDevInfo;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    switch (remoteDevInfo->RemDevType) {

        case phNfc_eISO14443_A_PICC:
        case phNfc_eISO14443_4A_PICC:
        case phNfc_eISO14443_4B_PICC:
            RemoteDevInfo->eType = NfcType4Tag;
            RemoteDevInfo->eProtocol = PROTOCOL_ISO_DEP;

            if (phNfc_eISO14443_4B_PICC == remoteDevInfo->RemDevType) {
                RemoteDevInfo->eRFTech = NFC_RF_TECHNOLOGY_B;
                RemoteDevInfo->cbUid = PHNFC_PUPI_LENGTH;
                RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.Pupi, RemoteDevInfo->cbUid);
            }
            else {
                RemoteDevInfo->eRFTech = NFC_RF_TECHNOLOGY_A;
                RemoteDevInfo->cbUid = remoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength;
                RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.Iso14443A_Info.Uid, RemoteDevInfo->cbUid);
            }
            break;

        case phNfc_eISO14443_3A_PICC:
        case phNfc_eMifare_PICC:

            if (NfcCxDTAInterfaceVerifyType2Tag(remoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak)) {
                RemoteDevInfo->eType = NfcType2Tag;
                RemoteDevInfo->eProtocol = PROTOCOL_T2T;
                RemoteDevInfo->eRFTech = NFC_RF_TECHNOLOGY_A;
                RemoteDevInfo->cbUid = remoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength;
                RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.Iso14443A_Info.Uid, RemoteDevInfo->cbUid);
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case phNfc_eFelica_PICC:
            RemoteDevInfo->eType = NfcType3Tag;
            RemoteDevInfo->eProtocol = PROTOCOL_T3T;
            RemoteDevInfo->eRFTech = NFC_RF_TECHNOLOGY_F;
            RemoteDevInfo->cbUid = remoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength;
            RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.Felica_Info.IDm, RemoteDevInfo->cbUid);
            break;

        case phNfc_eJewel_PICC:
            RemoteDevInfo->eType = NfcType1Tag;
            RemoteDevInfo->eProtocol = PROTOCOL_T1T;
            RemoteDevInfo->eRFTech = NFC_RF_TECHNOLOGY_A;
            RemoteDevInfo->cbUid = remoteDevInfo->RemoteDevInfo.Jewel_Info.UidLength;
            RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.Jewel_Info.Uid, RemoteDevInfo->cbUid);
            break;

        case phNfc_eNfcIP1_Target:
        case phNfc_eNfcIP1_Initiator:
            RemoteDevInfo->eType = (phNfc_eNfcIP1_Target == remoteDevInfo->RemDevType) ? NfcIP1Target : NfcIP1Initiator;
            RemoteDevInfo->eProtocol = PROTOCOL_NFC_DEP;
            RemoteDevInfo->eRFTech = (NFC_LF_NFCID2_LENGTH == remoteDevInfo->RemoteDevInfo.NfcIP_Info.NFCID_Length) ? NFC_RF_TECHNOLOGY_F : NFC_RF_TECHNOLOGY_A;
            RemoteDevInfo->cbUid = remoteDevInfo->RemoteDevInfo.NfcIP_Info.NFCID_Length;
            RtlCopyMemory(RemoteDevInfo->pbUid, remoteDevInfo->RemoteDevInfo.NfcIP_Info.NFCID, RemoteDevInfo->cbUid);
            break;

        case phNfc_eISO14443_A_PCD:
        case phNfc_eISO14443_B_PCD:
            RemoteDevInfo->eType = NfcReader;
            RemoteDevInfo->eProtocol = PROTOCOL_ISO_DEP;
            RemoteDevInfo->eRFTech = (phNfc_eISO14443_A_PCD == remoteDevInfo->RemDevType) ? NFC_RF_TECHNOLOGY_A : NFC_RF_TECHNOLOGY_B;
            RemoteDevInfo->cbUid = 0;
            break;
        case phNfc_eKovio_PICC:
        case phNfc_eISO15693_PICC:
        case phNfc_eEpcGen2_PICC:
        case phNfc_eISO14443_BPrime_PICC:
        case phNfc_eISO14443_B_PICC:
        case phNfc_ePICC_DevType:
        case phNfc_eISO14443_BPrime_PCD:
        case phNfc_eFelica_PCD:
        case phNfc_eJewel_PCD:
        case phNfc_eISO15693_PCD:
        case phNfc_eEpcGen2_PCD:
        case phNfc_ePCD_DevType:
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxDTAInterfaceHandleRemoteDev(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ phLibNfc_RemoteDevList_t* RemoteDev
    )
{
    CNFCPayload* queuedRemoteDevInfo = NULL;
    PNFC_REMOTE_DEV_INFO remoteDevInfo = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

    queuedRemoteDevInfo = new CNFCPayload();
    if (NULL == queuedRemoteDevInfo) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    if (!NT_SUCCESS(queuedRemoteDevInfo->Initialize(NULL, sizeof(NFC_REMOTE_DEV_INFO)))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedRemoteDevInfo;
        goto Done;
    }

    remoteDevInfo = (PNFC_REMOTE_DEV_INFO)queuedRemoteDevInfo->GetPayload();
    remoteDevInfo->hRemoteDev = RemoteDev->hTargetDev;

    if (!NT_SUCCESS(NfcCxDTAInterfaceMapRemoteDevInfo(RemoteDev, remoteDevInfo))) {
        TRACE_LINE(LEVEL_ERROR, "Unsupported remote dev %d", RemoteDev->psRemoteDevInfo->RemDevType);
        delete queuedRemoteDevInfo;
        goto Done;
    }

    if (!NT_SUCCESS(DTAInterface->RemoteDevGetNext.ProcessPayload(queuedRemoteDevInfo))) {
        TRACE_LINE(LEVEL_ERROR, "Process payload failed");
        delete queuedRemoteDevInfo;
        goto Done;
    }

Done:

    WdfWaitLockRelease(DTAInterface->DeviceLock);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxDTAInterfacePrepareTransceiveBuffer(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_REMOTE_DEV_SEND_INFO remoteDevSendInfo,
    _In_ PNFC_REMOTE_DEV_RECV_INFO remoteDevRecvInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    phLibNfc_sRemoteDevInformation_t* remoteDevInfo = (phLibNfc_sRemoteDevInformation_t*) remoteDevSendInfo->hRemoteDev;

    RtlZeroMemory(&DTAInterface->sTransceiveBuffer, sizeof(DTAInterface->sTransceiveBuffer));

    switch (remoteDevInfo->RemDevType)
    {
    case phNfc_eJewel_PICC:
        DTAInterface->sTransceiveBuffer.cmd.JewelCmd = phNfc_eJewel_Raw;
        break;

    case phNfc_eMifare_PICC:
    case phNfc_eISO14443_3A_PICC:
        if (NfcCxDTAInterfaceVerifyType2Tag(remoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak)) {
            DTAInterface->sTransceiveBuffer.cmd.MfCmd = phNfc_eMifareRaw;
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            TRACE_LINE(LEVEL_ERROR, "Target type not supported for the transceive command %!phNfc_eRFDevType_t!", remoteDevInfo->RemDevType);
            goto Done;
        }
        break;

    case phNfc_eFelica_PICC:
        DTAInterface->sTransceiveBuffer.cmd.FelCmd = phNfc_eFelica_Raw;
        break;

    case phNfc_eISO14443_A_PICC:
    case phNfc_eISO14443_4A_PICC:
    case phNfc_eISO14443_4B_PICC:
        DTAInterface->sTransceiveBuffer.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
        break;

    case phNfc_eNfcIP1_Target:
        DTAInterface->sTransceiveBuffer.cmd.NfcIP1Cmd = phNfc_eNfcIP1_Raw;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Target type not supported for the transceive command %!phNfc_eRFDevType_t!", remoteDevInfo->RemDevType);
        goto Done;
    }

    DTAInterface->sTransceiveBuffer.timeout = remoteDevSendInfo->usTimeOut;
    DTAInterface->sTransceiveBuffer.sSendData.buffer = remoteDevSendInfo->sSendBuffer.pbBuffer;
    DTAInterface->sTransceiveBuffer.sSendData.length = remoteDevSendInfo->sSendBuffer.cbBuffer;
    DTAInterface->sTransceiveBuffer.sRecvData.buffer = remoteDevRecvInfo->sRecvBuffer.pbBuffer;
    DTAInterface->sTransceiveBuffer.sRecvData.length = remoteDevRecvInfo->sRecvBuffer.cbBuffer;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxDTAInterfaceHandleRemoteDevRecv(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ USHORT DataLength
    )
{
    CNFCPayload* queuedPacket = NULL;
    PNFC_REMOTE_DEV_RECV_INFO dataPacket = NULL;
    DWORD requiredLength = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

    queuedPacket = new CNFCPayload();
    if (NULL == queuedPacket) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    requiredLength = offsetof(NFC_REMOTE_DEV_RECV_INFO, sRecvBuffer) + offsetof(NFC_DATA_BUFFER, pbBuffer) + DataLength;
    if (!NT_SUCCESS(queuedPacket->Initialize(NULL, requiredLength))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedPacket;
        goto Done;
    }

    dataPacket = (PNFC_REMOTE_DEV_RECV_INFO) queuedPacket->GetPayload();
    dataPacket->hRemoteDev = RemoteDevHandle;
    dataPacket->sRecvBuffer.cbBuffer = DataLength;
    RtlCopyMemory(dataPacket->sRecvBuffer.pbBuffer, Data, DataLength);

    if (!NT_SUCCESS(DTAInterface->RemoteDevRecv.ProcessPayload(queuedPacket))) {
        TRACE_LINE(LEVEL_ERROR, "Process payload failed");
        delete queuedPacket;
        goto Done;
    }

Done:

    WdfWaitLockRelease(DTAInterface->DeviceLock);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxDTAInterfaceValidateRoutingTable(
    _In_ PNFC_SE_ROUTING_TABLE RoutingTable
    )
/*++

Routine Description:

    This internal helper routine is used to perform light validation of the
    routing table before it is sent to libnfc. Libnfc does validate other
    NCI requirement as uniqueness of the route protocol, technology, and aid
    route entries. It also validate the ranges for aid lengths, technology,
    and protocol.

    This routine makes sure that there are no nfc-dep routing rule.
    Also, make sure that AID length is OK

Arguments:

    pRoutingTable - Route table to be validate

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = (RoutingTable->NumberOfEntries > 0 &&
                       RoutingTable->NumberOfEntries <= MAX_ROUTING_TABLE_SIZE) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (USHORT i = 0; (i < RoutingTable->NumberOfEntries) && NT_SUCCESS(status); i++) {
        switch (RoutingTable->TableEntries[i].eRoutingType) {
        case RoutingTypeTech:
            break;
        case RoutingTypeProtocol:
            if (RoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType == phNfc_RfProtocolsNfcDepProtocol) {
                status = STATUS_INVALID_PARAMETER;
            }
            break;
        case RoutingTypeAid:
            if ((RoutingTable->TableEntries[i].AidRoutingInfo.cbAid < ISO_7816_MINIMUM_AID_LENGTH) ||
                (RoutingTable->TableEntries[i].AidRoutingInfo.cbAid > ISO_7816_MAXIMUM_AID_LENGTH)) {
                status = STATUS_INVALID_PARAMETER;
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceConvertRoutingTable(
    _In_ PNFC_SE_ROUTING_TABLE RoutingTable,
    _Out_ phLibNfc_RtngConfig_t* RtngTable
    )
/*++

Routine Description:

    This internal helper routine is used to convert route table format from
    MSFT defined to libnfc defined format

Arguments:

    RoutingTable - The pointer to DTA DDI routing table structure
    pRtngTable   - The LIBNFC route table

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    _Analysis_assume_(RoutingTable->NumberOfEntries > 0);

    for (uint8_t i = 0; (i < RoutingTable->NumberOfEntries) && NT_SUCCESS(status); i++) {

        switch (RoutingTable->TableEntries[i].eRoutingType) {
        case RoutingTypeTech:
            RtngTable[i].hSecureElement = RoutingTable->TableEntries[i].TechRoutingInfo.hSecureElement;
            RtngTable[i].Type = phNfc_LstnModeRtngTechBased;
            RtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = (phNfc_eRfTechnologies_t)RoutingTable->TableEntries[i].TechRoutingInfo.eRfTechType;
            RtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn  = BIT_AT_POSITION(RoutingTable->TableEntries[i].TechRoutingInfo.bPowerState, 1);
            RtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOff = BIT_AT_POSITION(RoutingTable->TableEntries[i].TechRoutingInfo.bPowerState, 2);
            RtngTable[i].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bBatteryOff  = BIT_AT_POSITION(RoutingTable->TableEntries[i].TechRoutingInfo.bPowerState, 3);
            break;

        case RoutingTypeProtocol:
            RtngTable[i].hSecureElement = RoutingTable->TableEntries[i].ProtoRoutingInfo.hSecureElement;
            RtngTable[i].Type = phNfc_LstnModeRtngProtocolBased;
            RtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = (phNfc_eRfProtocols_t)RoutingTable->TableEntries[i].ProtoRoutingInfo.eRfProtocolType;
            RtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn  = BIT_AT_POSITION(RoutingTable->TableEntries[i].ProtoRoutingInfo.bPowerState, 1);
            RtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOff = BIT_AT_POSITION(RoutingTable->TableEntries[i].ProtoRoutingInfo.bPowerState, 2);
            RtngTable[i].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bBatteryOff  = BIT_AT_POSITION(RoutingTable->TableEntries[i].ProtoRoutingInfo.bPowerState, 3);
            break;

        case RoutingTypeAid:
            RtngTable[i].hSecureElement = RoutingTable->TableEntries[i].AidRoutingInfo.hSecureElement;
            RtngTable[i].Type = phNfc_LstnModeRtngAidBased;
            RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.bAidSize = (uint8_t)RoutingTable->TableEntries[i].AidRoutingInfo.cbAid;
            RtlCopyMemory(RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.aAid, RoutingTable->TableEntries[i].AidRoutingInfo.pbAid, RoutingTable->TableEntries[i].AidRoutingInfo.cbAid);
            RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOn  = BIT_AT_POSITION(RoutingTable->TableEntries[i].AidRoutingInfo.bPowerState, 1);
            RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bSwitchedOff = BIT_AT_POSITION(RoutingTable->TableEntries[i].AidRoutingInfo.bPowerState, 2);
            RtngTable[i].LstnModeRtngValue.tAidBasedRtngValue.tPowerState.bBatteryOff  = BIT_AT_POSITION(RoutingTable->TableEntries[i].AidRoutingInfo.bPowerState, 3);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

#define EVT_TRANSACTION_AID     (0x81U)
#define EVT_TRANSACTION_PARAM   (0x82U)

NTSTATUS
NfcCxDTAInterfaceGetEventPayload(
    _In_ NFC_SE_HANDLE hSecureElement,
    _In_ SECURE_ELEMENT_EVENT_TYPE EventType,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _Outptr_ CNFCPayload **EventPayload
    )
/*++

Routine Description:

    This routine retrieves the secure element event payload

Arguments:

    hSecureElement - The handle to the secure element object
    EventType - The secure element event type
    pSeEvtInfo - The optional data for the event
    EventPayload - The event payload as expected by the client

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    CNFCPayload *payload = NULL;
    PNFC_SE_EVENT_INFO eventInfo = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *EventPayload = NULL;

    payload = new CNFCPayload();

    if (NULL == payload) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    if (!NT_SUCCESS(payload->Initialize(NULL, SECURE_ELEMENT_EVENT_INFO_HEADER))) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        delete payload;
        payload = NULL;
        goto Done;
    }

    eventInfo = (PNFC_SE_EVENT_INFO)payload->GetPayload();
    eventInfo->hSecureElement = hSecureElement;
    eventInfo->eEventType = EventType;
    eventInfo->cbEventData = 0;

    switch (EventType) {
    case ExternalReaderArrival:
    case ExternalReaderDeparture:
        break;

    case Transaction:
        //
        // The Transaction event from the UICC maps to this event
        // The Event Data for Transaction event contains the AID and PARAM fields
        // The AID and PARAM fields are in BER-TLV format as per ETSI
        //
        if (pSeEvtInfo->UiccEvtInfo.aid.length > UCHAR_MAX ||
            pSeEvtInfo->UiccEvtInfo.param.length > UCHAR_MAX) {
            NT_ASSERTMSG("The EVT_TRANSACTION should never be larger than UCHAR_MAX", FALSE);
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        if (pSeEvtInfo->UiccEvtInfo.aid.length != 0) {
            eventInfo->pbEventData[eventInfo->cbEventData++] = EVT_TRANSACTION_AID;
            eventInfo->pbEventData[eventInfo->cbEventData++] = (uint8_t)pSeEvtInfo->UiccEvtInfo.aid.length;
            RtlCopyMemory(&eventInfo->pbEventData[eventInfo->cbEventData], pSeEvtInfo->UiccEvtInfo.aid.buffer, pSeEvtInfo->UiccEvtInfo.aid.length);
            eventInfo->cbEventData += pSeEvtInfo->UiccEvtInfo.aid.length;
        }

        if (pSeEvtInfo->UiccEvtInfo.param.length != 0) {
            eventInfo->pbEventData[eventInfo->cbEventData++] = EVT_TRANSACTION_PARAM;
            eventInfo->pbEventData[eventInfo->cbEventData++] = (uint8_t)pSeEvtInfo->UiccEvtInfo.param.length;
            RtlCopyMemory(&eventInfo->pbEventData[eventInfo->cbEventData], pSeEvtInfo->UiccEvtInfo.param.buffer, pSeEvtInfo->UiccEvtInfo.param.length);
            eventInfo->cbEventData += pSeEvtInfo->UiccEvtInfo.param.length;
        }
        break;

    default:
        TRACE_LINE(LEVEL_WARNING, "No mapping for SE Event Type=%d", EventType);
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    *EventPayload = payload;
    payload = NULL;

Done:

    if (NULL != payload) {
        delete payload;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxDTAInterfaceHandleSeEvent(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_SE_EVENT_TYPE EventType,
    _In_ NFC_SE_HANDLE hSecureElement,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo
    )
{
    CNFCPayload* queuedEvent = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(DTAInterface->DeviceLock, NULL);

    if (!NT_SUCCESS(NfcCxDTAInterfaceGetEventPayload(hSecureElement,
                                                     EventType,
                                                     pSeEvtInfo,
                                                     &queuedEvent))) {
        goto Done;
    }

    if (!NT_SUCCESS(DTAInterface->SeGetNextEvent.ProcessPayload(queuedEvent))) {
        TRACE_LINE(LEVEL_ERROR, "Process payload failed");
        delete queuedEvent;
        goto Done;
    }

Done:
    WdfWaitLockRelease(DTAInterface->DeviceLock);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxDTAInterfaceHandleLlcpLinkStatus(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ phFriNfc_LlcpMac_eLinkStatus_t LlcpLinkStatus
    )
{
    CNFCPayload* queuedPacket = NULL;
    PNFC_LLCP_LINK_STATUS llcpLinkStatus = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    queuedPacket = new CNFCPayload();
    if (NULL == queuedPacket) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    if (!NT_SUCCESS(queuedPacket->Initialize(NULL, sizeof(NFC_LLCP_LINK_STATUS)))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedPacket;
        goto Done;
    }

    llcpLinkStatus = (PNFC_LLCP_LINK_STATUS)queuedPacket->GetPayload();
    *llcpLinkStatus = (phFriNfc_LlcpMac_eLinkActivated == LlcpLinkStatus) ? LinkActivated : LinkDeactivated;

    if (!NT_SUCCESS(DTAInterface->LlcpLinkStatus.ProcessPayload(queuedPacket))) {
        TRACE_LINE(LEVEL_ERROR, "Process payload failed");
        delete queuedPacket;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxDTAInterfaceVerifySocketHandle(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PLIST_ENTRY ple = &DTAInterface->SocketContext;
    CNFCDtaSocketContext* dtaSocketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(DTAInterface->SocketLock, NULL);

    if (IsListEmpty(&DTAInterface->SocketContext)) {
        TRACE_LINE(LEVEL_INFO, "There are no DTA sockets!");
        goto Done;
    }

    ple = ple->Flink;

    do {
        dtaSocketContext = CNFCDtaSocketContext::FromListEntry(ple);

        if (LlcpSocketHandle == dtaSocketContext) {
            status = STATUS_SUCCESS;
            goto Done;
        }

        ple = ple->Flink;

    } while (ple != &DTAInterface->SocketContext);

Done:
    WdfWaitLockRelease(DTAInterface->SocketLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxDTAInterfaceVerifySnepServerHandle(
_In_ PNFCCX_DTA_INTERFACE DTAInterface,
_In_ NFC_SNEP_SERVER_HANDLE SnepServerHandle
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PLIST_ENTRY ple = &DTAInterface->SnepServerContext;
    CNFCDtaSnepServerContext* dtaSnepServerContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(DTAInterface->SnepServerLock, NULL);

    if (IsListEmpty(&DTAInterface->SnepServerContext)) {
        TRACE_LINE(LEVEL_INFO, "There are no DTA SNEP servers!");
        goto Done;
    }

    ple = ple->Flink;

    do {
        dtaSnepServerContext = CNFCDtaSnepServerContext::FromListEntry(ple);

        if (SnepServerHandle == dtaSnepServerContext) {
            status = STATUS_SUCCESS;
            goto Done;
        }

        ple = ple->Flink;

    } while (ple != &DTAInterface->SnepServerContext);

Done:
    WdfWaitLockRelease(DTAInterface->SnepServerLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

