/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxTml.cpp

Abstract:

    This module implements the TML layer to adapt the NfcCoreLib to the
    class extension.

--*/

#include "NfcCxPch.h"
#include "NfcCxTml.tmh"

static VOID FORCEINLINE
NfcCxTmlInterfacePostLibNfcThreadMessage(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ DWORD Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    )
{
    if (NFC_CX_DEVICE_MODE_NCI == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        NfcCxPostLibNfcThreadMessage(FdoContext->RFInterface, Message, Param1, Param2, Param3, Param4);
    }
    else {
        NfcCxDTAPostLibNfcThreadMessage(FdoContext->DTAInterface, Message, Param1, Param2, Param3, Param4);
    }
}

static BOOLEAN FORCEINLINE
NfcCxTmlInterfaceProcessRequestAsync(
    _In_ PNFCCX_FDO_CONTEXT FdoContext
    )
{
    if (NFC_CX_DEVICE_MODE_NCI == FdoContext->NfcCxClientGlobal->Config.DeviceMode) {
        return FdoContext->RFInterface != NULL &&
               FdoContext->RFInterface->pLibNfcContext != NULL &&
               FdoContext->RFInterface->pLibNfcContext->LibNfcThreadId != GetCurrentThreadId();
    }
    else {
        return FdoContext->DTAInterface != NULL &&
               FdoContext->DTAInterface->pLibNfcContext != NULL &&
               FdoContext->DTAInterface->pLibNfcContext->LibNfcThreadId != GetCurrentThreadId();
    }
}

NTSTATUS
NfcCxTmlInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_TML_INTERFACE * PPTmlInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the Tml Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    PPTmlInterface - A pointer to a memory location to receive 
                    the allocated Tml interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES objectAttrib;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    (*PPTmlInterface) = (PNFCCX_TML_INTERFACE)malloc(sizeof((**PPTmlInterface)));
    if (NULL == (*PPTmlInterface)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the Tml interface");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory((*PPTmlInterface), sizeof((**PPTmlInterface)));

    (*PPTmlInterface)->FdoContext = FdoContext;
    InitializeListHead(&(*PPTmlInterface)->ReadQueue);
    InitializeListHead(&(*PPTmlInterface)->WriteQueue);
    InitializeListHead(&(*PPTmlInterface)->ReadNotificationQueue);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = (*PPTmlInterface)->FdoContext->Device;
    status = WdfWaitLockCreate(&objectAttrib, &(*PPTmlInterface)->QueueLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the WdfWaitLock, %!STATUS!", status);
        goto Done;
    }

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != (*PPTmlInterface)) {
            NfcCxTmlInterfaceDestroy((*PPTmlInterface));
            (*PPTmlInterface) = NULL;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxTmlInterfaceDestroy(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    )
/*++

Routine Description:

    This routine cleans up the Tml Interface

Arguments:

    TmlInterface - A pointer to the TmlInterface to cleanup.

Return Value:

    None

--*/
{
    NfcCxTml_DrainQueue(TmlInterface);
    free(TmlInterface);
}

VOID
NfcCxTml_DrainQueue(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    )
/*++

Routine Description:

    This routine will drain the Tml read queues.

Arguments:

    TmlInterface - A pointer to the TmlInterface to cleanup.

Return Value:

    None

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    if (NULL != TmlInterface->QueueLock) {
        WdfWaitLockAcquire(TmlInterface->QueueLock, NULL);
        while (!IsListEmpty(&TmlInterface->ReadQueue)) {
            PLIST_ENTRY ple;
            PNFCCX_TML_READ_QUEUE_ENTRY pReadQueueEntry;

            ple = RemoveHeadList(&TmlInterface->ReadQueue);
            pReadQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_READ_QUEUE_ENTRY, ListEntry);

            free(pReadQueueEntry);
        }

        while (!IsListEmpty(&TmlInterface->ReadNotificationQueue)) {
            PLIST_ENTRY ple = NULL;
            PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY pReadQueueEntry = NULL;

            ple = RemoveHeadList(&TmlInterface->ReadNotificationQueue);
            pReadQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY, ListEntry);

            free(pReadQueueEntry);
        }

        while(!IsListEmpty(&TmlInterface->WriteQueue)) {
            PLIST_ENTRY ple = NULL;
            PNFCCX_TML_WRITE_QUEUE_ENTRY pWriteQueueEntry = NULL;

            ple = RemoveHeadList(&TmlInterface->WriteQueue);
            pWriteQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_WRITE_QUEUE_ENTRY, ListEntry);
            InitializeListHead(&pWriteQueueEntry->ListEntry);
            
            free(pWriteQueueEntry);
        }
        WdfWaitLockRelease(TmlInterface->QueueLock);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxTmlInterfaceStart(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    )
/*++

Routine Description:

    Start the Tml Interface

Arguments:

    TmlInterface - The Tml Interface

Return Value:

    NTSTATUS

--*/
{
    TmlInterface->InterfaceState = INTERFACE_STATE_RUNNING;
    return STATUS_SUCCESS;
}

NTSTATUS
NfcCxTmlInterfaceStop(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    )
/*++

Routine Description:

    Stop the Tml Interface

Arguments:

    TmlInterface - The Tml Interface

Return Value:

    NTSTATUS

--*/
{
    TmlInterface->InterfaceState = INTERFACE_STATE_STOPPED;
    return STATUS_SUCCESS;
}

VOID
NfcCxTml_DeferredReadCompletion(
    _In_ LPVOID pContext
    )
{
    PNFCCX_TML_READ_QUEUE_ENTRY pReadQueueEntry;
    phTmlNfc_TransactInfo_t transactionInfo;

    pReadQueueEntry = (PNFCCX_TML_READ_QUEUE_ENTRY)pContext;

    RtlZeroMemory(&transactionInfo, sizeof(transactionInfo));

    transactionInfo.wLength = pReadQueueEntry->BufferLength;
    transactionInfo.pBuff = pReadQueueEntry->Buffer;
    transactionInfo.wStatus = NFCSTATUS_SUCCESS;
    
    TRACE_LINE(LEVEL_INFO, "TML Read Context %p, readQueueEntry 0x%p", 
                           pReadQueueEntry->Context, pReadQueueEntry);
    pReadQueueEntry->PfnReadCompletion(pReadQueueEntry->Context,
                                       &transactionInfo);

    if (pReadQueueEntry->hCompletionEvent != NULL) {
        SetEvent(pReadQueueEntry->hCompletionEvent);
    }
    else {
        //
        // Free the ReadQueue entry since no one is waiting on
        // the completion event.
        //
        free(pReadQueueEntry);
        pReadQueueEntry = NULL;
    }
}

VOID
NfcCxTml_PostWriteCompletion(
    _In_ PNFCCX_TML_INTERFACE TmlInterface
    )
{
    PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY pReadNotificationQueueEntry = NULL;
    PNFCCX_TML_READ_QUEUE_ENTRY pReadQueueEntry = NULL;
    PLIST_ENTRY pleReadQueue = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(TmlInterface->QueueLock, NULL);
    NT_ASSERT(TRUE == TmlInterface->IsWriteCompletionPending);
    TmlInterface->IsWriteCompletionPending = FALSE;

    //
    // Upper layer expects the following order of completion when a write is pended
    // 1. The thread on which the write is requested should complete first.
    // 2. The call back associated with the write should be completed next.
    // 3. Finally any responses(read) due to the write should be completed.
    //
    // If the above sequence is not followed, it will lead to corruption of the Send state machine.
    // Hence after the write is completed first address any reads that are queued to be processed.
    // Then process any writes that may be pended on the WriteQueue.
    //
    // If NOTEMPTY(ReadNotificationQueue) && NOTEMPTY(ReadQueue)
    //      Complete ENTRY(ReadQueue) with data from ENTRY(ReadNotificationQueue).
    // If the entry could not be completed because 
    //      BUFFERSIZE(ReadNotificationQueue) > BUFFERSIZE(ReadQueue) then discard
    //      ENTRY(ReadNotificationQueue) and check
    //      if additional entries are available in the ReadNotificationQueue
    //
    if (IsListEmpty(&TmlInterface->ReadQueue)) {
        goto ProcessWrite;
    }

    pleReadQueue = TmlInterface->ReadQueue.Flink;
    pReadQueueEntry = CONTAINING_RECORD(pleReadQueue, NFCCX_TML_READ_QUEUE_ENTRY, ListEntry);
        
    while(!IsListEmpty(&TmlInterface->ReadNotificationQueue)) {
        PLIST_ENTRY ple = NULL;
        
        ple = RemoveHeadList(&TmlInterface->ReadNotificationQueue);
        pReadNotificationQueueEntry = CONTAINING_RECORD(ple, 
                                                        NFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY, 
                                                        ListEntry);
        if (pReadQueueEntry->BufferLength < pReadNotificationQueueEntry->BufferLength) {
            TRACE_LINE(LEVEL_ERROR, "Received buffer is smaller than the buffer received from within the Read Notification queue");
            free(pReadNotificationQueueEntry);
            pReadNotificationQueueEntry = NULL;
            continue;
        }
        break;
    }

    if (NULL != pReadNotificationQueueEntry) {
        RtlCopyMemory(pReadQueueEntry->Buffer, 
                      pReadNotificationQueueEntry->Buffer, 
                      pReadNotificationQueueEntry->BufferLength);
        pReadQueueEntry->BufferLength = pReadNotificationQueueEntry->BufferLength;

        RemoveHeadList(&TmlInterface->ReadQueue);

        if (!IsListEmpty(&TmlInterface->ReadQueue)) {
            TRACE_LINE(LEVEL_WARNING, "There should only be one read pending in the ReadQueue");
            NT_ASSERT(IsListEmpty(&TmlInterface->ReadNotificationQueue));
            
            if (!IsListEmpty(&TmlInterface->ReadNotificationQueue)) {
                TRACE_LINE(LEVEL_ERROR, "ReadQueue and ReadNotificationQueue both have data, which is unexpected. Calling NfcCxDeviceSetFailed");
                MICROSOFT_TELEMETRY_ASSERT_MSG(false, "NfcCxTmlInvalidQueueState");

                NfcCxDeviceSetFailed(TmlInterface->FdoContext->Device);

                WdfWaitLockRelease(TmlInterface->QueueLock);
                goto Done;
            }
        }

        WdfWaitLockRelease(TmlInterface->QueueLock);

        TRACE_LINE(LEVEL_INFO, "Completing a queued read notification");
        NfcCxTml_DeferredReadCompletion(pReadQueueEntry);

        WdfWaitLockAcquire(TmlInterface->QueueLock, NULL);
        goto ProcessWrite;
    }

ProcessWrite:

    if (!IsListEmpty(&TmlInterface->WriteQueue) &&
        (FALSE == TmlInterface->IsWriteCompletionPending)) {
        PLIST_ENTRY ple = NULL;
        PNFCCX_TML_WRITE_QUEUE_ENTRY pWriteQueueEntry = NULL;

        ple = RemoveHeadList(&TmlInterface->WriteQueue);
        pWriteQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_WRITE_QUEUE_ENTRY, ListEntry);
        InitializeListHead(&pWriteQueueEntry->ListEntry);

        TmlInterface->IsWriteCompletionPending = TRUE;
        WdfWaitLockRelease(TmlInterface->QueueLock);
        
        NfcCxTml_Write(pWriteQueueEntry);
        goto Done;
    
    }
    
    WdfWaitLockRelease(TmlInterface->QueueLock);

Done:
    if (pReadNotificationQueueEntry) {
        free(pReadNotificationQueueEntry);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

}

VOID
NfcCxTml_DeferredWriteCompletion(
    _In_ LPVOID pContext
    )
{
    phTmlNfc_TransactInfo_t transactionInfo;
    PNFCCX_TML_WRITE_CONTEXT writeContext = NULL;
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    WDFREQUEST request = (WDFREQUEST)pContext; 

    writeContext = NfcCxTmlWriteGetContext(request);
    fdoContext = NfcCxFdoGetContext(writeContext->Device);

    RtlZeroMemory(&transactionInfo, sizeof(transactionInfo));

    transactionInfo.wLength = writeContext->Length;
    transactionInfo.wStatus = NT_SUCCESS(writeContext->Status) ? 
                                         NFCSTATUS_SUCCESS : 
                                         NFCSTATUS_BOARD_COMMUNICATION_ERROR;

    TRACE_LINE(LEVEL_INFO, "TML Write Context %p", writeContext->Context);
    writeContext->PfnWriteCompletion(writeContext->Context,
                                     &transactionInfo);

    if (writeContext->hCompletionEvent != NULL) {
        SetEvent(writeContext->hCompletionEvent);
    }

    WdfObjectDelete(request);

    //
    // Now that the write has completed, check if there is a read that needs
    // to be completed.
    //
    NfcCxTml_PostWriteCompletion(fdoContext->TmlInterface);
}

VOID
NfcCxTml_WriteCompletion(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
    )
/*++

Routine Description:

    This routine signals the completion of a client write request.  This function will
    call LIBNFC's completion callback routine.

Arguments:

    Request - The WDF Request
    Target - The Io Target of the WdfRequest
    Params - The completion parameters
    Context - The Write request Context

Return Value:

    None

--*/
{
    PNFCCX_TML_WRITE_CONTEXT writeContext = NULL;
    PNFCCX_FDO_CONTEXT fdoContext = NULL;
    DWORD dwWait = 0;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    writeContext = NfcCxTmlWriteGetContext(Request);
    fdoContext = NfcCxFdoGetContext(writeContext->Device);

    writeContext->Length = (USHORT)Params->IoStatus.Information;
    writeContext->Status = Params->IoStatus.Status;

    //
    // Decrement the ReferenceCount. If this is 0, then the completion routine
    // owns completing the write. If it is not 0, then the thread issuing the write has
    // still not completed and will take care of completing the write to the LIBNFC layer.
    //
    if (InterlockedDecrement(&writeContext->ReferenceCount) != 0) {
        TRACE_LINE(LEVEL_INFO, "TML invoking write callback directly %p", writeContext->Context);
        goto Done;
    }

    if (NfcCxTmlInterfaceProcessRequestAsync(fdoContext)) {
        writeContext->hCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (writeContext->hCompletionEvent != NULL) {
            //
            // Take a reference on the request so that even if NfcCxTml_DeferredWriteCompletion
            // deletes the request, writeContext->hCompletionEvent is still valid.
            //
            WdfObjectReference(Request);
        }
    }

    NfcCxTmlInterfacePostLibNfcThreadMessage(fdoContext,
                                             PH_OSALNFC_DEFERRED_CALLBACK,
                                             (UINT_PTR)NfcCxTml_DeferredWriteCompletion,
                                             (UINT_PTR)Request,
                                             NULL,
                                             NULL);
    
    if (writeContext->hCompletionEvent != NULL) {
        dwWait = WaitForSingleObject(writeContext->hCompletionEvent, MAX_CALLBACK_TIMEOUT);

        if (dwWait != WAIT_OBJECT_0) {
            if (dwWait == WAIT_FAILED) {
                dwWait = GetLastError();
            }

            TRACE_LINE(LEVEL_ERROR, "Timer for write completion callback timed out. Error: 0x%08X", dwWait);
            MICROSOFT_TELEMETRY_ASSERT_MSG(false, "NfcCxTmlPostWriteCallbackTimeout");

            NfcCxDeviceSetFailed(fdoContext->Device);
        }

        CloseHandle(writeContext->hCompletionEvent);
        WdfObjectDereference(Request);
    }

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxTml_Write(
    _In_ PNFCCX_TML_WRITE_QUEUE_ENTRY WriteQueueEntry
    )
/*++

Routine Description:

    This routine is called from the TML callback routine to
    signal a Write request.  The implementation of this function
    will create a WDF request and forward it through the CX to the
    client driver.

Arguments:

    WriteQueueEntry - contains Request which in turn contains the information
                      to be written to the controller.

Return Value:

    None

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_TML_WRITE_CONTEXT writeContext = NULL;
    PNFCCX_FDO_CONTEXT fdoContext = WriteQueueEntry->TmlInterface->FdoContext;
    WDFREQUEST request = WriteQueueEntry->Request;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    writeContext = NfcCxTmlWriteGetContext(request);

    NT_ASSERT(IsListEmpty(&WriteQueueEntry->ListEntry));
    NT_ASSERT(NULL != writeContext);

    //
    // ReferenceCount is initially set to 1. Increment the reference count by 1 (to 2) 
    // before sending it to the controller. 
    // If the call to EvtNfcCxWriteNciPacket succeeds then there are 2 conditions that can occur.
    //    1. The NfcCxTml_WriteCompletion gets called before EvtNfcCxWriteNciPacket returns. In 
    //       this case, this function handles the completion. NfcCxTml_WriteCompletion will
    //       decrement the ReferenceCount by 1 (to 1). When it gets decremented again
    //       by this function, it drops down to 0 and NfcCxTml_DeferredWriteCompletion is invoked
    //    2. The EvtNfcCxWriteNciPacket call completes before NfcCxTml_WriteCompletion, in which
    //       case this function decrements ReferenceCount by 1 (to 1). NfcCxTml_WriteCompletion
    //       is responsible for calling the NfcCxTml_DeferredWriteCompletion in this case.
    //
    // The call to EvtNfcCxWriteNciPacket fails and so the NfcCxTml_WriteCompletion will not be called.
    // ReferenceCount is decremented by 1 (to 1) in the failure path. The final cleanup of this
    // function will decrement ReferenceCount again by 1 (to 0) and call NfcCxTml_DeferredWriteCompletion.
    //
    InterlockedIncrement(&writeContext->ReferenceCount);

    //
    // Forward the request to the class extension client driver
    //
    if (!WdfRequestSend(request,
                        CxProxyWdfDeviceGetSelfIoTarget(fdoContext->Device),
                        NULL)) {
        status = WdfRequestGetStatus(request);
        TRACE_LINE(LEVEL_ERROR, "Failed to write the Nci Packet to the CX, %!STATUS!", status);
        writeContext->Status = STATUS_UNSUCCESSFUL;
        InterlockedDecrement(&writeContext->ReferenceCount);
        goto Done;
    }

    status = STATUS_PENDING;

Done:

    if (InterlockedDecrement(&writeContext->ReferenceCount) == 0) {
        //
        // If the ReferenceCount is 0 and status is SUCCESS, then
        // it implies that the completion routine has been called. The completion
        // routine would not have completed the read. 
        //
        NfcCxTmlInterfacePostLibNfcThreadMessage(fdoContext,
                                                 PH_OSALNFC_DEFERRED_CALLBACK,
                                                 (UINT_PTR)NfcCxTml_DeferredWriteCompletion,
                                                 (UINT_PTR)request,
                                                 NULL,
                                                 NULL);
    }

    free(WriteQueueEntry);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

WDFREQUEST
NfcCxTml_PrepareWriteRequest(
    _In_ PNFCCX_TML_INTERFACE TmlInterface,
    _In_bytecount_(BufferLength) PUCHAR Buffer,
    _In_ USHORT BufferLength,
    _In_ pphTmlNfc_TransactCompletionCb_t PfnWriteCompletion,
    _In_opt_ PVOID Context
    )
/*++

Routine Description:

    Creates the write request and context associated with it.

Arguments:

    TmlInterface - A pointer to the TmlInterface
    Buffer - The Buffer to write
    BufferLength - The Buffer size
    PfnWriteCompletion - The LibNfc write completion routine
    Context - The LibNfc write completion context

Return Value:

    WDFREQUEST

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_TML_WRITE_CONTEXT writeContext = NULL;
    WDFMEMORY memory;
    WDF_OBJECT_ATTRIBUTES requestAttrib;
    WDF_OBJECT_ATTRIBUTES memoryAttrib;
    WDFREQUEST request = NULL;
    NCI_PACKET_HEADER nciHeader = {0};
    PNFCCX_FDO_CONTEXT fdoContext = TmlInterface->FdoContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttrib, NFCCX_TML_WRITE_CONTEXT);
    requestAttrib.ParentObject = fdoContext->Device;

    status = WdfRequestCreate(&requestAttrib,
                              CxProxyWdfDeviceGetSelfIoTarget(fdoContext->Device),
                              &request);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the write request, %!STATUS!", status);
        goto Done;
    }

    writeContext = NfcCxTmlWriteGetContext(request);
    writeContext->Device = fdoContext->Device;
    writeContext->Context = Context;
    writeContext->PfnWriteCompletion = PfnWriteCompletion;
    writeContext->ReferenceCount = 1;
    writeContext->hCompletionEvent = NULL;

    TRACE_LINE(LEVEL_INFO, "TML Write Context %p", Context);

    if (FALSE == NciPacketHeaderGetFromBuffer(Buffer,
                                              (UCHAR)BufferLength,
                                              &nciHeader)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to parse the Nci Packet Header");
    }

    WdfRequestSetCompletionRoutine(request, NfcCxTml_WriteCompletion, NULL);

    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttrib);
    memoryAttrib.ParentObject = request;

    status = WdfMemoryCreatePreallocated(&memoryAttrib,
                                         Buffer,
                                         BufferLength,
                                         &memory);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the WdfMemory object, %!STATUS!", status);
        goto Done;
    }
    writeContext->Memory = memory;
    
    status = WdfIoTargetFormatRequestForIoctl(CxProxyWdfDeviceGetSelfIoTarget(fdoContext->Device),
                                              request,
                                              IOCTL_NFCCX_WRITE_PACKET,
                                              memory,
                                              NULL,
                                              NULL,
                                              NULL);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to format the request, %!STATUS!", status);
        goto Done;
    }

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != request) {
            WdfObjectDelete(request);
            request = NULL;
        }
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return request;
}

NTSTATUS
NfcCxTml_Read(
    _In_ PNFCCX_TML_INTERFACE TmlInterface,
    _In_bytecount_(BufferLength) PUCHAR Buffer,
    _In_ USHORT BufferLength,
    _In_ pphTmlNfc_TransactCompletionCb_t PfnReadCompletion,
    _In_opt_ PVOID Context
    )
/*++

Routine Description:

    This routine is called from the TML callback routine to
    signal a Read request.  The implementation of this function
    will simply enqueue the read request as the reads from the
    client driver are delivered through a read notification pattern.

Arguments:

    TmlInterface - A pointer to the TmlInterface
    Buffer - The read buffer
    BufferLength - The Buffer size
    PfnWriteCompletion - The LibNfc read completion routine
    Context - The LibNfc read completion context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_TML_READ_QUEUE_ENTRY readQueueEntry = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(TmlInterface->QueueLock, NULL);

    if (INTERFACE_STATE_STOPPED == TmlInterface->InterfaceState) {
        status = STATUS_DEVICE_NOT_CONNECTED;
        WdfWaitLockRelease(TmlInterface->QueueLock);
        TRACE_LINE(LEVEL_ERROR, "TmlInterface is in the process of being destroyed");
        goto Done;
    }

    readQueueEntry = (PNFCCX_TML_READ_QUEUE_ENTRY)malloc(sizeof(*readQueueEntry));
    if (NULL == readQueueEntry) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the read queue entry");
        status = STATUS_INSUFFICIENT_RESOURCES;
        WdfWaitLockRelease(TmlInterface->QueueLock);
        goto Done;
    }

    //
    // Status should be STATUS_PENDING after this point.
    //
    status = STATUS_PENDING;

    RtlZeroMemory(readQueueEntry, sizeof(*readQueueEntry));

    InitializeListHead(&readQueueEntry->ListEntry);

    readQueueEntry->Buffer = Buffer;
    readQueueEntry->BufferLength = BufferLength;
    readQueueEntry->PfnReadCompletion = PfnReadCompletion;
    readQueueEntry->Context = Context;
    readQueueEntry->hCompletionEvent = NULL;

    //
    // See if we have a read notification that can service this read request.
    // There is no need to check TmlInterface->IsWriteCompletionPending since
    // Write happens on the Libnfc thread and the message is also going to
    // be posted on the Libnfc thread.
    //
    if (!IsListEmpty(&TmlInterface->ReadNotificationQueue) &&
        FALSE == TmlInterface->IsWriteCompletionPending) {

        PLIST_ENTRY ple = NULL;
        PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY pReadNotificationQueueEntry = NULL;

        ple = RemoveHeadList(&TmlInterface->ReadNotificationQueue);
        pReadNotificationQueueEntry = CONTAINING_RECORD(ple, 
                                                        NFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY, 
                                                        ListEntry);
        //
        // If the received buffer is smaller than the buffer associated with the entry
        // in ReadNotificationQueue, then discard the entry in the ReadNotificationQueue.
        // Also insert the entry just created to ReadQueue and return STATUS_PENDING
        //
        if (BufferLength < pReadNotificationQueueEntry->BufferLength) {
            TRACE_LINE(LEVEL_ERROR, "Received buffer is smaller than the buffer received from within the Read Notification queue");            
            InsertTailList(&TmlInterface->ReadQueue, 
                           &readQueueEntry->ListEntry);
            free(pReadNotificationQueueEntry);            
            WdfWaitLockRelease(TmlInterface->QueueLock);
            goto Done;
        }
        RtlCopyMemory(readQueueEntry->Buffer, 
                      pReadNotificationQueueEntry->Buffer, 
                      pReadNotificationQueueEntry->BufferLength);
        readQueueEntry->BufferLength = pReadNotificationQueueEntry->BufferLength;

        WdfWaitLockRelease(TmlInterface->QueueLock);

        TRACE_LINE(LEVEL_INFO, "Completing a queued read notification");

        NfcCxTmlInterfacePostLibNfcThreadMessage(TmlInterface->FdoContext,
                                                 PH_OSALNFC_DEFERRED_CALLBACK,
                                                 (UINT_PTR)NfcCxTml_DeferredReadCompletion,
                                                 (UINT_PTR)readQueueEntry,
                                                 NULL,
                                                 NULL);

        free(pReadNotificationQueueEntry);
        readQueueEntry = NULL;

        goto Done;
    }
    else {
        //
        // Enqueue the context, buffer and callback.  When a read notification
        // comes through, we must use that data when calling back the callback.
        // 
        InsertTailList(&TmlInterface->ReadQueue, 
                       &readQueueEntry->ListEntry);
    }
    WdfWaitLockRelease(TmlInterface->QueueLock);

Done:
    if (!NT_SUCCESS(status)) {
        if (NULL != readQueueEntry) {
            free(readQueueEntry);
            readQueueEntry = NULL;
        }
    }
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxTmlDispatchReadNotification(
    _In_ PNFCCX_TML_INTERFACE TmlInterface,
    _In_opt_bytecount_(BufferSize) PUCHAR Buffer,
    _In_ USHORT BufferSize
    )
/*++

Routine Description:

    This routine is called from the CX implementation when
    a read notification is received.  It will look at the pended
    read queue and complete any available read request.

Arguments:

    TmlInterface - A pointer to the TmlInterface
    Buffer - The read buffer
    BufferLength - The Buffer size

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY ple = NULL;
    PNFCCX_TML_READ_QUEUE_ENTRY pReadQueueEntry = NULL;
    NCI_PACKET_HEADER nciHeader = {0};
    DWORD dwWait = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "Read Notification received with size :0x%x",
                           BufferSize);

    if (FALSE == NciPacketHeaderGetFromBuffer(Buffer,
                                              (UCHAR)BufferSize,
                                              &nciHeader)) {
        // Ignore the error
        TRACE_LINE(LEVEL_ERROR, "Failed to parse the Nci Packet Header");
    }

    WdfWaitLockAcquire(TmlInterface->QueueLock, NULL);

    if (IsListEmpty(&TmlInterface->ReadQueue) ||
        TRUE == TmlInterface->IsWriteCompletionPending) {

        PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY pReadNotificationQueueEntry;

        TRACE_LINE(LEVEL_WARNING, "Read notification received without a queued read");

        //
        // If we get here, this means that we have received data from the hardware
        // before the CoreLib has issued a read request or a write has not been completed by the
        // transport layer. In this situation we enqueue the read data so it is available when the next read request
        // comes in.
        //
        pReadNotificationQueueEntry = (PNFCCX_TML_READ_NOTIFICATION_QUEUE_ENTRY)malloc(sizeof(*pReadNotificationQueueEntry));
        if (NULL == pReadNotificationQueueEntry) {
            TRACE_LINE(LEVEL_ERROR, "Failed to allocate buffer for READ_NOTIFICATION_QUEUE_ENTRY");
            status = STATUS_INSUFFICIENT_RESOURCES;
            WdfWaitLockRelease(TmlInterface->QueueLock);
            goto Done;
        }

        NT_ASSERT(NCI_PACKET_MAX_SIZE >= BufferSize);

        InitializeListHead(&pReadNotificationQueueEntry->ListEntry);
        RtlCopyMemory(pReadNotificationQueueEntry->Buffer, Buffer, BufferSize);
        pReadNotificationQueueEntry->BufferLength = BufferSize;

        TRACE_LINE(LEVEL_INFO, "Read Notification queued");
        InsertTailList(&TmlInterface->ReadNotificationQueue, 
                        &pReadNotificationQueueEntry->ListEntry);
        WdfWaitLockRelease(TmlInterface->QueueLock);
        goto Done;
    }

    ple = TmlInterface->ReadQueue.Flink;

    pReadQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_READ_QUEUE_ENTRY, ListEntry);

    if (BufferSize > pReadQueueEntry->BufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Notification buffer is greater than the buffer received from LIBNFC");
        WdfWaitLockRelease(TmlInterface->QueueLock);
        goto Done;
    }

    RemoveHeadList(&TmlInterface->ReadQueue);

    WdfWaitLockRelease(TmlInterface->QueueLock);

    RtlCopyMemory(pReadQueueEntry->Buffer, Buffer, BufferSize);
    pReadQueueEntry->BufferLength = BufferSize;

    if (NfcCxTmlInterfaceProcessRequestAsync(TmlInterface->FdoContext)) {
        pReadQueueEntry->hCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    NfcCxTmlInterfacePostLibNfcThreadMessage(TmlInterface->FdoContext,
                                             PH_OSALNFC_DEFERRED_CALLBACK,
                                             (UINT_PTR)NfcCxTml_DeferredReadCompletion,
                                             (UINT_PTR)pReadQueueEntry,
                                             NULL,
                                             NULL);
    
    if (pReadQueueEntry->hCompletionEvent != NULL) {
        dwWait = WaitForSingleObject(pReadQueueEntry->hCompletionEvent, MAX_CALLBACK_TIMEOUT);

        if (dwWait != WAIT_OBJECT_0) {
            if (dwWait == WAIT_FAILED) {
                dwWait = GetLastError();
            }

            TRACE_LINE(LEVEL_ERROR, "Timer for read completion callback timed out. Error: 0x%08X", dwWait);
            MICROSOFT_TELEMETRY_ASSERT_MSG(false, "NfcCxTmlPostReadCallbackTimeout");

            status = STATUS_UNSUCCESSFUL;
            NfcCxDeviceSetFailed(TmlInterface->FdoContext->Device);
        }

        CloseHandle(pReadQueueEntry->hCompletionEvent);
    }
    else {
        //
        // If the completion event is not set, then pReadQueueEntry will be released
        // by NfcCxTml_DeferredReadCompletion.
        //
        pReadQueueEntry = NULL;
    }

Done:
    if (NULL != pReadQueueEntry) {
        free(pReadQueueEntry);
        pReadQueueEntry = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NFCSTATUS
phTmlNfc_Init( void *pHwRef )
{
    UNREFERENCED_PARAMETER(pHwRef);
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS
phTmlNfc_Shutdown( void *pHwRef )
{
    UNREFERENCED_PARAMETER(pHwRef);
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS
phTmlNfc_Write(
    void *pHwRef,
    uint8_t *pBuffer,
    uint16_t wLength,
    pphTmlNfc_TransactCompletionCb_t pTmlWriteComplete,
    void *pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_TML_INTERFACE tmlInterface = ((PNFCCX_FDO_CONTEXT)pHwRef)->TmlInterface;
    PNFCCX_TML_WRITE_QUEUE_ENTRY writeQueueEntry = NULL;

    WdfWaitLockAcquire(tmlInterface->QueueLock, NULL);

    if (INTERFACE_STATE_STOPPED == tmlInterface->InterfaceState) {
        status = STATUS_DEVICE_NOT_CONNECTED;
        WdfWaitLockRelease(tmlInterface->QueueLock);
        TRACE_LINE(LEVEL_ERROR, "TmlInterface is in the process of being destroyed");
        goto Done;
    }

    writeQueueEntry = (PNFCCX_TML_WRITE_QUEUE_ENTRY) malloc (sizeof(*writeQueueEntry));
    if (NULL == writeQueueEntry) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Write context could not be allocated");
        WdfWaitLockRelease(tmlInterface->QueueLock);
        goto Done;
    }

    InitializeListHead(&writeQueueEntry->ListEntry);
    writeQueueEntry->TmlInterface = tmlInterface;

    writeQueueEntry->Request = NfcCxTml_PrepareWriteRequest(tmlInterface,
                                                            pBuffer,
                                                            wLength,
                                                            pTmlWriteComplete,
                                                            pContext);
    if (NULL == writeQueueEntry->Request) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the write request");
        status = STATUS_UNSUCCESSFUL;
        WdfWaitLockRelease(tmlInterface->QueueLock);
        goto Done;
    }

    //
    // Set the status to pending. After TML write is called, it is responsible
    // for releasing the writeQueueEntry
    //
    status = STATUS_PENDING;
    
    //
    // If there is a write already pending in the queue or if there is
    // a write pended with the tranport layer, then return.
    //
    if (!IsListEmpty(&tmlInterface->WriteQueue) ||
        (TRUE == tmlInterface->IsWriteCompletionPending)) {
        status = STATUS_PENDING;
        TRACE_LINE(LEVEL_INFO,
                   "A write is already in the queue (%d) or write is pended with transport layer %d",
                   (IsListEmpty(&tmlInterface->WriteQueue) == 0),
                   tmlInterface->IsWriteCompletionPending);
        InsertTailList(&tmlInterface->WriteQueue, &writeQueueEntry->ListEntry);
    }
    else {
        NT_ASSERT(FALSE == tmlInterface->IsWriteCompletionPending);
        tmlInterface->IsWriteCompletionPending = TRUE;

        WdfWaitLockRelease(tmlInterface->QueueLock);
        
        NfcCxTml_Write(writeQueueEntry);
        goto Done;
    }

    WdfWaitLockRelease(tmlInterface->QueueLock);

Done:

    if (!NT_SUCCESS(status)) {
        if (NULL != writeQueueEntry) {
            free(writeQueueEntry);
        }
    }

    return NfcCxNfcStatusFromNtStatus(status);
}

NFCSTATUS
phTmlNfc_Read(
    void *pHwRef,
    uint8_t *pBuffer,
    uint16_t wLength,
    pphTmlNfc_TransactCompletionCb_t pTmlReadComplete,
    void *pContext
    )
{
    return NfcCxNfcStatusFromNtStatus(
                NfcCxTml_Read(((PNFCCX_FDO_CONTEXT)pHwRef)->TmlInterface,
                             pBuffer,
                             wLength,
                             pTmlReadComplete,
                             pContext));
}

NFCSTATUS 
phTmlNfc_ReadAbort(void *pHwRef)
{
    PNFCCX_TML_INTERFACE tmlInterface = ((PNFCCX_FDO_CONTEXT)pHwRef)->TmlInterface;

    WdfWaitLockAcquire(tmlInterface->QueueLock, NULL);
    
    while (!IsListEmpty(&tmlInterface->ReadQueue)) {
        PLIST_ENTRY ple = NULL;
        PNFCCX_TML_READ_QUEUE_ENTRY pReadQueueEntry = NULL;
        
        ple = RemoveHeadList(&tmlInterface->ReadQueue);

        pReadQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_READ_QUEUE_ENTRY, ListEntry);
        free(pReadQueueEntry);
    }

    WdfWaitLockRelease(tmlInterface->QueueLock);

    return NFCSTATUS_SUCCESS;
}

NFCSTATUS
phTmlNfc_WriteAbort(void *pHwRef)
{
    PNFCCX_TML_INTERFACE tmlInterface = ((PNFCCX_FDO_CONTEXT)pHwRef)->TmlInterface;

    WdfWaitLockAcquire(tmlInterface->QueueLock, NULL);
    
    while (!IsListEmpty(&tmlInterface->WriteQueue)) {
        PLIST_ENTRY ple = NULL;
        PNFCCX_TML_WRITE_QUEUE_ENTRY pWriteQueueEntry = NULL;

        ple = RemoveHeadList(&tmlInterface->WriteQueue);
        pWriteQueueEntry = CONTAINING_RECORD(ple, NFCCX_TML_WRITE_QUEUE_ENTRY, ListEntry);
        InitializeListHead(&pWriteQueueEntry->ListEntry);

        free(pWriteQueueEntry);
    }

    WdfWaitLockRelease(tmlInterface->QueueLock);
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS
phTmlNfc_IoCtl(void *pHwRef, phTmlNfc_ControlCode_t eControlCode)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PNFCCX_FDO_CONTEXT fdoContext = (PNFCCX_FDO_CONTEXT)pHwRef;

    switch(eControlCode)
    {
    case phTmlNfc_e_ResetDevice:
        NfcCxStateInterfaceQueueEvent(fdoContext->RFInterface->pLibNfcContext->StateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
        break;

    default:
        status = NFCSTATUS_INVALID_PARAMETER;
        break;
    }
    
    return status;
}
