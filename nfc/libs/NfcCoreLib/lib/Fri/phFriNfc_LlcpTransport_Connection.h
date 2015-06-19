/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc.h>

#include <phFriNfc_Llcp.h>

void Handle_ConnectionOriented_IncommingFrame(phFriNfc_LlcpTransport_t      *pLlcpTransport,
                                              phNfc_sData_t                 *psData,
                                              uint8_t                       dsap,
                                              uint8_t                       ptype,
                                              uint8_t                       ssap);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_HandlePendingOperations(phFriNfc_LlcpTransport_Socket_t *pSocket);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_SocketGetLocalOptions(phFriNfc_LlcpTransport_Socket_t  *pLlcpSocket,
                                                                          phLibNfc_Llcp_sSocketOptions_t   *psLocalOptions);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_SocketGetRemoteOptions(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket,
                                                                           phLibNfc_Llcp_sSocketOptions_t*    psRemoteOptions);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Close(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Listen(phFriNfc_LlcpTransport_Socket_t*          pLlcpSocket,
                                                           pphFriNfc_LlcpTransportSocketListenCb_t   pListen_Cb,
                                                           void*                                     pContext);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Accept(phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                                           phFriNfc_LlcpTransport_sSocketOptions_t*     psOptions,
                                                           phNfc_sData_t*                               psWorkingBuffer,
                                                           pphFriNfc_LlcpTransportSocketErrCb_t         pErr_Cb,
                                                           pphFriNfc_LlcpTransportSocketAcceptCb_t      pAccept_RspCb,
                                                           void*                                        pContext);

NFCSTATUS phLibNfc_LlcpTransport_ConnectionOriented_Reject( phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                                            pphFriNfc_LlcpTransportSocketRejectCb_t   pReject_RspCb,
                                                            void                                      *pContext);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Connect( phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                                             uint8_t                                    nSap,
                                                             phNfc_sData_t*                             psUri,
                                                             pphFriNfc_LlcpTransportSocketConnectCb_t   pConnect_RspCb,
                                                             void*                                      pContext);

NFCSTATUS phLibNfc_LlcpTransport_ConnectionOriented_Disconnect(phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                                               pphLibNfc_LlcpSocketDisconnectCb_t         pDisconnect_RspCb,
                                                               void*                                      pContext);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Send(phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                                         phNfc_sData_t*                               psBuffer,
                                                         pphFriNfc_LlcpTransportSocketSendCb_t        pSend_RspCb,
                                                         void*                                        pContext);

NFCSTATUS phFriNfc_LlcpTransport_ConnectionOriented_Recv( phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                                          phNfc_sData_t*                               psBuffer,
                                                          pphFriNfc_LlcpTransportSocketRecvCb_t        pRecv_RspCb,
                                                          void*                                        pContext);
