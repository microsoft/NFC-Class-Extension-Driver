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
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

typedef
NTSTATUS
(FN_NFCCX_CX_SEQUENCE_EXIT)(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

typedef FN_NFCCX_CX_SEQUENCE_ENTRY *PFN_NFCCX_CX_SEQUENCE_ENTRY;
typedef FN_NFCCX_CX_SEQUENCE_EXIT *PFN_NFCCX_CX_SEQUENCE_EXIT;

typedef struct _NFCCX_CX_SEQUENCE {
    PFN_NFCCX_CX_SEQUENCE_ENTRY SequenceInitiate;
    PFN_NFCCX_CX_SEQUENCE_EXIT  SequenceProcess;
} NFCCX_CX_SEQUENCE, *PNFCCX_CX_SEQUENCE;

typedef const NFCCX_CX_SEQUENCE* PCNFCCX_CX_SEQUENCE;

#define NFCCX_CX_BEGIN_SEQUENCE_MAP(Sequence) static NFCCX_CX_SEQUENCE Sequence[] = {
#define NFCCX_CX_SEQUENCE_ENTRY(SequenceInitiate) { SequenceInitiate, NULL },
#define NFCCX_CX_END_SEQUENCE_MAP() { NULL, NULL } };

typedef struct _NFCCX_CX_SEQUENCE_REQUEST {
    PNFCCX_RF_INTERFACE     RFInterface;
    NTSTATUS                Status;
    VOID*                   Param1;
    VOID*                   Param2;
} NFCCX_CX_SEQUENCE_REQUEST, *PNFCCX_CX_SEQUENCE_REQUEST;

NTSTATUS
NfcCxSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

VOID
NfcCxInternalSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

UCHAR
NfcCxGetSequenceLength(
    PCNFCCX_CX_SEQUENCE Sequence
    );

NTSTATUS
NfcCxSkipSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ UCHAR Value
    );

NTSTATUS
NfcCxRepeatSequence(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PNFCCX_CX_SEQUENCE Sequence,
    _In_ UCHAR Value
    );

#define NFCCX_INIT_SEQUENCE(RFInterface, Sequence) \
    (RFInterface)->pSeqHandler = (Sequence); \
    (RFInterface)->SeqNext = 0; \
    (RFInterface)->SeqMax = NfcCxGetSequenceLength((Sequence))
