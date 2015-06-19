/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

#include <phNfcHalTypes2.h>
#include <phNfcLlcpTypes.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc_Llcp.h>
#include <phFriNfc_LlcpUtils.h>


typedef uint32_t    phFriNfc_Socket_Handle;

struct phFriNfc_LlcpTransport;
typedef struct phFriNfc_LlcpTransport phFriNfc_LlcpTransport_t;

struct phFriNfc_LlcpTransport_Socket;
typedef struct phFriNfc_LlcpTransport_Socket phFriNfc_LlcpTransport_Socket_t;

struct phFriNfc_Llcp_CachedServiceName;
typedef struct phFriNfc_Llcp_CachedServiceName phFriNfc_Llcp_CachedServiceName_t;

/* Enum reperesents the different LLCP Link status*/
typedef enum phFriNfc_LlcpTransportSocket_eSocketState
{
   phFriNfc_LlcpTransportSocket_eSocketDefault,
   phFriNfc_LlcpTransportSocket_eSocketCreated,
   phFriNfc_LlcpTransportSocket_eSocketBound,
   phFriNfc_LlcpTransportSocket_eSocketRegistered,
   phFriNfc_LlcpTransportSocket_eSocketConnected,
   phFriNfc_LlcpTransportSocket_eSocketConnecting,
   phFriNfc_LlcpTransportSocket_eSocketAccepted,
   phFriNfc_LlcpTransportSocket_eSocketDisconnected,
   phFriNfc_LlcpTransportSocket_eSocketDisconnecting,
   phFriNfc_LlcpTransportSocket_eSocketRejected
}phFriNfc_LlcpTransportSocket_eSocketState_t;


typedef void (*pphFriNfc_LlcpTransportSocketErrCb_t) ( void*      pContext,
                                                       uint8_t    nErrCode);


typedef void (*pphFriNfc_LlcpTransportSocketListenCb_t) (void*                            pContext,
                                                         phFriNfc_LlcpTransport_Socket_t  *IncomingSocket);

typedef void (*pphFriNfc_LlcpTransportSocketConnectCb_t)  ( void*        pContext,
                                                            uint8_t      nErrCode,
                                                            NFCSTATUS    status);

typedef void (*pphFriNfc_LlcpTransportSocketDisconnectCb_t) (void*        pContext,
                                                             NFCSTATUS    status);

typedef void (*pphFriNfc_LlcpTransportSocketAcceptCb_t) (void*        pContext,
                                                         NFCSTATUS    status);

typedef void (*pphFriNfc_LlcpTransportSocketRejectCb_t) (void*        pContext,
                                                         NFCSTATUS    status);

typedef void (*pphFriNfc_LlcpTransportSocketRecvCb_t) (void*     pContext,
                                                       NFCSTATUS status);

typedef void (*pphFriNfc_LlcpTransportSocketRecvFromCb_t) (void*       pContext,
                                                           uint8_t     ssap,
                                                           NFCSTATUS   status);

typedef void (*pphFriNfc_LlcpTransportSocketSendCb_t) (void*        pContext,
                                                       NFCSTATUS    status);


struct phFriNfc_LlcpTransport_Socket
{
   phFriNfc_LlcpTransportSocket_eSocketState_t    eSocket_State;
   phFriNfc_LlcpTransport_eSocketType_t           eSocket_Type;
   phFriNfc_LlcpTransport_sSocketOptions_t        sSocketOption;
   pphFriNfc_LlcpTransportSocketErrCb_t           pSocketErrCb;

   /* Remote and local socket info */
   uint8_t                                        socket_sSap;
   uint8_t                                        socket_dSap;
   // TODO: copy service name (could be deallocated by upper layer)
   phNfc_sData_t                                  sServiceName;
   uint8_t                                        remoteRW;
   uint8_t                                        localRW;
   uint16_t                                       remoteMIU;
   uint16_t                                       localMIUX;
   uint8_t                                        index;

   /* SDP related fields */
   uint8_t                                       nTid;

   /* Information Flags */
   bool_t                                        bSocketRecvPending;
   bool_t                                        bSocketSendPending;
   bool_t                                        bSocketListenPending;
   bool_t                                        bSocketDiscPending;
   bool_t                                        bSocketConnectPending;
   bool_t                                        bSocketAcceptPending;
   bool_t                                        bSocketRRPending;
   bool_t                                        bSocketRNRPending;

   /* Buffers */
   phNfc_sData_t                                  sSocketSendBuffer;
   phNfc_sData_t                                  sSocketLinearBuffer;
   phNfc_sData_t*                                 sSocketRecvBuffer;
   uint32_t                                       *receivedLength;
   uint32_t                                       bufferLinearLength;
   uint32_t                                       bufferSendMaxLength;
   uint32_t                                       bufferRwMaxLength;
   bool_t                                         ReceiverBusyCondition;
   bool_t                                         RemoteBusyConditionInfo;
   UTIL_FIFO_BUFFER                               sCyclicFifoBuffer;
   uint32_t                                       indexRwRead;
   uint32_t                                       indexRwWrite;

   /* Construction Frame */
   phFriNfc_Llcp_sPacketHeader_t                  sLlcpHeader;
   phFriNfc_Llcp_sPacketSequence_t                sSequence;
   uint8_t                                        socket_VS;
   uint8_t                                        socket_VSA;
   uint8_t                                        socket_VR;
   uint8_t                                        socket_VRA;

   /* Callbacks */
   pphFriNfc_LlcpTransportSocketAcceptCb_t        pfSocketAccept_Cb;
   pphFriNfc_LlcpTransportSocketSendCb_t          pfSocketSend_Cb;
   pphFriNfc_LlcpTransportSocketRecvFromCb_t      pfSocketRecvFrom_Cb;
   pphFriNfc_LlcpTransportSocketRecvCb_t          pfSocketRecv_Cb;
   pphFriNfc_LlcpTransportSocketListenCb_t        pfSocketListen_Cb;
   pphFriNfc_LlcpTransportSocketConnectCb_t       pfSocketConnect_Cb;
   pphFriNfc_LlcpTransportSocketDisconnectCb_t    pfSocketDisconnect_Cb;

   /* Table of PHFRINFC_LLCP_RW_MAX Receive Windows Buffers */
   phNfc_sData_t                                  sSocketRwBufferTable[PHFRINFC_LLCP_RW_MAX];

   /* Pointer a the socket table */
   phFriNfc_LlcpTransport_t                       *psTransport;
   /* Context */
   void                                          *pListenContext;
   void                                          *pAcceptContext;
   void                                          *pRejectContext;
   void                                          *pConnectContext;
   void                                          *pDisonnectContext;
   void                                          *pSendContext;
   void                                          *pRecvContext;
   void                                          *pContext;
};

struct phFriNfc_Llcp_CachedServiceName
{
   phNfc_sData_t                         sServiceName;
   uint8_t                               nSap;
};

struct phFriNfc_LlcpTransport
{
   phFriNfc_LlcpTransport_Socket_t       pSocketTable[PHFRINFC_LLCP_NB_SOCKET_MAX];
   phFriNfc_Llcp_CachedServiceName_t     pCachedServiceNames[PHFRINFC_LLCP_SDP_ADVERTISED_NB];
   phFriNfc_Llcp_t                       *pLlcp;
   bool_t                                bSendPending;
   bool_t                                bRecvPending;
   bool_t                                bDmPending;
   bool_t                                bFrmrPending;

   phFriNfc_Llcp_Send_CB_t               pfLinkSendCb;
   void                                  *pLinkSendContext;

   uint8_t                               socketIndex;

   /**< Info field of pending FRMR packet*/
   uint8_t                               FrmrInfoBuffer[4];
   phFriNfc_Llcp_sPacketHeader_t         sLlcpHeader;
   phFriNfc_Llcp_sPacketSequence_t       sSequence;

  /**< Info field of pending DM packet*/
   phFriNfc_Llcp_sPacketHeader_t         sDmHeader;
   phNfc_sData_t                         sDmPayload;
   uint8_t                               DmInfoBuffer[3];

   uint8_t                               LinkStatusError;

   /**< Service discovery related infos */
   phNfc_sData_t                         *psDiscoveryServiceNameList;
   uint8_t                               *pnDiscoverySapList;
   uint8_t                               nDiscoveryListSize;
   uint8_t                               nDiscoveryReqOffset;
   uint8_t                               nDiscoveryResOffset;

   uint8_t                               nDiscoveryResTidList[PHFRINFC_LLCP_SNL_RESPONSE_MAX];
   uint8_t                               nDiscoveryResSapList[PHFRINFC_LLCP_SNL_RESPONSE_MAX];
   uint8_t                               nDiscoveryResListSize;

   uint8_t                               pDiscoveryBuffer[PHFRINFC_LLCP_MIU_DEFAULT];
   pphFriNfc_Cr_t                        pfDiscover_Cb;
   void                                  *pDiscoverContext;

};

NFCSTATUS
phFriNfc_LlcpTransport_Reset(
    _Out_   phFriNfc_LlcpTransport_t    *pLlcpTransport,
    _In_    phFriNfc_Llcp_t             *pLlcp
    );

NFCSTATUS
phFriNfc_LlcpTransport_CloseAll(
    _Inout_ phFriNfc_LlcpTransport_t *pLlcpTransport
    );

NFCSTATUS
phFriNfc_LlcpTransport_LinkSend(
    _Inout_ phFriNfc_LlcpTransport_t        *LlcpTransport,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo,
    _In_    phFriNfc_Llcp_Send_CB_t         pfSend_CB,
    _In_    void                            *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_SendDisconnectMode(
    _Inout_ phFriNfc_LlcpTransport_t    *psTransport,
    _In_    uint8_t                     dsap,
    _In_    uint8_t                     ssap,
    _In_    uint8_t                     dmOpCode
    );

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
    );

NFCSTATUS
phFriNfc_LlcpTransport_DiscoverServices(
    _Inout_ phFriNfc_LlcpTransport_t    *pLlcpTransport,
    _In_    phNfc_sData_t               *psServiceNameList,
    _In_    uint8_t                     *pnSapList,
    _In_    uint8_t                     nListSize,
    _In_    pphFriNfc_Cr_t              pDiscover_Cb,
    _In_    void                        *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_SocketGetLocalOptions(
    _In_    phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _Out_   phLibNfc_Llcp_sSocketOptions_t  *psLocalOptions
    );

NFCSTATUS
phFriNfc_LlcpTransport_SocketGetRemoteOptions(
    _In_    phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _Out_   phLibNfc_Llcp_sSocketOptions_t  *psRemoteOptions
    );

NFCSTATUS
phFriNfc_LlcpTransport_Socket(
    _Inout_ phFriNfc_LlcpTransport_t                *pLlcpTransport,
    _In_    phFriNfc_LlcpTransport_eSocketType_t    eType,
    _In_    phFriNfc_LlcpTransport_sSocketOptions_t *psOptions,
    _In_    phNfc_sData_t                           *psWorkingBuffer,
    _Outptr_ phFriNfc_LlcpTransport_Socket_t         **pLlcpSocket,
    _In_    pphFriNfc_LlcpTransportSocketErrCb_t    pErr_Cb,
    _In_    void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Close(
    _Inout_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket
    );

NFCSTATUS
phFriNfc_LlcpTransport_Bind(
    _Inout_ phFriNfc_LlcpTransport_Socket_t *pLlcpSocket,
    _In_    uint8_t                         nSap,
    _In_    phNfc_sData_t                   *psServiceName
    );

NFCSTATUS
phFriNfc_LlcpTransport_Listen(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    pphFriNfc_LlcpTransportSocketListenCb_t pListen_Cb,
    _In_    void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Accept(
    _In_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_ phFriNfc_LlcpTransport_sSocketOptions_t *psOptions,
    _In_ phNfc_sData_t                           *psWorkingBuffer,
    _In_ pphFriNfc_LlcpTransportSocketErrCb_t    pErr_Cb,
    _In_ pphFriNfc_LlcpTransportSocketAcceptCb_t pAccept_RspCb,
    _In_ void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Reject(
    _In_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_ pphFriNfc_LlcpTransportSocketRejectCb_t pReject_RspCb,
    _In_ void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Connect(
    _In_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_ uint8_t                                     nSap,
    _In_ pphFriNfc_LlcpTransportSocketConnectCb_t    pConnect_RspCb,
    _In_ void                                        *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_ConnectByUri(
    _Inout_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_    phNfc_sData_t                               *psUri,
    _In_    pphFriNfc_LlcpTransportSocketConnectCb_t    pConnect_RspCb,
    _In_    void                                        *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Disconnect(
    _Inout_ phFriNfc_LlcpTransport_Socket_t     *pLlcpSocket,
    _In_    pphLibNfc_LlcpSocketDisconnectCb_t  pDisconnect_RspCb,
    _In_    void                                *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Send(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketSendCb_t   pSend_RspCb,
    _In_    void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_Recv(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketRecvCb_t   pRecv_RspCb,
    _In_    void                                    *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_RecvFrom(
    _Inout_ phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
    _In_    phNfc_sData_t                               *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketRecvFromCb_t   pRecv_Cb,
    _In_    void                                        *pContext
    );

NFCSTATUS
phFriNfc_LlcpTransport_SendTo(
    _Inout_ phFriNfc_LlcpTransport_Socket_t         *pLlcpSocket,
    _In_    uint8_t                                 nSap,
    _In_    phNfc_sData_t                           *psBuffer,
    _In_    pphFriNfc_LlcpTransportSocketSendCb_t   pSend_RspCb,
    _In_    void                                    *pContext
    );
