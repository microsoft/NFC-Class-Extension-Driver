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
#include <phFriNfc_Llcp.h>
#include <phFriNfc_LlcpTransport.h>
#include <phFriNfc_LlcpTransport_Connectionless.h>
#include <phFriNfc_LlcpTransport_Connection.h>

#include "phFriNfc_LLcpTransport.tmh"

#define IS_BETWEEN(x, a, b) (((x)>=(a)) && ((x)<(b)))

static
NFCSTATUS
phFriNfc_LlcpTransport_RegisterName(
    _In_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _In_ uint8_t                         nSap,
    _In_ phNfc_sData_t                   *psServiceName
    );

static
NFCSTATUS
phFriNfc_LlcpTransport_DiscoverServicesEx(
    _Inout_ phFriNfc_LlcpTransport_t *psTransport
    );

static
void
phFriNfc_LlcpTransport_Send_CB(
    _At_((phFriNfc_LlcpTransport_t*)pContext, _Inout_)  void        *pContext,
    _In_                                                NFCSTATUS   status
    );

static
NFCSTATUS
phFriNfc_LlcpTransport_GetFreeSap(
    _In_    phFriNfc_LlcpTransport_t  *psTransport,
    _In_    phNfc_sData_t                   *psServiceName,
    _Out_   uint8_t                         *pnSap
    )
{
   uint8_t i;
   uint8_t sap;
   uint8_t min_sap_range, max_sap_range;
   phFriNfc_LlcpTransport_Socket_t* pSocketTable = psTransport->pSocketTable;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Calculate authorized SAP range */
   if ((psServiceName != NULL) && (psServiceName->length > 0))
   {
      /* Make sure that we will return the same SAP if service name was already used in the past */
      for(i=0 ; i<PHFRINFC_LLCP_SDP_ADVERTISED_NB ; i++)
      {
         if((psTransport->pCachedServiceNames[i].sServiceName.length > 0) &&
             (phOsalNfc_MemCompare((void*)psTransport->pCachedServiceNames[i].sServiceName.buffer,(void *)psServiceName->buffer, psServiceName->length) == 0))
         {
            /* Service name matched in cached service names list */
            *pnSap = psTransport->pCachedServiceNames[i].nSap;
            PH_LOG_LLCP_FUNC_EXIT();
            return NFCSTATUS_SUCCESS;
         }
      }

      /* SDP advertised service */
      min_sap_range = PHFRINFC_LLCP_SAP_SDP_ADVERTISED_FIRST;
      max_sap_range = PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST;
   }
   else
   {
      /* Non-SDP advertised service */
      min_sap_range = PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST;
      max_sap_range = PHFRINFC_LLCP_SAP_NUMBER;
   }

   /* Try all possible SAPs */
   for(sap=min_sap_range ; sap<max_sap_range ; sap++)
   {
      /* Go through socket list to check if current SAP is in use */
      for(i=0 ; i<PHFRINFC_LLCP_NB_SOCKET_MAX ; i++)
      {
         if((pSocketTable[i].eSocket_State >= phFriNfc_LlcpTransportSocket_eSocketBound) &&
            (pSocketTable[i].socket_sSap == sap))
         {
            /* SAP is already in use */
            break;
         }
      }

      if (i >= PHFRINFC_LLCP_NB_SOCKET_MAX)
      {
         /* No socket is using current SAP, proceed with binding */
         *pnSap = sap;
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_SUCCESS;
      }
   }

   /* If we reach this point, it means that no SAP is free */
   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_INSUFFICIENT_RESOURCES;
}

static
NFCSTATUS
phFriNfc_LlcpTransport_EncodeSdreqTlv(
    _In_    phNfc_sData_t   *psTlvData,
    _Inout_ uint32_t        *pOffset,
    _In_    uint8_t         nTid,
    _In_    phNfc_sData_t   *psServiceName
    )
{
   NFCSTATUS result;
   uint32_t nTlvOffset = *pOffset;
   uint32_t nTlvStartOffset = nTlvOffset;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Encode the TID */
   result = phFriNfc_Llcp_EncodeTLV(psTlvData,
                                    &nTlvOffset,
                                    PHFRINFC_LLCP_TLV_TYPE_SDREQ,
                                    1,
                                    &nTid);
   if (result != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   /* Encode the service name itself */
   result = phFriNfc_Llcp_AppendTLV(psTlvData,
                                    nTlvStartOffset,
                                    &nTlvOffset,
                                    (uint8_t)psServiceName->length,
                                    psServiceName->buffer);
   if (result != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

clean_and_return:
   /* Save offset if no error occured */
   if (result == NFCSTATUS_SUCCESS)
   {
      *pOffset = nTlvOffset;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

static
NFCSTATUS
phFriNfc_LlcpTransport_EncodeSdresTlv(
    _In_    phNfc_sData_t   *psTlvData,
    _Inout_ uint32_t        *pOffset,
    _In_    uint8_t         nTid,
    _In_    uint8_t         nSap
    )
{
   NFCSTATUS result;
   uint32_t nTlvStartOffset = *pOffset;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Encode the TID */
   result = phFriNfc_Llcp_EncodeTLV(psTlvData,
                                    pOffset,
                                    PHFRINFC_LLCP_TLV_TYPE_SDRES,
                                    1,
                                    &nTid);
   if (result != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   /* Encode the service name itself */
   result = phFriNfc_Llcp_AppendTLV(psTlvData,
                                    nTlvStartOffset,
                                    pOffset,
                                    1,
                                    &nSap);
   if (result != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

clean_and_return:
   /* Restore previous offset if an error occured */
   if (result != NFCSTATUS_SUCCESS)
   {
      *pOffset = nTlvStartOffset;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

_Ret_maybenull_
static
phFriNfc_LlcpTransport_Socket_t*
phFriNfc_LlcpTransport_ServiceNameLoockup(
    _In_ phFriNfc_LlcpTransport_t    *psTransport,
    _In_ phNfc_sData_t               *pServiceName
    )
{
   uint32_t                            index;
   uint8_t                             cacheIndex;
   phFriNfc_Llcp_CachedServiceName_t * pCachedServiceName;
   phFriNfc_LlcpTransport_Socket_t *   pSocket;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Search a socket with the SN */
   for(index=0;index<PHFRINFC_LLCP_NB_SOCKET_MAX;index++)
   {
      pSocket = &psTransport->pSocketTable[index];
      /* Test if the CO socket is in Listen state or the CL socket is bound
         and if its SN is the good one */
      if((((pSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
         && (pSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketRegistered))
         || ((pSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionLess)
         && (pSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketBound)))
         &&
         (pServiceName->length == pSocket->sServiceName.length)
         && !phOsalNfc_MemCompare((void *)pServiceName->buffer,(void *)pSocket->sServiceName.buffer, pServiceName->length))
      {
         /* Add new entry to cached service name/sap if not already in table */
         for(cacheIndex=0;cacheIndex<PHFRINFC_LLCP_SDP_ADVERTISED_NB;cacheIndex++)
         {
            pCachedServiceName = &psTransport->pCachedServiceNames[cacheIndex];
            if (pCachedServiceName->sServiceName.buffer != NULL)
            {
               if ((pCachedServiceName->sServiceName.length == pServiceName->length) &&
                   (phOsalNfc_MemCompare((void *)pCachedServiceName->sServiceName.buffer, (void *)pServiceName->buffer, pServiceName->length) == 0))
               {
                  /* Already registered */
                  break;
               }
            }
            else
            {
               /* Reached end of existing entries and not found the service name,
                * => Add the new entry
                */
               pCachedServiceName->nSap = pSocket->socket_sSap;
               pCachedServiceName->sServiceName.buffer = phOsalNfc_GetMemory(pServiceName->length);
               if (pCachedServiceName->sServiceName.buffer == NULL)
               {
                  /* Unable to cache this entry, so report this service as not found */
                  PH_LOG_LLCP_FUNC_EXIT();
                  return NULL;
               }
               phOsalNfc_MemCopy((void *)pCachedServiceName->sServiceName.buffer,(void *) pServiceName->buffer, pServiceName->length);
               pCachedServiceName->sServiceName.length = pServiceName->length;
               break;
            }
         }

         PH_LOG_LLCP_FUNC_EXIT();
         return pSocket;
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return NULL;
}


static
NFCSTATUS
phFriNfc_LlcpTransport_DiscoveryAnswer(
    _Inout_ phFriNfc_LlcpTransport_t *psTransport
    )
{
   NFCSTATUS         result = NFCSTATUS_PENDING;
   phNfc_sData_t     sInfoBuffer;
   uint32_t          nTlvOffset;
   uint8_t           index;
   uint8_t           nTid, nSap;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Test if a send is pending */
   if(!psTransport->bSendPending)
   {
      /* Set the header */
      psTransport->sLlcpHeader.dsap  = PHFRINFC_LLCP_SAP_SDP;
      psTransport->sLlcpHeader.ptype = PHFRINFC_LLCP_PTYPE_SNL;
      psTransport->sLlcpHeader.ssap  = PHFRINFC_LLCP_SAP_SDP;

      /* Prepare the info buffer */
      sInfoBuffer.buffer = psTransport->pDiscoveryBuffer;
      sInfoBuffer.length = sizeof(psTransport->pDiscoveryBuffer);

      /* Encode as many requests as possible */
      nTlvOffset = 0;
      for(index=0 ; index<psTransport->nDiscoveryResListSize ; index++)
      {
         /* Get current TID/SAP and try to encode them in SNL frame */
         nTid = psTransport->nDiscoveryResTidList[index];
         nSap = psTransport->nDiscoveryResSapList[index];
         /* Encode response */
         result = phFriNfc_LlcpTransport_EncodeSdresTlv(&sInfoBuffer,
                                                        &nTlvOffset,
                                                        nTid,
                                                        nSap);
         if (result != NFCSTATUS_SUCCESS)
         {
            /* Impossible to fit the entire response */
            /* TODO: support reponse framgentation */
            break;
         }
      }

      /* Reset list size to be able to handle a new request */
      psTransport->nDiscoveryResListSize = 0;

      /* Update buffer length to match real TLV size */
      sInfoBuffer.length = nTlvOffset;

      /* Send Pending */
      psTransport->bSendPending = TRUE;

      /* Send SNL frame */
      result =  phFriNfc_Llcp_Send(psTransport->pLlcp,
                                   &psTransport->sLlcpHeader,
                                   NULL,
                                   &sInfoBuffer,
                                   phFriNfc_LlcpTransport_Send_CB,
                                   psTransport);
   }
   else
   {
      /* Impossible to send now, this function will be called again on next opportunity */
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}


static
void
Handle_Discovery_IncomingFrame(
    _Inout_ phFriNfc_LlcpTransport_t    *psTransport,
    _In_    phNfc_sData_t               *psData
    )
{
   NFCSTATUS                        result;
   phNfc_sData_t                    sValue;
   phNfc_sData_t                    sResponseData;
   phNfc_sData_t                    sServiceName;
   uint32_t                         nInTlvOffset;
   uint8_t                          nType;
   uint8_t                          nTid;
   uint8_t                          nSap;
   pphFriNfc_Cr_t                   pfSavedCb;
   void                             *pfSavedContext;
   phFriNfc_LlcpTransport_Socket_t  *pSocket;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Prepare buffer */
   sResponseData.buffer = psTransport->pDiscoveryBuffer;
   sResponseData.length = sizeof(psTransport->pDiscoveryBuffer);

   /* Parse all TLVs in frame */
   nInTlvOffset = 0;
   while(nInTlvOffset < psData->length)
   {
      result = phFriNfc_Llcp_DecodeTLV(psData,
                                       &nInTlvOffset,
                                       &nType,
                                       &sValue );
      if (NFCSTATUS_SUCCESS == result)
      {
          switch(nType)
          {
             case PHFRINFC_LLCP_TLV_TYPE_SDREQ:
                if (sValue.length < 2)
                {
                   /* Erroneous request, ignore */
                   break;
                }
                /* Decode TID */
                nTid = sValue.buffer[0];
                /* Decode service name */
                sServiceName.buffer = sValue.buffer + 1;
                sServiceName.length = sValue.length - 1;

                /* Handle SDP service name */
                if((sServiceName.length == sizeof(PHFRINFC_LLCP_SERVICENAME_SDP)-1)
                    && !phOsalNfc_MemCompare((void *)sServiceName.buffer,(void *) PHFRINFC_LLCP_SERVICENAME_SDP, sServiceName.length))
                {
                   nSap = PHFRINFC_LLCP_SAP_SDP;
                }
                else
                {
                   /* Match service name in socket list */
                   pSocket = phFriNfc_LlcpTransport_ServiceNameLoockup(psTransport, &sServiceName);
                   if (pSocket != NULL)
                   {
                      nSap = pSocket->socket_sSap;
                   }
                   else
                   {
                      nSap = 0;
                   }
                }

                /* Encode response */
                psTransport->nDiscoveryResSapList[psTransport->nDiscoveryResListSize] = nSap;
                psTransport->nDiscoveryResTidList[psTransport->nDiscoveryResListSize] = nTid;
                psTransport->nDiscoveryResListSize++;
                break;

             case PHFRINFC_LLCP_TLV_TYPE_SDRES:
                if (psTransport->pfDiscover_Cb == NULL)
                {
                   /* Ignore response when no requests are pending */
                   break;
                }
                if (sValue.length != 2)
                {
                   /* Erroneous response, ignore it */
                   break;
                }
                /* Decode TID and SAP */
                nTid = sValue.buffer[0];
                if (nTid >= psTransport->nDiscoveryListSize)
                {
                   /* Unkown TID, ignore it */
                   break;
                }
                nSap = sValue.buffer[1];
                /* Save response */
                psTransport->pnDiscoverySapList[nTid] = nSap;
                /* Update response counter */
                psTransport->nDiscoveryResOffset++;
                break;

             default:
                /* Ignored */
                break;
          }
      }
      else
      {
          /* Error in TLV */
          break;
      }
   }

   /* If discovery requests have been received, send response */
   if (psTransport->nDiscoveryResListSize > 0)
   {
      phFriNfc_LlcpTransport_DiscoveryAnswer(psTransport);
   }

   /* If all discovery responses have been received, trigger callback (if any) */
   if ((psTransport->pfDiscover_Cb != NULL) &&
       (psTransport->nDiscoveryResOffset >= psTransport->nDiscoveryListSize))
   {
      pfSavedCb = psTransport->pfDiscover_Cb;
      pfSavedContext = psTransport->pDiscoverContext;

      psTransport->pfDiscover_Cb = NULL;
      psTransport->pDiscoverContext = NULL;

      pfSavedCb(pfSavedContext, NFCSTATUS_SUCCESS);
   }

   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_LlcpTransport__Recv_CB(
    _At_((phFriNfc_LlcpTransport_t*)pContext, _Inout_)  void            *pContext,
    _Inout_                                             phNfc_sData_t   *psData,
    _In_                                                NFCSTATUS       status
    )
{
   phFriNfc_Llcp_sPacketHeader_t   sLlcpLocalHeader;
   uint8_t   dsap;
   uint8_t   ptype;
   uint8_t   ssap;

   phFriNfc_LlcpTransport_t* pLlcpTransport = (phFriNfc_LlcpTransport_t*)pContext;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(status != NFCSTATUS_SUCCESS)
   {
      pLlcpTransport->LinkStatusError = TRUE;
   }
   else
   {
      if(psData->length >= PHFRINFC_LLCP_PACKET_HEADER_SIZE)
      {

          phFriNfc_Llcp_Buffer2Header( psData->buffer,0x00, &sLlcpLocalHeader);

          dsap  = (uint8_t)sLlcpLocalHeader.dsap;
          ptype = (uint8_t)sLlcpLocalHeader.ptype;
          ssap  = (uint8_t)sLlcpLocalHeader.ssap;

          /* Update the length value (without the header length) */
          psData->length = psData->length - PHFRINFC_LLCP_PACKET_HEADER_SIZE;

          /* Update the buffer pointer */
          psData->buffer = psData->buffer + PHFRINFC_LLCP_PACKET_HEADER_SIZE;

          switch(ptype)
          {
          /* Connectionless */
          case PHFRINFC_LLCP_PTYPE_UI:
             {
                Handle_Connectionless_IncommingFrame(pLlcpTransport,
                                                     psData,
                                                     dsap,
                                                     ssap);
             }break;

          /* Service Discovery Protocol */
          case PHFRINFC_LLCP_PTYPE_SNL:
             {
                if ((ssap == PHFRINFC_LLCP_SAP_SDP) && (dsap == PHFRINFC_LLCP_SAP_SDP))
                {
                   Handle_Discovery_IncomingFrame(pLlcpTransport,
                                                  psData);
                }
                else
                {
                   /* Ignore frame if source and destination are not the SDP service */
                }
             }break;

          /* Connection oriented */
          /* NOTE: forward reserved PTYPE to enable FRMR sending */
          case PHFRINFC_LLCP_PTYPE_CONNECT:
          case PHFRINFC_LLCP_PTYPE_CC:
          case PHFRINFC_LLCP_PTYPE_DISC:
          case PHFRINFC_LLCP_PTYPE_DM:
          case PHFRINFC_LLCP_PTYPE_I:
          case PHFRINFC_LLCP_PTYPE_RR:
          case PHFRINFC_LLCP_PTYPE_RNR:
          case PHFRINFC_LLCP_PTYPE_FRMR:
          case PHFRINFC_LLCP_PTYPE_RESERVED1:
          case PHFRINFC_LLCP_PTYPE_RESERVED2:
          case PHFRINFC_LLCP_PTYPE_RESERVED3:
             {
                Handle_ConnectionOriented_IncommingFrame(pLlcpTransport,
                                                         psData,
                                                         dsap,
                                                         ptype,
                                                         ssap);
             }break;
          default:
             {

             }break;
          }
      }

      /*Restart the Receive Loop */
      status  = phFriNfc_Llcp_Recv(pLlcpTransport->pLlcp,
                                   phFriNfc_LlcpTransport__Recv_CB,
                                   pLlcpTransport);
   }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_LlcpTransport_Send_CB(
    _At_((phFriNfc_LlcpTransport_t*)pContext, _Inout_)  void        *pContext,
    _In_                                                NFCSTATUS   status
    )
{
   phFriNfc_LlcpTransport_t         *psTransport = (phFriNfc_LlcpTransport_t*)pContext;
   NFCSTATUS                        result = NFCSTATUS_FAILED;
   phNfc_sData_t                    sFrmrBuffer;
   phFriNfc_Llcp_Send_CB_t          pfSavedCb;
   void                             *pSavedContext;
   phFriNfc_LlcpTransport_Socket_t  *pCurrentSocket = NULL;
   uint8_t                          index;

   /* 1 - Reset the FLAG send pending*/

   PH_LOG_LLCP_FUNC_ENTRY();
   psTransport->bSendPending = FALSE;

   /* 2 - Handle pending error responses */

   if(psTransport->bFrmrPending)
   {
      /* Reset FRMR pending */
      psTransport->bFrmrPending = FALSE;

      /* Send Frmr */
      sFrmrBuffer.buffer = psTransport->FrmrInfoBuffer;
      sFrmrBuffer.length = 0x04; /* Size of FRMR Information field */

      /* Send Pending */
      psTransport->bSendPending = TRUE;

      result =  phFriNfc_Llcp_Send(psTransport->pLlcp,
                                   &psTransport->sLlcpHeader,
                                   NULL,
                                   &sFrmrBuffer,
                                   phFriNfc_LlcpTransport_Send_CB,
                                   psTransport);

   }
   else if(psTransport->bDmPending)
   {
      /* Reset DM pending */
      psTransport->bDmPending = FALSE;

      /* Send DM pending */
      result = phFriNfc_LlcpTransport_SendDisconnectMode(psTransport,
                                                         psTransport->DmInfoBuffer[0],
                                                         psTransport->DmInfoBuffer[1],
                                                         psTransport->DmInfoBuffer[2]);
   }

   /* 3 - Call the original callback */

   if (psTransport->pfLinkSendCb != NULL)
   {
      pfSavedCb = psTransport->pfLinkSendCb;
      pSavedContext = psTransport->pLinkSendContext;

      psTransport->pfLinkSendCb = NULL;
      psTransport->pLinkSendContext = NULL;

      (*pfSavedCb)(pSavedContext, status);
   }

   /* 4 - Handle pending send operations */

   /* Check for pending discovery requests/responses */
   if (psTransport->nDiscoveryResListSize > 0)
   {
      phFriNfc_LlcpTransport_DiscoveryAnswer(psTransport);
   }
   if ( (psTransport->pfDiscover_Cb != NULL) &&
        (psTransport->nDiscoveryReqOffset < psTransport->nDiscoveryListSize) )
   {
      result = phFriNfc_LlcpTransport_DiscoverServicesEx(psTransport);
   }

   /* Init index */
   index = psTransport->socketIndex;

   /* Check all sockets for pending operation */
   do
   {
      /* Modulo-increment index */
      index = (index + 1) % PHFRINFC_LLCP_NB_SOCKET_MAX;

      pCurrentSocket = &psTransport->pSocketTable[index];

      /* Dispatch to the corresponding transport layer */
      if (pCurrentSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
      {
         result = phFriNfc_LlcpTransport_ConnectionOriented_HandlePendingOperations(pCurrentSocket);
      }
      else if (pCurrentSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionLess)
      {
         result = phFriNfc_LlcpTransport_Connectionless_HandlePendingOperations(pCurrentSocket);
      }

      if (result != NFCSTATUS_FAILED)
      {
         /* Stop looping if pending operation has been found */
         break;
      }

   } while(index != psTransport->socketIndex);

   /* Save the new index */
   /* psTransport->socketIndex = index; */
   PH_LOG_LLCP_FUNC_EXIT();
}

NFCSTATUS
phFriNfc_LlcpTransport_Reset(
    _Out_   phFriNfc_LlcpTransport_t    *pLlcpTransport,
    _In_    phFriNfc_Llcp_t             *pLlcp
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check for NULL pointers */
   if(pLlcpTransport == NULL || pLlcp == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Reset Transport structure */
      pLlcpTransport->pLlcp            = pLlcp;
      pLlcpTransport->LinkStatusError  = FALSE;
      pLlcpTransport->bSendPending     = FALSE;
      pLlcpTransport->bRecvPending     = FALSE;
      pLlcpTransport->bDmPending       = FALSE;
      pLlcpTransport->bFrmrPending     = FALSE;
      pLlcpTransport->socketIndex      = FALSE;
      pLlcpTransport->LinkStatusError  = 0;
      pLlcpTransport->pfDiscover_Cb    = NULL;

      /* Initialize cached service name/sap table */
      phOsalNfc_SetMemory((void *)pLlcpTransport->pCachedServiceNames, 0x00, sizeof(phFriNfc_Llcp_CachedServiceName_t)*PHFRINFC_LLCP_SDP_ADVERTISED_NB);

      /* Reset all the socket info in the table */
      for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
      {
         pLlcpTransport->pSocketTable[i].eSocket_State                  = phFriNfc_LlcpTransportSocket_eSocketDefault;
         pLlcpTransport->pSocketTable[i].eSocket_Type                   = phFriNfc_LlcpTransport_eDefaultType;
         pLlcpTransport->pSocketTable[i].index                          = i;
         pLlcpTransport->pSocketTable[i].pContext                       = NULL;
         pLlcpTransport->pSocketTable[i].pListenContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pAcceptContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pRejectContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pConnectContext                = NULL;
         pLlcpTransport->pSocketTable[i].pDisonnectContext              = NULL;
         pLlcpTransport->pSocketTable[i].pSendContext                   = NULL;
         pLlcpTransport->pSocketTable[i].pRecvContext                   = NULL;
         pLlcpTransport->pSocketTable[i].pSocketErrCb                   = NULL;
         pLlcpTransport->pSocketTable[i].bufferLinearLength             = 0;
         pLlcpTransport->pSocketTable[i].bufferSendMaxLength            = 0;
         pLlcpTransport->pSocketTable[i].bufferRwMaxLength              = 0;
         pLlcpTransport->pSocketTable[i].ReceiverBusyCondition          = FALSE;
         pLlcpTransport->pSocketTable[i].RemoteBusyConditionInfo        = FALSE;
         pLlcpTransport->pSocketTable[i].socket_sSap                    = PHFRINFC_LLCP_SAP_DEFAULT;
         pLlcpTransport->pSocketTable[i].socket_dSap                    = PHFRINFC_LLCP_SAP_DEFAULT;
         pLlcpTransport->pSocketTable[i].bSocketRecvPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketSendPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketListenPending           = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketDiscPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketConnectPending          = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketAcceptPending           = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketRRPending               = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketRNRPending              = FALSE;
         pLlcpTransport->pSocketTable[i].psTransport                    = pLlcpTransport;
         pLlcpTransport->pSocketTable[i].pfSocketSend_Cb                = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketRecv_Cb                = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketRecvFrom_Cb            = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketListen_Cb              = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketConnect_Cb             = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketDisconnect_Cb          = NULL;
         pLlcpTransport->pSocketTable[i].socket_VS                      = 0;
         pLlcpTransport->pSocketTable[i].socket_VSA                     = 0;
         pLlcpTransport->pSocketTable[i].socket_VR                      = 0;
         pLlcpTransport->pSocketTable[i].socket_VRA                     = 0;
         pLlcpTransport->pSocketTable[i].remoteRW                       = 0;
         pLlcpTransport->pSocketTable[i].localRW                        = 0;
         pLlcpTransport->pSocketTable[i].remoteMIU                      = 0;
         pLlcpTransport->pSocketTable[i].localMIUX                      = 0;
         pLlcpTransport->pSocketTable[i].index                          = 0;
         pLlcpTransport->pSocketTable[i].indexRwRead                    = 0;
         pLlcpTransport->pSocketTable[i].indexRwWrite                   = 0;

         phOsalNfc_SetMemory((void *)&pLlcpTransport->pSocketTable[i].sSocketOption, 0x00, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

         if (pLlcpTransport->pSocketTable[i].sServiceName.buffer != NULL) {
            phOsalNfc_FreeMemory(pLlcpTransport->pSocketTable[i].sServiceName.buffer);
         }
         pLlcpTransport->pSocketTable[i].sServiceName.buffer = NULL;
         pLlcpTransport->pSocketTable[i].sServiceName.length = 0;
      }

      /* Start The Receive Loop */
      status  = phFriNfc_Llcp_Recv(pLlcpTransport->pLlcp,
                                   phFriNfc_LlcpTransport__Recv_CB,
                                   pLlcpTransport);
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_CloseAll(
    _Inout_ phFriNfc_LlcpTransport_t *pLlcpTransport
    )
{
   NFCSTATUS                           status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_CachedServiceName_t * pCachedServiceName;
   uint8_t                             i;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check for NULL pointers */
   if(pLlcpTransport == NULL)
   {
       PH_LOG_LLCP_WARN_STR("Invalid pLlcpTransport 'LlcpTransportContext'");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
       PH_LOG_LLCP_INFO_STR("Closing all sockets...");
        /* Close all sockets */
        for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
        {
          if(pLlcpTransport->pSocketTable[i].eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
          {
              PH_LOG_LLCP_INFO_STR("eSocket_Type is phFriNfc_LlcpTransport_eConnectionOriented");
             switch(pLlcpTransport->pSocketTable[i].eSocket_State)
             {
             case phFriNfc_LlcpTransportSocket_eSocketConnected:
             case phFriNfc_LlcpTransportSocket_eSocketConnecting:
             case phFriNfc_LlcpTransportSocket_eSocketAccepted:
             case phFriNfc_LlcpTransportSocket_eSocketDisconnected:
             case phFriNfc_LlcpTransportSocket_eSocketDisconnecting:
             case phFriNfc_LlcpTransportSocket_eSocketRejected:
                phFriNfc_LlcpTransport_Close(&pLlcpTransport->pSocketTable[i]);
                break;
             default:
                 PH_LOG_LLCP_INFO_STR("Invalid Socket state");
                /* Do nothing */
                break;
             }
          }
          else
          {
              PH_LOG_LLCP_INFO_STR("eSocket_Type is phFriNfc_LlcpTransport_eConnectionLess");
             phFriNfc_LlcpTransport_Close(&pLlcpTransport->pSocketTable[i]);
          }
        }
		
		/* Reset Transport structure */ 
   pLlcpTransport->LinkStatusError  = FALSE;
   pLlcpTransport->bSendPending     = FALSE;
   pLlcpTransport->bRecvPending     = FALSE;
   pLlcpTransport->bDmPending       = FALSE;
   pLlcpTransport->bFrmrPending     = FALSE;
   pLlcpTransport->LinkStatusError  = 0;
   pLlcpTransport->pfLinkSendCb     = NULL;

        /* Reset cached service name/sap table */
        for(i=0;i<PHFRINFC_LLCP_SDP_ADVERTISED_NB;i++)
        {
          pCachedServiceName = &pLlcpTransport->pCachedServiceNames[i];

          pCachedServiceName->nSap = 0;
          if (pCachedServiceName->sServiceName.buffer != NULL)
          {
             phOsalNfc_FreeMemory(pCachedServiceName->sServiceName.buffer);
             pCachedServiceName->sServiceName.buffer = NULL;
          }
          pCachedServiceName->sServiceName.length = 0;
        }
   }
   PH_LOG_LLCP_INFO_X32MSG("Return status:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}


/* TODO: comment function Transport LinkSend */
NFCSTATUS
phFriNfc_LlcpTransport_LinkSend(
    _Inout_ phFriNfc_LlcpTransport_t        *LlcpTransport,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo,
    _In_    phFriNfc_Llcp_Send_CB_t         pfSend_CB,
    _In_    void                            *pContext
    )
{
    NFCSTATUS status;
    PH_LOG_LLCP_FUNC_ENTRY();
   /* Check if a send is already ongoing */
   if (LlcpTransport->pfLinkSendCb != NULL)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_BUSY;
   }

   /* Save callback details */
   LlcpTransport->pfLinkSendCb = pfSend_CB;
   LlcpTransport->pLinkSendContext = pContext;

   /* Call the link-level send function */
   status = phFriNfc_Llcp_Send(LlcpTransport->pLlcp, psHeader, psSequence, psInfo, phFriNfc_LlcpTransport_Send_CB, (void*)LlcpTransport);
   if (status != NFCSTATUS_PENDING && status != NFCSTATUS_SUCCESS) {
       // Clear out callbacks
       LlcpTransport->pfLinkSendCb = NULL;
       LlcpTransport->pLinkSendContext = NULL;
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_SendFrameReject(
    _Inout_ phFriNfc_LlcpTransport_t        *psTransport,
    _In_    uint8_t                         dsap,
    _In_    uint8_t                         rejectedPTYPE,
    _In_    uint8_t                         ssap,
    _In_    phFriNfc_Llcp_sPacketSequence_t *sLlcpSequence,
    _In_    uint8_t                         WFlag,
    _In_    uint8_t                         IFlag,
    _In_    uint8_t                         RFlag,
    _In_    uint8_t                         SFlag,
    _In_    uint8_t                         vs,
    _In_    uint8_t                         vsa,
    _In_    uint8_t                         vr,
    _In_    uint8_t                         vra
    )
{
   NFCSTATUS                       status = NFCSTATUS_SUCCESS;
   phNfc_sData_t                   sFrmrBuffer;
   uint8_t                         flagValue;
   uint8_t                         sequence = 0;
   uint8_t     index;
   uint8_t     socketFound = FALSE;

   /* Search a socket waiting for a FRAME */
   for(index=0;index<PHFRINFC_LLCP_NB_SOCKET_MAX;index++)
   {
      /* Test if the socket is in connected state and if its SSAP and DSAP are valid */
      if(psTransport->pSocketTable[index].socket_sSap == dsap
         && psTransport->pSocketTable[index].socket_dSap == ssap)
      {
         /* socket found */
         socketFound = TRUE;
         break;
      }
   }

   /* Test if a socket has been found */
   if(socketFound)
   {
      /* Set socket state to disconnected */
      psTransport->pSocketTable[index].eSocket_State =  phFriNfc_LlcpTransportSocket_eSocketDefault;

      /* Call ErrCB due to a FRMR*/
      psTransport->pSocketTable[index].pSocketErrCb( psTransport->pSocketTable[index].pContext,PHFRINFC_LLCP_ERR_FRAME_REJECTED);

      /* Close the socket */
      status = phFriNfc_LlcpTransport_ConnectionOriented_Close(&psTransport->pSocketTable[index]);

      /* Set FRMR Header */
      psTransport->sLlcpHeader.dsap   = ssap;
      psTransport->sLlcpHeader.ptype  = PHFRINFC_LLCP_PTYPE_FRMR;
      psTransport->sLlcpHeader.ssap   = dsap;

      /* Set FRMR Information Field */
      flagValue = (WFlag<<7) | (IFlag<<6) | (RFlag<<5) | (SFlag<<4) | rejectedPTYPE;
      if (sLlcpSequence != NULL)
      {
         sequence = (uint8_t)((sLlcpSequence->ns<<4)|(sLlcpSequence->nr));
      }

      psTransport->FrmrInfoBuffer[0] = flagValue;
      psTransport->FrmrInfoBuffer[1] = sequence;
      psTransport->FrmrInfoBuffer[2] = (vs<<4)|vr ;
      psTransport->FrmrInfoBuffer[3] = (vsa<<4)|vra ;

      /* Test if a send is pending */
      if(psTransport->bSendPending)
      {
         psTransport->bFrmrPending = TRUE;
         status = NFCSTATUS_PENDING;
      }
      else
      {
         sFrmrBuffer.buffer =  psTransport->FrmrInfoBuffer;
         sFrmrBuffer.length =  0x04; /* Size of FRMR Information field */

         /* Send Pending */
         psTransport->bSendPending = TRUE;

         /* Send FRMR frame */
         status =  phFriNfc_Llcp_Send(psTransport->pLlcp,
                                      &psTransport->sLlcpHeader,
                                      NULL,
                                      &sFrmrBuffer,
                                      phFriNfc_LlcpTransport_Send_CB,
                                      psTransport);
      }
   }
   else
   {
      /* No active  socket*/
      /* FRMR Frame not handled*/
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_SendDisconnectMode(
    _Inout_ phFriNfc_LlcpTransport_t    *psTransport,
    _In_    uint8_t                     dsap,
    _In_    uint8_t                     ssap,
    _In_    uint8_t                     dmOpCode
    )
{
   NFCSTATUS                       status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(psTransport->bSendPending)
   {
      /* DM pending */
      psTransport->bDmPending        = TRUE;

      /* Store DM Info */
      psTransport->DmInfoBuffer[0] = dsap;
      psTransport->DmInfoBuffer[1] = ssap;
      psTransport->DmInfoBuffer[2] = dmOpCode;

     status = NFCSTATUS_PENDING;
   }
   else
   {
      /* Set the header */
      psTransport->sDmHeader.dsap  = dsap;
      psTransport->sDmHeader.ptype = PHFRINFC_LLCP_PTYPE_DM;
      psTransport->sDmHeader.ssap  = ssap;

      /* Save Operation Code to be provided in DM frame payload */
      psTransport->DmInfoBuffer[2] = dmOpCode;
      psTransport->sDmPayload.buffer    = &psTransport->DmInfoBuffer[2];
      psTransport->sDmPayload.length    = PHFRINFC_LLCP_DM_LENGTH;

      /* Send Pending */
      psTransport->bSendPending = TRUE;

      /* Send DM frame */
      status =  phFriNfc_Llcp_Send(psTransport->pLlcp,
                                   &psTransport->sDmHeader,
                                   NULL,
                                   &psTransport->sDmPayload,
                                   phFriNfc_LlcpTransport_Send_CB,
                                   psTransport);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_SocketGetLocalOptions(
    _In_    phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _Out_   phLibNfc_Llcp_sSocketOptions_t  *psLocalOptions
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if (pLlcpSocket == NULL || psLocalOptions == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
     status = phFriNfc_LlcpTransport_ConnectionOriented_SocketGetLocalOptions(pLlcpSocket,
                                                                              psLocalOptions);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_SocketGetRemoteOptions(
    _In_    phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _Out_   phLibNfc_Llcp_sSocketOptions_t  *psRemoteOptions
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if (pLlcpSocket == NULL || psRemoteOptions == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_SocketGetRemoteOptions(pLlcpSocket,
                                                                                psRemoteOptions);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}


static
NFCSTATUS
phFriNfc_LlcpTransport_DiscoverServicesEx(
    _Inout_ phFriNfc_LlcpTransport_t *psTransport
    )
{
   NFCSTATUS         result = NFCSTATUS_PENDING;
   phNfc_sData_t     sInfoBuffer;
   phNfc_sData_t     *psServiceName;
   uint32_t          nTlvOffset;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(!psTransport->bSendPending)
   {
      /* Set the header */
      psTransport->sLlcpHeader.dsap  = PHFRINFC_LLCP_SAP_SDP;
      psTransport->sLlcpHeader.ptype = PHFRINFC_LLCP_PTYPE_SNL;
      psTransport->sLlcpHeader.ssap  = PHFRINFC_LLCP_SAP_SDP;

      /* Prepare the info buffer */
      sInfoBuffer.buffer = psTransport->pDiscoveryBuffer;
      sInfoBuffer.length = sizeof(psTransport->pDiscoveryBuffer);

      /* Encode as many requests as possible */
      nTlvOffset = 0;
      while(psTransport->nDiscoveryReqOffset < psTransport->nDiscoveryListSize)
      {
         /* Get current service name and try to encode it in SNL frame */
         psServiceName = &psTransport->psDiscoveryServiceNameList[psTransport->nDiscoveryReqOffset];
         result = phFriNfc_LlcpTransport_EncodeSdreqTlv(&sInfoBuffer,
                                                        &nTlvOffset,
                                                        psTransport->nDiscoveryReqOffset,
                                                        psServiceName);
         if (result != NFCSTATUS_SUCCESS)
         {
            /* Impossible to fit more requests in a single frame,
             * will be continued on next opportunity
             */
            break;
         }

         /* Update request counter */
         psTransport->nDiscoveryReqOffset++;
      }

      /* Update buffer length to match real TLV size */
      sInfoBuffer.length = nTlvOffset;

      /* Send Pending */
      psTransport->bSendPending = TRUE;

      /* Send SNL frame */
      result =  phFriNfc_Llcp_Send(psTransport->pLlcp,
                                   &psTransport->sLlcpHeader,
                                   NULL,
                                   &sInfoBuffer,
                                   phFriNfc_LlcpTransport_Send_CB,
                                   psTransport);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

NFCSTATUS
phFriNfc_LlcpTransport_DiscoverServices(
    _Inout_ phFriNfc_LlcpTransport_t    *pLlcpTransport,
    _In_    phNfc_sData_t               *psServiceNameList,
    _In_    uint8_t                     *pnSapList,
    _In_    uint8_t                     nListSize,
    _In_    pphFriNfc_Cr_t              pDiscover_Cb,
    _In_    void                        *pContext
    )
{
   NFCSTATUS         result = NFCSTATUS_FAILED;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Save request details */
   pLlcpTransport->psDiscoveryServiceNameList = psServiceNameList;
   pLlcpTransport->pnDiscoverySapList = pnSapList;
   pLlcpTransport->nDiscoveryListSize = nListSize;
   pLlcpTransport->pfDiscover_Cb = pDiscover_Cb;
   pLlcpTransport->pDiscoverContext = pContext;

   /* Reset internal counters */
   pLlcpTransport->nDiscoveryReqOffset = 0;
   pLlcpTransport->nDiscoveryResOffset = 0;

   /* Perform request */
   result = phFriNfc_LlcpTransport_DiscoverServicesEx(pLlcpTransport);

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}

NFCSTATUS
phFriNfc_LlcpTransport_Socket(
    _Inout_ phFriNfc_LlcpTransport_t                *pLlcpTransport,
    _In_    phFriNfc_LlcpTransport_eSocketType_t    eType,
    _In_    phFriNfc_LlcpTransport_sSocketOptions_t *psOptions,
    _In_    phNfc_sData_t                           *psWorkingBuffer,
    _Outptr_ phFriNfc_LlcpTransport_Socket_t         **pLlcpSocket,
    _In_    pphFriNfc_LlcpTransportSocketErrCb_t    pErr_Cb,
    _In_    void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_sLinkParameters_t  LlcpLinkParamInfo;
   uint8_t index=0;
   uint8_t cpt;
   PH_LOG_LLCP_FUNC_ENTRY();
   if (   ((psOptions == NULL) && (eType == phFriNfc_LlcpTransport_eConnectionOriented))
       || ((psWorkingBuffer == NULL) && (eType == phFriNfc_LlcpTransport_eConnectionOriented))
       || (pLlcpSocket == NULL)
       || (pErr_Cb == NULL)
       || (pContext == NULL)
       || (pLlcpTransport == NULL))
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_LLCP_FUNC_EXIT();
      return status;
   }
   else if(eType != phFriNfc_LlcpTransport_eConnectionOriented && eType != phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_LLCP_FUNC_EXIT();
      return status;
   }
   else if ((psOptions != NULL) && (eType == phFriNfc_LlcpTransport_eConnectionLess))
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      PH_LOG_LLCP_FUNC_EXIT();
      return status;
   }

   /* Get the local parameters of the LLCP Link */
   status = phFriNfc_Llcp_GetLocalInfo(pLlcpTransport->pLlcp,&LlcpLinkParamInfo);
   if(status != NFCSTATUS_SUCCESS)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_FAILED);
      PH_LOG_LLCP_FUNC_EXIT();
      return status;
   }
   else
   {
      /* Search a socket free in the Socket Table*/
      do
      {
         if(pLlcpTransport->pSocketTable[index].eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
         {
            /* Set the socket pointer to socket of the table */
            *pLlcpSocket = &pLlcpTransport->pSocketTable[index];

            /* Store the socket info in the socket pointer */
            pLlcpTransport->pSocketTable[index].eSocket_Type     = eType;
            pLlcpTransport->pSocketTable[index].pSocketErrCb     = pErr_Cb;

            /* Store the context of the upper layer */
            pLlcpTransport->pSocketTable[index].pContext   = pContext;

            /* Set the pointers to the different working buffers */
            if(psOptions != NULL)
            {
               if (eType == phFriNfc_LlcpTransport_eConnectionOriented)
               {
                  /* Test the socket options */
                  if (psOptions->rw > PHFRINFC_LLCP_RW_MAX)
                  {
                     status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
                     PH_LOG_LLCP_FUNC_EXIT();
                     return status;
                  }

                  /* Set socket options */
                  phOsalNfc_MemCopy((void *)&pLlcpTransport->pSocketTable[index].sSocketOption,(void *) psOptions, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

                  /* Set socket local params (MIUX & RW) */
                  pLlcpTransport->pSocketTable[index].localMIUX = (pLlcpTransport->pSocketTable[index].sSocketOption.miu - PHFRINFC_LLCP_MIU_DEFAULT) & PHFRINFC_LLCP_TLV_MIUX_MASK;
                  pLlcpTransport->pSocketTable[index].localRW   = pLlcpTransport->pSocketTable[index].sSocketOption.rw & PHFRINFC_LLCP_TLV_RW_MASK;

                  /* Set the Max length for the Send and Receive Window Buffer */
                  pLlcpTransport->pSocketTable[index].bufferSendMaxLength   = pLlcpTransport->pSocketTable[index].sSocketOption.miu;
                  pLlcpTransport->pSocketTable[index].bufferRwMaxLength     = pLlcpTransport->pSocketTable[index].sSocketOption.miu * ((pLlcpTransport->pSocketTable[index].sSocketOption.rw & PHFRINFC_LLCP_TLV_RW_MASK));
                  pLlcpTransport->pSocketTable[index].bufferLinearLength    = psWorkingBuffer->length - pLlcpTransport->pSocketTable[index].bufferSendMaxLength - pLlcpTransport->pSocketTable[index].bufferRwMaxLength;

                  /* Test the connection oriented buffers length */
                  if((pLlcpTransport->pSocketTable[index].bufferSendMaxLength + pLlcpTransport->pSocketTable[index].bufferRwMaxLength) > psWorkingBuffer->length
                      || ((pLlcpTransport->pSocketTable[index].bufferLinearLength < PHFRINFC_LLCP_MIU_DEFAULT) && (pLlcpTransport->pSocketTable[index].bufferLinearLength != 0)))
                  {
                     status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_BUFFER_TOO_SMALL);
                     PH_LOG_LLCP_FUNC_EXIT();
                     return status;
                  }

                  /* Set the pointer and the length for the Receive Window Buffer */
                  for(cpt=0;cpt<pLlcpTransport->pSocketTable[index].localRW;cpt++)
                  {
                     pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].buffer = psWorkingBuffer->buffer + (cpt*pLlcpTransport->pSocketTable[index].sSocketOption.miu);
                     pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].length = 0;
                  }

                  /* Set the pointer and the length for the Send Buffer */
                  pLlcpTransport->pSocketTable[index].sSocketSendBuffer.buffer     = psWorkingBuffer->buffer + pLlcpTransport->pSocketTable[index].bufferRwMaxLength;
                  pLlcpTransport->pSocketTable[index].sSocketSendBuffer.length     = pLlcpTransport->pSocketTable[index].bufferSendMaxLength;

                  /** Set the pointer and the length for the Linear Buffer */
                  pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.buffer   = psWorkingBuffer->buffer + pLlcpTransport->pSocketTable[index].bufferRwMaxLength + pLlcpTransport->pSocketTable[index].bufferSendMaxLength;
                  pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length   = pLlcpTransport->pSocketTable[index].bufferLinearLength;

                  if(pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length != 0)
                  {
                     /* Init Cyclic Fifo */
                     phFriNfc_Llcp_CyclicFifoInit(&pLlcpTransport->pSocketTable[index].sCyclicFifoBuffer,
                                                  pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.buffer,
                                                  pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length);
                  }
               }
            }
            /* Handle connectionless socket with buffering option */
            else if (eType == phFriNfc_LlcpTransport_eConnectionLess)
            {
               /* Determine how many packets can be bufferized in working buffer */
               if (psWorkingBuffer != NULL)
               {
                  /* NOTE: the extra byte is used to store SSAP */
                  pLlcpTransport->pSocketTable[index].localRW = (uint8_t)( psWorkingBuffer->length / (pLlcpTransport->pLlcp->sLocalParams.miu + 1));
               }
               else
               {
                  pLlcpTransport->pSocketTable[index].localRW = 0;
               }

               if (pLlcpTransport->pSocketTable[index].localRW > PHFRINFC_LLCP_RW_MAX)
               {
                  pLlcpTransport->pSocketTable[index].localRW = PHFRINFC_LLCP_RW_MAX;
               }

               /* Set the pointers and the lengths for buffering */
               for(cpt=0 ; cpt<pLlcpTransport->pSocketTable[index].localRW ; cpt++)
               {
                  pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].buffer = psWorkingBuffer->buffer + (cpt*(pLlcpTransport->pLlcp->sLocalParams.miu + 1));
                  pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].length = 0;
               }

               /* Set other socket internals */
               pLlcpTransport->pSocketTable[index].indexRwRead      = 0;
               pLlcpTransport->pSocketTable[index].indexRwWrite     = 0;
            }

            /* Store index of the socket */
            pLlcpTransport->pSocketTable[index].index = index;

            /* Set the socket into created state */
            pLlcpTransport->pSocketTable[index].eSocket_State = phFriNfc_LlcpTransportSocket_eSocketCreated;
            PH_LOG_LLCP_FUNC_EXIT();
            return status;
         }
         else
         {
            index++;
         }
      }while(index<PHFRINFC_LLCP_NB_SOCKET_MAX);

      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INSUFFICIENT_RESOURCES);
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Close(
    _Inout_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if( pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      goto Done;
   }

   if(pLlcpSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Close(pLlcpSocket);
   }
   else if(pLlcpSocket->eSocket_Type ==  phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = phFriNfc_LlcpTransport_Connectionless_Close(pLlcpSocket);
   }
   else
   {
      PH_LOG_LLCP_INFO_STR("Invalid eSocket_Type, %!phFriNfc_LlcpTransport_eSocketType_t!", 
                            pLlcpSocket->eSocket_Type);
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      goto Done;
   }

Done:

   PH_LOG_LLCP_INFO_STR("Return status: %!NFCSTATUS!", status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Bind(
    _Inout_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _In_    uint8_t                         nSap,
    _In_    phNfc_sData_t                   *psServiceName
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;
   uint8_t min_sap_range;
   uint8_t max_sap_range;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketCreated)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      /* Calculate authorized SAP range */
      if ((psServiceName != NULL) && (psServiceName->length > 0))
      {
         /* SDP advertised service */
         min_sap_range = PHFRINFC_LLCP_SAP_SDP_ADVERTISED_FIRST;
         max_sap_range = PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST;
      }
      else
      {
         /* Non-SDP advertised service */
         min_sap_range = PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST;
         max_sap_range = PHFRINFC_LLCP_SAP_NUMBER;
      }

      /* Handle dynamic SAP allocation */
      if (nSap == 0)
      {
         status = phFriNfc_LlcpTransport_GetFreeSap(pLlcpSocket->psTransport, psServiceName, &nSap);
         if (status != NFCSTATUS_SUCCESS)
         {
            PH_LOG_LLCP_FUNC_EXIT();
            return status;
         }
      }

      /* Test the SAP range */
      if(!IS_BETWEEN(nSap, min_sap_range, max_sap_range) &&
         !IS_BETWEEN(nSap, PHFRINFC_LLCP_SAP_WKS_FIRST, PHFRINFC_LLCP_SAP_SDP_ADVERTISED_FIRST))
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      }
      else
      {
         /* Test if the nSap it is used by another socket */
         for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
         {
            if(pLlcpSocket->psTransport->pSocketTable[i].socket_sSap == nSap)
            {
               PH_LOG_LLCP_FUNC_EXIT();
               return status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_ALREADY_REGISTERED);
            }
         }
         /* Set service name */
         status = phFriNfc_LlcpTransport_RegisterName(pLlcpSocket, nSap, psServiceName);
         if (status != NFCSTATUS_SUCCESS)
         {
            PH_LOG_LLCP_FUNC_EXIT();
            return status;
         }
         /* Set the nSap value of the socket */
         pLlcpSocket->socket_sSap = nSap;
         /* Set the socket state */
         pLlcpSocket->eSocket_State = phFriNfc_LlcpTransportSocket_eSocketBound;
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Listen(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    pphFriNfc_LlcpTransportSocketListenCb_t pListen_Cb,
    _In_    void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || pListen_Cb == NULL|| pContext == NULL )
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->bSocketListenPending)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Listen(pLlcpSocket,
                                                                pListen_Cb,
                                                                pContext);
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

static
NFCSTATUS
phFriNfc_LlcpTransport_RegisterName(
    _In_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _In_ uint8_t                         nSap,
    _In_ phNfc_sData_t                   *psServiceName
    )
{
   phFriNfc_LlcpTransport_t *       psTransport = pLlcpSocket->psTransport;
   uint8_t                          index;
   uint8_t                          bSnMatch, bSapMatch;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Check in cache if sap has been used for different service name */
   for(index=0 ; index<PHFRINFC_LLCP_SDP_ADVERTISED_NB ; index++)
   {
      if(psTransport->pCachedServiceNames[index].sServiceName.length == 0)
      {
         /* Reached end of table */
         break;
      }

      bSnMatch = (phOsalNfc_MemCompare((void *)psTransport->pCachedServiceNames[index].sServiceName.buffer, (void *)psServiceName->buffer, psServiceName->length) == 0);
      bSapMatch = psTransport->pCachedServiceNames[index].nSap == nSap;
      if(bSnMatch && bSapMatch)
      {
         /* Request match cache */
         break;
      }
      else if((bSnMatch && !bSapMatch) || (!bSnMatch && bSapMatch))
      {
         /* Request mismatch with cache */
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_INVALID_PARAMETER;
      }
   }

   /* Handle service with no name */
   if (psServiceName == NULL)
   {
      if (pLlcpSocket->sServiceName.buffer != NULL)
      {
         phOsalNfc_FreeMemory(pLlcpSocket->sServiceName.buffer);
      }
      pLlcpSocket->sServiceName.buffer = NULL;
      pLlcpSocket->sServiceName.length = 0;
   }
   else
   {
      /* Check if name already in use */
      for(index=0;index<PHFRINFC_LLCP_NB_SOCKET_MAX;index++)
      {
         phFriNfc_LlcpTransport_Socket_t* pCurrentSocket = &pLlcpSocket->psTransport->pSocketTable[index];

         if(   (pCurrentSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
            && (pCurrentSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketRegistered))
         {
            /* Only bound or listening sockets may have a service name */
            continue;
         }
         if(pCurrentSocket->sServiceName.length != psServiceName->length) {
            /* Service name do not match, check next */
            continue;
         }
         if(phOsalNfc_MemCompare((void *)pCurrentSocket->sServiceName.buffer, (void *)psServiceName->buffer, psServiceName->length) == 0)
         {
            /* Service name already in use */
            PH_LOG_LLCP_FUNC_EXIT();
            return NFCSTATUS_INVALID_PARAMETER;
         }
      }

      /* Store the listen socket SN */
      pLlcpSocket->sServiceName.length = psServiceName->length;
      pLlcpSocket->sServiceName.buffer = phOsalNfc_GetMemory(psServiceName->length);
      if (pLlcpSocket->sServiceName.buffer == NULL)
      {
          PH_LOG_LLCP_FUNC_EXIT();
          return NFCSTATUS_NOT_ENOUGH_MEMORY;
      }
      phOsalNfc_MemCopy((void *)pLlcpSocket->sServiceName.buffer,(void *) psServiceName->buffer, psServiceName->length);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}

NFCSTATUS
phFriNfc_LlcpTransport_Accept(
    _In_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_ phFriNfc_LlcpTransport_sSocketOptions_t *psOptions,
    _In_ phNfc_sData_t                           *psWorkingBuffer,
    _In_ pphFriNfc_LlcpTransportSocketErrCb_t    pErr_Cb,
    _In_ pphFriNfc_LlcpTransportSocketAcceptCb_t pAccept_RspCb,
    _In_ void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || psOptions == NULL || psWorkingBuffer == NULL || pErr_Cb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(psOptions->rw > PHFRINFC_LLCP_RW_MAX)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Set the Max length for the Send and Receive Window Buffer */
      pLlcpSocket->bufferSendMaxLength   = psOptions->miu;
      pLlcpSocket->bufferRwMaxLength     = psOptions->miu * ((psOptions->rw & PHFRINFC_LLCP_TLV_RW_MASK));
      pLlcpSocket->bufferLinearLength    = psWorkingBuffer->length - pLlcpSocket->bufferSendMaxLength - pLlcpSocket->bufferRwMaxLength;

      if((pLlcpSocket->bufferSendMaxLength + pLlcpSocket->bufferRwMaxLength) > psWorkingBuffer->length
          || ((pLlcpSocket->bufferLinearLength < PHFRINFC_LLCP_MIU_DEFAULT)  && (pLlcpSocket->bufferLinearLength != 0)))
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_BUFFER_TOO_SMALL);
      }
      else
      {
         pLlcpSocket->psTransport->socketIndex = pLlcpSocket->index;

         status  = phFriNfc_LlcpTransport_ConnectionOriented_Accept(pLlcpSocket,
                                                                    psOptions,
                                                                    psWorkingBuffer,
                                                                    pErr_Cb,
                                                                    pAccept_RspCb,
                                                                    pContext);
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Reject(
    _In_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_ pphFriNfc_LlcpTransportSocketRejectCb_t pReject_RspCb,
    _In_ void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Check for socket state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   /* Check for socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phLibNfc_LlcpTransport_ConnectionOriented_Reject(pLlcpSocket,
                                                                pReject_RspCb,
                                                                pContext);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Connect(
    _In_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_ uint8_t                                     nSap,
    _In_ pphFriNfc_LlcpTransportSocketConnectCb_t    pConnect_RspCb,
    _In_ void                                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t nLocalSap;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || pConnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(nSap<02 || nSap>63)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->sServiceName.length != 0)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketCreated && pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      /* Implicit bind if socket is not already bound */
      if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
      {
         /* Bind to a free sap */
         status = phFriNfc_LlcpTransport_GetFreeSap(pLlcpSocket->psTransport, NULL, &nLocalSap);
         if (status != NFCSTATUS_SUCCESS)
         {
            PH_LOG_LLCP_FUNC_EXIT();
            return status;
         }
         pLlcpSocket->socket_sSap = nLocalSap;
         pLlcpSocket->eSocket_State = phFriNfc_LlcpTransportSocket_eSocketBound;
      }

      /* Test the SAP range for non SDP-advertised services */
      if(!IS_BETWEEN(pLlcpSocket->socket_sSap, PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST, PHFRINFC_LLCP_SAP_NUMBER))
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      }
      else
      {
         status = phFriNfc_LlcpTransport_ConnectionOriented_Connect(pLlcpSocket,
                                                                    nSap,
                                                                    NULL,
                                                                    pConnect_RspCb,
                                                                    pContext);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_ConnectByUri(
    _Inout_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_    phNfc_sData_t                               *psUri,
    _In_    pphFriNfc_LlcpTransportSocketConnectCb_t    pConnect_RspCb,
    _In_    void                                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t nLocalSap;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || pConnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketConnecting || pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(psUri->length > PHFRINFC_LLCP_SN_MAX_LENGTH)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Implicit bind if socket is not already bound */
      if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
      {
         /* Bind to a free sap */
         status = phFriNfc_LlcpTransport_GetFreeSap(pLlcpSocket->psTransport, NULL, &nLocalSap);
         if (status != NFCSTATUS_SUCCESS)
         {
            PH_LOG_LLCP_FUNC_EXIT();
            return status;
         }
         pLlcpSocket->socket_sSap = nLocalSap;
         pLlcpSocket->eSocket_State = phFriNfc_LlcpTransportSocket_eSocketBound;
      }

      /* Test the SAP range for non SDP-advertised services */
      if(!IS_BETWEEN(pLlcpSocket->socket_sSap, PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST, PHFRINFC_LLCP_SAP_NUMBER))
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      }
      else
      {
         status = phFriNfc_LlcpTransport_ConnectionOriented_Connect(pLlcpSocket,
                                                                    PHFRINFC_LLCP_SAP_DEFAULT,
                                                                    psUri,
                                                                    pConnect_RspCb,
                                                                    pContext);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Disconnect(
    _Inout_ phFriNfc_LlcpTransport_Socket_t     *pLlcpSocket,
    _In_    pphLibNfc_LlcpSocketDisconnectCb_t  pDisconnect_RspCb,
    _In_    void                                *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || pDisconnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
       status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phLibNfc_LlcpTransport_ConnectionOriented_Disconnect(pLlcpSocket,
                                                                    pDisconnect_RspCb,
                                                                    pContext);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Send(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketSendCb_t   pSend_RspCb,
    _In_    void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || psBuffer == NULL || pSend_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(psBuffer->length > pLlcpSocket->remoteMIU )
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->pfSocketSend_Cb != NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Send(pLlcpSocket,
                                                              psBuffer,
                                                              pSend_RspCb,
                                                              pContext);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_Recv(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketRecvCb_t   pRecv_RspCb,
    _In_    void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || psBuffer == NULL || pRecv_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->bSocketRecvPending == TRUE)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Recv(pLlcpSocket,
                                                              psBuffer,
                                                              pRecv_RspCb,
                                                              pContext);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_SendTo(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    uint8_t                                 nSap,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketSendCb_t   pSend_RspCb,
    _In_    void                                    *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_sLinkParameters_t  LlcpRemoteLinkParamInfo;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || psBuffer == NULL || pSend_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(nSap<2 || nSap>63)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionLess)
   {
       status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(pLlcpSocket->pfSocketSend_Cb != NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      /* Get the local parameters of the LLCP Link */
      status = phFriNfc_Llcp_GetRemoteInfo(pLlcpSocket->psTransport->pLlcp,&LlcpRemoteLinkParamInfo);
      if(status != NFCSTATUS_SUCCESS)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_FAILED);
      }
      else if(psBuffer->length > LlcpRemoteLinkParamInfo.miu)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      }
      else if(pLlcpSocket->psTransport->LinkStatusError)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
      }
      else
      {
         status = phFriNfc_LlcpTransport_Connectionless_SendTo(pLlcpSocket,
                                                               nSap,
                                                               psBuffer,
                                                               pSend_RspCb,
                                                               pContext);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpTransport_RecvFrom(
    _Inout_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_    phNfc_sData_t                               *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketRecvFromCb_t   pRecv_Cb,
    _In_    void                                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if(pLlcpSocket == NULL || psBuffer == NULL || pRecv_Cb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      if(pLlcpSocket->bSocketRecvPending)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
      }
      else
      {
         status = phLibNfc_LlcpTransport_Connectionless_RecvFrom(pLlcpSocket,
                                                                 psBuffer,
                                                                 pRecv_Cb,
                                                                 pContext);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}
