/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxUtils.cpp

Abstract:

    This module implements utility functions that are used throughout
    the class extension implementation

--*/

#include "NfcCxPch.h"

#include "NFcCxUtils.tmh"

NFCSTATUS
NfcCxNfcStatusFromNtStatus(
    _In_ NTSTATUS Status
    )
/*++

Routine Description:

   Converts from an NTSTATUS to an NFCSTATUS code

Arguments:

    Status - The NTSTATUS code

Return Value:

    NFCSTATUS

--*/
{
    return phOsalNfc_NfcStatusFromNtStatus(Status);
}

NTSTATUS
NfcCxNtStatusFromNfcStatus(
    _In_ NFCSTATUS NfcStatus
    )
/*++

Routine Description:

   Converts from an NfcStatus to an NTSTATUS code

Arguments:

    NfcStatus - The NFC status

Return Value:

    NTSTATUS

--*/
{
    return phOsalNfc_NtStatusFromNfcStatus(NfcStatus);
}

NTSTATUS
NfcCxGuidFromUnicodeString(
    _In_ PCUNICODE_STRING GuidString,
    _Out_ GUID* Guid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG temp[10] = {0};
    WCHAR nullTerminated[STR_GUID_LENGTH] = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RtlZeroMemory(Guid, sizeof(*Guid));

    //
    // Make sure we can null terminate the string
    //
    if (GuidString->Length > sizeof(nullTerminated) - sizeof(WCHAR)) {
        TRACE_LINE(LEVEL_ERROR, "String too large");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }
    RtlCopyMemory(nullTerminated, GuidString->Buffer, GuidString->Length);

    if (11 != swscanf_s(nullTerminated, STR_GUID_FMTW,
                            &Guid->Data1,
                            &temp[0],
                            &temp[1],
                            &temp[2],
                            &temp[3],
                            &temp[4],
                            &temp[5],
                            &temp[6],
                            &temp[7],
                            &temp[8],
                            &temp[9])) {
        TRACE_LINE(LEVEL_ERROR, "Failed to convert the string to a GUID");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    //
    // copy the temp values into the GUID
    //
    Guid->Data2 = (USHORT)temp[0];
    Guid->Data3 = (USHORT)temp[1];
    Guid->Data4[0] = (UCHAR)temp[2];
    Guid->Data4[1] = (UCHAR)temp[3];
    Guid->Data4[2] = (UCHAR)temp[4];
    Guid->Data4[3] = (UCHAR)temp[5];
    Guid->Data4[4] = (UCHAR)temp[6];
    Guid->Data4[5] = (UCHAR)temp[7];
    Guid->Data4[6] = (UCHAR)temp[8];
    Guid->Data4[7] = (UCHAR)temp[9];

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxUnicodeStringFromGuid(
    _In_ REFGUID Guid,
    _Out_ PUNICODE_STRING GuidString
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int cch = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    cch = StringFromGUID2(Guid, GuidString->Buffer, GuidString->MaximumLength);
    if (cch == 0) {
        TRACE_LINE(LEVEL_ERROR, "Failed to convert the GUID to a string.");
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    NT_ASSERT(cch == STR_GUID_LENGTH);

    GuidString->Length = (USHORT)(cch * sizeof(WCHAR));

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

// If this function succeeds, you will need to deallocate the memory for NarrowStr with free()
NTSTATUS
NfcCxWideStringToNarrowString(
    _In_ size_t cchWideStr,
    _In_reads_z_(cchWideStr) PCWSTR WideStr,
    _Out_ size_t* cchNarrowStr,
    _Out_writes_z_(*cchNarrowStr) PSTR* NarrowStr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cbNarrowStr = 0;
    PSTR tempNarrowStr = NULL;
    size_t convertedIncludingTerminator = 0;
    errno_t err = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(NULL != WideStr);
    NT_ASSERT(NULL != cchNarrowStr);
    NT_ASSERT(NULL != NarrowStr);

    cbNarrowStr = (cchWideStr + 1) * sizeof(char); // Add a byte for NULL terminator
    tempNarrowStr = (PSTR)malloc(cbNarrowStr);
    if (NULL == tempNarrowStr) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory for string");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    err = wcstombs_s(&convertedIncludingTerminator,
                     tempNarrowStr,
                     cbNarrowStr,
                     WideStr,
                     cchWideStr);
    if (err != 0) {
        TRACE_LINE(LEVEL_ERROR, "wcstombs_s failed with error code %d", err);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    NT_ASSERT(convertedIncludingTerminator == (cchWideStr + 1));

    *NarrowStr = tempNarrowStr;

    // Per MSDN the wcstombs_s includes counting the terminator, but for "cch" that's not expected/typical
    *cchNarrowStr = convertedIncludingTerminator - 1;

Done:
    if (!NT_SUCCESS(status) && NULL != tempNarrowStr) {
        free(tempNarrowStr);
        tempNarrowStr = NULL;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxCopyToBuffer(
    _In_reads_bytes_(cbInput) const void* pbInput,
    _In_ size_t cbInput,
    _Out_writes_bytes_to_(*pcbOutputBuffer, *pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    )
/*++

Routine Description:

   Copies one byte array into another, after ensuring there is enough space.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if (*pcbOutputBuffer < cbInput) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    RtlCopyMemory(pbOutputBuffer, pbInput, cbInput);
    *pcbOutputBuffer = cbInput;

Done:
    return status;
}

NTSTATUS
NfcCxRegistryQueryULong(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ ULONG* Value
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    UNICODE_STRING valueNameString;
    status = RtlUnicodeStringInit(&valueNameString, ValueName);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "RtlUnicodeStringInit failed. %!STATUS!", status);
        goto Done;
    }

    status = WdfRegistryQueryULong(
        Key,
        &valueNameString,
        Value);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_INFO, "WdfRegistryQueryULong failed. %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRegistryQueryBoolean(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ BOOLEAN* Value
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ULONG tmpValue;
    NTSTATUS status = NfcCxRegistryQueryULong(
        Key,
        ValueName,
        &tmpValue);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_INFO, "WdfRegistryQueryULong failed. %!STATUS!", status);
        goto Done;
    }

    *Value = (tmpValue != FALSE);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRegistryAssignULong(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ ULONG Value
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    UNICODE_STRING valueNameString;
    status = RtlUnicodeStringInit(&valueNameString, ValueName);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "RtlUnicodeStringInit failed. %!STATUS!", status);
        goto Done;
    }

    status = WdfRegistryAssignULong(
        Key,
        &valueNameString,
        Value);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_INFO, "WdfRegistryAssignULong failed. %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRegistryAssignBoolean(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ BOOLEAN Value
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = NfcCxRegistryAssignULong(
        Key,
        ValueName,
        !!Value);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_INFO, "NfcCxRegistryAssignULong failed. %!STATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

_No_competing_thread_
NTSTATUS
CNFCPendedRequest::Initialize(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ BOOL HintSize,
    _In_ BYTE MaxQueueLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES objectAttrib;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(!m_bInitialized);

    m_bHintSize = HintSize;
    m_bMaxQueueLength = MaxQueueLength;

    InitializeListHead(&m_PayloadQueue);
    m_ucPayloadQueueLength = 0;

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrib);
    objectAttrib.ParentObject = FdoContext->Device;

    status = WdfIoQueueCreate(FdoContext->Device,
                              &queueConfig,
                              &objectAttrib,
                              &m_RequestQueue);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed with Status code %!STATUS!", status);
        goto Done;
    }

    status = WdfWaitLockCreate(&objectAttrib, &m_QueueLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the queue lock, %!STATUS!", status);

        WdfObjectDelete(m_RequestQueue);
        m_RequestQueue = NULL;
        goto Done;
    }

    m_bInitialized = TRUE;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

void
CNFCPendedRequest::Deinitialize()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!m_bInitialized) {
        TRACE_LINE(LEVEL_ERROR, "Object is not initialized");
        goto Done;
    }
    //
    // Drain the received queue
    //
    if (NULL != m_QueueLock) {

        WdfWaitLockAcquire(m_QueueLock, NULL);
        while (!IsListEmpty(&m_PayloadQueue)) {

            PLIST_ENTRY ple = RemoveHeadList(&m_PayloadQueue);
            m_ucPayloadQueueLength--;
            delete CNFCPayload::FromListEntry(ple);
        }
        WdfWaitLockRelease(m_QueueLock);

        WdfObjectDelete(m_QueueLock);
        m_QueueLock = NULL;
    }

    if (NULL != m_RequestQueue) {
        WdfObjectDelete(m_RequestQueue);
        m_RequestQueue = NULL;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
CNFCPendedRequest::ProcessRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
/*++

Routine Description:

   Process the request immediately if there are payload queued against it
   Otherwise, queue the request pending payload arrival

Arguments:

    FileContext - Point to the CNFCPayload object data
    Request - WDF request object
    OutputBuffer - Point to the outhput buffer provided
    OutputBufferLength - Ouput buffer length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(m_QueueLock, NULL);

    if (!m_bInitialized) {
        TRACE_LINE(LEVEL_ERROR, "Object is not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (!IsListEmpty(&m_PayloadQueue)) {
        //
        // Get the head of the list and complete the request
        //
        PLIST_ENTRY ple = m_PayloadQueue.Flink;
        ULONG usedBufferSize = 0;
        CNFCPayload * pBuffer = CNFCPayload::FromListEntry(ple);

        //
        // Complete the request
        //
        status = CopyPayloadData(OutputBuffer,
                                 (ULONG)OutputBufferLength,
                                 pBuffer->GetPayload(),
                                 pBuffer->GetSize(),
                                 &usedBufferSize);

        if (STATUS_BUFFER_OVERFLOW != status &&
            !NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to copy the payload data, %!STATUS!", status);
            goto Done;
        } else if (NT_SUCCESS(status)) {
            RemoveEntryList(ple);
            m_ucPayloadQueueLength--;
            delete pBuffer;
        }

        //
        // Complete the request
        //
        WdfRequestCompleteWithInformation(Request, status, usedBufferSize);

    } else {

        WDFREQUEST payloadRequest = NULL;
        //
        // Ensure that there are no other requests in the queue.
        //
        status = WdfIoQueueFindRequest(m_RequestQueue,
                                       NULL,
                                       FileContext->FileObject,
                                       NULL,
                                       &payloadRequest);
        if (NULL != payloadRequest) {
            TRACE_LINE(LEVEL_ERROR, "Request is already pending in the queue");
            WdfObjectDereference(payloadRequest);
            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }
        else {
            NT_ASSERT(STATUS_NO_MORE_ENTRIES == status);
        }
        
        //
        // Else, forward the request to the holding queue
        //
        status = WdfRequestForwardToIoQueue(Request, m_RequestQueue);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to forward the request to the RequestQueue, %!STATUS!", status);
            goto Done;
        }
    }

Done:
    WdfWaitLockRelease(m_QueueLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
CNFCPendedRequest::ProcessPayload(
    _In_ CNFCPayload* Payload
    )
/*++

Routine Description:

   Process the payload immediately if there are request queued for it
   Otherwise, queue the payload awaiting request arrival

Arguments:

    Payload - Point to the CNFCPayload object data

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_PENDING;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(m_QueueLock, NULL);

    if (!m_bInitialized) {
        TRACE_LINE(LEVEL_ERROR, "Object is not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    if (!NT_SUCCESS(CompleteRequest(Payload->GetPayload(), Payload->GetSize()))) {
        TRACE_LINE(LEVEL_WARNING, "No request to complete, enqueueing packet");

        if (m_bMaxQueueLength <= m_ucPayloadQueueLength) {
            TRACE_LINE(LEVEL_ERROR, "Too many queued, dropping packet");
            goto Done;
        }

        //
        // now hold onto the packet such that caller does not free it
        //
        status = STATUS_SUCCESS;
        InsertTailList(&m_PayloadQueue, Payload->GetListEntry());
        m_ucPayloadQueueLength++;
    }

Done:
    WdfWaitLockRelease(m_QueueLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
CNFCPendedRequest::CopyPayloadData(
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG BufferUsed
    )
/*++

Routine Description:

   Copies the payload data into the payload output buffer.

Arguments:

    OutputBuffer - The Output buffer
    OutputBufferLength - The output buffer length
    Data - The message data buffer
    DataLength - The message data buffer length
    BufferUsed - A pointer to a ULONG to receive how many bytes of the output buffer was used.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG requiredBufferSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!m_bInitialized) {
        TRACE_LINE(LEVEL_ERROR, "Object is not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    *BufferUsed = 0;

    if (m_bHintSize) {
        //
        // Since the output buffer has already been validated
        // to contain 4 bytes, we know we can safely make this assumption
        //
        _Analysis_assume_(sizeof(ULONG) <= OutputBufferLength);

        //
        // The first 4 bytes will provide the payload size to the client
        // in the payload the buffer is not large enough to contain the full
        // payload, the client can allocate a buffer large enough using
        // this size on future requests
        //
        RtlCopyMemory(OutputBuffer, &DataLength, sizeof(ULONG));
        *BufferUsed = sizeof(ULONG);
    }

    status = RtlULongAdd(DataLength, *BufferUsed, &requiredBufferSize);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to calculate the required buffer size, %!STATUS!", status);
        goto Done;
    }

    if (OutputBufferLength < requiredBufferSize) {
        TRACE_LINE(LEVEL_ERROR, "Buffer overflow OutputBufferLength = %d, requiredBufferSize = %d",
                   OutputBufferLength, requiredBufferSize);
        status = STATUS_BUFFER_OVERFLOW;
        goto Done;
    }

    _Analysis_assume_(NULL != OutputBuffer);
    CopyMemory((((PUCHAR)OutputBuffer) + *BufferUsed), Data, DataLength);
    *BufferUsed = requiredBufferSize;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
CNFCPendedRequest::CompleteRequest(
    _In_bytecount_(DataLength) PVOID Data,
    _In_ ULONG DataLength
    )
/*++

Routine Description:

    This routine attempts to complete a request from the provided queue
    with the provided data.

Arguments:

    Data - Data to complete the request with
    DataLength - The Data Length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST wdfRequest = NULL;
    WDFMEMORY reqOutMemory = NULL;
    PVOID pOutBuffer = NULL;
    size_t sizeOutBuffer = 0;
    ULONG actualSize;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!m_bInitialized) {
        TRACE_LINE(LEVEL_ERROR, "Object is not initialized");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    //
    // Get the next pended request
    //
    status = WdfIoQueueRetrieveNextRequest(m_RequestQueue, &wdfRequest);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "No requests pended, %!STATUS!", status);
        goto Done;
    }

    //
    //  Get the request's output buffer
    //
    status = WdfRequestRetrieveOutputMemory(wdfRequest, &reqOutMemory);
    if (NT_SUCCESS(status)) {
        pOutBuffer = WdfMemoryGetBuffer(reqOutMemory, &sizeOutBuffer);
    } else {
        TRACE_LINE(LEVEL_ERROR, "Failed to get output buffer, %!STATUS!", status);
        goto Done;
    }

    //
    // Copy the payload onto the output memory
    //
    status = CopyPayloadData(pOutBuffer,
                             (ULONG)sizeOutBuffer,
                             Data,
                             DataLength,
                             &actualSize);

    TRACE_LINE(LEVEL_INFO, "Completing request %p, with %!STATUS!, 0x%I64x, required size 0x%I64x",
               wdfRequest, status, actualSize, (m_bHintSize) ? *(DWORD*)pOutBuffer : actualSize);

    WdfRequestCompleteWithInformation(wdfRequest, status, actualSize);
    wdfRequest = NULL;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
