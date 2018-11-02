/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Pch.h

Abstract:

    This module is the precompiled header for the smart card library

--*/

#pragma once

#include <windows.h>
#include <ntstatus.h>
#include <ntintsafe.h>

#include <wudfwdm.h>

//
// LibNfc Headers
//
extern "C" {
#include <phLibNfc.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
}

#include "StorageClass.h"
#include "StorageClassLoadKey.h"
#include "StorageClassJewel.h"
#include "StorageClassMifare.h"
#include "StorageClassMifareStd.h"
#include "StorageClassFelica.h"
#include "StorageClassISO15693.h"

//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
// D976D933-B88B-4227-95F8-00513C0986DE
//
#define WPP_CONTROL_GUIDS                                                        \
    WPP_DEFINE_CONTROL_GUID(                                                     \
        NfcSmartCardTraceControl, (D976D933, B88B, 4227, 95F8, 00513C0986DE),    \
        WPP_DEFINE_BIT(DUMMY)                                                    \
        )

#include "tracecommon.h"
