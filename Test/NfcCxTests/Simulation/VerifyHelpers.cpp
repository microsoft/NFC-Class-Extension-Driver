//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <NfpDev.h>

#include <Tests\TestLogging.h>
#include "VerifyHelpers.h"

void VerifyNciPacket(const NciPacket& controlPacket, const NciSimCallbackView& callback)
{
    // Ensure Simulator Driver callback is what we expect.
    VERIFY_ARE_EQUAL(callback.Header->Type, NciSimCallbackType::NciWrite);
    auto nciWrite = static_cast<const NciSimCallbackNciWrite*>(callback.Header);

    DWORD nciPacketSize = callback.Length - NciSimCallbackNciWriteMinSize;

    VerifyArraysAreEqual(
        L"Expected packet", controlPacket.PacketBytes(), controlPacket.PacketBytesLength(),
        L"Actual packet  ", nciWrite->NciMessage, nciPacketSize);
}

void VerifySequenceHandler(
    NFC_CX_SEQUENCE type,
    const NciSimCallbackView& callback)
{
    VERIFY_ARE_EQUAL(callback.Header->Type, NciSimCallbackType::SequenceHandler);

    auto params = static_cast<const NciSimCallbackSequenceHandler*>(callback.Header);
    VERIFY_ARE_EQUAL(type, params->Sequence);
}

void VerifyArraysAreEqual(
    _In_ PCWSTR arrayAName,
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_ PCWSTR arrayBName,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength)
{
    // Log the expected packet.
    LogByteBuffer(arrayAName, arrayA, arrayALength);

    // Log actual packet.
    LogByteBuffer(arrayBName, arrayB, arrayBLength);

    // Ensure packets are the same.
    VERIFY_ARE_EQUAL(arrayALength, arrayBLength);
    VERIFY_IS_TRUE(0 == memcmp(arrayA, arrayB, arrayALength));
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
        L"Expected message", expectedMessage, expectedMessageLength,
        L"Actual message  ", message->payload, ioResultLength - offsetof(SUBSCRIBED_MESSAGE, payload));
}
