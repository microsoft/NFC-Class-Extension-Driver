/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#include "phLibNfc_Pch.h"

#include "phLibNfc_Sequence.tmh"

NFCSTATUS phLibNfc_SeqHandler(void *pCtx, NFCSTATUS Status, void* pInfo)
{
    NFCSTATUS wStatus = Status;
    pphLibNfc_Context_t pContext = (pphLibNfc_Context_t )pCtx;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pContext) && (NFCSTATUS_SUCCESS == wStatus))
    {
        if((pContext->SeqNext <= pContext->SeqMax) && (pContext->SeqMax > 0))
        {
            if(0 == pContext->SeqNext) /*Sequence is not yet started, so initiated*/
            {
                if(NULL != pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate)
                {
                    wStatus = pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate((void *)pContext, wStatus, pInfo);

                    /* Skip current sequence*/
                    if((NFCSTATUS_SUCCESS == wStatus) && (pContext->SeqMax > pContext->SeqNext))
                    {
                        pContext->SeqNext++;
                        if(NULL != pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate)
                        {
                            wStatus = pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate((void *)pContext, wStatus, pInfo);
                        }

                        /* Skip any further sequence and goto last */
                        if(NFCSTATUS_SUCCESS == wStatus)
                        {
                            pContext->SeqNext = pContext->SeqMax;
                        }
                    }
                }
            }
            else /*Sequence is already started*/
            {
                if (NULL != pContext->pSeqHandler[pContext->SeqNext - 1].SequenceProcess)
                {
                    wStatus = pContext->pSeqHandler[pContext->SeqNext - 1].SequenceProcess((void *)pContext, wStatus, pInfo);
                }

                if (NFCSTATUS_SUCCESS == wStatus)
                {
                    if(NULL != pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate)
                    {
                        wStatus = pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate((void *)pContext, wStatus, pInfo);

                        /*Skip current sequence*/
                        if((NFCSTATUS_SUCCESS == wStatus) && (pContext->SeqMax > pContext->SeqNext))
                        {
                            pContext->SeqNext++;
                            if(NULL != pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate)
                            {
                                wStatus = pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate((void *)pContext, wStatus, pInfo);
                            }

                            while((NFCSTATUS_SUCCESS == wStatus) && (pContext->SeqMax > pContext->SeqNext))
                            {
                                pContext->SeqNext++;
                                if(NULL != pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate)
                                {
                                    wStatus = pContext->pSeqHandler[pContext->SeqNext].SequnceInitiate((void *)pContext, wStatus, pInfo);
                                }
                            }
                        }
                    }
                }
                else
                {
                    /* One of the response to a command has failed, exit the sequence */
                    pContext->SeqNext = pContext->SeqMax;
                    if( (NFCSTATUS_BOARD_COMMUNICATION_ERROR == wStatus) )
                    {
                        (void)phTmlNfc_IoCtl(pContext->sHwReference.pDriverHandle, phTmlNfc_e_ResetDevice);
                    }
                }
            }
        }
    }
    /* Reset Controller and Re-Initialize if board communication error is received */
    else if((NULL != pContext) && (NFCSTATUS_BOARD_COMMUNICATION_ERROR == wStatus))
    {
        (void)phTmlNfc_IoCtl(pContext->sHwReference.pDriverHandle, phTmlNfc_e_ResetDevice);
    }
    else
    {
        if(NULL != pContext)
        {
            /* Complete the sequence  */
            pContext->SeqNext = pContext->SeqMax;
        }
    }
    if((NFCSTATUS_SUCCESS != wStatus) && (NFCSTATUS_PENDING != wStatus))
    {
        if(NULL != pContext)
        {
            if(0 != pContext->SeqNext) /*Sequence is in async mode*/
            {
                pContext->SeqNext = pContext->SeqMax;
                /*Complete sequence must be able to handle status code*/
            }
        }
    }
    /*Complete the sequence if returned error code is not */
    if(NULL != pContext)
    {
        if(pContext->SeqNext  == pContext->SeqMax)
        {
            if (NULL != pContext->pSeqHandler &&
                NULL != pContext->pSeqHandler[pContext->SeqNext].SequenceProcess)
            {
                pContext->pSeqHandler[pContext->SeqNext].SequenceProcess((void *)pContext, wStatus, pInfo);
            }
        }
        else
        {
            pContext->SeqNext++;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

void phLibNfc_InternalSequence(void *pCtx, NFCSTATUS Status, void* pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    (void)phLibNfc_SeqHandler(pCtx,Status,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

uint8_t phLibNfc_getSequenceLength(const phLibNfc_Sequence_t * sequence)
{
    uint8_t len = 0;
    if ( sequence != NULL )
    {
        while ( NULL != sequence->SequnceInitiate )
        {
            len++;
            sequence++;
        }
    }
    return len;
}

void phLibNfc_AddSequence(  void *pCtx,\
                            phLibNfc_Sequence_t * pSequence,\
                            SequenceEntry pSeqEntry,\
                            SequenceExit pSeqExit)
{
    pphLibNfc_Context_t pContext = (pphLibNfc_Context_t )pCtx;
    uint8_t bSkipAddSequence = FALSE;
    if( (NULL != pContext)&&
        (NULL != pSequence)&&
        (NULL != pSeqEntry)&&
        (NULL != pSeqExit) )
    {
        /* Traverse till the end of sequence */
        while ( NULL != pSequence->SequnceInitiate )
        {
            pSequence++;
            if( (pSequence->SequnceInitiate == pSeqEntry) &&\
                (pSequence->SequenceProcess == pSeqExit) )
            {
                bSkipAddSequence = TRUE;
                break;
            }
        }

        if( FALSE == bSkipAddSequence)
        {
            /* Shift the last sequence by one */
            pSequence[1].SequenceProcess = pSequence->SequenceProcess;
            pSequence->SequnceInitiate = pSeqEntry;
            pSequence->SequenceProcess = pSeqExit;
            pContext->SeqMax++;
        }
    }
}

uint8_t phLibNfc_SkipSequenceSeq(void *pLibCtx,
                                 phLibNfc_Sequence_t *pReqSeq,
                                 uint8_t bValue)
{
    pphLibNfc_Context_t pLibContext = pLibCtx;
    uint8_t bReturn = 0;
    if((NULL != pLibCtx) && (NULL != pReqSeq) && (0 != bValue))
    {
        /* Check if the requested sequence is matching with the sequence in progress */
        if(pReqSeq == pLibContext->pSeqHandler)
        {
            if((pLibContext->SeqNext + bValue) <= pLibContext->SeqMax)
            {
                pLibContext->SeqNext += bValue;
                bReturn = 1;
            }
            else
            {
                PH_LOG_LIBNFC_WARN_STR("Value to increment is invalid");
            }
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("Sequence mismatch, can not increment the current sequence");
        }
    }
    return bReturn;
}

uint8_t phLibNfc_RepeatSequenceSeq(void *pLibCtx,
                                   phLibNfc_Sequence_t *pReqSeq,
                                   uint8_t bValue)
{
    pphLibNfc_Context_t pLibContext = pLibCtx;
    uint8_t bReturn = 0;
    if((NULL != pLibCtx) && (NULL != pReqSeq) && (0 != bValue))
    {
        /* Check if the requested sequence is matching with the sequence in progress */
        if(pReqSeq == pLibContext->pSeqHandler)
        {
            if((pLibContext->SeqNext ) >= bValue)
            {
                pLibContext->SeqNext -= bValue;
                bReturn = 1;
            }
            else
            {
                PH_LOG_LIBNFC_WARN_STR("Value to decrement is invalid");
            }
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("Sequence mismatch, can not decrement the current sequence");
        }
    }
    return bReturn;
}
