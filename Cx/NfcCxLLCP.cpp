/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxLLCP.cpp

Abstract:

    LLCP Interface implementation
    
Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxLLCP.tmh"

FORCEINLINE PNFCCX_LIBNFC_CONTEXT
NfcCxLLCPInterfaceGetLibNfcContext(
    _In_ PNFCCX_LLCP_INTERFACE  LLCPInterface
    )
{
    return LLCPInterface->FdoContext->RFInterface->pLibNfcContext;
}

FORCEINLINE PNFCCX_RF_INTERFACE
NfcCxLLCPInterfaceGetRFInterface(
    _In_ PNFCCX_LLCP_INTERFACE  LLCPInterface
    )
{
    return LLCPInterface->FdoContext->RFInterface;
}

NTSTATUS
NfcCxLLCPInterfaceCreate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Outptr_ PNFCCX_LLCP_INTERFACE * PPLLCPInterface
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    *PPLLCPInterface = (PNFCCX_LLCP_INTERFACE)malloc(sizeof(NFCCX_LLCP_INTERFACE));
    if (NULL == *PPLLCPInterface) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(*PPLLCPInterface, sizeof(NFCCX_LLCP_INTERFACE));

    (*PPLLCPInterface)->FdoContext = RFInterface->FdoContext;
    (*PPLLCPInterface)->sLlcpConfigParams.uMIU = NFC_CX_LLCP_MIU_DEFAULT;
    (*PPLLCPInterface)->sLlcpConfigParams.uLTO = NFC_CX_LLCP_LTO_DEFAULT;
    (*PPLLCPInterface)->sLlcpConfigParams.uRecvWindowSize = NFC_CX_LLCP_RECV_WINDOW_SIZE;
    (*PPLLCPInterface)->eLinkStatus = phFriNfc_LlcpMac_eLinkDeactivated;
    (*PPLLCPInterface)->eRequestState = NFCCX_LLCP_REQUEST_COMPLETE;

Done:
    if (!NT_SUCCESS(status)) {
        if (NULL != *PPLLCPInterface) {
            NfcCxLLCPInterfaceDestroy(*PPLLCPInterface);
            *PPLLCPInterface = NULL;
        }
    }
    return status;
}

VOID
NfcCxLLCPInterfaceDestroy(
    _In_ PNFCCX_LLCP_INTERFACE  LLCPInterface
    )
{
    free(LLCPInterface);
}

static VOID
NfcCxLLCPInterfaceCheckCB(
    _In_ VOID* pContext, 
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_LLCP_INTERFACE LLCPInterface = (PNFCCX_LLCP_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LLCPInterface->eRequestState = NFCCX_LLCP_REQUEST_COMPLETE;
    NfcCxSequenceDispatchResume(NfcCxLLCPInterfaceGetRFInterface(LLCPInterface), NfcCxLLCPInterfaceGetRFInterface(LLCPInterface)->CurrentSequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

static VOID
NfcCxLLCPInterfaceLinkStatusCB(
    _In_ VOID *pContext, 
    _In_ phFriNfc_LlcpMac_eLinkStatus_t eLinkStatus
    )
{
    PNFCCX_LLCP_INTERFACE LLCPInterface = (NFCCX_LLCP_INTERFACE*)pContext;
    PNFCCX_RF_INTERFACE RFInterface = NfcCxLLCPInterfaceGetRFInterface(LLCPInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LLCPInterface->eLinkStatus = eLinkStatus;

    TRACE_LINE(LEVEL_INFO, "Link Status: %!phFriNfc_LlcpMac_eLinkStatus_t!", eLinkStatus);

    switch (eLinkStatus)
    {
    case phFriNfc_LlcpMac_eLinkActivated:
        {
            NfcCxSNEPInterfaceServerInit(RFInterface->pLibNfcContext->SNEPInterface);
            NfcCxStateInterfaceQueueEvent(RFInterface->pLibNfcContext->StateInterface, NfcCxEventActivated, NULL, NULL, NULL);
        }
        break;

    case phFriNfc_LlcpMac_eLinkDeactivated:
        {
            NfcCxRFInterfaceP2pConnectionLost(RFInterface);
            NfcCxSNEPInterfaceDeinit(NfcCxLLCPInterfaceGetLibNfcContext(LLCPInterface)->SNEPInterface);

            if (LLCPInterface->IsDeactivatePending)
            {
                LLCPInterface->IsDeactivatePending = false;
                NfcCxSequenceDispatchResume(RFInterface, RFInterface->CurrentSequence, STATUS_SUCCESS, NULL, NULL);
            }
            else
            {
                NfcCxStateInterfaceQueueEvent(RFInterface->pLibNfcContext->StateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
            }
        }
        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxLLCPInterfaceCheck(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_LLCP_INTERFACE LLCPInterface = NfcCxRFInterfaceGetLLCPInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LLCPInterface->eRequestState = NFCCX_LLCP_REQUEST_PENDING;

    NFCSTATUS nfcStatus = phLibNfc_Llcp_CheckLlcp(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                        NfcCxLLCPInterfaceCheckCB,
                                        NfcCxLLCPInterfaceLinkStatusCB,
                                        (VOID *)LLCPInterface);

    NTSTATUS status;
    if (LLCPInterface->eRequestState == NFCCX_LLCP_REQUEST_COMPLETE) {
        status = STATUS_PENDING;
    }
    else {
        status = NfcCxNtStatusFromNfcStatus(nfcStatus);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static VOID
NfcCxLLCPInterfaceConfigureCB(
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    auto sequence = reinterpret_cast<NFCCX_CX_SEQUENCE*>(pContext);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxSequenceDispatchResume(sequence->RFInterface, sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxLLCPInterfaceConfigure(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    PNFCCX_LLCP_INTERFACE LLCPInterface = NfcCxRFInterfaceGetLLCPInterface(RFInterface);
    uint8_t ServiceName[] = "NFC Application";
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LLCPInterface->eLinkStatus = phFriNfc_LlcpMac_eLinkDefault;
    LLCPInterface->sServiceName.buffer = ServiceName;
    LLCPInterface->sServiceName.length = ARRAYSIZE(ServiceName) - 1;
    LLCPInterface->sLocalLinkInfo.miu = LLCPInterface->sLlcpConfigParams.uMIU;
    LLCPInterface->sLocalLinkInfo.lto = LLCPInterface->sLlcpConfigParams.uLTO;
    LLCPInterface->sLocalLinkInfo.wks = PHFRINFC_LLCP_WKS_DEFAULT;
    LLCPInterface->sLocalLinkInfo.option = PHFRINFC_LLCP_OPTION_DEFAULT;

    nfcStatus = phLibNfc_Mgt_SetLlcp_ConfigParams(&LLCPInterface->sLocalLinkInfo,
                                                  NfcCxLLCPInterfaceConfigureCB,
                                                  RFInterface->CurrentSequence);

    NTSTATUS status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxLLCPInterfaceActivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    nfcStatus = phLibNfc_Llcp_Activate(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev);
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxLLCPInterfaceDeactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    PNFCCX_LLCP_INTERFACE LLCPInterface = NfcCxRFInterfaceGetLLCPInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (LLCPInterface->eLinkStatus == phFriNfc_LlcpMac_eLinkActivated) {
        nfcStatus = phLibNfc_Llcp_Deactivate(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev);
        if (nfcStatus == NFCSTATUS_PENDING)
        {
            LLCPInterface->IsDeactivatePending = true;
        }
    }

    NTSTATUS status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
