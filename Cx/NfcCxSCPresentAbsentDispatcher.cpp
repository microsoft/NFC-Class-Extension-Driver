/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSCPresentAbsentDispatcher.cpp

Abstract:

    For the Smartcard DDI, only a single IOCTL_SMARTCARD_IS_ABSENT request and a single IOCTL_SMARTCARD_IS_PRESENT
    request may be pending at any given time. If a second request for either IOCTL is issued then the driver is
    required to complete the request with STATUS_DEVICE_BUSY.

    This class handles the logic that ensures there is only a single pending IOCTL. This includes cleaning up the
    request if it is canceled.

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxSCPresentAbsentDispatcher.tmh"

VOID
NfcCxSCPresentAbsentDispatcherInitialize(
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher,
    _In_ BOOLEAN PowerManaged
    )
/*++

Routine Description:

   Initializes a Smartcard Present/Absent Request Dispatcher.

Arguments:

    Dispatcher - Pointer to the Dispatcher.
    PowerManaged - Whether or not a power reference (NfcCxPower*) will be acquired when there is a pending request.

Return Value:

    VOID

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    Dispatcher->CurrentRequest = nullptr;
    Dispatcher->PowerManaged = PowerManaged;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxSCPresentAbsentDispatcherSetRequest(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

    Stores a request in the dispatcher. If a request is already pending in the dispatcher, the new request will
    be completed with STATUS_DEVICE_BUSY.

Arguments:

    FdoContext - Pointer to the FDO Context
    Dispatcher - Pointer to the Dispatcher.
    Request - The request to store in the dispatcher.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    bool powerReferenceAcquired = false;
    bool cancelCallbackSet = false;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(Request);
    PNFCCX_FILE_CONTEXT fileContext = NfcCxFileGetContext(fileObject);

    // Pre-check if there is a current request.
    if (ReadPointerAcquire((void**)&Dispatcher->CurrentRequest) != nullptr)
    {
        status = STATUS_DEVICE_BUSY;
        TRACE_LINE(LEVEL_ERROR, "An Is Present/Absent request is already pending. %!STATUS!", status);
        goto Done;
    }

    // Allocate and initialize the request context
    PNFCCX_SC_REQUEST_CONTEXT requestContext = nullptr;

    WDF_OBJECT_ATTRIBUTES contextAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&contextAttributes, NFCCX_SC_REQUEST_CONTEXT);

    status = WdfObjectAllocateContext(Request, &contextAttributes, (void**)&requestContext);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to set request's context. %!STATUS!", status);
        goto Done;
    }

    requestContext->Dispatcher = Dispatcher;

    // Add a power reference (if required).
    if (Dispatcher->PowerManaged)
    {
        status = NfcCxPowerFileAddReference(FdoContext->Power, fileContext, NfcCxPowerReferenceType_Proximity);
        if (!NT_SUCCESS(status))
        {
            TRACE_LINE(LEVEL_VERBOSE, "Failed to acquire power reference. %!STATUS!", status);
            goto Done;
        }

        powerReferenceAcquired = true;
    }

    // Setup cancel callback.
    status = WdfRequestMarkCancelableEx(Request, NfcCxSCPresentAbsentDispatcherRequestCanceled);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to set request canceled callback. %!STATUS!", status);
        goto Done;
    }

    // Add a ref-count to the request, which is owned by the cancel callback.
    WdfObjectReference(Request);
    cancelCallbackSet = true;

    // Try to set the current request.
    void* previousRequest = InterlockedCompareExchangePointer((void**)&Dispatcher->CurrentRequest, /*exchange*/ Request, /*compare*/ nullptr);

    // Check if we already have a pending request.
    if (previousRequest != nullptr)
    {
        status = STATUS_DEVICE_BUSY;
        TRACE_LINE(LEVEL_ERROR, "An Is Present/Absent request is already pending. %!STATUS!", status);
        goto Done;
    }

Done:
    if (!NT_SUCCESS(status))
    {
        if (cancelCallbackSet)
        {
            NTSTATUS unmarkStatus = WdfRequestUnmarkCancelable(Request);
            if (unmarkStatus != STATUS_CANCELLED)
            {
                // Cancel callback will not be called.
                // So release cancel callback's request ref-count.
                WdfObjectDereference(Request);
            }
        }

        if (powerReferenceAcquired)
        {
            NfcCxPowerFileRemoveReference(FdoContext->Power, fileContext, NfcCxPowerReferenceType_Proximity);
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxSCPresentAbsentDispatcherCompleteRequest(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ PNFCCX_SC_PRESENT_ABSENT_DISPATCHER Dispatcher
    )
/*++

Routine Description:

    If there is a request in the dispatcher, the request is completed.

Arguments:

    FdoContext - Pointer to the FDO Context
    Dispatcher - Pointer to the Dispatcher

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Remove the current request from the dispatcher.
    WDFREQUEST request = (WDFREQUEST)InterlockedExchangePointer((void**)&Dispatcher->CurrentRequest, nullptr);

    // Check if another thread has already completed the request.
    if (request == nullptr)
    {
        // Nothing to do.
        goto Done;
    }

    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(request);
    PNFCCX_FILE_CONTEXT fileContext = NfcCxFileGetContext(fileObject);

    // Try to unset the request's cancel callback.
    status = WdfRequestUnmarkCancelable(request);
    if (status != STATUS_CANCELLED)
    {
        // The cancel callback will not be called.
        // So we have to release its ref-count for it.
        WdfObjectDereference(request);

        if (!NT_SUCCESS(status))
        {
            // Something is pretty wrong.
            NT_ASSERT(false);

            TRACE_LINE(LEVEL_ERROR, "Failed to unset request canceled callback. %!STATUS!", status);
            goto Done;
        }
    }

    // Release power reference (if required).
    if (Dispatcher->PowerManaged)
    {
        NfcCxPowerFileRemoveReference(FdoContext->Power, fileContext, NfcCxPowerReferenceType_Proximity);
    }

    // Complete the request
    WdfRequestComplete(request, STATUS_SUCCESS);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return;
}

VOID
NfcCxSCPresentAbsentDispatcherRequestCanceled(
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

    Called when a pending request has been canceled.

Arguments:

    Request - The request

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(Request);
    WDFDEVICE device = WdfFileObjectGetDevice(fileObject);

    PNFCCX_FILE_CONTEXT fileContext = NfcCxFileGetContext(fileObject);
    PNFCCX_FDO_CONTEXT fdoContext = NfcCxFdoGetContext(device);
    PNFCCX_SC_REQUEST_CONTEXT requestContext = NfcCxSCGetRequestContext(Request);
    PNFCCX_SC_PRESENT_ABSENT_DISPATCHER dispatcher = requestContext->Dispatcher;

    // Remove this request from the dispatcher
    void* previousRequest = InterlockedCompareExchangePointer((void**)&dispatcher->CurrentRequest, /*exchange*/ nullptr, /*compare*/ Request);

    // Check if another thread has already completed the request.
    if (previousRequest != Request)
    {
        // Request already completed by a different thread.
        // Nothing to do.
        goto Done;
    }

    // Release power reference (if required).
    if (dispatcher->PowerManaged)
    {
        NfcCxPowerFileRemoveReference(fdoContext->Power, fileContext, NfcCxPowerReferenceType_Proximity);
    }

    // Complete the request.
    TRACE_LINE(LEVEL_ERROR, "Smartcard Present/Absent request canceled. %!STATUS!", STATUS_CANCELLED);
    WdfRequestComplete(Request, STATUS_CANCELLED);

Done:
    // Release the cancel callback's extra ref-count
    WdfObjectDereference(Request);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}
