/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    fileObject.cpp

Abstract:

    Cx file Object implementation.

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "fileObject.tmh"

BOOLEAN
NfcCxEvtDeviceFileCreate(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing to a device.
    This callback is called in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters for create

Return Value:

    BOOLEAN

--*/
{
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    PNFCCX_FILE_CONTEXT fileContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PUNICODE_STRING fileName = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttrib;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    NFCCX_IMPERSONATION_CONTEXT impersonationContext = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Get the device context given the device handle.
    //
    fdoContext = NfcCxFdoGetContext(Device);
    fileContext = NfcCxFileGetContext(FileObject);

    ZeroMemory(fileContext, sizeof(*fileContext));
    fileContext->Signature = NFCCX_FILE_CONTEXT_SIGNATURE;
    InitializeListHead(&fileContext->ListEntry);
    InitializeListHead(&fileContext->SendListEntry);
    fileContext->FdoContext = fdoContext;
    fileContext->FileObject = FileObject;
    fileContext->Enabled = TRUE;
    fileContext->Role = ROLE_UNDEFINED;
    fileContext->pszTypes = NULL;
    fileContext->cchTypes = 0;

    //
    // Get the initiator process id
    //
    impersonationContext.ProcessId = WdfFileObjectGetInitiatorProcessId(fileContext->FileObject);
    if (0 == impersonationContext.ProcessId) {
        impersonationContext.ProcessId = WdfRequestGetRequestorProcessId(Request);
    }

    if (0 == impersonationContext.ProcessId) {
        TRACE_LINE(LEVEL_ERROR, "Couldn't get the requestor process ID");
        fileContext->IsAppContainerProcess = FALSE;
    }
    else {

        impersonationContext.FileContext = fileContext;

        //
        // In order to check the initiator process, we must
        // impersonate the caller.
        //
        status = WdfRequestImpersonate(Request,
                                        SECURITY_IMPERSONATION_LEVEL::SecurityImpersonation,
                                        NfcCxFileObjectImpersonate,
                                        (PVOID)&impersonationContext);
        if (!NT_SUCCESS(status)) {
            //
            // In the event that we failed to impersonate the calling process, this implies
            // that the calling process is not an app container process.
            //
            fileContext->IsAppContainerProcess = FALSE;
            TRACE_LINE(LEVEL_INFO, "Failed to impersonate initiator process %!STATUS!", status);
            status = STATUS_SUCCESS;
        }
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FileObject;

    status = WdfWaitLockCreate(&objectAttrib,
                                &fileContext->StateLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the PowerPolicy WaitLock, %!STATUS!", status);
        goto Done;
    }

    fileName = WdfFileObjectGetFileName(FileObject);
    status = NfcCxFileObjectDetectRole(fileContext,
                                       fileName);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to detect Role, %!STATUS!", status);
        goto Done;
    }

    if ((ROLE_PUBLICATION == fileContext->Role ||
        ROLE_SUBSCRIPTION == fileContext->Role ||
        ROLE_SMARTCARD == fileContext->Role)) {
        //
        // The above roles require the NFP radio to be on.
        //
        if (!NfcCxPowerIsAllowedNfp(fdoContext->Power)) {
            TRACE_LINE(LEVEL_ERROR, "NFP radio state is off, client role %!FILE_OBJECT_ROLE! not supported", fileContext->Role);
            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }
    }

    if ((ROLE_SECUREELEMENTEVENT == fileContext->Role ||
         ROLE_SECUREELEMENTMANAGER == fileContext->Role)) {
        //
        // The above roles require the SE to be supported and SE radio to be on.
        //
        if (!NfcCxPowerIsAllowedSE(fdoContext->Power)) {
            TRACE_LINE(LEVEL_ERROR, "SE radio state is off, client role %!FILE_OBJECT_ROLE! not supported", fileContext->Role);
            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }
    }

    if (ROLE_SUBSCRIPTION == fileContext->Role) {

        InitializeListHead(&fileContext->RoleParameters.Sub.SubscribedMessageQueue);
        fileContext->RoleParameters.Sub.SubscribedMessageQueueLength = 0;

        //
        // Create the manual dispatch queue
        //
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                                 WdfIoQueueDispatchManual);
        queueConfig.PowerManaged = WdfFalse;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
        objectAttrib.ParentObject = FileObject;

        status = WdfIoQueueCreate(fdoContext->Device,
                                    &queueConfig,
                                    &objectAttrib,
                                    &queue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed with Status code %!STATUS!", status);
            goto Done;
        }
        fileContext->RoleParameters.Sub.SubsMessageRequestQueue = queue;
    } else if (ROLE_PUBLICATION == fileContext->Role) {

        //
        // Create the publication buffer
        //
        fileContext->RoleParameters.Pub.PublicationBuffer = new CNFCProximityBuffer();
        if (NULL == fileContext->RoleParameters.Pub.PublicationBuffer) {
            TRACE_LINE(LEVEL_ERROR, "Failed to allocate the publication buffer");
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        //
        // Create the manual dispatch queue
        //
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                                 WdfIoQueueDispatchManual);
        queueConfig.PowerManaged = WdfFalse;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
        objectAttrib.ParentObject = FileObject;

        status = WdfIoQueueCreate(fdoContext->Device,
                                    &queueConfig,
                                    &objectAttrib,
                                    &queue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed with Status code %!STATUS!", status);
            goto Done;
        }
        fileContext->RoleParameters.Pub.SendMsgRequestQueue = queue;
    } else if (ROLE_SECUREELEMENTEVENT == fileContext->Role) {

        InitializeListHead(&fileContext->RoleParameters.SEEvent.EventQueue);
        fileContext->RoleParameters.SEEvent.EventQueueLength = 0;

        //
        // Create the manual dispatch queue
        //
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                                 WdfIoQueueDispatchManual);
        queueConfig.PowerManaged = WdfFalse;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
        objectAttrib.ParentObject = FileObject;

        status = WdfIoQueueCreate(fdoContext->Device,
                                    &queueConfig,
                                    &objectAttrib,
                                    &queue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed with Status code %!STATUS!", status);
            goto Done;
        }
        fileContext->RoleParameters.SEEvent.EventRequestQueue = queue;
    }
    else if (ROLE_SECUREELEMENTMANAGER == fileContext->Role) {

        InitializeListHead(&fileContext->RoleParameters.SEManager.PacketQueue);
        fileContext->RoleParameters.SEManager.PacketQueueLength = 0;

        //
        // Create the manual dispatch queue
        //
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                                    WdfIoQueueDispatchManual);
        queueConfig.PowerManaged = WdfFalse;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
        objectAttrib.ParentObject = FileObject;

        status = WdfIoQueueCreate(fdoContext->Device,
                                    &queueConfig,
                                    &objectAttrib,
                                    &queue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed with Status code %!STATUS!", status);
            goto Done;
        }
        fileContext->RoleParameters.SEManager.PacketRequestQueue = queue;
    }

    //
    // If this is a pub/sub role, create the client unresponsive detection timer
    //
    if (ROLE_SUBSCRIPTION == fileContext->Role ||
        ROLE_PUBLICATION == fileContext->Role ||
        ROLE_SECUREELEMENTEVENT == fileContext->Role) {

        WDF_TIMER_CONFIG  timerConfig;

        WDF_TIMER_CONFIG_INIT(&timerConfig,
                              NfcCxFileObjectUnresponsiveClientDetectionTimer
                              );

        timerConfig.AutomaticSerialization = TRUE;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
        objectAttrib.ParentObject = fileContext->FileObject;

        status = WdfTimerCreate(&timerConfig,
                                &objectAttrib,
                                &fileContext->UnresponsiveClientDetectionTimer);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "WdfTimerCreate failed with %!STATUS!", status);
            goto Done;
        }
    }

    //
    // If it is a subscription, insert the the client
    // into the proper list
    //
    if (ROLE_SUBSCRIPTION == fileContext->Role) {
        NfcCxNfpInterfaceAddSubscriptionClient(NfcCxFileObjectGetNfpInterface(fileContext),
                                               fileContext);
    }
    else if (ROLE_PUBLICATION == fileContext->Role) {
        //
        // Since we are now a pub, add the client to the pub and send list
        //
        NfcCxNfpInterfaceAddPublicationClient(NfcCxFileObjectGetNfpInterface(fileContext),
                                              fileContext);
    } else if (ROLE_SMARTCARD == fileContext->Role) {

        status = NfcCxSCInterfaceAddClient(NfcCxFileObjectGetScInterface(fileContext),
                                           fileContext);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to add a smartcard client %!STATUS!", status);
            goto Done;
        }
    } else if (ROLE_SECUREELEMENTEVENT == fileContext->Role ||
               ROLE_SECUREELEMENTMANAGER == fileContext->Role) {

        status = NfcCxSEInterfaceAddClient(NfcCxFileObjectGetSEInterface(fileContext),
                                           fileContext);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to add a SE client %!STATUS!", status);
            goto Done;
        }
    }
    else if (ROLE_EMBEDDED_SE == fileContext->Role) {
        status = NfcCxESEInterfaceAddClient(fdoContext->ESEInterface,
                                            fileContext);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to add a eSE smartcard client %!STATUS!", status);
            goto Done;
        }
    }


    TRACE_LINE(LEVEL_INFO, "Client Added");
    TRACE_LINE(LEVEL_INFO, "    Client Role = %!FILE_OBJECT_ROLE!",                     fileContext->Role);
    TRACE_LINE(LEVEL_INFO, "    Client TranslationType = %!TRANSLATION_TYPE_PROTOCOL!", fileContext->TranslationType);
    TRACE_LINE(LEVEL_INFO, "    Client Type = %s",                                      fileContext->pszTypes);
    TRACE_LINE(LEVEL_INFO, "    Client Tnf = %d",                                       fileContext->Tnf);

#ifdef EVENT_WRITE
    EventWriteNfpClientCreate(FileObject,
                              fileContext->Role,
                              fileContext->pszTypes,
                              fileContext->TranslationType,
                              fileContext->Tnf);
#endif

    if (NFC_CX_DEVICE_MODE_RAW == fdoContext->NfcCxClientGlobal->Config.DeviceMode)
    {
        // In RAW mode, we place the device in D0 and then just leave it there.
        // This is presumably because the hardware is being tested, so it is fine to leave the NFC Controller in
        // a powered up state..
        TRACE_LINE(LEVEL_INFO, "Raw device mode. Powering up");
        status = WdfDeviceStopIdle(fdoContext->Device, /*WaitForD0*/ TRUE);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_INFO, "WdfDeviceStopIdle failed, %!STATUS!", status);
            goto Done;
        }
    }
    else
    {
        switch (fileContext->Role)
        {
        case ROLE_SUBSCRIPTION:
        case ROLE_PUBLICATION:
        case ROLE_EMBEDDED_SE:
        {
            NFC_CX_POWER_REFERENCE_TYPE powerReferenceType = (fileContext->Role == ROLE_EMBEDDED_SE) ?
                NfcCxPowerReferenceType_ESe :
                NfcCxPowerReferenceType_Proximity;

            status = NfcCxPowerFileAddReference(fdoContext->Power, fileContext, powerReferenceType);
            if (!NT_SUCCESS(status))
            {
                TRACE_LINE(LEVEL_ERROR, "Failed to add a power policy reference, %!STATUS!", status);
                goto Done;
            }
            break;
        }
        }
    }

Done:
    if (!NT_SUCCESS(status))
    {
        if (NULL != fileContext)
        {
            if (NULL != fileContext->pszTypes) {
                free(fileContext->pszTypes);
                fileContext->pszTypes = NULL;
                fileContext->cchTypes = 0;
            }

            switch (fileContext->Role)
            {
                case ROLE_SUBSCRIPTION:
                {
                    NfcCxNfpInterfaceRemoveSubscriptionClient(NfcCxFileObjectGetNfpInterface(fileContext), fileContext);
                    break;
                }
                case ROLE_PUBLICATION:
                {
                    NfcCxNfpInterfaceRemovePublicationClient(NfcCxFileObjectGetNfpInterface(fileContext), fileContext);

                    if (NULL != fileContext->RoleParameters.Pub.PublicationBuffer)
                    {
                        delete fileContext->RoleParameters.Pub.PublicationBuffer;
                        fileContext->RoleParameters.Pub.PublicationBuffer = NULL;
                    }

                    break;
                }
                case ROLE_SMARTCARD:
                {
                    NfcCxSCInterfaceRemoveClient(NfcCxFileObjectGetScInterface(fileContext), fileContext);
                    break;
                }
                case ROLE_SECUREELEMENTEVENT:
                case ROLE_SECUREELEMENTMANAGER:
                {
                    NfcCxSEInterfaceRemoveClient(NfcCxFileObjectGetSEInterface(fileContext), fileContext);
                    break;
                }
                case ROLE_EMBEDDED_SE:
                {
                    NfcCxESEInterfaceRemoveClient(fdoContext->ESEInterface, fileContext);
                    break;
                }
            }
        }
    }

    WdfRequestComplete(Request, status);

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, NT_SUCCESS(status));

    return TRUE;
}

VOID
NfcCxFileObjectImpersonate (
    _In_ WDFREQUEST Request,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_IMPERSONATION_CONTEXT impersonationContext = (PNFCCX_IMPERSONATION_CONTEXT)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Request);

    status = NfcCxFileObjectCheckInitiatorProcess(impersonationContext);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to check the initiator process, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxEvtFileClose (
    _In_ WDFFILEOBJECT    FileObject
    )

/*++

Routine Description:

   EvtFileClose is called when all the handles represented by the FileObject
   is closed and all the references to FileObject is removed. This callback
   may get called in an arbitrary thread context instead of the thread that
   called CloseHandle. If you want to delete any per FileObject context that
   must be done in the context of the user thread that made the Create call,
   you should do that in the EvtDeviceCleanp callback.

Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

    None

--*/
{
    PNFCCX_FDO_CONTEXT   fdoContext;
    PNFCCX_FILE_CONTEXT  fileContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    fdoContext = NfcCxFdoGetContext(WdfFileObjectGetDevice(FileObject));
    fileContext = NfcCxFileGetContext(FileObject);

    TRACE_LINE(LEVEL_INFO, "Client Removed");
    TRACE_LINE(LEVEL_INFO, "    Client Role = %!FILE_OBJECT_ROLE!", fileContext->Role);
    TRACE_LINE(LEVEL_INFO, "    Client TranslationType = %!TRANSLATION_TYPE_PROTOCOL!", fileContext->TranslationType);
    TRACE_LINE(LEVEL_INFO, "    Client Type = %s", fileContext->pszTypes);
    TRACE_LINE(LEVEL_INFO, "    Client Tnf = %d", fileContext->Tnf);

#ifdef EVENT_WRITE
    EventWriteNfpClientDestroy(FileObject,
                               fileContext->Role,
                               fileContext->pszTypes,
                               fileContext->TranslationType,
                               fileContext->Tnf);
#endif

    switch (fileContext->Role) {
    case ROLE_SUBSCRIPTION:
        if (NULL != fileContext->FdoContext->NfpInterface)
        {
            NfcCxNfpInterfaceRemoveSubscriptionClient(fileContext->FdoContext->NfpInterface,
                                                      fileContext);
        }
        break;
    case ROLE_PUBLICATION:
        if (NULL != fileContext->FdoContext->NfpInterface)
        {
            NfcCxNfpInterfaceRemovePublicationClient(fileContext->FdoContext->NfpInterface,
                                                     fileContext);

            NfcCxNfpInterfaceRemoveSendClient(fileContext->FdoContext->NfpInterface,
                                              fileContext);
        }
        break;
    case ROLE_SMARTCARD:
        if (NULL != fileContext->FdoContext->SCInterface)
        {
            NfcCxSCInterfaceRemoveClient(fileContext->FdoContext->SCInterface,
                                         fileContext);
        }
        break;
    case ROLE_SECUREELEMENTEVENT:
    case ROLE_SECUREELEMENTMANAGER:
        if (NULL != fileContext->FdoContext->SEInterface)
        {
            NfcCxSEInterfaceRemoveClient(fileContext->FdoContext->SEInterface,
                                         fileContext);
        }
        break;
    case ROLE_EMBEDDED_SE:
        if (NULL != fdoContext->ESEInterface)
        {
            NfcCxESEInterfaceRemoveClient(fdoContext->ESEInterface,
                                          fileContext);
        }
        break;
    default:
        // Nothing to do.
        break;
    }


    WdfWaitLockAcquire(fileContext->StateLock, NULL);

    if (ROLE_SUBSCRIPTION == fileContext->Role) {
        //
        // Drain the received message queue
        //
        while (!IsListEmpty(&fileContext->RoleParameters.Sub.SubscribedMessageQueue)) {

            PLIST_ENTRY ple = RemoveHeadList(&fileContext->RoleParameters.Sub.SubscribedMessageQueue);
            fileContext->RoleParameters.Sub.SubscribedMessageQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }

    } else if (ROLE_PUBLICATION == fileContext->Role) {

        if (NULL != fileContext->RoleParameters.Pub.PublicationBuffer) {
            delete fileContext->RoleParameters.Pub.PublicationBuffer;
            fileContext->RoleParameters.Pub.PublicationBuffer = NULL;
        }

    } else if (ROLE_SECUREELEMENTEVENT == fileContext->Role) {
        //
        // Drain the received event queue
        //
        while (!IsListEmpty(&fileContext->RoleParameters.SEEvent.EventQueue)) {

            PLIST_ENTRY ple = RemoveHeadList(&fileContext->RoleParameters.SEEvent.EventQueue);
            fileContext->RoleParameters.SEEvent.EventQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }
    }
    else if (ROLE_SECUREELEMENTMANAGER == fileContext->Role) {
        //
        // Drain the received packet queue
        //
        while (!IsListEmpty(&fileContext->RoleParameters.SEManager.PacketQueue)) {

            PLIST_ENTRY ple = RemoveHeadList(&fileContext->RoleParameters.SEManager.PacketQueue);
            fileContext->RoleParameters.SEManager.PacketQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }
    }

    if (NULL != fileContext->pszTypes) {
        free(fileContext->pszTypes);
        fileContext->pszTypes = NULL;
        fileContext->cchTypes = 0;
    }

    WdfWaitLockRelease(fileContext->StateLock);

    //
    // Make sure the unresponsive client detection timer is stopped
    //
    if (NULL != fileContext->UnresponsiveClientDetectionTimer) {
        NfcCxFileObjectStopUnresponsiveClientDetectionTimer(fileContext, TRUE);
    }

    //
    // Cleanup any left over power references from this file object
    //
    NfcCxPowerCleanupFilePolicyReferences(fdoContext->Power, fileContext);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return;
}

NTSTATUS
NfcCxFileObjectCheckInitiatorProcess(
    _In_ PNFCCX_IMPERSONATION_CONTEXT ImpersonationContext
    )
/*++

Routine Description:

   This function checks the initiator process to determine
   if the initiator process is the application container.

Arguments:

    FileContext - Pointer to the file object context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hProcess = NULL;
    HANDLE hProcessToken = NULL;
    DWORD isAppContainer = 0;
    DWORD returnLength = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Open the process
    //
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,
                            FALSE,
                            ImpersonationContext->ProcessId);
    if (NULL == hProcess) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to get the process handle, %!STATUS!", status);
        goto Done;
    }

    if (!OpenProcessToken(hProcess,
                            TOKEN_QUERY,
                            &hProcessToken)) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to get the process token, %!STATUS!", status);
        goto Done;
    }

    if (!GetTokenInformation(hProcessToken,
                             TokenIsAppContainer,
                             &isAppContainer,
                             sizeof(isAppContainer),
                             &returnLength)) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to get the token information, %!STATUS!", status);
        goto Done;
    }

    if (sizeof(DWORD) != returnLength) {
        TRACE_LINE(LEVEL_ERROR, "Invalid token information retrieved");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    ImpersonationContext->FileContext->IsAppContainerProcess = (0 != isAppContainer);

    TRACE_LINE(LEVEL_INFO, "IsAppContainerProcess = %d",
                        ImpersonationContext->FileContext->IsAppContainerProcess);

Done:

    if (NULL != hProcessToken) {
        CloseHandle(hProcessToken);
        hProcessToken = NULL;
    }

    if (NULL != hProcess) {
        CloseHandle(hProcess);
        hProcess = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxFileObjectDetectRole(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_opt_ PUNICODE_STRING FileName
    )
/*++

Routine Description:

   Based on the file name, this function detects the role of the
   file object client.

Arguments:

    FileContext - Pointer to the file object context
    FileName - File name

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD cchFileName = 0;
    LPWSTR pszFileName;
    LPWSTR pszProtocol = NULL;
    LPWSTR pszProtocolEnd = NULL;
    DWORD cchProtocol = 0;
    LPWSTR pszSubTypeExt = nullptr;
    uint8_t chTnf = 0;
    TRANSLATION_TYPE_PROTOCOL translationType;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL == FileName ||
        0 == FileName->Length) {
        TRACE_LINE(LEVEL_INFO, "No file name provided, this is a configuration client");
        FileContext->Role = ROLE_CONFIGURATION;
        goto Done;
    }

    cchFileName = (FileName->Length / sizeof(WCHAR));
    if (MAX_FILE_NAME_LENGTH <= cchFileName)
    {
        TRACE_LINE(LEVEL_ERROR, "Invalid file name length, %!STATUS!", status);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    pszFileName = FileName->Buffer;

    //
    // Skip the first \
    //
    if (L'\\' == pszFileName[0]) {
        pszFileName++;
        cchFileName--;
    }

    if (L'\0' == pszFileName[0]) {
        FileContext->Role = ROLE_CONFIGURATION;
        goto Done;
    }

    if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                            min(PUBS_NAMESPACE_LENGTH, cchFileName),
                                            PUBS_NAMESPACE,
                                            PUBS_NAMESPACE_LENGTH,
                                            TRUE)) {
        FileContext->Role = ROLE_PUBLICATION;
        pszProtocol = pszFileName + PUBS_NAMESPACE_LENGTH;
        cchProtocol = cchFileName - PUBS_NAMESPACE_LENGTH;

    } else if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                                min(SUBS_NAMESPACE_LENGTH, cchFileName),
                                                SUBS_NAMESPACE,
                                                SUBS_NAMESPACE_LENGTH,
                                                TRUE)) {
        FileContext->Role = ROLE_SUBSCRIPTION;
        pszProtocol = pszFileName + SUBS_NAMESPACE_LENGTH;
        cchProtocol = cchFileName - SUBS_NAMESPACE_LENGTH;

    } else if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                                min(SMARTCARD_READER_NAMESPACE_LENGTH, cchFileName),
                                                SMARTCARD_READER_NAMESPACE,
                                                SMARTCARD_READER_NAMESPACE_LENGTH,
                                                TRUE)) {
        FileContext->Role = ROLE_SMARTCARD;
        goto Done;

    } else if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                                min(SEEVENTS_NAMESPACE_LENGTH, cchFileName),
                                                SEEVENTS_NAMESPACE,
                                                SEEVENTS_NAMESPACE_LENGTH,
                                                TRUE)) {
        FileContext->Role = ROLE_SECUREELEMENTEVENT;
        goto Done;

    } else if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                                min(SEMANAGER_NAMESPACE_LENGTH, cchFileName),
                                                SEMANAGER_NAMESPACE,
                                                SEMANAGER_NAMESPACE_LENGTH,
                                                TRUE)) {
        FileContext->Role = ROLE_SECUREELEMENTMANAGER;
        goto Done;

    } else if (CSTR_EQUAL == CompareStringOrdinal(pszFileName,
                                                  min(EMBEDDED_SE_NAMESPACE_LENGTH, cchFileName),
                                                  EMBEDDED_SE_NAMESPACE,
                                                  EMBEDDED_SE_NAMESPACE_LENGTH,
                                                  TRUE)) {
        FileContext->Role = ROLE_EMBEDDED_SE;
        goto Done;

    } else {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Done;
    }

    //
    // Determine the protocol type
    //
    if (0 == cchProtocol) {
        TRACE_LINE(LEVEL_ERROR, "Invalid protocol length");
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto Done;
    }

    NT_ASSERT(NULL != pszProtocol);

    pszProtocolEnd = (WCHAR *) wcschr(pszProtocol, CHAR_PROTOCOL_TERMINATOR);
    if (NULL != pszProtocolEnd) {
        cchProtocol = (DWORD)(pszProtocolEnd - pszProtocol);
    } else {
        cchProtocol = (DWORD)wcslen(pszProtocol);
    }

    if (cchProtocol > MAX_TYPE_LENGTH) {
        TRACE_LINE(LEVEL_ERROR, "Protocol portion too large, got %d, max %d", cchProtocol, MAX_TYPE_LENGTH);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    status = CNFCProximityBuffer::AnalyzeMessageType(pszProtocol,
                                                    &pszSubTypeExt,
                                                    (FileContext->Role == ROLE_PUBLICATION),
                                                    &chTnf,
                                                    &translationType);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to parse the message type, %!STATUS!", status);
        goto Done;
    }

    //
    // Set the type info
    //
    if (NULL != pszSubTypeExt) {
        status = NfcCxFileObjectSetType(FileContext,
                                        pszSubTypeExt);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to set the object type, %!STATUS!", status);
            goto Done;
        }
    }

    FileContext->TranslationType = translationType;
    FileContext->Tnf = chTnf;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFileObjectSetType(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ LPWSTR Type
    )
/*++

Routine Description:

    Sets the client type string.

Arguments:

    FileContext - Pointer to the file object context
    Type - The type string.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchWideStr = wcslen(Type);
    size_t cchNarrowStr = 0;
    PSTR narrowStr;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != FileContext->pszTypes){
        TRACE_LINE(LEVEL_ERROR, "Types already set");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    // In case of wkt:U or wkt:T cchWideStr will be 1 i.e. MIN_TYPE_LENGTH
    if (cchWideStr < MIN_TYPE_LENGTH ||
        cchWideStr > MAX_TYPE_LENGTH) {
        TRACE_LINE(LEVEL_ERROR, "Invalid type string");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    //
    // All future comparison takes the ansi version of the type string
    //
    status = NfcCxWideStringToNarrowString(cchWideStr,
                                           Type,
                                           &cchNarrowStr,
                                           &narrowStr);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to convert the type string to ansi");
        goto Done;
    }

    FileContext->pszTypes = narrowStr;
    FileContext->cchTypes = (UCHAR)cchNarrowStr;

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFileObjectNfpDisable(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   Disables the pub/subs associated with this file object.

Arguments:

    FileContext - Pointer to the file object context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (!NfcCxFileObjectIsPubSub(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Enable only applies to the pub sub clients");
        WdfWaitLockRelease(FileContext->StateLock);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (!FileContext->Enabled) {
        TRACE_LINE(LEVEL_ERROR, "Already disabled, %!STATUS!", status);
        WdfWaitLockRelease(FileContext->StateLock);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Drain and stop the pended request queues
    //
    if (ROLE_PUBLICATION == FileContext->Role) {
        WdfIoQueuePurgeSynchronously(FileContext->RoleParameters.Pub.SendMsgRequestQueue);
    } else if (ROLE_SUBSCRIPTION == FileContext->Role) {
        WdfIoQueuePurgeSynchronously(FileContext->RoleParameters.Sub.SubsMessageRequestQueue);
    }

    FileContext->Enabled = FALSE;
    WdfWaitLockRelease(FileContext->StateLock);

    //
    // Remove the power reference
    //
    status = NfcCxPowerFileRemoveReference(
        FileContext->FdoContext->Power,
        FileContext,
        NfcCxPowerReferenceType_Proximity);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set the power policy, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFileObjectNfpEnable(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   Enables the pub/subs associated with this file object.

Arguments:

    FileContext - Pointer to the file object context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (!NfcCxFileObjectIsPubSub(FileContext)) {
        TRACE_LINE(LEVEL_ERROR, "Enable only applies to the pub sub clients");
        WdfWaitLockRelease(FileContext->StateLock);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (FileContext->Enabled) {
        TRACE_LINE(LEVEL_ERROR, "Already enabled, %!STATUS!", status);
        WdfWaitLockRelease(FileContext->StateLock);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Restart the pended request queues
    //
    if (ROLE_PUBLICATION == FileContext->Role) {
        WdfIoQueueStart(FileContext->RoleParameters.Pub.SendMsgRequestQueue);
    } else if (ROLE_SUBSCRIPTION == FileContext->Role) {
        WdfIoQueueStart(FileContext->RoleParameters.Sub.SubsMessageRequestQueue);
    }

    FileContext->Enabled = TRUE;
    WdfWaitLockRelease(FileContext->StateLock);

    //
    // Add the power reference
    //
    status = NfcCxPowerFileAddReference(
        FileContext->FdoContext->Power,
        FileContext,
        NfcCxPowerReferenceType_Proximity);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to set the power policy, %!STATUS!", status);
        goto Done;
    }

Done:

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFileObjectValidateAndSetPayload(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_bytecount_(PayloadLength) PVOID Payload,
    _In_ size_t PayloadLength
    )
/*++

Routine Description:

    Validate the payload and set it unto the clients's payload buffer.

Arguments:

    FileContext - Pointer to the file object context
    Payload - The message payload
    PayloadLength - The message payload length.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (ROLE_PUBLICATION != FileContext->Role) {
        TRACE_LINE(LEVEL_ERROR, "Client not a publisher");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (TRANSLATION_TYPE_SETTAG_READONLY != FileContext->TranslationType &&
        NFP_MINIMUM_MESSAGE_PAYLOAD_SIZE > PayloadLength) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Input buffer.  Expected %I64x, got %I64x",
            NFP_MINIMUM_MESSAGE_PAYLOAD_SIZE,
            PayloadLength);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    //
    // Is there already a payload set?
    //
    if (NULL != FileContext->RoleParameters.Pub.PublicationBuffer->Get()) {
        TRACE_LINE(LEVEL_ERROR, "Publication already set");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (NFP_MAXIMUM_MESSAGE_PAYLOAD_SIZE < PayloadLength) {
        TRACE_LINE(LEVEL_ERROR, "Payload too large, got [0x%I64x], maximum [0x%x]",
                        PayloadLength,
                        NFP_MAXIMUM_MESSAGE_PAYLOAD_SIZE);
        status = STATUS_INVALID_BUFFER_SIZE;
        goto Done;
    }

    status = FileContext->RoleParameters.Pub.PublicationBuffer->InitializeWithMessagePayload(FileContext->Tnf,
                                                                         FileContext->TranslationType,
                                                                         FileContext->cchTypes,
                                                                         FileContext->pszTypes,
                                                                         (USHORT)PayloadLength,
                                                                         (PBYTE)Payload);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Payload initialization failed, %!STATUS!", status);
        goto Done;
    }

Done:

    WdfWaitLockRelease(FileContext->StateLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxFileObjectValidateAndSubscribeForEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ GUID& SEIdentifier,
    _In_ SECURE_ELEMENT_EVENT_TYPE eSEEventType
    )
/*++

Routine Description:

   Validate the subscribe for SE Event

Arguments:

    FileContext - Pointer to the file object context
    SEIdentifier - The secure element identifier
    eSEEventType - THe secure element event type

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(FileContext->StateLock, NULL);

    if (ROLE_SECUREELEMENTEVENT != FileContext->Role) {
        TRACE_LINE(LEVEL_ERROR, "Client not a SEEvent file handle");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    FileContext->RoleParameters.SEEvent.SEIdentifier = SEIdentifier;
    FileContext->RoleParameters.SEEvent.eSEEventType = eSEEventType;

Done:

    WdfWaitLockRelease(FileContext->StateLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

_Requires_lock_held_(FileContext->StateLock)
NTSTATUS
NfcCxFileObjectCompleteSentMessageRequestLocked(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
/*++

Routine Description:

   Dequeues a IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE request and complete it

Arguments:

    FileContext - Pointer to the file object context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST wdfRequest = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(ROLE_PUBLICATION == FileContext->Role);

    //
    // Get the next IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE request to see
    // if any of them matches the event
    //
    status = WdfIoQueueRetrieveNextRequest(FileContext->RoleParameters.Pub.SendMsgRequestQueue,
                                           &wdfRequest);
    if (!NT_SUCCESS(status)) {

        if (NFP_MAX_PUB_SUB_QUEUE_LENGTH <= FileContext->RoleParameters.Pub.SentMsgCounter) {
            TRACE_LINE(LEVEL_ERROR, "Too many queued messages, dropping event");
            status = STATUS_INVALID_DEVICE_STATE;
        } else if (FileContext->IsUnresponsiveClientDetected) {
            TRACE_LINE(LEVEL_ERROR, "Ignoring event, client is unresponsive");
            status = STATUS_INVALID_DEVICE_STATE;
        } else {
            FileContext->RoleParameters.Pub.SentMsgCounter++;
            TRACE_LINE(LEVEL_WARNING,
                "No requests pended, %!STATUS!, CurrentCounter=%d",
                status,
                FileContext->RoleParameters.Pub.SentMsgCounter);

            NfcCxFileObjectStartUnresponsiveClientDetectionTimer(FileContext);
        }
    } else {

        TRACE_LINE(LEVEL_INFO,
            "Completing request %p, with %!STATUS!, 0x%I64x", wdfRequest, status, 0);

#ifdef EVENT_WRITE
        EventWriteNfpGetNextTransmittedMsgStop(FileContext->FileObject, status);
#endif

        WdfRequestCompleteWithInformation(wdfRequest, status, 0);
        wdfRequest = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
NfcCxFileObjectUnresponsiveClientDetectionTimer(
    _In_ WDFTIMER UnresponsiveClientDetectionTimer
    )
/*++

Routine Description:

   This routine implements the timer callback to detect unresponsive clients.  It will
   fire after NFP_UNRESPONSIVE_CLIENT_TIMER_MS to see if there are any messages left
   in the queue that the client isn't reading.

Arguments:

    UnresponsiveClientDetectionTimer - The Timer handle.

Return Value:

    NTSTATUS

--*/
{
    PNFCCX_FILE_CONTEXT fileContext =
        NfcCxFileGetContext(WdfTimerGetParentObject(UnresponsiveClientDetectionTimer));

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(fileContext->StateLock, NULL);

    //
    // Drain the receive queue and mark the client as unresponsive
    //
    if (ROLE_SUBSCRIPTION == fileContext->Role) {

        while (!IsListEmpty(&fileContext->RoleParameters.Sub.SubscribedMessageQueue)) {
            PLIST_ENTRY ple = RemoveHeadList(&fileContext->RoleParameters.Sub.SubscribedMessageQueue);
            fileContext->RoleParameters.Sub.SubscribedMessageQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }

    } else if (ROLE_PUBLICATION == fileContext->Role) {
        fileContext->RoleParameters.Pub.SentMsgCounter = 0;
    } else if (ROLE_SECUREELEMENTEVENT == fileContext->Role) {

        while (!IsListEmpty(&fileContext->RoleParameters.SEEvent.EventQueue)) {
            PLIST_ENTRY ple = RemoveHeadList(&fileContext->RoleParameters.SEEvent.EventQueue);
            fileContext->RoleParameters.SEEvent.EventQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }
    }

    fileContext->IsUnresponsiveClientDetected = TRUE;

    WdfWaitLockRelease(fileContext->StateLock);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}
