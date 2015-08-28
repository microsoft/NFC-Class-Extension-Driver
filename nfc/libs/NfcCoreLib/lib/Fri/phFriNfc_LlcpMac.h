/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcLlcpTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc.h>

#include <phFriNfc_OvrHal.h>

struct phFriNfc_LlcpMac;
typedef struct phFriNfc_LlcpMac phFriNfc_LlcpMac_t;

typedef enum phFriNfc_LlcpMac_eType
{
   phFriNfc_LlcpMac_eTypeNfcip,
   phFriNfc_LlcpMac_eTypeIso14443
}phFriNfc_LlcpMac_eType_t;

typedef enum phFriNfc_LlcpMac_ePeerType
{
   phFriNfc_LlcpMac_ePeerTypeInitiator,
   phFriNfc_LlcpMac_ePeerTypeTarget
}phFriNfc_LlcpMac_ePeerType_t;

typedef void (*phFriNfc_LlcpMac_Chk_CB_t) (void        *pContext,
                                           NFCSTATUS   status);

typedef void (*phFriNfc_LlcpMac_LinkStatus_CB_t) (void                             *pContext,
                                                  phFriNfc_LlcpMac_eLinkStatus_t   eLinkStatus,
                                                  phNfc_sData_t                    *psData,
                                                  phFriNfc_LlcpMac_ePeerType_t     PeerRemoteDevType);

typedef void (*phFriNfc_LlcpMac_Send_CB_t) (void            *pContext,
                                            NFCSTATUS       status);


typedef void (*phFriNfc_LlcpMac_Reveive_CB_t) (void               *pContext,
                                               NFCSTATUS          status,
                                               phNfc_sData_t      *psData);

typedef NFCSTATUS (*pphFriNfcLlpcMac_Chk_t) ( phFriNfc_LlcpMac_t               *LlcpMac,
                                              phFriNfc_LlcpMac_Chk_CB_t        ChkLlcpMac_Cb,
                                              void                             *pContext);

typedef NFCSTATUS (*pphFriNfcLlpcMac_Activate_t) (phFriNfc_LlcpMac_t                   *LlcpMac);

typedef NFCSTATUS (*pphFriNfcLlpcMac_Deactivate_t) (phFriNfc_LlcpMac_t                 *LlcpMac);

typedef NFCSTATUS (*pphFriNfcLlpcMac_Send_t) (phFriNfc_LlcpMac_t               *LlcpMac,
                                              phNfc_sData_t                    *psData,
                                              phFriNfc_LlcpMac_Send_CB_t       LlcpMacSend_Cb,
                                              void                             *pContext);

typedef NFCSTATUS (*pphFriNfcLlpcMac_Receive_t) (phFriNfc_LlcpMac_t               *LlcpMac,
                                                 phNfc_sData_t                    *psData,
                                                 phFriNfc_LlcpMac_Reveive_CB_t    LlcpMacReceive_Cb,
                                                 void                             *pContext);

typedef struct phFriNfc_LlcpMac_Interface
{
   pphFriNfcLlpcMac_Chk_t              chk;
   pphFriNfcLlpcMac_Activate_t         activate;
   pphFriNfcLlpcMac_Deactivate_t       deactivate;
   pphFriNfcLlpcMac_Send_t             send;
   pphFriNfcLlpcMac_Receive_t          receive;
} phFriNfc_LlcpMac_Interface_t;

struct phFriNfc_LlcpMac
{
   phFriNfc_LlcpMac_eLinkStatus_t      LinkState;
   phHal_sRemoteDevInformation_t       *psRemoteDevInfo;
   phFriNfc_LlcpMac_LinkStatus_CB_t    LinkStatus_Cb;
   void                                *LinkStatus_Context;
   phFriNfc_LlcpMac_Interface_t        LlcpMacInterface;
   phFriNfc_LlcpMac_ePeerType_t        PeerRemoteDevType;
   phFriNfc_LlcpMac_eType_t            MacType;

   /**<\internal Holds the completion routine informations of the Map Layer*/
   phFriNfc_CplRt_t                   MacCompletionInfo;
   void                               *LowerDevice;
   phFriNfc_LlcpMac_Send_CB_t         MacSend_Cb;
   void                               *MacSend_Context;
   phFriNfc_LlcpMac_Reveive_CB_t      MacReceive_Cb;
   void                               *MacReceive_Context;
   phNfc_sData_t                      *psReceiveBuffer;
   phNfc_sData_t                      *psSendBuffer;
   phNfc_sData_t                      sConfigParam;
   uint8_t                            RecvPending;
   uint8_t                            SendPending;
   uint8_t                            RecvStatus;
   phHal_uCmdList_t                   Cmd;
   phHal_sDepAdditionalInfo_t         psDepAdditionalInfo;

   /**<\ LLCP Deactivated flag*/
   bool_t                             bLlcpDeactivated;
};

NFCSTATUS
phFriNfc_LlcpMac_Reset(
    _Inout_ phFriNfc_LlcpMac_t                  *LlcpMac,
    _In_    void                                *LowerDevice,
    _In_    phFriNfc_LlcpMac_LinkStatus_CB_t    LinkStatus_Cb,
    _In_    void                                *pContext
    );

NFCSTATUS
phFriNfc_LlcpMac_ChkLlcp(
    _Inout_ phFriNfc_LlcpMac_t              *LlcpMac,
    _In_    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
    _In_    phFriNfc_LlcpMac_Chk_CB_t       ChkLlcpMac_Cb,
    _In_    void                            *pContext
    );

NFCSTATUS
phFriNfc_LlcpMac_Activate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    );

NFCSTATUS
phFriNfc_LlcpMac_Deactivate(
    _Inout_ phFriNfc_LlcpMac_t *LlcpMac
    );

NFCSTATUS
phFriNfc_LlcpMac_Send(
    _Inout_ phFriNfc_LlcpMac_t          *LlcpMac,
    _In_    phNfc_sData_t               *psData,
    _In_    phFriNfc_LlcpMac_Send_CB_t  LlcpMacSend_Cb,
    _In_    void                        *pContext
    );

NFCSTATUS
phFriNfc_LlcpMac_Receive(
    _Inout_ phFriNfc_LlcpMac_t              *LlcpMac,
    _In_    phNfc_sData_t                   *psData,
    _In_    phFriNfc_LlcpMac_Reveive_CB_t   ReceiveLlcpMac_Cb,
    _In_    void                            *pContext
    );
