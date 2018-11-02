//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#define LOG_COMMENT(fmt, ...) ::WEX::Logging::Log::Comment(::WEX::Common::String().Format(fmt, __VA_ARGS__))

void LogByteBuffer(
    _In_ PCWSTR bufferName,
    _In_reads_bytes_(bufferLength) const void* buffer,
    _In_ size_t bufferLength);
