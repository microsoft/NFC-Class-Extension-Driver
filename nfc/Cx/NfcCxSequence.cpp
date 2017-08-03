/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxSequence.cpp

Abstract:

    This module implements the sequence in the NFC CX

--*/

#include "NfcCxPch.h"

#include "NFcCxSequence.tmh"

NTSTATUS
NfcCxSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
/*++

Routine Description:

    This routine for initiating and processing sequences

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    Status - The status of the sequence
    Param - The parameters for the sequence

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (RFInterface->pSeqHandler != Sequence) {
        TRACE_LINE(LEVEL_WARNING, "Sequence mismatch");
        Status = STATUS_INVALID_PARAMETER;
    }
    else if (!RFInterface->bSeqHandler) {
        NT_ASSERT(RFInterface->SeqMax > 0);
        RFInterface->bSeqHandler = TRUE;

        if ((STATUS_SUCCESS == Status) && (NULL != RFInterface->pSeqHandler)) {
            if (RFInterface->SeqNext <= RFInterface->SeqMax) {
                if (0 == RFInterface->SeqNext) {
                    if (NULL != RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate) {
                        Status = RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate(RFInterface, Status, Param1, Param2);
                    }

                    while ((STATUS_SUCCESS == Status) && (RFInterface->SeqMax > RFInterface->SeqNext)) {
                        RFInterface->SeqNext++;

                        if (NULL != RFInterface->pSeqHandler[RFInterface->SeqNext-1].SequenceProcess) {
                             Status = RFInterface->pSeqHandler[RFInterface->SeqNext-1].SequenceProcess(RFInterface, Status, Param1, Param2);
                        }

                        if ((STATUS_SUCCESS == Status) && (NULL != RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate)) {
                            Status = RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate(RFInterface, Status, Param1, Param2);
                        }
                    }
                }
                else {
                    while ((STATUS_SUCCESS == Status) && (RFInterface->SeqMax > RFInterface->SeqNext)) {
                        if (NULL != RFInterface->pSeqHandler[RFInterface->SeqNext-1].SequenceProcess) {
                             Status = RFInterface->pSeqHandler[RFInterface->SeqNext-1].SequenceProcess(RFInterface, Status, Param1, Param2);
                        }

                        if ((STATUS_SUCCESS == Status) && (NULL != RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate)) {
                            Status = RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceInitiate(RFInterface, Status, Param1, Param2);
                        }

                        RFInterface->SeqNext = (STATUS_SUCCESS == Status) ? RFInterface->SeqNext+1 : RFInterface->SeqNext;
                    }
                }
            }
        }
        else {
            RFInterface->SeqNext = RFInterface->SeqMax;
        }

        if ((STATUS_SUCCESS != Status) && (STATUS_PENDING != Status)) {
            RFInterface->SeqNext = RFInterface->SeqMax;
        }

        if (RFInterface->SeqNext == RFInterface->SeqMax) {
            if (NULL != RFInterface->pSeqHandler &&
                NULL != RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceProcess) {
                RFInterface->pSeqHandler[RFInterface->SeqNext].SequenceProcess(RFInterface, Status, Param1, Param2);
            }
        }
        else {
            RFInterface->SeqNext++;
        }

        RFInterface->bSeqHandler = FALSE;
    }
    else {
        TRACE_LINE(LEVEL_INFO, "Sequence handling in progress");
        NfcCxPostLibNfcThreadMessage(RFInterface, LIBNFC_SEQUENCE_HANDLER, (UINT_PTR)Sequence, (UINT_PTR)Status, (UINT_PTR)Param1, (UINT_PTR)Param2);
        Status =  STATUS_PENDING;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

VOID
NfcCxInternalSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
/*++

Routine Description:

    This routine is invoked by completing the sequence

Arguments:

    RFInterface - A pointer to the RF interface
    Sequence - A pointer to sequence
    Status - The status of the sequence
    Param - The parameters for the sequence

Return Value:

    VOID

--*/
{
    NfcCxPostLibNfcThreadMessage(RFInterface, LIBNFC_SEQUENCE_HANDLER, (UINT_PTR)Sequence, (UINT_PTR)Status, (UINT_PTR)Param1, (UINT_PTR)Param2);
}

UCHAR
NfcCxGetSequenceLength(
    PCNFCCX_CX_SEQUENCE Sequence
    )
/*++

Routine Description:

    This routine is used to calculate the length of the sequence

Arguments:

    Sequence - A pointer to sequence

Return Value:

    The length of the sequence

--*/
{
    UCHAR length = 0;

    if (Sequence == NULL) {
        goto Done;
    }

    while (NULL != Sequence->SequenceInitiate) {
        length++;
        Sequence++;
    }

Done:
    return length;
}

NTSTATUS
NfcCxSkipSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
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
    NTSTATUS status = STATUS_SUCCESS;

    NT_ASSERT(RFInterface);
    NT_ASSERT(Sequence);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (0 == Value) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (Sequence != RFInterface->pSeqHandler) {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Sequence mismatch, can not decrement the current sequence");
        goto Done;
    }

    if ((RFInterface->SeqNext + Value) > RFInterface->SeqMax) {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Value to increment is invalid");
        goto Done;
    }

    RFInterface->SeqNext += Value;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}

NTSTATUS
NfcCxRepeatSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
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
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(RFInterface);
    NT_ASSERT(Sequence);

    if (0 == Value) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    if (Sequence != RFInterface->pSeqHandler) {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Sequence mismatch, can not decrement the current sequence");
        goto Done;
    }

    if (RFInterface->SeqNext < Value) {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Value to decrement is invalid");
        goto Done;
    }

    RFInterface->SeqNext -= Value;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}
