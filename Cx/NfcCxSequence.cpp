/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxSequence.cpp

Abstract:

    This module implements the sequence in the NFC CX

--*/

#include "NfcCxPch.h"

#include "NFcCxSequence.tmh"

static NTSTATUS
NfcCxSequenceRun(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

NTSTATUS
NfcCxSequenceStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_(SequenceStepsSize) const PFN_NFCCX_CX_SEQUENCE_ENTRY* SequenceSteps,
    _In_ UCHAR SequenceStepsSize,
    _In_ PFN_NFCCX_CX_SEQUENCE_EXIT SequenceCompleteStep,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    )
/*++

Routine Description:

    This routine for initiating sequences.

Arguments:

    RFInterface - A pointer to the RF interface
    SequenceSteps - A pointer to a sequence handler list
    SequenceStepsSize - The size of the sequence handler list
    SequenceCompleteStep - The handler called when the sequence has completed
    Param - The parameters for the sequence

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    // Check if there is an existing sequence.
    if (RFInterface->CurrentSequence)
    {
        // Check if the sequence handling is being called recursively.
        if (RFInterface->CurrentSequence->IsSequenceRunning)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            TRACE_LINE(LEVEL_ERROR, "Failed to start a new sequence. An existing sequence is currently executing. %!STATUS!", status);
            goto Done;
        }

        TRACE_LINE(LEVEL_WARNING, "WARNING: Overriding existing sequence.");

        // Cleanup existing sequence.
        NFCCX_CX_SEQUENCE* existingSequence = RFInterface->CurrentSequence;
        RFInterface->CurrentSequence = nullptr;
        delete existingSequence;
    }

    // Allocate the data context for the sequence.
    auto sequence = new NFCCX_CX_SEQUENCE{ RFInterface, SequenceSteps, 0, SequenceStepsSize, SequenceCompleteStep, false };
    if (!sequence)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate NFCCX_CX_SEQUENCE. %!STATUS!", status);
        goto Done;
    }

    // Set the sequence.
    RFInterface->CurrentSequence = sequence;

    // Run the first handler within the sequence.
    NTSTATUS sequenceStatus = NfcCxSequenceRun(RFInterface, sequence, Param1, Param2);
    if (sequenceStatus != STATUS_PENDING)
    {
        // The sequence handler completed synchronously (success or failure).
        // Defer the completed handler to avoid reentrancy issues.
        TRACE_LINE(LEVEL_INFO, "Sequence handler completed synchronously. Deferring complete function. %!STATUS!", sequenceStatus);

        // Ensure that the complete function is called when the sequence is next resumed.
        sequence->SequenceStepsNext = sequence->SequenceStepsSize;

        // Queue the sequence handler.
        NfcCxSequenceDispatchResume(RFInterface, sequence, sequenceStatus, Param1, Param2);

        // Sequence will complete asynchronously.
        status = STATUS_PENDING;
        goto Done;
    }

    // Sequence will complete asynchronously.
    status = STATUS_PENDING;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

void
NfcCxSequenceResume(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ NTSTATUS SequenceStatus,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    )
/*++

Routine Description:

    Resumes a sequence that was suspended. This is usually called when an asynchronous operation has completed.

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    SequenceStatus - The status of the sequence
    Param - The parameters for the sequence

Return Value:

    void

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (RFInterface->CurrentSequence != Sequence)
    {
        TRACE_LINE(LEVEL_WARNING, "Sequence mismatch.");
        goto Done;
    }

    if (!Sequence)
    {
        TRACE_LINE(LEVEL_WARNING, "Empty sequence.");
        goto Done;
    }

    // Check if the sequence handling is being called recursively.
    if (Sequence->IsSequenceRunning)
    {
        TRACE_LINE(LEVEL_INFO, "Sequence handling already in progress. Deferring.");
        NfcCxSequenceDispatchResume(RFInterface, Sequence, SequenceStatus, Param1, Param2);
        goto Done;
    }

    if (SequenceStatus == STATUS_PENDING)
    {
        SequenceStatus = STATUS_CANCELLED;
        TRACE_LINE(LEVEL_WARNING, "Sequence result has an unexpected value of STATUS_PENDING. Converting to STATUS_CANCELLED.");
    }

    if (SequenceStatus == STATUS_SUCCESS)
    {
        // Call the next sequence handler.
        SequenceStatus = NfcCxSequenceRun(RFInterface, Sequence, Param1, Param2);
    }

    if (SequenceStatus != STATUS_PENDING)
    {
        // The sequence has completed (success or failure).

        // Allow another sequence to start during the complete handler.
        RFInterface->CurrentSequence = nullptr;

        // Call the complete handler.
        Sequence->SequenceCompleteStep(RFInterface, SequenceStatus, Param1, Param2);

        // Cleanup sequence's memory.
        delete Sequence;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxSequenceRun(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    )
/*++

Routine Description:

    Processes the next handler in the sequence

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    SequenceStatus - The status of the sequence
    Param - The parameters for the sequence

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    Sequence->IsSequenceRunning = true;

    while (Sequence->SequenceStepsNext < Sequence->SequenceStepsSize)
    {
        // Invoke the next sequence handler.
        PFN_NFCCX_CX_SEQUENCE_ENTRY handler = Sequence->SequenceSteps[Sequence->SequenceStepsNext];
        Sequence->SequenceStepsNext++;

        status = handler(RFInterface, Param1, Param2);
        if (status == STATUS_SUCCESS)
        {
            // The sequence handler completed its work synchronously. Continue on to the next handler.
            continue;
        }

        // Either an error occured or an asynchronous operation is pending.
        break;
    }

    Sequence->IsSequenceRunning = false;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

void
NfcCxSequenceDispatchResume(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    )
/*++

Routine Description:

    Queues a message on the LibNfc thread that will call NfcCxSequenceResume. This helps prevent reentrancy issues.

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    Status - The status of the sequence
    Param - The parameters for the sequence

Return Value:

    VOID

--*/
{
    NfcCxPostLibNfcThreadMessage(RFInterface, LIBNFC_SEQUENCE_RESUME, (UINT_PTR)Sequence, (UINT_PTR)Status, (UINT_PTR)Param1, (UINT_PTR)Param2);
}

NTSTATUS
NfcCxSkipSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ UCHAR Value
    )
/*++

Routine Description:

    This routine is used to skip sequences

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    Value - The number of sequences to skip

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    if (RFInterface->CurrentSequence != Sequence)
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Sequence mismatch. %!STATUS!", status);
        goto Done;
    }

    if (Sequence->SequenceStepsNext + Value > Sequence->SequenceStepsSize)
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Value to increment is invalid. %!STATUS!", status);
        goto Done;
    }

    Sequence->SequenceStepsNext += Value;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}

NTSTATUS
NfcCxRepeatSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ UCHAR Value
    )
/*++

Routine Description:

    This routine is used to repeat sequences

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    Value - The number of sequences to repeat

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    if (RFInterface->CurrentSequence != Sequence)
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Sequence mismatch. %!STATUS!", status);
        goto Done;
    }

    if (Sequence->SequenceStepsNext < Value)
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Value to decrement is invalid. %!STATUS!", status);
        goto Done;
    }

    Sequence->SequenceStepsNext -= Value;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}
