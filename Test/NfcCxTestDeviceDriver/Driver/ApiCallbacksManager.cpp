//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "Precomp.h"
#include "ApiCallbacksManager.h"

#include "ApiCallbacksManager.tmh"

static constexpr size_t MAX_NCI_PACKET_SIZE = 258;

ApiCallbacksManager::ApiCallbacksManager()
{
    InitializeListHead(&_DeferredCallbacks);
}

ApiCallbacksManager::~ApiCallbacksManager()
{
    // Free any remaining items in '_DeferredCallbacks'.
    for (LIST_ENTRY* listEntry = RemoveHeadList(&_DeferredCallbacks); listEntry != &_DeferredCallbacks; listEntry = RemoveHeadList(&_DeferredCallbacks))
    {
        free(listEntry);
    }

    _Lock = nullptr;
    _DeferredCallbackRequests = nullptr;
}

NTSTATUS
ApiCallbacksManager::Initialize(
    _In_ WDFDEVICE Device)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    // Create lock for handling I/O callbacks.
    WDF_OBJECT_ATTRIBUTES lockAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
    lockAttributes.ParentObject = Device;

    status = WdfWaitLockCreate(&lockAttributes, &_Lock);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfWaitLockCreate failed. %!STATUS!", status);
        return status;
    }

    // Create I/O queue for storing pending IOCTL_NCISIM_GET_NEXT_CALLBACK requests.
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;

    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&queueAttributes);

    status = WdfIoQueueCreate(Device, &queueConfig, &queueAttributes, &_DeferredCallbackRequests);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfIoQueueCreate failed. %!STATUS!", status);
        return status;
    }

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
ApiCallbacksManager::PostNciWriteCallback(
    _In_reads_bytes_(nciPacketLength) void* nciPacket,
    _In_ size_t nciPacketLength
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    // Ensure NCI packet size is not larger than expected.
    if (nciPacketLength > MAX_NCI_PACKET_SIZE)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        TRACE_LINE(LEVEL_ERROR, "Write NCI Request input buffer is too big. %!STATUS!", status);
        return status;
    }

    // Calculate callback data's length.
    DWORD callbackDataSize = NciSimCallbackNciWriteMinSize + DWORD(nciPacketLength);

    // Allocate memory for the deferred callback.
    DeferredCallbackListItem* callbackListItem;
    status = AllocateDeferredCallbackListItem(callbackDataSize, &callbackListItem);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "AllocateDeferredCallbackListItem failed. %!STATUS!", status);
        return status;
    }

    // Set callback data.
    auto callbackData = reinterpret_cast<NciSimCallbackNciWrite*>(callbackListItem->Data);
    callbackData->Type = NciSimCallbackType::NciWrite;
    CopyMemory(callbackData->NciMessage, nciPacket, nciPacketLength);

    // Add deferred callback to list.
    QueueNewCallback(callbackListItem);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

NTSTATUS
ApiCallbacksManager::PostSequenceHandlerCallback(
    _In_ NFC_CX_SEQUENCE Sequence
    )
{
    NciSimCallbackSequenceHandler data;
    data.Type = NciSimCallbackType::SequenceHandler;
    data.Sequence = Sequence;

    return PostSimpleCallback(data);
}

// Queues up a callback message that signals that a D0 entry was triggered.
NTSTATUS
ApiCallbacksManager::PostD0EntryCallback(
    )
{
    NciSimCallbackHeader data;
    data.Type = NciSimCallbackType::D0Entry;

    return PostSimpleCallback(data);
}

// Queues up a callback message that signals that a D0 exit was triggered.
NTSTATUS
ApiCallbacksManager::PostD0ExitCallback(
    )
{
    NciSimCallbackHeader data;
    data.Type = NciSimCallbackType::D0Exit;

    return PostSimpleCallback(data);
}

template <typename CallbackDataType>
NTSTATUS
ApiCallbacksManager::PostSimpleCallback(const CallbackDataType& callbackData)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status;

    // Allocate memory for the deferred callback.
    DeferredCallbackListItem* callbackListItem;
    status = AllocateDeferredCallbackListItem(sizeof(CallbackDataType), &callbackListItem);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "AllocateDeferredCallbackListItem failed. %!STATUS!", status);
        return status;
    }

    // Set callback data.
    auto data = reinterpret_cast<CallbackDataType*>(callbackListItem->Data);
    *data = callbackData;

    // Add deferred callback to list.
    QueueNewCallback(callbackListItem);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

void
ApiCallbacksManager::HandleCallbackRequest(
    _In_ WDFREQUEST Request
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;
    WdfWaitLockAcquire(_Lock, nullptr);

    // Add callback request to deferred callbacks requests queue.
    status = WdfRequestForwardToIoQueue(Request, _DeferredCallbackRequests);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfRequestForwardToIoQueue failed. %!STATUS!", status);
        WdfWaitLockRelease(_Lock);
        WdfRequestComplete(Request, status);
        return;
    }

    // Try to match the callback request to a callback.
    TryFulfillCallbackRequest();

    WdfWaitLockRelease(_Lock);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return;
}

NTSTATUS
ApiCallbacksManager::AllocateDeferredCallbackListItem(
    _In_ DWORD DataLength,
    _Outptr_ DeferredCallbackListItem** result
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    size_t allocSize = sizeof(DeferredCallbackListItem) + DataLength;
    auto callbackListItem = reinterpret_cast<DeferredCallbackListItem*>(malloc(allocSize));
    if (!callbackListItem)
    {
        status = STATUS_NO_MEMORY;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory. %!STATUS!", status);
        return status;
    }

    callbackListItem->DataLength = DataLength;

    *result = callbackListItem;
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

void
ApiCallbacksManager::QueueNewCallback(
    _In_ DeferredCallbackListItem* deferredCallbackListItem
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Add deferred callback to list.
    WdfWaitLockAcquire(_Lock, nullptr);
    InsertTailList(&_DeferredCallbacks, &deferredCallbackListItem->ListEntry);

    // Try to match the callback to a callback request.
    TryFulfillCallbackRequest();

    WdfWaitLockRelease(_Lock);
    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
}

void
ApiCallbacksManager::TryFulfillCallbackRequest()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status;

    // Look for a deferred callback.
    LIST_ENTRY* deferredCallbackListEntry = _DeferredCallbacks.Flink;
    if (deferredCallbackListEntry == &_DeferredCallbacks)
    {
        // Deferred callbacks list is empty. Nothing to do.
        TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
        return;
    }

    // Look for a deferred callback request.
    WDFREQUEST callbackRequest;
    status = WdfIoQueueRetrieveNextRequest(_DeferredCallbackRequests, &callbackRequest);
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        // Callback request queue is empty. Nothing to do.
        TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
        return;
    }
    else if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "WdfIoQueueFindRequest failed. %!STATUS!", status);
        return;
    }

    // A deferred callback was found.
    DeferredCallbackListItem* deferredCallback = CONTAINING_RECORD(deferredCallbackListEntry, DeferredCallbackListItem, ListEntry);

    // Ensure output buffer is large enough.
    void* outputBuffer;
    status = WdfRequestRetrieveOutputBuffer(callbackRequest, deferredCallback->DataLength, &outputBuffer, nullptr);
    if (!NT_SUCCESS(status))
    {
        // The caller's buffer is too small.
        TRACE_LINE(LEVEL_ERROR, "WdfRequestRetrieveOutputBuffer failed. %!STATUS!", status);

        // Provide the buffer size to the caller.
        WdfRequestCompleteWithInformation(callbackRequest, status, deferredCallback->DataLength);
        return;
    }

    // Copy callback data into the caller's buffer.
    CopyMemory(outputBuffer, &deferredCallback->Data, deferredCallback->DataLength);
    WdfRequestCompleteWithInformation(callbackRequest, STATUS_SUCCESS, deferredCallback->DataLength);

    // Remove deferred callback from list and free it.
    RemoveHeadList(&_DeferredCallbacks);
    free(deferredCallback);

    TRACE_FUNCTION_SUCCESS(LEVEL_VERBOSE);
    return;
}
