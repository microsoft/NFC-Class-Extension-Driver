/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSCPresentAbsentDispatcher.h

Abstract:

    For the Smartcard DDI, only a single IOCTL_SMARTCARD_IS_ABSENT request and a single IOCTL_SMARTCARD_IS_PRESENT
    request may be pending at any given time. If a second request for either IOCTL is issued then the driver is
    required to complete the request with STATUS_DEVICE_BUSY.

    This class handles the logic that ensures there is only a single pending IOCTL. This includes cleaning up the
    request if it is canceled.

Environment:

    User-mode Driver Framework

--*/

#pragma once

typedef struct _NFCCX_SC_PRESENT_ABSENT_DISPATCHER {
    WDFREQUEST CurrentRequest;
    BOOLEAN PowerManaged;
} NFCCX_SC_PRESENT_ABSENT_DISPATCHER, *PNFCCX_SC_PRESENT_ABSENT_DISPATCHER;

VOID
NfcCxSCPresentAbsentDispatcherInitialize(
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher,
    _In_ BOOLEAN PowerManaged
    );

NTSTATUS
NfcCxSCPresentAbsentDispatcherSetRequest(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher,
    _In_ WDFREQUEST Request
    );

VOID
NfcCxSCPresentAbsentDispatcherCompleteRequest(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher
    );

//
// Internal functions
//

typedef struct _NFCCX_SC_REQUEST_CONTEXT {
    PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher;
} NFCCX_SC_REQUEST_CONTEXT, *PNFCCX_SC_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NFCCX_SC_REQUEST_CONTEXT, NfcCxSCGetRequestContext)

VOID
NfcCxSCPresentAbsentDispatcherRequestCanceled(
    _In_ WDFREQUEST Request
    );
