/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Sequence.tmh"

NFCSTATUS phNciNfc_SeqHandler(void *pNciCtx, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t )pNciCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext && NFCSTATUS_SUCCESS == wStatus)
    {
        if(pNciContext->SeqNext <= pNciContext->SeqMax && (NULL != pNciContext->pSeqHandler))
        {
            if(0 == pNciContext->SeqNext) /*Sequence is not yet started, so initiated*/
            {
                if(NULL != pNciContext->pSeqHandler[pNciContext->SeqNext].SequnceInitiate)
                {
                    wStatus = pNciContext->pSeqHandler[pNciContext->SeqNext].SequnceInitiate((void *)pNciContext);
                }
            }else /*Sequence is already started*/
            {
                if (NULL != pNciContext->pSeqHandler[pNciContext->SeqNext - 1].SequenceProcess)
                {
                    wStatus = pNciContext->pSeqHandler[pNciContext->SeqNext - 1].SequenceProcess((void *)pNciCtx, wStatus);
                }
                if (NFCSTATUS_SUCCESS == wStatus)
                {
                     if(NULL != pNciContext->pSeqHandler[pNciContext->SeqNext].SequnceInitiate)
                     {
                        wStatus = pNciContext->pSeqHandler[pNciContext->SeqNext].SequnceInitiate((void *)pNciContext);
                     }
                }
                else
                {
                    /* One of the response to a command has failed, exit the sequence */
                    pNciContext->SeqNext = pNciContext->SeqMax;
                }
            }
        }
    }
    else
    {
        if(NULL != pNciContext)
        {
            /* Complete the sequence  */
            pNciContext->SeqNext = pNciContext->SeqMax;
        }
    }
    if((NFCSTATUS_SUCCESS != wStatus) && (NFCSTATUS_PENDING != wStatus))
    {
        if(NULL != pNciContext)
        {
            if(0 != pNciContext->SeqNext) /*Sequence is in async mode*/
            {
                /* Complete the sequence  */
                pNciContext->SeqNext = pNciContext->SeqMax;
            }
        }
    }
    /*Complete the sequence if returned error code is not */
    if(NULL != pNciContext)
    {
        if((pNciContext->SeqNext  == pNciContext->SeqMax) && (NULL != pNciContext->pSeqHandler))
        {
            PH_LOG_NCI_CRIT_EXPECT(pNciContext->SeqNext != 0);
            if (NULL != pNciContext->pSeqHandler[pNciContext->SeqNext].SequenceProcess)
            {
                pNciContext->pSeqHandler[pNciContext->SeqNext].SequenceProcess((void *)pNciCtx, wStatus);
            }
        }else
        {
            /*Update Next Sequence*/
            pNciContext->SeqNext++;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

uint8_t phNciNfc_getSequenceLength(const phNciNfc_SequenceP_t * sequence)
{
    uint8_t len = 0;
    if ( sequence != NULL )
    {
        while ( NULL != sequence->SequnceInitiate)
        {
            len++;
            sequence++;
        }
    }
    return len;
}

uint8_t phNciNfc_RepeatSequenceSeq(void *pNciCtx,
                                   phNciNfc_SequenceP_t *pReqSeq,
                                   uint8_t bValue)
{
    pphNciNfc_Context_t pNciContext = pNciCtx;
    uint8_t bReturn = 0;
    if((NULL != pNciCtx) && (NULL != pReqSeq) && (0 != bValue))
    {
        if(pReqSeq == pNciContext->pSeqHandler)
        {
            if((pNciContext->SeqNext ) >= bValue)
            {
                pNciContext->SeqNext -= bValue;
                bReturn = 1;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Value to decrement is invalid");
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Sequence mismatch, cannot decrement the current sequence");
        }
    }
    return bReturn;
}

uint8_t phNciNfc_SkipSequenceSeq(void *pNciCtx,
                                 phNciNfc_SequenceP_t *pReqSeq,
                                 uint8_t bValue)
{
    pphNciNfc_Context_t pNciContext = pNciCtx;
    uint8_t bReturn = 0;
    if((NULL != pNciCtx) && (NULL != pReqSeq) && (0 != bValue))
    {
        if(pReqSeq == pNciContext->pSeqHandler)
        {
            if((pNciContext->SeqNext + bValue) <= pNciContext->SeqMax)
            {
                pNciContext->SeqNext += bValue;
                bReturn = 1;
            }
            else
            {
                PH_LOG_NCI_WARN_STR("Value to increment is invalid");
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Sequence mismatch, can not increment the current sequence");
        }
    }
    return bReturn;
}
