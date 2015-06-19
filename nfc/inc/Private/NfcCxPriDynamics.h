/*++

Module Name: NfccxpriDynamics.h

Abstract:
    Generated header for NFCCX APIs

Environment:
    kernel mode only

    Warning: manual changes to this file will be lost.
--*/

#ifndef _NFCCXPRIDYNAMICS_H_
#define _NFCCXPRIDYNAMICS_H_


typedef struct _NFCCXFUNCTIONS {

    PFN_NFCCXDEVICEINITCONFIG                                 pfnNfcCxDeviceInitConfig;
    PFN_NFCCXDEVICEINITIALIZE                                 pfnNfcCxDeviceInitialize;
    PFN_NFCCXDEVICEDEINITIALIZE                               pfnNfcCxDeviceDeinitialize;
    PFN_NFCCXHARDWAREEVENT                                    pfnNfcCxHardwareEvent;
    PFN_NFCCXNCIREADNOTIFICATION                              pfnNfcCxNciReadNotification;
    PFN_NFCCXSETRFDISCOVERYCONFIG                             pfnNfcCxSetRfDiscoveryConfig;
    PFN_NFCCXSETLLCPCONFIG                                    pfnNfcCxSetLlcpConfig;
    PFN_NFCCXREGISTERSEQUENCEHANDLER                          pfnNfcCxRegisterSequenceHandler;
    PFN_NFCCXUNREGISTERSEQUENCEHANDLER                        pfnNfcCxUnregisterSequenceHandler;

} NFCCXFUNCTIONS, *PNFCCXFUNCTIONS;


typedef struct _NFCCXVERSION {

    ULONG         Size;
    ULONG         FuncCount;
    NFCCXFUNCTIONS  Functions;

} NFCCXVERSION, *PNFCCXVERSION;


_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxDeviceInitConfig)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PNFC_CX_CLIENT_CONFIG Config
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxDeviceInitialize)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxDeviceDeinitialize)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxHardwareEvent)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PNFC_CX_HARDWARE_EVENT HardwareEvent
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxNciReadNotification)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFMEMORY Memory
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxSetRfDiscoveryConfig)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCNFC_CX_RF_DISCOVERY_CONFIG Config
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxSetLlcpConfig)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCNFC_CX_LLCP_CONFIG Config
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxRegisterSequenceHandler)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    NFC_CX_SEQUENCE Sequence,
    _In_
    PFN_NFC_CX_SEQUENCE_HANDLER EvtNfcCxSequenceHandler
    );

_Must_inspect_result_
WDFAPI
NTSTATUS
NFCCXEXPORT(NfcCxUnregisterSequenceHandler)(
    _In_
    PNFCCX_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    NFC_CX_SEQUENCE Sequence
    );


#ifdef NFCCXPRI_DYNAMICS_GENERATE_TABLE

NFCCXVERSION NfccxVersion = {
    sizeof(NFCCXVERSION),
    sizeof(NFCCXFUNCTIONS)/sizeof(PVOID),
    {
        NFCCXEXPORT(NfcCxDeviceInitConfig),
        NFCCXEXPORT(NfcCxDeviceInitialize),
        NFCCXEXPORT(NfcCxDeviceDeinitialize),
        NFCCXEXPORT(NfcCxHardwareEvent),
        NFCCXEXPORT(NfcCxNciReadNotification),
        NFCCXEXPORT(NfcCxSetRfDiscoveryConfig),
        NFCCXEXPORT(NfcCxSetLlcpConfig),
        NFCCXEXPORT(NfcCxRegisterSequenceHandler),
        NFCCXEXPORT(NfcCxUnregisterSequenceHandler),
    }
};

#endif // NFCCXPRI_DYNAMICS_GENERATE_TABLE

#endif // _NFCCXPRIDYNAMICS_H_

