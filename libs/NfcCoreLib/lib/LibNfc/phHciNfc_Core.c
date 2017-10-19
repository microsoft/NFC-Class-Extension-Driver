/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phHciNfc_Core.tmh"


static void
phHciNfc_HciCoreLowerLayerSendCb(void *pContext, NFCSTATUS wStatus,void *pInfo);

static NFCSTATUS
phHciNfc_HciCoreBuildHcipacket(_In_ phHciNfc_SendParams_t   *pSendMsgParams,
                               _In_ uint8_t                 bChaining,
                               _In_range_(<=, PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE)
                               uint32_t                     dwLenOfHcpPktData,
                               _Out_ uint32_t               *pdwTotalLenOfBuiltPkt,
                               _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPkt)
                               uint8_t                      *pBuiltHciPkt
                              );

static NFCSTATUS
phHciNfc_HciCoreBuildHcpFragment(_In_ uint32_t  dwCurrIndexOfPayloadData,
                                 _In_range_(<=, PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH)
                                 uint32_t       dwLenOfHcpFragData,
                                 _In_ uint8_t   bChaining,
                                 _In_ phHciNfc_SendParams_t *pSendMsgParams,
                                 _Out_range_(<=, PHHCI_HCP_MAX_PACKET_SIZE)
                                 uint32_t       *pdwTotalLenOfBuiltPkt,
                                 _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPkt)
                                 uint8_t        pFragmentData[]
                                );

static NFCSTATUS
phHciNfc_HciCoreCheckBuildHciFragments(_Inout_ phHciNfc_HciContext_t *pHciContext,
                                       _In_ NFCSTATUS   wStatus,
                                       _Out_range_(<=, PHHCI_HCP_MAX_PACKET_SIZE)
                                       uint32_t         *pdwTotalLenOfBuiltPkt,
                                       _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPkt)
                                       uint8_t          aHcpFragmentData[]);

static void phHciNfc_CoreInitialiseContext(void *pContext);

NFCSTATUS phHciNfc_CoreInit(void *pContext)
{
    NFCSTATUS wStatus;
    phHciNfc_HciRegData_t  tHciRegData;

    if(pContext != NULL)
    {
        phHciNfc_HciContext_t      *pHciContext     = (phHciNfc_HciContext_t *) pContext;
        phHciNfc_CoreInitialiseContext(pContext);
        wStatus = phNciNfc_RegisterHciSeEvent(pHciContext->pNciContext,
                                              pHciContext->pSeHandle,
                                              &phHciNfc_CoreRecvCB,
                                              pHciContext);

        if(wStatus == NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_INFO_STR("Receive Data Registeration Sucessfull with NCI");
            /* Register for Events when are received over ADM pipe */
            tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeCmd;
            tHciRegData.bPipeId = (uint8_t) phHciNfc_e_HciAdminPipeId;
            wStatus = phHciNfc_RegisterCmdRspEvt(pHciContext,
                                    &tHciRegData,
                                    &phHciNfc_ReceiveAdminNotifyCmd,
                                    pHciContext);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Adm pipe command registered!");
                /* Register for Events when are received over ADM pipe */
                tHciRegData.eMsgType = phHciNfc_e_HciMsgTypeEvent;
                tHciRegData.bPipeId = (uint8_t) phHciNfc_e_HciAdminPipeId;
                wStatus = phHciNfc_RegisterCmdRspEvt(pHciContext,
                                        &tHciRegData,
                                        &phHciNfc_ReceiveAdminNotifyEvt,
                                        pHciContext);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    PH_LOG_LIBNFC_INFO_STR("Adm pipe events registered!");
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Failed to register Adm pipe events!");
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to register Adm pipe commands!");
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Receive Data Registeration Failed with NCI");
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
        return wStatus;
}
static void phHciNfc_CoreInitialiseContext(void *pContext)
{
    phHciNfc_HciContext_t      *pHciContext     = (phHciNfc_HciContext_t *) pContext;
    phHciNfc_HciCoreContext_t  *pHciCoreContext = &(pHciContext->pHciCoreContext);

    phOsalNfc_SetMemory(&pHciCoreContext->tHciCtxSendMsgParams,0,sizeof(phHciNfc_SendParams_t));
    pHciCoreContext->phHciNfcCoreUpperLayerSendCb           = NULL;
    pHciCoreContext->pUpperLayerContext                     = NULL;

    pHciCoreContext->tHciCtxRxMsgParams.bIns                = 0;
    pHciCoreContext->tHciCtxRxMsgParams.bMsgType            = 0;
    pHciCoreContext->tHciCtxRxMsgParams.bPipeId             = 0;
    pHciCoreContext->tHciCtxRxMsgParams.wLen                = 0;
    pHciCoreContext->tHciCtxRxMsgParams.pData               = NULL;

    pHciCoreContext->tHciFragInfo.dwTxdHcpPayloadNoOfDataBytes = 0;
    phOsalNfc_SetMemory(pHciCoreContext->tReceiveInfo.ListHead.tMem.aBuffer,
                        0,
                        sizeof(pHciCoreContext->tReceiveInfo.ListHead.tMem.aBuffer)
                        );
    pHciCoreContext->tReceiveInfo.ListHead.tMem.wLen           = 0;

    pHciCoreContext->tReceiveInfo.ListHead.pNext = NULL;
    /* Node head shall not be deleted */
    pHciCoreContext->tReceiveInfo.wNumOfNodes = 1;
    pHciCoreContext->tReceiveInfo.wPayloadSize = 0;

    pHciContext->tHciSeTxRxTimerInfo.dwRspTimerId    = 0;
    pHciContext->tHciSeTxRxTimerInfo.dwTimeOut       = PHHCINFC_DEFAULT_HCI_TX_RX_TIME_OUT;
    pHciContext->tHciSeTxRxTimerInfo.dwTimerStatus   = 0;

    phOsalNfc_SetMemory(pHciContext->aGetHciSessionId, 0xFF, sizeof(pHciContext->aGetHciSessionId));
    phOsalNfc_SetMemory(pHciContext->aSEPipeList, 0x00, sizeof(phHciNfc_PipeGateInfo_t) * 3);
    phOsalNfc_SetMemory(pHciContext->aUICCPipeList, 0x00, sizeof(phHciNfc_PipeGateInfo_t));
}

NFCSTATUS
phHciNfc_CoreRecvCB(void                           *pContext,
                    void                           *pInfo,
                    NFCSTATUS                      wStatus
                   )
{
    uint8_t bChainBit = 0;
    pphHciNfc_sCoreRecvBuff_List_t pNode = NULL;
    phHciNfc_ReceiveParams_t tHciNfcRxdParams;
    phNciNfc_TransactInfo_t        *pHciCoreReceiveInfo;
    NFCSTATUS wStat;

    if(pContext != NULL)
    {
        if(pInfo != NULL)
        {
            phHciNfc_HciContext_t      *pHciContext     = (phHciNfc_HciContext_t *) pContext;
            phHciNfc_HciCoreContext_t  *pHciCoreContext = &(pHciContext->pHciCoreContext);
            pHciCoreReceiveInfo                         = (phNciNfc_TransactInfo_t*)pInfo;

            if(wStatus == NFCSTATUS_SUCCESS)
            {
                bChainBit = (uint8_t) GET_BITS8(*(pHciCoreReceiveInfo->pbuffer),
                                                PHHCINFC_HCP_CHAINBIT_OFFSET,
                                                PHHCINFC_HCP_CHAINBIT_LEN);

                /* Store/Buffer HCP Header along with Data irrespective of whether chained or Unchained */
                if(pHciCoreReceiveInfo->wLength <= PHHCI_HCP_MAX_RECEIVE_PACKET_SIZE)
                {
                    /* Create the Data Node and copy the Data */
                    pNode = phHciNfc_CoreGetNewNode(pHciCoreContext,pHciCoreReceiveInfo->wLength);
                    if(pNode != NULL)
                    {
                        phOsalNfc_MemCopy((uint8_t*)&(pNode->tMem.aBuffer[0]),
                                          (uint8_t*)(pHciCoreReceiveInfo->pbuffer),
                                          pHciCoreReceiveInfo->wLength);
                        pNode->tMem.wLen      = pHciCoreReceiveInfo->wLength;
                    }else
                    {
                        PH_LOG_LIBNFC_CRIT_STR(" HCI Core Receive- Failed To Allocate Memory for Node");
                    }
                }else
                {
                    /* HCP Packet should not be bigger than PHHCI_HCP_MAX_PACKET_SIZE */
                }
                /* Check Chaining Bit to asceratain whether complete Message is received or Not*/
                if(bChainBit == HCP_CHAINBIT_UN_CHAINED)
                {
                    /* HCI message is complete extract the Data from linked list*/
                    tHciNfcRxdParams.pData = phOsalNfc_GetMemory(pHciCoreContext->tReceiveInfo.wPayloadSize);
                    wStat = phHciNfc_HciCoreExtractData(pHciCoreContext,&tHciNfcRxdParams);
                    if(wStat == NFCSTATUS_SUCCESS)
                    {
                        /*Once the HCI Packets are Extracted ,HCI packet info is captured so delete the linked list */
                        phHciNfc_CoreDeleteList(pHciCoreContext);
                        /* Send the Received Data To upper layer*/
                        /* TO DO Registration mechanism to be implemented*/
                        phHciNfc_ReceiveHandler(pHciContext,
                                                &tHciNfcRxdParams,
                                                wStatus
                                                );
                        phOsalNfc_FreeMemory(tHciNfcRxdParams.pData);
                    }else
                    {
                            PH_LOG_LIBNFC_CRIT_STR(" HCI Core - HCI Packet Extraction Failed");
                    }
                }
                else
                {
                    /* Chained HCI packets which are already buffered
                    ** No Action here waits for complete HCI Msg to be Rxd
                    */
                }
            }else
            {
                /* Failed Status from lower layer- Call the upper layer call back with the returned status */
                /* Send the Received Data To upper layer*/
                phHciNfc_ReceiveHandler(pHciContext->pHciCoreContext.pUpperLayerContext,
                                        NULL,
                                        wStatus
                                        );
            }
        }
        else
        {
            /* Do Nothing if no information is received from lower layer */
            PH_LOG_LIBNFC_CRIT_STR(" Invalid Receive Info Pointer received from lower layer ");
        }
    }else
    {
        PH_LOG_LIBNFC_CRIT_STR(" Invalid HCI Context received from Lower Layer ");
    }
    return wStatus;

}

static void
phHciNfc_PrintHciCommand(_In_ phHciNfc_HciContext_t* pHciContext, const phHciNfc_SendParams_t* params)
{
    if (WPP_FLAG_LEVEL_ENABLED(TF_HCI, LEVEL_INFO))
    {
        PH_LOG_HCI_INFO_STR("HCI Command Details:");
        PH_LOG_HCI_INFO_STR("====================");

        PH_LOG_HCI_INFO_STR("Pipe Id: %u", params->bPipeId);
        PH_LOG_HCI_INFO_STR("INS: %u", params->bIns);
        PH_LOG_HCI_INFO_STR("Message type: %u", params->bMsgType);
        PH_LOG_HCI_INFO_STR("Data length: %u", params->dwLen);

        if (pHciContext->bLogDataMessages)
        {
            #define PRINT_HCI_COMMAND_MAX_COUNT 25
            uint32_t len = min(params->dwLen, PRINT_HCI_COMMAND_MAX_COUNT);

            CHAR data[PRINT_HCI_COMMAND_MAX_COUNT*3 + 1] = { 0 };
            for (uint32_t i = 0; i != len; ++i)
            {
                const size_t arraySize = 4; // 3 chars per byte + null terminator
                sprintf_s(data + i * 3, arraySize, "%02X ", (int)params->pData[i]);
            }

            PH_LOG_HCI_INFO_STR("Data: %s", data);
        }

        PH_LOG_HCI_INFO_STR("====================");
    }
}

NFCSTATUS
phHciNfc_CoreSend(void                            *pHciContextHandle,
                  phHciNfc_SendParams_t           *pSendMsgParams,
                  pphHciNfc_UpperLayerSendCb_t    phHciNfcUpperLayerSendCb,
                  void                            *pContext
                 )
{
    NFCSTATUS wStatus;
    phHciNfc_HciContext_t *pHciContext = (phHciNfc_HciContext_t *) pHciContextHandle;
    uint8_t               *pBuiltHciPkt;
    uint32_t              dwTotalLenOfBuiltPkt=0;
    phNfc_sData_t         tSendData;

    if( (pHciContext == NULL) || (pSendMsgParams == NULL) )
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid parameter");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }else
    {
        pBuiltHciPkt = phOsalNfc_GetMemory(PHHCI_HCP_MAX_PACKET_SIZE);

        if( pBuiltHciPkt == NULL )
        {
            /* Failed to allocate memory for HCI packet */
            wStatus = NFCSTATUS_FAILED;
        }else
        {
            phHciNfc_PrintHciCommand(pHciContext, pSendMsgParams);

            /* Store Send Call back, Uppler Layer Context and received msg params in Hci context */
            phOsalNfc_MemCopy(&(pHciContext->pHciCoreContext.tHciCtxSendMsgParams),pSendMsgParams,sizeof(phHciNfc_SendParams_t));

            /* Check whether Fragmenation is required
            *  If fragmentation is required fragmentation is performed in phHciNfcLowerLayerSendCb callback
            *  which is invoked by NCI after transmission of first packet here
            */
            if( pSendMsgParams->dwLen <= ( PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE ) )
            {
                wStatus = phHciNfc_HciCoreBuildHcipacket(pSendMsgParams,
                                                         HCP_CHAINBIT_UN_CHAINED,
                                                         pSendMsgParams->dwLen,
                                                         &dwTotalLenOfBuiltPkt,
                                                         pBuiltHciPkt);
                pHciContext->pHciCoreContext.tHciFragInfo.dwTxdHcpPayloadNoOfDataBytes = pSendMsgParams->dwLen;

            }else
            {
                /* The HCI packets needs to be fragmented, send the First HCI packet and start
                *  HCI Fragmenation in the Send Call Back(after sucessfull transmission by lower layer)
                */
                wStatus = phHciNfc_HciCoreBuildHcipacket(pSendMsgParams,
                                                         HCP_CHAINBIT_CHAINED,
                                                         PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE,
                                                         &dwTotalLenOfBuiltPkt,
                                                         pBuiltHciPkt);
                pHciContext->pHciCoreContext.tHciFragInfo.dwTxdHcpPayloadNoOfDataBytes =
                                                                          PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE;
            }
            /* Send the HCI packet to lower layer */
            if( wStatus == NFCSTATUS_SUCCESS )
            {
                tSendData.length = dwTotalLenOfBuiltPkt;
                tSendData.buffer = pBuiltHciPkt;
                wStatus = phNciNfc_SeSendData(pHciContext->pNciContext,
                                  pHciContext->pSeHandle,
                                  (pphNciNfc_IfNotificationCb_t)&phHciNfc_HciCoreLowerLayerSendCb,
                                  pHciContext,
                                  &tSendData);
                if(wStatus == NFCSTATUS_PENDING)
                {
                    /* Store Send Call back, Uppler Layer Context in Hci context */
                    pHciContext->pHciCoreContext.phHciNfcCoreUpperLayerSendCb = phHciNfcUpperLayerSendCb;
                    pHciContext->pHciCoreContext.pUpperLayerContext           = pContext;

                    PH_LOG_LIBNFC_INFO_STR(" HCP Packet Sent to NCI ");
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR(" Failed to send HCP to Lower ");
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR(" HCI packet Formation Failed ");
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }

            phOsalNfc_FreeMemory(pBuiltHciPkt);
        }
    }
    return wStatus;
}

static void phHciNfc_HciCoreLowerLayerSendCb(void *pContext, NFCSTATUS wStatus,void *pInfo)
{
    phNfc_sData_t tSendData;
    uint8_t  aHcpFragmentData[PHHCI_HCP_MAX_PACKET_SIZE];
    uint32_t dwTotalLenOfBuiltPkt = 0;
    PHNFC_UNUSED_VARIABLE(pInfo); /* No Data Expected from lower Layer */
    if(pContext != NULL)
    {
        phHciNfc_HciContext_t *pHciContext = (phHciNfc_HciContext_t *) pContext;
        if(wStatus == NFCSTATUS_SUCCESS)
        {
            wStatus = phHciNfc_HciCoreCheckBuildHciFragments(pHciContext,
                                                             wStatus,
                                                             &dwTotalLenOfBuiltPkt,
                                                             (uint8_t*)&aHcpFragmentData);

            _Analysis_assume_(dwTotalLenOfBuiltPkt <= PHHCI_HCP_MAX_PACKET_SIZE);
        }else
        {
            /* Failed Response from Lower Layer */
            PH_LOG_LIBNFC_CRIT_STR(" Failed /Pending Response from Lower Layer ");

            /* Intimate the upper layer */
            pHciContext->pHciCoreContext.phHciNfcCoreUpperLayerSendCb(pHciContext->pHciCoreContext.pUpperLayerContext,
                                                                      wStatus);
        }
        /* Send the built HCI packets to Lower Layer */
        if(dwTotalLenOfBuiltPkt > 0)
        {
            tSendData.length = dwTotalLenOfBuiltPkt;
            tSendData.buffer = aHcpFragmentData;
            wStatus = phNciNfc_SeSendData(pHciContext->pNciContext,
                                        pHciContext->pSeHandle,
                                        &phHciNfc_HciCoreLowerLayerSendCb,
                                        pHciContext,
                                        &tSendData);
            if(wStatus == NFCSTATUS_PENDING)
            {
                PH_LOG_LIBNFC_CRIT_STR(" HCP Packet Sent to NCI ");
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR(" Failed to send HCP to Lower ");
            }
        }
    }else
    {
        PH_LOG_LIBNFC_CRIT_STR(" Invalid HCI Context received from Lower Layer ");
    }
}

static NFCSTATUS
phHciNfc_HciCoreCheckBuildHciFragments(_Inout_ phHciNfc_HciContext_t *pHciContext,
                                       _In_ NFCSTATUS   wStatus,
                                       _Out_range_(<=, PHHCI_HCP_MAX_PACKET_SIZE)
                                       uint32_t         *pdwTotalLenOfBuiltPkt,
                                       _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPkt)
                                       uint8_t          aHcpFragmentData[])
{
    /* dwTxPayldDataCntLeft Total Number of bytes Left to be transmitted */
    uint32_t dwTxPayldDataCntLeft;
    uint32_t dwTxCurrentDataIndex = 0;

    dwTxCurrentDataIndex = pHciContext->pHciCoreContext.tHciFragInfo.dwTxdHcpPayloadNoOfDataBytes;
    dwTxPayldDataCntLeft = pHciContext->pHciCoreContext.tHciCtxSendMsgParams.dwLen - dwTxCurrentDataIndex;
    if( dwTxPayldDataCntLeft > 0)
    {
        if( dwTxPayldDataCntLeft < PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH )
        {
            wStatus = phHciNfc_HciCoreBuildHcpFragment(dwTxCurrentDataIndex,
                                                       dwTxPayldDataCntLeft,
                                                       HCP_CHAINBIT_UN_CHAINED,
                                                       &pHciContext->pHciCoreContext.tHciCtxSendMsgParams,
                                                       pdwTotalLenOfBuiltPkt,
                                                       aHcpFragmentData);
            /* Update the Current data index to point to Databuffer for Next Packet */
            dwTxCurrentDataIndex     = dwTxCurrentDataIndex + dwTxPayldDataCntLeft;
        }
        else if(dwTxPayldDataCntLeft == PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH)
        {
            /* This will be the last (complete filled to max lower layer limit ) packet so send the last
            *  packet with chaining bit set
            */
            wStatus = phHciNfc_HciCoreBuildHcpFragment(dwTxCurrentDataIndex,
                                                       dwTxPayldDataCntLeft,
                                                       HCP_CHAINBIT_UN_CHAINED,
                                                       &pHciContext->pHciCoreContext.tHciCtxSendMsgParams,
                                                       pdwTotalLenOfBuiltPkt,
                                                       aHcpFragmentData);
            /* Update the Current data index to point to Databuffer for Next Packet */
            dwTxCurrentDataIndex     = dwTxCurrentDataIndex + dwTxPayldDataCntLeft;
        }
        else
        {
            /* Fragmentation should be continued, More Fragmented packets to follow
            *  Build sucessive HCI fragments
            */
            wStatus = phHciNfc_HciCoreBuildHcpFragment(dwTxCurrentDataIndex,
                                                       PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH,
                                                       HCP_CHAINBIT_CHAINED,
                                                       &pHciContext->pHciCoreContext.tHciCtxSendMsgParams,
                                                       pdwTotalLenOfBuiltPkt,
                                                       aHcpFragmentData);
            /* Update the Current data index to point to Databuffer for Next Packet */
            dwTxCurrentDataIndex     = dwTxCurrentDataIndex + PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH;
        }
        pHciContext->pHciCoreContext.tHciFragInfo.dwTxdHcpPayloadNoOfDataBytes = dwTxCurrentDataIndex;
    }
    else
    {
        *pdwTotalLenOfBuiltPkt = dwTxPayldDataCntLeft;
        /* Response for the Final packet */
        if(pHciContext->pHciCoreContext.phHciNfcCoreUpperLayerSendCb != NULL)
        {
            pHciContext->pHciCoreContext.phHciNfcCoreUpperLayerSendCb(
                             pHciContext->pHciCoreContext.pUpperLayerContext,
                             wStatus);
        }else
        {
            PH_LOG_LIBNFC_CRIT_STR(" HCI Core Upper Layer Send Call back Not Defined ");
        }
    }
    return wStatus;
}

static NFCSTATUS
phHciNfc_HciCoreBuildHcpFragment(_In_ uint32_t  dwCurrIndexOfPayloadData,
                                 _In_range_(<=, PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH)
                                 uint32_t       dwLenOfHcpFragData,
                                 _In_ uint8_t   bChaining,
                                 _In_ phHciNfc_SendParams_t *pSendMsgParams,
                                 _Out_range_(<=, PHHCI_HCP_MAX_PACKET_SIZE)
                                 uint32_t       *pdwTotalLenOfBuiltPk,
                                 _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPk)
                                 uint8_t        pFragmentData[]
                                )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(pFragmentData != NULL)
    {
        *pdwTotalLenOfBuiltPk      = PHHCI_HCI_PACKET_FRAG_HEADER_LEN;
        phOsalNfc_SetMemory(pFragmentData,0,PHHCI_HCP_MAX_PACKET_SIZE);
        /* Set the Chaining bit to indicate chaining */
        *pFragmentData             = (uint8_t) SET_BITS8(*pFragmentData, PHHCINFC_HCP_CHAINBIT_OFFSET,
                                     PHHCINFC_HCP_CHAINBIT_LEN, bChaining);
        /* Populate the Pipe ID to the HCP Header */
        *pFragmentData             = (uint8_t) SET_BITS8(*pFragmentData,PHHCINFC_HCP_PIPEID_OFFSET,
                                    PHHCINFC_HCP_PIPEID_LEN, pSendMsgParams->bPipeId);
        if(dwLenOfHcpFragData > 0)
        {
            /* Append Data if any to the HCP Fragment Packet Payload */
            phOsalNfc_MemCopy(pFragmentData + PHHCI_HCI_PACKET_FRAG_HEADER_LEN,
                              pSendMsgParams->pData + dwCurrIndexOfPayloadData,
                              dwLenOfHcpFragData);
            *pdwTotalLenOfBuiltPk = *pdwTotalLenOfBuiltPk + dwLenOfHcpFragData;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    return wStatus;
}

static NFCSTATUS
phHciNfc_HciCoreBuildHcipacket(_In_ phHciNfc_SendParams_t   *pSendMsgParams,
                               _In_ uint8_t                 bChaining,
                               _In_range_(<=, PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE)
                               uint32_t                     dwLenOfHcpPktData,
                               _Out_ uint32_t               *pdwTotalLenOfBuiltPkt,
                               _Out_writes_bytes_to_(PHHCI_HCP_MAX_PACKET_SIZE, *pdwTotalLenOfBuiltPkt)
                               uint8_t                      *pBuiltHciPkt
                              )
{
    phHciNfc_HcpPacket_t tHciPacket;

    /* Initialize the HCP packet */
    tHciPacket.bHcpPktHeader          = 0;
    tHciPacket.tHcpMessage.bMsgHeader = 0;
    phOsalNfc_SetMemory(tHciPacket.tHcpMessage.aHcpPktPayload,0,sizeof(tHciPacket.tHcpMessage.aHcpPktPayload));

    *pdwTotalLenOfBuiltPkt             = PHHCINFC_HCP_HEADER_LEN;
    /* Set the Chaining bit to indicate chaining */
    tHciPacket.bHcpPktHeader          = (uint8_t) SET_BITS8(tHciPacket.bHcpPktHeader,
                                                            PHHCINFC_HCP_CHAINBIT_OFFSET,
                                                            PHHCINFC_HCP_CHAINBIT_LEN,
                                                            bChaining);

    /* Populate the Pipe ID to the HCP Header */
    tHciPacket.bHcpPktHeader          = (uint8_t) SET_BITS8(tHciPacket.bHcpPktHeader,
                                                            PHHCINFC_HCP_PIPEID_OFFSET,
                                                            PHHCINFC_HCP_PIPEID_LEN,
                                                            pSendMsgParams->bPipeId);

    /* Set the type to the provided message type in the HCP Message Header */
    tHciPacket.tHcpMessage.bMsgHeader = (uint8_t) SET_BITS8(tHciPacket.tHcpMessage.bMsgHeader,
                                                            PHHCINFC_HCP_MSG_TYPE_OFFSET,
                                                            PHHCINFC_HCP_MSG_TYPE_LEN,
                                                            pSendMsgParams->bMsgType);

    /* Set the instruction to the type of instruction in the HCP Message Header */
    tHciPacket.tHcpMessage.bMsgHeader = (uint8_t) SET_BITS8(tHciPacket.tHcpMessage.bMsgHeader,
                                                            PHHCINFC_HCP_MSG_INSTRUCTION_OFFSET,
                                                            PHHCINFC_HCP_MSG_INSTRUCTION_LEN,
                                                            pSendMsgParams->bIns);
    if(pSendMsgParams->pData != NULL && dwLenOfHcpPktData > 0)
    {
        /* Append Data if any to the HCP Packet Payload */
        phOsalNfc_MemCopy((uint8_t*)(tHciPacket.tHcpMessage.aHcpPktPayload),
                          (uint8_t*)(pSendMsgParams->pData),
                          dwLenOfHcpPktData);
    }
    *pBuiltHciPkt                                        = tHciPacket.bHcpPktHeader;
    *(pBuiltHciPkt + PHHCINFC_HCP_MESSAGE_HEADER_OFFSET) = tHciPacket.tHcpMessage.bMsgHeader;
    /* Append HCI payload to the built HCI packet */
    if( dwLenOfHcpPktData > 0)
    {
        phOsalNfc_MemCopy(pBuiltHciPkt + PHHCINFC_HCP_MESSAGE_PAYLOAD_OFFSET,
                           (uint8_t*)(tHciPacket.tHcpMessage.aHcpPktPayload),
                           dwLenOfHcpPktData);
        *pdwTotalLenOfBuiltPkt = *pdwTotalLenOfBuiltPkt + dwLenOfHcpPktData;
    }
    return NFCSTATUS_SUCCESS;
}
