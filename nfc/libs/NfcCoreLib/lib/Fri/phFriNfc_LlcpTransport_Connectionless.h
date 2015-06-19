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

void Handle_Connectionless_IncommingFrame(phFriNfc_LlcpTransport_t      *pLlcpTransport,
                                          phNfc_sData_t                 *psData,
                                          uint8_t                       dsap,
                                          uint8_t                       ssap);

NFCSTATUS phFriNfc_LlcpTransport_Connectionless_HandlePendingOperations(phFriNfc_LlcpTransport_Socket_t *pSocket);
NFCSTATUS phFriNfc_LlcpTransport_Connectionless_Close(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket);

NFCSTATUS phFriNfc_LlcpTransport_Connectionless_SendTo(phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
                                                       uint8_t                                     nSap,
                                                       phNfc_sData_t*                              psBuffer,
                                                       pphFriNfc_LlcpTransportSocketSendCb_t       pSend_RspCb,
                                                       void*                                       pContext);

NFCSTATUS phLibNfc_LlcpTransport_Connectionless_RecvFrom(phFriNfc_LlcpTransport_Socket_t                   *pLlcpSocket,
                                                         phNfc_sData_t*                                    psBuffer,
                                                         pphFriNfc_LlcpTransportSocketRecvFromCb_t         pRecv_Cb,
                                                         void*                                             pContext);
