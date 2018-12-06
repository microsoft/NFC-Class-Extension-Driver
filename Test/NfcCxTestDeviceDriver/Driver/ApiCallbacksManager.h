//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <NfcCxTestDeviceDriver.h>

// Manages 'IOCTL_NCISIM_GET_NEXT_CALLBACK' I/O requests. This includes immediately fulfilling the request if a message
// is already available or waiting for the next available message.
class ApiCallbacksManager
{
public:
    ApiCallbacksManager();
    ~ApiCallbacksManager();

    NTSTATUS Initialize(
        _In_ WDFDEVICE Device
        );

    // Queue up a NciSimCallbackType::NciWrite callback message.
    NTSTATUS PostNciWriteCallback(
        _In_reads_bytes_(nciPacketLength) void* nciPacket,
        _In_ size_t nciPacketLength
        );
    // Queue up a NciSimCallbackType::SequenceHandler callback message.
    NTSTATUS PostSequenceHandlerCallback(
        _In_ NFC_CX_SEQUENCE Sequence
        );
    // Queues up a callback message that signals that a D0 entry was triggered.
    NTSTATUS PostD0EntryCallback(
        );
    // Queues up a callback message that signals that a D0 exit was triggered.
    NTSTATUS PostD0ExitCallback(
        );
    // Processes a IOCTL_NCISIM_GET_NEXT_CALLBACK request (including storing the request in a queue if a
    // callback is not available yet).
    void HandleCallbackRequest(
        _In_ WDFREQUEST Request
        );

private:
    struct DeferredCallbackListItem
    {
        LIST_ENTRY ListEntry;
        DWORD DataLength;
        BYTE Data[ANYSIZE_ARRAY];
    };
    static_assert(__is_trivial(DeferredCallbackListItem), "DeferredCallbackListItem must be trivial.");

    NTSTATUS AllocateDeferredCallbackListItem(
        _In_ DWORD DataLength,
        _Outptr_ DeferredCallbackListItem** result
        );
    void QueueNewCallback(
        _In_ DeferredCallbackListItem* deferredCallbackListItem
        );
    void TryFulfillCallbackRequest();

    template <typename CallbackDataType>
    NTSTATUS PostSimpleCallback(const CallbackDataType& callbackData);

    WDFWAITLOCK _Lock = nullptr;
    WDFQUEUE _DeferredCallbackRequests = nullptr;
    LIST_ENTRY _DeferredCallbacks = {};
};
