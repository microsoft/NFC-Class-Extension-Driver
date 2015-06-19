/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    stub.c

Abstract:

    This module contains the magical global variable definitions that form
    the binding structures which the UMDF stub library uses to bind the 
    WDF driver linked with this library to the NfcCx class extension.

--*/

#include <windows.h>
#include <wdf.h>
#include <wdfcx.h>
#include <wdfcxbase.h>
#include "NfcCx.h"

PNFCCX_DRIVER_GLOBALS NfccxDriverGlobals = NULL;
PFN_NFC_CX NfccxFunctions[NfccxFunctionTableNumEntries];

WDF_DECLARE_CLASS_BIND_INFO( 
    NfcCx,                          // Class Name
    1,                              // Major version
    0,                              // Minor version
    &NfccxFunctions,                // FunctionTable
    ARRAYSIZE(NfccxFunctions),      // FunctionTableCount
    &(NfccxDriverGlobals)           // ClassBindInfo
    );
