/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxDTAInternal.cpp

Abstract:

    DTA internal implementation

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxDTAInternal.tmh"


static VOID
NfcCxDTAInterfaceMessage(
    _Inout_ PVOID Context,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3
    );

//
// Libnfc thread
//
VOID
NfcCxDTAPostLibNfcThreadMessage(
    _Inout_ PVOID /*Context*/,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    )
/*++

Routine Description:

    This routine posts a message to the messaging thread.

Arguments:

    Context - A pointer to a memory location to receive the allocated RF interface
    Message - The type of message to be posted
    Params - Additional message-specific information

Return Value:

    VOID

--*/
{
#if defined(DISABLE_NFCCX_DTA)
    UNREFERENCED_PARAMETER(Message);
    UNREFERENCED_PARAMETER(Param1);
    UNREFERENCED_PARAMETER(Param2);
    UNREFERENCED_PARAMETER(Param3);
    UNREFERENCED_PARAMETER(Param4);
#else
    phOsalNfc_PostMsg(Message, Param1, Param2, Param3, Param4);
#endif
}

_Requires_lock_held_(DTAInterface->DeviceLock)
NTSTATUS
NfcCxDTAInterfaceExecute(
    _Inout_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    )
{
    DTAInterface->pLibNfcContext->Status = STATUS_SUCCESS;
    ResetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    NfcCxDTAPostLibNfcThreadMessage(DTAInterface, Message, Param1, Param2, Param3, Param4);
    WaitForSingleObject(DTAInterface->pLibNfcContext->hNotifyCompleteEvent, INFINITE);

    if (DTAInterface->pLibNfcContext->Status == STATUS_PENDING) {
        DTAInterface->pLibNfcContext->Status = (DTAInterface->pLibNfcContext->Status != STATUS_PENDING) ?
            DTAInterface->pLibNfcContext->Status : STATUS_CANCELLED;
    }

    if (!NT_SUCCESS(DTAInterface->pLibNfcContext->Status)) {
        TRACE_LINE(LEVEL_ERROR, "%!NFCCX_LIBNFC_MESSAGE! failed, %!STATUS!", Message, DTAInterface->pLibNfcContext->Status);
    }
    else {
        TRACE_LINE(LEVEL_INFO, "%!NFCCX_LIBNFC_MESSAGE! succeeded", Message);
    }

    return DTAInterface->pLibNfcContext->Status;
}

FORCEINLINE NTSTATUS
NfcCxDTAInterfaceGetSEEvent(
    _In_ phLibNfc_eSE_EvtType_t LibNfcEventType,
    _Out_ NFC_SE_EVENT_TYPE* pSEEventType
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (LibNfcEventType) {
    case phLibNfc_eSE_EvtReaderArrival:
        *pSEEventType = ExternalReaderArrival;
        break;
    case phLibNfc_eSE_EvtReaderDeparture:
        *pSEEventType = ExternalReaderDeparture;
        break;
    case phLibNfc_eSE_EvtTypeTransaction:
        *pSEEventType = Transaction;
        break;
    case phLibNfc_eSE_EvtRfFieldEnter:
        *pSEEventType = ExternalFieldEnter;
        break;
    case phLibNfc_eSE_EvtRfFieldExit:
        *pSEEventType = ExternalFieldExit;
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    return status;
}

static VOID
NfcCxDTAInterfaceHandleSEEvent(
    _In_ PVOID pContext,
    _In_ phLibNfc_eSE_EvtType_t EventType,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _In_ NFCSTATUS Status
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;
    NFC_SE_EVENT_TYPE seEventType;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "SE event received hSecureElement=%p, EventType=%!phLibNfc_eSE_EvtType_t!", 
                            hSecureElement, 
                            EventType);

    if (Status == NFCSTATUS_SUCCESS) {
        status = NfcCxDTAInterfaceGetSEEvent(EventType, &seEventType);

        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_WARNING, "Unsupported EventType=%!phLibNfc_eSE_EvtType_t!", EventType);
            goto Done;
        }

        NfcCxDTAInterfaceHandleSeEvent(dtaInterface,
                                       seEventType,
                                       hSecureElement,
                                       pSeEvtInfo);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevRecvCB(
    void*               pContext,
    phNfc_sData_t*      pRecvBufferInfo,
    NFCSTATUS           NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus && 
        NFCSTATUS_RF_ERROR != NfcStatus) {
        TRACE_LINE(LEVEL_WARNING, "Failed with status %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    if (NFCSTATUS_RF_ERROR == NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Received RF Error, queuing new Recv for device 0x%p", dtaInterface->hRemoteDev);
        status = phLibNfc_RemoteDev_Receive(dtaInterface->hRemoteDev,
                                            NfcCxDTAInterfaceRemoteDevRecvCB,
                                            dtaInterface);
        if (NFCSTATUS_PENDING != status) {
            TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive failed %!NFCSTATUS!", status);
        }
        goto Done;
    }

    if (0 == pRecvBufferInfo->length) {
        TRACE_LINE(LEVEL_WARNING, "Invalid APDU of size zero!");
        goto Done;
    }

    NfcCxDTAInterfaceHandleRemoteDevRecv(dtaInterface,
                                         dtaInterface->hRemoteDev,
                                         pRecvBufferInfo->buffer,
                                         (USHORT) pRecvBufferInfo->length);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceHandleSEDeviceHostEvent(
    _In_ PVOID pContext,
    _In_ phLibNfc_RemoteDevList_t* pRemoteDevList,
    _In_ uint8_t uiNoOfRemoteDev,
    _In_ NFCSTATUS Status
)
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE) pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (Status != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_WARNING, "Remote device error status %!NFCSTATUS!", Status);
        goto Done;
    }

    if (1 != uiNoOfRemoteDev) {
        TRACE_LINE(LEVEL_WARNING, "Invalid RemoteDev %d", uiNoOfRemoteDev);
        goto Done;
    }

    dtaInterface->hRemoteDev = pRemoteDevList[0].hTargetDev;
    Status = phLibNfc_RemoteDev_Receive(pRemoteDevList[0].hTargetDev,
                                        NfcCxDTAInterfaceRemoteDevRecvCB,
                                        dtaInterface);

    if (NFCSTATUS_PENDING != Status) {
        TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive retruns %!NFCSTATUS!", Status);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpLinkStatusEvent(
    _In_ PVOID Context, 
    _In_ phFriNfc_LlcpMac_eLinkStatus_t LinkStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->eLinkStatus = LinkStatus;

    if (phFriNfc_LlcpMac_eLinkActivated == LinkStatus ||
        phFriNfc_LlcpMac_eLinkDeactivated == LinkStatus) {

        NfcCxDTAInterfaceHandleLlcpLinkStatus(dtaInterface, LinkStatus);
    }
    else {
        TRACE_LINE(LEVEL_INFO, "Unsupported link status %d", LinkStatus);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpAutoLinkStatusCheckCB(
    _In_ PVOID Context,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->bLlcpLinkStatusEnabled = (NFCSTATUS_SUCCESS == NfcStatus);
    if (!dtaInterface->bLlcpLinkStatusEnabled) {
        // Generate link deactivated event for upper layer
        NfcCxDTAInterfaceLlcpLinkStatusEvent(Context, phFriNfc_LlcpMac_eLinkDeactivated);
        TRACE_LINE(LEVEL_WARNING, "LLCP Check failed %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    NfcStatus = phLibNfc_Llcp_Activate(dtaInterface->hRemoteDev);
    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_WARNING, "phLibNfc_Llcp_Activate failed %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpAutoActivate(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (DTAInterface->bLlcpLinkStatusEnabled) {
        TRACE_LINE(LEVEL_INFO, "LLCP check is already enabled");
        NfcCxDTAInterfaceLlcpAutoLinkStatusCheckCB(DTAInterface, NFCSTATUS_SUCCESS);
        goto Done;
    }

    status = phLibNfc_Llcp_CheckLlcp(DTAInterface->hRemoteDev,
                                     NfcCxDTAInterfaceLlcpAutoLinkStatusCheckCB,
                                     NfcCxDTAInterfaceLlcpLinkStatusEvent,
                                     DTAInterface);

    if ((NFCSTATUS_PENDING != status) && (NFCSTATUS_SUCCESS != status)) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_CheckLlcp completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpAutoLinkStatusCheckCB(DTAInterface, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNtfCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_RemoteDevList_t *psRemoteDevList,
    _In_ uint8_t uNofRemoteDev,
    _In_ NFCSTATUS NfcStatus
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_RemoteDevList_t *psRemoteDev = psRemoteDevList;
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(pContext);

    TRACE_LINE(LEVEL_INFO, "NfcStatus=%!NFCSTATUS! uNofRemoteDev=%d", NfcStatus, uNofRemoteDev);

    if ((NFCSTATUS_SUCCESS == NfcStatus) ||
        (NFCSTATUS_MULTIPLE_PROTOCOLS == NfcStatus) ||
        (NFCSTATUS_MULTIPLE_TAGS == NfcStatus)) {
        TRACE_LINE(LEVEL_INFO, "RemDevType=%!phNfc_eRFDevType_t!", psRemoteDev->psRemoteDevInfo->RemDevType);

        dtaInterface->hRemoteDev = psRemoteDev->hTargetDev;
        dtaInterface->bLlcpLinkStatusEnabled = FALSE;

        if (phNfc_eNfcIP1_Initiator == psRemoteDev->psRemoteDevInfo->RemDevType) {
            if (dtaInterface->bLlcpAutoActivate) {
                NfcCxDTAInterfaceLlcpAutoActivate(dtaInterface);
            }
            else {
                status = phLibNfc_RemoteDev_Receive(psRemoteDev->hTargetDev,
                                                    NfcCxDTAInterfaceRemoteDevRecvCB,
                                                    dtaInterface);
                if (NFCSTATUS_PENDING != status) {
                    TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive failed %!NFCSTATUS!", status);
                }
            }
        }

        TRACE_LINE(LEVEL_INFO, "Remote device handle 0x%p", psRemoteDev->hTargetDev);

        NfcCxDTAInterfaceHandleRemoteDev(dtaInterface, psRemoteDev);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSENtfRegister(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_SE_NtfRegister(NfcCxDTAInterfaceHandleSEEvent, DTAInterface);

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "Error phLibNfc_SE_NtfRegister with Status: %!NFCSTATUS!", status);
        goto Done;
    }

    status = phLibNfc_CardEmulation_NtfRegister(NfcCxDTAInterfaceHandleSEDeviceHostEvent, DTAInterface);

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "Error phLibNfc_CardEmulation_NtfRegister with Status: %!NFCSTATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNtfRegister(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_Registry_Info_t RegInfo;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RegInfo.MifareUL = 1;
    RegInfo.MifareStd = 1;
    RegInfo.ISO14443_4A = 1;
    RegInfo.ISO14443_4B = 1;
    RegInfo.Felica = 1;
    RegInfo.Jewel = 1;
    RegInfo.ISO15693 = 1;
    RegInfo.NFC = 1;

    status = phLibNfc_RemoteDev_NtfRegister(&RegInfo, NfcCxDTAInterfaceRemoteDevNtfCB, DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_RemoteDev_NtfRegister completed immediately with Status: %!NFCSTATUS!", status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceInitializeCB(
    _In_  void       *pContext,
    _In_  uint8_t    /*ConfigStatus*/,
    _In_  NFCSTATUS  NfcStatus 
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    if (NFCSTATUS_SUCCESS == NfcStatus) {
        NfcCxDTAInterfaceRemoteDevNtfRegister(dtaInterface);
        NfcCxDTAInterfaceSENtfRegister(dtaInterface);
    }
    
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceInitialize(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Mgt_Initialize(DTAInterface->FdoContext,
                                     phLibNfc_InitType_Default,
                                     &DTAInterface->LibConfig,
                                     NfcCxDTAInterfaceInitializeCB,
                                     DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_Initialize completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceInitializeCB(DTAInterface, 0, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceDeinitializeCB(
    _In_  void       *pContext,
    _In_  NFCSTATUS  NfcStatus 
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceDeinitialize(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Mgt_DeInitialize(DTAInterface->FdoContext,
                                       NfcCxDTAInterfaceDeinitializeCB,
                                       DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_Initialize completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceDeinitializeCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceSetDTAModeCB(
    _In_ PVOID pContext, 
    _In_ phNfc_sData_t* pOutParam, 
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    UNREFERENCED_PARAMETER(pOutParam);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxDTAInterfaceSetDTAMode(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ BOOLEAN DTAEnabled
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Mgt_IoCtl(DTAInterface->FdoContext,
                                DTAEnabled ? PHLIBNFC_ENABLE_DTA_MODE : PHLIBNFC_DISABLE_DTA_MODE,
                                NULL, 
                                NULL,
                                NfcCxDTAInterfaceSetDTAModeCB,
                                DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_IoCtl completed immediately for mode %d with Status: %!NFCSTATUS!", DTAEnabled, status);
        NfcCxDTAInterfaceSetDTAModeCB(DTAInterface, NULL, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxDTAInterfaceLibNfcMessageHandler(
    _In_ PVOID Context,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(NULL != dtaInterface);
    NT_ASSERT(NULL != dtaInterface->pLibNfcContext);

    TRACE_LINE(LEVEL_INFO, "%!NFCCX_LIBNFC_MESSAGE!: %p, %p", Message, (VOID*)Param1, (VOID*)Param2);

    switch (Message)
    {
    case LIBNFC_INIT:
        NfcCxDTAInterfaceInitialize(dtaInterface);
        break;

    case LIBNFC_DEINIT:
        NfcCxDTAInterfaceDeinitialize(dtaInterface);
        break;

    case LIBNFC_DTA_MESSAGE:
        NfcCxDTAInterfaceMessage(Context, (UINT32)Param1, Param2, Param3, Param4);
        break;

    default:
        NT_ASSERTMSG("Invalid Message", FALSE);
        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxDTAInterfaceMapRfDiscoveryMode(
    _In_ NFC_RF_DISCOVERY_MODE RfDiscovery,
    _Out_ phNfc_eDiscoveryConfigMode_t* pRfDiscovery
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (RfDiscovery) {
    case RfDiscoveryConfig:
        *pRfDiscovery = NFC_DISCOVERY_CONFIG;
        break;

    case RfDiscoveryStart:
        *pRfDiscovery = NFC_DISCOVERY_START;
        break;

    case RFDiscoveryResume:
        *pRfDiscovery = NFC_DISCOVERY_RESUME;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

static VOID
NfcCxDTAInterfaceRfDiscoveryConfigCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID NfcCxDTAInterfacDumpRfConfig(
    _In_ PNFC_RF_DISCOVERY_CONFIG RfDiscoveryConfig
    )
{
    TRACE_LINE(LEVEL_INFO, "RF Configurations:");
    TRACE_LINE(LEVEL_INFO, "==================");
    TRACE_LINE(LEVEL_INFO, "usTotalDuration %d", RfDiscoveryConfig->usTotalDuration);
    TRACE_LINE(LEVEL_INFO, "ulPollConfig %d", RfDiscoveryConfig->ulPollConfig);
    TRACE_LINE(LEVEL_INFO, "fDisableCardEmulation %d", RfDiscoveryConfig->fDisableCardEmulation);
    TRACE_LINE(LEVEL_INFO, "ucNfcIPMode %d", RfDiscoveryConfig->ucNfcIPMode);
    TRACE_LINE(LEVEL_INFO, "fNfcIPTgtModeDisable %d", RfDiscoveryConfig->fNfcIPTgtModeDisable);
    TRACE_LINE(LEVEL_INFO, "ucNfcIPTgtMode %d", RfDiscoveryConfig->ucNfcIPTgtMode);
    TRACE_LINE(LEVEL_INFO, "ucNfcCEMode %d", RfDiscoveryConfig->ucNfcCEMode);
    TRACE_LINE(LEVEL_INFO, "ucBailoutConfig %d", RfDiscoveryConfig->ucBailoutConfig);
    TRACE_LINE(LEVEL_INFO, "ucSystemCode[0] %d", RfDiscoveryConfig->ucSystemCode[0]);
    TRACE_LINE(LEVEL_INFO, "ucSystemCode[1] %d", RfDiscoveryConfig->ucSystemCode[1]);
    TRACE_LINE(LEVEL_INFO, "ucRequestCode %d", RfDiscoveryConfig->ucRequestCode);
    TRACE_LINE(LEVEL_INFO, "ucTimeSlotNumber %d", RfDiscoveryConfig->ucTimeSlotNumber);
    TRACE_LINE(LEVEL_INFO, "eRfDiscoveryMode %!NFC_RF_DISCOVERY_MODE!", RfDiscoveryConfig->eRfDiscoveryMode);
    TRACE_LINE(LEVEL_INFO, "==================");
}

static VOID
NfcCxDTAInterfaceRfDiscoveryConfig(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_RF_DISCOVERY_CONFIG RfDiscoveryConfig
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_sADD_Cfg_t DiscoveryConfig;
    phLibNfc_eDiscoveryConfigMode_t rfDiscoveryMode;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxDTAInterfacDumpRfConfig(RfDiscoveryConfig);

    ntStatus = NfcCxDTAInterfaceMapRfDiscoveryMode(RfDiscoveryConfig->eRfDiscoveryMode, &rfDiscoveryMode);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid Discovery mode %d", RfDiscoveryConfig->eRfDiscoveryMode);
        NfcCxDTAInterfaceRfDiscoveryConfigCB(DTAInterface, NfcCxNfcStatusFromNtStatus(ntStatus));
        goto Done;
    }
    
    RtlZeroMemory(&DiscoveryConfig, sizeof(phLibNfc_sADD_Cfg_t));

    DiscoveryConfig.PollDevInfo.PollEnabled = RfDiscoveryConfig->ulPollConfig;
    DiscoveryConfig.Duration = RfDiscoveryConfig->usTotalDuration * 1000;
    DiscoveryConfig.NfcIP_Mode = RfDiscoveryConfig->ucNfcIPMode;
    DiscoveryConfig.NfcIP_Tgt_Disable = RfDiscoveryConfig->fNfcIPTgtModeDisable;
    DiscoveryConfig.NfcIP_Tgt_Mode_Config = RfDiscoveryConfig->ucNfcIPTgtMode;
    DiscoveryConfig.CE_Mode_Config = RfDiscoveryConfig->ucNfcCEMode;
    DiscoveryConfig.PollDevInfo.PollCfgInfo.DisableCardEmulation = RfDiscoveryConfig->fDisableCardEmulation;

    DiscoveryConfig.aPollParms[0].Bailout = (RfDiscoveryConfig->ucBailoutConfig & NFC_CX_POLL_BAILOUT_NFC_A) != 0;
    DiscoveryConfig.aPollParms[1].Bailout = (RfDiscoveryConfig->ucBailoutConfig & NFC_CX_POLL_BAILOUT_NFC_B) != 0;

    RtlCopyMemory(DiscoveryConfig.FelicaPollCfg.SystemCode, RfDiscoveryConfig->ucSystemCode, sizeof(DiscoveryConfig.FelicaPollCfg.SystemCode));
    DiscoveryConfig.FelicaPollCfg.ReqCode = RfDiscoveryConfig->ucRequestCode;
    DiscoveryConfig.FelicaPollCfg.TimeSlotNum = RfDiscoveryConfig->ucTimeSlotNumber;

    status = phLibNfc_Mgt_ConfigureDiscovery(rfDiscoveryMode,
                                             DiscoveryConfig,
                                             NfcCxDTAInterfaceRfDiscoveryConfigCB,
                                             DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_ConfigureDiscovery completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRfDiscoveryConfigCB(DTAInterface, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevConnectCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_Handle hRemoteDev,
    _In_ phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);
    UNREFERENCED_PARAMETER(psRemoteDevInfo);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevConnect(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDev
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_RemoteDev_Connect(RemoteDev,
                                        NfcCxDTAInterfaceRemoteDevConnectCB,
                                        DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_RemoteDev_Connect completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevConnectCB(DTAInterface, RemoteDev, NULL, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxDTAInterfaceMapReleaseType(
    _In_ NFC_RELEASE_TYPE ReleaseType,
    _Out_ phLibNfc_eReleaseType_t* pReleaseType
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (ReleaseType) {
    case IdleMode:
        *pReleaseType = NFC_DISCOVERY_STOP;
        break;

    case SleepMode:
        *pReleaseType = NFC_DEVICE_SLEEP;
        break;

    case Discovery:
        *pReleaseType = NFC_DISCOVERY_CONTINUE; //NFC_DISCOVERY_RESTART
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

static VOID
NfcCxDTAInterfaceRemoteDevDisonnectCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_Handle hRemoteDev,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevDisconnect(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_REMOTE_DEVICE_DISCONNET RemoteDev
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_eReleaseType_t releaseType;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ntStatus = NfcCxDTAInterfaceMapReleaseType(RemoteDev->eReleaseType, &releaseType);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Unsupported release type %d", RemoteDev->eReleaseType);
        NfcCxDTAInterfaceRemoteDevDisonnectCB(DTAInterface, RemoteDev, NfcCxNfcStatusFromNtStatus(ntStatus));
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Remote device handle 0x%p, release type %d", RemoteDev->hRemoteDev, RemoteDev->eReleaseType);

    status = phLibNfc_RemoteDev_Disconnect(RemoteDev->hRemoteDev,
                                           releaseType,
                                           NfcCxDTAInterfaceRemoteDevDisonnectCB,
                                           DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_RemoteDev_Disconnect completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevDisonnectCB(DTAInterface, RemoteDev, status);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevTranscieveCB(
    _In_ VOID *             pContext,
    _In_ phLibNfc_Handle    hRemoteDev,
    _In_ phNfc_sData_t*     pResBuffer,
    _In_ NFCSTATUS          NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);

    if ((NFCSTATUS_SUCCESS == NfcStatus) && (NULL != pResBuffer)) {
        if (pResBuffer->length > dtaInterface->sTransceiveBuffer.sRecvData.length) {
            NfcStatus = NFCSTATUS_BUFFER_TOO_SMALL;
            TRACE_LINE(LEVEL_ERROR, "Receive buffer too small for response");
        }
        else {
            RtlCopyMemory(dtaInterface->sTransceiveBuffer.sRecvData.buffer,
                          pResBuffer->buffer,
                          pResBuffer->length);
            dtaInterface->sTransceiveBuffer.sRecvData.length = pResBuffer->length;
        }
    }

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevTranscieve(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_RemoteDev_Transceive(RemoteDevHandle,
                                           &DTAInterface->sTransceiveBuffer,
                                           NfcCxDTAInterfaceRemoteDevTranscieveCB,
                                           DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_RemoteDev_Transceive completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevTranscieveCB(DTAInterface, RemoteDevHandle, NULL, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevSendCB(
    _In_ void* pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NFCSTATUS nfcStatus;
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS == NfcStatus) {
        nfcStatus = phLibNfc_RemoteDev_Receive(dtaInterface->hRemoteDev,
                                               NfcCxDTAInterfaceRemoteDevRecvCB,
                                               dtaInterface);

        if (NFCSTATUS_PENDING != nfcStatus) {
            TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive failed %!NFCSTATUS!", nfcStatus);
        }
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "Remote device send failed %!NFCSTATUS!", NfcStatus);
    }

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevSend(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "Remote device handle 0x%p", RemoteDevHandle);

    DTAInterface->hRemoteDev = RemoteDevHandle;
    status = phLibNfc_RemoteDev_Send(RemoteDevHandle,
                                     &DTAInterface->sSendBuffer,
                                     NfcCxDTAInterfaceRemoteDevSendCB,
                                     DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_RemoteDev_Send completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevSendCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

FORCEINLINE
static NTSTATUS
NfcCxDTAInterfaceMapP2pMode(
    _In_ NFC_P2P_MODE P2pMode,
    _Out_ phNfc_eConfigP2PMode_t* pP2pMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (P2pMode) {
    case NfcDepDefault:
        *pP2pMode = NFC_DEP_DEFAULT;
        break;

    case NfcDepPoll:
        *pP2pMode = NFC_DEP_POLL;
        break;

    case NfcDepListen:
        *pP2pMode = NFC_DEP_LISTEN;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

static VOID
NfcCxDTAInterfaceP2pConfigCB(
    _In_ void* pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceP2pConfig(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_P2P_PARAM_CONFIG P2pConfig
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_sNfcIPCfg_t nfcIpCfg = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ntStatus = NfcCxDTAInterfaceMapP2pMode(P2pConfig->eP2pMode, &nfcIpCfg.p2pMode);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid P2P mode %d", P2pConfig->eP2pMode);
        NfcCxDTAInterfaceP2pConfigCB(DTAInterface, NFCSTATUS_INVALID_PARAMETER);
        goto Done;
    }

    nfcIpCfg.generalBytesLength = P2pConfig->cbGeneralBytes;
    RtlCopyMemory(nfcIpCfg.generalBytes, P2pConfig->pbGeneralBytes, P2pConfig->cbGeneralBytes);

    status = phLibNfc_Mgt_SetP2P_ConfigParams(&nfcIpCfg,
                                              NfcCxDTAInterfaceP2pConfigCB,
                                              DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_SetP2P_ConfigParams completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceP2pConfigCB(DTAInterface, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRfConfigCB(
    _In_ PVOID pContext, 
    _In_ phNfc_sData_t* pOutParam, 
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(pOutParam);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRfConfig(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_bytecount_(Size) PBYTE RFConfigBuffer,
    _In_ DWORD Size
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phNfc_sData_t rfRawConfig = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfRawConfig.buffer = RFConfigBuffer;
    rfRawConfig.length = Size;

    status = phLibNfc_Mgt_IoCtl(DTAInterface->FdoContext,
                                PHLIBNFC_SET_RAW_CONFIG,
                                &rfRawConfig,
                                NULL,
                                NfcCxDTAInterfaceRfConfigCB,
                                DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_IoCtl completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRfConfigCB(DTAInterface, NULL, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefWriteCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefWrite(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Ndef_Write(RemoteDevHandle,
                                 &DTAInterface->sSendBuffer,
                                 NfcCxDTAInterfaceRemoteDevNdefWriteCB,
                                 DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Ndef_Write completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevNdefWriteCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefReadCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    RtlZeroMemory(&dtaInterface->sNdefInfo, sizeof(dtaInterface->sNdefInfo));
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefRead(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Ndef_Read(RemoteDevHandle,
                                &DTAInterface->sNdefMsg,
                                phLibNfc_Ndef_EBegin,
                                NfcCxDTAInterfaceRemoteDevNdefReadCB,
                                DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Ndef_Read completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevNdefReadCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefConvertReadOnlyCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefConvertReadOnly(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    static uint8_t defaultRawKey[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    phNfc_sData_t defaultKey = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    defaultKey.buffer = defaultRawKey;
    defaultKey.length = sizeof(defaultRawKey);

    status = phLibNfc_ConvertToReadOnlyNdef(RemoteDevHandle,
                                            &defaultKey,
                                            NfcCxDTAInterfaceRemoteDevNdefConvertReadOnlyCB,
                                            DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_ConvertToReadOnlyNdef completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceRemoteDevNdefConvertReadOnlyCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefCheckCB(
    _In_ VOID *                  pContext,
    _In_ phLibNfc_ChkNdef_Info_t NdefInfo,
    _In_ NFCSTATUS               NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfcStatus == NFCSTATUS_SUCCESS && NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_INVALID) {
        dtaInterface->sNdefInfo.dwActualMessageLength = NdefInfo.ActualNdefMsgLength;
        dtaInterface->sNdefInfo.dwMaxMessageLength = NdefInfo.MaxNdefMsgLength;
        dtaInterface->sNdefInfo.fIsNdefFormatted = TRUE;

        TRACE_LINE(LEVEL_INFO, "NdefCardState=%d ActualNdefMsgLength=%d MaxNdefMsgLength=%d",
                   NdefInfo.NdefCardState, NdefInfo.ActualNdefMsgLength, NdefInfo.MaxNdefMsgLength);

        dtaInterface->sNdefInfo.fIsReadOnly = (NdefInfo.NdefCardState == PHLIBNFC_NDEF_CARD_READ_ONLY);
    }
    else {
        dtaInterface->sNdefInfo.fIsNdefFormatted = FALSE;
    }

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceRemoteDevNdefCheck(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_ChkNdef_Info_t NdefInfo;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Ndef_CheckNdef(RemoteDevHandle,
                                     NfcCxDTAInterfaceRemoteDevNdefCheckCB,
                                     DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Ndef_CheckNdef completed immediately with Status: %!NFCSTATUS!", status);
        NdefInfo.NdefCardState = PHLIBNFC_NDEF_CARD_INVALID;
        NfcCxDTAInterfaceRemoteDevNdefCheckCB(DTAInterface, NdefInfo, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID NfcCxDTAInterfacDumpLlcpConfig(
    _In_ PNFC_LLCP_CONFIG LlcpConfig
    )
{
    TRACE_LINE(LEVEL_INFO, "LLCP Configurations:");
    TRACE_LINE(LEVEL_INFO, "==================");
    TRACE_LINE(LEVEL_INFO, "uMIU %d", LlcpConfig->uMIU);
    TRACE_LINE(LEVEL_INFO, "bLTO %d", LlcpConfig->bLTO);
    TRACE_LINE(LEVEL_INFO, "uWKS %d", LlcpConfig->uWKS);
    TRACE_LINE(LEVEL_INFO, "bOptions %d", LlcpConfig->bOptions);
    TRACE_LINE(LEVEL_INFO, "fAutoActivate %d", LlcpConfig->fAutoActivate);
    TRACE_LINE(LEVEL_INFO, "==================");
}

static VOID
NfcCxDTAInterfaceLlcpConfigCB(
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        dtaInterface->bLlcpAutoActivate = FALSE;
    }

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpConfig(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_LLCP_CONFIG LlcpConfig
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_Llcp_sLinkParameters_t LocalLinkInfo;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxDTAInterfacDumpLlcpConfig(LlcpConfig);

    LocalLinkInfo.miu = LlcpConfig->uMIU;
    LocalLinkInfo.lto = LlcpConfig->bLTO;
    LocalLinkInfo.wks = LlcpConfig->uWKS;
    LocalLinkInfo.option = LlcpConfig->bOptions;

    DTAInterface->bLlcpAutoActivate = LlcpConfig->fAutoActivate;

    status = phLibNfc_Mgt_SetLlcp_ConfigParams(&LocalLinkInfo,
                                               NfcCxDTAInterfaceLlcpConfigCB,
                                               DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_SetLlcp_ConfigParams completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpConfigCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpActivate(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (phFriNfc_LlcpMac_eLinkActivated == DTAInterface->eLinkStatus) {
        TRACE_LINE(LEVEL_ERROR, "LLCP link is already activated or auto activate is enabled");
        status = NFCSTATUS_INVALID_STATE;
        goto Done;
    }

    status = phLibNfc_Llcp_Activate(RemoteDevHandle);

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpDeactivate(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (phFriNfc_LlcpMac_eLinkDeactivated == DTAInterface->eLinkStatus) {
        TRACE_LINE(LEVEL_ERROR, "LLCP link is already deactivated");
        status = NFCSTATUS_INVALID_STATE;
        goto Done;
    }

    status = phLibNfc_Llcp_Deactivate(RemoteDevHandle);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Deactivate completed immediately with Status: %!NFCSTATUS!", status);
    }

    if (phFriNfc_LlcpMac_eLinkDeactivated == DTAInterface->eLinkStatus) {
        status = NFCSTATUS_SUCCESS;
    }

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpDiscoverServicesCB(
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != dtaInterface->psServiceNameList) {
        free(dtaInterface->psServiceNameList);
        dtaInterface->psServiceNameList = NULL;
    }

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpDiscoverServices(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_LLCP_SERVICE_DISCOVER_REQUEST LlcpServiceDiscoverRequest,
    _Out_ PNFC_LLCP_SERVICE_DISCOVER_SAP LlcpServiceDiscoverSap
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PNFC_LLCP_SERVICE_NAME_ENTRY llcpServiceNameEntry = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL != DTAInterface->psServiceNameList) {
        TRACE_LINE(LEVEL_ERROR, "Invalid state psServiceNameList 0x%p", DTAInterface->psServiceNameList);
        status = NFCSTATUS_INVALID_STATE;
        NfcCxDTAInterfaceLlcpDiscoverServicesCB(DTAInterface, status);
        goto Done;
    }

    DTAInterface->psServiceNameList = (phNfc_sData_t*) malloc(LlcpServiceDiscoverRequest->NumberOfEntries * sizeof(phNfc_sData_t));
    if (NULL == DTAInterface->psServiceNameList) {
        TRACE_LINE(LEVEL_ERROR, "Failed memory allocation");
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        NfcCxDTAInterfaceLlcpDiscoverServicesCB(DTAInterface, status);
        goto Done;
    }

    llcpServiceNameEntry = LlcpServiceDiscoverRequest->ServiceNameEntries;
    for (uint8_t i = 0; i < LlcpServiceDiscoverRequest->NumberOfEntries; i++) {
        DTAInterface->psServiceNameList[i].buffer = llcpServiceNameEntry->pbServiceName;
        DTAInterface->psServiceNameList[i].length = llcpServiceNameEntry->cbServiceName;

        llcpServiceNameEntry = (PNFC_LLCP_SERVICE_NAME_ENTRY)((PBYTE)llcpServiceNameEntry +
                                                              offsetof(NFC_LLCP_SERVICE_NAME_ENTRY, pbServiceName) +
                                                              llcpServiceNameEntry->cbServiceName);
    }

    LlcpServiceDiscoverSap->NumberOfEntries = LlcpServiceDiscoverRequest->NumberOfEntries;
    status = phLibNfc_Llcp_DiscoverServices(LlcpServiceDiscoverRequest->hRemoteDev,
                                            DTAInterface->psServiceNameList,
                                            LlcpServiceDiscoverSap->SAPEntries,
                                            (uint8_t) LlcpServiceDiscoverRequest->NumberOfEntries,
                                            NfcCxDTAInterfaceLlcpDiscoverServicesCB,
                                            DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_DiscoverServices completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpDiscoverServicesCB(DTAInterface, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpLinkStatusCheckCB(
    _In_ PVOID Context,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->bLlcpLinkStatusEnabled = (NFCSTATUS_SUCCESS == NfcStatus);
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpLinkStatusCheck(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (DTAInterface->bLlcpLinkStatusEnabled) {
        TRACE_LINE(LEVEL_ERROR, "Link status already enabled");
        NfcCxDTAInterfaceLlcpLinkStatusCheckCB(DTAInterface, status);
        goto Done;
    }

    status = phLibNfc_Llcp_CheckLlcp(RemoteDevHandle,
                                     NfcCxDTAInterfaceLlcpLinkStatusCheckCB,
                                     NfcCxDTAInterfaceLlcpLinkStatusEvent,
                                     DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_CheckLlcp completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpLinkStatusCheckCB(DTAInterface, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpHandleSocketErrorCB(
    _In_ PVOID Context,
    _In_ uint8_t ErrCode)
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;
    CNFCPayload* queuedSocketError = NULL;
    PNFC_LLCP_SOCKET_ERROR_INFO socketPayload = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    queuedSocketError = new CNFCPayload();
    if (NULL == queuedSocketError) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    if (!NT_SUCCESS(queuedSocketError->Initialize(NULL, sizeof(NFC_LLCP_SOCKET_ERROR_INFO)))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedSocketError;
        goto Done;
    }

    socketPayload = (PNFC_LLCP_SOCKET_ERROR_INFO)queuedSocketError->GetPayload();
    socketPayload->hSocket = socketContext->GetSocket();
    socketPayload->eSocketError = (NFC_LLCP_SOCKET_ERROR)ErrCode;

    if (!NT_SUCCESS(socketContext->GetSocketError()->ProcessPayload(queuedSocketError))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSocketError;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketCreate(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_LLCP_SOCKET_INFO LlcpSocketInfo,
    _Out_ PNFC_LLCP_SOCKET_HANDLE LlcpSocketHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_Llcp_sSocketOptions_t socketOption;
    CNFCDtaSocketContext* socketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *LlcpSocketHandle = NULL;
    socketContext = new CNFCDtaSocketContext();
    if (NULL == socketContext) {
        TRACE_LINE(LEVEL_ERROR, "Memory allocation failed for socket context");
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    ntStatus = socketContext->Initialize(NULL, DTAInterface, (LlcpSocketInfo->sSocketOption.bRW + 1) * LlcpSocketInfo->sSocketOption.uMIUX);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Socket initialization failed");
        status = NfcCxNfcStatusFromNtStatus(ntStatus);
        delete socketContext;
        goto Done;
    }

    socketOption.miu = LlcpSocketInfo->sSocketOption.uMIUX;
    socketOption.rw  = LlcpSocketInfo->sSocketOption.bRW;

    status = phLibNfc_Llcp_Socket((Connectionless == LlcpSocketInfo->eSocketType) ? phFriNfc_LlcpTransport_eConnectionLess : phFriNfc_LlcpTransport_eConnectionOriented,
                                  (Connectionless == LlcpSocketInfo->eSocketType) ? NULL : &socketOption,
                                  socketContext->GetWorkingBuffer(),
                                  LlcpSocketHandle,
                                  NfcCxDTAInterfaceLlcpHandleSocketErrorCB,
                                  socketContext);

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Socket completed immediately with %!NFCSTATUS!", status);
        delete socketContext;
        goto Done;
    }

    ntStatus = socketContext->EnableSocketError();
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSocketError failed with %!STATUS!", ntStatus);
        status = NfcCxNfcStatusFromNtStatus(ntStatus);
        delete socketContext;
        goto Done;
    }

    socketContext->SetSocket(*LlcpSocketHandle);
    socketContext->SetSocketType(LlcpSocketInfo->eSocketType);
    *LlcpSocketHandle = socketContext;

    WdfWaitLockAcquire(DTAInterface->SocketLock, NULL);
    InsertTailList(&DTAInterface->SocketContext, socketContext->GetListEntry());
    WdfWaitLockRelease(DTAInterface->SocketLock);

    TRACE_LINE(LEVEL_INFO, "Socket 0x%p created successfully", socketContext);

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketClose(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)LlcpSocketHandle;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Llcp_Close(socketContext->GetSocket());

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Close failed with %!NFCSTATUS!", status);
        goto Done;
    }

    WdfWaitLockAcquire(DTAInterface->SocketLock, NULL);
    RemoveEntryList(socketContext->GetListEntry());
    WdfWaitLockRelease(DTAInterface->SocketLock);

    delete socketContext;

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketBind(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_LLCP_SOCKET_SERVICE_INFO LlcpSocketServiceInfo
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)LlcpSocketServiceInfo->hSocket;
    phNfc_sData_t llcpServiceName;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    llcpServiceName.buffer = LlcpSocketServiceInfo->sServiceName.pbServiceName;
    llcpServiceName.length = LlcpSocketServiceInfo->sServiceName.cbServiceName;

    status = phLibNfc_Llcp_Bind(socketContext->GetSocket(),
                                LlcpSocketServiceInfo->bSAP,
                                &llcpServiceName);

    ntStatus = NfcCxNtStatusFromNfcStatus(status);
    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Bind failed with %!NFCSTATUS!", status);
        goto Done;
    }

    if (Connectionless == socketContext->GetSocketType()) {
        ntStatus = socketContext->EnableSocketReceive();
        if (!NT_SUCCESS(ntStatus)) {
            TRACE_LINE(LEVEL_ERROR, "EnableSocketReceive failed with %!STATUS!", ntStatus);
            goto Done;
        }
    }

Done:
    DTAInterface->pLibNfcContext->Status = ntStatus;
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketListenCB(
    _In_ PVOID Context,
    _In_ phLibNfc_Handle IncomingSocket
    )
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)socketContext->GetDTAInterface();
    CNFCPayload* queuedSocket = NULL;
    PNFC_LLCP_SOCKET_HANDLE socketHandle = NULL;
    CNFCDtaSocketContext* incomingSocketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    queuedSocket = new CNFCPayload();
    if (NULL == queuedSocket) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    if (!NT_SUCCESS(queuedSocket->Initialize(NULL, sizeof(NFC_LLCP_SOCKET_HANDLE)))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedSocket;
        goto Done;
    }

    incomingSocketContext = new CNFCDtaSocketContext();
    if (NULL == incomingSocketContext) {
        TRACE_LINE(LEVEL_ERROR, "Memory allocation failed for the incoming socket context");
        delete queuedSocket;
        goto Done;
    }

    incomingSocketContext->SetSocket(IncomingSocket);

    WdfWaitLockAcquire(dtaInterface->SocketLock, NULL);
    InsertTailList(&dtaInterface->SocketContext, incomingSocketContext->GetListEntry());
    WdfWaitLockRelease(dtaInterface->SocketLock);

    socketHandle = (PNFC_LLCP_SOCKET_HANDLE)queuedSocket->GetPayload();
    *socketHandle = incomingSocketContext;

    if (!NT_SUCCESS(socketContext->GetSocketListen()->ProcessPayload(queuedSocket))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSocket;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketListen(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle,
    _In_ PNFCCX_DTA_REQUEST_CONTEXT RequestContext
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)LlcpSocketHandle;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (!socketContext->GetListenEnabled()) {
        status = phLibNfc_Llcp_Listen(socketContext->GetSocket(),
                                      NfcCxDTAInterfaceLlcpSocketListenCB,
                                      LlcpSocketHandle);

        if (NFCSTATUS_SUCCESS != status) {
            TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Listen failed with %!NFCSTATUS!", status);
            goto Done;
        }

        socketContext->EnableSocketListen();
    }

    //
    // forward the request to socket listen pended request
    //
    socketContext->GetSocketListen()->ProcessRequest(RequestContext->FileContext,
                                                     RequestContext->Request,
                                                     RequestContext->OutputBuffer,
                                                     RequestContext->OutputBufferLength);

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketAcceptCB(
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "Socket accept failed with %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    socketContext->SetSocketType(ConnectionOriented);
    ntStatus = socketContext->EnableSocketReceive();
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSocketReceive failed with %!STATUS!", ntStatus);
        NfcStatus = NfcCxNfcStatusFromNtStatus(ntStatus);
        goto Done;
    }

Done:
    socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketAccept(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_LLCP_SOCKET_ACCEPT_INFO LlcpSocketAcceptInfo
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_Llcp_sSocketOptions_t socketOption;
    CNFCDtaSocketContext* socketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*)LlcpSocketAcceptInfo->hSocket;

    socketContext->Initialize(socketContext->GetSocket(),
                              DTAInterface,
                              (LlcpSocketAcceptInfo->sSocketOption.bRW + 1) * LlcpSocketAcceptInfo->sSocketOption.uMIUX);

    socketOption.miu = LlcpSocketAcceptInfo->sSocketOption.uMIUX;
    socketOption.rw = LlcpSocketAcceptInfo->sSocketOption.bRW;

    status = phLibNfc_Llcp_Accept(socketContext->GetSocket(),
                                  &socketOption,
                                  socketContext->GetWorkingBuffer(),
                                  NfcCxDTAInterfaceLlcpHandleSocketErrorCB,
                                  NfcCxDTAInterfaceLlcpSocketAcceptCB,
                                  socketContext);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Accept completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpSocketAcceptCB(socketContext, status);
        goto Done;
    }

    ntStatus = socketContext->EnableSocketError();
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSocketError failed with %!STATUS!", ntStatus);
        NfcCxDTAInterfaceLlcpSocketAcceptCB(socketContext, NfcCxNfcStatusFromNtStatus(ntStatus));
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketConnectCB(
    _In_ PVOID Context,
    _In_ uint8_t ErrCode,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "Llcp connect callback with status %!NFCSTATUS!, error code %d", NfcStatus, ErrCode);
        goto Done;
    }

    ntStatus = socketContext->EnableSocketReceive();
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSocketReceive failed with %!STATUS!", ntStatus);
        NfcStatus = NfcCxNfcStatusFromNtStatus(ntStatus);
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Socket 0x%p is connected", socketContext);
Done:
    socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketConnect(
    _In_ PNFC_LLCP_SOCKET_CONNECT_INFO LlcpSocketConnectInfo
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phNfc_sData_t serviceName;
    CNFCDtaSocketContext* socketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*)LlcpSocketConnectInfo->hSocket;

    if (NfcConnectBySap == LlcpSocketConnectInfo->eConnectType) {
        status = phLibNfc_Llcp_Connect(LlcpSocketConnectInfo->hRemoteDev,
                                       socketContext->GetSocket(),
                                       LlcpSocketConnectInfo->bSAP,
                                       NfcCxDTAInterfaceLlcpSocketConnectCB,
                                       socketContext);
    }
    else {
        serviceName.buffer = LlcpSocketConnectInfo->sServiceName.pbServiceName;
        serviceName.length = LlcpSocketConnectInfo->sServiceName.cbServiceName;

        status = phLibNfc_Llcp_ConnectByUri(LlcpSocketConnectInfo->hRemoteDev,
                                            socketContext->GetSocket(),
                                            &serviceName,
                                            NfcCxDTAInterfaceLlcpSocketConnectCB,
                                            socketContext);
    }

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Connect completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpSocketConnectCB(socketContext, 0, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketDisconnectCB(
    _In_ PVOID Context,
    _In_ NFCSTATUS NfcStatus
    )
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketDisconnect(
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*) LlcpSocketHandle;

    status = phLibNfc_Llcp_Disconnect(socketContext->GetDTAInterface()->hRemoteDev,
                                      socketContext->GetSocket(),
                                      NfcCxDTAInterfaceLlcpSocketDisconnectCB,
                                      socketContext);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Disconnect completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpSocketDisconnectCB(socketContext, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketRecvCB(
    _In_ PVOID Context,
    _In_ NFCSTATUS NfcStatus
    )
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;
    CNFCPayload* queuedSocketRecv = NULL;
    PNFC_LLCP_SOCKET_PAYLOAD socketPayload = NULL;
    phNfc_sData_t* receiveBuffer = socketContext->GetReceiveBuffer();
    DWORD requiredBufferSpace;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
        goto Done;
    }

    queuedSocketRecv = new CNFCPayload();
    if (NULL == queuedSocketRecv) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        socketContext->GetDTAInterface()->pLibNfcContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    requiredBufferSpace = receiveBuffer->length +
                          offsetof(NFC_LLCP_SOCKET_PAYLOAD, sPayload) +
                          offsetof(NFC_DATA_BUFFER, pbBuffer);

    if (!NT_SUCCESS(queuedSocketRecv->Initialize(NULL, requiredBufferSpace))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedSocketRecv;
        goto Done;
    }

    socketPayload = (PNFC_LLCP_SOCKET_PAYLOAD)queuedSocketRecv->GetPayload();
    socketPayload->hSocket = socketContext;

    socketPayload->bSAP = 0;
    socketPayload->sPayload.cbBuffer = (USHORT)receiveBuffer->length;
    RtlCopyMemory(socketPayload->sPayload.pbBuffer, receiveBuffer->buffer, receiveBuffer->length);

    if (!NT_SUCCESS(socketContext->GetSocketReceive()->ProcessPayload(queuedSocketRecv))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSocketRecv;
    }

Done:
    if (NULL != receiveBuffer->buffer) {
        delete receiveBuffer->buffer;
    }
    receiveBuffer->buffer = NULL;
    receiveBuffer->length = 0;
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketRecvFromCB(
    _In_ PVOID Context,
    _In_ uint8_t SourceSap,
    _In_ NFCSTATUS NfcStatus
    )
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;
    CNFCPayload* queuedSocketRecv = NULL;
    PNFC_LLCP_SOCKET_PAYLOAD socketPayload = NULL;
    phNfc_sData_t* receiveBuffer = socketContext->GetReceiveBuffer();
    DWORD requiredBufferSpace;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
        goto Done;
    }

    queuedSocketRecv = new CNFCPayload();
    if (NULL == queuedSocketRecv) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        socketContext->GetDTAInterface()->pLibNfcContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    requiredBufferSpace = receiveBuffer->length +
                          offsetof(NFC_LLCP_SOCKET_PAYLOAD, sPayload) +
                          offsetof(NFC_DATA_BUFFER, pbBuffer);

    if (!NT_SUCCESS(queuedSocketRecv->Initialize(NULL, requiredBufferSpace))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        delete queuedSocketRecv;
        goto Done;
    }

    socketPayload = (PNFC_LLCP_SOCKET_PAYLOAD)queuedSocketRecv->GetPayload();
    socketPayload->hSocket = socketContext;
    socketPayload->bSAP = SourceSap;
    socketPayload->sPayload.cbBuffer = (USHORT) receiveBuffer->length;
    RtlCopyMemory(socketPayload->sPayload.pbBuffer, receiveBuffer->buffer, receiveBuffer->length);

    if (!NT_SUCCESS(socketContext->GetSocketReceive()->ProcessPayload(queuedSocketRecv))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSocketRecv;
    }

Done:
    if (NULL != receiveBuffer->buffer) {
        delete receiveBuffer->buffer;
    }
    receiveBuffer->buffer = NULL;
    receiveBuffer->length = 0;
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketRecv(
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle,
    _In_ PNFCCX_DTA_REQUEST_CONTEXT RequestContext
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = NULL;
    DWORD availableBufferSpace;
    phNfc_sData_t* receiveBuffer = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*)LlcpSocketHandle;

    //
    // Pend a request to receive
    //
    ntStatus = socketContext->GetSocketReceive()->ProcessRequest(RequestContext->FileContext,
                                                                 RequestContext->Request,
                                                                 RequestContext->OutputBuffer,
                                                                 RequestContext->OutputBufferLength);

    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", ntStatus);
        status = NfcCxNfcStatusFromNtStatus(ntStatus);
        NfcCxDTAInterfaceLlcpSocketRecvFromCB(socketContext, 0, status);
        goto Done;
    }

    receiveBuffer = socketContext->GetReceiveBuffer();
    availableBufferSpace = (DWORD) (RequestContext->OutputBufferLength -
                                    offsetof(NFC_LLCP_SOCKET_PAYLOAD, sPayload) -
                                    offsetof(NFC_DATA_BUFFER, pbBuffer));

        receiveBuffer->buffer = new BYTE[availableBufferSpace];
    if (NULL == receiveBuffer->buffer) {
        TRACE_LINE(LEVEL_ERROR, "memory allocation failed");
        NfcCxDTAInterfaceLlcpSocketRecvFromCB(socketContext, 0, NFCSTATUS_INSUFFICIENT_RESOURCES);
        goto Done;
    }
    receiveBuffer->length = availableBufferSpace;

    if (ConnectionOriented == socketContext->GetSocketType()) {
        status = phLibNfc_Llcp_Recv(socketContext->GetDTAInterface()->hRemoteDev,
                                    socketContext->GetSocket(),
                                    receiveBuffer,
                                    NfcCxDTAInterfaceLlcpSocketRecvCB,
                                    socketContext);
    }
    else {
        status = phLibNfc_Llcp_RecvFrom(socketContext->GetDTAInterface()->hRemoteDev,
                                        socketContext->GetSocket(),
                                        receiveBuffer,
                                        NfcCxDTAInterfaceLlcpSocketRecvFromCB,
                                        socketContext);
    }

    if ((NFCSTATUS_PENDING != status) &&
        (NFCSTATUS_SUCCESS != status)) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Recv completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpSocketRecvFromCB(socketContext, 0, status);
        goto Done;
    }

Done:
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketSendCB(
    _In_ PVOID Context,
    _In_ NFCSTATUS NfcStatus
    )
{
    CNFCDtaSocketContext* socketContext = (CNFCDtaSocketContext*)Context;
    phNfc_sData_t* sendBuffer = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    sendBuffer = socketContext->GetSendBuffer();
    sendBuffer->buffer = NULL;
    sendBuffer->length = 0;

    socketContext->GetDTAInterface()->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketSend(
    _In_ PNFC_LLCP_SOCKET_PAYLOAD LlcpSocketPayload
)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = NULL;
    phNfc_sData_t* sendBuffer = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*)LlcpSocketPayload->hSocket;

    sendBuffer = socketContext->GetSendBuffer();
    sendBuffer->buffer = LlcpSocketPayload->sPayload.pbBuffer;
    sendBuffer->length = LlcpSocketPayload->sPayload.cbBuffer;

    if (ConnectionOriented == socketContext->GetSocketType()) {
        status = phLibNfc_Llcp_Send(socketContext->GetDTAInterface()->hRemoteDev,
                                    socketContext->GetSocket(),
                                    sendBuffer,
                                    NfcCxDTAInterfaceLlcpSocketSendCB,
                                    socketContext);
    }
    else {
        status = phLibNfc_Llcp_SendTo(socketContext->GetDTAInterface()->hRemoteDev,
                                      socketContext->GetSocket(),
                                      LlcpSocketPayload->bSAP,
                                      sendBuffer,
                                      NfcCxDTAInterfaceLlcpSocketSendCB,
                                      socketContext);
    }

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Llcp_Send completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceLlcpSocketSendCB(socketContext, status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceLlcpSocketGetError(
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle,
    _In_ PNFCCX_DTA_REQUEST_CONTEXT RequestContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CNFCDtaSocketContext* socketContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    socketContext = (CNFCDtaSocketContext*) LlcpSocketHandle;

    //
    // Pend a request to receive
    //
    status = socketContext->GetSocketError()->ProcessRequest(RequestContext->FileContext,
                                                             RequestContext->Request,
                                                             RequestContext->OutputBuffer,
                                                             RequestContext->OutputBufferLength);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

Done:
    socketContext->GetDTAInterface()->pLibNfcContext->Status = status;
    SetEvent(socketContext->GetDTAInterface()->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerConnCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Handle ConnHandle
    )
{
    CNFCDtaSnepServerContext* snepServerContext = (CNFCDtaSnepServerContext*)pContext;
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)snepServerContext->GetDTAInterface();
    CNFCPayload* queuedConnectionHandle = NULL;
    PNFC_SNEP_SERVER_CONNECTION_HANDLE connectionHandle = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_INCOMING_CONNECTION == NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Incoming Connection 0x%p", ConnHandle);

        queuedConnectionHandle = new CNFCPayload();
        if (NULL == queuedConnectionHandle) {
            TRACE_LINE(LEVEL_ERROR, "Insufficient resources (CNFCPayload)");
            goto Done;
        }

        if (!NT_SUCCESS(queuedConnectionHandle->Initialize(NULL, sizeof(NFC_SNEP_SERVER_CONNECTION_HANDLE)))) {
            TRACE_LINE(LEVEL_ERROR, "Insufficient resources (Initialize)");
            delete queuedConnectionHandle;
            goto Done;
        }

        connectionHandle = (PNFC_SNEP_SERVER_CONNECTION_HANDLE)queuedConnectionHandle->GetPayload();
        *connectionHandle = ConnHandle;

        if (!NT_SUCCESS(snepServerContext->GetConnection()->ProcessPayload(queuedConnectionHandle))) {
            TRACE_LINE(LEVEL_INFO, "Process payload failed");
            delete queuedConnectionHandle;
            goto Done;
        }
    }
    else {
        TRACE_LINE(LEVEL_INFO, "Connection to SNEP client returned status %!NFCSTATUS!", NfcStatus);
        dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
        SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID NfcCxDTAInterfacDumpSnepServerConfig(
    _In_ PNFC_SNEP_SERVER_INFO SnepServerInfo
    )
{
    TRACE_LINE(LEVEL_INFO, "SNEP Server Init Configurations:");
    TRACE_LINE(LEVEL_INFO, "================================");
    TRACE_LINE(LEVEL_INFO, "sService.cbServiceName %d", SnepServerInfo->sService.cbServiceName);
    TRACE_LINE(LEVEL_INFO, "eServerType %d", SnepServerInfo->eServerType);
    TRACE_LINE(LEVEL_INFO, "sSocketOption.uMIUX %d", SnepServerInfo->sSocketOption.uMIUX);
    TRACE_LINE(LEVEL_INFO, "sSocketOption.bRW %d", SnepServerInfo->sSocketOption.bRW);
    TRACE_LINE(LEVEL_INFO, "usInboxSize %d", SnepServerInfo->usInboxSize);
    TRACE_LINE(LEVEL_INFO, "================================");
}

static VOID
NfcCxDTAInterfaceSnepServerInit(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SNEP_SERVER_INFO SnepServerInfo,
    _Out_ PNFC_SNEP_SERVER_HANDLE SnepServerHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_SnepConfig_t snepConfig;
    phNfc_sData_t snepServerName;
    CNFCDtaSnepServerContext* snepServerContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerName.length = SnepServerInfo->sService.cbServiceName;
    snepServerName.buffer = SnepServerInfo->sService.pbServiceName;

    snepConfig.SnepServerType = (DefaultSnepServer == SnepServerInfo->eServerType) ? phLibNfc_SnepServer_Default : phLibNfc_SnepServer_NonDefault;
    snepConfig.sOptions.miu = SnepServerInfo->sSocketOption.uMIUX;
    snepConfig.sOptions.rw = SnepServerInfo->sSocketOption.bRW;
    snepConfig.SnepServerName = (DefaultSnepServer == SnepServerInfo->eServerType) ? NULL : &snepServerName;
    snepConfig.bDtaFlag = TRUE;

    NfcCxDTAInterfacDumpSnepServerConfig(SnepServerInfo);

    snepServerContext = new CNFCDtaSnepServerContext();
    if (NULL == snepServerContext) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        status = NFCSTATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    status = phLibNfc_SnepServer_Init(&snepConfig,
                                      NfcCxDTAInterfaceSnepServerConnCB,
                                      SnepServerHandle,
                                      snepServerContext);

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepServer_Init failed with %!NFCSTATUS!", status);
        delete snepServerContext;
        goto Done;
    }

    ntStatus = snepServerContext->Initialize(SnepServerHandle, DTAInterface, SnepServerInfo->usInboxSize);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "snepServerContext initialization failed with %!STATUS!", ntStatus);
        status = NfcCxNfcStatusFromNtStatus(ntStatus);
        delete snepServerContext;
        goto Done;
    }

    ntStatus = snepServerContext->EnableSnepConnection();
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSnepConnection failed with %!STATUS!", ntStatus);
        status = NfcCxNfcStatusFromNtStatus(ntStatus);
        delete snepServerContext;
        goto Done;
    }

    WdfWaitLockAcquire(DTAInterface->SnepServerLock, NULL);
    InsertTailList(&DTAInterface->SnepServerContext, snepServerContext->GetListEntry());
    WdfWaitLockRelease(DTAInterface->SnepServerLock);

    snepServerContext->SetSnepServer(*SnepServerHandle);
    *SnepServerHandle = snepServerContext;
    snepServerContext->SetServerType(SnepServerInfo->eServerType);

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerDeinit(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_SNEP_SERVER_HANDLE SnepServerHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    CNFCDtaSnepServerContext* snepServerContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    snepServerContext = (CNFCDtaSnepServerContext*)SnepServerHandle;

    status = phLibNfc_SnepServer_DeInit(snepServerContext->GetSnepServer());
    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepServer_DeInit failed with Status: %!NFCSTATUS!", status);
        goto Done;
    }

    WdfWaitLockAcquire(DTAInterface->SnepServerLock, NULL);
    RemoveEntryList(snepServerContext->GetListEntry());
    WdfWaitLockRelease(DTAInterface->SnepServerLock);

    delete snepServerContext;

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerGetConnection(
    _In_ NFC_SNEP_SERVER_HANDLE SnepServerHandle,
    _In_ PNFCCX_DTA_REQUEST_CONTEXT RequestContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CNFCDtaSnepServerContext* snepServerContext = NULL;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*)SnepServerHandle;
    dtaInterface = snepServerContext->GetDTAInterface();

    //
    // Pend a request to get connection
    //
    status = snepServerContext->GetConnection()->ProcessRequest(RequestContext->FileContext,
                                                                RequestContext->Request,
                                                                RequestContext->OutputBuffer,
                                                                RequestContext->OutputBufferLength);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

Done:
    dtaInterface->pLibNfcContext->Status = status;
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerGetNtfCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Data_t *pDataInbox,
    _In_ phLibNfc_Handle ConnHandle
)
{
    CNFCDtaSnepServerContext* snepServerContext = NULL;
    CNFCPayload* queuedSnepRequest = NULL;
    PNFC_SNEP_SERVER_REQUEST snepServerRequest = NULL;
    DWORD requiredBufferSpace;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*) pContext;

    if (NfcStatus != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_ERROR, "Get request error status %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    queuedSnepRequest = new CNFCPayload();
    if (NULL == queuedSnepRequest) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources (CNFCPayload)");
        goto Done;
    }

    requiredBufferSpace = pDataInbox->length +
                          offsetof(NFC_SNEP_SERVER_REQUEST, sRequestPayload) +
                          offsetof(NFC_DATA_BUFFER, pbBuffer);

    if (!NT_SUCCESS(queuedSnepRequest->Initialize(NULL, requiredBufferSpace))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources (Initialize)");
        delete queuedSnepRequest;
        goto Done;
    }

    snepServerRequest = (PNFC_SNEP_SERVER_REQUEST)queuedSnepRequest->GetPayload();
    snepServerRequest->hSnepServer = snepServerContext;
    snepServerRequest->hConnection = ConnHandle;
    snepServerRequest->eRequestType = SnepRequestGet;
    snepServerRequest->sRequestPayload.cbBuffer = (USHORT)pDataInbox->length;
    RtlCopyMemory(snepServerRequest->sRequestPayload.pbBuffer, pDataInbox->buffer, pDataInbox->length);

    if (!NT_SUCCESS(snepServerContext->GetRequest()->ProcessPayload(queuedSnepRequest))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSnepRequest;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerPutNtfCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Data_t *pDataInbox,
    _In_ phLibNfc_Handle ConnHandle
)
{
    CNFCDtaSnepServerContext* snepServerContext = NULL;
    CNFCPayload* queuedSnepRequest = NULL;
    PNFC_SNEP_SERVER_REQUEST snepServerRequest = NULL;
    DWORD requiredBufferSpace;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*) pContext;

    if (NfcStatus != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_ERROR, "Put request error status %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    queuedSnepRequest = new CNFCPayload();
    if (NULL == queuedSnepRequest) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources (CNFCPayload)");
        goto Done;
    }

    requiredBufferSpace = pDataInbox->length +
                          offsetof(NFC_SNEP_SERVER_REQUEST, sRequestPayload) +
                          offsetof(NFC_DATA_BUFFER, pbBuffer);

    if (!NT_SUCCESS(queuedSnepRequest->Initialize(NULL, requiredBufferSpace))) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources (Initialize)");
        delete queuedSnepRequest;
        goto Done;
    }

    snepServerRequest = (PNFC_SNEP_SERVER_REQUEST)queuedSnepRequest->GetPayload();
    snepServerRequest->hSnepServer = snepServerContext;
    snepServerRequest->hConnection = ConnHandle;
    snepServerRequest->eRequestType = SnepRequestPut;
    snepServerRequest->sRequestPayload.cbBuffer = (USHORT)pDataInbox->length;
    RtlCopyMemory(snepServerRequest->sRequestPayload.pbBuffer, pDataInbox->buffer, pDataInbox->length);

    if (!NT_SUCCESS(snepServerContext->GetRequest()->ProcessPayload(queuedSnepRequest))) {
        TRACE_LINE(LEVEL_INFO, "Process payload failed");
        delete queuedSnepRequest;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerAccept(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SNEP_SERVER_ACCEPT_INFO SnepServerAcceptInfo
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    phLibNfc_Llcp_sSocketOptions_t socketOption;
    CNFCDtaSnepServerContext* snepServerContext = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*)SnepServerAcceptInfo->hSnepServer;

    socketOption.miu = SnepServerAcceptInfo->sSocketOption.uMIUX;
    socketOption.rw = SnepServerAcceptInfo->sSocketOption.bRW;

    status = phLibNfc_SnepServer_Accept(snepServerContext->GetDataInbox(),
                                        &socketOption,
                                        DTAInterface->hRemoteDev,
                                        snepServerContext->GetSnepServer(),
                                        SnepServerAcceptInfo->hConnection,
                                        NfcCxDTAInterfaceSnepServerPutNtfCB,
                                        NfcCxDTAInterfaceSnepServerGetNtfCB,
                                        snepServerContext);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepServer_Accept completed immediately with %!NFCSTATUS!", status);
        DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
        SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);
        goto Done;
    }

    ntStatus = snepServerContext->EnableSnepRequest();
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "EnableSnepRequest failed with %!STATUS!", ntStatus);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerGetRequest(
    _In_ NFC_SNEP_SERVER_HANDLE SnepServerHandle,
    _In_ PNFCCX_DTA_REQUEST_CONTEXT RequestContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CNFCDtaSnepServerContext* snepServerContext = NULL;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*)SnepServerHandle;
    dtaInterface = snepServerContext->GetDTAInterface();

    //
    // Pend a request
    //
    status = snepServerContext->GetRequest()->ProcessRequest(RequestContext->FileContext,
                                                             RequestContext->Request,
                                                             RequestContext->OutputBuffer,
                                                             RequestContext->OutputBufferLength);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to process request with status %!STATUS!", status);
        goto Done;
    }

Done:
    dtaInterface->pLibNfcContext->Status = status;
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxDTAInterfaceMapSnepResponse(
    _In_ DWORD SnepResponse,
    _Out_ NFCSTATUS* pSnepResponse
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (SnepResponse) {
    case SNEP_RESPONSE_CONTINUE:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_CONTINUE;
        break;

    case SNEP_RESPONSE_SUCCESS:
        *pSnepResponse = NFCSTATUS_SUCCESS;
        break;

    case SNEP_RESPONSE_NOT_FOUND:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_NOT_FOUND;
        break;

    case SNEP_RESPONSE_EXCESS_DATA:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA;
        break;

    case SNEP_RESPONSE_BAD_REQUEST:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST;
        break;

    case SNEP_RESPONSE_NOT_IMPLEMENTED:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED;
        break;

    case SNEP_RESPONSE_UNSUPPORTED_VERSION:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION;
        break;

    case SNEP_RESPONSE_REJECT:
        *pSnepResponse = NFCSTATUS_SNEP_RESPONSE_REJECT;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

static VOID
NfcCxDTAInterfaceSnepServerSendResponseCB(
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus, 
    _In_ phLibNfc_Handle ConnHandle
    )
{
    CNFCDtaSnepServerContext* snepServerContext = NULL;
    PNFCCX_DTA_INTERFACE dtaInterface = NULL;
    PNFC_SNEP_SERVER_RESPONSE_INFO snepServerResponse = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*)Context;
    dtaInterface = snepServerContext->GetDTAInterface();
    snepServerResponse = snepServerContext->GetServerResponseInfo();

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "SNEP response error %!STATUS!", NfcStatus);
        goto Done;
    }

    snepServerResponse->hSnepServer = snepServerContext;
    snepServerResponse->hConnection = ConnHandle;
    snepServerResponse->dwResponseStatus = SNEP_RESPONSE_SUCCESS;
    snepServerResponse->sResponsePayload.cbBuffer = (USHORT)snepServerContext->GetDataInbox()->length;
    RtlCopyMemory(snepServerResponse->sResponsePayload.pbBuffer,
                  snepServerContext->GetDataInbox()->buffer,
                  snepServerContext->GetDataInbox()->length);

Done:
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepServerSendResponse(
    _In_ PNFC_SNEP_SERVER_RESPONSE_INFO SnepServerResponseEgress,
    _Out_ PNFC_SNEP_SERVER_RESPONSE_INFO SnepServerResponseIngress
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NFCSTATUS snepStatus = NFCSTATUS_SUCCESS;

    CNFCDtaSnepServerContext* snepServerContext = NULL;
    phNfc_sData_t responseData;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerContext = (CNFCDtaSnepServerContext*)SnepServerResponseEgress->hSnepServer;
    snepServerContext->SetServerResponseInfo(SnepServerResponseIngress);

    ntStatus = NfcCxDTAInterfaceMapSnepResponse(SnepServerResponseEgress->dwResponseStatus, &snepStatus);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid SNEP response code %d", SnepServerResponseEgress->dwResponseStatus);
        NfcCxDTAInterfaceSnepServerSendResponseCB(SnepServerResponseEgress->hSnepServer,
                                                  NfcCxNfcStatusFromNtStatus(ntStatus),
                                                  SnepServerResponseEgress->hConnection);
        goto Done;
    }

    responseData.length = SnepServerResponseEgress->sResponsePayload.cbBuffer;
    responseData.buffer = SnepServerResponseEgress->sResponsePayload.pbBuffer;
    snepServerContext->GetDataInbox()->length = 0;

    status = phLibNfc_SnepProtocolSrvSendResponse(SnepServerResponseEgress->hConnection,
                                                  (0 == responseData.length) ? NULL : &responseData,
                                                  snepStatus,
                                                  NfcCxDTAInterfaceSnepServerSendResponseCB,
                                                  SnepServerResponseEgress->hSnepServer);
    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepProtocolSrvSendResponse completed immediately with Status: %!NFCSTATUS!", status);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID NfcCxDTAInterfacDumpSnepClientConfig(
    _In_ PNFC_SNEP_CLIENT_INFO SnepClientInfo
    )
{
    TRACE_LINE(LEVEL_INFO, "SNEP Client Init Configurations:");
    TRACE_LINE(LEVEL_INFO, "================================");
    TRACE_LINE(LEVEL_INFO, "sService.cbServiceName %d", SnepClientInfo->sService.cbServiceName);
    TRACE_LINE(LEVEL_INFO, "eServerType %d", SnepClientInfo->eServerType);
    TRACE_LINE(LEVEL_INFO, "sSocketOption.uMIUX %d", SnepClientInfo->sSocketOption.uMIUX);
    TRACE_LINE(LEVEL_INFO, "sSocketOption.bRW %d", SnepClientInfo->sSocketOption.bRW);
    TRACE_LINE(LEVEL_INFO, "================================");
}

static VOID
NfcCxDTAInterfaceSnepClientInitCB(
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus, 
    _In_ phLibNfc_Handle ConnHandle
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NULL == ConnHandle || NFCSTATUS_CONNECTION_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_INFO, "Connection Failed, connection handle 0x%p, status %!NFCSTATUS!", ConnHandle, NfcStatus);
        NfcStatus = (NFCSTATUS_SUCCESS == NfcStatus) ? NFCSTATUS_FAILED : NfcStatus;
        *(dtaInterface->SnepClientHandle) = NULL;
        goto Done;
    }

    *(dtaInterface->SnepClientHandle) = ConnHandle;

Done:
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientInit(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SNEP_CLIENT_INFO SnepClientInfo,
    _Out_ PNFC_SNEP_CLIENT_HANDLE SnepClientHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phLibNfc_SnepConfig_t snepConfig;
    phNfc_sData_t snepServerName;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    snepServerName.length = SnepClientInfo->sService.cbServiceName;
    snepServerName.buffer = SnepClientInfo->sService.pbServiceName;

    snepConfig.SnepServerType = (DefaultSnepServer == SnepClientInfo->eServerType) ? phLibNfc_SnepServer_Default : phLibNfc_SnepServer_NonDefault;
    snepConfig.sOptions.miu = SnepClientInfo->sSocketOption.uMIUX;
    snepConfig.sOptions.rw = SnepClientInfo->sSocketOption.bRW;
    snepConfig.SnepServerName = (DefaultSnepServer == SnepClientInfo->eServerType) ? NULL : &snepServerName;
    snepConfig.bDtaFlag = TRUE;

    NfcCxDTAInterfacDumpSnepClientConfig(SnepClientInfo);

    DTAInterface->SnepClientHandle = SnepClientHandle;
    status = phLibNfc_SnepClient_Init(&snepConfig,
                                      SnepClientInfo->hRemoteDev,
                                      NfcCxDTAInterfaceSnepClientInitCB,
                                      DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepClient_Init complete immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSnepClientInitCB(DTAInterface, status, NULL);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientDeinit(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_SNEP_CLIENT_HANDLE SnepClientHandle
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_SnepClient_DeInit(SnepClientHandle);

    if (NFCSTATUS_SUCCESS != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepClient_DeInit failed with %!NFCSTATUS!", status);
        goto Done;
    }

Done:
    DTAInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(status);
    SetEvent(DTAInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientPutCB(
    _In_ phLibNfc_Handle /*ConnHandle*/, 
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Data_t* /*ReqResponse*/
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "SNEP client put operation failed with %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

Done:
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientPut(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SNEP_CLIENT_PUT_INFO SnepClientPutInfo
)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phNfc_sData_t putBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    putBuffer.buffer = SnepClientPutInfo->sPutPayload.pbBuffer;
    putBuffer.length = SnepClientPutInfo->sPutPayload.cbBuffer;

    status = phLibNfc_SnepProtocolCliReqPut(SnepClientPutInfo->hSnepClient,
                                            &putBuffer,
                                            NfcCxDTAInterfaceSnepClientPutCB,
                                            DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepProtocolCliReqPut completed immediately with %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSnepClientPutCB(SnepClientPutInfo->hSnepClient,
                                         DTAInterface,
                                         status,
                                         NULL);
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientGetCB(
    _In_ phLibNfc_Handle /*ConnHandle*/,
    _In_ PVOID Context, 
    _In_ NFCSTATUS NfcStatus,
    _In_ phLibNfc_Data_t* ReqResponse
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "SNEP client get operation failed with %!NFCSTATUS!", NfcStatus);
        goto Done;
    }

    if (NULL == ReqResponse) {
        dtaInterface->SnepClientDataBuffer->cbBuffer = 0;
        TRACE_LINE(LEVEL_ERROR, "SNEP client get response buffer null");
        goto Done;
    }

    if (ReqResponse->length > dtaInterface->SnepClientDataBuffer->cbBuffer) {
        NfcStatus = NFCSTATUS_BUFFER_TOO_SMALL;
        dtaInterface->SnepClientDataBuffer->cbBuffer = (USHORT) ReqResponse->length;
        TRACE_LINE(LEVEL_ERROR, "Receive buffer too small for response");
        goto Done;
    }

    dtaInterface->SnepClientDataBuffer->cbBuffer = (USHORT) ReqResponse->length;
    RtlCopyMemory(dtaInterface->SnepClientDataBuffer->pbBuffer,
                  ReqResponse->buffer,
                  ReqResponse->length);

Done:
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSnepClientGet(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SNEP_CLIENT_GET_INFO SnepClientGetInfo,
    _In_ PNFC_SNEP_CLIENT_DATA_BUFFER SnepClientDataBuffer
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phNfc_sData_t getData;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    getData.buffer = SnepClientGetInfo->sGetPayload.pbBuffer;
    getData.length = SnepClientGetInfo->sGetPayload.cbBuffer;

    DTAInterface->SnepClientDataBuffer = SnepClientDataBuffer;

    status = phLibNfc_SnepProtocolCliReqGet(SnepClientGetInfo->hSnepClient,
                                            &getData,
                                            SnepClientDataBuffer->cbBuffer,
                                            NfcCxDTAInterfaceSnepClientGetCB,
                                            DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SnepProtocolCliReqGet completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSnepClientGetCB(SnepClientGetInfo->hSnepClient,
                                         DTAInterface,
                                         status,
                                         NULL);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSeEnumerateCB(
    _In_ VOID *pContext, 
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;
    PNFC_SE_LIST seList = (PNFC_SE_LIST)(dtaInterface->Buffer + sizeof(DWORD));
    phLibNfc_SE_List_t nfcSEList[MAX_NUMBER_OF_SE];
    uint8_t nfcSECount;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "Se enumaertion failed");
        goto Done;
    }

    NfcStatus = phLibNfc_SE_GetSecureElementList(&nfcSEList[0], &nfcSECount);
    if (NFCSTATUS_SUCCESS != NfcStatus) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SE_GetSecureElementList failed with %!NFCSTATUS!", NfcStatus);
        goto Done;
    }
    nfcSECount += 1;

    *(DWORD*)dtaInterface->Buffer = sizeof(DWORD) + offsetof(NFC_SE_LIST, EndpointList) + nfcSECount * sizeof(NFC_SE_INFO);
    if (dtaInterface->BufferSize < *(DWORD*)dtaInterface->Buffer) {
        TRACE_LINE(LEVEL_ERROR, "Buffer overflow");
        NfcStatus = NFCSTATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    seList->NumberOfEndpoints = nfcSECount;
    seList->EndpointList[0].hSecureElement = NULL;
    seList->EndpointList[0].eSecureElementType = DeviceHost;

    for (uint8_t i = 1; i < nfcSECount; i++) {
        seList->EndpointList[i].hSecureElement = nfcSEList[i - 1].hSecureElement;
        seList->EndpointList[i].eSecureElementType = NfcCxSEInterfaceGetSecureElementType(nfcSEList[i - 1].eSE_Type);
        TRACE_LINE(LEVEL_INFO, "SE Endpoint [%d], type %d, handle 0x%p",
                   i,
                   seList->EndpointList[i].eSecureElementType,
                   seList->EndpointList[i].hSecureElement);
    }

Done:
    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSeEnumerate(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _Out_ PBYTE SeListBuffer,
    _In_ DWORD SeListBufferSize
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    DTAInterface->Buffer = SeListBuffer;
    DTAInterface->BufferSize = SeListBufferSize;

    status = phLibNfc_SE_Enumerate(NfcCxDTAInterfaceSeEnumerateCB,
                                   DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SE_Enumerate completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSeEnumerateCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSetCardEmualtionModeCB(
    _In_ VOID* pContext,
    _In_ phLibNfc_Handle /*hSecureElement*/,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSetCardEmualtionMode(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_SE_EMULATION_MODE_INFO SeEmulationMode
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_SE_SetMode(SeEmulationMode->hSecureElement,
                                 (SeEmulationMode->eMode == EmulationOff) ? phLibNfc_SE_ActModeOff : phLibNfc_SE_ActModeOn,
                                 NfcCxDTAInterfaceSetCardEmualtionModeCB,
                                 DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_SE_SetMode completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSetCardEmualtionModeCB(DTAInterface, NULL, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSetRoutingTableCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    dtaInterface->pLibNfcContext->Status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    SetEvent(dtaInterface->pLibNfcContext->hNotifyCompleteEvent);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceSetRoutingTable(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_reads_(RtngTableCount) phLibNfc_RtngConfig_t* pRtngTable,
    _In_ uint8_t RtngTableCount
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = phLibNfc_Mgt_ConfigRoutingTable(RtngTableCount,
                                             pRtngTable,
                                             NfcCxDTAInterfaceSetRoutingTableCB,
                                             DTAInterface);

    if (NFCSTATUS_PENDING != status) {
        TRACE_LINE(LEVEL_ERROR, "phLibNfc_Mgt_ConfigRoutingTable completed immediately with Status: %!NFCSTATUS!", status);
        NfcCxDTAInterfaceSetRoutingTableCB(DTAInterface, status);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxDTAInterfaceMessage(
    _Inout_ PVOID Context,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3
    )
{
    UNREFERENCED_PARAMETER(Context);
    PNFCCX_DTA_INTERFACE dtaInterface = (PNFCCX_DTA_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(NULL != dtaInterface);
    NT_ASSERT(NULL != dtaInterface->pLibNfcContext);

    TRACE_LINE(LEVEL_INFO, "%!NFCCX_DTA_MESSAGE!: %p, %p, %p", Message, (VOID*)Param1, (VOID*)Param2, (VOID*)Param3);

    switch (Message)
    {
    case DTA_SET_MODE:
        NfcCxDTAInterfaceSetDTAMode(dtaInterface, (BOOLEAN)Param1);
        break;

    case DTA_DISCOVER_CONFIG:
        NfcCxDTAInterfaceRfDiscoveryConfig(dtaInterface, (PNFC_RF_DISCOVERY_CONFIG)Param1);
        break;

    case DTA_REMOTE_DEV_CONNECT:
        NfcCxDTAInterfaceRemoteDevConnect(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_REMOTE_DEV_DISCONNECT:
        NfcCxDTAInterfaceRemoteDevDisconnect(dtaInterface, (PNFC_REMOTE_DEVICE_DISCONNET)Param1);
        break;

    case DTA_REMOTE_DEV_TRANSCEIVE:
        NfcCxDTAInterfaceRemoteDevTranscieve(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_REMOTE_DEV_SEND:
        NfcCxDTAInterfaceRemoteDevSend(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_P2P_CONFIG:
        NfcCxDTAInterfaceP2pConfig(dtaInterface, (PNFC_P2P_PARAM_CONFIG)Param1);
        break;

    case DTA_RF_CONFIG:
        NfcCxDTAInterfaceRfConfig(dtaInterface, (PBYTE)Param1, (DWORD)Param2);
        break;

    case DTA_REMOTE_DEV_NDEF_WRITE:
        NfcCxDTAInterfaceRemoteDevNdefWrite(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_REMOTE_DEV_NDEF_READ:
        NfcCxDTAInterfaceRemoteDevNdefRead(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_REMOTE_DEV_NDEF_CONVERT_READ_ONLY:
        NfcCxDTAInterfaceRemoteDevNdefConvertReadOnly(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_REMOTE_DEV_NDEF_CHECK:
        NfcCxDTAInterfaceRemoteDevNdefCheck(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_LLCP_CONFIG:
        NfcCxDTAInterfaceLlcpConfig(dtaInterface, (PNFC_LLCP_CONFIG)Param1);
        break;

    case DTA_LLCP_ACTIVATE:
        NfcCxDTAInterfaceLlcpActivate(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_LLCP_DEACTIVATE:
        NfcCxDTAInterfaceLlcpDeactivate(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_LLCP_DISCOVER_SERVICES:
        NfcCxDTAInterfaceLlcpDiscoverServices(dtaInterface,
                                              (PNFC_LLCP_SERVICE_DISCOVER_REQUEST)Param1,
                                              (PNFC_LLCP_SERVICE_DISCOVER_SAP)Param2);
        break;

    case DTA_LLCP_LINK_STATUS_CHECK:
        NfcCxDTAInterfaceLlcpLinkStatusCheck(dtaInterface, (NFC_REMOTE_DEV_HANDLE)Param1);
        break;

    case DTA_LLCP_SOCKET_CREATE:
        NfcCxDTAInterfaceLlcpSocketCreate(dtaInterface,
                                          (PNFC_LLCP_SOCKET_INFO)Param1,
                                          (PNFC_LLCP_SOCKET_HANDLE)Param2);
        break;

    case DTA_LLCP_SOCKET_CLOSE:
        NfcCxDTAInterfaceLlcpSocketClose(dtaInterface, (NFC_LLCP_SOCKET_HANDLE)Param1);
        break;

    case DTA_LLCP_SOCKET_BIND:
        NfcCxDTAInterfaceLlcpSocketBind(dtaInterface, (PNFC_LLCP_SOCKET_SERVICE_INFO)Param1);
        break;

    case DTA_LLCP_SOCKET_LISTEN:
        NfcCxDTAInterfaceLlcpSocketListen(dtaInterface,
                                          (NFC_LLCP_SOCKET_HANDLE)Param1,
                                          (PNFCCX_DTA_REQUEST_CONTEXT)Param2);
        break;

    case DTA_LLCP_SOCKET_ACCEPT:
        NfcCxDTAInterfaceLlcpSocketAccept(dtaInterface, (PNFC_LLCP_SOCKET_ACCEPT_INFO)Param1);
        break;

    case DTA_LLCP_SOCKET_CONNECT:
        NfcCxDTAInterfaceLlcpSocketConnect((PNFC_LLCP_SOCKET_CONNECT_INFO)Param1);
        break;

    case DTA_LLCP_SOCKET_DISCONNECT:
        NfcCxDTAInterfaceLlcpSocketDisconnect((NFC_LLCP_SOCKET_HANDLE)Param1);
        break;

    case DTA_LLCP_SOCKET_RECV:
        NfcCxDTAInterfaceLlcpSocketRecv((NFC_LLCP_SOCKET_HANDLE)Param1, (PNFCCX_DTA_REQUEST_CONTEXT)Param2);
        break;

    case DTA_LLCP_SOCKET_SEND:
        NfcCxDTAInterfaceLlcpSocketSend((PNFC_LLCP_SOCKET_PAYLOAD)Param1);
        break;

    case DTA_LLCP_SOCKET_GET_ERROR:
        NfcCxDTAInterfaceLlcpSocketGetError((NFC_LLCP_SOCKET_HANDLE)Param1, (PNFCCX_DTA_REQUEST_CONTEXT)Param2);
        break;

    case DTA_SNEP_SERVER_INIT:
        NfcCxDTAInterfaceSnepServerInit(dtaInterface,
                                        (PNFC_SNEP_SERVER_INFO)Param1,
                                        (PNFC_SNEP_SERVER_HANDLE)Param2);
        break;

    case DTA_SNEP_SERVER_DEINIT:
        NfcCxDTAInterfaceSnepServerDeinit(dtaInterface, (NFC_SNEP_SERVER_HANDLE)Param1);
        break;

    case DTA_SNEP_SERVER_GET_NEXT_CONNECTION:
        NfcCxDTAInterfaceSnepServerGetConnection((NFC_SNEP_SERVER_HANDLE)Param1, (PNFCCX_DTA_REQUEST_CONTEXT)Param2);
        break;

    case DTA_SNEP_SERVER_ACCEPT:
        NfcCxDTAInterfaceSnepServerAccept(dtaInterface, (PNFC_SNEP_SERVER_ACCEPT_INFO)Param1);
        break;

    case DTA_SNEP_SERVER_GET_NEXT_REQUEST:
        NfcCxDTAInterfaceSnepServerGetRequest((NFC_SNEP_SERVER_HANDLE)Param1, (PNFCCX_DTA_REQUEST_CONTEXT)Param2);
        break;

    case DTA_SNEP_SERVER_SEND_RESPONSE:
        NfcCxDTAInterfaceSnepServerSendResponse((PNFC_SNEP_SERVER_RESPONSE_INFO)Param1,
                                                (PNFC_SNEP_SERVER_RESPONSE_INFO)Param2);
        break;

    case DTA_SNEP_CLIENT_INIT:
        NfcCxDTAInterfaceSnepClientInit(dtaInterface,
                                        (PNFC_SNEP_CLIENT_INFO)Param1,
                                        (PNFC_SNEP_CLIENT_HANDLE)Param2);
        break;

    case DTA_SNEP_CLIENT_DEINIT:
        NfcCxDTAInterfaceSnepClientDeinit(dtaInterface, (NFC_SNEP_CLIENT_HANDLE)Param1);
        break;

    case DTA_SNEP_CLIENT_PUT:
        NfcCxDTAInterfaceSnepClientPut(dtaInterface, (PNFC_SNEP_CLIENT_PUT_INFO)Param1);
        break;

    case DTA_SNEP_CLIENT_GET:
        NfcCxDTAInterfaceSnepClientGet(dtaInterface,
                                       (PNFC_SNEP_CLIENT_GET_INFO)Param1,
                                       (PNFC_SNEP_CLIENT_DATA_BUFFER)Param2);
        break;

    case DTA_SE_ENUMERATE:
        NfcCxDTAInterfaceSeEnumerate(dtaInterface, (PBYTE)Param1, (DWORD)Param2);
        break;

    case DTA_SE_SET_EMULATION_MODE:
        NfcCxDTAInterfaceSetCardEmualtionMode(dtaInterface, (PNFC_SE_EMULATION_MODE_INFO)Param1);
        break;

    case DTA_SE_SET_ROUTING_TABLE:
        NfcCxDTAInterfaceSetRoutingTable(dtaInterface, (phLibNfc_RtngConfig_t*)Param1, (uint8_t)Param2);
        break;

    default:
        NT_ASSERTMSG("Invalid Message", FALSE);
        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}
