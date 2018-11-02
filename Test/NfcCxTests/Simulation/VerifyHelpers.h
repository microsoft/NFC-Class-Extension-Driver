//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <NfcCxTestDeviceDriver.h>

#include "NciControlPacket.h"
#include "NciSimConnector.h"

void VerifyNciPacket(const NciPacket& controlPacket, const NciSimCallbackView& callback);

void VerifySequenceHandler(
    NFC_CX_SEQUENCE type,
    const NciSimCallbackView& callback);

void VerifyArraysAreEqual(
    _In_ PCWSTR arrayAName,
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_ PCWSTR arrayBName,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength);

void VerifyProximitySubscribeMessage(
    _In_reads_bytes_(ioResultLength) const void* ioResult,
    _In_ size_t ioResultLength,
    _In_reads_bytes_(expectedMessageLength) const void* expectedMessage,
    _In_ size_t expectedMessageLength
    );
