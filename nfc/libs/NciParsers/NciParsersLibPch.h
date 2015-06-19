/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NciParsersLibPch.h

Abstract:

    This module is the precompiled header for the Nci Parser library.

--*/

#include <windows.h>

#include "NciDef.h"
#include "NciParsersLib.h"

//
//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
// 9d97cb90-8dee-42b8-b553-d1816be6fb9e
//
#define WPP_CONTROL_GUIDS                                                        \
    WPP_DEFINE_CONTROL_GUID(                                                     \
        NciParsersTraceControl, (9d97cb90, 8dee, 42b8, b553, d1816be6fb9e),      \
        WPP_DEFINE_BIT(DUMMY)                                                    \
        )

#include "tracecommon.h"