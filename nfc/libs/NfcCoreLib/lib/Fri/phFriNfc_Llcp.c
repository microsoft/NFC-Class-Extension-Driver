/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include "phFriNfc_Pch.h"

#include <phNfcLlcpTypes.h>
#include <phOsalNfc_Timer.h>

#include <phFriNfc_Llcp.h>
#include <phFriNfc_LlcpUtils.h>

#include "phFriNfc_Llcp.tmh"

#define PHFRINFC_LLCP_STATE_RESET_INIT               0   /**< \internal Initial state.*/
#define PHFRINFC_LLCP_STATE_CHECKED                  1   /**< \internal The tag has been checked for LLCP compliance.*/
#define PHFRINFC_LLCP_STATE_ACTIVATION               2   /**< \internal The deactivation phase.*/
#define PHFRINFC_LLCP_STATE_PAX                      3   /**< \internal Parameter exchange phase.*/
#define PHFRINFC_LLCP_STATE_OPERATION_RECV           4   /**< \internal Normal operation phase (ready to receive).*/
#define PHFRINFC_LLCP_STATE_OPERATION_SEND           5   /**< \internal Normal operation phase (ready to send).*/
#define PHFRINFC_LLCP_STATE_DEACTIVATION             6   /**< \internal The deactivation phase.*/

#define PHFRINFC_LLCP_VERSION_MAJOR_MASK             0xF0    /**< \internal Mask to apply to get major version number.*/
#define PHFRINFC_LLCP_VERSION_MINOR_MASK             0x0F    /**< \internal Mask to apply to get major version number.*/

#define PHFRINFC_LLCP_INVALID_VERSION                0x00    /**< \internal Invalid VERSION value.*/

#define PHFRINFC_LLCP_MAX_PARAM_TLV_LENGTH \
   (( PHFRINFC_LLCP_TLV_LENGTH_HEADER + PHFRINFC_LLCP_TLV_LENGTH_VERSION ) + \
    ( PHFRINFC_LLCP_TLV_LENGTH_HEADER + PHFRINFC_LLCP_TLV_LENGTH_MIUX ) + \
    ( PHFRINFC_LLCP_TLV_LENGTH_HEADER + PHFRINFC_LLCP_TLV_LENGTH_WKS ) + \
    ( PHFRINFC_LLCP_TLV_LENGTH_HEADER + PHFRINFC_LLCP_TLV_LENGTH_LTO ) + \
    ( PHFRINFC_LLCP_TLV_LENGTH_HEADER + PHFRINFC_LLCP_TLV_LENGTH_OPT ))   /**< \internal Maximum size of link params TLV.*/

#define PHFRINFC_LLCP_SLOW_SYMMETRY                16

#ifdef ENABLE_FUZZ
void FuzzLlcpBuffer(unsigned char buffer[], int size, unsigned char **ppbuffer, DWORD *pSize);
#endif

static
void
phFriNfc_Llcp_Receive_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void            *pContext,
    _In_                                        NFCSTATUS       status,
    _In_                                        phNfc_sData_t   *psData
    );

static
NFCSTATUS
phFriNfc_Llcp_HandleIncomingPacket(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psPacket
    );

static
void
phFriNfc_Llcp_ResetLTO(
    _Inout_ phFriNfc_Llcp_t *Llcp
    );

static
NFCSTATUS
phFriNfc_Llcp_InternalSend(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo
    );

static
bool_t
phFriNfc_Llcp_HandlePendingSend(
    _Inout_ phFriNfc_Llcp_t *Llcp
    );

static
void
phFriNfc_Llcp_LinkStatus_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void                            *pContext,
    _In_                                        phFriNfc_LlcpMac_eLinkStatus_t  eLinkStatus,
    _In_                                        phNfc_sData_t                   *psParamsTLV,
    _In_                                        phFriNfc_LlcpMac_eType_t        PeerRemoteDevType
    );

_Must_inspect_impl_
_Success_(return != NULL)
__drv_allocatesMem(mem)
static
phNfc_sData_t*
phFriNfc_Llcp_AllocateAndCopy(
    _In_ const phNfc_sData_t *pOrig
    )
{
    phNfc_sData_t * pDest = NULL;

    PH_LOG_LLCP_FUNC_ENTRY();

    if (pOrig == NULL)
    {
        goto error;
    }

    pDest = phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
    if (pDest == NULL)
    {
        goto error;
    }

    pDest->buffer = phOsalNfc_GetMemory(pOrig->length);
    if (pDest->buffer == NULL)
    {
        goto error;
    }

    phOsalNfc_MemCopy(pDest->buffer, pOrig->buffer, pOrig->length);
    pDest->length = pOrig->length;

error:
    if ((pDest != NULL) && (pDest->buffer == NULL))
    {
        phOsalNfc_FreeMemory(pDest);
        pDest = NULL;
    }
    PH_LOG_LLCP_FUNC_EXIT();
    return pDest;
}

__drv_freesMem(mem)
static
void
phFriNfc_Llcp_Deallocate(
    _In_ _Post_ptr_invalid_ phNfc_sData_t *pData
    )
{
    PH_LOG_LLCP_FUNC_ENTRY();
    if (pData != NULL)
    {
        if (pData->buffer != NULL)
        {
            phOsalNfc_FreeMemory(pData->buffer);
        }
        else
        {
            PH_LOG_LLCP_WARN_STR("Warning, deallocating empty buffer");
        }
        phOsalNfc_FreeMemory(pData);
    }
    PH_LOG_LLCP_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_Llcp_InternalDeactivate(
    _Inout_ phFriNfc_Llcp_t *Llcp
    )
{
   phFriNfc_Llcp_Send_CB_t pfSendCB;
   void * pSendContext;
   NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();

   if ((Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_RECV) ||
       (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND) ||
       (Llcp->state == PHFRINFC_LLCP_STATE_PAX)            ||
       (Llcp->state == PHFRINFC_LLCP_STATE_ACTIVATION))
   {
       if (Llcp->bDtaFlag && Llcp->MAC.bLlcpDeactivated)
      {
          PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
              Llcp->state,
              PHFRINFC_LLCP_STATE_OPERATION_SEND);
          Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_SEND;
          goto Exit;
      }

      PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                            Llcp->state,
                            PHFRINFC_LLCP_STATE_DEACTIVATION);
      Llcp->state = PHFRINFC_LLCP_STATE_DEACTIVATION;

      PH_LOG_LLCP_INFO_X32MSG("Llcp_InternalDeactivate: Stopping SymmTimer timer:",Llcp->hSymmTimer);
      phOsalNfc_Timer_Stop(Llcp->hSymmTimer);

      if (Llcp->psSendInfo != NULL)
      {
         PH_LOG_LLCP_INFO_STR("Return delayed send operation in error (Llcp->psSendInfo != NULL)");
         phFriNfc_Llcp_Deallocate(Llcp->psSendInfo);
         Llcp->psSendInfo = NULL;
         Llcp->psSendHeader = NULL;
         Llcp->psSendSequence = NULL;
      }
      if (Llcp->pfSendCB != NULL)
      {
          PH_LOG_LLCP_INFO_STR("Returning: NFCSTATUS_FAILED (Llcp->pfSendCB != NULL)");
         pfSendCB = Llcp->pfSendCB;
         pSendContext = Llcp->pSendContext;
         Llcp->pfSendCB = NULL;
         Llcp->pSendContext = NULL;
         (pfSendCB)(pSendContext, NFCSTATUS_FAILED);
      }
      PH_LOG_LLCP_INFO_STR("Notifying service layer with 'phFriNfc_LlcpMac_eLinkDeactivated' status");
      Llcp->pfLink_CB(Llcp->pLinkContext, phFriNfc_LlcpMac_eLinkDeactivated);

      /* Forward check request to MAC layer */
      phFriNfc_LlcpMac_Deactivate(&Llcp->MAC);
      wStatus = phFriNfc_LlcpMac_Reset(&Llcp->MAC, Llcp->MAC.LowerDevice, (phFriNfc_LlcpMac_LinkStatus_CB_t)phFriNfc_Llcp_LinkStatus_CB, Llcp);
   }

Exit:
   PH_LOG_LLCP_FUNC_EXIT();
   return wStatus;
}


static
NFCSTATUS
phFriNfc_Llcp_SendSymm(
    _Inout_ phFriNfc_Llcp_t *Llcp
    )
{
   phFriNfc_Llcp_sPacketHeader_t sHeader;
   bool_t                        bPendingFlag;

   PH_LOG_LLCP_FUNC_ENTRY();

   bPendingFlag = phFriNfc_Llcp_HandlePendingSend(Llcp);

   if (bPendingFlag == FALSE)
   {
      /* Update activity monitor */
      if (Llcp->nSymmetryCounter < PHFRINFC_LLCP_SLOW_SYMMETRY)
      {
         Llcp->nSymmetryCounter++;
         PH_LOG_LLCP_INFO_X32MSG("Llcp->nSymmetryCounter Incremented, it value is",Llcp->nSymmetryCounter);
      }
      /* No send pending, send a SYMM instead */
      sHeader.dsap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ssap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ptype = PHFRINFC_LLCP_PTYPE_SYMM;
      PH_LOG_LLCP_FUNC_EXIT();
      return phFriNfc_Llcp_InternalSend(Llcp, &sHeader, NULL, NULL);
   }
   else
   {
      /* A pending send has been sent, there is no need to send SYMM */
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_SUCCESS;
   }
}


static
NFCSTATUS
phFriNfc_Llcp_SendPax(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phFriNfc_Llcp_sLinkParameters_t *psLinkParams
    )
{
   uint8_t                       pTLVBuffer[PHFRINFC_LLCP_MAX_PARAM_TLV_LENGTH];
   phNfc_sData_t                 sParamsTLV;
   phFriNfc_Llcp_sPacketHeader_t sHeader;
   NFCSTATUS                     result;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Prepare link parameters TLV */
   sParamsTLV.buffer = pTLVBuffer;
   sParamsTLV.length = PHFRINFC_LLCP_MAX_PARAM_TLV_LENGTH;
   result = phFriNfc_Llcp_EncodeLinkParams(&sParamsTLV, psLinkParams, PHFRINFC_LLCP_VERSION);
   if (result != NFCSTATUS_SUCCESS)
   {
      /* Error while encoding */
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_FAILED;
   }

   /* Check if ready to send */
   if (Llcp->state != PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
      /* No send pending, send the PAX packet */
      sHeader.dsap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ssap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ptype = PHFRINFC_LLCP_PTYPE_PAX;
      PH_LOG_LLCP_FUNC_EXIT();
      return phFriNfc_Llcp_InternalSend(Llcp, &sHeader, NULL, &sParamsTLV);
   }
   else
   {
      /* Error: A send is already pending, cannot send PAX */
      /* NOTE: this should not happen since PAX are sent before any other packet ! */
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_FAILED;
   }
}


static
NFCSTATUS
phFriNfc_Llcp_SendDisconnect(
    _Inout_ phFriNfc_Llcp_t *Llcp
    )
{
    phFriNfc_Llcp_sPacketHeader_t sHeader;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check if ready to send */
   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
      /* No send pending, send the DISC packet */
      sHeader.dsap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ssap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ptype = PHFRINFC_LLCP_PTYPE_DISC;
      PH_LOG_LLCP_FUNC_EXIT();
      return phFriNfc_Llcp_InternalSend(Llcp, &sHeader, NULL, NULL);
   }
   else
   {
      /* A send is already pending, raise a flag to send DISC as soon as possible */
      Llcp->bDiscPendingFlag = TRUE;
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_PENDING;
   }
}

static
void
phFriNfc_Llcp_Timer_CBRecv(
    _In_                                        uint32_t    TimerId,
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void        *pContext
    )
{
   phFriNfc_Llcp_t               *Llcp = (phFriNfc_Llcp_t*)pContext;

   PHNFC_UNUSED_VARIABLE(TimerId);
   PH_LOG_LLCP_FUNC_ENTRY();
   PH_LOG_LLCP_INFO_X32MSG("phFriNfc_Llcp_Timer_CBRecv: SymmTimer expired - TimerId:", TimerId);
   /* Check current state */
   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_RECV)
   {
        PH_LOG_LLCP_INFO_STR("No data is coming before LTO, disconnecting");
        /* No data is coming before LTO, disconnecting */
        phFriNfc_Llcp_InternalDeactivate(Llcp);
   }
   else if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
        PH_LOG_LLCP_INFO_STR("Llcp->state is 'PHFRINFC_LLCP_STATE_OPERATION_SEND', sending SymmPDU");
        /* Send SYMM */
        phFriNfc_Llcp_SendSymm(Llcp);
   }
   else
   {
        PH_LOG_LLCP_INFO_STR("phFriNfc_Llcp_Timer_CBRecv: Nothing to do if not in Normal Operation state");
        /* Nothing to do if not in Normal Operation state */
   }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_Llcp_Timer_CBSend(
    _In_                                        uint32_t    TimerId,
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void        *pContext
    )
{
   phFriNfc_Llcp_t               *Llcp = (phFriNfc_Llcp_t*)pContext;
   PH_LOG_LLCP_FUNC_ENTRY();
   UNUSED(TimerId);
   PH_LOG_LLCP_INFO_X32MSG("phFriNfc_Llcp_Timer_CBSend: SymmTimer expired - TimerId:", TimerId);
   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
        PH_LOG_LLCP_INFO_STR("Llcp->state is 'PHFRINFC_LLCP_STATE_OPERATION_SEND', sending Symm PDU...");
        /* Send SYMM */
        phFriNfc_Llcp_SendSymm(Llcp);
   }
   else
   {
        PH_LOG_LLCP_INFO_STR("phFriNfc_Llcp_Timer_CBSend: Nothing to do if not in Normal Operation state...");
        /* Nothing to do if not in Normal Operation state */
   }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_Llcp_HandleAggregatedPacket(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psRawPacket
    )
{
   phNfc_sData_t  sInfo;
   phNfc_sData_t  sCurrentInfo;
   uint16_t       length;
   NFCSTATUS      status;

   /* Check to make sure the LLCP header size is valid */
   if (psRawPacket == NULL || psRawPacket->length < PHFRINFC_LLCP_PACKET_HEADER_SIZE)
   {
       return NFCSTATUS_FAILED;
   }

   /* Get info field */
   sInfo.buffer = psRawPacket->buffer + PHFRINFC_LLCP_PACKET_HEADER_SIZE;
   sInfo.length = psRawPacket->length - PHFRINFC_LLCP_PACKET_HEADER_SIZE;

   /* Check for empty info field */
   if (sInfo.length == 0)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_FAILED;
   }

   /* Check consistency */
   while (sInfo.length != 0)
   {
      /* The encapsulated PDU should be preceded by 2 octets of LENGTH */
      if (sInfo.length < 2)
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_FAILED;
      }
      /* Read length */
      length = (sInfo.buffer[0] << 8) | sInfo.buffer[1];
      /* Update info buffer */
      sInfo.buffer += 2; /*Size of length field is 2*/
      sInfo.length -= 2; /*Size of length field is 2*/
      /* Check if declared length fits in remaining space */
      if (length > sInfo.length)
      {
         PH_LOG_LLCP_FUNC_EXIT();
         return NFCSTATUS_FAILED;
      }
      /* Update info buffer */
      sInfo.buffer += length;
      sInfo.length -= length;
   }

   /* Get info field */
   sInfo.buffer = psRawPacket->buffer + PHFRINFC_LLCP_PACKET_HEADER_SIZE;
   sInfo.length = psRawPacket->length - PHFRINFC_LLCP_PACKET_HEADER_SIZE;

   /* Handle aggregated packets */
   while (sInfo.length != 0)
   {
      /* Packet has already been checked for consistency above */
      _Analysis_assume_(sInfo.length >= 2);

      /* Read length */
      length = (sInfo.buffer[0] << 8) | sInfo.buffer[1];
      /* Update info buffer */
      sInfo.buffer += 2;        /* Size of length field is 2 */
      sInfo.length -= 2;    /*Size of length field is 2*/

      /* Packet has already been checked for consistency above */
      _Analysis_assume_(length <= sInfo.length);

      /* Handle aggregated packet */
      sCurrentInfo.buffer=sInfo.buffer;
      sCurrentInfo.length=length;
      status = phFriNfc_Llcp_HandleIncomingPacket(Llcp, &sCurrentInfo);
      if ( (status != NFCSTATUS_SUCCESS) &&
           (status != NFCSTATUS_PENDING) )
      {
         /* TODO: Error: invalid frame */
      }
      /* Update info buffer */
      sInfo.buffer += length;
      sInfo.length -= length;
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


static
NFCSTATUS
phFriNfc_Llcp_ParseLinkParams(
    _In_    phNfc_sData_t                   *psParamsTLV,
    _Out_   phFriNfc_Llcp_sLinkParameters_t *psParsedParams,
    _Out_   uint8_t                         *pnParsedVersion
    )
{
   NFCSTATUS                        status;
   uint8_t                          type;
   phFriNfc_Llcp_sLinkParameters_t  sParams;
   phNfc_sData_t                    sValueBuffer;
   uint32_t                         offset = 0;
   uint8_t                          version = PHFRINFC_LLCP_INVALID_VERSION;

   /* Check for NULL pointers */
   PH_LOG_LLCP_FUNC_ENTRY();
   if ((psParamsTLV == NULL) || (psParsedParams == NULL) || (pnParsedVersion == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Prepare default param structure */
   sParams.miu    = PHFRINFC_LLCP_MIU_DEFAULT;
   sParams.wks    = PHFRINFC_LLCP_WKS_DEFAULT;
   sParams.lto    = PHFRINFC_LLCP_LTO_DEFAULT;
   sParams.option = PHFRINFC_LLCP_OPTION_DEFAULT;

   /* Decode TLV */
   while (offset < psParamsTLV->length)
   {
      status = phFriNfc_Llcp_DecodeTLV(psParamsTLV, &offset, &type, &sValueBuffer);
      if (status != NFCSTATUS_SUCCESS)
      {
         /* Error: Ill-formed TLV */
         PH_LOG_LLCP_FUNC_EXIT();
         return status;
      }
      switch(type)
      {
         case PHFRINFC_LLCP_TLV_TYPE_VERSION:
         {
            /* Check length */
            if (sValueBuffer.length != PHFRINFC_LLCP_TLV_LENGTH_VERSION)
            {
               /* Error : Ill-formed VERSION parameter TLV */
               break;
            }
            /* Get VERSION */
            version = sValueBuffer.buffer[0];
            break;
         }
         case PHFRINFC_LLCP_TLV_TYPE_MIUX:
         {
            /* Check length */
            if (sValueBuffer.length != PHFRINFC_LLCP_TLV_LENGTH_MIUX)
            {
               /* Error : Ill-formed MIUX parameter TLV */
               break;
            }
            /* Get MIU */
            sParams.miu = PHFRINFC_LLCP_MIU_DEFAULT + (((sValueBuffer.buffer[0] << 8) | sValueBuffer.buffer[1]) & PHFRINFC_LLCP_TLV_MIUX_MASK);
            break;
         }
         case PHFRINFC_LLCP_TLV_TYPE_WKS:
         {
            /* Check length */
            if (sValueBuffer.length != PHFRINFC_LLCP_TLV_LENGTH_WKS)
            {
               /* Error : Ill-formed MIUX parameter TLV */
               break;
            }
            /* Get WKS */
            sParams.wks = (sValueBuffer.buffer[0] << 8) | sValueBuffer.buffer[1];
            /* Ignored bits must always be set */
            sParams.wks |= PHFRINFC_LLCP_TLV_WKS_MASK;
            break;
         }
         case PHFRINFC_LLCP_TLV_TYPE_LTO:
         {
            /* Check length */
            if (sValueBuffer.length != PHFRINFC_LLCP_TLV_LENGTH_LTO)
            {
               /* Error : Ill-formed LTO parameter TLV */
               break;
            }
            /* Get LTO */
            sParams.lto = sValueBuffer.buffer[0];
            break;
         }
         case PHFRINFC_LLCP_TLV_TYPE_OPT:
         {
            /* Check length */
            if (sValueBuffer.length != PHFRINFC_LLCP_TLV_LENGTH_OPT)
            {
               /* Error : Ill-formed OPT parameter TLV */
               break;
            }
            /* Get OPT */
            sParams.option = sValueBuffer.buffer[0] & PHFRINFC_LLCP_TLV_OPT_MASK;
            break;
         }
         default:
         {
            /* Error : Unknown Type */
            break;
         }
      }
   }

   /* Check if a VERSION parameter has been provided */
   if (version == PHFRINFC_LLCP_INVALID_VERSION)
   {
      /* Error : Mandatory VERSION parameter not provided */
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Save response */
   *pnParsedVersion = version;
   phOsalNfc_MemCopy(psParsedParams, &sParams, sizeof(phFriNfc_Llcp_sLinkParameters_t));

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


static
NFCSTATUS
phFriNfc_Llcp_VersionAgreement(
    _In_    uint8_t localVersion,
    _In_    uint8_t remoteVersion,
    _Out_   uint8_t *pNegociatedVersion
    )
{
   uint8_t     localMajor  = localVersion  & PHFRINFC_LLCP_VERSION_MAJOR_MASK;
   uint8_t     localMinor  = localVersion  & PHFRINFC_LLCP_VERSION_MINOR_MASK;
   uint8_t     remoteMajor = remoteVersion & PHFRINFC_LLCP_VERSION_MAJOR_MASK;
   uint8_t     remoteMinor = remoteVersion & PHFRINFC_LLCP_VERSION_MINOR_MASK;
   uint8_t     negociatedVersion;

   PH_LOG_LLCP_FUNC_ENTRY();

   if (pNegociatedVersion == NULL)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Compare Major numbers */
   if (localMajor == remoteMajor)
   {
      /* Version agreement succeed : use lowest version */
      negociatedVersion = localMajor | ((remoteMinor<localMinor)?remoteMinor:localMinor);
   }
   else if (localMajor > remoteMajor)
   {
      /* Decide if versions are compatible */
      /* Currently, there is no backward compatibility to handle */
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_FAILED;
   }
   else /* if (localMajor < remoteMajor) */
   {
      /* It is up to the remote host to decide if versions are compatible */
      /* Set negociated version to our local version, the remote will
         deacivate the link if its own version agreement fails */
      negociatedVersion = localVersion;
   }

   /* Save response */
   *pNegociatedVersion = negociatedVersion;

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


static
NFCSTATUS
phFriNfc_Llcp_InternalActivate(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psParamsTLV
    )
{
   NFCSTATUS                        status;
   phFriNfc_Llcp_sLinkParameters_t  sRemoteParams;
   uint8_t                          remoteVersion;
   uint8_t                          negociatedVersion;
   const uint16_t nMaxHeaderSize =  PHFRINFC_LLCP_PACKET_HEADER_SIZE +
                                    PHFRINFC_LLCP_PACKET_SEQUENCE_SIZE;

   PH_LOG_LLCP_FUNC_ENTRY();
  /* Parse parameters  */
   status = phFriNfc_Llcp_ParseLinkParams(psParamsTLV, &sRemoteParams, &remoteVersion);
   if (status != NFCSTATUS_SUCCESS)
   {
       PH_LOG_LLCP_INFO_STR("phFriNfc_Llcp_ParseLinkParams returned failed");
      /* Error: invalid parameters TLV */
      status = NFCSTATUS_FAILED;
   }
   else
   {
      /* Version agreement procedure */
      status = phFriNfc_Llcp_VersionAgreement(PHFRINFC_LLCP_VERSION , remoteVersion, &negociatedVersion);
      if (status != NFCSTATUS_SUCCESS)
      {
          PH_LOG_LLCP_INFO_STR("phFriNfc_Llcp_VersionAgreement returned failed");
         /* Error: version agreement failed */
         status = NFCSTATUS_FAILED;
      }
      else
      {
         /* Save parameters */
         Llcp->version = negociatedVersion;
         phOsalNfc_MemCopy(&Llcp->sRemoteParams, &sRemoteParams, sizeof(phFriNfc_Llcp_sLinkParameters_t));

         /* Update remote MIU to match local Tx buffer size */
         if (Llcp->nTxBufferLength < (Llcp->sRemoteParams.miu + nMaxHeaderSize))
         {
            Llcp->sRemoteParams.miu = Llcp->nTxBufferLength - nMaxHeaderSize;
         }
         PH_LOG_LLCP_INFO_STR("Initiate Symmetry procedure by resetting LTO timer");
         PH_LOG_LLCP_INFO_STR("Resetting LTO...");

		 /* Initialize state */
         if (Llcp->eRole == phFriNfc_LlcpMac_ePeerTypeInitiator)
         {
            PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                                  Llcp->state,
                                  PHFRINFC_LLCP_STATE_OPERATION_SEND);
            Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_SEND;

         }
         else
         {
            PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                                  Llcp->state,
                                  PHFRINFC_LLCP_STATE_OPERATION_RECV);
            Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_RECV;
         }
         /* Initiate Symmetry procedure by resetting LTO timer */
         /* NOTE: this also updates current state */
         phFriNfc_Llcp_ResetLTO(Llcp);
      }
   }

   /* Notify upper layer, if Activation failed CB called by Deactivate */
   if (status == NFCSTATUS_SUCCESS)
   {
       PH_LOG_LLCP_INFO_STR("Invoke 'pfLink_CB' with status 'phFriNfc_LlcpMac_eLinkActivated'");
      /* Link activated ! */
      Llcp->pfLink_CB(Llcp->pLinkContext, phFriNfc_LlcpMac_eLinkActivated);
   }
   PH_LOG_LLCP_INFO_X32MSG("Returning status:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}


static
NFCSTATUS
phFriNfc_Llcp_HandleMACLinkActivated(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psParamsTLV
    )
{
   NFCSTATUS                     status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   PH_LOG_LLCP_CRIT_STR("Creating LLCP 'SymmTimer' timer");
    /* Create the timer */
    Llcp->hSymmTimer = phOsalNfc_Timer_Create();
    if (Llcp->hSymmTimer == PH_OSALNFC_TIMER_ID_INVALID)
    {
        PH_LOG_LLCP_CRIT_STR("Invalid Llcp->hSymmTimer, return 'NFCSTATUS_INSUFFICIENT_RESOURCES'");
        /* Error: unable to create timer */
        PH_LOG_LLCP_FUNC_EXIT();
        return NFCSTATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        PH_LOG_LLCP_CRIT_X32MSG("SymmTimer successfully created:",Llcp->hSymmTimer);
    }

    /* Check if params received from MAC activation procedure */
    if (psParamsTLV == NULL)
    {
        PH_LOG_LLCP_INFO_STR("No params with selected MAC mapping, enter PAX mode for parameter exchange");
        PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                              Llcp->state,
                              PHFRINFC_LLCP_STATE_PAX);
        /* No params with selected MAC mapping, enter PAX mode for parameter exchange */
        Llcp->state = PHFRINFC_LLCP_STATE_PAX;
        /* Set default MIU for PAX exchange */
        Llcp->sRemoteParams.miu = PHFRINFC_LLCP_MIU_DEFAULT;
        /* If the local device is the initiator, it must initiate PAX exchange */
        if (Llcp->eRole == (phFriNfc_LlcpMac_eType_t)phFriNfc_LlcpMac_ePeerTypeInitiator)
        {
            PH_LOG_LLCP_INFO_STR("Local device is the initiator, initiating PAX exchange");
            /* Send PAX */
            status = phFriNfc_Llcp_SendPax(Llcp, &Llcp->sLocalParams);
            PH_LOG_LLCP_INFO_X32MSG("phFriNfc_Llcp_SendPax returns:",status);
        }
    }
    else
    {
        PH_LOG_LLCP_INFO_STR("Params exchanged during MAC activation, try LLC activation");
        /* Params exchanged during MAX activation, try LLC activation */
        status = phFriNfc_Llcp_InternalActivate(Llcp, psParamsTLV);
        PH_LOG_LLCP_CRIT_X32MSG("phFriNfc_Llcp_InternalActivate returns:",status);
    }

    if (status == NFCSTATUS_SUCCESS)
    {
       PH_LOG_LLCP_INFO_STR("Start listening for incoming packets...");
      /* Start listening for incoming packets */
	  Llcp->sRxBuffer.buffer = Llcp->pRxBuffer;
      Llcp->sRxBuffer.length = Llcp->nRxBufferLength;
      phFriNfc_LlcpMac_Receive(&Llcp->MAC, &Llcp->sRxBuffer, phFriNfc_Llcp_Receive_CB, Llcp);
    }
    PH_LOG_LLCP_INFO_X32MSG("Return status:",status);
    PH_LOG_LLCP_FUNC_EXIT();
    return status;
}

static
void
phFriNfc_Llcp_HandleMACLinkDeactivated(
    phFriNfc_Llcp_t *Llcp
    )
{
   uint8_t state = Llcp->state;

   PH_LOG_LLCP_FUNC_ENTRY();
  /* Delete the timer */
   if (Llcp->hSymmTimer != PH_OSALNFC_TIMER_ID_INVALID)
   {
       PH_LOG_LLCP_CRIT_X32MSG("Llcp_HandleMACLinkDeactivated: Deleting SymmTimer timer:",Llcp->hSymmTimer);
      phOsalNfc_Timer_Delete(Llcp->hSymmTimer);
	  Llcp->hSymmTimer = PH_OSALNFC_INVALID_TIMER_ID;
   }

   /* Reset state - JB Port*/
   PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                         Llcp->state,
                         PHFRINFC_LLCP_STATE_CHECKED);
   Llcp->state = PHFRINFC_LLCP_STATE_CHECKED;
   PH_LOG_LLCP_INFO_STR("Resetting Llcp->state to 'PHFRINFC_LLCP_STATE_CHECKED'");
   switch (state)
   {
      case PHFRINFC_LLCP_STATE_DEACTIVATION:
      {
         /* The service layer has already been notified, nothing more to do */
         break;
      }
      default:
      {
          PH_LOG_LLCP_INFO_STR("Notifying service layer of link failure (phFriNfc_LlcpMac_eLinkDeactivated)");
         /* Notify service layer of link failure */
         Llcp->pfLink_CB(Llcp->pLinkContext, phFriNfc_LlcpMac_eLinkDeactivated);
         break;
      }
   }
}

static
void
phFriNfc_Llcp_ChkLlcp_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void        *pContext,
    _In_                                        NFCSTATUS   status
    )
{
   /* Get monitor from context */
   phFriNfc_Llcp_t *Llcp = (phFriNfc_Llcp_t*)pContext;

   PH_LOG_LLCP_FUNC_ENTRY();
  /* Update state */
  PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                        Llcp->state,
                        PHFRINFC_LLCP_STATE_CHECKED);
   Llcp->state = PHFRINFC_LLCP_STATE_CHECKED;

   /* Invoke callback */
   Llcp->pfChk_CB(Llcp->pChkContext, status);
   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_Llcp_LinkStatus_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void                            *pContext,
    _In_                                        phFriNfc_LlcpMac_eLinkStatus_t  eLinkStatus,
    _In_                                        phNfc_sData_t                   *psParamsTLV,
    _In_                                        phFriNfc_LlcpMac_eType_t        PeerRemoteDevType
    )
{
   NFCSTATUS status;

   /* Get monitor from context */
   phFriNfc_Llcp_t *Llcp = (phFriNfc_Llcp_t*)pContext;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Save the local peer role (initiator/target) */
   Llcp->eRole = PeerRemoteDevType;

   /* Check new link status */
   switch(eLinkStatus)
   {
      case phFriNfc_LlcpMac_eLinkActivated:
      {
          PH_LOG_LLCP_INFO_STR("Entered phFriNfc_LlcpMac_eLinkActivated");
         /* Handle MAC link activation */
         status = phFriNfc_Llcp_HandleMACLinkActivated(Llcp, psParamsTLV);
         if (status != NFCSTATUS_SUCCESS)
         {
             PH_LOG_LLCP_WARN_STR("LLC link activation procedure failed, deactivate MAC link");
            /* Error: LLC link activation procedure failed, deactivate MAC link */
            status = phFriNfc_Llcp_InternalDeactivate(Llcp);
         }
         break;
      }
      case phFriNfc_LlcpMac_eLinkDeactivated:
      {
          PH_LOG_LLCP_INFO_STR("Entered phFriNfc_LlcpMac_eLinkDeactivated");
         /* Handle MAC link deactivation (cannot fail) */
         phFriNfc_Llcp_HandleMACLinkDeactivated(Llcp);
         break;
      }
      default:
      {
          PH_LOG_LLCP_WARN_STR("Entering 'default' in switch");
         /* Warning: Unknown link status, should not happen */
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_Llcp_DeferredSymm(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void    *pContext
    )
{
   phFriNfc_Llcp_t *Llcp = (phFriNfc_Llcp_t *)pContext;

   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
       phFriNfc_Llcp_SendSymm(Llcp);
   }
}


static
void
phFriNfc_Llcp_ResetLTO(
    _Inout_ phFriNfc_Llcp_t *Llcp
    )
{
   uint32_t nDuration;

   /* Stop timer */
   PH_LOG_LLCP_FUNC_ENTRY();
   PH_LOG_LLCP_INFO_X32MSG("phFriNfc_Llcp_ResetLTO: Stopping SymmTimer timer:",Llcp->hSymmTimer);
   phOsalNfc_Timer_Stop(Llcp->hSymmTimer);

   /* Calculate timer duration */
   /* NOTE: nDuration is in 1/100s, and timer system takes values in 1/1000s */
   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_RECV)
   {
      /* Response must be received before LTO announced by remote peer */
      nDuration = Llcp->sRemoteParams.lto * 10;
      /* Restart timer */
      phOsalNfc_Timer_Start(Llcp->hSymmTimer,
                            nDuration,
                            phFriNfc_Llcp_Timer_CBRecv,
                            Llcp);
   }
   else
   {
      /* Must answer before the local announced LTO */
      /* NOTE: to ensure the answer is completely sent before LTO, the
               timer is triggered _before_ LTO expiration */
      /* TODO: make sure time scope is enough, and avoid use of magic number */
      nDuration = Llcp->sLocalParams.lto;

      /* Reduce timeout when there is activity */
      if (Llcp->nSymmetryCounter >= PHFRINFC_LLCP_SLOW_SYMMETRY || Llcp->bDtaFlag)
      {
         /* Restart timer */
         phOsalNfc_Timer_Start(Llcp->hSymmTimer,
                               nDuration,
                               phFriNfc_Llcp_Timer_CBSend,
                               Llcp);
      }
      else
      {
         /* Perform deferred handling of the Symm packet after handling */
         /* everything that's currently on the stack */
         phOsalNfc_QueueDeferredCallback(phFriNfc_Llcp_DeferredSymm, Llcp);
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_Llcp_HandleLinkPacket(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psPacket
    )
{
   NFCSTATUS                        result = NFCSTATUS_FAILED;
   phFriNfc_Llcp_sPacketHeader_t    sHeader;

   /* Parse header */
   PH_LOG_LLCP_FUNC_ENTRY();
   phFriNfc_Llcp_Buffer2Header(psPacket->buffer, 0, &sHeader);

   /* Check packet type */
   switch (sHeader.ptype)
   {
      case PHFRINFC_LLCP_PTYPE_SYMM:
      {
         /* Update activity monitor */
         if (Llcp->nSymmetryCounter < PHFRINFC_LLCP_SLOW_SYMMETRY)
         {
            Llcp->nSymmetryCounter++;
            PH_LOG_LLCP_INFO_X32MSG("Llcp->nSymmetryCounter Incremented, it value is",Llcp->nSymmetryCounter);
         }
         /* Nothing to do, the LTO is handled upon all packet reception */
         result = NFCSTATUS_SUCCESS;
         break;
      }

      case PHFRINFC_LLCP_PTYPE_AGF:
      {
         /* Handle the aggregated packet */
         result = phFriNfc_Llcp_HandleAggregatedPacket(Llcp, psPacket);
         if (result != NFCSTATUS_SUCCESS)
         {
            /* Error: invalid info field, dropping frame */
         }
         break;
      }

      case PHFRINFC_LLCP_PTYPE_DISC:
      {
          Llcp->MAC.bLlcpDeactivated = TRUE;
         /* Handle link disconnection request */
         result = phFriNfc_Llcp_InternalDeactivate(Llcp);
         break;
      }


      case PHFRINFC_LLCP_PTYPE_FRMR:
      {
         /* TODO: what to do upon reception of FRMR on Link SAP ? */
         result = NFCSTATUS_SUCCESS;
         break;
      }

      case PHFRINFC_LLCP_PTYPE_PAX:
      {
         /* Ignore PAX when in Normal Operation */
         result = NFCSTATUS_SUCCESS;
         break;
      }

      default:
      {
         /* Error: invalid ptype field, dropping packet */
         break;
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}


static
NFCSTATUS
phFriNfc_Llcp_HandleTransportPacket(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psPacket
    )
{
   phFriNfc_Llcp_Recv_CB_t          pfRecvCB;
   void                             *pContext;
   NFCSTATUS                        result = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Forward to upper layer */
   if (Llcp->pfRecvCB != NULL)
   {
      /* Get callback details */
      pfRecvCB = Llcp->pfRecvCB;
      pContext = Llcp->pRecvContext;
      /* Reset callback details */
      Llcp->pfRecvCB = NULL;
      Llcp->pRecvContext = NULL;
      /* Call the callback */
      (pfRecvCB)(pContext, psPacket, NFCSTATUS_SUCCESS);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}


static
bool_t
phFriNfc_Llcp_HandlePendingSend(
    _Inout_ phFriNfc_Llcp_t *Llcp
    )
{
   phFriNfc_Llcp_sPacketHeader_t    sHeader;
   phNfc_sData_t                    sInfoBuffer;
   phFriNfc_Llcp_sPacketHeader_t    *psSendHeader = NULL;
   phFriNfc_Llcp_sPacketSequence_t  *psSendSequence = NULL;
   phNfc_sData_t                    *psSendInfo = NULL;
   NFCSTATUS                        result;
   uint8_t                          bDeallocate = FALSE;
   uint8_t                          return_value = FALSE;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Handle pending disconnection request */
   if (Llcp->bDiscPendingFlag == TRUE)
   {
       PH_LOG_LLCP_INFO_STR("Last send si acheived, send the pending DISC packet");
      /* Last send si acheived, send the pending DISC packet */
      sHeader.dsap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ssap  = PHFRINFC_LLCP_SAP_LINK;
      sHeader.ptype = PHFRINFC_LLCP_PTYPE_DISC;
      /* Set send params */
      psSendHeader = &sHeader;
      /* Reset flag */
      Llcp->bDiscPendingFlag = FALSE;
   }
   /* Handle pending frame reject request */
   else if (Llcp->bFrmrPendingFlag == TRUE)
   {
       PH_LOG_LLCP_INFO_STR("Last send si acheived, send the pending FRMR packet");
      /* Last send si acheived, send the pending FRMR packet */
      sInfoBuffer.buffer = Llcp->pFrmrInfo;
      sInfoBuffer.length = sizeof(Llcp->pFrmrInfo);
      /* Set send params */
      psSendHeader = &Llcp->sFrmrHeader;
      psSendInfo   = &sInfoBuffer;
      /* Reset flag */
      Llcp->bFrmrPendingFlag = FALSE;
   }
   /* Handle pending service frame */
   else if (Llcp->pfSendCB != NULL)
   {
       PH_LOG_LLCP_INFO_STR("Handle pending service frame");
      /* Set send params */
      psSendHeader = Llcp->psSendHeader;
      psSendSequence = Llcp->psSendSequence;
      psSendInfo = Llcp->psSendInfo;
      /* Reset pending send infos */
      Llcp->psSendHeader = NULL;
      Llcp->psSendSequence = NULL;
      Llcp->psSendInfo = NULL;
      bDeallocate = TRUE;
   }

   /* Perform send, if needed */
   if (psSendHeader != NULL)
   {
      result = phFriNfc_Llcp_InternalSend(Llcp, psSendHeader, psSendSequence, psSendInfo);
      if ((result != NFCSTATUS_SUCCESS) && (result != NFCSTATUS_PENDING))
      {
          PH_LOG_LLCP_WARN_STR("Send failed, impossible to recover");
          PH_LOG_LLCP_WARN_X32MSG("phFriNfc_Llcp_InternalSend returns status",result);
          PH_LOG_LLCP_INFO_STR("Calling 'phFriNfc_Llcp_InternalDeactivate'...");
         /* Error: send failed, impossible to recover */
         phFriNfc_Llcp_InternalDeactivate(Llcp);
      }
      return_value = TRUE;
   }

   if (bDeallocate)
   {
       phFriNfc_Llcp_Deallocate(psSendInfo);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return return_value;
}

static
NFCSTATUS
phFriNfc_Llcp_HandleIncomingPacket(
    _Inout_ phFriNfc_Llcp_t *Llcp,
    _In_    phNfc_sData_t   *psPacket
    )
{
   NFCSTATUS                        status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_sPacketHeader_t    sHeader;

   PH_LOG_LLCP_FUNC_ENTRY();
  /* Parse header */
   phFriNfc_Llcp_Buffer2Header(psPacket->buffer, 0, &sHeader);

   /* Check destination */
   if (sHeader.dsap == PHFRINFC_LLCP_SAP_LINK)
   {
      /* Handle packet as destinated to the Link SAP */
      status = phFriNfc_Llcp_HandleLinkPacket(Llcp, psPacket);
   }
   else if (sHeader.dsap >= PHFRINFC_LLCP_SAP_NUMBER)
   {
     /* NOTE: this cannot happen since "psHeader->dsap" is only 6-bit wide */
     status = NFCSTATUS_FAILED;
   }
   else
   {
       PH_LOG_LLCP_INFO_STR("Resetting 'Llcp->nSymmetryCounter' to 0");
      /* Reset activity counter */
      Llcp->nSymmetryCounter = 0;

      if (Llcp->bDtaFlag && Llcp->MAC.bLlcpDeactivated)
      {
          Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_SEND;
          PH_LOG_LLCP_CRIT_STR("-------->LLC Deactivated SENDING SYMM PDU");
      }
      else
      {
          /* Handle packet as destinated to the SDP and transport SAPs */
          status = phFriNfc_Llcp_HandleTransportPacket(Llcp, psPacket);
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}


static
void
phFriNfc_Llcp_Receive_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void            *pContext,
    _In_                                        NFCSTATUS       status,
    _In_                                        phNfc_sData_t   *psData
    )
{
    /* Get monitor from context */
    phFriNfc_Llcp_t                 *Llcp = (phFriNfc_Llcp_t*)pContext;
    NFCSTATUS                       result = NFCSTATUS_SUCCESS;
    phNfc_sData_t                   dataWithoutHeader;
    phFriNfc_Llcp_sPacketHeader_t   sPacketHeader;
    BOOLEAN                         resetLTO = TRUE;

    PH_LOG_LLCP_FUNC_ENTRY();
    /* Check reception status and for pending disconnection */
    if ((status != NFCSTATUS_SUCCESS) || (Llcp->bDiscPendingFlag == TRUE))
    {
        PH_LOG_LLCP_WARN_X32MSG("Received LLCP packet error - status", status);
        /* Reset disconnection operation */
        Llcp->bDiscPendingFlag = FALSE;
        /* Deactivate the link */
        phFriNfc_Llcp_InternalDeactivate(Llcp);
        return;
    }

#ifdef ENABLE_FUZZ
    /// Fuzz packet before pass to parser
    DWORD size=0;
    PBYTE pFuzzedBuffer = NULL;
    FuzzLlcpBuffer(psData->buffer, psData->length, &pFuzzedBuffer, &size);
    psData->length = size;
    psData->buffer = pFuzzedBuffer;
#endif

    /* Parse header */
    phFriNfc_Llcp_Buffer2Header(psData->buffer, 0, &sPacketHeader);

    if (sPacketHeader.ptype != PHFRINFC_LLCP_PTYPE_SYMM)
    {
        PH_LOG_LLCP_INFO_HEXDATA("Received LLCP packet :", psData->buffer, psData->length);
    }
    else
    {
        PH_LOG_LLCP_WARN_STR("sPacketHeader.ptype == PHFRINFC_LLCP_PTYPE_SYMM");
    }


    /* Check new link status */
    switch(Llcp->state)
    {
        /* Handle packets in PAX-waiting state */
        case PHFRINFC_LLCP_STATE_PAX:
        {
            /* Check packet type */
            if (sPacketHeader.ptype == PHFRINFC_LLCP_PTYPE_PAX)
            {
                /* Params exchanged during MAC activation, try LLC activation */
                dataWithoutHeader.buffer = psData->buffer + PHFRINFC_LLCP_PACKET_HEADER_SIZE;
                dataWithoutHeader.length = psData->length - PHFRINFC_LLCP_PACKET_HEADER_SIZE;

                result = phFriNfc_Llcp_InternalActivate(Llcp, &dataWithoutHeader);
                /* If the local LLC is the target, it must answer the PAX */
                if (Llcp->eRole == (phFriNfc_LlcpMac_eType_t)phFriNfc_LlcpMac_ePeerTypeTarget)
                {
                    /* Send PAX */
                    result = phFriNfc_Llcp_SendPax(Llcp, &Llcp->sLocalParams);
                }
            }
            else
            {
                /* Warning: Received packet with unhandled type in PAX-waiting state, drop it */
            }
        break;
        }

        /* Bad State */
        case PHFRINFC_LLCP_STATE_OPERATION_SEND:
        /*TODO handle error*/
        break;

        /* Handle normal operation packets */
        case PHFRINFC_LLCP_STATE_OPERATION_RECV:
        {
            /* Handle Symmetry procedure by resetting LTO timer */

            PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                                Llcp->state,
                                PHFRINFC_LLCP_STATE_OPERATION_SEND);
            Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_SEND;

            /* Handle packet */
            result = phFriNfc_Llcp_HandleIncomingPacket(Llcp, psData);
            if ((result != NFCSTATUS_SUCCESS) &&
                (result != NFCSTATUS_PENDING) )
            {
                /* TODO: Error: invalid frame */
            }

            //
            // If a send is already pending, then the LTO would be reset appropriately,
            // no need to do it again.
            //
            /* Perform pending send request, if any */
            resetLTO = (TRUE == phFriNfc_Llcp_HandlePendingSend(Llcp)) ? FALSE: TRUE;

            if (resetLTO) {
                phFriNfc_Llcp_ResetLTO(Llcp);
            }
        break;
        }

        default:
        {
            /* Warning: Should not receive packets in other states, drop them */
        }
    }

    /* Restart reception */
    Llcp->sRxBuffer.buffer = Llcp->pRxBuffer;
    Llcp->sRxBuffer.length = Llcp->nRxBufferLength;
    phFriNfc_LlcpMac_Receive(&Llcp->MAC, &Llcp->sRxBuffer, phFriNfc_Llcp_Receive_CB, Llcp);
}


static
void
phFriNfc_Llcp_Send_CB(
    _At_((phFriNfc_Llcp_t*)pContext, _Inout_)   void        *pContext,
    _In_                                        NFCSTATUS   status
    )
{
   /* Get monitor from context */
   phFriNfc_Llcp_t                  *Llcp = (phFriNfc_Llcp_t*)pContext;
   phFriNfc_Llcp_Send_CB_t          pfSendCB;
   void                             *pSendContext;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Call the upper layer callback if last packet sent was  */
   /* NOTE: if Llcp->psSendHeader is not NULL, this means that the send operation is still not initiated */
   if (Llcp->psSendHeader == NULL)
   {
      if (Llcp->pfSendCB != NULL)
      {
         /* Get Callback params */
         pfSendCB = Llcp->pfSendCB;
         pSendContext = Llcp->pSendContext;
         /* Reset callback params */
         Llcp->pfSendCB = NULL;
         Llcp->pSendContext = NULL;
         /* Call the callback */
         (pfSendCB)(pSendContext, status);
      }
   }

   /* Check reception status */
   if (status != NFCSTATUS_SUCCESS)
   {
       /* Error: Reception failed, link must be down */
       phFriNfc_Llcp_InternalDeactivate(Llcp);
   }
   PH_LOG_LLCP_FUNC_EXIT();
}


static
NFCSTATUS
phFriNfc_Llcp_InternalSend(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo
    )
{
   NFCSTATUS status;
   phNfc_sData_t  *psRawPacket = &Llcp->sTxBuffer; /* Use internal Tx buffer */

   PH_LOG_LLCP_FUNC_ENTRY();
   if (Llcp->state != PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
       PH_LOG_LLCP_INFO_STR("Invalid LLCP State SEND");
   }

   PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                         Llcp->state,
                         PHFRINFC_LLCP_STATE_OPERATION_RECV);
   Llcp->state = PHFRINFC_LLCP_STATE_OPERATION_RECV;
   /* Handle Symmetry procedure */
   phFriNfc_Llcp_ResetLTO(Llcp);

   /* Generate raw packet to send (aggregate header + sequence + info fields) */
   psRawPacket->length = 0;
   psRawPacket->length += phFriNfc_Llcp_Header2Buffer(psHeader, psRawPacket->buffer, psRawPacket->length);
   if (psSequence != NULL)
   {
      psRawPacket->length += phFriNfc_Llcp_Sequence2Buffer(psSequence, psRawPacket->buffer, psRawPacket->length);
   }
   if (psInfo != NULL)
   {
      phOsalNfc_MemCopy(psRawPacket->buffer + psRawPacket->length, psInfo->buffer, psInfo->length);
      psRawPacket->length += psInfo->length;
   }

   if (psHeader->ptype != PHFRINFC_LLCP_PTYPE_SYMM)
   {
      PH_LOG_LLCP_INFO_HEXDATA("Sending LLCP packet :", psRawPacket->buffer, psRawPacket->length);
   }
   else
   {
       PH_LOG_LLCP_WARN_STR("psHeader->ptype == PHFRINFC_LLCP_PTYPE_SYMM");
   }

   /* Send raw packet */
   status = phFriNfc_LlcpMac_Send (
               &Llcp->MAC,
               psRawPacket,
               phFriNfc_Llcp_Send_CB,
               Llcp );

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

/* ---------------------------- Public functions ------------------------------- */

NFCSTATUS
phFriNfc_Llcp_EncodeLinkParams(
    _In_ phNfc_sData_t                   *psRawBuffer,
    _In_ phFriNfc_Llcp_sLinkParameters_t *psLinkParams,
    _In_ uint8_t                         nVersion
    )
{
   uint32_t    nOffset = 0;
   uint16_t    miux;
   uint16_t    wks;
   uint8_t     pValue[2];
   NFCSTATUS   result = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ((psRawBuffer == NULL) || (psLinkParams == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Encode mandatory VERSION field */
   if (result == NFCSTATUS_SUCCESS)
   {
      result = phFriNfc_Llcp_EncodeTLV(
                  psRawBuffer,
                  &nOffset,
                  PHFRINFC_LLCP_TLV_TYPE_VERSION,
                  PHFRINFC_LLCP_TLV_LENGTH_VERSION,
                  &nVersion);
   }

   /* Encode mandatory VERSION field */
   if (result == NFCSTATUS_SUCCESS)
   {
      /* Encode MIUX field, if needed */
      if (psLinkParams->miu != PHFRINFC_LLCP_MIU_DEFAULT)
      {
         miux = (psLinkParams->miu - PHFRINFC_LLCP_MIU_DEFAULT) & PHFRINFC_LLCP_TLV_MIUX_MASK;
         pValue[0] = (miux >> 8) & 0xFF;
         pValue[1] =  miux       & 0xFF;
         result = phFriNfc_Llcp_EncodeTLV(
                     psRawBuffer,
                     &nOffset,
                     PHFRINFC_LLCP_TLV_TYPE_MIUX,
                     PHFRINFC_LLCP_TLV_LENGTH_MIUX,
                     pValue);
      }
   }

   /* Encode WKS field */
   if (result == NFCSTATUS_SUCCESS)
   {
      wks = psLinkParams->wks | PHFRINFC_LLCP_TLV_WKS_MASK;
      pValue[0] = (wks >> 8) & 0xFF;
      pValue[1] =  wks       & 0xFF;
      result = phFriNfc_Llcp_EncodeTLV(
                  psRawBuffer,
                  &nOffset,
                  PHFRINFC_LLCP_TLV_TYPE_WKS,
                  PHFRINFC_LLCP_TLV_LENGTH_WKS,
                  pValue);
   }

   /* Encode LTO field, if needed */
   if (result == NFCSTATUS_SUCCESS)
   {
      if (psLinkParams->lto != PHFRINFC_LLCP_LTO_DEFAULT)
      {
         result = phFriNfc_Llcp_EncodeTLV(
                     psRawBuffer,
                     &nOffset,
                     PHFRINFC_LLCP_TLV_TYPE_LTO,
                     PHFRINFC_LLCP_TLV_LENGTH_LTO,
                     &psLinkParams->lto);
      }
   }

   /* Encode OPT field, if needed */
   if (result == NFCSTATUS_SUCCESS)
   {
      if (psLinkParams->option != PHFRINFC_LLCP_OPTION_DEFAULT)
      {
         result = phFriNfc_Llcp_EncodeTLV(
                     psRawBuffer,
                     &nOffset,
                     PHFRINFC_LLCP_TLV_TYPE_OPT,
                     PHFRINFC_LLCP_TLV_LENGTH_OPT,
                     &psLinkParams->option);
      }
   }

   if (result != NFCSTATUS_SUCCESS)
   {
      /* Error: failed to encode TLV */
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_FAILED;
   }

   /* Save new buffer size */
   psRawBuffer->length = nOffset;

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}


NFCSTATUS
phFriNfc_Llcp_Reset(
    _Out_                                       phFriNfc_Llcp_t                 *Llcp,
    _In_                                        void                            *LowerDevice,
    _In_                                        phFriNfc_Llcp_sLinkParameters_t *psLinkParams,
    _Pre_writable_byte_size_(nRxBufferLength)   void                            *pRxBuffer,
    _In_                                        uint16_t                        nRxBufferLength,
    _Pre_writable_byte_size_(nTxBufferLength)   void                            *pTxBuffer,
    _In_                                        uint16_t                        nTxBufferLength,
    _In_                                        bool_t                          bDtaFlag,
    _In_                                        phFriNfc_Llcp_LinkStatus_CB_t   pfLink_CB,
    _In_                                        void                            *pContext
    )
{
   const uint16_t nMaxHeaderSize =  PHFRINFC_LLCP_PACKET_HEADER_SIZE +
                                    PHFRINFC_LLCP_PACKET_SEQUENCE_SIZE;
   NFCSTATUS result;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters presence */
   if ((Llcp == NULL) || (LowerDevice == NULL) || (pfLink_CB == NULL) ||
       (pRxBuffer == NULL) || (pTxBuffer == NULL) )
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Check parameters value */
   if (psLinkParams->miu < PHFRINFC_LLCP_MIU_DEFAULT)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* Check if buffers are large enough to support minimal MIU */
   if ((nRxBufferLength < (nMaxHeaderSize + PHFRINFC_LLCP_MIU_DEFAULT)) ||
       (nTxBufferLength < (nMaxHeaderSize + PHFRINFC_LLCP_MIU_DEFAULT)) )
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_BUFFER_TOO_SMALL;
   }

   /* Check compatibility between reception buffer size and announced MIU */
   if (nRxBufferLength < (nMaxHeaderSize + psLinkParams->miu))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_BUFFER_TOO_SMALL;
   }

   /* Start with a zero-filled monitor */
   phOsalNfc_SetMemory(Llcp, 0x00, sizeof(phFriNfc_Llcp_t));

   /* Reset the MAC Mapping layer */
   result = phFriNfc_LlcpMac_Reset(&Llcp->MAC, LowerDevice,
                    (phFriNfc_LlcpMac_LinkStatus_CB_t)phFriNfc_Llcp_LinkStatus_CB, Llcp);
   if (result != NFCSTATUS_SUCCESS) {
      PH_LOG_LLCP_FUNC_EXIT();
      return result;
   }

   Llcp->sDeferredSymm.def_call = phFriNfc_Llcp_DeferredSymm;
   Llcp->sDeferredSymm.params   = (void*)Llcp;

   /* Save the working buffers */
   Llcp->sRxBuffer.buffer = pRxBuffer;
   Llcp->sRxBuffer.length = nRxBufferLength;
   Llcp->nRxBufferLength = nRxBufferLength;
   Llcp->pRxBuffer = pRxBuffer;
   Llcp->sTxBuffer.buffer = pTxBuffer;
   Llcp->sTxBuffer.length = nTxBufferLength;
   Llcp->nTxBufferLength = nTxBufferLength;
   Llcp->bDtaFlag = bDtaFlag;

   /* Save the link status callback references */
   Llcp->pfLink_CB = pfLink_CB;
   Llcp->pLinkContext = pContext;

   /* Save the local link parameters */
   phOsalNfc_MemCopy(&Llcp->sLocalParams, psLinkParams, sizeof(phFriNfc_Llcp_sLinkParameters_t));
    PH_LOG_LLCP_INFO_STR("Resetting 'Llcp->nSymmetryCounter' to 0");
   /* Reset activity monitor */
   Llcp->nSymmetryCounter = 0;
   PH_LOG_LLCP_INFO_STR("Returning status: NFCSTATUS_SUCCESS");
   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


NFCSTATUS
phFriNfc_Llcp_ChkLlcp(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
    _In_    phFriNfc_Llcp_Check_CB_t        pfCheck_CB,
    _In_    void                            *pContext
    )
{
    PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ( (Llcp == NULL) || (psRemoteDevInfo == NULL) || (pfCheck_CB == NULL) )
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Check current state */
   if( Llcp->state != PHFRINFC_LLCP_STATE_RESET_INIT ) {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_STATE);
   }

   /* Save the compliance check callback */
   Llcp->pfChk_CB = pfCheck_CB;
   Llcp->pChkContext = pContext;

   /* Forward check request to MAC layer */
   PH_LOG_LLCP_FUNC_EXIT();
   return phFriNfc_LlcpMac_ChkLlcp(&Llcp->MAC, psRemoteDevInfo, phFriNfc_Llcp_ChkLlcp_CB, (void*)Llcp);
}


NFCSTATUS
phFriNfc_Llcp_Activate(
    _In_ phFriNfc_Llcp_t *Llcp
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LLCP_FUNC_ENTRY();
    if (Llcp == NULL)
    {
        PH_LOG_LLCP_WARN_STR("Invalid input argument!");
        PH_LOG_LLCP_WARN_STR("Returning NFCSTATUS_INVALID_PARAMETER");
        PH_LOG_LLCP_FUNC_EXIT();
        return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
    }

    if( Llcp->state != PHFRINFC_LLCP_STATE_CHECKED )
    {
        PH_LOG_LLCP_WARN_STR("Invalid llcp state!");
        PH_LOG_LLCP_WARN_STR("Returning NFCSTATUS_INVALID_STATE");
        PH_LOG_LLCP_FUNC_EXIT();
        return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_STATE);
    }

    /* Update state */
    PH_LOG_LLCP_INFO_STR("Setting LLCP state from %!PHFRINFC_LLCP_STATE! to %!PHFRINFC_LLCP_STATE!",
                          Llcp->state,
                          PHFRINFC_LLCP_STATE_ACTIVATION);
    Llcp->state = PHFRINFC_LLCP_STATE_ACTIVATION;

    wStatus = phFriNfc_LlcpMac_Activate(&Llcp->MAC);
    PH_LOG_LLCP_WARN_X32MSG("Returning:",wStatus);
    /* Forward check request to MAC layer */
    PH_LOG_LLCP_FUNC_EXIT();
    return wStatus;
}


NFCSTATUS
phFriNfc_Llcp_Deactivate(
    _In_ phFriNfc_Llcp_t *Llcp
    )
{
   NFCSTATUS status;

   PH_LOG_LLCP_FUNC_ENTRY();
   if (Llcp == NULL)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   if( (Llcp->state != PHFRINFC_LLCP_STATE_OPERATION_RECV) &&
       (Llcp->state != PHFRINFC_LLCP_STATE_OPERATION_SEND) ) {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_STATE);
   }

   /* Send DISC packet */
   status = phFriNfc_Llcp_SendDisconnect(Llcp);
   if (status == NFCSTATUS_PENDING)
   {
      /* Wait for packet to be sent before deactivate link */
      PH_LOG_LLCP_FUNC_EXIT();
      return status;
   }

   /* Perform actual deactivation */
   PH_LOG_LLCP_FUNC_EXIT();
   return phFriNfc_Llcp_InternalDeactivate(Llcp);
}


NFCSTATUS
phFriNfc_Llcp_GetLocalInfo(
    _In_    const phFriNfc_Llcp_t           *Llcp,
    _Out_   phFriNfc_Llcp_sLinkParameters_t *pParams
    )
{
    PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ((Llcp == NULL) || (pParams == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Copy response */
   phOsalNfc_MemCopy(pParams, &Llcp->sLocalParams, sizeof(phFriNfc_Llcp_sLinkParameters_t));

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


NFCSTATUS
phFriNfc_Llcp_GetRemoteInfo(
    _In_    const phFriNfc_Llcp_t           *Llcp,
    _Out_   phFriNfc_Llcp_sLinkParameters_t *pParams
    )
{
    PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ((Llcp == NULL) || (pParams == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Copy response */
   phOsalNfc_MemCopy(pParams, &Llcp->sRemoteParams, sizeof(phFriNfc_Llcp_sLinkParameters_t));

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


NFCSTATUS
phFriNfc_Llcp_Send(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo,
    _In_    phFriNfc_Llcp_Send_CB_t         pfSend_CB,
    _In_    void                            *pContext
    )
{
   NFCSTATUS result;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ((Llcp == NULL) || (psHeader == NULL) || (pfSend_CB == NULL))
   {
       PH_LOG_LLCP_INFO_STR("Invalid input parameter");
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Check if previous phFriNfc_Llcp_Send() has finished */
   if (Llcp->pfSendCB != NULL)
   {
       PH_LOG_LLCP_INFO_STR("'Llcp->pfSendCB' is Invalid");
      /* Error: a send operation is already running */
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_REJECTED);
   }

   /* Save the callback parameters */
   Llcp->pfSendCB = pfSend_CB;
   Llcp->pSendContext = pContext;
    PH_LOG_LLCP_INFO_STR("Resetting 'Llcp->nSymmetryCounter' to 0");
   /* Reset activity counter */
   Llcp->nSymmetryCounter = 0;

   if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_SEND)
   {
       PH_LOG_LLCP_INFO_STR("'Ready to send...");
      /* Ready to send */
      result = phFriNfc_Llcp_InternalSend(Llcp, psHeader, psSequence, psInfo);
   }
   else if (Llcp->state == PHFRINFC_LLCP_STATE_OPERATION_RECV)
   {
       PH_LOG_LLCP_INFO_STR("'Not ready to send, save send params for later use (as receive is in progress)");
      /* Not ready to send, save send params for later use */
      Llcp->psSendHeader = psHeader;
      Llcp->psSendSequence = psSequence;
      Llcp->psSendInfo = phFriNfc_Llcp_AllocateAndCopy(psInfo);
      result = NFCSTATUS_PENDING;
   }
   else
   {
      Llcp->pfSendCB = NULL;
      Llcp->pSendContext = NULL;
      PH_LOG_LLCP_INFO_STR("Incorrect state for sending, returning 'NFCSTATUS_INVALID_STATE'");
      /* Incorrect state for sending ! */
      result = PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_STATE);
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}


NFCSTATUS
phFriNfc_Llcp_Recv(
    _In_ phFriNfc_Llcp_t         *Llcp,
    _In_ phFriNfc_Llcp_Recv_CB_t pfRecv_CB,
    _In_ void                    *pContext
    )
{
   NFCSTATUS result = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   /* Check parameters */
   if ((Llcp == NULL) || (pfRecv_CB == NULL))
   {
       PH_LOG_LLCP_INFO_STR("Invalid input parameters");
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Check if previous phFriNfc_Llcp_Recv() has finished */
   if (Llcp->pfRecvCB != NULL)
   {
       PH_LOG_LLCP_INFO_STR("'Llcp->pfRecvCB' Invalid");
      /* Error: a send operation is already running */
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_REJECTED);
   }

   /* Save the callback parameters */
   Llcp->pfRecvCB = pfRecv_CB;
   Llcp->pRecvContext = pContext;

   /* NOTE: nothing more to do, the receive function is called in background */

   PH_LOG_LLCP_FUNC_EXIT();
   return result;
}
