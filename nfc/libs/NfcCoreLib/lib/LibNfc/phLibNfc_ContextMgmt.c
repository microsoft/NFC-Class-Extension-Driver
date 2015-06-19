/*
* =============================================================================
*
*          Modifications Copyright ? Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#include "phLibNfc_Pch.h"

#include"phLibNfc_ContextMgmt.h"

#include "phLibNfc_ContextMgmt.tmh"

/* Server Context */
phLibNfc_SnepServerContext_t *gpServerContext = NULL;
phLibNfc_SnepClientContext_t *gpClientContext = NULL;

BOOL
phLibNfc_SnepServer_InitServerContext(void)
{
    if (NULL == gpServerContext)
    {
        gpServerContext = (phLibNfc_SnepServerContext_t*)phOsalNfc_GetMemory(sizeof(phLibNfc_SnepServerContext_t));
        if (NULL == gpServerContext)
        {
            return FALSE;
        }
        memset(gpServerContext, 0x0, sizeof(phLibNfc_SnepServerContext_t));
    }
    return TRUE;
}

phLibNfc_SnepServerContext_t *
phLibNfc_SnepServer_GetServerContext(void)
{
    return gpServerContext;
}

BOOL
phLibNfc_SnepServer_RemoveServerContext(void)
{
    BOOL status = FALSE;

    if ((NULL != gpServerContext) &&
        (0 == gpServerContext->CurrentServerCnt))
    {
        phOsalNfc_FreeMemory(gpServerContext);
        gpServerContext = NULL;
        status = TRUE;
    }
    return status;
}

phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_GetSessionContext(phLibNfc_Handle ServerHandle)
{
    uint8_t count;
    phLibNfc_SnepServerSession_t *pSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    pServerContext = phLibNfc_SnepServer_GetServerContext();
    if ((NULL != pServerContext) &&
        (0 != pServerContext->CurrentServerCnt))
    {
        for (count = 0; count < MAX_SNEP_SERVER_CNT; count++)
        {
            if (NULL != pServerContext->pServerSession[count])
            {
                if (ServerHandle == pServerContext->pServerSession[count]->hSnepServerHandle)
                {
                    pSession = pServerContext->pServerSession[count];
                    break;
                }
            } 
        }
    }
    return pSession;
}

phLibNfc_SnepServerConnection_t *
phLibNfc_SnepServer_GetConnectionContext(phLibNfc_Handle ConnHandle)
{
    uint8_t count_srv, count_con;
    phLibNfc_SnepServerConnection_t *pConnection = NULL;
    phLibNfc_SnepServerSession_t *pServerSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if ((NULL != pServerContext) &&
        (0 != pServerContext->CurrentServerCnt))
    {
        for (count_srv = 0; count_srv < MAX_SNEP_SERVER_CNT; count_srv++)
        {
            pServerSession = pServerContext->pServerSession[count_srv];
            if ((NULL != pServerSession) &&
            (0 != pServerSession->CurrentConnCnt))
            {
                for (count_con = 0; count_con < MAX_SNEP_SERVER_CONN; count_con++)
                {
                    if (NULL != pServerSession->pServerConnection[count_con])
                    {
                        if (ConnHandle == pServerSession->pServerConnection[count_con]->hSnepServerConnHandle)
                        {
                            pConnection = pServerSession->pServerConnection[count_con];
                            break;
                        } 
                    } 
                }
            } 
            if (NULL != pConnection)
            {
                break;
            } 
        }
    } 
    return pConnection;
}

phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_AddSession(void)
{
    uint8_t count;
    phLibNfc_SnepServerSession_t *pSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    pServerContext = phLibNfc_SnepServer_GetServerContext();
    if ((NULL != pServerContext) && (MAX_SNEP_SERVER_CNT > pServerContext->CurrentServerCnt))
    {
        for (count = 0;count < MAX_SNEP_SERVER_CNT;count++)
        {
            if (NULL == pServerContext->pServerSession[count])
            {
                pSession = (phLibNfc_SnepServerSession_t*)phOsalNfc_GetMemory(sizeof(phLibNfc_SnepServerSession_t));
                if (NULL != pSession)
                {
                    memset(pSession, 0x0, sizeof(phLibNfc_SnepServerSession_t));
                    pServerContext->pServerSession[count] = pSession;
                    pServerContext->CurrentServerCnt++;
                }
                break;
            }
        }
    }
    return pSession;
}

phLibNfc_SnepServerConnection_t *
phLibNfc_SnepServer_AddConnection(phLibNfc_Handle ServerHandle){
    uint8_t count;
    phLibNfc_SnepServerConnection_t *pConnection = NULL;
    phLibNfc_SnepServerSession_t *pServerConnContext = NULL;

    pServerConnContext = phLibNfc_SnepServer_GetSessionContext(ServerHandle);
    if ((NULL != pServerConnContext) && (MAX_SNEP_SERVER_CONN > pServerConnContext->CurrentConnCnt))
    {
        for (count = 0;count < MAX_SNEP_SERVER_CONN;count++)
        {
            if (NULL == pServerConnContext->pServerConnection[count])
            {
                pConnection = (phLibNfc_SnepServerConnection_t*)phOsalNfc_GetMemory(sizeof(phLibNfc_SnepServerConnection_t));
                if (NULL != pConnection)
                {
                    memset(pConnection, 0x0, sizeof(phLibNfc_SnepServerConnection_t));
                    pServerConnContext->pServerConnection[count] = pConnection;
                    pServerConnContext->CurrentConnCnt++;
                }
                break;
            }
        }
    }
    return pConnection;
}

NFCSTATUS
phLibNfc_SnepServer_RemoveSession(phLibNfc_Handle ServerHandle)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t count;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;
    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if (NULL != pServerContext)
    {
        for (count = 0; count < MAX_SNEP_SERVER_CNT; count++)
        {
            if (NULL != pServerContext->pServerSession[count])
            {
                if (ServerHandle == pServerContext->pServerSession[count]->hSnepServerHandle)
                {
                    if (0 != pServerContext->pServerSession[count]->CurrentConnCnt)
                    {
                        status = phLibNfc_SnepServer_RemoveAllConnections(ServerHandle);
                    }
                    else
                    {
                        status = NFCSTATUS_SUCCESS;
                    }

                    status = phLibNfc_Llcp_Close(ServerHandle);

                    phOsalNfc_FreeMemory(pServerContext->pServerSession[count]->sWorkingBuffer.buffer);

                    phOsalNfc_FreeMemory(pServerContext->pServerSession[count]);
                    pServerContext->pServerSession[count] = NULL;
                    pServerContext->CurrentServerCnt--;

                    break;
                } 
            } 
        }
    }
    return status;
}

NFCSTATUS
phLibNfc_SnepServer_RemoveOneConnection(phLibNfc_Handle ConnHandle)
{
    NFCSTATUS status = NFCSTATUS_NOT_REGISTERED;
    uint8_t count_srv, count_con;
    phLibNfc_SnepServerSession_t *pServerSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if ((NULL != pServerContext) &&
        (0 != pServerContext->CurrentServerCnt))
    {
        for (count_srv = 0; count_srv < MAX_SNEP_SERVER_CNT; count_srv++)
        {
            pServerSession = pServerContext->pServerSession[count_srv];
            if ((NULL != pServerSession) &&
            (0 != pServerSession->CurrentConnCnt))
            {
                for (count_con = 0; count_con < MAX_SNEP_SERVER_CONN; count_con++)
                {
                    if (NULL != pServerSession->pServerConnection[count_con])
                    {
                        if (ConnHandle == pServerSession->pServerConnection[count_con]->hSnepServerConnHandle)
                        {
                            status = phLibNfc_Llcp_Close(pServerSession->pServerConnection[count_con]->hSnepServerConnHandle);

                            // This should free the memory even in failure cases.
                            if (NULL != pServerSession->pServerConnection[count_con]->sConnWorkingBuffer.buffer)
                            {
                                phOsalNfc_FreeMemory(pServerSession->pServerConnection[count_con]->sConnWorkingBuffer.buffer);
                            }

                            if (NULL != pServerSession->pServerConnection[count_con]->responseDataContext.pProcessingBuffer)
                            {
                                phOsalNfc_FreeMemory(pServerSession->pServerConnection[count_con]->responseDataContext.pProcessingBuffer->buffer);
                                phOsalNfc_FreeMemory(pServerSession->pServerConnection[count_con]->responseDataContext.pProcessingBuffer);
                            }

                            phOsalNfc_FreeMemory(pServerSession->pServerConnection[count_con]);
                            pServerSession->pServerConnection[count_con] = NULL;
                            pServerSession->CurrentConnCnt--;
                            break;
                        } 
                    } 
                }
            } 
        }
    }
    else
    {
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    return status;
}

NFCSTATUS
phLibNfc_SnepServer_RemoveAllConnections(phLibNfc_Handle ServerHandle)
{
    NFCSTATUS status = NFCSTATUS_NOT_REGISTERED;
    uint8_t count;
    phLibNfc_SnepServerSession_t *pServerSession = NULL;

    pServerSession = phLibNfc_SnepServer_GetSessionContext(ServerHandle);
    if ((NULL != pServerSession) &&
        (0 != pServerSession->CurrentConnCnt))
    {
        for (count = 0;count < MAX_SNEP_SERVER_CONN;count++)
        {
            if (NULL != pServerSession->pServerConnection[count])
            {
                status = phLibNfc_Llcp_Close(pServerSession->pServerConnection[count]->hSnepServerConnHandle);
                if (NULL != pServerSession->pServerConnection[count]->sConnWorkingBuffer.buffer)
                {
                    phOsalNfc_FreeMemory(pServerSession->pServerConnection[count]->sConnWorkingBuffer.buffer);
                }
                if (NULL != pServerSession->pServerConnection[count]->responseDataContext.pProcessingBuffer)
                {
                    phOsalNfc_FreeMemory(pServerSession->pServerConnection[count]->responseDataContext.pProcessingBuffer->buffer);
                    phOsalNfc_FreeMemory(pServerSession->pServerConnection[count]->responseDataContext.pProcessingBuffer);
                }

                phOsalNfc_FreeMemory(pServerSession->pServerConnection[count]);
                pServerSession->pServerConnection[count] = NULL;
                pServerSession->CurrentConnCnt--;

                if (NFCSTATUS_SUCCESS == status)
                {
                    PH_LOG_LIBNFC_INFO_STR("> phLibNfc_SnepServer_RemoveAllConnections OK!");

                }
            }
        }
    }
    else
    {
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    return status;
}

phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_GetSessionByConnection(phLibNfc_Handle ConnHandle)
{
    uint8_t count_srv, count_con;
    phLibNfc_SnepServerSession_t *pServerSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if ((NULL != pServerContext) &&
        (0 != pServerContext->CurrentServerCnt))
    {
        for (count_srv = 0; count_srv < MAX_SNEP_SERVER_CNT; count_srv++)
        {
            pServerSession = pServerContext->pServerSession[count_srv];
            if ((NULL != pServerSession) &&
            (0 != pServerSession->CurrentConnCnt))
            {
                for (count_con = 0; count_con < MAX_SNEP_SERVER_CONN; count_con++)
                {
                    if (NULL != pServerSession->pServerConnection[count_con])
                    {
                        if (ConnHandle == pServerSession->pServerConnection[count_con]->hSnepServerConnHandle)
                        {
                            return pServerSession;
                        } 
                    } 
                }
            } 
        }
    } 
    return pServerSession;
}

BOOL
phLibNfc_SnepServer_InitClientContext(void)
{
    if (NULL == gpClientContext)
    {
        gpClientContext = (phLibNfc_SnepClientContext_t*)phOsalNfc_GetMemory(sizeof(phLibNfc_SnepClientContext_t));
        if (NULL == gpClientContext)
        {
            return FALSE;
        }
        memset(gpClientContext, 0x0, sizeof(phLibNfc_SnepClientContext_t));
    }
    return TRUE;
}

phLibNfc_SnepClientContext_t *
phLibNfc_SnepClient_GetClientContext(void)
{
    return gpClientContext;
}

BOOL
phLibNfc_SnepClient_RemoveClientContext(void)
{
    BOOL status = FALSE;

    if ((NULL != gpClientContext) &&
        (0 == gpClientContext->CurrentClientCnt))
    {
        phOsalNfc_FreeMemory(gpClientContext);
        gpClientContext = NULL;
        status = TRUE;
    }
    return status;
}

phLibNfc_SnepClientSession_t *
phLibNfc_SnepClient_GetClientSession(phLibNfc_Handle ClientHandle)
{
    uint8_t count;
    phLibNfc_SnepClientSession_t *pSession = NULL;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    pClientContext = phLibNfc_SnepClient_GetClientContext();
    if ((NULL != pClientContext) &&
        (0 != pClientContext->CurrentClientCnt))
    {
        for (count = 0; count < MAX_SNEP_CLIENT_CNT; count++)
        {
            if (NULL != pClientContext->pClientSession[count])
            {
                if (ClientHandle == pClientContext->pClientSession[count]->hSnepClientHandle)
                {
                    pSession = pClientContext->pClientSession[count];
                    break;
                } 
            } 
        }
    }
    return pSession;
}

phLibNfc_SnepClientSession_t *
phLibNfc_SnepClient_AddSession(void)
{
    uint8_t count;
    phLibNfc_SnepClientSession_t *pSession = NULL;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    pClientContext = phLibNfc_SnepClient_GetClientContext();
    if ((NULL != pClientContext) && (MAX_SNEP_SERVER_CNT > pClientContext->CurrentClientCnt))
    {
        for (count = 0;count < MAX_SNEP_SERVER_CNT;count++)
        {
            if (NULL == pClientContext->pClientSession[count])
            {
                pSession = (phLibNfc_SnepClientSession_t*)phOsalNfc_GetMemory(sizeof(phLibNfc_SnepClientSession_t));
                if (NULL != pSession)
                {
                    memset(pSession, 0x0, sizeof(phLibNfc_SnepClientSession_t));
                    pClientContext->pClientSession[count] = pSession;
                    pClientContext->CurrentClientCnt++;
                }
                break;
            }
        }
    }
    return pSession;
}

NFCSTATUS
phLibNfc_SnepClient_RemoveSession(phLibNfc_Handle ClientHandle)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t count;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    pClientContext = phLibNfc_SnepClient_GetClientContext();
    if (NULL != pClientContext)
    {
        for (count = 0; count < MAX_SNEP_CLIENT_CNT; count++)
        {
            if (NULL != pClientContext->pClientSession[count])
            {
                if (ClientHandle == pClientContext->pClientSession[count]->hSnepClientHandle)
                {
                    status = phLibNfc_Llcp_Close(pClientContext->pClientSession[count]->hSnepClientHandle);

                    if (NULL != pClientContext->pClientSession[count]->sWorkingBuffer.buffer)
                    {
                        phOsalNfc_FreeMemory(pClientContext->pClientSession[count]->sWorkingBuffer.buffer);
                    }
                    
                    phOsalNfc_FreeMemory(pClientContext->pClientSession[count]);
                    pClientContext->pClientSession[count] = NULL;
                    pClientContext->CurrentClientCnt--;

                    if (NFCSTATUS_SUCCESS == status)
                    {
                        PH_LOG_LIBNFC_INFO_STR("> phLibNfc_SnepClient_RemoveSession OK!");
                    } else
                    {
                        PH_LOG_LIBNFC_CRIT_STR("> phLibNfc_SnepClient_RemoveSession FAILED!");
                    }
                    break;
                }
            }
        }
    }
    return status;
}

NFCSTATUS
phLibNfc_SnepClient_RemoveIncompleteSession(phLibNfc_SnepClientSession_t *pClientSession )
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t count;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    pClientContext = phLibNfc_SnepClient_GetClientContext();
    if (NULL != pClientContext &&
        NULL != pClientSession)
    {
        for (count = 0; count < MAX_SNEP_CLIENT_CNT; count++)
        {
            if (NULL != pClientContext->pClientSession[count])
            {
                if (pClientSession == pClientContext->pClientSession[count])
                {
                    if (NULL != pClientContext->pClientSession[count]->sWorkingBuffer.buffer)
                    {
                        phLibNfc_Llcp_Close(pClientContext->pClientSession[count]->hSnepClientHandle);
                        phOsalNfc_FreeMemory(pClientContext->pClientSession[count]->sWorkingBuffer.buffer);
                    }
                    phOsalNfc_FreeMemory(pClientContext->pClientSession[count]);
                    pClientContext->pClientSession[count] = NULL;
                    pClientContext->CurrentClientCnt--;
                }
            }
        }
    }
    return status;
}

NFCSTATUS
phLibNfc_SnepClient_RemoveAllSessions(void)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t count;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    pClientContext = phLibNfc_SnepClient_GetClientContext();
    if (NULL != pClientContext) 
    {
        for (count = 0; count < MAX_SNEP_CLIENT_CNT; count++)
        {
            if (NULL != pClientContext->pClientSession[count])
            {
                if (NULL != pClientContext->pClientSession[count]->sWorkingBuffer.buffer)
                {
                    phLibNfc_Llcp_Close(pClientContext->pClientSession[count]->hSnepClientHandle);
                    phOsalNfc_FreeMemory(pClientContext->pClientSession[count]->sWorkingBuffer.buffer);
                }
                phOsalNfc_FreeMemory(pClientContext->pClientSession[count]);
                pClientContext->pClientSession[count] = NULL;
                pClientContext->CurrentClientCnt--;

                if (NFCSTATUS_SUCCESS == status)
                {
                    PH_LOG_LIBNFC_INFO_STR("> phLibNfc_SnepClient_RemoveSession OK!");
                } else
                {
                    PH_LOG_LIBNFC_INFO_STR("> phLibNfc_SnepClient_RemoveSession FAILED!");
                }
            }
        }
    }
    return status;
}
