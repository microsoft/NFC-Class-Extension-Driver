/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phLibNfc.h>
#include <phOsalNfc.h>
#include <phLibNfc_Snep.h>

NFCSTATUS phLibNfc_SnepProtocolReq (pphLibNfc_SnepClientSession_t pClientSessionContext);

NFCSTATUS phLibNfc_SnepProtocolSendResponse (pphLibNfc_SnepServerConnection_t pSnepServerConnection);

void SnepSocketSendCb (void *pContext, NFCSTATUS status);

void SnepSocketSendData (void *pContext, NFCSTATUS status);
void SnepSocketReceiveData (void *pContext, NFCSTATUS status);
void SnepSocketSendDataSrv (void *pContext, NFCSTATUS status);
void SnepSocketReceiveDataCli (void *pContext, NFCSTATUS status);

void LlcpSocketSendResponseCb (void *pContext, NFCSTATUS status);

phNfc_sData_t* phLibNfc_PrepareSnepPacket(phLibNfc_SnepPacket_t packetType, phNfc_sData_t *pData,
                                          uint8_t version, uint32_t acceptableLength);

void LlcpSocketRecvCbForRspContinue (void* pContext,
                                     NFCSTATUS status);

void LlcpSocketRecvCbForReqContinue (void* pContext,
                                     NFCSTATUS status);

NFCSTATUS CollectReply(pphLibNfc_SnepClientSession_t pClientSessionContext);

void LlcpSocketRecvCbForRecvBegin(void* pContext,
                                  NFCSTATUS status);

NFCSTATUS GetNfcStatusFromSnepResponse(uint8_t snepCode);

uint8_t GetSnepResponseCodeFromPacketType(phLibNfc_SnepPacket_t packetType);
uint8_t GetSnepResponseCodeFromNfcStatus(NFCSTATUS nfcStatus);

phLibNfc_SnepPacket_t GetSnepPacketType(NFCSTATUS nfcStatus);

void sendSnepRequestContinue(pphLibNfc_SnepClientSession_t pClientSessionContext);
void sendSnepRequestReject(pphLibNfc_SnepClientSession_t pClientSessionContext);

void LlcpSocketSendCbForReqContinue (void *pContext, NFCSTATUS status);
void LlcpSocketSendCbForReqReject (void *pContext, NFCSTATUS status);
void LlcpSocketReceiveCb(void* pContext, NFCSTATUS status);

void phLibNfc_ClearMemNCallCb(pphLibNfc_SnepClientSession_t pClientSessionContext, NFCSTATUS status,
                              phNfc_sData_t *pReqResponse);

void phLibNfc_ClearMemNCallResponseCb(pphLibNfc_SnepServerConnection_t pSnepServerConnection,
                                      NFCSTATUS status);

void phLibNfc_NotifyUpperLayer(pphLibNfc_SnepServerConnection_t pServerConnectionContext);

void SnepSrvSendContinuecomplete (void *pContext, NFCSTATUS status, phLibNfc_Handle ConnHandle);
void SnepSrvSendcompleteInternal (void* pContext, NFCSTATUS  Status, phLibNfc_Handle ConnHandle);

BOOL areVersionsCompatible(uint8_t ver1, uint8_t ver2);

void ResetServerConnectionContext(pphLibNfc_SnepServerConnection_t pServerConnectionContext);
void ResetCliDataContext(pphLibNfc_SnepClientSession_t pClientSessionContext);

void SnepSocketRecvCbForServer(void* pContext, NFCSTATUS status);
