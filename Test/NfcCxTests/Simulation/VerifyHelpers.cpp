//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <NfpDev.h>

#include <Tests\TestLogging.h>
#include "VerifyHelpers.h"

bool AreArraysEqual(
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength)
{
    return arrayALength == arrayBLength &&
        0 == memcmp(arrayA, arrayB, arrayALength);
}

void VerifyArraysAreEqual(
    _In_ PCWSTR name,
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength)
{
    if (!AreArraysEqual(arrayA, arrayALength, arrayB, arrayBLength))
    {
        // Log the expected packet.
        LogByteBuffer(L"Expected", arrayA, arrayALength);

        // Log actual packet.
        LogByteBuffer(L"Actual  ", arrayB, arrayBLength);

        VERIFY_FAIL_MSG(L"'%s' arrays don't match.", name);
        return;
    }

    LOG_COMMENT(L"'%s' arrays match.", name);
}

void VerifyProximitySubscribeMessage(
    _In_reads_bytes_(ioResultLength) const void* ioResult,
    _In_ size_t ioResultLength,
    _In_reads_bytes_(expectedMessageLength) const void* expectedMessage,
    _In_ size_t expectedMessageLength
    )
{
    auto message = reinterpret_cast<const SUBSCRIBED_MESSAGE*>(ioResult);
    VERIFY_ARE_EQUAL(expectedMessageLength, size_t(message->cbPayloadHint));

    VerifyArraysAreEqual(
        L"Proximity subscription message",
        expectedMessage, expectedMessageLength,
        message->payload, ioResultLength - offsetof(SUBSCRIBED_MESSAGE, payload));
}
