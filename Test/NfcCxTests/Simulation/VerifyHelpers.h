//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <NfcCxTestDeviceDriver.h>

#include "NciControlPacket.h"
#include "NciSimConnector.h"

void VerifyArraysAreEqual(
    _In_ PCWSTR name,
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength);

bool AreArraysEqual(
    _In_reads_bytes_(arrayALength) const void* arrayA,
    _In_ size_t arrayALength,
    _In_reads_bytes_(arrayBLength) const void* arrayB,
    _In_ size_t arrayBLength);

void VerifyProximitySubscribeMessage(
    _In_reads_bytes_(ioResultLength) const void* ioResult,
    _In_ size_t ioResultLength,
    _In_reads_bytes_(expectedMessageLength) const void* expectedMessage,
    _In_ size_t expectedMessageLength
    );
