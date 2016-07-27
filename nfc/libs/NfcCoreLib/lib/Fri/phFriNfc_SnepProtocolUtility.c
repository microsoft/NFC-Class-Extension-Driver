/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_SnepProtocolUtility.h"
#include "phLibNfc_ContextMgmt.h"

#include "phFriNfc_SnepProtocolUtility.tmh"

phOsalNfc_Message_t Message = {0};

NFCSTATUS phLibNfc_SnepProtocolReq (pphLibNfc_SnepClientSession_t pClientSessionContext)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (pClientSessionContext->putGetDataContext.bWaitForContinue)
    {
        if ((pClientSessionContext->putGetDataContext.pProcessingBuffer->length < pClientSessionContext->iRemoteMiu) ||
            (pClientSessionContext->putGetDataContext.pSnepPacket->length < pClientSessionContext->iRemoteMiu))
        {
            PH_LOG_SNEP_CRIT_STR("Packet buffers not initialized!");
            ret = NFCSTATUS_BUFFER_TOO_SMALL;
            goto Done;
        }

        memcpy(pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer,
               pClientSessionContext->putGetDataContext.pSnepPacket->buffer,
               pClientSessionContext->iRemoteMiu);

        pClientSessionContext->putGetDataContext.iDataSent = pClientSessionContext->iRemoteMiu;

        ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                                  pClientSessionContext->hSnepClientHandle,
                                  pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                  SnepSocketSendCb, pClientSessionContext);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
        }
    }
    else
    {
        pClientSessionContext->putGetDataContext.iDataSent = pClientSessionContext->putGetDataContext.pSnepPacket->length;
        ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                                  pClientSessionContext->hSnepClientHandle,
                                  pClientSessionContext->putGetDataContext.pSnepPacket,
                                  SnepSocketSendCb, pClientSessionContext);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
        }
    }

Done:
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}

NFCSTATUS phLibNfc_SnepProtocolSendResponse (pphLibNfc_SnepServerConnection_t pSnepServerConnection)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    PH_LOG_SNEP_FUNC_ENTRY();

    if (pSnepServerConnection->responseDataContext.bWaitForContinue)
    {
        if((pSnepServerConnection->responseDataContext.pProcessingBuffer->length < pSnepServerConnection->iRemoteMiu) ||
           (pSnepServerConnection->responseDataContext.pSnepPacket->length < pSnepServerConnection->iRemoteMiu))
        {
            PH_LOG_SNEP_CRIT_STR("Packet buffers not initialized!");
            ret = NFCSTATUS_BUFFER_TOO_SMALL;
            goto Done;
        }

        memcpy(pSnepServerConnection->responseDataContext.pProcessingBuffer->buffer,
               pSnepServerConnection->responseDataContext.pSnepPacket->buffer,
               pSnepServerConnection->iRemoteMiu);

        pSnepServerConnection->responseDataContext.pProcessingBuffer->length = pSnepServerConnection->iRemoteMiu;
        pSnepServerConnection->responseDataContext.iDataSent = pSnepServerConnection->iRemoteMiu;

        ret = phLibNfc_Llcp_Send( pSnepServerConnection->hRemoteDevHandle,
                                  pSnepServerConnection->hSnepServerConnHandle,
                                  pSnepServerConnection->responseDataContext.pProcessingBuffer,
                                  LlcpSocketSendResponseCb, pSnepServerConnection);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, ret);
        }
    }
    else
    {
        pSnepServerConnection->responseDataContext.iDataSent = pSnepServerConnection->responseDataContext.pSnepPacket->length;
        ret = phLibNfc_Llcp_Send( pSnepServerConnection->hRemoteDevHandle,
                                  pSnepServerConnection->hSnepServerConnHandle,
                                  pSnepServerConnection->responseDataContext.pSnepPacket,
                                  LlcpSocketSendResponseCb, pSnepServerConnection);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, ret);
        }
    }

Done:
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}

void SnepSocketSendCb (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    uint32_t iFragmentLength = 0;
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;

    if (pClientSessionContext->putGetDataContext.pProcessingBuffer == NULL)
    {
        // TODO
        //pProcessingBuffer is occasionally NULL on fast-taps.
        // This should probably be fixed better than this...
        status = NFCSTATUS_FAILED;
    }

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NFCSTATUS_SUCCESS == status)
    {
        pClientSessionContext->putGetDataContext.pProcessingBuffer->length = pClientSessionContext->iMiu;

        if (pClientSessionContext->putGetDataContext.pSnepPacket->length > pClientSessionContext->putGetDataContext.iDataSent)
        {
            /*Send remianing data, check for continue packet */
            if (pClientSessionContext->putGetDataContext.bWaitForContinue &&
                !pClientSessionContext->putGetDataContext.bContinueReceived)
            {
                ret = phLibNfc_Llcp_Recv( pClientSessionContext->hRemoteDevHandle,
                                          pClientSessionContext->hSnepClientHandle,
                                          pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                          LlcpSocketRecvCbForRspContinue,
                                          pClientSessionContext);
                if (NFCSTATUS_PENDING != ret)
                {
                    phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
                }
            }
            else
            {
                iFragmentLength = ((pClientSessionContext->putGetDataContext.pSnepPacket->length - pClientSessionContext->putGetDataContext.iDataSent) < pClientSessionContext->iRemoteMiu)?
                                                (pClientSessionContext->putGetDataContext.pSnepPacket->length - pClientSessionContext->putGetDataContext.iDataSent) : 
                                                (pClientSessionContext->iRemoteMiu);

                memcpy(pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer,
                        pClientSessionContext->putGetDataContext.pSnepPacket->buffer + pClientSessionContext->putGetDataContext.iDataSent,
                        iFragmentLength);

                pClientSessionContext->putGetDataContext.pProcessingBuffer->length = iFragmentLength;
                pClientSessionContext->putGetDataContext.iDataSent += iFragmentLength;

                if (TRUE == pClientSessionContext->putGetDataContext.bWaitForContinue)
                {

                    ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                                                pClientSessionContext->hSnepClientHandle,
                                                pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                                SnepSocketSendCb, pClientSessionContext);
                    if (NFCSTATUS_PENDING != ret)
                    {
                        phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
                    }
                    else
                    {
                        pClientSessionContext->putGetDataContext.bWaitForContinue = FALSE;
                    }
                }
                else
                {
                    SnepSocketSendData (pContext, NFCSTATUS_SUCCESS);
                }
            }
        }
        else
        {
            /* TODO: start the reply collection*/
            CollectReply(pClientSessionContext);
        }
    }
    else
    {
        phLibNfc_ClearMemNCallCb(pClientSessionContext, status, NULL);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/*@TODO Remove once LLCP Send issue resolved */
void SnepSocketSendData (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pClientSessionContext &&
        NFCSTATUS_SUCCESS == status)
    {
        ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                                  pClientSessionContext->hSnepClientHandle,
                                  pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                  SnepSocketSendCb, pClientSessionContext);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/*@TODO Remove once LLCP Receive issue resolved */
void SnepSocketReceiveData (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepServerConnection_t pServerConnectionContext = (pphLibNfc_SnepServerConnection_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pServerConnectionContext &&
        NFCSTATUS_SUCCESS == status)
    {
        ret = phLibNfc_Llcp_Recv(pServerConnectionContext->hRemoteDevHandle,
                                pServerConnectionContext->hSnepServerConnHandle,
                                pServerConnectionContext->responseDataContext.pProcessingBuffer,
                                SnepSocketRecvCbForServer,
                                pServerConnectionContext);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/*@TODO Remove once LLCP Send issue resolved */
void SnepSocketSendDataSrv (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepServerConnection_t pServerConnectionContext = (pphLibNfc_SnepServerConnection_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pServerConnectionContext &&
        NFCSTATUS_SUCCESS == status)
    {
        ret = phLibNfc_Llcp_Send( pServerConnectionContext->hRemoteDevHandle,
                                  pServerConnectionContext->hSnepServerConnHandle,
                                  pServerConnectionContext->responseDataContext.pProcessingBuffer,
                                  SnepSocketSendCb, pServerConnectionContext);
        if (NFCSTATUS_PENDING != ret)
        {
            phLibNfc_ClearMemNCallResponseCb(pServerConnectionContext, ret);
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/*@TODO Remove once LLCP Receive issue resolved */
void SnepSocketReceiveDataCli (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pClientSessionContext &&
        NFCSTATUS_SUCCESS == status)
    {
        ret = phLibNfc_Llcp_Recv(pClientSessionContext->hRemoteDevHandle,
                                pClientSessionContext->hSnepClientHandle,
                                pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                SnepSocketRecvCbForServer,
                                pClientSessionContext);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

void LlcpSocketSendResponseCb (void *pContext, NFCSTATUS status)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    uint32_t iFragmentLength = 0;
    pphLibNfc_SnepServerConnection_t pSnepServerConnection = (pphLibNfc_SnepServerConnection_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();
    pSnepServerConnection->responseDataContext.pProcessingBuffer->length = pSnepServerConnection->iMiu;

    if (NFCSTATUS_SUCCESS == status)
    {
        if (pSnepServerConnection->responseDataContext.pSnepPacket->length > pSnepServerConnection->responseDataContext.iDataSent)
        {
            /*Send remianing data, check for continue packet */
            if (pSnepServerConnection->responseDataContext.bWaitForContinue)
            {
                if (!pSnepServerConnection->responseDataContext.bContinueReceived)
                {
                    ret = phLibNfc_Llcp_Recv( pSnepServerConnection->hRemoteDevHandle,
                                              pSnepServerConnection->hSnepServerConnHandle,
                                              pSnepServerConnection->responseDataContext.pProcessingBuffer,
                                              LlcpSocketRecvCbForReqContinue,
                                              pSnepServerConnection);
                    if (NFCSTATUS_PENDING != ret)
                    {
                        phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, ret);
                    }
                }
                else
                {
                    iFragmentLength = ((pSnepServerConnection->responseDataContext.pSnepPacket->length - pSnepServerConnection->responseDataContext.iDataSent) < pSnepServerConnection->iRemoteMiu) ?
                                           (pSnepServerConnection->responseDataContext.pSnepPacket->length - pSnepServerConnection->responseDataContext.iDataSent) : 
                                           (pSnepServerConnection->iRemoteMiu);


                    memcpy(pSnepServerConnection->responseDataContext.pProcessingBuffer->buffer,
                           pSnepServerConnection->responseDataContext.pSnepPacket->buffer + pSnepServerConnection->responseDataContext.iDataSent,
                           iFragmentLength);

                    pSnepServerConnection->responseDataContext.pProcessingBuffer->length = iFragmentLength;
                    pSnepServerConnection->responseDataContext.iDataSent += iFragmentLength;

                    ret = phLibNfc_Llcp_Send( pSnepServerConnection->hRemoteDevHandle,
                                              pSnepServerConnection->hSnepServerConnHandle,
                                              pSnepServerConnection->responseDataContext.pProcessingBuffer,
                                              LlcpSocketSendResponseCb, pSnepServerConnection);
                    if (NFCSTATUS_PENDING != ret)
                    {
                        phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, ret);
                    }
                }
            }
        }
        else
        {
            if (pSnepServerConnection->responseDataContext.bIsExcessData)
            {
                phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA);
            }
            else
            {
                phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, ret);
            }
        }
    }
    else
    {
        phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, status);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

phNfc_sData_t* phLibNfc_PrepareSnepPacket(phLibNfc_SnepPacket_t packetType, phNfc_sData_t *pData,
                                          uint8_t version, uint32_t acceptableLength)
{
    phNfc_sData_t *pSnepPacket = NULL;
    uint8_t *pSnepPktTraverse = NULL;
    uint8_t *pBuffer=0;
    uint32_t iSnepPacketSize = 0;
    uint32_t dwSnepPayloadLen = 0x00;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pData)
    {
        iSnepPacketSize = SNEP_HEADER_SIZE + pData->length;
    }
    else
    {
        iSnepPacketSize = SNEP_HEADER_SIZE;
    }

    if (phLibNfc_SnepGet == packetType)
    {
        iSnepPacketSize += SNEP_REQUEST_ACCEPTABLE_LENGTH_SIZE;
    }

    pSnepPacket = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
    if (NULL != pSnepPacket)
    {
        pSnepPacket->buffer = NULL;
        pSnepPacket->buffer = phOsalNfc_GetMemory(iSnepPacketSize);
    }

    if (NULL != pSnepPacket && NULL != pSnepPacket->buffer)
    {
        pSnepPacket->length = iSnepPacketSize;
        pSnepPktTraverse = pSnepPacket->buffer;
        memset(pSnepPktTraverse, 0, iSnepPacketSize);
        *pSnepPktTraverse = version;
        pSnepPktTraverse += SNEP_VERSION_LENGTH;

        *pSnepPktTraverse = GetSnepResponseCodeFromPacketType(packetType);

        pSnepPktTraverse += SNEP_REQUEST_LENGTH;

        if (NULL != pData)
        {
            /* Update the snep payload length including the Acceptable length */
            if (phLibNfc_SnepGet == packetType)
            {
                dwSnepPayloadLen = pData->length + SNEP_REQUEST_ACCEPTABLE_LENGTH_SIZE;
                pBuffer = (uint8_t*)(&dwSnepPayloadLen);
            }
            else
            {
                /* Big endian length */
                pBuffer = (uint8_t*)(&pData->length);
            }

            *pSnepPktTraverse = pBuffer[3];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[2];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[1];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[0];
            pSnepPktTraverse++;
        }
        else
        {
            *((uint32_t *)pSnepPktTraverse) = 0x00;
        }
        if (phLibNfc_SnepGet == packetType)
        {
            pBuffer = (uint8_t*)&acceptableLength;
            *pSnepPktTraverse = pBuffer[3];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[2];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[1];
            pSnepPktTraverse++;
            *pSnepPktTraverse = pBuffer[0];
            pSnepPktTraverse++;
        }
        if (NULL != pData)
        {
            memcpy(pSnepPktTraverse, pData->buffer, pData->length);
        }
    }
    else
    {
        PH_LOG_SNEP_CRIT_STR("phLibNfc_PrepareSnepPacket: ***failed allocate memory");
        if (NULL != pSnepPacket)
        {
            phOsalNfc_FreeMemory(pSnepPacket->buffer);
            phOsalNfc_FreeMemory(pSnepPacket);
            pSnepPacket = NULL;
        }
    }

    PH_LOG_SNEP_FUNC_EXIT();
    return pSnepPacket;
}

void LlcpSocketRecvCbForInvalidResponse(void* pContext, NFCSTATUS status)
{
    pphLibNfc_SnepClientSession_t pClientSessionContext = NULL;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pContext)
    {
        PH_LOG_SNEP_INFO_X32MSG("Status: ", status);
        pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;
        if (pClientSessionContext->putGetDataContext.bContinueReceived)
        {
            /* We have received an erroneous response while sending fragments
            is in progress. Invoke upper layer with REJECT */
            PH_LOG_SNEP_INFO_STR("Error: Received an erroneous response while send in progress");
            phLibNfc_Llcp_CancelPendingSend(pClientSessionContext->hRemoteDevHandle,
                                            pClientSessionContext->hSnepClientHandle);
        }
    }

    PH_LOG_SNEP_FUNC_EXIT();
}

void LlcpSocketRecvCbForRspContinue(void* pContext, NFCSTATUS status)
{
    uint8_t *pSnepPktTraverse = NULL;
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;
    NFCSTATUS returnValue = NFCSTATUS_SUCCESS;

    PH_LOG_SNEP_FUNC_ENTRY();

    pSnepPktTraverse = pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer;
    if (NFCSTATUS_SUCCESS == status &&
        SNEP_RESPONSE_CONTINUE == pSnepPktTraverse[SNEP_VERSION_LENGTH])
    {
        if (areVersionsCompatible(pClientSessionContext->SnepClientVersion, *pSnepPktTraverse))
        {
            /* Check if server erroneously sends any packet.
               Cb will be invoked when packets are received erroneously from server */
            if (FALSE != pClientSessionContext->bDtaFlag)
            {
                returnValue = phLibNfc_Llcp_Recv(pClientSessionContext->hRemoteDevHandle,
                                                 pClientSessionContext->hSnepClientHandle,
                                                 pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                                 LlcpSocketRecvCbForInvalidResponse,
                                                 pClientSessionContext);

                PH_LOG_SNEP_INFO_X32MSG("phLibNfc_Llcp_Recv status: ", returnValue);

                if (NFCSTATUS_PENDING != returnValue)
                {
                    phLibNfc_ClearMemNCallCb(pClientSessionContext, returnValue, NULL);
                    pClientSessionContext->pReqCb = NULL;
                }
            }

            pClientSessionContext->putGetDataContext.bContinueReceived = TRUE;
            SnepSocketSendCb(pClientSessionContext, NFCSTATUS_SUCCESS);
        }
        else
        {
            pClientSessionContext->putGetDataContext.bWaitForContinue = FALSE;
            phLibNfc_ClearMemNCallCb(pClientSessionContext, SNEP_RESPONSE_UNSUPPORTED_VERSION, NULL);
        }

    }
    else
    {
        pClientSessionContext->putGetDataContext.bWaitForContinue = FALSE;
        if (areVersionsCompatible(pClientSessionContext->SnepClientVersion, *pSnepPktTraverse))
        {
            PH_LOG_SNEP_CRIT_STR("LlcpSocketRecvCbForRspContinue: ***did not receive continue");
            /* SNEP_RESPONSE_REJECT will be received in this path */
            phLibNfc_ClearMemNCallCb(pClientSessionContext,
                                     GetNfcStatusFromSnepResponse(pSnepPktTraverse[SNEP_VERSION_LENGTH]),
                                     NULL);
        }
        else
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, SNEP_RESPONSE_UNSUPPORTED_VERSION, NULL);
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

void LlcpSocketRecvCbForInvalidRequest(void* pContext, NFCSTATUS status)
{
    pphLibNfc_SnepServerConnection_t pSnepServerConnection = NULL;

    PH_LOG_SNEP_FUNC_ENTRY();

    if (NULL != pContext)
    {
        PH_LOG_SNEP_INFO_X32MSG("Status: ", status);
        pSnepServerConnection = (pphLibNfc_SnepServerConnection_t)pContext;
        if (pSnepServerConnection->responseDataContext.bContinueReceived)
        {
            /* We have received an invalid message from Client.
               Invoke upper layer with Invalid protocol data */
            PH_LOG_SNEP_CRIT_STR("Error: Received a packet while sending fragments in progress");

            phLibNfc_Llcp_CancelPendingSend(pSnepServerConnection->hRemoteDevHandle,
                                            pSnepServerConnection->hSnepServerConnHandle);
        }
    }

    PH_LOG_SNEP_FUNC_EXIT();
}

void LlcpSocketRecvCbForReqContinue(void* pContext, NFCSTATUS status)
{
    uint8_t *pSnepPktTraverse = NULL;
    NFCSTATUS returnValue = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepServerConnection_t pSnepServerConnection = (pphLibNfc_SnepServerConnection_t)pContext;
    phLibNfc_SnepServerSession_t* pServerSession = NULL;

    PHNFC_UNUSED_VARIABLE(status);

    PH_LOG_SNEP_FUNC_ENTRY();
    pSnepPktTraverse = pSnepServerConnection->responseDataContext.pProcessingBuffer->buffer;
    if (SNEP_REQUEST_CONTINUE == pSnepPktTraverse[SNEP_VERSION_LENGTH])
    {
        if (areVersionsCompatible(pSnepServerConnection->SnepServerVersion, *pSnepPktTraverse))
        {
            pServerSession = pSnepServerConnection->pServerSession;

            /* Check if we receive any packets from client erroneously.
               Cb will be invoked when any erroneous packet is received */
            if (FALSE != pServerSession->bDtaFlag)
            {
                returnValue = phLibNfc_Llcp_Recv(pSnepServerConnection->hRemoteDevHandle,
                                                 pSnepServerConnection->hSnepServerConnHandle,
                                                 pSnepServerConnection->responseDataContext.pProcessingBuffer,
                                                 LlcpSocketRecvCbForInvalidRequest,
                                                 pSnepServerConnection);
                if (NFCSTATUS_PENDING != returnValue)
                {
                    phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, returnValue);
                }
            }

            pSnepServerConnection->responseDataContext.bContinueReceived = TRUE;
            LlcpSocketSendResponseCb(pSnepServerConnection, NFCSTATUS_SUCCESS);
        }
        else
        {
            pSnepServerConnection->responseDataContext.bWaitForContinue = FALSE;
            /* send Reject, in completion reset context and restart read */
            phLibNfc_SnepProtocolSrvSendResponse(pSnepServerConnection->hSnepServerConnHandle,
                                                 NULL,
                                                 NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION,
                                                 SnepSrvSendcompleteInternal,
                                                 pSnepServerConnection);
        }
    }
    else if (SNEP_REQUEST_REJECT == pSnepPktTraverse[SNEP_VERSION_LENGTH])
    {
        pSnepServerConnection->responseDataContext.bWaitForContinue = FALSE;
        if (areVersionsCompatible(pSnepServerConnection->SnepServerVersion, *pSnepPktTraverse))
        {
            phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, NFCSTATUS_SNEP_REQUEST_REJECT);
        }
        else
        {

            phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION);
            /* send Reject, in completion reset context and restart read */
            phLibNfc_SnepProtocolSrvSendResponse(pSnepServerConnection->hSnepServerConnHandle,
                                                 NULL,
                                                 NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION,
                                                 SnepSrvSendcompleteInternal,
                                                 pSnepServerConnection);
        }
    }
    else
    {
        phLibNfc_ClearMemNCallResponseCb(pSnepServerConnection, NFCSTATUS_SNEP_INVALID_PROTOCOL_DATA);
        phLibNfc_SnepProtocolSrvSendResponse(pSnepServerConnection->hSnepServerConnHandle,
                                             NULL,
                                             NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST,
                                             SnepSrvSendcompleteInternal,
                                             pSnepServerConnection);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}


NFCSTATUS CollectReply(pphLibNfc_SnepClientSession_t pClientSessionContext)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    PH_LOG_SNEP_FUNC_ENTRY();

    pClientSessionContext->putGetDataContext.pProcessingBuffer->length = pClientSessionContext->iMiu;
    ret = phLibNfc_Llcp_Recv( pClientSessionContext->hRemoteDevHandle,
                              pClientSessionContext->hSnepClientHandle,
                              pClientSessionContext->putGetDataContext.pProcessingBuffer,
                              LlcpSocketRecvCbForRecvBegin,
                              pClientSessionContext);

    if (NFCSTATUS_PENDING != ret &&
        NFCSTATUS_SUCCESS != ret)
    {
        if (FALSE == pClientSessionContext->bDtaFlag)
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, ret, NULL);
        }
    } else if (NFCSTATUS_SUCCESS == ret)
    {
        /* sometimes receive call returns success, so call again */
        ret = phLibNfc_Llcp_Recv( pClientSessionContext->hRemoteDevHandle,
                              pClientSessionContext->hSnepClientHandle,
                              pClientSessionContext->putGetDataContext.pProcessingBuffer,
                              LlcpSocketRecvCbForRecvBegin,
                              pClientSessionContext);
    }
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}

void LlcpSocketRecvCbForRecvBegin(void* pContext,
                                  NFCSTATUS status)
{
    uint8_t *pSnepPktTraverse     = NULL;
    uint8_t *pSnepReqPktTraverse  = NULL;
    uint8_t *pdata_len = NULL;
    uint8_t *pConvert = NULL;
    uint32_t iInfoLength          = 0;

    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;
    PH_LOG_SNEP_FUNC_ENTRY();

    if (NFCSTATUS_SUCCESS ==  status)
    {
        pSnepReqPktTraverse = pClientSessionContext->putGetDataContext.pSnepPacket->buffer;
        pSnepPktTraverse = pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer;
        pdata_len = (pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer + SNEP_REQUEST_LENGTH + SNEP_VERSION_LENGTH);
        pConvert = (uint8_t*)&iInfoLength;
        pConvert[3] = *pdata_len;
        pdata_len++;
        pConvert[2] = *pdata_len;
        pdata_len++;
        pConvert[1] = *pdata_len;
        pdata_len++;
        pConvert[0] = *pdata_len;
        if (0 == iInfoLength)
        {
            phLibNfc_ClearMemNCallCb (pClientSessionContext,
                                      GetNfcStatusFromSnepResponse(pSnepPktTraverse[SNEP_VERSION_LENGTH]),
                                      NULL);
        }
        else if ((NULL != pSnepReqPktTraverse) &&
                 (SNEP_REQUEST_GET == pSnepReqPktTraverse[SNEP_VERSION_LENGTH]) &&
                 (iInfoLength > pClientSessionContext->acceptableLength))
        {
            if (SNEP_RESPONSE_EXCESS_DATA != pSnepPktTraverse[SNEP_VERSION_LENGTH])
            {
                PH_LOG_SNEP_WARN_STR("Invlaid response returned from SNEP server, %d, client state %d",
                                     pSnepPktTraverse[SNEP_VERSION_LENGTH], pClientSessionContext->Client_state);
            }

            sendSnepRequestReject(pClientSessionContext);
        }
        else
        {
            pClientSessionContext->putGetDataContext.pReqResponse = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
            if (pClientSessionContext->putGetDataContext.pReqResponse)
            {
                pClientSessionContext->putGetDataContext.pReqResponse->buffer = phOsalNfc_GetMemory(iInfoLength);
            }
            if (NULL != pClientSessionContext->putGetDataContext.pReqResponse &&
                NULL != pClientSessionContext->putGetDataContext.pReqResponse->buffer)
            {
                pClientSessionContext->putGetDataContext.pReqResponse->length = iInfoLength;
                if (iInfoLength + SNEP_HEADER_SIZE <= pClientSessionContext->iMiu &&
                    ((NULL != pSnepReqPktTraverse && SNEP_REQUEST_PUT == pSnepReqPktTraverse[SNEP_VERSION_LENGTH]) ||
                    iInfoLength == pClientSessionContext->putGetDataContext.pProcessingBuffer->length - SNEP_HEADER_SIZE))
                {
                    pClientSessionContext->putGetDataContext.pReqResponse->length = iInfoLength;
                    memcpy(pClientSessionContext->putGetDataContext.pReqResponse->buffer, pSnepPktTraverse + SNEP_HEADER_SIZE, iInfoLength);
                    phLibNfc_ClearMemNCallCb(pClientSessionContext,
                                             GetNfcStatusFromSnepResponse(pSnepPktTraverse[SNEP_VERSION_LENGTH]),
                                             pClientSessionContext->putGetDataContext.pReqResponse);
                }
                else
                {
                    if (areVersionsCompatible(pClientSessionContext->SnepClientVersion, *pSnepPktTraverse))
                    {
                        /*send request continue, and in callback start receive again*/
                        memcpy(pClientSessionContext->putGetDataContext.pReqResponse->buffer, pSnepPktTraverse + SNEP_HEADER_SIZE,
                            pClientSessionContext->putGetDataContext.pProcessingBuffer->length - SNEP_HEADER_SIZE);
                        pClientSessionContext->putGetDataContext.iDataReceived = (pClientSessionContext->putGetDataContext.pProcessingBuffer->length - SNEP_HEADER_SIZE);
                        sendSnepRequestContinue(pClientSessionContext);
                    }
                    else
                    {
                        /*send request Reject, and in callback start receive again*/
                        pClientSessionContext->putGetDataContext.iDataReceived = 0;
                        sendSnepRequestReject(pClientSessionContext);
                    }
                }
            }
            else
            {
                sendSnepRequestReject(pClientSessionContext);
            }
        }
    }
    else
    {
        phLibNfc_ClearMemNCallCb (pClientSessionContext,
                                  status,
                                  NULL);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

NFCSTATUS GetNfcStatusFromSnepResponse(uint8_t snepCode)
{
    NFCSTATUS ret = NFCSTATUS_SNEP_INVALID_PROTOCOL_DATA;

    switch(snepCode)
    {
    case SNEP_RESPONSE_CONTINUE:
        ret = NFCSTATUS_SNEP_RESPONSE_CONTINUE;
        break;
    case SNEP_RESPONSE_SUCCESS:
        ret = NFCSTATUS_SUCCESS;
        break;
    case SNEP_RESPONSE_NOT_FOUND:
        ret = NFCSTATUS_SNEP_RESPONSE_NOT_FOUND;
        break;
    case SNEP_RESPONSE_EXCESS_DATA:
        ret = NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA;
        break;
    case SNEP_RESPONSE_BAD_REQUEST:
        ret = NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST;
        break;
    case SNEP_RESPONSE_NOT_IMPLEMENTED:
        ret = NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED;
        break;
    case SNEP_RESPONSE_UNSUPPORTED_VERSION:
        ret = NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION;
        break;
    case SNEP_RESPONSE_REJECT:
        ret = NFCSTATUS_SNEP_RESPONSE_REJECT;
        break;
    default:
        ret = NFCSTATUS_SNEP_INVALID_PROTOCOL_DATA;
    }
    return ret;
}

uint8_t GetSnepResponseCodeFromPacketType(phLibNfc_SnepPacket_t packetType)
{
    uint8_t ret = SNEP_RESPONSE_REJECT;
    switch(packetType)
        {
    case phLibNfc_SnepPut:
        ret = SNEP_REQUEST_PUT;
        break;
    case phLibNfc_SnepGet:
        ret = SNEP_REQUEST_GET;
        break;
    case phLibNfc_SnepContinue:
        ret = SNEP_RESPONSE_CONTINUE;
        break;
    case phLibNfc_SnepSuccess:
        ret = SNEP_RESPONSE_SUCCESS;
        break;
    case phLibNfc_SnepNotFound:
        ret = SNEP_RESPONSE_NOT_FOUND;
        break;
    case phLibNfc_SnepExcessData:
        ret = SNEP_RESPONSE_EXCESS_DATA;
        break;
    case phLibNfc_SnepBadRequest:
        ret = SNEP_RESPONSE_BAD_REQUEST;
        break;
    case phLibNfc_SnepNotImplemented:
        ret = SNEP_RESPONSE_NOT_IMPLEMENTED;
        break;
    case phLibNfc_SnepUnsupportedVersion:
        ret = SNEP_RESPONSE_UNSUPPORTED_VERSION;
        break;
    case phLibNfc_SnepReject:
        ret = SNEP_RESPONSE_REJECT;
        break;
    default:
        ret = SNEP_RESPONSE_REJECT;
    }
    return ret;
}

uint8_t GetSnepResponseCodeFromNfcStatus(NFCSTATUS nfcStatus)
{
    uint8_t ret = SNEP_RESPONSE_REJECT;
    switch(nfcStatus)
        {
    case NFCSTATUS_SNEP_RESPONSE_CONTINUE:
        ret = SNEP_RESPONSE_CONTINUE;
        break;
    case NFCSTATUS_SUCCESS:
        ret = SNEP_RESPONSE_SUCCESS;
        break;
    case NFCSTATUS_SNEP_RESPONSE_NOT_FOUND:
        ret = SNEP_RESPONSE_NOT_FOUND;
        break;
    case NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA:
        ret = SNEP_RESPONSE_EXCESS_DATA;
        break;
    case NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST:
        ret = SNEP_RESPONSE_BAD_REQUEST;
        break;
    case NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED:
        ret = SNEP_RESPONSE_NOT_IMPLEMENTED;
        break;
    case NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION:
        ret = SNEP_RESPONSE_UNSUPPORTED_VERSION;
        break;
    case NFCSTATUS_SNEP_RESPONSE_REJECT:
        ret = SNEP_RESPONSE_REJECT;
        break;
    default:
        ret = SNEP_RESPONSE_REJECT;
    }
    return ret;
}

phLibNfc_SnepPacket_t GetSnepPacketType(NFCSTATUS nfcStatus)
{
    phLibNfc_SnepPacket_t ret = phLibNfc_SnepInvalid;
    switch(nfcStatus)
    {
    case NFCSTATUS_SNEP_RESPONSE_CONTINUE:
        ret = phLibNfc_SnepContinue;
        break;
    case NFCSTATUS_SUCCESS:
        ret = phLibNfc_SnepSuccess;
        break;
    case NFCSTATUS_SNEP_RESPONSE_NOT_FOUND:
        ret = phLibNfc_SnepNotFound;
        break;
    case NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA:
        ret = phLibNfc_SnepExcessData;
        break;
    case NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST:
        ret = phLibNfc_SnepBadRequest;
        break;
    case NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED:
        ret = phLibNfc_SnepNotImplemented;
        break;
    case NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION:
        ret = phLibNfc_SnepUnsupportedVersion;
        break;
    case NFCSTATUS_SNEP_RESPONSE_REJECT:
        ret = phLibNfc_SnepReject;
        break;
    default:
        ret = phLibNfc_SnepInvalid;
    }
    return ret;
}

void sendSnepRequestContinue(pphLibNfc_SnepClientSession_t pClientSessionContext)
{
    uint8_t *pSnepPktTraverse = pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer ;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    
    memset(pSnepPktTraverse, 0, pClientSessionContext->iRemoteMiu);
    *pSnepPktTraverse = SNEP_VERSION;
    pSnepPktTraverse += SNEP_VERSION_LENGTH;

    *pSnepPktTraverse = SNEP_REQUEST_CONTINUE;
    pSnepPktTraverse += SNEP_REQUEST_LENGTH;
    pClientSessionContext->putGetDataContext.pProcessingBuffer->length = SNEP_HEADER_SIZE;

    ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                              pClientSessionContext->hSnepClientHandle,
                              pClientSessionContext->putGetDataContext.pProcessingBuffer,
                              LlcpSocketSendCbForReqContinue, pClientSessionContext);
}

void LlcpSocketSendCbForReqContinue (void *pContext, NFCSTATUS status)
{
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();
    if (NFCSTATUS_SUCCESS == status)
    {
        pClientSessionContext->putGetDataContext.pProcessingBuffer->length = 0;
        LlcpSocketReceiveCb (pClientSessionContext, NFCSTATUS_SUCCESS);
    }
    else
    {
        phLibNfc_ClearMemNCallCb(pClientSessionContext, NFCSTATUS_SNEP_REQUEST_CONTINUE_FAILED, NULL);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

void sendSnepRequestReject(pphLibNfc_SnepClientSession_t pClientSessionContext)
{
    uint8_t *pSnepPktTraverse = pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer ;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    
    memset(pSnepPktTraverse, 0, pClientSessionContext->iRemoteMiu);
    *pSnepPktTraverse = SNEP_VERSION;
    pSnepPktTraverse += SNEP_VERSION_LENGTH;

    *pSnepPktTraverse = SNEP_REQUEST_REJECT;
    pSnepPktTraverse += SNEP_REQUEST_LENGTH;
    pClientSessionContext->putGetDataContext.pProcessingBuffer->length = SNEP_HEADER_SIZE;

    ret = phLibNfc_Llcp_Send( pClientSessionContext->hRemoteDevHandle,
                              pClientSessionContext->hSnepClientHandle,
                              pClientSessionContext->putGetDataContext.pProcessingBuffer,
                              LlcpSocketSendCbForReqReject, pClientSessionContext);
}

void LlcpSocketSendCbForReqReject (void *pContext, NFCSTATUS status)
{
    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;

    PH_LOG_SNEP_FUNC_ENTRY();
    if (NFCSTATUS_SUCCESS == status)
    {
        status = NFCSTATUS_SNEP_REQUEST_REJECT;
    }
    else
    {
        status = NFCSTATUS_SNEP_REQUEST_REJECT_FAILED;
    }
    phLibNfc_ClearMemNCallCb(pClientSessionContext, status, NULL);
    PH_LOG_SNEP_FUNC_EXIT();
}

void LlcpSocketReceiveCb(void* pContext, NFCSTATUS status)
{
    uint8_t *pSnepPktTraverse     = NULL;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;

    pphLibNfc_SnepClientSession_t pClientSessionContext = (pphLibNfc_SnepClientSession_t)pContext;
    PH_LOG_SNEP_FUNC_ENTRY();

    if (NFCSTATUS_SUCCESS ==  status)
    {
        pSnepPktTraverse = pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer;

        if (pClientSessionContext->putGetDataContext.pProcessingBuffer->length)
        {
            memcpy (pClientSessionContext->putGetDataContext.pReqResponse->buffer + pClientSessionContext->putGetDataContext.iDataReceived,
                    pSnepPktTraverse, pClientSessionContext->putGetDataContext.pProcessingBuffer->length);

            pClientSessionContext->putGetDataContext.iDataReceived += pClientSessionContext->putGetDataContext.pProcessingBuffer->length;
        }
        if (pClientSessionContext->putGetDataContext.iDataReceived < pClientSessionContext->putGetDataContext.pReqResponse->length)
        {
            pClientSessionContext->putGetDataContext.pProcessingBuffer->length = pClientSessionContext->iMiu;
            ret = phLibNfc_Llcp_Recv( pClientSessionContext->hRemoteDevHandle,
                                      pClientSessionContext->hSnepClientHandle,
                                      pClientSessionContext->putGetDataContext.pProcessingBuffer,
                                      LlcpSocketReceiveCb,
                                      pClientSessionContext);
            if(!((NFCSTATUS_SUCCESS == ret) || (NFCSTATUS_PENDING == ret)))
            {
                /* TODO: delete response message buffer in all error cases */
                phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pReqResponse->buffer);
                phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pReqResponse);
                pClientSessionContext->putGetDataContext.pReqResponse = NULL;
                phLibNfc_ClearMemNCallCb(pClientSessionContext, status, NULL);
            }
        }
        else
        {
            phLibNfc_ClearMemNCallCb(pClientSessionContext, status, pClientSessionContext->putGetDataContext.pReqResponse);
        }

    }
    else
    {
        phLibNfc_ClearMemNCallCb (pClientSessionContext,
                                  status,
                                  NULL);
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/* This function is called in the cases of failure or success to clean memory and call the callback to inform failure */

void phLibNfc_ClearMemNCallCb(pphLibNfc_SnepClientSession_t pClientSessionContext, NFCSTATUS status,
                              phNfc_sData_t *pReqResponse)
{
    PH_LOG_SNEP_FUNC_ENTRY();

    if (pClientSessionContext->putGetDataContext.pSnepPacket)
    {
        if (NULL != pClientSessionContext->putGetDataContext.pSnepPacket->buffer)
        {
            phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pSnepPacket->buffer);
        } /* No Else */
        phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pSnepPacket);
        pClientSessionContext->putGetDataContext.pSnepPacket = NULL;
    }

    if (pClientSessionContext->putGetDataContext.pProcessingBuffer)
    {
        phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer);
        phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer);
        pClientSessionContext->putGetDataContext.pProcessingBuffer = NULL;
    }
    pClientSessionContext->pReqCb (pClientSessionContext->hSnepClientHandle, pClientSessionContext->pReqCbContext,
                                   status, pReqResponse );

    PH_LOG_SNEP_FUNC_EXIT();
}


void phLibNfc_ClearMemNCallResponseCb(pphLibNfc_SnepServerConnection_t pSnepServerConnection, NFCSTATUS status)
{
    NFCSTATUS ret;
    PH_LOG_SNEP_FUNC_ENTRY();

    if (pSnepServerConnection->responseDataContext.pSnepPacket)
    {
        phOsalNfc_FreeMemory(pSnepServerConnection->responseDataContext.pSnepPacket->buffer);
        phOsalNfc_FreeMemory(pSnepServerConnection->responseDataContext.pSnepPacket);
        pSnepServerConnection->responseDataContext.pSnepPacket = NULL;
    }
    pSnepServerConnection->responseDataContext.fSendCompleteCb (pSnepServerConnection->responseDataContext.cbContext,
                                                                status,
                                                                pSnepServerConnection->hSnepServerConnHandle);

    /* reset context and restart receive */
    /*pSnepServerConnection->iDataTobeReceived = 0;*/
    pSnepServerConnection->responseDataContext.pProcessingBuffer->length = pSnepServerConnection->iMiu;
    ret = phLibNfc_Llcp_Recv( pSnepServerConnection->hRemoteDevHandle,
                        pSnepServerConnection->hSnepServerConnHandle,
                        pSnepServerConnection->responseDataContext.pProcessingBuffer,
                        SnepSocketRecvCbForServer,
                        pSnepServerConnection);
    if (NFCSTATUS_PENDING != ret &&
        NFCSTATUS_SUCCESS != ret)
    {
        PH_LOG_SNEP_CRIT_STR("RECEIVE FAILURE");
    }
    PH_LOG_SNEP_FUNC_EXIT();
}

/* Server receive callback. reads the complete message and informs the upper layer */

void SnepSocketRecvCbForServer(void* pContext,
                               NFCSTATUS status)
{
    uint8_t *pSnepPktTraverse     = NULL;
    uint8_t *pdata_len = NULL;
    uint8_t *pConvert = NULL;
    uint32_t iInfoLength          = 0;
    uint8_t req_type;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;

    pphLibNfc_SnepServerConnection_t pServerConnectionContext = (pphLibNfc_SnepServerConnection_t)pContext;
    pphLibNfc_SnepServerSession_t pServerSessionContext = NULL;
    PH_LOG_SNEP_FUNC_ENTRY();

    if (NFCSTATUS_SUCCESS ==  status)
    {
        pServerSessionContext = phLibNfc_SnepServer_GetSessionByConnection(pServerConnectionContext->hSnepServerConnHandle);
        pSnepPktTraverse = pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer;
        if (!pServerConnectionContext->iDataTobeReceived)
        {
            //iInfoLength = *((int *)(pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer + SNEP_REQUEST_LENGTH + SNEP_VERSION_LENGTH));
            pdata_len = (pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer + SNEP_REQUEST_LENGTH + SNEP_VERSION_LENGTH);
            pConvert = (uint8_t*)&iInfoLength;
            pConvert[3] = *pdata_len;
            pdata_len++;
            pConvert[2] = *pdata_len;
            pdata_len++;
            pConvert[1] = *pdata_len;
            pdata_len++;
            pConvert[0] = *pdata_len;
            pServerConnectionContext->iDataTobeReceived = iInfoLength;
            req_type = pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer[SNEP_VERSION_LENGTH];
            if ((SNEP_REQUEST_PUT == req_type || SNEP_REQUEST_GET == req_type) && 0 != iInfoLength)
            {
                if (areVersionsCompatible(pServerConnectionContext->SnepServerVersion, *pSnepPktTraverse))
                {
                    if (pServerConnectionContext->SnepServerVersion > (*pSnepPktTraverse))
                    {
                        pServerConnectionContext->SnepServerVersion = *pSnepPktTraverse;
                    }
                    if (iInfoLength > pServerConnectionContext->iInboxSize)
                    {
                        /* send Reject, in completion reset context and restart read */
                        phLibNfc_SnepProtocolSrvSendResponse (pServerConnectionContext->hSnepServerConnHandle,
                            NULL, NFCSTATUS_SNEP_RESPONSE_REJECT, SnepSrvSendcompleteInternal,
                            pServerConnectionContext);
                    }
                    else if (NULL != pServerSessionContext &&
                            SNEP_REQUEST_GET == pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer[SNEP_VERSION_LENGTH] &&
                            phLibNfc_SnepServer_Default == pServerSessionContext->SnepServerType)
                    {
                        /* Default server should Reject Incoming GET Request */
                        phLibNfc_SnepProtocolSrvSendResponse (pServerConnectionContext->hSnepServerConnHandle,
                            NULL, NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED, SnepSrvSendcompleteInternal,
                            pServerConnectionContext);
                    }
                    else
                    {
                        if (SNEP_REQUEST_PUT == req_type)
                        {
                            pServerConnectionContext->ServerConnectionState = phLibNfc_SnepServer_Received_Put;
                            pServerConnectionContext->pSnepWorkingBuffer = pServerConnectionContext->pDataInbox;
                        }
                        else if (SNEP_REQUEST_GET == req_type)
                        {
                            pdata_len = (pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer + SNEP_HEADER_SIZE);
                            pConvert = (uint8_t*)&(pServerConnectionContext->responseDataContext.iAcceptableLength);
                            pConvert[3] = *pdata_len;
                            pdata_len++;
                            pConvert[2] = *pdata_len;
                            pdata_len++;
                            pConvert[1] = *pdata_len;
                            pdata_len++;
                            pConvert[0] = *pdata_len;

                            pServerConnectionContext->ServerConnectionState = phLibNfc_SnepServer_Received_Get;
                            pServerConnectionContext->pSnepWorkingBuffer = (phNfc_sData_t*)phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                            if (NULL != pServerConnectionContext->pSnepWorkingBuffer)
                            {
                                pServerConnectionContext->pSnepWorkingBuffer->buffer = phOsalNfc_GetMemory((pServerConnectionContext->iDataTobeReceived));
                                if (NULL == pServerConnectionContext->pSnepWorkingBuffer->buffer)
                                {
                                    PH_LOG_SNEP_CRIT_STR("GET memory alloc failed");
                                }
                            } else
                            {
                                PH_LOG_SNEP_CRIT_STR("GET memory alloc failed");
                            }
                        } /* No Else */

                        if (NULL != pServerConnectionContext->pSnepWorkingBuffer)
                        {
                            memcpy(pServerConnectionContext->pSnepWorkingBuffer->buffer, pSnepPktTraverse + SNEP_HEADER_SIZE,
                                pServerConnectionContext->responseDataContext.pProcessingBuffer->length - SNEP_HEADER_SIZE);
                            pServerConnectionContext->pSnepWorkingBuffer->length =
                                (pServerConnectionContext->responseDataContext.pProcessingBuffer->length- SNEP_HEADER_SIZE);
                        }

                        if (NULL != pServerConnectionContext->pSnepWorkingBuffer)
                        {
                            if (pServerConnectionContext->iDataTobeReceived > pServerConnectionContext->pSnepWorkingBuffer->length)
                            {
                                /* send continue, in completion restart receive*/
                                phLibNfc_SnepProtocolSrvSendResponse (pServerConnectionContext->hSnepServerConnHandle,
                                NULL, NFCSTATUS_SNEP_RESPONSE_CONTINUE,
                                SnepSrvSendContinuecomplete,
                                pServerConnectionContext);
                            }
                            else
                            {
                                //call put/Get Notify
                                phLibNfc_NotifyUpperLayer(pServerConnectionContext);
                            }
                        }
                    }
                }
                else
                {
                    /* Reject request with Unsupported version, in completion reset context and restart read */
                    phLibNfc_SnepProtocolSrvSendResponse (pServerConnectionContext->hSnepServerConnHandle,
                        NULL, NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION, SnepSrvSendcompleteInternal,
                        pServerConnectionContext);
                    /* Invoke upper layer indicating the snep client request is rejected */
                    if(NULL != pServerSessionContext->pConnectionCb)
                    {
                        pServerSessionContext->pConnectionCb(pServerSessionContext->pListenContext,
                                                             NFCSTATUS_REJECTED,
                                                             pServerConnectionContext);
                    }
                }
            }
            else
            {
                /* reset context and restart receive */
                pServerConnectionContext->iDataTobeReceived = 0;
                pServerConnectionContext->responseDataContext.pProcessingBuffer->length = pServerConnectionContext->iMiu;
                /* Check whether a undesired packet is received such as 'Snep request Continue' */
                if ((0x00 != iInfoLength) ||
                   ((SNEP_REQUEST_CONTINUE != req_type) && (req_type < SNEP_REQUEST_REJECT)))
                {
                    /* send 'Bad Request' response, as it is a malformed request */
                    phLibNfc_SnepProtocolSrvSendResponse(pServerConnectionContext->hSnepServerConnHandle,
                                                         NULL,
                                                         NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST,
                                                         SnepSrvSendcompleteInternal,
                                                         pServerConnectionContext);
                }
                /* Invoke upper layer indicating the snep client request is rejected */
                if(NULL != pServerSessionContext->pConnectionCb)
                {
                    pServerSessionContext->pConnectionCb(pServerSessionContext->pListenContext,
                                                         NFCSTATUS_REJECTED,
                                                         pServerConnectionContext);
                }
            }
        }
        else
        {
            memcpy((pServerConnectionContext->pSnepWorkingBuffer->buffer+pServerConnectionContext->pSnepWorkingBuffer->length), pSnepPktTraverse,
                        pServerConnectionContext->responseDataContext.pProcessingBuffer->length);
            pServerConnectionContext->pSnepWorkingBuffer->length +=
                pServerConnectionContext->responseDataContext.pProcessingBuffer->length;

            if (pServerConnectionContext->iDataTobeReceived > pServerConnectionContext->pSnepWorkingBuffer->length)
            {
                pServerConnectionContext->responseDataContext.pProcessingBuffer->length = pServerConnectionContext->iMiu;
                SnepSocketReceiveData (pContext, NFCSTATUS_SUCCESS);
            }
            else
            {
                //call put/Get Notify
                phLibNfc_NotifyUpperLayer(pServerConnectionContext);
            }
        }
    }
    else
    {
        pServerConnectionContext->iDataTobeReceived = 0;
        if (NULL == pServerConnectionContext->responseDataContext.pProcessingBuffer)
        {
            pServerConnectionContext->responseDataContext.pProcessingBuffer = (phNfc_sData_t*)phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
            if (NULL != pServerConnectionContext->responseDataContext.pProcessingBuffer)
            {
                pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer = (uint8_t*)phOsalNfc_GetMemory(max(pServerConnectionContext->iRemoteMiu,
                                                                                                                          pServerConnectionContext->iMiu));
                if (NULL == pServerConnectionContext->responseDataContext.pProcessingBuffer->buffer)
                {
                    return;
                }
            } else
            {
                return;
            }
        }/* No Else */

        pServerConnectionContext->responseDataContext.pProcessingBuffer->length = pServerConnectionContext->iMiu;
        ret = phLibNfc_Llcp_Recv( pServerConnectionContext->hRemoteDevHandle,
                                  pServerConnectionContext->hSnepServerConnHandle,
                                  pServerConnectionContext->responseDataContext.pProcessingBuffer,
                                  SnepSocketRecvCbForServer,
                                  pServerConnectionContext);
        if (NFCSTATUS_PENDING != ret)
        {
            PH_LOG_SNEP_INFO_STR("SnepSocketRecvCbForServer: *** ");
        }
    }

    PH_LOG_SNEP_FUNC_EXIT();
}

void phLibNfc_NotifyUpperLayer(pphLibNfc_SnepServerConnection_t pServerConnectionContext)
{
    PH_LOG_SNEP_FUNC_ENTRY();
    if (pServerConnectionContext)
    {
        if (phLibNfc_SnepServer_Received_Get == pServerConnectionContext->ServerConnectionState)
        {
            pServerConnectionContext->pGetNtfCb(pServerConnectionContext->pConnectionContext, NFCSTATUS_SUCCESS,
                pServerConnectionContext->pSnepWorkingBuffer,
                pServerConnectionContext->hSnepServerConnHandle);
            if (NULL != pServerConnectionContext->pSnepWorkingBuffer->buffer)
            {
                phOsalNfc_FreeMemory(pServerConnectionContext->pSnepWorkingBuffer->buffer);
                phOsalNfc_FreeMemory(pServerConnectionContext->pSnepWorkingBuffer);
                pServerConnectionContext->pSnepWorkingBuffer = NULL;
            }
        }
        else if (phLibNfc_SnepServer_Received_Put == pServerConnectionContext->ServerConnectionState)
        {
            pServerConnectionContext->pPutNtfCb(pServerConnectionContext->pConnectionContext, NFCSTATUS_SUCCESS,
                pServerConnectionContext->pSnepWorkingBuffer,
                pServerConnectionContext->hSnepServerConnHandle);
        }
        else
        {
            phLibNfc_SnepProtocolSrvSendResponse (pServerConnectionContext->hSnepServerConnHandle,
                                                  NULL, NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST,
                                                  SnepSrvSendcompleteInternal, /*TODO: define fn*/
                                                  pServerConnectionContext);
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
}


void SnepSrvSendContinuecomplete (void *pContext, NFCSTATUS status, phLibNfc_Handle ConnHandle)
{
    pphLibNfc_SnepServerConnection_t pSnepServerConnection = (pphLibNfc_SnepServerConnection_t)pContext;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;

    PHNFC_UNUSED_VARIABLE(status);
    PHNFC_UNUSED_VARIABLE(ConnHandle);

    PH_LOG_SNEP_FUNC_ENTRY();
    pSnepServerConnection->responseDataContext.pProcessingBuffer->length = pSnepServerConnection->iMiu;
    ret = phLibNfc_Llcp_Recv( pSnepServerConnection->hRemoteDevHandle,
                              pSnepServerConnection->hSnepServerConnHandle,
                              pSnepServerConnection->responseDataContext.pProcessingBuffer,
                              SnepSocketRecvCbForServer,
                              pSnepServerConnection);

    if (NFCSTATUS_PENDING != ret)
    {
        PH_LOG_SNEP_INFO_STR("SnepSrvSendContinuecomplete: ***");
    }

    PH_LOG_SNEP_FUNC_EXIT();
}

void SnepSrvSendcompleteInternal (void* pContext, NFCSTATUS  Status,
                                 phLibNfc_Handle ConnHandle)
{
    pphLibNfc_SnepServerConnection_t pSnepServerConnection = (pphLibNfc_SnepServerConnection_t)pContext;
    NFCSTATUS ret = NFCSTATUS_SUCCESS;

    PHNFC_UNUSED_VARIABLE(Status);
    PHNFC_UNUSED_VARIABLE(ConnHandle);

    PH_LOG_SNEP_FUNC_ENTRY();
    pSnepServerConnection->iDataTobeReceived = 0;
    pSnepServerConnection->responseDataContext.pProcessingBuffer->length = pSnepServerConnection->iMiu;
    ret = phLibNfc_Llcp_Recv( pSnepServerConnection->hRemoteDevHandle,
                              pSnepServerConnection->hSnepServerConnHandle,
                              pSnepServerConnection->responseDataContext.pProcessingBuffer,
                              SnepSocketRecvCbForServer,
                              pSnepServerConnection);

    PH_LOG_SNEP_FUNC_EXIT();
}

BOOL areVersionsCompatible(uint8_t ver1, uint8_t ver2)
{
    if ((ver1>>SNEP_VERSION_FIELD) == (ver2>>SNEP_VERSION_FIELD))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void ResetServerConnectionContext(pphLibNfc_SnepServerConnection_t pServerConnectionContext)
{
    pServerConnectionContext->iDataTobeReceived = 0;
    pServerConnectionContext->responseDataContext.iAcceptableLength = 0;
    pServerConnectionContext->responseDataContext.bIsExcessData = FALSE;

    pServerConnectionContext->responseDataContext.iDataSent = 0;
    pServerConnectionContext->responseDataContext.pSnepPacket = NULL;
    pServerConnectionContext->responseDataContext.bWaitForContinue = FALSE;  /* Do we need to wait for continue before send? */
    pServerConnectionContext->responseDataContext.bContinueReceived = FALSE; /* Have we received continue to send */
    pServerConnectionContext->responseDataContext.fSendCompleteCb = NULL;

}

void ResetCliDataContext(pphLibNfc_SnepClientSession_t pClientSessionContext)
{
    pClientSessionContext->putGetDataContext.iDataSent = 0;         /* count of data sent so far */
    pClientSessionContext->putGetDataContext.iDataReceived = 0;     /* count of data received so far */
    pClientSessionContext->putGetDataContext.pSnepPacket = NULL;      /* prepared snep packet */
    pClientSessionContext->putGetDataContext.bWaitForContinue = FALSE;  /* Do we need to wait for continue before send? */
    pClientSessionContext->putGetDataContext.bContinueReceived = FALSE; /* Have we received continue to send */
    pClientSessionContext->putGetDataContext.pReqResponse = NULL;     /* response data to be sentback to upper layer */
}
