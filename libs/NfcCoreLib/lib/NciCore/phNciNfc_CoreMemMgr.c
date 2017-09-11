/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_CoreMemMgr.h"

#include "phNciNfc_CoreMemMgr.tmh"

static uint16_t phNciNfc_CoreExtractData(pphNciNfc_sCoreReceiveInfo_t pRecvInfo,
                                     uint8_t *pBuff,
                                     uint16_t wBuffLen);

NFCSTATUS phNciNfc_CoreGetDataLength(pphNciNfc_CoreContext_t pCoreCtx, uint16_t *pDataLen, uint16_t *wNumOfNode)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint16_t wHeaderSize = 0;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pDataLen))
    {
        if(NULL != wNumOfNode)
        {
            *wNumOfNode = pCoreCtx->tReceiveInfo.wNumOfNodes;
        }
        wHeaderSize = (uint16_t)(pCoreCtx->tReceiveInfo.wNumOfNodes * PHNCINFC_CORE_PKT_HEADER_LEN);
        if(pCoreCtx->tReceiveInfo.wPayloadSize >= wHeaderSize)
        {
            *pDataLen = pCoreCtx->tReceiveInfo.wPayloadSize - wHeaderSize;
            wStatus = NFCSTATUS_SUCCESS;
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Total Payload size of less than header size of all nodes");
            wStatus = NFCSTATUS_FAILED;
            *pDataLen = 0;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreGetData(pphNciNfc_CoreContext_t pCoreCtx, uint8_t *pBuff, uint16_t wLen)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_sCoreRecvBuff_List_t pList = NULL;
    uint16_t wExtractDataLen = 0;
    uint16_t wActualPayloadLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pBuff))
    {
        pList = &(pCoreCtx->tReceiveInfo.ListHead);
        (void)phNciNfc_CoreGetDataLength(pCoreCtx,&wActualPayloadLen, NULL);

        if((NULL != pList) &&(0 != wActualPayloadLen)
            && (0 != pCoreCtx->tReceiveInfo.wNumOfNodes))
        {
            /* Check if the user buffer is sufficient enough to store the entire payload */
            if(wLen >= wActualPayloadLen)
            {
                wStatus = NFCSTATUS_SUCCESS;
                /* User buffer can accomodate entire received payload */
                wExtractDataLen = wActualPayloadLen;
            }
            else
            {
                wStatus = NFCSTATUS_MORE_INFORMATION;
                /* User buffer can not accomodate entire received payload.
                   Copy only the payload that can be accomodated in user buffer */
                wExtractDataLen = wLen;
            }
            /* Extract the data from the linked list */
            (void )phNciNfc_CoreExtractData(&pCoreCtx->tReceiveInfo,pBuff,wExtractDataLen);
        }
        else
        {
            /* Either list is empty or payload length in list is '0' */
            wStatus = NFCSTATUS_FAILED;
        }
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void phNciNfc_CoreUpdatePacketLen(pphNciNfc_CoreContext_t pCoreCtx,uint16_t wLength)
{
    pphNciNfc_sCoreRecvBuff_List_t pList = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pCoreCtx)
    {
        PH_LOG_NCI_CRIT_STR("NULL Nci Core context!");
    }
    else if (pCoreCtx != phNciNfc_GetCoreContext())
    {
        PH_LOG_NCI_CRIT_STR("Invalid Nci Core context!");
    }
    else
    {
        pList = &(pCoreCtx->tReceiveInfo.ListHead);
        if(NULL == pList)
        {
            PH_LOG_NCI_WARN_STR("List is empty, can not update length");
        }
        else
        {
            /* Traverse to the last node */
            while(NULL != pList->pNext)
            {
                pList = pList->pNext;
            }
            pList->tMem.wLen = wLength;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

void phNciNfc_CoreDeleteList(pphNciNfc_CoreContext_t pCoreCtx)
{
    pphNciNfc_sCoreRecvBuff_List_t pList = NULL;
    pphNciNfc_sCoreRecvBuff_List_t pTemp = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pCoreCtx) {
        PH_LOG_NCI_CRIT_STR("NULL Nci Core context!");
    }
    else if (pCoreCtx != phNciNfc_GetCoreContext())
    {
        PH_LOG_NCI_CRIT_STR("Invalid Nci Core context!");
    }
    else
    {
        /*Head is satically allcoted by context so dont delete it, else heap curruption*/
        pCoreCtx->tReceiveInfo.wPayloadSize = 0; /*after calling deletelist function it
                                                 is assumed that all the data is copied from the list*/
        pList = pCoreCtx->tReceiveInfo.ListHead.pNext;
        if(NULL != pList)
        {
            while(NULL != pList->pNext)
            {
                pTemp = pList->pNext;
                phOsalNfc_FreeMemory(pList);
                pList = pTemp;
            }
            /*Reached last node, free it*/
            phOsalNfc_FreeMemory(pList);
            pCoreCtx->tReceiveInfo.ListHead.pNext = NULL;
            /* Node head shall not be deleted */
            pCoreCtx->tReceiveInfo.wNumOfNodes = 1;
            pCoreCtx->tReceiveInfo.wPayloadSize = 0;
        }
        else
        {
            PH_LOG_NCI_INFO_STR("List is empty");
            pCoreCtx->tReceiveInfo.wPayloadSize = 0; /*after calling deletelist function it
                                                 is assumed that all the data is copied from the list*/
            pCoreCtx->tReceiveInfo.wNumOfNodes = 1;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

void phNciNfc_CoreRemoveLastChainedNode(pphNciNfc_CoreContext_t pCoreCtx)
{
    pphNciNfc_sCoreRecvBuff_List_t pCurrent = NULL;
    pphNciNfc_sCoreRecvBuff_List_t pPrevious = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if (NULL == pCoreCtx) 
    {
        PH_LOG_NCI_CRIT_STR("NULL Nci Core context!");
    }
    else if (pCoreCtx != phNciNfc_GetCoreContext())
    {
        PH_LOG_NCI_CRIT_STR("Invalid Nci Core context!");
    }
    else
    {
        /* If only the head exists, it will automatically be reused, no need to remove anything */
        if (pCoreCtx->tReceiveInfo.wNumOfNodes > 1 && pCoreCtx->tReceiveInfo.wPayloadSize != 0)
        {
            pPrevious = &(pCoreCtx->tReceiveInfo.ListHead);
            pCurrent = pPrevious->pNext;

            while (NULL != pCurrent->pNext)
            {
                pPrevious = pCurrent;
                pCurrent = pCurrent->pNext;
            }

            phOsalNfc_FreeMemory(pCurrent);
            pCurrent = NULL;

            pPrevious->pNext = NULL;
            pCoreCtx->tReceiveInfo.wNumOfNodes--;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
}

pphNciNfc_sCoreRecvBuff_List_t phNciNfc_CoreGetNewNode(pphNciNfc_CoreContext_t pCoreCtx)
{
    pphNciNfc_sCoreRecvBuff_List_t pList = NULL;
    pphNciNfc_sCoreRecvBuff_List_t pNewNode = NULL;
    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (pCoreCtx == phNciNfc_GetCoreContext()))
    {
        /*Check if head is free to use*/
        pList = &(pCoreCtx->tReceiveInfo.ListHead);
        if(pCoreCtx->tReceiveInfo.wPayloadSize == 0) /*There is no data into head so it can be used*/
        {
            pNewNode = pList;
            pNewNode->tMem.wLen = PHNCINFC_CORE_MAX_RECV_BUFF_SIZE;
        }else if(NULL != pList)
        {
            /* Traverse till last node in the list is reached */
            while(NULL != pList->pNext)
            {
                pList = pList->pNext;
            }
            /*Reached last node*/
            pList->pNext = (pphNciNfc_sCoreRecvBuff_List_t)
                phOsalNfc_GetMemory(sizeof(phNciNfc_sCoreRecvBuff_List_t));
            if(NULL != pList->pNext)
            {
                /* Increment the total number of node present in the list */
                pCoreCtx->tReceiveInfo.wNumOfNodes++;
                phOsalNfc_SetMemory(pList->pNext,0x00,sizeof(phNciNfc_sCoreRecvBuff_List_t));
                /* Initialize the new node contents */
                pList->pNext->tMem.wLen = PHNCINFC_CORE_MAX_BUFF_SIZE;
                pList->pNext->pNext = NULL;
                pNewNode = pList->pNext;
            }
            else
            {
                PH_LOG_NCI_INFO_STR("Memory allcation failed while creating new node!");
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return pNewNode;
}

static uint16_t phNciNfc_CoreExtractData(pphNciNfc_sCoreReceiveInfo_t pRecvInfo,
                                     uint8_t *pBuff,
                                     uint16_t wBuffLen)
{
    pphNciNfc_sCoreRecvBuff_List_t pList = NULL;
    uint16_t wTotalNumNodes = 0,
            bCount = 0;
    uint16_t wCopyLen = 0,
             wCurrBuffSize = 0,
             wRemSize = 0,
             wPayloadLen = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    pList = &(pRecvInfo->ListHead);
    wTotalNumNodes = pRecvInfo->wNumOfNodes;

    for(bCount = 0; bCount < wTotalNumNodes; bCount++)
    {
        wPayloadLen = (pList->tMem.wLen - PHNCINFC_CORE_PKT_HEADER_LEN);
        /* Make sure that each node contains payload field */
        if(wPayloadLen)
        {
            /* Make sure there is enough memory in the user buffer */
            if(wCurrBuffSize < wBuffLen)
            {
                wRemSize = wBuffLen - wCurrBuffSize;
                if(wRemSize >= wPayloadLen)
                {
                    wCopyLen = wPayloadLen;
                }
                else
                {
                    wCopyLen = wRemSize;
                }
                phOsalNfc_MemCopy(pBuff,&pList->tMem.aBuffer[PHNCINFC_CORE_PKT_HEADER_LEN],
                                wCopyLen);
                pBuff += wCopyLen;
                pList = pList->pNext;
                wCurrBuffSize += wCopyLen;
                /* To be on a safer side, this check has been added */
                if(NULL == pList)
                {
                    break;
                }
            }
            else
            {
                PH_LOG_NCI_INFO_STR("Reached end of user buffer");
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wCurrBuffSize;
}
