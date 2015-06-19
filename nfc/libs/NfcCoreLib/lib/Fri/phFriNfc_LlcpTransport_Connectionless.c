/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include "phFriNfc_Pch.h"

#include <phOsalNfc.h>
#include <phNfcStatus.h>
#include <phLibNfc.h>
#include <phNfcLlcpTypes.h>
#include <phFriNfc_LlcpTransport.h>
#include <phFriNfc_Llcp.h>

#include "phFriNfc_LLcpTransport_Connectionless.tmh"

static void phFriNfc_LlcpTransport_Connectionless_SendTo_CB(void*        pContext,
                                                            NFCSTATUS    status);

static void phFriNfc_LlcpTransport_Connectionless_TriggerSendCb(phFriNfc_LlcpTransport_Socket_t  *pLlcpSocket,
                                                                NFCSTATUS                        status)
{
   pphFriNfc_LlcpTransportSocketSendCb_t  pfSendCb;
   void*                                  pSendContext;

   if (pLlcpSocket->pfSocketSend_Cb != NULL)
   {
      pfSendCb = pLlcpSocket->pfSocketSend_Cb;
      pSendContext = pLlcpSocket->pSendContext;

      pLlcpSocket->pfSocketSend_Cb = NULL;
      pLlcpSocket->pSendContext = NULL;

      pfSendCb(pSendContext, status);
   }
}

static void phFriNfc_LlcpTransport_Connectionless_TriggerRecvFromCb(phFriNfc_LlcpTransport_Socket_t  *pLlcpSocket,
                                                                    uint8_t                          ssap,
                                                                    NFCSTATUS                        status)
{
   pphFriNfc_LlcpTransportSocketRecvFromCb_t  pfSocketRecvFrom_Cb;
   void*                                      pRecvContext;

   /* TODO: use phFriNfc_LlcpTransport_Connectionless_TriggerRecvFromCb? */
   if (pLlcpSocket->pfSocketRecvFrom_Cb != NULL)
   {
      pfSocketRecvFrom_Cb = pLlcpSocket->pfSocketRecvFrom_Cb;
      pRecvContext = pLlcpSocket->pRecvContext;

      pLlcpSocket->pfSocketRecvFrom_Cb = NULL;
      pLlcpSocket->pfSocketRecv_Cb = NULL;
      pLlcpSocket->pRecvContext = NULL;

      pfSocketRecvFrom_Cb(pRecvContext, ssap, status);
   }
}

NFCSTATUS phFriNfc_LlcpTransport_Connectionless_HandlePendingOperations(phFriNfc_LlcpTransport_Socket_t *pSocket)
{
   NFCSTATUS status = NFCSTATUS_FAILED;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Check if something is pending and if transport layer is ready to send */
   if ((pSocket->pfSocketSend_Cb != NULL) &&
       (pSocket->psTransport->bSendPending == FALSE))
   {
      /* Fill the psLlcpHeader stuture with the DSAP,PTYPE and the SSAP */
      pSocket->sLlcpHeader.dsap  = pSocket->socket_dSap;
      pSocket->sLlcpHeader.ptype = PHFRINFC_LLCP_PTYPE_UI;
      pSocket->sLlcpHeader.ssap  = pSocket->socket_sSap;

      /* Send to data to the approiate socket */
      status =  phFriNfc_LlcpTransport_LinkSend(pSocket->psTransport,
                                   &pSocket->sLlcpHeader,
                                   NULL,
                                   &pSocket->sSocketSendBuffer,
                                   phFriNfc_LlcpTransport_Connectionless_SendTo_CB,
                                   pSocket);
   }
   else
   {
      /* Cannot send now, retry later */
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

void Handle_Connectionless_IncommingFrame(phFriNfc_LlcpTransport_t      *pLlcpTransport,
                                          phNfc_sData_t                 *psData,
                                          uint8_t                       dsap,
                                          uint8_t                       ssap)
{
   phFriNfc_LlcpTransport_Socket_t * pSocket = NULL;
   uint8_t                           i       = 0;
   uint8_t                           writeIndex;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Look through the socket table for a match */
   for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
   {
      if(pLlcpTransport->pSocketTable[i].socket_sSap == dsap)
      {
         /* Socket found ! */
         pSocket = &pLlcpTransport->pSocketTable[i];

         /* Forward directly to application if a read is pending */
         if (pSocket->bSocketRecvPending == TRUE)
         {
            /* Reset the RecvPending variable */
            pSocket->bSocketRecvPending = FALSE;

            /* Copy the received buffer into the receive buffer */
            phOsalNfc_MemCopy(pSocket->sSocketRecvBuffer->buffer, psData->buffer, psData->length);

            /* Update the received length */
            *pSocket->receivedLength = psData->length;

            /* call the recv callback */
            phFriNfc_LlcpTransport_Connectionless_TriggerRecvFromCb(pSocket, ssap, NFCSTATUS_SUCCESS);
         }
         /* If no read is pending, try to bufferize for later reading */
         else
         {
            if((pSocket->indexRwWrite - pSocket->indexRwRead) < pSocket->localRW)
            {
               writeIndex = pSocket->indexRwWrite % pSocket->localRW;
               /* Save SSAP */
               pSocket->sSocketRwBufferTable[writeIndex].buffer[0] = ssap;
               /* Save UI frame payload */
               phOsalNfc_MemCopy(pSocket->sSocketRwBufferTable[writeIndex].buffer + 1,
                      psData->buffer,
                      psData->length);
               pSocket->sSocketRwBufferTable[writeIndex].length = psData->length + 1;

               /* Update the RW write index */
               pSocket->indexRwWrite++;
            }
            else
            {
               /* Unable to bufferize the packet, drop it */
            }
         }
         break;
      }
   }
}

static void phFriNfc_LlcpTransport_Connectionless_SendTo_CB(void*        pContext,
                                                            NFCSTATUS    status)
{
   phFriNfc_LlcpTransport_Socket_t   *pLlcpSocket = (phFriNfc_LlcpTransport_Socket_t*)pContext;

   /* Reset the SendPending variable */
   pLlcpSocket->bSocketSendPending = FALSE;

   /* Call the send callback */
   phFriNfc_LlcpTransport_Connectionless_TriggerSendCb(pLlcpSocket,status);
}

static void phFriNfc_LlcpTransport_Connectionless_Abort(phFriNfc_LlcpTransport_Socket_t* pLlcpSocket)
{
   PH_LOG_LLCP_FUNC_ENTRY();
   phFriNfc_LlcpTransport_Connectionless_TriggerSendCb(pLlcpSocket, NFCSTATUS_ABORTED);
   phFriNfc_LlcpTransport_Connectionless_TriggerRecvFromCb(pLlcpSocket, 0, NFCSTATUS_ABORTED);
   pLlcpSocket->pAcceptContext = NULL;
   pLlcpSocket->pfSocketAccept_Cb = NULL;
   pLlcpSocket->pListenContext = NULL;
   pLlcpSocket->pfSocketListen_Cb = NULL;
   pLlcpSocket->pConnectContext = NULL;
   pLlcpSocket->pfSocketConnect_Cb = NULL;
   pLlcpSocket->pDisonnectContext = NULL;
   pLlcpSocket->pfSocketDisconnect_Cb = NULL;
   PH_LOG_LLCP_FUNC_EXIT();
}

NFCSTATUS phFriNfc_LlcpTransport_Connectionless_Close(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket)
{
   PH_LOG_LLCP_FUNC_ENTRY();
   /* Reset the pointer to the socket closed */
   pLlcpSocket->eSocket_State                      = phFriNfc_LlcpTransportSocket_eSocketDefault;
   pLlcpSocket->eSocket_Type                       = phFriNfc_LlcpTransport_eDefaultType;
   pLlcpSocket->pContext                           = NULL;
   pLlcpSocket->pSocketErrCb                       = NULL;
   pLlcpSocket->socket_sSap                        = PHFRINFC_LLCP_SAP_DEFAULT;
   pLlcpSocket->socket_dSap                        = PHFRINFC_LLCP_SAP_DEFAULT;
   pLlcpSocket->bSocketRecvPending                 = FALSE;
   pLlcpSocket->bSocketSendPending                 = FALSE;
   pLlcpSocket->bSocketListenPending               = FALSE;
   pLlcpSocket->bSocketDiscPending                 = FALSE;
   pLlcpSocket->RemoteBusyConditionInfo            = FALSE;
   pLlcpSocket->ReceiverBusyCondition              = FALSE;
   pLlcpSocket->socket_VS                          = 0;
   pLlcpSocket->socket_VSA                         = 0;
   pLlcpSocket->socket_VR                          = 0;
   pLlcpSocket->socket_VRA                         = 0;

   phFriNfc_LlcpTransport_Connectionless_Abort(pLlcpSocket);

   phOsalNfc_SetMemory(&pLlcpSocket->sSocketOption, 0x00, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

   if (pLlcpSocket->sServiceName.buffer != NULL) {
       phOsalNfc_FreeMemory(pLlcpSocket->sServiceName.buffer);
   }
   pLlcpSocket->sServiceName.buffer = NULL;
   pLlcpSocket->sServiceName.length = 0;
    PH_LOG_LLCP_INFO_X32MSG("Return status:",NFCSTATUS_SUCCESS);
   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}

NFCSTATUS phFriNfc_LlcpTransport_Connectionless_SendTo(phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
                                                       uint8_t                                     nSap,
                                                       phNfc_sData_t*                              psBuffer,
                                                       pphFriNfc_LlcpTransportSocketSendCb_t       pSend_RspCb,
                                                       void*                                       pContext)
{
   NFCSTATUS status = NFCSTATUS_FAILED;

   PH_LOG_LLCP_FUNC_ENTRY();

   pLlcpSocket->pfSocketSend_Cb = pSend_RspCb;
   pLlcpSocket->pSendContext    = pContext;

   if(pLlcpSocket->psTransport->bSendPending == TRUE)
   {
      /* Save the request so it can be handled in phFriNfc_LlcpTransport_Connectionless_HandlePendingOperations() */
      pLlcpSocket->sSocketSendBuffer = *psBuffer;
      pLlcpSocket->socket_dSap      = nSap;
      status = NFCSTATUS_PENDING;
   }
   else
   {
      /* Fill the psLlcpHeader stuture with the DSAP,PTYPE and the SSAP */
      pLlcpSocket->sLlcpHeader.dsap  = nSap;
      pLlcpSocket->sLlcpHeader.ptype = PHFRINFC_LLCP_PTYPE_UI;
      pLlcpSocket->sLlcpHeader.ssap  = pLlcpSocket->socket_sSap;

      /* Send to data to the approiate socket */
      status =  phFriNfc_LlcpTransport_LinkSend(pLlcpSocket->psTransport,
                                   &pLlcpSocket->sLlcpHeader,
                                   NULL,
                                   psBuffer,
                                   phFriNfc_LlcpTransport_Connectionless_SendTo_CB,
                                   pLlcpSocket);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS phLibNfc_LlcpTransport_Connectionless_RecvFrom(phFriNfc_LlcpTransport_Socket_t                   *pLlcpSocket,
                                                         phNfc_sData_t*                                    psBuffer,
                                                         pphFriNfc_LlcpTransportSocketRecvFromCb_t         pRecv_Cb,
                                                         void                                              *pContext)
{
   NFCSTATUS   status = NFCSTATUS_PENDING;
   uint8_t     readIndex;
   uint8_t     ssap;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket->bSocketRecvPending)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      /* Check if pending packets in RW */
      if(pLlcpSocket->indexRwRead != pLlcpSocket->indexRwWrite)
      {
         readIndex = pLlcpSocket->indexRwRead % pLlcpSocket->localRW;

         if(psBuffer->length < (pLlcpSocket->sSocketRwBufferTable[readIndex].length - 1))
         {
            PH_LOG_LLCP_CRIT_STR("Recv buffer too small! "
                                 "Size: %u Required: %u",
                                 psBuffer->length,
                                 pLlcpSocket->sSocketRwBufferTable[readIndex].length - 1);

            status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_BUFFER_TOO_SMALL);

            goto Done;
         }

         /* Extract ssap and buffer from RW buffer */
         ssap = pLlcpSocket->sSocketRwBufferTable[readIndex].buffer[0];
         phOsalNfc_MemCopy(psBuffer->buffer,
                pLlcpSocket->sSocketRwBufferTable[readIndex].buffer + 1,
                pLlcpSocket->sSocketRwBufferTable[readIndex].length - 1);
         psBuffer->length = pLlcpSocket->sSocketRwBufferTable[readIndex].length - 1;

         /* Reset RW buffer length */
         pLlcpSocket->sSocketRwBufferTable[readIndex].length = 0;

         /* Update Value Rw Read Index */
         pLlcpSocket->indexRwRead++;

         /* call the recv callback */
         pRecv_Cb(pContext, ssap, NFCSTATUS_SUCCESS);

         status = NFCSTATUS_SUCCESS;
      }
      /* Otherwise, wait for a packet to come */
      else
      {
         /* Store the callback and context*/
         pLlcpSocket->pfSocketRecvFrom_Cb  = pRecv_Cb;
         pLlcpSocket->pRecvContext         = pContext;

         /* Store the pointer to the receive buffer */
         pLlcpSocket->sSocketRecvBuffer   =  psBuffer;
         pLlcpSocket->receivedLength      =  &psBuffer->length;

         /* Set RecvPending to TRUE */
         pLlcpSocket->bSocketRecvPending = TRUE;
      }
   }

Done:

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}
