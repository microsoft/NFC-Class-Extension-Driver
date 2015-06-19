/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcCxRMPch

Abstract:
    Precompiled header for Nfc Radio Manager component

Notes:

Environment:
   User mode.

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
#include <radiomgr.h>
#include <dbt.h>
#include <initguid.h>
#include <devpkey.h>
#include <DevQuery.H>
#include <deviceassociationkeys.h>
#include <cfgmgr32.h>
#include <resource.h>

#include <tracecommon.h>

//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(03162b40,c068,478f,8f8d,11279331e7C9),  \
    WPP_DEFINE_BIT(DUMMY) \
    )

#include "NfcRadioDev.h"

//
// Local includes
//
#include "NfcRadioMedia.h"
#include "NfcRadioInstance.h"
#include "NfcRadioInstanceCollection.h"
#include "NfcRadioManager.h"

#define DECLARE_OBJECT_ENTRY_AUTO(clsid, class) \
    extern "C" ATL::_ATL_OBJMAP_ENTRY __objMap_##class = {&clsid, class::UpdateRegistry, class::_ClassFactoryCreatorClass::CreateInstance, class::_CreatorClass::CreateInstance, NULL, 0, class::GetObjectDescription, class::GetCategoryMap, class::ObjectMain };

extern HINSTANCE g_hInstance;

#define NFC_RADIO_MANAGER_NAME TEXT("NfcRadioMediaClass")

