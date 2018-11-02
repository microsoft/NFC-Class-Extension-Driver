/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxTraceLogging.h

Abstract:

    This module defines the trace logging macros for the NfcCx driver
    
Environment:


--*/

#pragma once

#ifdef TELEMETRY

#include <TlgAggregate.h>
#include <Telemetry\MicrosoftTelemetry.h>
#include <Telemetry\MicrosoftTelemetryAssert.h>
#include <Telemetry\MicrosoftTelemetryPrivacy.h>
#include <TraceLoggingActivity.h>

#define LOG_BUFFER_LENGTH 1024

// Declare provider
TRACELOGGING_DECLARE_PROVIDER(g_hNfcCxProvider);

#else

#define TraceLoggingWrite(hProvider, eventName, ...)
#define TlgAggregateWrite(hProvider, eventName, ...)
#define MICROSOFT_TELEMETRY_ASSERT(_exp)
#define MICROSOFT_TELEMETRY_ASSERT_MSG(_exp, _msg)

#endif // TELEMETRY
