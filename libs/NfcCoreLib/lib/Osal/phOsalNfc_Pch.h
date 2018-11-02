/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:
    phOsalNfc_Pch.h

Abstract:
    This module contains the precompiled headers for the OSAL component

Environment:
   User mode.

--*/

#pragma once

#include <phOsalNfc.h>
#include <phOsalNfc_Timer.h>
#include "phOsalNfc_Internal.h"

#include <phNfcConfig.h>
#include <phNfcHalTypes2.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNfcConfig.h>
#include <phNfcTypes_Mapping.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <memory.h>
#include <intsafe.h>

#include <wdf.h>
#include <wudfwdm.h>

#include "NfcCoreLibTracing.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)
