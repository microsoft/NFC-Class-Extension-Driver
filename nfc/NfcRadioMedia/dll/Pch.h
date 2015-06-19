/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:

    pch.h

Abstract:

    Precompiled header file

Environment:

    User mode only.

Revision History:

--*/

#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h> 
#include <windows.h>

//ATL includes
#include <ATLBase.h>
#include <ATLCom.h>
#include <ATLColl.h>
#include <ATLStr.h>

#include <strsafe.h>
#include <dbt.h>
#include <initguid.h>
#include <devpkey.h>
#include <cfgmgr32.h>

#include <nfcradiodev.h>
#include <nfcradiomedia.h>

#include <tracecommon.h>

//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(E79E077F,E35E,4263,B44B,FDBEE03D2C24),  \
    WPP_DEFINE_BIT(DUMMY) \
    )

#define IMPLEMENT_OBJECT_ENTRY_AUTO(class) \
    extern "C" ATL::_ATL_OBJMAP_ENTRY __objMap_##class; \
    extern "C" __declspec(allocate("ATL$__m")) ATL::_ATL_OBJMAP_ENTRY* const __pobjMap_##class = &__objMap_##class; \
    OBJECT_ENTRY_PRAGMA(class)