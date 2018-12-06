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
    std::wstring result = bufferName;
    result += L": ";

    const uint8_t* bufferAsBytes = reinterpret_cast<const uint8_t*>(buffer);
    for (const uint8_t* itr = bufferAsBytes; itr != bufferAsBytes + bufferLength; itr++)
    {
        WCHAR sz[4]; // provide enough space for "0xXX " strings.
        swprintf_s(sz, L"%02X ", *itr);
        result += sz;
    }

    WEX::Logging::Log::Comment(result.c_str());
}
