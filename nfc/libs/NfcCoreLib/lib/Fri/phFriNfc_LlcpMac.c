/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include "phFriNfc_Pch.h"

#include <phFriNfc_LlcpMac.h>
#include <phFriNfc_LlcpMacNfcip.h>
#include <phNfcStatus.h>
#include <phLibNfc.h>

#include "phFriNfc_LlcpMac.tmh"

NFCSTATUS
phFriNfc_LlcpMac_Reset(
    _Inout_ phFriNfc_LlcpMac_t                  *LlcpMac,
    _In_    void                                *LowerDevice,
    _In_    phFriNfc_LlcpMac_LinkStatus_CB_t    LinkStatus_Cb,
    _In_    void                                *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   /* Store the Linkstatus callback function of the upper layer */
   PH_LOG_LLCP_FUNC_ENTRY();
   LlcpMac->LinkStatus_Cb = LinkStatus_Cb;

   /* Store a pointer to the upper layer context */
   LlcpMac->LinkStatus_Context = pContext;

   /* Set the LinkStatus variable to the default state */
   LlcpMac->LinkState = phFriNfc_LlcpMac_eLinkDefault;

   /* Store a pointer to the lower layer */
   LlcpMac->LowerDevice =  LowerDevice;

   LlcpMac->psRemoteDevInfo         = NULL;
   LlcpMac->PeerRemoteDevType       = 0;
   LlcpMac->MacType                 = 0;
   LlcpMac->MacReceive_Cb           = NULL;
   LlcpMac->MacSend_Cb              = NULL;
   LlcpMac->psSendBuffer            = NULL;
   LlcpMac->RecvPending             = 0;
   LlcpMac->SendPending             = 0;
    PH_LOG_LLCP_INFO_X32MSG("Return status:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpMac_ChkLlcp(
    _Inout_ phFriNfc_LlcpMac_t              *LlcpMac,
    _In_    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
    _In_    phFriNfc_LlcpMac_Chk_CB_t       ChkLlcpMac_Cb,
    _In_    void                            *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   PH_LOG_LLCP_FUNC_ENTRY();
   if (NULL == LlcpMac || NULL == psRemoteDevInfo)
   {
       PH_LOG_LLCP_INFO_STR("Invalid input parameter");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Store the Remote Device info received from Device Discovery  */
      LlcpMac->psRemoteDevInfo = psRemoteDevInfo;

      if(LlcpMac->psRemoteDevInfo->RemDevType == phHal_eNfcIP1_Initiator)
      {
          PH_LOG_LLCP_INFO_STR("PeerRemoteDevType is set to phFriNfc_LlcpMac_ePeerTypeTarget");
         /* Set the PeerRemoteDevType variable to the Target type */
         LlcpMac->PeerRemoteDevType = phFriNfc_LlcpMac_ePeerTypeTarget;
      }
      else if(LlcpMac->psRemoteDevInfo->RemDevType == phHal_eNfcIP1_Target)
      {
          PH_LOG_LLCP_INFO_STR("PeerRemoteDevType is set to phFriNfc_LlcpMac_ePeerTypeInitiator");
         /* Set the PeerRemoteDevType variable to the Initiator type */
         LlcpMac->PeerRemoteDevType = phFriNfc_LlcpMac_ePeerTypeInitiator;
      }

      switch(LlcpMac->psRemoteDevInfo->RemDevType)
      {
      case phHal_eNfcIP1_Initiator:
      case phHal_eNfcIP1_Target:
         {
            /* Set the MAC mapping type detected */
            LlcpMac->MacType = phFriNfc_LlcpMac_eTypeNfcip;
            PH_LOG_LLCP_INFO_STR("Register the lower layer to the MAC mapping component");
            /* Register the lower layer to the MAC mapping component */
            status = phFriNfc_LlcpMac_Nfcip_Register (LlcpMac);
            if(status == NFCSTATUS_SUCCESS)
            {
                PH_LOG_LLCP_INFO_STR("Invoking 'phFriNfc_LlcpMac_Nfcip_Chk'");
               status  = LlcpMac->LlcpMacInterface.chk(LlcpMac,ChkLlcpMac_Cb,pContext);
            }
            else
            {
                PH_LOG_LLCP_INFO_STR("phFriNfc_LlcpMac_Nfcip_Register failed!");
               status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_FAILED);
            }
         }break;
      case phHal_eISO14443_A_PICC:
      case phHal_eISO14443_B_PICC:
         {
            /* Set the MAC mapping type detected */
            LlcpMac->MacType = phFriNfc_LlcpMac_eTypeIso14443;
            status = NFCSTATUS_SUCCESS;
         }break;
      default:
         {
            status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_DEVICE);
         }break;
      }
   }
    PH_LOG_LLCP_INFO_X32MSG("Return status:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpMac_Activate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(LlcpMac->LlcpMacInterface.activate == NULL)
   {
       PH_LOG_LLCP_WARN_STR("Invalid 'LlcpMacInterface.activate' function pointer");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = LlcpMac->LlcpMacInterface.activate(LlcpMac);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpMac_Deactivate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   PH_LOG_LLCP_FUNC_ENTRY();
   if(LlcpMac->LlcpMacInterface.deactivate == NULL)
   {
       PH_LOG_LLCP_WARN_STR("Invalid 'LlcpMacInterface.deactivate' function pointer");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
       LlcpMac->bLlcpDeactivated = FALSE;
      status = LlcpMac->LlcpMacInterface.deactivate(LlcpMac);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpMac_Send(
    _Inout_ phFriNfc_LlcpMac_t          *LlcpMac,
    _In_    phNfc_sData_t               *psData,
    _In_    phFriNfc_LlcpMac_Send_CB_t  LlcpMacSend_Cb,
    _In_    void                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   if(NULL== LlcpMac->LlcpMacInterface.send || NULL==psData || NULL==LlcpMacSend_Cb || NULL==pContext)
   {
       PH_LOG_LLCP_WARN_STR("Invalid input parameter, return NFCSTATUS_INVALID_PARAMETER");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = LlcpMac->LlcpMacInterface.send(LlcpMac,psData,LlcpMacSend_Cb,pContext);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

NFCSTATUS
phFriNfc_LlcpMac_Receive(
    _Inout_ phFriNfc_LlcpMac_t              *LlcpMac,
    _In_    phNfc_sData_t                   *psData,
    _In_    phFriNfc_LlcpMac_Reveive_CB_t   ReceiveLlcpMac_Cb,
    _In_    void                            *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(LlcpMac->LlcpMacInterface.receive == NULL || NULL==psData || NULL==ReceiveLlcpMac_Cb || NULL==pContext)
   {
       PH_LOG_LLCP_WARN_STR("Invalid input parameter, return NFCSTATUS_INVALID_PARAMETER");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = LlcpMac->LlcpMacInterface.receive(LlcpMac,psData,ReceiveLlcpMac_Cb,pContext);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}
