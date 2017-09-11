/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"
#include "phLibNfc_Internal.h"

#include "phFriNfc_LlcpMacNfcip.tmh"

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Send(
    _Inout_ phFriNfc_LlcpMac_t          *LlcpMac,
    _In_    phNfc_sData_t               *psData,
    _In_    phFriNfc_LlcpMac_Send_CB_t  LlcpMacSend_Cb,
    _In_    void                        *pContext
    );

static
void
phFriNfc_LlcpMac_Nfcip_TriggerRecvCb(
    _Inout_ phFriNfc_LlcpMac_t  *LlcpMac,
    _In_    NFCSTATUS           status
    )
{
   phFriNfc_LlcpMac_Reveive_CB_t pfReceiveCB;
   void                          *pReceiveContext;

   PH_LOG_LLCP_FUNC_ENTRY();
   if (LlcpMac->MacReceive_Cb != NULL)
   {
      /* Save callback params */
      pfReceiveCB = LlcpMac->MacReceive_Cb;
      pReceiveContext = LlcpMac->MacReceive_Context;

      /* Reset the pointer to the Receive Callback and Context*/
      LlcpMac->MacReceive_Cb = NULL;
      LlcpMac->MacReceive_Context = NULL;

      /* Call the receive callback */
      pfReceiveCB(pReceiveContext, status, LlcpMac->psReceiveBuffer);
   }

   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_LlcpMac_Nfcip_TriggerSendCb(
    _Inout_ phFriNfc_LlcpMac_t  *LlcpMac,
    _In_    NFCSTATUS           status
    )
{
   phFriNfc_LlcpMac_Send_CB_t pfSendCB;
   void                       *pSendContext;

   PH_LOG_LLCP_FUNC_ENTRY();
   if (LlcpMac->MacSend_Cb != NULL)
   {
      /* Save context in local variables */
      pfSendCB     = LlcpMac->MacSend_Cb;
      pSendContext = LlcpMac->MacSend_Context;

      /* Reset the pointer to the Send Callback */
      LlcpMac->MacSend_Cb = NULL;
      LlcpMac->MacSend_Context = NULL;

      /* Call Send callback */
      pfSendCB(pSendContext, status);
   }

   PH_LOG_LLCP_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Chk(
    _Inout_ phFriNfc_LlcpMac_t          *LlcpMac,
    _In_    phFriNfc_LlcpMac_Chk_CB_t   ChkLlcpMac_Cb,
    _In_    void                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t Llcp_Magic_Number[] = {0x46,0x66,0x6D};

   PH_LOG_LLCP_FUNC_ENTRY();
   if(NULL == LlcpMac || NULL == ChkLlcpMac_Cb || NULL == pContext)
   {
       PH_LOG_LLCP_WARN_STR("Invalid input parameter");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = (NFCSTATUS)memcmp(Llcp_Magic_Number,LlcpMac->psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo,3);
      if(!status)
      {
         LlcpMac->sConfigParam.buffer = &LlcpMac->psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo[3] ;
         LlcpMac->sConfigParam.length = (LlcpMac->psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo_Length - 3);
         status = NFCSTATUS_SUCCESS;
      }
      else
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_FAILED);
      }
      ChkLlcpMac_Cb(pContext,status);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return status:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Activate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    )
{
   NFCSTATUS status  = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(LlcpMac == NULL)
   {
       PH_LOG_LLCP_WARN_STR("Invalid 'LlcpMac' pointer");
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
       PH_LOG_LLCP_INFO_STR("Invoking LinkStatus_Cb with link state 'phFriNfc_LlcpMac_eLinkActivated'");
      LlcpMac->LinkState = phFriNfc_LlcpMac_eLinkActivated;
      LlcpMac->LinkStatus_Cb(LlcpMac->LinkStatus_Context,
                             LlcpMac->LinkState,
                             &LlcpMac->sConfigParam,
                             LlcpMac->PeerRemoteDevType);
   }
   PH_LOG_LLCP_INFO_X32MSG("Return:",status);
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Deactivate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    )
{
   NFCSTATUS status  = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(NULL == LlcpMac)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Set the flag of LinkStatus to deactivate */
      LlcpMac->LinkState = phFriNfc_LlcpMac_eLinkDeactivated;

      if (LlcpMac->SendPending)
      {
         /* Reset Flag */
         LlcpMac->SendPending = FALSE;
         phFriNfc_LlcpMac_Nfcip_TriggerSendCb(LlcpMac, NFCSTATUS_FAILED);
      }

      if (LlcpMac->RecvPending)
      {
         /* Reset Flag */
         LlcpMac->RecvPending = FALSE;
         phFriNfc_LlcpMac_Nfcip_TriggerRecvCb(LlcpMac, NFCSTATUS_FAILED);
      }

      LlcpMac->LinkStatus_Cb(LlcpMac->LinkStatus_Context,
                             LlcpMac->LinkState,
                             NULL,
                             LlcpMac->PeerRemoteDevType);
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

static
void
phFriNfc_LlcpMac_Nfcip_Send_Cb(
    _At_((phFriNfc_LlcpMac_t*)pContext, _Inout_)    void        *pContext,
    _In_                                            NFCSTATUS   Status
    )
{
   phFriNfc_LlcpMac_t            *LlcpMac = (phFriNfc_LlcpMac_t *)pContext;
   phLibNfc_State_t State = phLibNfc_StateInvalid;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   State = phLibNfc_GetState(pLibContext);
   if(phLibNfc_StateReset == State)
   {
      Status = NFCSTATUS_SHUTDOWN;
   }
   /* Reset Send and Receive Flag */
   LlcpMac->SendPending = FALSE;
   LlcpMac->RecvPending = FALSE;

   phFriNfc_LlcpMac_Nfcip_TriggerSendCb(LlcpMac, Status);

   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_LlcpMac_Nfcip_Receive_Cb(
    _At_((phFriNfc_LlcpMac_t*)pContext, _Inout_)    void        *pContext,
    _In_                                            NFCSTATUS   Status
    )
{
   phFriNfc_LlcpMac_t               *LlcpMac = (phFriNfc_LlcpMac_t *)pContext;
   phLibNfc_State_t State;
   phFriNfc_LlcpMac_Send_CB_t       pfSendCB = NULL;
   void                             *pSendContext = NULL;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   State = phLibNfc_GetState(pLibContext);
   if(phLibNfc_StateReset == State)
   {
      Status = NFCSTATUS_SHUTDOWN;
   }

   if (NFCSTATUS_SHUTDOWN == Status)
   {
      /* Save context in local variables */
      pfSendCB = LlcpMac->MacSend_Cb;
      pSendContext = LlcpMac->MacSend_Context;

      /* Reset the pointer to the Send Callback */
      LlcpMac->MacSend_Cb = NULL;
      LlcpMac->MacSend_Context = NULL;

      /* Reset Send and Receive Flag */
      LlcpMac->SendPending = FALSE;
      LlcpMac->RecvPending = FALSE;
   }

   phFriNfc_LlcpMac_Nfcip_TriggerRecvCb(LlcpMac, Status);

   if (NFCSTATUS_SHUTDOWN == Status)
   {
       if ((LlcpMac->SendPending) && (NULL != pfSendCB))
       {
           pfSendCB(pSendContext, Status);
      }
   }
   else
   {
        /* Test if a send is pending */
        if(LlcpMac->SendPending)
        {
            Status = phFriNfc_LlcpMac_Nfcip_Send(LlcpMac,LlcpMac->psSendBuffer,LlcpMac->MacSend_Cb,LlcpMac->MacSend_Context);
        }
    }
   PH_LOG_LLCP_FUNC_EXIT();
}

static
void
phFriNfc_LlcpMac_Nfcip_Transceive_Cb(
    _At_((phFriNfc_LlcpMac_t*)pContext, _Inout_)    void        *pContext,
    _In_                                            NFCSTATUS   Status
    )
{
   phFriNfc_LlcpMac_t               *LlcpMac = (phFriNfc_LlcpMac_t *)pContext;
   phLibNfc_State_t State;
   phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

   PH_LOG_LLCP_FUNC_ENTRY();
   State = phLibNfc_GetState(pLibContext);
   if(phLibNfc_StateReset == State)
   {
      Status = NFCSTATUS_SHUTDOWN;
   }
   /* Reset Send and Receive Flag */
   LlcpMac->SendPending = FALSE;
   LlcpMac->RecvPending = FALSE;

   /* Call the callbacks */
   phFriNfc_LlcpMac_Nfcip_TriggerSendCb(LlcpMac, Status);
   phFriNfc_LlcpMac_Nfcip_TriggerRecvCb(LlcpMac, Status);

   PH_LOG_LLCP_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Send(
    _Inout_ phFriNfc_LlcpMac_t          *LlcpMac,
    _In_    phNfc_sData_t               *psData,
    _In_    phFriNfc_LlcpMac_Send_CB_t  LlcpMacSend_Cb,
    _In_    void                        *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(NULL == LlcpMac || NULL == psData || NULL == LlcpMacSend_Cb || NULL == pContext)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(LlcpMac->MacSend_Cb != NULL && LlcpMac->PeerRemoteDevType == phFriNfc_LlcpMac_ePeerTypeInitiator)
   {
      /*Previous callback is pending */
      status = NFCSTATUS_REJECTED;
   }
   else
   {
      /* Save the LlcpMacSend_Cb */
      LlcpMac->MacSend_Cb = LlcpMacSend_Cb;
      LlcpMac->MacSend_Context = pContext;

      switch(LlcpMac->PeerRemoteDevType)
      {
      case phFriNfc_LlcpMac_ePeerTypeInitiator:
         {
            if(LlcpMac->RecvPending)
            {
               /*set the completion routines for the LLCP Transceive function*/
                LlcpMac->MacCompletionInfo.CompletionRoutine = phFriNfc_LlcpMac_Nfcip_Transceive_Cb;
                LlcpMac->MacCompletionInfo.Context = LlcpMac;

                /* set the command type*/
                LlcpMac->Cmd.NfcIP1Cmd = phHal_eNfcIP1_Raw;

                /* set the Additional Info*/
                LlcpMac->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
                LlcpMac->psDepAdditionalInfo.DepFlags.NADPresent = 0;
                LlcpMac->SendPending = TRUE;

                status = phFriNfc_OvrHal_Transceive(LlcpMac->LowerDevice,
                                                    &LlcpMac->MacCompletionInfo,
                                                    LlcpMac->psRemoteDevInfo,
                                                    LlcpMac->Cmd,
                                                    &LlcpMac->psDepAdditionalInfo,
                                                    psData->buffer,
                                                    (uint16_t)psData->length,
                                                    LlcpMac->psReceiveBuffer->buffer,
                                                    (uint16_t*)&LlcpMac->psReceiveBuffer->length);
            }
            else
            {
               LlcpMac->SendPending = TRUE;
               LlcpMac->psSendBuffer = psData;
               PH_LOG_LLCP_FUNC_EXIT();
               return status = NFCSTATUS_PENDING;
            }
         }break;
      case phFriNfc_LlcpMac_ePeerTypeTarget:
         {
            if(!LlcpMac->RecvPending)
            {
               LlcpMac->SendPending = TRUE;
               LlcpMac->psSendBuffer = psData;
               PH_LOG_LLCP_FUNC_EXIT();
               return status = NFCSTATUS_PENDING;
            }
            else
            {
               /*set the completion routines for the LLCP Send function*/
               LlcpMac->MacCompletionInfo.CompletionRoutine = phFriNfc_LlcpMac_Nfcip_Send_Cb;
               LlcpMac->MacCompletionInfo.Context = LlcpMac;
               status = phFriNfc_OvrHal_Send(LlcpMac->LowerDevice,
                                             &LlcpMac->MacCompletionInfo,
                                             LlcpMac->psRemoteDevInfo,
                                             psData->buffer,
                                             (uint16_t)psData->length);
            }
         }break;
      default:
         {
            status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_DEVICE);
         }break;
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

static
NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Receive(
    _Inout_ phFriNfc_LlcpMac_t              *LlcpMac,
    _In_    phNfc_sData_t                   *psData,
    _In_    phFriNfc_LlcpMac_Reveive_CB_t   LlcpMacReceive_Cb,
    _In_    void                            *pContext
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   PH_LOG_LLCP_FUNC_ENTRY();
   if(NULL == LlcpMac || NULL==psData || NULL == LlcpMacReceive_Cb || NULL == pContext)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(LlcpMac->MacReceive_Cb != NULL)
   {
      /*Previous callback is pending */
      status = NFCSTATUS_REJECTED;
   }
   else
   {
      /* Save the LlcpMacReceive_Cb */
      LlcpMac->MacReceive_Cb = LlcpMacReceive_Cb;
      LlcpMac->MacReceive_Context = pContext;

      /* Save the pointer to the receive buffer */
      LlcpMac->psReceiveBuffer= psData;

      switch(LlcpMac->PeerRemoteDevType)
      {
      case phFriNfc_LlcpMac_ePeerTypeInitiator:
         {
            if(LlcpMac->SendPending)
            {
               /*set the completion routines for the LLCP Transceive function*/
               LlcpMac->MacCompletionInfo.CompletionRoutine = phFriNfc_LlcpMac_Nfcip_Transceive_Cb;
               LlcpMac->MacCompletionInfo.Context = LlcpMac;
               /* set the command type*/
               LlcpMac->Cmd.NfcIP1Cmd = phHal_eNfcIP1_Raw;
               /* set the Additional Info*/
               LlcpMac->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
               LlcpMac->psDepAdditionalInfo.DepFlags.NADPresent = 0;
               LlcpMac->RecvPending = TRUE;

               status = phFriNfc_OvrHal_Transceive(LlcpMac->LowerDevice,
                                                   &LlcpMac->MacCompletionInfo,
                                                   LlcpMac->psRemoteDevInfo,
                                                   LlcpMac->Cmd,
                                                   &LlcpMac->psDepAdditionalInfo,
                                                   LlcpMac->psSendBuffer->buffer,
                                                   (uint16_t)LlcpMac->psSendBuffer->length,
                                                   psData->buffer,
                                                   (uint16_t*)&psData->length);
            }
            else
            {
               LlcpMac->RecvPending = TRUE;
               PH_LOG_LLCP_FUNC_EXIT();
               return status = NFCSTATUS_PENDING;
            }
         }break;
      case phFriNfc_LlcpMac_ePeerTypeTarget:
         {
             /*set the completion routines for the LLCP Receive function*/
            LlcpMac->MacCompletionInfo.CompletionRoutine = phFriNfc_LlcpMac_Nfcip_Receive_Cb;
            /* save the context of LlcpMacNfcip */
            LlcpMac->MacCompletionInfo.Context = LlcpMac;
            LlcpMac->RecvPending = TRUE;

            status = phFriNfc_OvrHal_Receive(LlcpMac->LowerDevice,
                                             &LlcpMac->MacCompletionInfo,
                                             LlcpMac->psRemoteDevInfo,
                                             LlcpMac->psReceiveBuffer->buffer,
                                             (uint16_t*)&LlcpMac->psReceiveBuffer->length);
         }break;
      default:
         {
            status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_INVALID_DEVICE);
         }break;
      }
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}


NFCSTATUS
phFriNfc_LlcpMac_Nfcip_Register(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    )
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   PH_LOG_LLCP_FUNC_ENTRY();
   if(NULL != LlcpMac)
   {
       PH_LOG_LLCP_INFO_STR("Mapping Mac function...");
      LlcpMac->LlcpMacInterface.chk = phFriNfc_LlcpMac_Nfcip_Chk;
      LlcpMac->LlcpMacInterface.activate   = phFriNfc_LlcpMac_Nfcip_Activate;
      LlcpMac->LlcpMacInterface.deactivate = phFriNfc_LlcpMac_Nfcip_Deactivate;
      LlcpMac->LlcpMacInterface.send = phFriNfc_LlcpMac_Nfcip_Send;
      LlcpMac->LlcpMacInterface.receive = phFriNfc_LlcpMac_Nfcip_Receive;
      PH_LOG_LLCP_INFO_STR("Returning success!");
      PH_LOG_LLCP_FUNC_EXIT();
      return NFCSTATUS_SUCCESS;
   }
   else
   {
       PH_LOG_LLCP_WARN_STR("Returning failure!");
      PH_LOG_LLCP_FUNC_EXIT();
      return status = PHNFCSTVAL(CID_FRI_NFC_LLCP_MAC, NFCSTATUS_FAILED);
   }
}
