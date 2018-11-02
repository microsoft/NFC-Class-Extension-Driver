/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxPch.h

Abstract:

    Precompiled Header for the NFC Class Extension driver

Environment:

    User-mode Driver Framework

--*/

#pragma once

#include <windows.h>

#include <malloc.h>
#include <stddef.h>

#include <devioctl.h>
#include <objbase.h>
#include <ncidef.h>

#include <windows.devices.smartcards.h>

#include <initguid.h>

#include <devpropdef.h>
#include <winsmcrd.h>
#include <nfpdev.h>
#include <nfcsedev.h>
#include <nfcdtadev.h>
#include <nfcradiodev.h>

#include <wdf.h>
#include <wudfwdm.h>

#include <ntintsafe.h>

// Avoid conflicts with ntdef.h
#define _NTDEF_
#include <ntstrsafe.h>

//
// LibNfc Headers
//
WDF_EXTERN_C_START
#include <phLibNfc.h>
#include <phLibNfc_Snep.h>
#include <phOsalNfc_Timer.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phTmlNfc.h>
#include <phLibNfc_ioctl.h>
WDF_EXTERN_C_END

#include <NfcCx.h>
#include <WdfCxProxy.h>

//
// Forward declarations
//
typedef struct _NFCCX_CLIENT_GLOBALS      NFCCX_CLIENT_GLOBALS,     * PNFCCX_CLIENT_GLOBALS;
typedef struct _NFCCX_FDO_CONTEXT         NFCCX_FDO_CONTEXT,        * PNFCCX_FDO_CONTEXT;
typedef struct _NFCCX_FILE_CONTEXT        NFCCX_FILE_CONTEXT,       * PNFCCX_FILE_CONTEXT;
typedef struct _NFCCX_STATE_INTERFACE     NFCCX_STATE_INTERFACE,    * PNFCCX_STATE_INTERFACE;
typedef struct _NFCCX_NFP_INTERFACE       NFCCX_NFP_INTERFACE,      * PNFCCX_NFP_INTERFACE;
typedef struct _NFCCX_RF_INTERFACE        NFCCX_RF_INTERFACE,       * PNFCCX_RF_INTERFACE;
typedef struct _NFCCX_LLCP_INTERFACE      NFCCX_LLCP_INTERFACE,     * PNFCCX_LLCP_INTERFACE;
typedef struct _NFCCX_SNEP_INTERFACE      NFCCX_SNEP_INTERFACE,     * PNFCCX_SNEP_INTERFACE;
typedef struct _NFCCX_SC_INTERFACE        NFCCX_SC_INTERFACE,       * PNFCCX_SC_INTERFACE;
typedef struct _NFCCX_ESE_INTERFACE       NFCCX_ESE_INTERFACE,      * PNFCCX_ESE_INTERFACE;
typedef struct _NFCCX_SE_INTERFACE        NFCCX_SE_INTERFACE,       * PNFCCX_SE_INTERFACE;
typedef struct _NFCCX_DTA_INTERFACE       NFCCX_DTA_INTERFACE,      * PNFCCX_DTA_INTERFACE;
typedef struct _NFCCX_POWER_MANAGER       NFCCX_POWER_MANAGER,      * PNFCCX_POWER_MANAGER;

enum NFC_CX_POWER_RF_STATE : LONG;

DEFINE_GUID(GUID_NULL, 0,0,0,0,0,0,0,0,0,0,0);

//
// Resource tracking tags
//
#define SEND_LIST_ENTRY_TAG                     (PVOID)'tels'  // Send List Entry Tag

//
// Registry settings
//
#define NFCCX_REG_NFC_RADIO_STATE               L"NfcRadioState"
#define NFCCX_REG_NFC_FLIGHT_MODE               L"NfcRadioFlightMode"
#define NFCCX_REG_NFC_RADIO_STATE_BEFORE_FLIGHT_MODE L"NfcRadioStateBeforeFlightMode"
#define NFCCX_REG_SESSION_IDENTIFIER            L"SessionIdentifier"
#define NFCCX_REG_LOG_DATA_MESSAGES             L"LogNciDataMessages"
#define NFCCX_REG_DISABLE_POWER_MANAGER_STOP_IDLE L"DisablePowerManagerStopIdle"
#define NFCCX_REG_DISABLE_RF_INTERFACES         L"DisableRfInterfaces"

//
// As per spec The maximum length of the
// Protocol component and sub type is 250 characters
//
#define MAX_TYPE_LENGTH 250
#define MIN_TYPE_LENGTH 1

//
// Namespace
//
#define PUBS_NAMESPACE                      L"Pubs\\"
#define PUBS_NAMESPACE_LENGTH               (ARRAYSIZE(PUBS_NAMESPACE) - 1)

#define SUBS_NAMESPACE                      L"Subs\\"
#define SUBS_NAMESPACE_LENGTH               (ARRAYSIZE(SUBS_NAMESPACE) - 1)

#define SMARTCARD_READER_NAMESPACE          L"SmardCardReader"
#define SMARTCARD_READER_NAMESPACE_LENGTH   (ARRAYSIZE(SMARTCARD_READER_NAMESPACE) - 1)

#define SEEVENTS_NAMESPACE                  L"SEEvents"
#define SEEVENTS_NAMESPACE_LENGTH           (ARRAYSIZE(SEEVENTS_NAMESPACE) - 1)

#define SEMANAGER_NAMESPACE                 L"SEManage"
#define SEMANAGER_NAMESPACE_LENGTH          (ARRAYSIZE(SEMANAGER_NAMESPACE) - 1)

#define EMBEDDED_SE_NAMESPACE               L"ESE"
#define EMBEDDED_SE_NAMESPACE_LENGTH        (ARRAYSIZE(EMBEDDED_SE_NAMESPACE) - 1)

//
// DDI Module Abstraction
//
typedef
BOOLEAN
(FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED)(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ ULONG IoControlCode
    );
typedef FN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED *PFN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED;

typedef
NTSTATUS
(FN_NFCCX_DDI_MODULE_IODISPATCH)(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    );
typedef FN_NFCCX_DDI_MODULE_IODISPATCH *PFN_NFCCX_DDI_MODULE_IODISPATCH;

typedef struct _NFCCX_DDI_MODULE {
    LPWSTR Name;
    BOOLEAN IsNullFileObjectOk;
    BOOLEAN IsAppContainerAllowed;
    PFN_NFCCX_DDI_MODULE_ISIOCTLSUPPORTED IsIoctlSupported;
    PFN_NFCCX_DDI_MODULE_IODISPATCH IoDispatch;
} NFCCX_DDI_MODULE, *PNFCCX_DDI_MODULE;

//
// Local Includes
//
#ifdef EVENT_WRITE
#include "NfcCx.Events.h"
#endif

#include "NfcCxUtils.h"

#include "StorageCardManager.h"
#include "StorageClassLoadKey.h"

#include "NfcProximityBuffer.h"
#include "NfcCxState.h"
#include "NfcCxSequence.h"
#include "NfcCxNFP.h"
#include "NfcCxSCPresentAbsentDispatcher.h"
#include "NfcCxSC.h"
#include "NfcCxESE.h"
#include "NfcCxSE.h"
#include "NfcCxTml.h"
#include "NciParsersLib.h" // for ETW events

#include "NfcCxLLCP.h"
#include "NfcCxSNEP.h"
#include "NfcCxRF.h"
#include "NfcCxDTA.h"
#include "device.h"
#include "driver.h"
#include "fileObject.h"
#include "power.h"

#define NFCCX_POOL_TAG 'xcfN'

//
//
// Define the Control GUID and a DUMMY flag.
// The DUMMY flag is just needed as an entry point into the WPP control
// structures. ETW levels are used instead of WPP flags.
// 351734b9-8706-4cee-9247-04accd448c76
//
#define NFCCX_CONTROL_GUID                                             \
    WPP_DEFINE_CONTROL_GUID(                                          \
        NfcCxTraceControl, (351734b9,8706,4cee,9247,04accd448c76),    \
        WPP_DEFINE_BIT(DUMMY)                                         \
        )

// Note: The WPP_CONTROL_GUIDS has to include the control guids for all the static libraries that are linked.
#define NFCCORELIB_CONTROL_GUID                                                \
    WPP_DEFINE_CONTROL_GUID(                                                   \
        NfcCoreLibTraceControl, (696D4914, 12A4, 422C, A09E, E7E0EB25806A),    \
        WPP_DEFINE_BIT(TF_NCI)                                                 \
        WPP_DEFINE_BIT(TF_OSAL)                                                \
        WPP_DEFINE_BIT(TF_LIBNFC)                                              \
        WPP_DEFINE_BIT(TF_FRI)                                                 \
        WPP_DEFINE_BIT(TF_DNLD)                                                \
        WPP_DEFINE_BIT(TF_RBTR)                                                \
        WPP_DEFINE_BIT(TF_LLCP)                                                \
        WPP_DEFINE_BIT(TF_SNEP)                                                \
        WPP_DEFINE_BIT(TF_NDEF)                                                \
        )

// WPP Control guids
#define WPP_CONTROL_GUIDS                       \
        NFCCORELIB_CONTROL_GUID                 \
        NFCCX_CONTROL_GUID                      \

#define NFCCX_TRACING_ID      L"Microsoft\\Nfc\\NfcCx"

#include "NfcCxTracing.h"
#include "NfcCxTraceLogging.h"
