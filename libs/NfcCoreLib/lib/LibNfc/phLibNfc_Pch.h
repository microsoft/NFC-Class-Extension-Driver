/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:
    phLibNfc_Pch.h

Abstract:
    This module contains the precompiled headers for the NfcCoreLib component

Environment:
   User mode.

--*/

#pragma once

#include <phLibNfc.h>

#include <phNfcConfig.h>

#include <phNfcCompId.h>
#include <phOsalNfc.h>
#include <phOsalNfc_Timer.h>
#include <phOsalNfc_Internal.h>
#include <phTmlNfc.h>
#include <phNfcConfig.h>
#include <phNfcHalTypes2.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNfcConfig.h>
#include <phNfcTypes_Mapping.h>

#include <phNciNfc.h>
#include <phNciNfcTypes.h>
#include "phNciNfc_Context.h"
#include "phNciNfc_Core.h"
#include "phNciNfc_CoreStatus.h"
#include "phNciNfc_CoreMemMgr.h"
#include "phNciNfc_CoreUtils.h"
#include "phNciNfc_Utils.h"
#include "phNciNfc_Common.h"
#include "phNciNfc_RfConfig.h"
#include "phNciNfc_ListenMgmt.h"
#include "phNciNfc_ListenNfcDep.h"
#include "phNciNfc_ListenIsoDep.h"
#include "phNciNfc_RFReader.h"
#include "phNciNfc_Init.h"
#include "phNciNfc_Discovery.h"
#include "phNciNfc_LogicalConn.h"
#include "phNciNfc_Common.h"
#include "phNciNfc_CoreRecvMgr.h"
#include "phNciNfc_PollMgmt.h"
#include "phNciNfc_DbgDescription.h"
#include "phNciNfc_LoglConnMgmt.h"
#include "phNciNfc_NfceeMgmt.h"
#include "phNciNfc_TlvUtils.h"
#include "phNciNfc_NfcIMgmt.h"
#include "phNciNfc_PollNfcDep.h"
#include "phNciNfc_ListenNfcDep.h"
#include "phNciNfc_NfcIMgmt.h"
#include "phHciNfc_Core.h"
#include "phHciNfc_CoreRxMemMgr.h"
#include "phHciNfc_Pipe.h"

#include <phLibNfc.h>
#include "phLibNfc_Internal.h"
#include "phLibNfc_Initiator.h"
#include <phLibNfc_Ioctl.h>
#include "phLibNfc_IoctlUtils.h"
#include "phLibNfc_State.h"
#include "phLibNfc_Sequence.h"
#include "phLibNfc_Init.h"
#include "phLibNfc_Discovery.h"
#include "phLibNfc_Target.h"
#include "phLibNfc_Se.h"
#include "phLibNfc_Hci.h"

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
#include <phFriNfc_MifULFormat.h>
#include <phFriNfc_MifStdFormat.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phFriNfc_ISO15693Format.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <memory.h>
#include <intsafe.h>
#include <math.h>

#include "NfcCoreLibTracing.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)
