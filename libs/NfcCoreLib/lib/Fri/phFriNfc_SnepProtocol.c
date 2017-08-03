/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_SnepProtocol.h"
#include "phLibNfc_Snep.h"
#include "phFriNfc_SnepProtocolUtility.h"
#include "phLibNfc_ContextMgmt.h"

#include "phFriNfc_SnepProtocol.tmh"

NFCSTATUS phLibNfc_SnepProtocolCliReqPut(phLibNfc_Handle ConnHandle, phNfc_sData_t *pPutData,
                                         pphLibNfc_SnepReqCb_t fCbPut, void *cbContext)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepClientSession_t pClientSessionContext = NULL;

    PH_LOG_SNEP_FUNC_ENTRY();
    if (NULL == pPutData ||
        NULL == pPutData->buffer ||
        0 == pPutData->length)
    {
        PH_LOG_SNEP_FUNC_EXIT();
        return NFCSTATUS_INVALID_PARAMETER;
    }

    pClientSessionContext = phLibNfc_SnepClient_GetClientSession(ConnHandle);

    if (NULL != pClientSessionContext)
    {
        ResetCliDataContext(pClientSessionContext);

        pClientSessionContext->putGetDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(phLibNfc_SnepPut,
                                                                                          pPutData,
                                                                                          pClientSessionContext->SnepClientVersion,
                                                                                          pClientSessionContext->acceptableLength);

        if (pClientSessionContext->putGetDataContext.pSnepPacket != NULL )
        {
            //this transfer needs processing, so lets allocate processing buffer
            if (!pClientSessionContext->putGetDataContext.pProcessingBuffer)
            {
                /* Delete this memory after put is complete */
                pClientSessionContext->putGetDataContext.pProcessingBuffer = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                if (pClientSessionContext->putGetDataContext.pProcessingBuffer)
                {
                    pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer = phOsalNfc_GetMemory(max(pClientSessionContext->iRemoteMiu, 
                                                                                                               pClientSessionContext->iMiu));
                    if(!pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer)
                    {
                        ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
                    {
                        pClientSessionContext->putGetDataContext.pProcessingBuffer->length = 
                            pClientSessionContext->iRemoteMiu;
                    }
                }
                else
                {
                    ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                }
            }
            if (NFCSTATUS_SUCCESS == ret)
            {
                if (pClientSessionContext->putGetDataContext.pSnepPacket->length > pClientSessionContext->iRemoteMiu)
                {
                    pClientSessionContext->putGetDataContext.bWaitForContinue = TRUE;
                    pClientSessionContext->putGetDataContext.bContinueReceived = FALSE;
                }

            }
            pClientSessionContext->pReqCb = fCbPut;
            pClientSessionContext->pReqCbContext = cbContext;
        }
    } else
    {
        ret = NFCSTATUS_FAILED;
    }

    if (NFCSTATUS_SUCCESS == ret &&
        NULL != pClientSessionContext)
    {
        ret = phLibNfc_SnepProtocolReq (pClientSessionContext);
        /* if sending of request is successfull then get the reply and analyze it to report
            apropriate NFC Status message and data to upperlayer */
    }
    else if (NULL != pClientSessionContext)
    {
        if (pClientSessionContext->putGetDataContext.pProcessingBuffer)
        {
            phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer);
            phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer);
            pClientSessionContext->putGetDataContext.pProcessingBuffer = NULL;
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}

NFCSTATUS phLibNfc_SnepProtocolCliReqGet(phLibNfc_Handle ConnHandle, phNfc_sData_t *pGetData, uint32_t acceptable_length,
                                         pphLibNfc_SnepReqCb_t fCbGet, void *cbContext)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepClientSession_t pClientSessionContext = NULL;

    PH_LOG_SNEP_FUNC_ENTRY();
    pClientSessionContext = phLibNfc_SnepClient_GetClientSession(ConnHandle);

    if (NULL != pClientSessionContext)
    {
        memset(&(pClientSessionContext->putGetDataContext), 0, sizeof(putGetDataContext_t));
        /* TODO Temporary Fix */
        pClientSessionContext->acceptableLength = acceptable_length;
        pClientSessionContext->putGetDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(phLibNfc_SnepGet,
                                                                                          pGetData,
                                                                                          pClientSessionContext->SnepClientVersion,
                                                                                          pClientSessionContext->acceptableLength);
        if (pClientSessionContext->putGetDataContext.pSnepPacket)
        {
            //this transfer needs processing, so lets allocate processing buffer
            if (!pClientSessionContext->putGetDataContext.pProcessingBuffer)
            {
                /* Delete this memory after put is complete */
                pClientSessionContext->putGetDataContext.pProcessingBuffer = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                if (pClientSessionContext->putGetDataContext.pProcessingBuffer)
                {
                    pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer = phOsalNfc_GetMemory(max(pClientSessionContext->iRemoteMiu,
                                                                                                                pClientSessionContext->iMiu));
                    if(!pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer)
                    {
                        ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
                    {
                        pClientSessionContext->putGetDataContext.pProcessingBuffer->length = 
                            pClientSessionContext->iRemoteMiu;
                    }
                }
                else
                {
                    ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                }
            }
           if (pClientSessionContext->putGetDataContext.pSnepPacket->length > pClientSessionContext->iRemoteMiu)
            {
                if (NFCSTATUS_SUCCESS == ret)
                {
                    pClientSessionContext->putGetDataContext.bWaitForContinue = TRUE;
                    pClientSessionContext->putGetDataContext.bContinueReceived = FALSE;

                }
            }
            pClientSessionContext->pReqCb = fCbGet;
            pClientSessionContext->pReqCbContext = cbContext;
        }
    }

    if ((NFCSTATUS_SUCCESS == ret) && (NULL != pClientSessionContext))
    {
        ret = phLibNfc_SnepProtocolReq (pClientSessionContext);
        /* if sending of request is successfull then get the reply and analyze it to report
            apropriate NFC Status message and data to upperlayer */
    }
    else
    {
        if(NULL != pClientSessionContext)
        {
            if (pClientSessionContext->putGetDataContext.pProcessingBuffer)
            {
                phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer->buffer);
                phOsalNfc_FreeMemory(pClientSessionContext->putGetDataContext.pProcessingBuffer);
                pClientSessionContext->putGetDataContext.pProcessingBuffer = NULL;
            }
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}

NFCSTATUS phLibNfc_SnepProtocolSrvSendResponse(phLibNfc_Handle ConnHandle, phNfc_sData_t *pResponseData, NFCSTATUS responseStatus,
                                                pphLibNfc_SnepProtocol_SendRspComplete_t fSendCompleteCb, void *cbContext)
{
    NFCSTATUS ret = NFCSTATUS_SUCCESS;
    pphLibNfc_SnepServerConnection_t pServerSessionContext = NULL;

    PH_LOG_SNEP_FUNC_ENTRY();
    pServerSessionContext = phLibNfc_SnepServer_GetConnectionContext(ConnHandle);

    if ( (NULL != pServerSessionContext ) && \
         (NULL != pServerSessionContext->responseDataContext.pProcessingBuffer) )
    {
        if (phLibNfc_SnepServer_Received_Get == pServerSessionContext->ServerConnectionState)
        {
            if (NFCSTATUS_SUCCESS == responseStatus)
            {
                if (NULL != pResponseData &&
                    NULL != pResponseData->buffer &&
                    0 != pResponseData->length)
                {
                    if (pResponseData->length > pServerSessionContext->responseDataContext.iAcceptableLength)
                    {
                        /* fail the request with excess data error */
                        pServerSessionContext->responseDataContext.bIsExcessData = TRUE;
                        if (NULL != pServerSessionContext->responseDataContext.pSnepPacket &&
                            NULL != pServerSessionContext->responseDataContext.pSnepPacket->buffer)
                        {
                            phOsalNfc_FreeMemory(pServerSessionContext->responseDataContext.pSnepPacket->buffer);
                            phOsalNfc_FreeMemory(pServerSessionContext->responseDataContext.pSnepPacket);
                            pServerSessionContext->responseDataContext.pSnepPacket = NULL;
                        }
                        pServerSessionContext->responseDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(
                            GetSnepPacketType(NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA), NULL,
                            pServerSessionContext->SnepServerVersion,
                            0); /* acceptable length is not used for server packets */
                    }
                    else
                    {
                        pServerSessionContext->responseDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(
                            GetSnepPacketType(responseStatus), pResponseData,
                            pServerSessionContext->SnepServerVersion, 0);
                    }
                }
                else
                {
                    PH_LOG_SNEP_FUNC_EXIT();
                    return NFCSTATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                pServerSessionContext->responseDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(
                            GetSnepPacketType(responseStatus), NULL,
                            pServerSessionContext->SnepServerVersion,
                            0); /* acceptable length is not used for server packets */
            }
            pServerSessionContext->iDataTobeReceived = 0;
        }
        else
        {
            /* reset Context (Dont do memset to zero as it would erase pointers in data context */
            if (NFCSTATUS_SNEP_RESPONSE_CONTINUE != responseStatus)
            {
                ResetServerConnectionContext(pServerSessionContext);
            }/* No Else */
            pServerSessionContext->responseDataContext.pSnepPacket = phLibNfc_PrepareSnepPacket(GetSnepPacketType(responseStatus),
                                                                                            pResponseData,
                                                                                            pServerSessionContext->SnepServerVersion,
                                                                                            0); /* acceptable length is not used for server packets */
        }

        if (pServerSessionContext->responseDataContext.pSnepPacket)
        {
            if (pServerSessionContext->responseDataContext.pSnepPacket->length > pServerSessionContext->iRemoteMiu)
            {
                if (!pServerSessionContext->responseDataContext.pProcessingBuffer)
                {
                    /* Delete this memory after send is complete */
                    pServerSessionContext->responseDataContext.pProcessingBuffer = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                    if (pServerSessionContext->responseDataContext.pProcessingBuffer)
                    {
                        pServerSessionContext->responseDataContext.pProcessingBuffer->buffer = phOsalNfc_GetMemory(max(pServerSessionContext->iRemoteMiu,
                                                                                                                     pServerSessionContext->iMiu));
                        if(!pServerSessionContext->responseDataContext.pProcessingBuffer->buffer)
                        {
                            ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                        }
                        else
                        {
                            pServerSessionContext->responseDataContext.pProcessingBuffer->length = 
                                pServerSessionContext->iRemoteMiu;
                        }
                    }
                    else
                    {
                        ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
                    }
                } 
                else
                {
                    pServerSessionContext->responseDataContext.pProcessingBuffer->length = 
                                pServerSessionContext->iRemoteMiu;
                }

                if (NFCSTATUS_SUCCESS == ret)
                {
                    pServerSessionContext->responseDataContext.bWaitForContinue = TRUE;
                    pServerSessionContext->responseDataContext.bContinueReceived = FALSE;

                }
            }
            pServerSessionContext->responseDataContext.fSendCompleteCb = fSendCompleteCb;
            pServerSessionContext->responseDataContext.cbContext = cbContext;
        } else
        {
            ret = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NFCSTATUS_SUCCESS == ret &&
        NULL != pServerSessionContext)
    {
        ret = phLibNfc_SnepProtocolSendResponse (pServerSessionContext);
    }
    if (NFCSTATUS_PENDING != ret &&
        NULL != pServerSessionContext)
    {
        if (pServerSessionContext->responseDataContext.pProcessingBuffer)
        {
            phOsalNfc_FreeMemory(pServerSessionContext->responseDataContext.pProcessingBuffer->buffer);
            phOsalNfc_FreeMemory(pServerSessionContext->responseDataContext.pProcessingBuffer);
            pServerSessionContext->responseDataContext.pProcessingBuffer = NULL;
        }
    }
    PH_LOG_SNEP_FUNC_EXIT();
    return ret;
}
