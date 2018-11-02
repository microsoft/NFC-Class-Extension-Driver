//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "TestLogging.h"

void LogByteBuffer(
    _In_ PCWSTR bufferName,
    _In_reads_bytes_(bufferLength) const void* buffer,
    _In_ size_t bufferLength)
{
    const uint8_t* bufferAsBytes = reinterpret_cast<const uint8_t*>(buffer);

    std::wstring bufferAsString;
    for (size_t i = 0; i != bufferLength; ++i)
    {
        WCHAR sz[6]; // provide enough space for "0xXX " strings.
        swprintf_s(sz, L"0x%02X ", bufferAsBytes[i]);
        bufferAsString += sz;
    }

    LOG_COMMENT(L"%s: %s", bufferName, bufferAsString.c_str());
}
