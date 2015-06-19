/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#pragma once

#include "phNfcTypes.h"

typedef NFCSTATUS (*SequenceEntry)(void *pContext, NFCSTATUS wStatus, void * pInfo);
typedef NFCSTATUS (*SequenceExit)(void *pContext, NFCSTATUS wStatus, void * pInfo);

typedef struct phLibNfc_SequenceParams
{
    SequenceEntry SequnceInitiate; /**< To initiate sequence, generally it sends command/data*/
    SequenceExit SequenceProcess;  /**< To Process sequence, generally it completes command/data*/
}phLibNfc_Sequence_t;

NFCSTATUS phLibNfc_SeqHandler(void *pCtx, NFCSTATUS Status, void* pInfo);
void phLibNfc_InternalSequence(void *pCtx, NFCSTATUS Status, void* pInfo);
uint8_t phLibNfc_getSequenceLength(const phLibNfc_Sequence_t * sequence);

void phLibNfc_AddSequence(  void *pCtx,\
                            phLibNfc_Sequence_t * pSequence,\
                            SequenceEntry pSeqEntry,\
                            SequenceExit pSeqExit);

extern uint8_t phLibNfc_SkipSequenceSeq(void *pLibCtx,
                              phLibNfc_Sequence_t *pReqSeq,
                              uint8_t bValue);

extern uint8_t phLibNfc_RepeatSequenceSeq(void *pLibCtx,
                              phLibNfc_Sequence_t *pReqSeq,
                              uint8_t bValue);

#define PHLIBNFC_INIT_SEQUENCE(CONTEXT,SEQUENCE) \
    (CONTEXT)->pSeqHandler = (SEQUENCE);\
    (CONTEXT)->SeqNext = 0;\
    (CONTEXT)->SeqMax = phLibNfc_getSequenceLength((SEQUENCE))
