/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxSequence.h

Abstract:

    This module declares sequence types in NFC CX

--*/

#pragma once

typedef
NTSTATUS
(FN_NFCCX_CX_SEQUENCE_ENTRY)(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

typedef
void
(FN_NFCCX_CX_SEQUENCE_EXIT)(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

typedef FN_NFCCX_CX_SEQUENCE_ENTRY* PFN_NFCCX_CX_SEQUENCE_ENTRY;
typedef FN_NFCCX_CX_SEQUENCE_EXIT* PFN_NFCCX_CX_SEQUENCE_EXIT;

#define NFCCX_CX_BEGIN_SEQUENCE_MAP(Sequence) static constexpr PFN_NFCCX_CX_SEQUENCE_ENTRY Sequence[] = {
#define NFCCX_CX_SEQUENCE_ENTRY(SequenceInitiate) SequenceInitiate,
#define NFCCX_CX_END_SEQUENCE_MAP() };

struct NFCCX_CX_SEQUENCE
{
    PNFCCX_RF_INTERFACE RFInterface;
    const PFN_NFCCX_CX_SEQUENCE_ENTRY* SequenceSteps;
    UCHAR SequenceStepsNext;
    UCHAR SequenceStepsSize;
    PFN_NFCCX_CX_SEQUENCE_EXIT SequenceCompleteStep;
    bool IsSequenceRunning;
};

NTSTATUS
NfcCxSequenceStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_(SequenceStepsSize) const PFN_NFCCX_CX_SEQUENCE_ENTRY* SequenceSteps,
    _In_ UCHAR SequenceStepsSize,
    _In_ PFN_NFCCX_CX_SEQUENCE_EXIT SequenceCompleteStep,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

template <UCHAR SequenceStepsSize>
NTSTATUS
NfcCxSequenceStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ const PFN_NFCCX_CX_SEQUENCE_ENTRY (&SequenceSteps)[SequenceStepsSize],
    _In_ PFN_NFCCX_CX_SEQUENCE_EXIT SeqComplete,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    )
{
    return NfcCxSequenceStart(RFInterface, SequenceSteps, SequenceStepsSize, SeqComplete, Param1, Param2);
}

void
NfcCxSequenceResume(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ NTSTATUS SequenceStatus,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

void
NfcCxSequenceDispatchResume(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ void* Param1,
    _In_opt_ void* Param2
    );

NTSTATUS
NfcCxSkipSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ UCHAR Value
    );

NTSTATUS
NfcCxRepeatSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFCCX_CX_SEQUENCE* Sequence,
    _In_ UCHAR Value
    );
