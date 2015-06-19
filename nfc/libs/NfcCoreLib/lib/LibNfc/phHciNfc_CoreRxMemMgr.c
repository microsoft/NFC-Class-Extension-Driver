/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include <phLibNfc_Pch.h>

#include "phHciNfc_CoreRxMemMgr.tmh"

void phHciNfc_CoreDeleteList(phHciNfc_HciCoreContext_t *pCoreCtx)
{
    pphHciNfc_sCoreRecvBuff_List_t pList = NULL;
    pphHciNfc_sCoreRecvBuff_List_t pTemp = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (NULL == pCoreCtx)
    {
        PH_LOG_LIBNFC_CRIT_STR("NULL HCI Core context!");
    }
    else
    {
        /*Head is satically allcoted by context so dont delete it, else heap curruption*/
        pCoreCtx->tReceiveInfo.wPayloadSize = 0; /*after calling CoreDeleteList function it
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
            PH_LOG_LIBNFC_INFO_STR("List is empty");
            pCoreCtx->tReceiveInfo.wPayloadSize = 0;
            pCoreCtx->tReceiveInfo.wNumOfNodes = 1;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

pphHciNfc_sCoreRecvBuff_List_t phHciNfc_CoreGetNewNode(phHciNfc_HciCoreContext_t *pCoreCtx,
                                                       uint16_t wLenOfPkt)
{
    pphHciNfc_sCoreRecvBuff_List_t pList = NULL;
    pphHciNfc_sCoreRecvBuff_List_t pNewNode = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCoreCtx)
    {
        /*Check if head is free to use*/
        pList = &(pCoreCtx->tReceiveInfo.ListHead);
        if(pCoreCtx->tReceiveInfo.wPayloadSize == 0) /*There is no data into head so it can be used*/
        {
            pNewNode = pList;
            pNewNode->tMem.wLen = wLenOfPkt;
            /* First HCI packet always has header of two bytes i.e Pkt Header and Msg Header */
            pCoreCtx->tReceiveInfo.wPayloadSize = wLenOfPkt - PHHCINFC_HCP_HEADER_LEN;
        }else if(NULL != pList)
        {
            /* Traverse till last node in the list is reached */
            while(NULL != pList->pNext)
            {
                pList = pList->pNext;
            }
            /*Reached last node*/
            pList->pNext = (pphHciNfc_sCoreRecvBuff_List_t)
            phOsalNfc_GetMemory(sizeof(phHciNfc_sCoreRecvBuff_List_t));
            if(NULL != pList->pNext)
            {
                /* Increment the total number of node present in the list */
                pCoreCtx->tReceiveInfo.wNumOfNodes++;
                phOsalNfc_SetMemory(pList->pNext,0x00,sizeof(phHciNfc_sCoreRecvBuff_List_t));

                /* Initialize the new node contents */
                pList->pNext->tMem.wLen = PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH;
                pList->pNext->pNext = NULL;
                pNewNode = pList->pNext;
                pNewNode->tMem.wLen = pNewNode->tMem.wLen + wLenOfPkt;
                /* Subsequent HCI packet after First Pkt always has header of One byte i.e Fragment Pkt Header */
                pCoreCtx->tReceiveInfo.wPayloadSize = pCoreCtx->tReceiveInfo.wPayloadSize +
                                                      (wLenOfPkt - PHHCI_HCI_PACKET_FRAG_HEADER_LEN);
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Memory allcation failed while creating new node!");
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return pNewNode;
}

NFCSTATUS phHciNfc_HciCoreExtractData(phHciNfc_HciCoreContext_t *pCoreCtx,
                                      phHciNfc_ReceiveParams_t *pHciNfcRxdParams)
{
    pphHciNfc_sCoreRecvBuff_List_t pList = NULL;
    uint16_t wTotalNumNodes = 0;
    uint16_t wCount         = 0;
    uint16_t wCurrBuffSize  = 0;
    uint16_t wPayloadLen    = 0;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;

    /* pStartAddrOfPayloadBuf- Store the Start Addres Of Payload Data as it will be modified for copying packets */
    uint8_t  *pStartAddrOfPayloadBuf = pHciNfcRxdParams->pData;

    pList = &(pCoreCtx->tReceiveInfo.ListHead);
    wTotalNumNodes = pCoreCtx->tReceiveInfo.wNumOfNodes;

    /* Extract the HCI Packet Header Information which is store in the List Head */
    pHciNfcRxdParams->bPipeId     = (uint8_t) GET_BITS8(pList->tMem.aBuffer[PHHCINFC_HCP_PACKET_HEADER_OFFSET],
                                                        PHHCINFC_HCP_PIPEID_OFFSET,
                                                        PHHCINFC_HCP_PIPEID_LEN
                                                        );
    pHciNfcRxdParams->bMsgType    = (uint8_t) GET_BITS8(pList->tMem.aBuffer[PHHCINFC_HCP_MESSAGE_HEADER_OFFSET],
                                                        PHHCINFC_HCP_MSG_TYPE_OFFSET,
                                                        PHHCINFC_HCP_MSG_TYPE_LEN
                                                        );
    pHciNfcRxdParams->bIns        = (uint8_t) GET_BITS8(pList->tMem.aBuffer[PHHCINFC_HCP_MESSAGE_HEADER_OFFSET],
                                                        PHHCI_HCP_MSG_INSTRUCTION_OFFSET,
                                                        PHHCI_HCP_MSG_INSTRUCTION_LEN);

    wPayloadLen                   = pList->tMem.wLen - PHHCINFC_HCP_HEADER_LEN;
    phOsalNfc_MemCopy(pHciNfcRxdParams->pData,
                      &pList->tMem.aBuffer[PHHCINFC_HCP_HEADER_LEN],
                      wPayloadLen
                      );
    wCurrBuffSize += wPayloadLen;
    pHciNfcRxdParams->pData += wPayloadLen;
    pList = pList->pNext; /*  Skip First Node as it is already processed */

    /* Extract the HCI Packets payload Information which is store in consecutive Lists (Data Nodes) */
    for(wCount = 1; wCount < wTotalNumNodes; wCount++) /*  wCount = 1 to Skip First Node as it is already processed */
    {
        wPayloadLen = pList->tMem.wLen-PHHCI_HCI_PACKET_FRAG_HEADER_LEN;
        /* Make sure that each node contains payload field */
        if(wPayloadLen)
        {
            phOsalNfc_MemCopy(pHciNfcRxdParams->pData,
                              &pList->tMem.aBuffer[PHHCI_HCI_PACKET_FRAG_HEADER_LEN],
                              wPayloadLen);
            pHciNfcRxdParams->pData += wPayloadLen;
            pList = pList->pNext;
            wCurrBuffSize += wPayloadLen;
            /* To be on a safer side, this check has been added */
            if(NULL == pList)
            {
                break;
            }
        }
        else
        {
            /* Zero Payload Len - Error in reception*/
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_LIBNFC_CRIT_STR("HCI Core Receive- Zero Payload Len");
        }
    }
    pHciNfcRxdParams->wLen  = pCoreCtx->tReceiveInfo.wPayloadSize;
    pHciNfcRxdParams->pData = pStartAddrOfPayloadBuf; /* Point to the start of the Received Data*/
    if(wCurrBuffSize == pCoreCtx->tReceiveInfo.wPayloadSize)
    {
        wStatus = NFCSTATUS_SUCCESS;
    }else
    {
        wStatus = NFCSTATUS_FAILED;
        PH_LOG_LIBNFC_CRIT_STR("HCI Core Receive- Mismatch in Number of Payload Bytes Processed and Stored");
    }
    return wStatus;
}
