/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"

typedef NFCSTATUS (*NciSequenceEntry)(void *pContext);
typedef NFCSTATUS (*NciSequenceExit)(void *pContext, NFCSTATUS wStatus);

typedef struct phNciNfc_SequenceParams
{
    NciSequenceEntry SequnceInitiate; /**< To initiate sequence, generally it sends command/data*/
    NciSequenceExit SequenceProcess;  /**< To Process sequence, generally it completes command/data*/
}phNciNfc_SequenceP_t;

NFCSTATUS phNciNfc_SeqHandler(void *pNciCtx, NFCSTATUS Status);

extern uint8_t phNciNfc_getSequenceLength(const phNciNfc_SequenceP_t * sequence);

extern uint8_t phNciNfc_RepeatSequenceSeq(void *pNciCtx,
                                          phNciNfc_SequenceP_t *pReqSeq,
                                          uint8_t bValue);

extern uint8_t phNciNfc_SkipSequenceSeq(void *pNciCtx,
                                        phNciNfc_SequenceP_t *pReqSeq,
                                        uint8_t bValue);

#define PHNCINFC_INIT_SEQUENCE(CONTEXT,SEQUENCE) \
    (CONTEXT)->pSeqHandler = (SEQUENCE);\
    (CONTEXT)->SeqNext = 0;\
    (CONTEXT)->SeqMax = phNciNfc_getSequenceLength((SEQUENCE))
