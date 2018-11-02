/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Pch.h

Abstract:

    This module is the precompiled header for the proximity library

--*/

#pragma once

#include <windows.h>
#include <ntstatus.h>
#include <ntintsafe.h>
#include <strsafe.h>

#include <wudfwdm.h>

//
// LibNfc Headers
//
extern "C" {
#include <phLibNfc.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
}

#include "NfcProximityBuffer.h"

//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
// 4EB7CC58-145C-4a79-9418-68CD290DD9D4
//
#define WPP_CONTROL_GUIDS                                                        \
    WPP_DEFINE_CONTROL_GUID(                                                     \
        NfcProximityTraceControl, (4EB7CC58, 145C, 4a79, 9418, 68CD290DD9D4),    \
        WPP_DEFINE_BIT(DUMMY)                                                    \
        )

#include "tracecommon.h"
