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

#include "phLibNfc_Snep.h"
#include "phLibNfc_ContextMgmt.h"
#include "phFriNfc_SnepProtocolUtility.h"

#include "phLibNfc_Snep.tmh"

#define SNEP_DEFAULT_SERVER_SAP 0x04
#define SNEP_NON_DEFAULT_SERVER_SAP 0x15
#define SNEP_DEFAULT_SERVER_NAME_LEN 0x0F
#define SNEP_PROTOCOL_VERSION 0x10

uint8_t DefaultServerName[SNEP_DEFAULT_SERVER_NAME_LEN] = {'u','r','n',':','n','f','c',':','s','n',':','s','n','e','p'};

/* ----------------------- Internal functions headers -------------------------- */

static void
phLibNfc_SnepLlcp_SocketErr_Cb(void* pContext, uint8_t nErrCode);

static void
phLibNfc_SnepLlcp_AcceptSocketErr_Cb(void *pContext, uint8_t nErrCode);

static void
phLibNfc_SnepLlcp_ListenSocket_Cb(void *pContext,
                                  phLibNfc_Handle hIncomingSocket);

static void
phLibNfc_SnepLlcp_ConnectSocket_Cb(void *pContext, uint8_t nErrCode,
                                   NFCSTATUS status);

static void
phLibNfc_SnepLlcp_AcceptSocket_Cb(void *pContext, NFCSTATUS status);

static void
phLibNfc_SnepLlcp_SendSocket_Cb(void *pContext, NFCSTATUS status);

static void
phLibNfc_SnepLlcp_RecvSocket_Cb(void *pContext, NFCSTATUS status);

/* --------------------------- Internal functions ------------------------------ */

static void
phLibNfc_SnepLlcp_SocketErr_Cb(void *pContext, uint8_t nErrCode)
{
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(nErrCode);
    PH_LOG_SNEP_CRIT_STR("phLibNfc_SnepLlcp_SocketErr_Cb:");
    /* Currently */
}

static void
phLibNfc_SnepLlcp_AcceptSocketErr_Cb(void *pContext, uint8_t nErrCode)
{
    NFCSTATUS status;
    phLibNfc_Handle ConnHandle = (phLibNfc_Handle)pContext;

    UNREFERENCED_PARAMETER(nErrCode);

    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepLlcp_AcceptSocketErr_Cb:");

    if (NULL != ConnHandle)
    {
        status = phLibNfc_SnepServer_RemoveOneConnection(ConnHandle);

        if (NFCSTATUS_SUCCESS == status)
        {
            PH_LOG_SNEP_INFO_STR("**** Server Connection Removed SUCCESS ****");

        } else
        {
            PH_LOG_SNEP_CRIT_STR("**** Server Connection Removed FAILED ****");
        }
    }
}

static void
phLibNfc_SnepLlcp_ListenSocket_Cb(void *pContext, phLibNfc_Handle hIncomingSocket)
{
    phLibNfc_SnepServerSession_t *pServerSession = (phLibNfc_SnepServerSession_t*)pContext;
    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepLlcp_ListenSocket_Cb:");

    if (NULL != pServerSession)
    {
        if (phLibNfc_SnepServer_Initialized == pServerSession->Server_state &&
            NULL != pServerSession->pConnectionCb)
        {
            pServerSession->pConnectionCb(pServerSession->pListenContext,
                                        NFCSTATUS_INCOMING_CONNECTION,
                                        hIncomingSocket);
        }
    }
}

static void
phLibNfc_SnepLlcp_ConnectSocket_Cb(void *pContext, uint8_t nErrCode, NFCSTATUS status)
{
    phLibNfc_SnepClientSession_t *pClientSession = (phLibNfc_SnepClientSession_t*)pContext;
    phLibNfc_Llcp_sSocketOptions_t sRemoteOptions;
    UNREFERENCED_PARAMETER(nErrCode);
    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepLlcp_ConnectSocket_Cb:");

    if (NFCSTATUS_SUCCESS == status)
    {
        status = NFCSTATUS_CONNECTION_FAILED;
        if (NULL != pClientSession)
        {
            if (phLibNfc_SnepClient_Initialized == pClientSession->Client_state &&
                NULL != pClientSession->pConnectionCb)
            {
                status = NFCSTATUS_CONNECTION_SUCCESS;
            }
        }
    }
    else
    {
        status = NFCSTATUS_CONNECTION_FAILED;
    }

    if (NULL != pClientSession &&
        NULL != pClientSession->pConnectionCb)
    {
        if (NFCSTATUS_CONNECTION_SUCCESS == status &&
            NFCSTATUS_SUCCESS == phLibNfc_Llcp_SocketGetRemoteOptions(pClientSession->hRemoteDevHandle, 
                                                                      pClientSession->hSnepClientHandle, 
                                                                      &sRemoteOptions))
        {
            pClientSession->iRemoteMiu = sRemoteOptions.miu;
        }
        else
        {
            pClientSession->iRemoteMiu = PHFRINFC_LLCP_MIU_DEFAULT;
        }

        pClientSession->pConnectionCb(pClientSession->pClientContext,
                                      status,
                                      pClientSession->hSnepClientHandle);
    }
}

static void
phLibNfc_SnepLlcp_AcceptSocket_Cb(void *pContext, NFCSTATUS status)
{
    phLibNfc_SnepServerConnection_t *pServerConn = phLibNfc_SnepServer_GetConnectionContext((phLibNfc_Handle)pContext);
    phLibNfc_SnepServerSession_t *pServerSession = NULL;
    phLibNfc_Llcp_sSocketOptions_t sRemoteOptions;

    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepLlcp_AcceptSocket_Cb:");

    if (NULL != pServerConn &&
        NFCSTATUS_SUCCESS == status)
    {
        pServerSession = phLibNfc_SnepServer_GetSessionByConnection(pServerConn->hSnepServerConnHandle);

        if (NULL != pServerSession &&
            NULL != pServerSession->pConnectionCb)
        {
            status = NFCSTATUS_CONNECTION_SUCCESS;
        }
        else
        {
            status = NFCSTATUS_CONNECTION_FAILED;
        }

    }
    else
    {
        status = NFCSTATUS_CONNECTION_FAILED;
    }

    if (NULL != pServerSession &&
        NULL != pServerSession->pConnectionCb)
    {
        if (NFCSTATUS_SUCCESS == phLibNfc_Llcp_SocketGetRemoteOptions(pServerConn->hRemoteDevHandle, 
                                                                      pServerConn->hSnepServerConnHandle, 
                                                                      &sRemoteOptions))
        {
            pServerConn->iRemoteMiu = sRemoteOptions.miu;
        }
        else
        {
            pServerConn->iRemoteMiu = PHFRINFC_LLCP_MIU_DEFAULT;
        }

        pServerConn->ServerConnectionState = phLibNfc_SnepServer_Initialized;
        pServerSession->pConnectionCb(pServerConn->pConnectionContext, status,
                                    pServerConn->hSnepServerConnHandle);

        status = phLibNfc_Llcp_Recv(pServerConn->hRemoteDevHandle,
                                    pServerConn->hSnepServerConnHandle,
                                    pServerConn->responseDataContext.pProcessingBuffer,
                                    SnepSocketRecvCbForServer,
                                    (void*) pServerConn);
        if (NFCSTATUS_PENDING == status)
        {
            PH_LOG_SNEP_INFO_STR("\n ACCEPT CALLBACK SUCCESS");
        }
    }
}

/* ---------------------------- Public functions ------------------------------- */

NFCSTATUS phLibNfc_SnepServer_Init(phLibNfc_SnepConfig_t *pConfigInfo,
                                    pphLibNfc_SnepConn_ntf_t pConnCb,
                                    phLibNfc_Handle *pServerHandle,
                                    void *pContext){
    NFCSTATUS status;
    phNfc_sData_t ServerName = {DefaultServerName,SNEP_DEFAULT_SERVER_NAME_LEN};
    phLibNfc_SnepServerSession_t *pServerSession = NULL;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    if (NULL == pConfigInfo ||
        NULL == pConnCb ||
        NULL == pServerHandle ||
        MIN_MIU_VAL > pConfigInfo->sOptions.miu)
    {
        return NFCSTATUS_INVALID_PARAMETER;
    }

    if(FALSE == phLibNfc_SnepServer_InitServerContext())
    {
        return NFCSTATUS_INSUFFICIENT_RESOURCES;
    }

    /*TODO Check LLCP MIU validity */
    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if (NULL != pServerContext)
    {
        pServerSession = phLibNfc_SnepServer_AddSession();

        if (NULL != pServerSession)
        {
            pServerSession->sWorkingBuffer.buffer = (uint8_t*)phOsalNfc_GetMemory((pConfigInfo->sOptions.miu *pConfigInfo->sOptions.rw)+ pConfigInfo->sOptions.miu);
            if (NULL != pServerSession->sWorkingBuffer.buffer)
            {
                pServerSession->sWorkingBuffer.length = ((pConfigInfo->sOptions.miu *pConfigInfo->sOptions.rw)+ pConfigInfo->sOptions.miu);
                status = phLibNfc_Llcp_Socket(phFriNfc_LlcpTransport_eConnectionOriented,
                                            &pConfigInfo->sOptions, &pServerSession->sWorkingBuffer,
                                            &pServerSession->hSnepServerHandle,
                                            phLibNfc_SnepLlcp_SocketErr_Cb,
                                            (void *)pServerSession);
            }
            else
            {
                status = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }

    }
    else
    {
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
    }

    if (NFCSTATUS_SUCCESS == status)
    {
        if (phLibNfc_SnepServer_Default == pConfigInfo->SnepServerType)
        {
           pServerSession->SnepServerSap = SNEP_DEFAULT_SERVER_SAP;
           /* Bind Socket */
            status = phLibNfc_Llcp_Bind( pServerSession->hSnepServerHandle,
                                        pServerSession->SnepServerSap,
                                        &ServerName);
        }
        else
        {
            /* TODO Server sap should be dynamically selected for non default server
             * Currently there is a limitation of one default and one non default server */
            pServerSession->SnepServerSap = SNEP_NON_DEFAULT_SERVER_SAP;
            /* Bind Socket */
            status = phLibNfc_Llcp_Bind( pServerSession->hSnepServerHandle,
                                        pServerSession->SnepServerSap,
                                        pConfigInfo->SnepServerName);
        }

    }

    if (NFCSTATUS_SUCCESS == status)
    {
        if (phLibNfc_SnepServer_Default == pConfigInfo->SnepServerType &&
            NULL == pConfigInfo->SnepServerName)
        {
            status = phLibNfc_Llcp_Listen(pServerSession->hSnepServerHandle,
                                        phLibNfc_SnepLlcp_ListenSocket_Cb, (void*)pServerSession);
            pServerSession->SnepServerType = phLibNfc_SnepServer_Default;
        }
        else if (phLibNfc_SnepServer_NonDefault == pConfigInfo->SnepServerType &&
                 NULL != pConfigInfo->SnepServerName)
        {
            status = phLibNfc_Llcp_Listen(pServerSession->hSnepServerHandle,
                                        phLibNfc_SnepLlcp_ListenSocket_Cb, (void*)pServerSession);
            pServerSession->SnepServerType = phLibNfc_SnepServer_NonDefault;
        }
        else
        {
            status = NFCSTATUS_INVALID_PARAMETER;
        }
        if (NFCSTATUS_SUCCESS == status)
        {
            pServerSession->pConnectionCb = pConnCb;
            pServerSession->Server_state = phLibNfc_SnepServer_Initialized;
            *pServerHandle = pServerSession->hSnepServerHandle;
            pServerSession->pListenContext = pContext;
            pServerSession->SnepServerVersion =SNEP_VERSION;
        }
    }
    else
    {
        /*TODO Deallocate the Server Session and Server Context */
    }

    return status;
}


NFCSTATUS phLibNfc_SnepServer_Accept( phLibNfc_Data_t *pDataInbox,
                                     phLibNfc_Llcp_sSocketOptions_t *pSockOps,
                                     phLibNfc_Handle hRemoteDevHandle,
                                     phLibNfc_Handle ServerHandle,
                                     phLibNfc_Handle ConnHandle,
                                     pphLibNfc_SnepPut_ntf_t pPutNtfCb,
                                     pphLibNfc_SnepGet_ntf_t pGetNtfCb,
                                     void *pContext ){

    NFCSTATUS status;
    phLibNfc_SnepServerConnection_t *pServerConn = NULL;
    phLibNfc_SnepServerSession_t *pServerSession = NULL;

    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepServer_Accept:");

    if (NULL == pDataInbox ||
        NULL == pSockOps ||
        MIN_MIU_VAL > pSockOps->miu)
    {
        return NFCSTATUS_INVALID_PARAMETER;
    }

    pServerConn = phLibNfc_SnepServer_AddConnection(ServerHandle);
    pServerSession = phLibNfc_SnepServer_GetSessionContext(ServerHandle);

    if (NULL != pServerConn)
    {
        if( pDataInbox->buffer != NULL )
        {
            pServerConn->iInboxSize = pDataInbox->length;
            if (phLibNfc_SnepServer_Default == pServerSession->SnepServerType &&
                MIN_INBOX_SIZE > pServerConn->iInboxSize)
            {
                return NFCSTATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            return NFCSTATUS_INVALID_PARAMETER;
        }
        pServerConn->hSnepServerConnHandle = ConnHandle;
        pServerConn->hRemoteDevHandle = hRemoteDevHandle;
        pServerConn->pDataInbox = pDataInbox;
        pServerConn->pPutNtfCb = pPutNtfCb;
        pServerConn->pGetNtfCb = pGetNtfCb;
        pServerConn->pConnectionContext = pContext;
        if( pDataInbox != NULL )
        {
            pServerConn->iInboxSize = pDataInbox->length;
         }
        pServerConn->iMiu = pSockOps->miu;
        pServerConn->ServerConnectionState = phLibNfc_SnepServer_Uninitialized;
        pServerConn->SnepServerVersion =SNEP_VERSION;

        pServerConn->responseDataContext.pProcessingBuffer = (phNfc_sData_t*)phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
        if (NULL != pServerConn->responseDataContext.pProcessingBuffer)
        {
            pServerConn->responseDataContext.pProcessingBuffer->buffer = (uint8_t*)phOsalNfc_GetMemory(pSockOps->miu);
            if (NULL != pServerConn->responseDataContext.pProcessingBuffer->buffer)
            {
                pServerConn->responseDataContext.pProcessingBuffer->length = pSockOps->miu;
            }
            else
            {
                return NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            return NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
        pServerConn->sConnWorkingBuffer.buffer = (uint8_t*)phOsalNfc_GetMemory((pSockOps->miu) * (pSockOps->rw)+pSockOps->miu);
        if (NULL != pServerConn->sConnWorkingBuffer.buffer)
        {
            pServerConn->sConnWorkingBuffer.length = ((pSockOps->miu) * (pSockOps->rw)+pSockOps->miu);
            status = phLibNfc_Llcp_Accept(ConnHandle,
                                        pSockOps,
                                        &pServerConn->sConnWorkingBuffer,
                                        phLibNfc_SnepLlcp_AcceptSocketErr_Cb,
                                        phLibNfc_SnepLlcp_AcceptSocket_Cb,
                                        (PVOID)ConnHandle);
        } 
        else
        {
            status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NFCSTATUS phLibNfc_SnepServer_DeInit(phLibNfc_Handle ServerHandle){

    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_SnepServerContext_t *pServerContext = NULL;

    /* Context should not be allocated if already NULL */
    pServerContext = phLibNfc_SnepServer_GetServerContext();

    if (NULL != pServerContext)
    {
        phLibNfc_SnepServer_RemoveSession(ServerHandle);
        if (0 == pServerContext->CurrentServerCnt)
            {
                phLibNfc_SnepServer_RemoveServerContext();
            }
    }
    else
    {
        status = NFCSTATUS_NOT_INITIALISED;
    }
    return status;
}

/*----------------------- SNEP CLIENT ---------------------------------*/

NFCSTATUS phLibNfc_SnepClient_Init( phLibNfc_SnepConfig_t *pConfigInfo,
                                   phLibNfc_Handle hRemDevHandle,
                                   pphLibNfc_SnepConn_ntf_t pConnClientCb,
                                   void *pContext){
    NFCSTATUS status;
    phNfc_sData_t ServerName = {DefaultServerName,SNEP_DEFAULT_SERVER_NAME_LEN};
    phLibNfc_SnepClientSession_t *pClientSession = NULL;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    if (NULL == pConfigInfo ||
        NULL == pConnClientCb ||
        MIN_MIU_VAL > pConfigInfo->sOptions.miu)
    {
        return NFCSTATUS_INVALID_PARAMETER;
    }

    if (FALSE == phLibNfc_SnepServer_InitClientContext())
    {
        return NFCSTATUS_INSUFFICIENT_RESOURCES;
    }

    /* TODO Check LLCP MIU validity */
    pClientContext = phLibNfc_SnepClient_GetClientContext();

    if (NULL != pClientContext)
    {
        pClientSession = phLibNfc_SnepClient_AddSession();

        if (NULL != pClientSession)
        {
            pClientSession->sWorkingBuffer.buffer = (uint8_t*)phOsalNfc_GetMemory((pConfigInfo->sOptions.miu) *(pConfigInfo->sOptions.rw)+pConfigInfo->sOptions.miu);
            if (NULL != pClientSession->sWorkingBuffer.buffer)
            {
                pClientSession->sWorkingBuffer.length = ((pConfigInfo->sOptions.miu *pConfigInfo->sOptions.rw)+pConfigInfo->sOptions.miu);
                /*Create Socket*/
                status = phLibNfc_Llcp_Socket(phFriNfc_LlcpTransport_eConnectionOriented,
                                            &pConfigInfo->sOptions, &pClientSession->sWorkingBuffer,
                                            &pClientSession->hSnepClientHandle,
                                            phLibNfc_SnepLlcp_SocketErr_Cb,
                                            (void *)pClientSession);
            }
            else
            {
                status = NFCSTATUS_INSUFFICIENT_RESOURCES;
                phLibNfc_SnepClient_RemoveIncompleteSession(pClientSession);
            }
        }
        else
        {
            status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
    }
    if (NFCSTATUS_SUCCESS == status)
    {
        pClientSession->Client_state = phLibNfc_SnepClient_Initialized;
        pClientSession->pConnectionCb = pConnClientCb;
        pClientSession->pClientContext = pContext;
        pClientSession->iMiu = pConfigInfo->sOptions.miu;
        pClientSession->SnepClientVersion = SNEP_VERSION;
        pClientSession->hRemoteDevHandle = hRemDevHandle;
        if (phLibNfc_SnepServer_Default == pConfigInfo->SnepServerType &&
            NULL == pConfigInfo->SnepServerName)
        {
            status = phLibNfc_Llcp_ConnectByUri(hRemDevHandle,
                                                pClientSession->hSnepClientHandle,
                                                &ServerName,
                                                phLibNfc_SnepLlcp_ConnectSocket_Cb,
                                                (void*)pClientSession);
        }
        else if (phLibNfc_SnepServer_NonDefault == pConfigInfo->SnepServerType &&
            NULL != pConfigInfo->SnepServerName)
        {
            status = phLibNfc_Llcp_ConnectByUri(hRemDevHandle,
                                                pClientSession->hSnepClientHandle,
                                                pConfigInfo->SnepServerName,
                                                phLibNfc_SnepLlcp_ConnectSocket_Cb,
                                                (void*)pClientSession);
        }

        if (NFCSTATUS_PENDING != status)
        {
            phLibNfc_SnepClient_RemoveIncompleteSession(pClientSession);
        }

    } 
    else
    {
        /*TODO Deallocate the Server Session and Server Context */
        phLibNfc_SnepClient_RemoveIncompleteSession(pClientSession);
    }

    PH_LOG_SNEP_INFO_STR("phLibNfc_SnepClient_Init:");

    return status;

}

NFCSTATUS phLibNfc_SnepClient_DeInit( phLibNfc_Handle ClientHandle){

    NFCSTATUS status;
    phLibNfc_SnepClientContext_t *pClientContext = NULL;

    /* Context should not be allocated if already NULL */
    pClientContext = phLibNfc_SnepClient_GetClientContext();

    if (NULL != pClientContext)
    {
        if (NULL != ClientHandle)
        {
            status = phLibNfc_SnepClient_RemoveSession(ClientHandle);

            if (0 == pClientContext->CurrentClientCnt)
            {
                phLibNfc_SnepClient_RemoveClientContext();
            }
            if (NFCSTATUS_SUCCESS != status)
            {
                status = NFCSTATUS_FAILED;
            }
        }
        else
        {
            status = phLibNfc_SnepClient_RemoveAllSessions();
        }
    } 
    else
    {
        status = NFCSTATUS_NOT_INITIALISED;
    }
    return status;
}
