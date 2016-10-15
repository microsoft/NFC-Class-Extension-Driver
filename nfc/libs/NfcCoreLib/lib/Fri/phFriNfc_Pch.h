/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:
    phFriNfc_Pch.h

Abstract:
    This module contains the precompiled headers for the FRI component

Environment:
   User mode.

--*/

#pragma once

#include <phNfcConfig.h>

#include <phNfcCompId.h>
#include <phOsalNfc.h>
#include <phOsalNfc_Timer.h>
#include <phOsalNfc_Internal.h>
#include <phNfcConfig.h>
#include <phNfcHalTypes2.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNfcConfig.h>
#include <phNfcTypes_Mapping.h>

#include <phFriNfc.h>
#include <phFriNfc_LlcpMac.h>
#include <phFriNfc_MapTools.h>
#include <phFriNfc_OvrHal.h>
#include <phFriNfc_NdefReg.h>
#include <phFriNfc_NdefRecord.h>
#include <phFriNfc_NdefMap.h>
#include <phFriNfc_FelicaMap.h>
#include <phFriNfc_MifareStdMap.h>
#include <phFriNfc_ISO15693Map.h>
#include <phFriNfc_MifareULMap.h>
#include <phFriNfc_TopazMap.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phFriNfc_MifULFormat.h>
#include <phFriNfc_MifStdFormat.h>
#include <phFriNfc_ISO15693Format.h>
#include <phFriNfc_TopazFormat.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <memory.h>
#include <assert.h>
#include <intsafe.h>

#include "NfcCoreLibTracing.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)
