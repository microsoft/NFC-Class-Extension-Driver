/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSNEP.cpp

Abstract:

    SNEP Interface implementation

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxSNEP.tmh"

FORCEINLINE PNFCCX_LIBNFC_CONTEXT
NfcCxSNEPInterfaceGetLibNfcContext(
    _In_ PNFCCX_SNEP_INTERFACE  SNEPInterface
    )
{
    return SNEPInterface->FdoContext->RFInterface->pLibNfcContext;
}

FORCEINLINE PNFCCX_RF_INTERFACE
NfcCxSNEPInterfaceGetRFInterface(
    _In_ PNFCCX_SNEP_INTERFACE  SNEPInterface
    )
{
    return SNEPInterface->FdoContext->RFInterface;
}

NTSTATUS
NfcCxSNEPInterfaceCreate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_ PNFCCX_SNEP_INTERFACE * PPSNEPInterface
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *PPSNEPInterface = (PNFCCX_SNEP_INTERFACE)malloc(sizeof(NFCCX_SNEP_INTERFACE));
    if (NULL == *PPSNEPInterface) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(*PPSNEPInterface, sizeof(NFCCX_SNEP_INTERFACE));

    (*PPSNEPInterface)->FdoContext = RFInterface->FdoContext;

    (*PPSNEPInterface)->sDataInbox.buffer = (PUCHAR)malloc(MAX_INBOX_SIZE);
    if(NULL == (*PPSNEPInterface)->sDataInbox.buffer) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the SNEP inbox buffer");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    
    (*PPSNEPInterface)->sDataInbox.length = MAX_INBOX_SIZE;
    RtlZeroMemory((*PPSNEPInterface)->sDataInbox.buffer, MAX_INBOX_SIZE);

    (*PPSNEPInterface)->sConfigInfo.SnepServerType = phLibNfc_SnepServer_Default;
    (*PPSNEPInterface)->sConfigInfo.SnepServerName = NULL;
    (*PPSNEPInterface)->sConfigInfo.sOptions.miu =
                        RFInterface->pLibNfcContext->LLCPInterface->sLlcpConfigParams.uMIU;
    (*PPSNEPInterface)->sConfigInfo.sOptions.rw =
                        RFInterface->pLibNfcContext->LLCPInterface->sLlcpConfigParams.uRecvWindowSize;
    (*PPSNEPInterface)->sConfigInfo.bDtaFlag = FALSE;

Done:
    if (!NT_SUCCESS(status)) {
        if (NULL != *PPSNEPInterface) {
            NfcCxSNEPInterfaceDestroy(*PPSNEPInterface);
            *PPSNEPInterface = NULL;
        }
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}

VOID
NfcCxSNEPInterfaceDestroy(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    )
{
    if (SNEPInterface->sDataInbox.buffer != NULL) {
        free(SNEPInterface->sDataInbox.buffer);
        SNEPInterface->sDataInbox.buffer = NULL;
    }

    free(SNEPInterface);
}

static VOID
NfcCxSNEPInterfaceServerRspNtfCB(
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus, 
    _In_ phLibNfc_Handle ConnHandle
    )
{
    PNFCCX_SNEP_INTERFACE snepInterface = (PNFCCX_SNEP_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(ConnHandle);

    if (NFCSTATUS_SUCCESS == NfcStatus) {
        NfcCxRFInterfaceHandleReceivedNdefMessage(NfcCxSNEPInterfaceGetRFInterface(snepInterface), 
                                                  ReceivedNdefFromPeer,
                                                  (USHORT)snepInterface->sDataInbox.length,
                                                  &snepInterface->sDataInbox.buffer[0]);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxSNEPInterfaceServerPutNtfCB(
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus, 
    _In_ phLibNfc_Data_t *pDataInbox,
    _In_ phLibNfc_Handle ConnHandle
    )
{
    PNFCCX_SNEP_INTERFACE snepInterface = (PNFCCX_SNEP_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(ConnHandle);

    if (NfcStatus == NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_INFO, "Data received = %d bytes", pDataInbox->length);
        NfcStatus = phLibNfc_SnepProtocolSrvSendResponse(snepInterface->pConnHandleDef,
                                                         NULL,
                                                         NFCSTATUS_SUCCESS,
                                                         NfcCxSNEPInterfaceServerRspNtfCB,
                                                         (VOID *)snepInterface);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxSNEPInterfaceConnCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Handle ConnHandle
    )
{
    PNFCCX_SNEP_INTERFACE snepInterface = (PNFCCX_SNEP_INTERFACE)pContext;
    PNFCCX_RF_INTERFACE rfInterface = NfcCxSNEPInterfaceGetRFInterface(snepInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_INCOMING_CONNECTION == NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Incoming Connection %p", ConnHandle);
        snepInterface->pConnHandleDef = ConnHandle;
        NfcStatus = phLibNfc_SnepServer_Accept(&snepInterface->sDataInbox,
                                               &snepInterface->sConfigInfo.sOptions,
                                               rfInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                               snepInterface->pServerHandleDef,
                                               snepInterface->pConnHandleDef,
                                               NfcCxSNEPInterfaceServerPutNtfCB,
                                               NULL,
                                               (VOID *)snepInterface);
    }
    else if (NFCSTATUS_CONNECTION_SUCCESS == NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Connection Success");
    }
    else {
        TRACE_LINE(LEVEL_INFO, "Connection Failed, %!NFCSTATUS!", NfcStatus);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxSNEPInterfaceServerInit(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    nfcStatus = phLibNfc_SnepServer_Init(&SNEPInterface->sConfigInfo,
                                         NfcCxSNEPInterfaceConnCB,
                                         &SNEPInterface->pServerHandleDef,
                                         (VOID *)SNEPInterface);

    status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxSNEPInterfaceDeinit(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != SNEPInterface->pClientHandleDef) {
        nfcStatus = phLibNfc_SnepClient_DeInit(SNEPInterface->pClientHandleDef);
        SNEPInterface->pClientHandleDef = NULL;
        TRACE_LINE(LEVEL_INFO, "phLibNfc_SnepClient_DeInit returned %!NFCSTATUS!", nfcStatus);
    }

    if (NULL != SNEPInterface->pServerHandleDef) {
        nfcStatus = phLibNfc_SnepServer_DeInit(SNEPInterface->pServerHandleDef);
        SNEPInterface->sDataInbox.length = MAX_INBOX_SIZE;
        SNEPInterface->pServerHandleDef = NULL;
        TRACE_LINE(LEVEL_INFO, "phLibNfc_SnepServer_DeInit returned %!NFCSTATUS!", nfcStatus);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return STATUS_SUCCESS;
}

static VOID
NfcCxSNEPInterfaceClientConnCB(
    _In_ PVOID pContext, 
    _In_ NFCSTATUS NfcStatus, 
    _In_ phLibNfc_Handle ConnHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_SNEP_INTERFACE snepInterface = ((PNFCCX_SNEP_LIBNFC_REQUEST_CONTEXT)pContext)->SNEPInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepInterface->pClientHandleDef = ConnHandle;

    if (NULL != snepInterface->pClientHandleDef && NFCSTATUS_CONNECTION_SUCCESS == NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Connection Success %p", ConnHandle);
        NfcCxRFInterfaceP2pConnectionEstablished(NfcCxSNEPInterfaceGetRFInterface(snepInterface));
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "Connection Failed, %!NFCSTATUS!", NfcStatus);
        status = STATUS_UNSUCCESSFUL;
    }

    NfcCxInternalSequence(NfcCxSNEPInterfaceGetRFInterface(snepInterface), ((PNFCCX_SNEP_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxSNEPInterfaceClientConnect(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    PNFCCX_SNEP_INTERFACE snepInterface = NfcCxRFInterfaceGetSNEPInterface(RFInterface);
    static NFCCX_SNEP_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.SNEPInterface = snepInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_SnepClient_Init(&snepInterface->sConfigInfo,
                                         RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                         NfcCxSNEPInterfaceClientConnCB,
                                         &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxSNEPInterfaceClientPutReqCB(
    _In_ phLibNfc_Handle ConnHandle, 
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Data_t *pReqResponse
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_SNEP_INTERFACE snepInterface = ((PNFCCX_SNEP_LIBNFC_REQUEST_CONTEXT)pContext)->SNEPInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(pReqResponse);
    UNREFERENCED_PARAMETER(ConnHandle);

    status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    NfcCxInternalSequence(NfcCxSNEPInterfaceGetRFInterface(snepInterface), ((PNFCCX_SNEP_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxSNEPInterfaceClientPut(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    PNFCCX_SNEP_INTERFACE snepInterface = NfcCxRFInterfaceGetSNEPInterface(RFInterface);
    static NFCCX_SNEP_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.SNEPInterface = snepInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_SnepProtocolCliReqPut(snepInterface->pClientHandleDef,
                                               &snepInterface->sSendDataBuff,
                                               NfcCxSNEPInterfaceClientPutReqCB,
                                               &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}
