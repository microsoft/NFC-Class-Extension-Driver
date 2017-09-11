/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include <phFriNfc_Llcp.h>
#include <phFriNfc_LlcpTransport.h>

#include "phLibNfc_Llcp.tmh"

static
NFCSTATUS static_CheckState();

static
NFCSTATUS static_CheckDevice(phLibNfc_Handle hRemoteDevice);

static
void phLibNfc_Llcp_CheckLlcp_Cb(void *pContext,NFCSTATUS status);

static
void phLibNfc_Llcp_Link_Cb(void *pContext,phLibNfc_Llcp_eLinkStatus_t status);

/* --------------------------- Internal functions ------------------------------ */

static NFCSTATUS static_CheckState()
{
    PH_LOG_LLCP_FUNC_ENTRY();
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   if(pLibContext == NULL)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_NOT_INITIALISED;
   }

   if(pLibContext->StateContext.CurrState < phLibNfc_StateInit)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_NOT_INITIALISED;
   }

   if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_SHUTDOWN;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}

static NFCSTATUS static_CheckDevice(phLibNfc_Handle hRemoteDevice)
{
   phLibNfc_sRemoteDevInformation_t*   psRemoteDevInfo = (phLibNfc_sRemoteDevInformation_t*)hRemoteDevice;
   NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
   phLibNfc_Handle pHandle;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   if ((void *) hRemoteDevice == NULL)
   {
       PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* If local device is the Initiator (remote is Target), check if connection is correct */
   if (psRemoteDevInfo->RemDevType == phHal_eNfcIP1_Target)
   {
      if(pLibContext->Connected_handle == NULL)
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_TARGET_NOT_CONNECTED;
      }

      wStatus = phLibNfc_GetConnectedHandle(&pHandle);

      if(hRemoteDevice != (pHandle))
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_INVALID_HANDLE;
      }
   }

   /* Check if previous callback is pending or if remote peer is not LLCP compliant */
   if ((pLibContext->status.GenCb_pending_status == TRUE) ||
            (pLibContext->llcp_cntx.bIsLlcp == FALSE))
   {
       PH_LOG_LLCP_FUNC_EXIT();
       return NFCSTATUS_REJECTED;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}

/* ---------------------------- Public functions ------------------------------- */

NFCSTATUS phLibNfc_Mgt_SetLlcp_ConfigParams( phLibNfc_Llcp_sLinkParameters_t* pConfigInfo,
                                             pphLibNfc_RspCb_t                pConfigRspCb,
                                             void*                            pContext
                                            )
{
   NFCSTATUS      result;
   phNfc_sData_t  sGeneralBytesBuffer;
   phLibNfc_sNfcIPCfg_t sNfcIPCfg;
   const uint8_t  pMagicBuffer[] = { 0x46, 0x66, 0x6D };
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((pConfigInfo == NULL) || (pConfigRspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Save the config for later use */
   phOsalNfc_MemCopy( &pLibContext->llcp_cntx.sLocalParams,
           pConfigInfo,
           sizeof(phLibNfc_Llcp_sLinkParameters_t) );

   /* Copy magic number in NFCIP General Bytes */
   phOsalNfc_MemCopy(sNfcIPCfg.generalBytes, pMagicBuffer, sizeof(pMagicBuffer));
   sNfcIPCfg.generalBytesLength = sizeof(pMagicBuffer);

   /* Encode link parameters in TLV to configure P2P General Bytes */
   sGeneralBytesBuffer.buffer = sNfcIPCfg.generalBytes + sizeof(pMagicBuffer);
   sGeneralBytesBuffer.length = sizeof(sNfcIPCfg.generalBytes) - sizeof(pMagicBuffer);
   result = phFriNfc_Llcp_EncodeLinkParams( &sGeneralBytesBuffer,
                                            pConfigInfo,
                                            PHFRINFC_LLCP_VERSION);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTATUS(result);
   }
   sNfcIPCfg.generalBytesLength += (uint8_t)sGeneralBytesBuffer.length;

   /* Set the P2P general bytes */
   result = phLibNfc_Mgt_SetP2P_ConfigParams(&sNfcIPCfg, pConfigRspCb, pContext);
   if (result != NFCSTATUS_PENDING)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTATUS(result);
   }

   /* Resets the LLCP LLC component */
   result = phFriNfc_Llcp_Reset( &pLibContext->llcp_cntx.sLlcpContext,
                                 pLibContext->psOverHalCtxt,
                                 pConfigInfo,
                                 pLibContext->llcp_cntx.pRxBuffer,
                                 sizeof(pLibContext->llcp_cntx.pRxBuffer),
                                 pLibContext->llcp_cntx.pTxBuffer,
                                 sizeof(pLibContext->llcp_cntx.pTxBuffer),
                                 pLibContext->bDtaFlag,
                                 phLibNfc_Llcp_Link_Cb,
                                 pLibContext);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTATUS(result);
   }

   /* Resets the LLCP Transport component */
   result = phFriNfc_LlcpTransport_Reset( &pLibContext->llcp_cntx.sLlcpTransportContext,
                                          &pLibContext->llcp_cntx.sLlcpContext );
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTATUS(result);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_PENDING;
}

NFCSTATUS phLibNfc_Llcp_CheckLlcp( phLibNfc_Handle              hRemoteDevice,
                                   pphLibNfc_ChkLlcpRspCb_t     pCheckLlcp_RspCb,
                                   pphLibNfc_LlcpLinkStatusCb_t pLink_Cb,
                                   void*                        pContext
                                   )
{
   NFCSTATUS                           result;
   phLibNfc_sRemoteDevInformation_t*   psRemoteDevInfo = (phLibNfc_sRemoteDevInformation_t*)hRemoteDevice;
   phLibNfc_Handle ConnHandle = 0;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)       ||
       (pCheckLlcp_RspCb == NULL) ||
       (pLink_Cb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* If local device is the Initiator (remote is Target), check if connection is correct */
   if (psRemoteDevInfo->RemDevType == phHal_eNfcIP1_Target)
   {
      if(pLibContext->Connected_handle == NULL)
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_TARGET_NOT_CONNECTED;
      }

      /* Check if handle corresponds to connected one */
      result = phLibNfc_GetConnectedHandle(&ConnHandle);
      if(hRemoteDevice != ConnHandle)
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_INVALID_HANDLE;
      }
   }

   /* Prepare callback */
   pLibContext->CBInfo.pClientLlcpLinkCb = pLink_Cb;
   pLibContext->CBInfo.pClientLlcpLinkCntx = pContext;

   // DEBUG: Reset at least the state
   pLibContext->llcp_cntx.sLlcpContext.state = 0;

   /* Prepare callback */
   pLibContext->CBInfo.pClientLlcpCheckRespCb = pCheckLlcp_RspCb;
   pLibContext->CBInfo.pClientLlcpCheckRespCntx = pContext;

   /* Call the component function */
   result = phFriNfc_Llcp_ChkLlcp( &pLibContext->llcp_cntx.sLlcpContext,
                                   psRemoteDevInfo,
                                   phLibNfc_Llcp_CheckLlcp_Cb,
                                   pLibContext
                                   );
   result = PHNFCSTATUS(result);
   if (result == NFCSTATUS_PENDING)
   {
      pLibContext->status.GenCb_pending_status = TRUE;
   }
   else if (result == NFCSTATUS_SUCCESS)
   {
      /* Nothing to do */
   }
   else if (result != NFCSTATUS_FAILED)
   {
      result = NFCSTATUS_TARGET_LOST;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

static
void phLibNfc_Llcp_Link_Cb(void *pContext, phLibNfc_Llcp_eLinkStatus_t status)
{
   phLibNfc_LibContext_t         *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)pContext;
   pphLibNfc_LlcpLinkStatusCb_t  pClientCb = NULL;
   void                          *pClientContext = NULL;
   phLibNfc_LlcpInfo_t           *pllcp_cntx = &pLibNfc_Ctxt->llcp_cntx;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(pLibNfc_Ctxt != phLibNfc_GetContext())
   {
       PH_LOG_LLCP_WARN_STR("Invalid Lib context!");
      phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
   }
   else
   {
      PH_LOG_LLCP_INFO_STR("Close all sockets");
      /* Close all sockets */
      if (!pllcp_cntx->sLlcpContext.bDtaFlag || phFriNfc_LlcpMac_eLinkDeactivated == status)
      {
          phFriNfc_LlcpTransport_CloseAll(&pLibNfc_Ctxt->llcp_cntx.sLlcpTransportContext);
      }

      /* Copy callback details */
      pClientCb = pLibNfc_Ctxt->CBInfo.pClientLlcpLinkCb;
      pClientContext = pLibNfc_Ctxt->CBInfo.pClientLlcpLinkCntx;

      if(pClientCb != NULL)
      {
         PH_LOG_LLCP_INFO_X32MSG("Invoking Client Llcp link call back with status:",status);
         pClientCb(pClientContext, status);
      }
   }
}
static NFCSTATUS phLibNfc_IsAborted(phLibNfc_State_t State)
{
    NFCSTATUS Status = NFCSTATUS_SUCCESS;
    PH_LOG_LLCP_FUNC_ENTRY();
    if(State < phLibNfc_StateDiscovered)
    {
        Status = NFCSTATUS_ABORTED;
    }
    PH_LOG_LLCP_FUNC_EXIT();
    return Status;
}


static
void phLibNfc_Llcp_CheckLlcp_Cb(void *pContext, NFCSTATUS status)
{
    phLibNfc_LibContext_t      *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)pContext;
    NFCSTATUS                  RetStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_ChkLlcpRspCb_t   pClientCb = NULL;
    void                       *pClientContext = NULL;
    phLibNfc_State_t State;

    PH_LOG_LLCP_FUNC_ENTRY();
    if(pLibNfc_Ctxt != phLibNfc_GetContext())
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        State = phLibNfc_GetState(pLibNfc_Ctxt);
        if(phLibNfc_StateReset == State)
        {
            RetStatus = NFCSTATUS_SHUTDOWN;
        }
        else
        {
            RetStatus = phLibNfc_IsAborted(State);
            if (NFCSTATUS_ABORTED != RetStatus)
            {
                if(status == NFCSTATUS_SUCCESS)
                {
                    /* Remote peer is LLCP compliant */
                    pLibNfc_Ctxt->llcp_cntx.bIsLlcp = TRUE;
                }
                else if(PHNFCSTATUS(status)== NFCSTATUS_FAILED)
                {
                    RetStatus = NFCSTATUS_FAILED;
                    pLibNfc_Ctxt->llcp_cntx.bIsLlcp = FALSE;
                }
                else
                {
                    RetStatus = NFCSTATUS_TARGET_LOST;
                }
            }
        }
        /* Update the current state */
        pLibNfc_Ctxt->status.GenCb_pending_status = FALSE;

        /* Copy callback details */
        pClientCb = pLibNfc_Ctxt->CBInfo.pClientLlcpCheckRespCb;
        pClientContext = pLibNfc_Ctxt->CBInfo.pClientLlcpCheckRespCntx;

        /* Reset saved callback */
        pLibNfc_Ctxt->CBInfo.pClientCkNdefCb = NULL;
        pLibNfc_Ctxt->CBInfo.pClientCkNdefCntx = NULL;

        if(pClientCb != NULL)
        {
            pClientCb(pClientContext,RetStatus);
        }
    }
    PH_LOG_LLCP_FUNC_EXIT();
}

NFCSTATUS phLibNfc_Llcp_Activate( phLibNfc_Handle hRemoteDevice )
{
    NFCSTATUS result;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LLCP_FUNC_ENTRY();

    result = static_CheckState();
    if (result != NFCSTATUS_SUCCESS)
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return result;
    }

    if (hRemoteDevice == 0)
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return NFCSTATUS_INVALID_PARAMETER;
    }

    result = static_CheckDevice(hRemoteDevice);
    if (result != NFCSTATUS_SUCCESS)
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return result;
    }

    /* Start activation */
    result = phFriNfc_Llcp_Activate(&pLibContext->llcp_cntx.sLlcpContext);

    PH_LOG_LLCP_FUNC_EXIT();
    return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Deactivate( phLibNfc_Handle  hRemoteDevice )
{
   NFCSTATUS result;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if (hRemoteDevice == 0)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Start deactivation */
   result = phFriNfc_Llcp_Deactivate(&pLibContext->llcp_cntx.sLlcpContext);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_GetLocalInfo( phLibNfc_Handle                  hRemoteDevice,
                                      phLibNfc_Llcp_sLinkParameters_t* pConfigInfo
                                      )
{
   NFCSTATUS result;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   UNUSED(hRemoteDevice);
   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if (pConfigInfo == NULL)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Get local infos */
   result = phFriNfc_Llcp_GetLocalInfo(&pLibContext->llcp_cntx.sLlcpContext, pConfigInfo);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_GetRemoteInfo( phLibNfc_Handle                    hRemoteDevice,
                                       phLibNfc_Llcp_sLinkParameters_t*   pConfigInfo
                                       )
{
   NFCSTATUS result;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0) ||
       (pConfigInfo == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Get local infos */
   result = phFriNfc_Llcp_GetRemoteInfo(&pLibContext->llcp_cntx.sLlcpContext, pConfigInfo);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_DiscoverServices( phLibNfc_Handle     hRemoteDevice,
                                          phNfc_sData_t       *psServiceNameList,
                                          uint8_t             *pnSapList,
                                          uint8_t             nListSize,
                                          pphLibNfc_RspCb_t   pDiscover_Cb,
                                          void                *pContext
                                          )
{
   NFCSTATUS                           result;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
   PHNFC_UNUSED_VARIABLE(hRemoteDevice);

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)       ||
       (psServiceNameList == NULL) ||
       (pnSapList == NULL) ||
       (nListSize == 0) ||
       (pDiscover_Cb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Prepare callback */
   pLibContext->CBInfo.pClientLlcpDiscoveryCb = pDiscover_Cb;
   pLibContext->CBInfo.pClientLlcpDiscoveryCntx = pContext;

   /* Call the component function */
   result = phFriNfc_LlcpTransport_DiscoverServices( &pLibContext->llcp_cntx.sLlcpTransportContext,
                                                     psServiceNameList,
                                                     pnSapList,
                                                     nListSize,
                                                     pDiscover_Cb,
                                                     pContext
                                                     );
   result = PHNFCSTATUS(result);
   if ((result == NFCSTATUS_PENDING) || (result == NFCSTATUS_SUCCESS))
   {
      /* Nothing to do */
   }
   else if (result != NFCSTATUS_FAILED)
   {
      result = NFCSTATUS_TARGET_LOST;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

NFCSTATUS phLibNfc_Llcp_Socket( phLibNfc_Llcp_eSocketType_t      eType,
                                phLibNfc_Llcp_sSocketOptions_t*  psOptions,
                                phNfc_sData_t*                   psWorkingBuffer,
                                phLibNfc_Handle*                 phSocket,
                                pphLibNfc_LlcpSocketErrCb_t      pErr_Cb,
                                void*                            pContext
                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   
   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* NOTE: Transport Layer test psOption and psWorkingBuffer value */
   if ((phSocket == NULL)        ||
       (pErr_Cb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Get local infos */
   result = phFriNfc_LlcpTransport_Socket(&pLibContext->llcp_cntx.sLlcpTransportContext,
                                          eType,
                                          psOptions,
                                          psWorkingBuffer,
                                          &psSocket,
                                          pErr_Cb,
                                          pContext);
    if (NFCSTATUS_SUCCESS == result)
    {
        /* Send back the socket handle */
        *phSocket = (phLibNfc_Handle)psSocket;
    }
    else
    {
        *phSocket = NULL;
    }

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Close( phLibNfc_Handle hSocket )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if (hSocket == 0)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Get local infos */
   /* TODO: if connected abort and close else close only */
   result = phFriNfc_LlcpTransport_Close(psSocket);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_SocketGetLocalOptions( phLibNfc_Handle                  hSocket,
                                               phLibNfc_Llcp_sSocketOptions_t*  psLocalOptions
                                               )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hSocket == 0) ||
       (psLocalOptions == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Get local options */
   result = phFriNfc_LlcpTransport_SocketGetLocalOptions(psSocket, psLocalOptions);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_SocketGetRemoteOptions( phLibNfc_Handle                  hRemoteDevice,
                                                phLibNfc_Handle                  hSocket,
                                                phLibNfc_Llcp_sSocketOptions_t*  psRemoteOptions
                                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0) ||
       (hSocket == 0)       ||
       (psRemoteOptions == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Get remote infos */
   result = phFriNfc_LlcpTransport_SocketGetRemoteOptions(psSocket, psRemoteOptions);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Bind( phLibNfc_Handle hSocket,
                              uint8_t         nSap,
                              phNfc_sData_t * psServiceName
                              )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* State checking */
   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Parameters checking */
   if (hSocket == 0)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Bind the socket to the designated port */
   result = phFriNfc_LlcpTransport_Bind(psSocket, nSap, psServiceName);

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Listen( phLibNfc_Handle                  hSocket,
                                pphLibNfc_LlcpSocketListenCb_t   pListen_Cb,
                                void*                            pContext
                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* NOTE : psServiceName may be NULL, do not test it ! */
   if ((hSocket == 0) ||
       (pListen_Cb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Start listening for incoming connections */
   result = phFriNfc_LlcpTransport_Listen( psSocket,
                                           (pphFriNfc_LlcpTransportSocketListenCb_t)pListen_Cb,
                                           pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Accept( phLibNfc_Handle                  hSocket,
                                phLibNfc_Llcp_sSocketOptions_t*  psOptions,
                                phNfc_sData_t*                   psWorkingBuffer,
                                pphLibNfc_LlcpSocketErrCb_t      pErr_Cb,
                                pphLibNfc_LlcpSocketAcceptCb_t   pAccept_RspCb,
                                void*                            pContext
                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hSocket == 0)            ||
       (psOptions == NULL)       ||
       (psWorkingBuffer == NULL) ||
       (pErr_Cb == NULL)         ||
       (pAccept_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Accept incoming connection */
   result = phFriNfc_LlcpTransport_Accept( psSocket,
                                           psOptions,
                                           psWorkingBuffer,
                                           pErr_Cb,
                                           pAccept_RspCb,
                                           pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Reject( phLibNfc_Handle                  hRemoteDevice,
                                phLibNfc_Handle                  hSocket,
                                pphLibNfc_LlcpSocketRejectCb_t   pReject_RspCb,
                                void*                            pContext
                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)      ||
       (hSocket == 0)            ||
       (pReject_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Reject incoming connection */
   result = phFriNfc_LlcpTransport_Reject( psSocket,
                                           pReject_RspCb,
                                           pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Connect( phLibNfc_Handle                 hRemoteDevice,
                                 phLibNfc_Handle                 hSocket,
                                 uint8_t                         nSap,
                                 pphLibNfc_LlcpSocketConnectCb_t pConnect_RspCb,
                                 void*                           pContext
                                 )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)      ||
       (hSocket == 0)            ||
       (pConnect_RspCb == NULL))
   {
      PH_LOG_LLCP_WARN_STR("phLibNfc_Llcp_Connect NFCSTATUS_INVALID_PARAMETER");
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Try to connect on a remote service, given its SAP */
   result = phFriNfc_LlcpTransport_Connect( psSocket,
                                            nSap,
                                            pConnect_RspCb,
                                            pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_ConnectByUri( phLibNfc_Handle                 hRemoteDevice,
                                      phLibNfc_Handle                 hSocket,
                                      phNfc_sData_t*                  psUri,
                                      pphLibNfc_LlcpSocketConnectCb_t pConnect_RspCb,
                                      void*                           pContext
                                      )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)      ||
       (hSocket == 0)            ||
       (psUri   == NULL)         ||
       (pConnect_RspCb == NULL))
   {
      PH_LOG_LLCP_WARN_STR("phLibNfc_Llcp_ConnectByUri NFCSTATUS_INVALID_PARAMETER");
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
       PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Try to connect on a remote service, using SDP */
   result = phFriNfc_LlcpTransport_ConnectByUri( psSocket,
                                                 psUri,
                                                 pConnect_RspCb,
                                                 pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Disconnect( phLibNfc_Handle                    hRemoteDevice,
                                    phLibNfc_Handle                    hSocket,
                                    pphLibNfc_LlcpSocketDisconnectCb_t pDisconnect_RspCb,
                                    void*                              pContext
                                    )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0) ||
       (hSocket == 0)       ||
       (pDisconnect_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Disconnect a logical link */
   result = phFriNfc_LlcpTransport_Disconnect( psSocket,
                                               pDisconnect_RspCb,
                                               pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Recv( phLibNfc_Handle              hRemoteDevice,
                              phLibNfc_Handle              hSocket,
                              phNfc_sData_t*               psBuffer,
                              pphLibNfc_LlcpSocketRecvCb_t pRecv_RspCb,
                              void*                        pContext
                              )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)   ||
       (hSocket == 0)         ||
       (psBuffer == NULL)     ||
       (pRecv_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Receive data from the logical link */
   result = phFriNfc_LlcpTransport_Recv( psSocket,
                                         psBuffer,
                                         pRecv_RspCb,
                                         pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_RecvFrom( phLibNfc_Handle                   hRemoteDevice,
                                  phLibNfc_Handle                   hSocket,
                                  phNfc_sData_t*                    psBuffer,
                                  pphLibNfc_LlcpSocketRecvFromCb_t  pRecv_Cb,
                                  void*                             pContext
                                  )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)   ||
       (hSocket == 0)         ||
       (psBuffer == NULL)     ||
       (pRecv_Cb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Receive data from the logical link */
   result = phFriNfc_LlcpTransport_RecvFrom( psSocket,
                                             psBuffer,
                                             pRecv_Cb,
                                             pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_Send( phLibNfc_Handle              hRemoteDevice,
                              phLibNfc_Handle              hSocket,
                              phNfc_sData_t*               psBuffer,
                              pphLibNfc_LlcpSocketSendCb_t pSend_RspCb,
                              void*                        pContext
                              )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)   ||
       (hSocket == 0)         ||
       (psBuffer == NULL)     ||
       (pSend_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Send data to the logical link */
   result = phFriNfc_LlcpTransport_Send( psSocket,
                                         psBuffer,
                                         pSend_RspCb,
                                         pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_SendTo( phLibNfc_Handle               hRemoteDevice,
                                phLibNfc_Handle               hSocket,
                                uint8_t                       nSap,
                                phNfc_sData_t*                psBuffer,
                                pphLibNfc_LlcpSocketSendCb_t  pSend_RspCb,
                                void*                         pContext
                                )
{
   NFCSTATUS                        result;
   phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   result = static_CheckState();
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   if ((hRemoteDevice == 0)   ||
       (hSocket == 0)         ||
       (psBuffer == NULL)     ||
       (pSend_RspCb == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   result = static_CheckDevice(hRemoteDevice);
   if (result != NFCSTATUS_SUCCESS)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   /* Send data to the logical link */
   result = phFriNfc_LlcpTransport_SendTo( psSocket,
                                           nSap,
                                           psBuffer,
                                           pSend_RspCb,
                                           pContext );

   PH_LOG_LLCP_FUNC_EXIT();
   return PHNFCSTATUS(result);
}

NFCSTATUS phLibNfc_Llcp_CancelPendingSend( phLibNfc_Handle hRemoteDevice,
                                           phLibNfc_Handle hSocket
                                           )
{
    NFCSTATUS                        result;
    phFriNfc_LlcpTransport_Socket_t  *psSocket = (phFriNfc_LlcpTransport_Socket_t*)hSocket;

    PH_LOG_LLCP_FUNC_ENTRY();

    result = static_CheckState();
    if (result != NFCSTATUS_SUCCESS)
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return result;
    }

    if ((hRemoteDevice == 0) ||
        (hSocket == 0))
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return NFCSTATUS_INVALID_PARAMETER;
    }

    result = static_CheckDevice(hRemoteDevice);
    if (result != NFCSTATUS_SUCCESS)
    {
        PH_LOG_LLCP_FUNC_EXIT();
        return result;
    }

    /* Cancel data at the logical link */
    result = phFriNfc_LlcpTransport_CancelPendingSend(psSocket);

    PH_LOG_LLCP_FUNC_EXIT();
    return PHNFCSTATUS(result);
}
