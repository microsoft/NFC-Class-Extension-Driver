/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxTraceLogging.h

Abstract:

    This module defines the trace logging macros for the NfcCx driver
    
Environment:


--*/

#pragma once

#include <TraceLoggingProvider.h>
#include <Telemetry\MicrosoftTelemetry.h> 
#include <TraceLoggingActivity.h>

#define LOG_BUFFER_LENGTH 1024

// Declare provider
TRACELOGGING_DECLARE_PROVIDER(g_hNfcCxProvider);

#define TRACE_LOG_NTSTATUS_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) { \
        TraceLoggingWrite(g_hNfcCxProvider, \
            __FUNCTION__, \
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), \
            TraceLoggingHexInt32(status, "NTStatus")); \
    }
