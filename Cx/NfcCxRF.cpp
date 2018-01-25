/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxRF.cpp

Abstract:

    RFInterface implementation

Environment:

    User-mode Driver Framework

--*/

#include "NfcCxPch.h"

#include "NfcCxRF.tmh"

// {D06B4CC1-1DA2-45c2-BD7B-877C4573F83B}
DEFINE_GUID(SESSION_ID,
0xd06b4cc1, 0x1da2, 0x45c2, 0xbd, 0x7b, 0x87, 0x7c, 0x45, 0x73, 0xf8, 0x3b);

//
// Local forward declarations
//
static VOID
NfcCxRFInterfaceLibNfcMessageHandler(
    _In_ LPVOID pContext,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    );

static VOID CALLBACK
NfcCxRFInterfaceTagPresenceThread(
    _In_ PTP_CALLBACK_INSTANCE pInstance,
    _In_ PVOID pContext,
    _In_ PTP_WORK pWork
    );

NTSTATUS
NfcCxRFInterfaceStateSEEvent(
    _In_ PNFCCX_RF_INTERFACE RfInterface,
    _In_ UINT32 eLibNfcMessage,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    );

FORCEINLINE PNFP_INTERFACE
NfcCxRFInterfaceGetNfpInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->FdoContext->NfpInterface;
}

FORCEINLINE PNFCCX_SC_INTERFACE
NfcCxRFInterfaceGetScInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->FdoContext->SCInterface;
}

FORCEINLINE PNFCCX_SE_INTERFACE
NfcCxRFInterfaceGetSEInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->FdoContext->SEInterface;
}

FORCEINLINE PNFCCX_STATE_INTERFACE
NfcCxRFInterfaceGetStateInterface(
    _In_ PNFCCX_RF_INTERFACE  RFInterface
    )
{
    return RFInterface->pLibNfcContext->StateInterface;
}

VOID
NfcCxRFInterfaceDumpState(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    TRACE_LINE(LEVEL_INFO, "Current RF state:");
    TRACE_LINE(LEVEL_INFO, "    bIsDiscoveryStarted = %d", RFInterface->bIsDiscoveryStarted);
    TRACE_LINE(LEVEL_INFO, "    DiscoveredDeviceList = %p", RFInterface->pLibNfcContext->pRemDevList);
}

static void
NfcCxRFInterfaceDumpSEList(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    TRACE_LINE(LEVEL_INFO, "SE Count = %d", RFInterface->pLibNfcContext->SECount);

    for (uint8_t i=0; i < RFInterface->pLibNfcContext->SECount; i++) {
        TRACE_LINE(LEVEL_INFO, "Secure Element [%d]:", i);
        TRACE_LINE(LEVEL_INFO, "    hSecureElement = %p", RFInterface->pLibNfcContext->SEList[i].hSecureElement);
        TRACE_LINE(LEVEL_INFO, "    Type = %!phLibNfc_SE_Type_t!", RFInterface->pLibNfcContext->SEList[i].eSE_Type);
        TRACE_LINE(LEVEL_INFO, "    ActivationMode = %!phLibNfc_eSE_ActivationMode!",  RFInterface->pLibNfcContext->SEList[i].eSE_ActivationMode);
    }
}

static void
NfcCxRFInterfaceGetSEList(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(MAX_NUMBER_OF_SE, *SECount) phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE],
    _Out_range_(<= , MAX_NUMBER_OF_SE) uint8_t *SECount
    )
{
    uint8_t bHceIndex = NfcCxRFInterfaceIsHCESupported(RFInterface) ? 1 : 0;
    phLibNfc_SE_GetSecureElementList(&SEList[bHceIndex], SECount);
    *SECount += bHceIndex;
}

static void
NfcCxRFInterfaceUpdateSEList(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    NfcCxRFInterfaceGetSEList(RFInterface,
                              RFInterface->pLibNfcContext->SEList,
                              &RFInterface->pLibNfcContext->SECount);

    NfcCxRFInterfaceDumpSEList(RFInterface);
}

FORCEINLINE NTSTATUS
NfcCxRFInterfaceGetSEEvent(
    _In_ phLibNfc_eSE_EvtType_t LibNfcEventType,
    _Out_ SECURE_ELEMENT_EVENT_TYPE* pSEEventType
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (LibNfcEventType) {
    case phLibNfc_eSE_EvtFieldOn:
        *pSEEventType = ExternalReaderArrival;
        break;
    case phLibNfc_eSE_EvtFieldOff:
        *pSEEventType = ExternalReaderDeparture;
        break;
    case phLibNfc_eSE_EvtTypeTransaction:
        *pSEEventType = Transaction;
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    return status;
}

VOID FORCEINLINE
NfcCxRFInterfaceClearRemoteDevList(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    if (RFInterface->pLibNfcContext->pRemDevList == NULL)
    {
        TRACE_LINE(LEVEL_INFO, "RemoteDevList is NULL, nothing to clear");
        return;
    }

    for (uint8_t bIndex = 0; bIndex < RFInterface->pLibNfcContext->uNoRemoteDevices; bIndex++)
    {
        if (RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo == NULL)
        {
            continue;
        }

        if ((RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
            (RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD))
        {
            RFInterface->pLibNfcContext->bIsHCEConnected = FALSE;
        }
        free(RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo);
        RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo = NULL;
    }
    free(RFInterface->pLibNfcContext->pRemDevList);
    RFInterface->pLibNfcContext->pRemDevList = NULL;
}


NTSTATUS FORCEINLINE
NfcCxRFInterfaceSetRemoteDevList(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_RemoteDevList_t *psRemoteDevList,
    _In_ uint8_t uNofRemoteDev
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    RFInterface->pLibNfcContext->SelectedProtocolIndex = 0;

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    RFInterface->pLibNfcContext->uNoRemoteDevices = uNofRemoteDev;

    RFInterface->pLibNfcContext->pRemDevList = (phLibNfc_RemoteDevList_t*)malloc(sizeof(phLibNfc_RemoteDevList_t) * uNofRemoteDev);
    if (RFInterface->pLibNfcContext->pRemDevList != NULL)
    {
        for (uint8_t bIndex = 0; bIndex < uNofRemoteDev; bIndex++)
        {
            RFInterface->pLibNfcContext->pRemDevList[bIndex].hTargetDev = psRemoteDevList[bIndex].hTargetDev;
            RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo =
                (phLibNfc_sRemoteDevInformation_t*)malloc(sizeof(phLibNfc_sRemoteDevInformation_t));

            if (RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo != NULL)
            {
                RtlCopyMemory(RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo,
                              psRemoteDevList[bIndex].psRemoteDevInfo,
                              sizeof(phLibNfc_sRemoteDevInformation_t));

                if ((RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
                    (RFInterface->pLibNfcContext->pRemDevList[bIndex].psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD))
                {
                    RFInterface->pLibNfcContext->bIsHCEConnected = TRUE;
                }
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                NfcCxRFInterfaceClearRemoteDevList(RFInterface);
                break;
            }
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}

phNfc_eRFDevType FORCEINLINE
NfcCxRFInterfaceGetGenericRemoteDevType(
    _In_ phNfc_eRFDevType RemoteDevType
    )
{
    switch (RemoteDevType) {
    case phNfc_eISO14443_4A_PICC:
    case phNfc_eISO14443_3A_PICC:
    case phNfc_eMifare_PICC:
    case phNfc_eISO14443_B_PICC:
    case phNfc_eISO14443_4B_PICC:
    case phNfc_eISO14443_BPrime_PICC:
    case phNfc_eFelica_PICC:
    case phNfc_eJewel_PICC:
    case phNfc_eISO15693_PICC:
    case phNfc_eISO14443_A_PICC:
    case phNfc_eKovio_PICC:
    case phNfc_ePICC_DevType:
        return phNfc_ePICC_DevType;

    case phNfc_eISO14443_A_PCD:
    case phNfc_eISO14443_B_PCD:
    case phNfc_eISO14443_BPrime_PCD:
    case phNfc_eFelica_PCD:
    case phNfc_eJewel_PCD:
    case phNfc_eISO15693_PCD:
    case phNfc_ePCD_DevType:
        return phNfc_ePCD_DevType;

    case phNfc_eNfcIP1_Target:
    case phNfc_eNfcIP1_Initiator:
        return RemoteDevType;

    default:
        return phNfc_eUnknown_DevType;
    }
}

NFCCX_CX_EVENT FORCEINLINE
NfcCxRFInterfaceGetEventType(
    _In_ UINT32 Message
    )
{
    switch (Message) {
    case LIBNFC_INIT:
        return NfcCxEventInit;
    case LIBNFC_DEINIT:
        return NfcCxEventDeinit;
    case LIBNFC_DISCOVER_CONFIG:
        return NfcCxEventConfigDiscovery;
    case LIBNFC_DISCOVER_STOP:
        return NfcCxEventStopDiscovery;
    case LIBNFC_TARGET_ACTIVATE:
        return NfcCxEventActivate;
    case LIBNFC_TARGET_DEACTIVATE_SLEEP:
        return NfcCxEventDeactivateSleep;
    case LIBNFC_TAG_WRITE:
    case LIBNFC_TAG_CONVERT_READONLY:
    case LIBNFC_TARGET_TRANSCEIVE:
    case LIBNFC_TARGET_SEND:
    case LIBNFC_TARGET_PRESENCE_CHECK:
    case LIBNFC_SNEP_CLIENT_PUT:
        return NfcCxEventDataXchg;
    case LIBNFC_SE_ENUMERATE:
    case LIBNFC_SE_SET_ROUTING_TABLE:
        return NfcCxEventConfig;
    case LIBNFC_SE_SET_MODE:
    case LIBNFC_EMEBEDDED_SE_TRANSCEIVE:
    case LIBNFC_EMBEDDED_SE_GET_ATR_STRING:
        return NfcCxEventSE;
    default:
        return NfcCxEventInvalid;
    }
}

VOID CALLBACK
NfcCxRFInterfaceWatchdogTimerCallback(
    _Inout_ PTP_CALLBACK_INSTANCE /*Instance*/,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_TIMER /*Timer*/
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_LINE(LEVEL_ERROR, "Watchdog timer timed out");

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxRfInterfaceWatchdogTimeout",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

    NfcCxDeviceSetFailed(rfInterface->FdoContext->Device);
}

VOID FORCEINLINE
NfcCxRFInterfaceStartWatchdogTimer(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ DWORD Timeout = MAX_WATCHDOG_TIMEOUT
    )
{
    if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_WTD_TIMER) == 0) {
        LONGLONG dueTime = WDF_REL_TIMEOUT_IN_MS(Timeout);
        SetThreadpoolTimer(RFInterface->tpWatchdogTimer, (FILETIME*)&dueTime, 0, 0);
    }
}

VOID FORCEINLINE
NfcCxRFInterfaceStopWatchdogTimer(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_WTD_TIMER) == 0) {
        SetThreadpoolTimer(RFInterface->tpWatchdogTimer, NULL, 0, 0);
    }
}

_Requires_lock_held_(RFInterface->DeviceLock)
NTSTATUS
NfcCxRFInterfaceExecute(
    _Inout_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    RFInterface->pLibNfcContext->Status = STATUS_SUCCESS;

    TRACE_LINE(LEVEL_VERBOSE, "ResetEvent, handle %p", RFInterface->pLibNfcContext->hNotifyCompleteEvent);
    if (!ResetEvent(RFInterface->pLibNfcContext->hNotifyCompleteEvent))
    {
        NTSTATUS status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "ResetEvent with handle %p failed, %!STATUS!", RFInterface->pLibNfcContext->hNotifyCompleteEvent, status);
    }

    NfcCxPostLibNfcThreadMessage(RFInterface, Message, Param1, Param2, NULL, NULL);
    DWORD dwWait = WaitForSingleObject(RFInterface->pLibNfcContext->hNotifyCompleteEvent, MAX_WATCHDOG_TIMEOUT);

    if (dwWait == WAIT_OBJECT_0 && RFInterface->pLibNfcContext->Status == STATUS_PENDING) {
        dwWait = NfcCxStateInterfaceWaitForUserEventToComplete(stateInterface, MAX_WATCHDOG_TIMEOUT);

        if (dwWait == WAIT_OBJECT_0) {
            RFInterface->pLibNfcContext->Status = (RFInterface->pLibNfcContext->Status != STATUS_PENDING) ?
                                                    RFInterface->pLibNfcContext->Status : STATUS_CANCELLED;
        }
    }

    if (dwWait != WAIT_OBJECT_0) {
        if (dwWait == WAIT_FAILED) {
            dwWait = GetLastError();
        }

        TRACE_LINE(LEVEL_ERROR, "%!NFCCX_LIBNFC_MESSAGE! timed out. Error: 0x%08X", Message, dwWait);

        TraceLoggingWrite(
            g_hNfcCxProvider,
            "NfcCxRfExecutionTimeout",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingUInt32(Message, "LibNfcMessage"),
            TraceLoggingUIntPtr(Param1, "Param1"),
            TraceLoggingUIntPtr(Param2, "Param2"),
            TraceLoggingHexUInt32(dwWait, "WaitError"));

        RFInterface->pLibNfcContext->Status = STATUS_UNSUCCESSFUL;
        NfcCxDeviceSetFailed(RFInterface->FdoContext->Device);

    } else if (!NT_SUCCESS(RFInterface->pLibNfcContext->Status)) {
        TRACE_LINE(LEVEL_ERROR, "%!NFCCX_LIBNFC_MESSAGE! failed, %!STATUS!", Message, RFInterface->pLibNfcContext->Status);
    }
    else {
        TRACE_LINE(LEVEL_INFO, "%!NFCCX_LIBNFC_MESSAGE! succeeded", Message);
    }

    return RFInterface->pLibNfcContext->Status;
}

NTSTATUS
NfcCxRFInterfaceCreate(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_RF_INTERFACE * PPRFInterface
    )
/*++

Routine Description:

    This routine creates and initalizes the RF Interface.

Arguments:

    FdoContext - A pointer to the FdoContext
    PPRFInterface - A pointer to a memory location to receive the allocated RF interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = NULL;
    phOsalNfc_Config_t config = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface = (PNFCCX_RF_INTERFACE)malloc(sizeof(*rfInterface));
    if (NULL == rfInterface) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the RF interface context");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(rfInterface, sizeof(*rfInterface));

    rfInterface->FdoContext = FdoContext;

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &rfInterface->DeviceLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the device lock, %!STATUS!", status);
        goto Done;
    }

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &rfInterface->SequenceLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the state lock, %!STATUS!", status);
        goto Done;
    }

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &rfInterface->PresenceCheckLock);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the presence check lock, %!STATUS!", status);
        goto Done;
    }

    rfInterface->tpWatchdogTimer = CreateThreadpoolTimer(NfcCxRFInterfaceWatchdogTimerCallback, rfInterface, NULL);
    if (rfInterface->tpWatchdogTimer == NULL) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create watchdog timer, %!STATUS!", status);
        goto Done;
    }

    rfInterface->pLibNfcContext = (PNFCCX_LIBNFC_CONTEXT)malloc(sizeof(*rfInterface->pLibNfcContext));
    if (NULL == rfInterface->pLibNfcContext) {
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate the LibNfc context");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }
    RtlZeroMemory(rfInterface->pLibNfcContext, sizeof(*rfInterface->pLibNfcContext));

    rfInterface->pLibNfcContext->hNotifyCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == rfInterface->pLibNfcContext->hNotifyCompleteEvent) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create notification complete event, %!STATUS!", status);
        goto Done;
    }

    rfInterface->hStartPresenceCheck = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (NULL == rfInterface->hStartPresenceCheck) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create presence check event, %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize DiscoveryConfig info with default values
    //
    rfInterface->uiPollDevInfo = NFC_CX_POLL_DEFAULT;
    rfInterface->uiNfcIP_Mode = NFC_CX_NFCIP_DEFAULT;
    rfInterface->uiDuration = NFC_CX_TOTAL_DURATION_DEFAULT * 1000;
    rfInterface->uiNfcIP_Tgt_Mode = NFC_CX_NFCIP_TGT_DEFAULT;
    rfInterface->uiNfcCE_Mode = NFC_CX_CE_DEFAULT;

    rfInterface->bKovioDetected = 0;

    //
    // Create the State Interface
    //
    status = NfcCxStateInterfaceCreate(FdoContext,
                                       &rfInterface->pLibNfcContext->StateInterface);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the State Interface, %!STATUS!", status);
        goto Done;
    }

    //
    // Create the LLCP Module
    //
    status = NfcCxLLCPInterfaceCreate(rfInterface,
                                      &rfInterface->pLibNfcContext->LLCPInterface);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the LLCP module, %!STATUS!", status);
        goto Done;
    }

    //
    // Create the SNEP Module
    //
    status = NfcCxSNEPInterfaceCreate(rfInterface,
                                      &rfInterface->pLibNfcContext->SNEPInterface);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to create the SNEP module, %!STATUS!", status);
        goto Done;
    }

    //
    // Initialize OSAL
    //
    config.pfnCallback = NfcCxRFInterfaceLibNfcMessageHandler;
    config.pCallbackContext = rfInterface;

    status = NfcCxNtStatusFromNfcStatus(phOsalNfc_Init(&config));
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Failed to initialize OSAL, %!STATUS!", status);
        goto Done;
    }

    rfInterface->pLibNfcContext->LibNfcThreadId = config.dwCallbackThreadId;

    //
    // CoreLib configurations
    //
    rfInterface->LibConfig.bConfigOpt = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_ENABLE_EEPROM_WRITE_PROTECTION) != 0;
    rfInterface->LibConfig.bHciNwkPerNfcee = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_HCI_NETWORK_PER_NFCEE) != 0;
    rfInterface->LibConfig.bNfceeActionNtf = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_NFCEE_ACTION_NTF) == 0;
    rfInterface->LibConfig.bIsoDepPresChkCmd = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_ISODEP_RNAK_PRESENCE_CHK_SUPPORTED) != 0;
    rfInterface->LibConfig.bSwitchedOnSubState = (FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_RF_ROUTING_POWER_SUB_STATES_SUPPORTED) != 0;
    rfInterface->LibConfig.bLogNciDataMessages = FdoContext->LogNciDataMessages;

    //
    // Register for below list of Protocols
    //
    rfInterface->RegInfo.MifareUL = 1;
    rfInterface->RegInfo.MifareStd = 1;
    rfInterface->RegInfo.ISO14443_4A = 1;
    rfInterface->RegInfo.ISO14443_4B = 1;
    rfInterface->RegInfo.Felica = 1;
    rfInterface->RegInfo.Jewel = 1;
    rfInterface->RegInfo.ISO15693 = 1;
    rfInterface->RegInfo.NFC = 1;
    rfInterface->RegInfo.Kovio = 1;

    rfInterface->tpTagPrescenceWork = CreateThreadpoolWork(NfcCxRFInterfaceTagPresenceThread, rfInterface, NULL);
    if (NULL == rfInterface->tpTagPrescenceWork) {
        status = NTSTATUS_FROM_WIN32(GetLastError());
        TRACE_LINE(LEVEL_ERROR, "Failed to create tag prescence work, %!STATUS!", status);
        goto Done;
    }

Done:
    if (!NT_SUCCESS(status)) {
        if (NULL != rfInterface) {
            NfcCxRFInterfaceDestroy(rfInterface);
            rfInterface = NULL;
        }
    }

    *PPRFInterface = rfInterface;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxRFInterfaceDestroy(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine cleans up the RF Interface

Arguments:

    RFInterface - A pointer to the RFInterface to cleanup.

Return Value:

    None

--*/
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Ensure that the prescence check thread has shutdown before deleting 'pLibNfcContext' to avoid a race condition.
    // Note: It is expected that 'NfcCxRFInterfaceStop' has been called at this point. So in theory no new versions of
    // the thread should be created.
    if (NULL != RFInterface->tpTagPrescenceWork) {
        WaitForThreadpoolWorkCallbacks(RFInterface->tpTagPrescenceWork, /*fCancelPendingCallbacks*/ TRUE);
        CloseThreadpoolWork(RFInterface->tpTagPrescenceWork);
        RFInterface->tpTagPrescenceWork = NULL;
    }

    //
    // Perform any cleanup associated with the RFInterface
    //
    if (NULL != RFInterface->pLibNfcContext) {

        phOsalNfc_DeInit();

        //
        // Destroy the SNEP Module
        //
        if (NULL != RFInterface->pLibNfcContext->SNEPInterface) {
            NfcCxSNEPInterfaceDestroy(RFInterface->pLibNfcContext->SNEPInterface);
            RFInterface->pLibNfcContext->SNEPInterface = NULL;
        }

        //
        // Destroy the LLCP Module
        //
        if (NULL != RFInterface->pLibNfcContext->LLCPInterface) {
            NfcCxLLCPInterfaceDestroy(RFInterface->pLibNfcContext->LLCPInterface);
            RFInterface->pLibNfcContext->LLCPInterface = NULL;
        }

        //
        // Destroy the State Module
        //
        if (NULL != RFInterface->pLibNfcContext->StateInterface) {
            NfcCxStateInterfaceDestroy(RFInterface->pLibNfcContext->StateInterface);
            RFInterface->pLibNfcContext->StateInterface = NULL;
        }

        if (NULL != RFInterface->pLibNfcContext->hNotifyCompleteEvent) {
            CloseHandle(RFInterface->pLibNfcContext->hNotifyCompleteEvent);
            RFInterface->pLibNfcContext->hNotifyCompleteEvent = NULL;
        }

        free(RFInterface->pLibNfcContext);
        RFInterface->pLibNfcContext = NULL;
    }

    if (NULL != RFInterface->hStartPresenceCheck) {
        CloseHandle(RFInterface->hStartPresenceCheck);
        RFInterface->hStartPresenceCheck = NULL;
    }

    if (NULL != RFInterface->tpWatchdogTimer) {
        CloseThreadpoolTimer(RFInterface->tpWatchdogTimer);
        RFInterface->tpWatchdogTimer = NULL;
    }

    if (NULL != RFInterface->PresenceCheckLock) {
        WdfObjectDelete(RFInterface->PresenceCheckLock);
        RFInterface->PresenceCheckLock = NULL;
    }

    if (NULL != RFInterface->SequenceLock) {
        WdfObjectDelete(RFInterface->SequenceLock);
        RFInterface->SequenceLock = NULL;
    }

    if (NULL != RFInterface->DeviceLock) {
        WdfObjectDelete(RFInterface->DeviceLock);
        RFInterface->DeviceLock = NULL;
    }

    free(RFInterface);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxRFInterfaceStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    Start the RFInterface

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

#ifdef EVENT_WRITE
    EventWriteRfInitializeStart();
#endif

    //
    // Start the RF Module
    //
    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);
    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_INIT, NULL, NULL);
    WdfWaitLockRelease(RFInterface->DeviceLock);

#ifdef EVENT_WRITE
    EventWriteRfInitializeStop(status);
#endif

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceStop(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    Stop the RFInterface

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Stop the RF Module
    //
    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DEINIT, NULL, NULL);

    RFInterface->RFPowerState = NfcCxPowerRfState_None;

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceSetDiscoveryConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PCNFC_CX_RF_DISCOVERY_CONFIG Config
    )
/*++

Routine Description:

    Set RF discovery configuration

Arguments:

    RFInterface - The RF Interface
    Config - The RF discovery configuration

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->uiDuration = Config->TotalDuration * 1000;
    RFInterface->uiPollDevInfo = Config->PollConfig;
    RFInterface->uiNfcIP_Mode = Config->NfcIPMode;
    RFInterface->uiNfcIP_Tgt_Mode = Config->NfcIPTgtMode;
    RFInterface->uiNfcCE_Mode = Config->NfcCEMode;
    RFInterface->uiBailout = Config->BailoutConfig;

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_LINE(LEVEL_INFO, "TotalDuration=%d PollConfig=0x%08x NfcIPMode=0x%02x NfcIPTgtMode=0x%02x NfcCEMode=0x%02x BailoutConfig=0x%02x",
                            Config->TotalDuration, Config->PollConfig, Config->NfcIPMode, Config->NfcIPTgtMode, Config->NfcCEMode, Config->BailoutConfig);

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxRFInterfaceSetDiscoveryConfig",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue(Config->TotalDuration, "TotalDuration"),
        TraceLoggingHexInt32(Config->PollConfig, "PollConfig"),
        TraceLoggingHexInt32(Config->NfcIPMode, "NfcIPMode"),
        TraceLoggingHexInt32(Config->NfcIPTgtMode, "NfcIPTgtMode"),
        TraceLoggingHexInt32(Config->NfcCEMode, "NfcCEMode"));

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceSetLLCPConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ PCNFC_CX_LLCP_CONFIG Config
    )
/*++

Routine Description:

    Set LLCP configuration

Arguments:

    RFInterface - The RF Interface
    Config - The LLCP configuration

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->pLibNfcContext->LLCPInterface->sLlcpConfigParams.uMIU = Config->Miu;
    RFInterface->pLibNfcContext->LLCPInterface->sLlcpConfigParams.uLTO = Config->LinkTimeout;
    RFInterface->pLibNfcContext->LLCPInterface->sLlcpConfigParams.uRecvWindowSize = Config->RecvWindowSize;

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_LINE(LEVEL_INFO, "Miu=%d LinkTimeout=%d RecvWindowSize=%d", Config->Miu, Config->LinkTimeout, Config->RecvWindowSize);

    TraceLoggingWrite(
        g_hNfcCxProvider,
        "NfcCxRFInterfaceSetLLCPConfig",
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue(Config->Miu, "Miu"),
        TraceLoggingValue(Config->LinkTimeout, "LinkTimeout"),
        TraceLoggingValue(Config->RecvWindowSize, "RecvWindowSize"));

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceRegisterSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_SEQUENCE Sequence,
    _In_ PFN_NFC_CX_SEQUENCE_HANDLER EvtNfcCxSequenceHandler
    )
/*++

Routine Description:

    Registers a sequence handler

Arguments:

    RFInterface - The RF Interface
    Sequence - The sequence to register
    EvtNfcCxSequenceHandler - The sequence handler callback

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[Sequence] != NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Sequence: %!NFC_CX_SEQUENCE!", Sequence);
    RFInterface->SeqHandlers[Sequence] = EvtNfcCxSequenceHandler;

Done:
    WdfWaitLockRelease(RFInterface->SequenceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceUnregisterSequenceHandler(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_SEQUENCE Sequence
    )
/*++

Routine Description:

    Unregisters a sequence handler

Arguments:

    RFInterface - The RF Interface
    Sequence - The sequence to unregister

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[Sequence] == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Sequence: %!NFC_CX_SEQUENCE!", Sequence);
    RFInterface->SeqHandlers[Sequence] = NULL;

Done:
    WdfWaitLockRelease(RFInterface->SequenceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceUpdateDiscoveryState(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NFC_CX_POWER_RF_STATE RFPowerState
    )
/*++

Routine Description:

    This routine is called from the power module to
    update the RF discovery state of the device

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    NFC_CX_POWER_RF_STATE previousRFPowerState = RFInterface->RFPowerState;

    if (previousRFPowerState != RFPowerState)
    {
        RFInterface->RFPowerState = RFPowerState;

        status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DISCOVER_CONFIG, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            // Restore 'RFPowerState' to its original value.
            RFInterface->RFPowerState = previousRFPowerState;
        }
    }

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
NfcCxRFInterfaceWriteTag(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ CNFCProximityBuffer * SendBuffer
    )
/*++

Routine Description:

    This routine is called from the Nfp module to
    perform a write to a tag once the tag connection
    has been established

Arguments:

    RFInterface - The RF Interface
    SendBuffer - The buffer to write

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if ((SendBuffer->GetSize() == 0) || (SendBuffer->Get() == NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->sSendBuffer.buffer = SendBuffer->Get();
    RFInterface->sSendBuffer.length = SendBuffer->GetSize();

    ResetEvent(RFInterface->hStartPresenceCheck);
    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TAG_WRITE, NULL, NULL);
    SetEvent(RFInterface->hStartPresenceCheck);

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceWriteP2P(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ CNFCProximityBuffer * SendBuffer
    )
/*++

Routine Description:

    This routine is called from the Nfp module to
    perform a write operation to a P2P device once
    a connection has been established.

Arguments:

    RFInterface - The RF Interface
    SendBuffer - The buffer to write

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (SendBuffer->GetSize() > MAX_MESSAGE_SIZE) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    if ((SendBuffer->GetSize() == 0) || (SendBuffer->Get() == NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->pLibNfcContext->SNEPInterface->sSendDataBuff.buffer = (uint8_t*)SendBuffer->Get();
    RFInterface->pLibNfcContext->SNEPInterface->sSendDataBuff.length = SendBuffer->GetSize();
    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_SNEP_CLIENT_PUT, NULL, NULL);

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTransmit(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed,
    _In_ USHORT Timeout
    )
/*++

Routine Description:

    This routine is called from the SmartCard module to perform a transmit

Arguments:

    RFInterface - The RF Interface
    RequestId - The request Id of the transmit operation
    InputBuffer - Pointer to the buffer to be transmitted
    InputBufferLength - Length of the input buffer
    OutputBuffer - Pointer to the buffer to be filled by the transmit response
    OutputBufferLength - Length of the output buffer
    pOutputBufferUsed - Pointer to the length of the output buffer used
    Timeout - Transceive timeout in milliseconds

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Validating the buffer sizes to safeguard the DWORD cast.
    //
    if (DWORD_MAX < InputBufferLength ||
        DWORD_MAX < OutputBufferLength) {
        NT_ASSERTMSG("Buffer size is validated at dispatch time, this should never be larger then DWORD_MAX", FALSE);
        status = STATUS_INVALID_BUFFER_SIZE;
        goto Done;
    }

    *pOutputBufferUsed = 0;

    if ((InputBufferLength == 0) || (InputBuffer == NULL) || (Timeout > MAX_TRANSCEIVE_TIMEOUT)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RtlZeroMemory(&RFInterface->sTransceiveBuffer, sizeof(RFInterface->sTransceiveBuffer));

    RFInterface->sTransceiveBuffer.timeout = Timeout;
    RFInterface->sTransceiveBuffer.sSendData.buffer = InputBuffer;
    RFInterface->sTransceiveBuffer.sSendData.length = (DWORD)InputBufferLength;
    RFInterface->sTransceiveBuffer.sRecvData.buffer = OutputBuffer;
    RFInterface->sTransceiveBuffer.sRecvData.length = (DWORD)OutputBufferLength;

    ResetEvent(RFInterface->hStartPresenceCheck);

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_TRANSCEIVE, NULL, NULL);
    *pOutputBufferUsed = (UINT32)RFInterface->sTransceiveBuffer.sRecvData.length;
    _Analysis_assume_(OutputBufferLength >= *pOutputBufferUsed);

    SetEvent(RFInterface->hStartPresenceCheck);

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTargetSend(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength
    )
/*++

Routine Description:

    This routine is called from the SmartCard module to perform a HCE remote send

Arguments:

    RFInterface - The RF Interface
    InputBuffer - Pointer to the buffer to be transmitted
    InputBufferLength - Length of the input buffer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    if (RFInterface->pLibNfcContext->bIsHCERecv) {
        TRACE_LINE(LEVEL_ERROR, "Received two or more consecutive responses");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    RFInterface->sSendBuffer.buffer = InputBuffer;
    RFInterface->sSendBuffer.length = (DWORD)InputBufferLength;

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_SEND, NULL, NULL);

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceConvertToReadOnlyTag(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine is called from the Nfp module to
    convert tag to readonly.

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    ResetEvent(RFInterface->hStartPresenceCheck);
    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TAG_CONVERT_READONLY, NULL, NULL);
    SetEvent(RFInterface->hStartPresenceCheck);

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTargetReactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine is called from the SC module to
    reactivate a tag.

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);
    ResetEvent(RFInterface->hStartPresenceCheck);

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_DEACTIVATE_SLEEP, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Target deactivation failed, %!STATUS!", status);
        goto Done;
    }

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_ACTIVATE, NULL, NULL);

Done:
    SetEvent(RFInterface->hStartPresenceCheck);
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTargetDeactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
)
/*++

Routine Description:

This routine is called from the SC module to
reactivate a tag.

Arguments:

RFInterface - The RF Interface

Return Value:

NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);
    ResetEvent(RFInterface->hStartPresenceCheck);

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_DEACTIVATE_SLEEP, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Target deactivation failed, %!STATUS!", status);
        SetEvent(RFInterface->hStartPresenceCheck);
        goto Done;
    }

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTargetActivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface
)
/*++

Routine Description:

This routine is called from the SC module to
reactivate a tag.

Arguments:

RFInterface - The RF Interface

Return Value:

NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_ACTIVATE, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_ERROR, "Target Activation failed, %!STATUS!", status);
    }
    SetEvent(RFInterface->hStartPresenceCheck);

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceTargetCheckPresence(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine is called to perform a target presence check.

Arguments:

    RFInterface - The RF Interface

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);
    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_TARGET_PRESENCE_CHECK, NULL, NULL);
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceGetSecureElementList(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(MAX_NUMBER_OF_SE, *SECount) phLibNfc_SE_List_t SEList[MAX_NUMBER_OF_SE],
    _Out_range_(<=, MAX_NUMBER_OF_SE) uint8_t *SECount,
    _In_ BOOLEAN EnableSEDiscovery
    )
/*++

Routine Description:

    This routine is called from the SE module to
    enumerate the list of secure elements attached to the controller.

Arguments:

    RFInterface - The RF Interface
    SEList - The list of secure elements enumerated
    SECount - The number of valid secure elements
    EnableSEDiscovery - A flag to indicate if enumeration/discovery of SEs needs to be performed

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS enumerateStatus = STATUS_SUCCESS;
    BOOLEAN fAcquireLock = RFInterface->pLibNfcContext->LibNfcThreadId != GetCurrentThreadId();

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (fAcquireLock) {
        WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);
    }

    if (EnableSEDiscovery &&
        !RFInterface->pLibNfcContext->EnableSEDiscovery &&
        (RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_NFCEE_DISCOVERY) == 0) {

        NT_ASSERT(fAcquireLock);

        //
        // Enumerating SE requires discovery process to be stopped
        //
        status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DISCOVER_STOP, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to stop discovery process, %!STATUS!", status);
            goto Done;
        }

        enumerateStatus = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_SE_ENUMERATE, NULL, NULL);
        if (NT_SUCCESS(enumerateStatus)) {
            RFInterface->pLibNfcContext->EnableSEDiscovery = TRUE;
        }
        else {
            // If enumerating SEs failed, don't goto Done yet. We want to restart the discovery
            // process first
            TRACE_LINE(LEVEL_ERROR, "Failed to enumerate SE, %!STATUS!", enumerateStatus);
        }

        status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DISCOVER_CONFIG, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            TRACE_LINE(LEVEL_ERROR, "Failed to start discovery process, %!STATUS!", status);
        }

        if (!NT_SUCCESS(enumerateStatus)) {
            status = enumerateStatus;
            goto Done;
        }
        else if (!NT_SUCCESS(status)) {
            goto Done;
        }
    }

    _Analysis_assume_(MAX_NUMBER_OF_SE >= RFInterface->pLibNfcContext->SECount);
    RtlCopyMemory(SEList, RFInterface->pLibNfcContext->SEList, RFInterface->pLibNfcContext->SECount * sizeof(phLibNfc_SE_List_t));
    *SECount = RFInterface->pLibNfcContext->SECount;

Done:
    if (fAcquireLock) {
        WdfWaitLockRelease(RFInterface->DeviceLock);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static phLibNfc_SE_List_t*
NfcCxRFInterfaceGetSecureElementFromHandle(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (uint8_t i = 0; i < RFInterface->pLibNfcContext->SECount; i++)
    {
        phLibNfc_SE_List_t* se = &RFInterface->pLibNfcContext->SEList[i];

        if (se->hSecureElement == hSecureElement)
        {
            TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
            return se;
        }
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return nullptr;
}

static NTSTATUS
NfcCxRFInterfaceGetSecureElementFromHandle(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _Outptr_ phLibNfc_SE_List_t** ppSecureElement)
{
    NTSTATUS status = STATUS_SUCCESS;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    phLibNfc_SE_List_t* pSecureElement = NfcCxRFInterfaceGetSecureElementFromHandle(RFInterface, hSecureElement);

    if (pSecureElement == nullptr)
    {
        status = STATUS_INVALID_PARAMETER;
        TRACE_LINE(LEVEL_ERROR, "Invalid SE handle, %!STATUS!", status);
        goto Done;
    }

    *ppSecureElement = pSecureElement;

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceSetCardActivationMode(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phLibNfc_eSE_ActivationMode eActivationMode,
    _In_ phLibNfc_PowerLinkModes_t ePowerAndLinkControl
    )
/*++

Routine Description:

    This routine is called from the SE module to
    set card emulation mode of the SE.

Arguments:

    RFInterface - The RF Interface
    hSecureElement - The handle to the secure element
    eActivationMode - The activation mode to set

Return Value:

    STATUS_SUCCESS - If card emulation mode is correctly set
    STATUS_INVALID_PARAMETER - If handle to secure element is invalid

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    TRACE_LINE(LEVEL_INFO, "%!phLibNfc_eSE_ActivationMode!, %!phLibNfc_PowerLinkModes_t!", eActivationMode, ePowerAndLinkControl);

    phLibNfc_SE_List_t* pSecureElement = nullptr;
    status = NfcCxRFInterfaceGetSecureElementFromHandle(RFInterface, hSecureElement, &pSecureElement);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }

    // Check if the activation mode and power-and-link-control are already correct.
    if (eActivationMode == pSecureElement->eSE_ActivationMode &&
        ePowerAndLinkControl == pSecureElement->eSE_PowerLinkMode)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_INFO, "Already correct. Nothing to do.");
        goto Done;
    }

    RFInterface->SEActivationMode = eActivationMode;
    RFInterface->SEPowerAndLinkControl = ePowerAndLinkControl;
    status = NfcCxRFInterfaceExecute(RFInterface,
                                     LIBNFC_SE_SET_MODE,
                                     (UINT_PTR)pSecureElement,
                                     0);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to set SE activation mode, %!STATUS!", status);
        goto Done;
    }

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceResetCard(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ phLibNfc_Handle hSecureElement
    )
/*++

Routine Description:

    Resets an SE.

Arguments:

    RFInterface - The RF Interface
    hSecureElement - The handle to the secure element

Return Value:

    STATUS_SUCCESS - If card emulation mode is correctly set
    STATUS_INVALID_PARAMETER - If handle to secure element is invalid

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    phLibNfc_SE_List_t* pSecureElement = nullptr;
    status = NfcCxRFInterfaceGetSecureElementFromHandle(RFInterface, hSecureElement, &pSecureElement);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "%!STATUS!", status);
        goto Done;
    }

    phLibNfc_eSE_ActivationMode currentActivationMode = pSecureElement->eSE_ActivationMode;
    phLibNfc_PowerLinkModes_t currentPowerLinkMode = pSecureElement->eSE_PowerLinkMode;

    //
    // Turn off SE
    //
    RFInterface->SEActivationMode = phLibNfc_SE_ActModeOff;
    RFInterface->SEPowerAndLinkControl = phLibNfc_PLM_NfccDecides;
    status = NfcCxRFInterfaceExecute(RFInterface,
                                     LIBNFC_SE_SET_MODE,
                                     (UINT_PTR)pSecureElement,
                                     0);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to turn off SE, %!STATUS!", status);
        goto Done;
    }

    //
    // Restore SE's activation mode
    //
    RFInterface->SEActivationMode = currentActivationMode;
    RFInterface->SEPowerAndLinkControl = currentPowerLinkMode;
    status = NfcCxRFInterfaceExecute(RFInterface,
                                     LIBNFC_SE_SET_MODE,
                                     (UINT_PTR)pSecureElement,
                                     0);
    if (!NT_SUCCESS(status))
    {
        TRACE_LINE(LEVEL_ERROR, "Failed to restore SE activation mode, %!STATUS!", status);
        goto Done;
    }

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceSetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_(RtngTableCount) phLibNfc_RtngConfig_t* pRtngTable,
    _In_ uint8_t RtngTableCount
    )
/*++

Routine Description:

    This routine is called from the SE module to configure the routing table

Arguments:

    RFInterface - The RF Interface
    pRtngTable - The pointer to the routing table
    RtngTableCount - The count of the routing table

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (RtngTableCount == 0 ||
        RtngTableCount > ARRAYSIZE(RFInterface->pLibNfcContext->RtngTable)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    //
    // Setting routing table requires discovery process to be stopped
    //
    NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DISCOVER_STOP, NULL, NULL);

    status = NfcCxRFInterfaceExecute(RFInterface,
                                     LIBNFC_SE_SET_ROUTING_TABLE,
                                     (UINT_PTR)RtngTableCount, (UINT_PTR)pRtngTable);

    if (NT_SUCCESS(status)) {
        RFInterface->pLibNfcContext->RtngTableCount = RtngTableCount;
        RtlCopyMemory(RFInterface->pLibNfcContext->RtngTable, pRtngTable, sizeof(phLibNfc_RtngConfig_t) * RtngTableCount);
    }

    NfcCxRFInterfaceExecute(RFInterface, LIBNFC_DISCOVER_CONFIG, NULL, NULL);

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceGetRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_to_(RtngTableCount, *pRtngTableCount) phLibNfc_RtngConfig_t* pRtngTable,
    _In_ uint8_t RtngTableCount,
    _Out_ uint8_t *pRtngTableCount
    )
/*++

Routine Description:

This routine is called from the SE module to retrieve the current routing table

Arguments:

RFInterface - The RF Interface
pRtngTable - The pointer to receive the routing table
RtngTableCount - The max size of the routing table
pRtngTableCount - The size of the copied routing table

Return Value:

NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    if (RFInterface->pLibNfcContext->RtngTableCount > RtngTableCount) {
        *pRtngTableCount = RFInterface->pLibNfcContext->RtngTableCount;
        status = STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    *pRtngTableCount = RFInterface->pLibNfcContext->RtngTableCount;
    RtlCopyMemory(pRtngTable, RFInterface->pLibNfcContext->RtngTable, *pRtngTableCount * sizeof(phLibNfc_RtngConfig_t));

Done:
    WdfWaitLockRelease(RFInterface->DeviceLock);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceGetNfccCapabilities(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_ PSECURE_ELEMENT_NFCC_CAPABILITIES pCapabilities
)
/*++

Routine Description:

    This routine is called from the SE module to retrieve the current NFCC capabilites

Arguments:

    RFInterface - The RF Interface
    pCapabilities - The pointer to receive the NFCC capabilites

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    pCapabilities->cbMaxRoutingTableSize =
        RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingTableSize;
    pCapabilities->IsAidRoutingSupported =
        (RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.AidBasedRouting == 1);
    pCapabilities->IsProtocolRoutingSupported =
        (RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.ProtocolBasedRouting == 1);
    pCapabilities->IsTechRoutingSupported =
        (RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.TechnBasedRouting == 1);

    WdfWaitLockRelease(RFInterface->DeviceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

BOOLEAN
NfcCxRFInterfaceIsHCESupported(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
/*++

Routine Description:

    This routine is used to determine if HCE is supported by the NFCCX

Arguments:

    RFInterface - The RF Interface

Return Value:

    TRUE if HCE is supported, FALSE is not supported

--*/
{
    return (RFInterface->pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.AidBasedRouting == 1) &&
           ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_HOST_CARD_EMULATION) == 0);
}

VOID
NfcCxPostLibNfcThreadMessage(
    _Inout_ PVOID /*Context*/,
    _In_ DWORD Message,
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
    return phOsalNfc_PostMsg(Message, Param1, Param2, Param3, Param4);
}

NTSTATUS
NfcCxRFInterfaceESETransmit(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_reads_bytes_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed,
    _In_ USHORT Timeout
    )
/*++

Routine Description:

    This routine is called from the EmbeddedSE module to perform a transmit

Arguments:

    RFInterface - The RF Interface
    RequestId - The request Id of the transmit operation
    InputBuffer - Pointer to the buffer to be transmitted
    InputBufferLength - Length of the input buffer
    OutputBuffer - Pointer to the buffer to be filled by the transmit response
    OutputBufferLength - Length of the output buffer
    pOutputBufferUsed - Pointer to the length of the output buffer used
    Timeout - Transceive timeout in milliseconds

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    UNREFERENCED_PARAMETER(Timeout);

    //
    // Validating the buffer sizes to safeguard the DWORD cast.
    // 
    if (DWORD_MAX < InputBufferLength ||
        DWORD_MAX < OutputBufferLength) {
        NT_ASSERTMSG("Buffer size is validated at dispatch time, this should never be larger than DWORD_MAX", FALSE);
        status = STATUS_INVALID_BUFFER_SIZE;
        goto Done;
    }

    *pOutputBufferUsed = 0;

    if ((InputBufferLength == 0) || (InputBuffer == NULL) || (Timeout > MAX_TRANSCEIVE_TIMEOUT)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->SeTransceiveInfo.sSendData.buffer = InputBuffer;
    RFInterface->SeTransceiveInfo.sSendData.length = (DWORD)InputBufferLength;
    RFInterface->SeTransceiveInfo.sRecvData.buffer = OutputBuffer;
    RFInterface->SeTransceiveInfo.sRecvData.length = (DWORD)OutputBufferLength;

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_EMEBEDDED_SE_TRANSCEIVE, NULL, NULL);
    *pOutputBufferUsed = (size_t)RFInterface->SeTransceiveInfo.sRecvData.length;
    _Analysis_assume_(OutputBufferLength >= *pOutputBufferUsed);

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceESEGetATRString(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_writes_bytes_to_(OutputBufferLength, *pOutputBufferUsed) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* pOutputBufferUsed
    )
/*++

Routine Description:

    This routine is called from the EmbeddedSE module to get the SE's ATR (answer to reset).

Arguments:

    RFInterface - The RF Interface
    OutputBuffer - Pointer to the buffer to be filled by the transmit response
    OutputBufferLength - Length of the output buffer
    pOutputBufferUsed - Pointer to the length of the output buffer used

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // Validating the buffer sizes to safeguard the DWORD cast.
    // 
    if (DWORD_MAX < OutputBufferLength) {
        NT_ASSERTMSG("Buffer size is validated at dispatch time, this should never be larger than DWORD_MAX", FALSE);
        status = STATUS_INVALID_BUFFER_SIZE;
        goto Done;
    }

    if (OutputBuffer == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    WdfWaitLockAcquire(RFInterface->DeviceLock, NULL);

    RFInterface->SeATRInfo.pBuff = OutputBuffer;
    RFInterface->SeATRInfo.dwLength = (DWORD)OutputBufferLength;

    status = NfcCxRFInterfaceExecute(RFInterface, LIBNFC_EMBEDDED_SE_GET_ATR_STRING, NULL, NULL);

    *pOutputBufferUsed = RFInterface->SeATRInfo.dwLength;

    WdfWaitLockRelease(RFInterface->DeviceLock);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

VOID
NfcCxRFInterfaceTagConnectionEstablished(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ DWORD ArrivalBitMask
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (RFInterface->pLibNfcContext->pRemDevList != NULL) {
        RFInterface->pLibNfcContext->bIsTagConnected = TRUE;
        NfcCxNfpInterfaceHandleTagConnectionEstablished(NfcCxRFInterfaceGetNfpInterface(RFInterface), ArrivalBitMask);
        NfcCxSCInterfaceHandleSmartCardConnectionEstablished(NfcCxRFInterfaceGetScInterface(RFInterface),
                                                             RFInterface);
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxRFInterfaceTagConnectionLost(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Reset all the parameters for info for tags
    RFInterface->pLibNfcContext->bIsTagPresent = FALSE;
    SetEvent(RFInterface->hStartPresenceCheck);

    if (RFInterface->pLibNfcContext->bIsTagConnected) {
        NfcCxNfpInterfaceHandleTagConnectionLost(NfcCxRFInterfaceGetNfpInterface(RFInterface));
        NfcCxSCInterfaceHandleSmartCardConnectionLost(NfcCxRFInterfaceGetScInterface(RFInterface));
        RFInterface->pLibNfcContext->bIsTagConnected = FALSE;
    }

    RFInterface->pLibNfcContext->bIsTagWriteAttempted = FALSE;
    RFInterface->pLibNfcContext->bIsTagReadOnlyAttempted = FALSE;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxRFInterfaceGetDiscoveryConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_ phLibNfc_sADD_Cfg_t *pDiscoveryConfig
    )
/*++

Routine Description:

    This routine gets the current discovery loop configuration based on the following:
        - Enable Reader Writer + P2P modes when proximity is used
        - Enable CE mode when card emulation mode is used

Arguments:

    RFInterface - A pointer to a memory location to receive the allocated RF interface
    pDiscoveryConfig - A point to the output discovery configuration

Return Value:

    VOID

--*/
{
    RtlZeroMemory(pDiscoveryConfig, sizeof(phLibNfc_sADD_Cfg_t));

    pDiscoveryConfig->Duration = RFInterface->uiDuration;
    pDiscoveryConfig->PollDevInfo.PollCfgInfo.DisableCardEmulation = 0x1;
    pDiscoveryConfig->NfcIP_Tgt_Disable = 0x1;

    if (RFInterface->uiBailout != NFC_CX_POLL_BAILOUT_DEFAULT) {
        pDiscoveryConfig->aPollParms[0].Bailout = (RFInterface->uiBailout & NFC_CX_POLL_BAILOUT_NFC_A) != 0;
        pDiscoveryConfig->aPollParms[1].Bailout = (RFInterface->uiBailout & NFC_CX_POLL_BAILOUT_NFC_B) != 0;
    }
    else {
        pDiscoveryConfig->aPollParms[0].Bailout = (uint8_t)-1;
        pDiscoveryConfig->aPollParms[1].Bailout = (uint8_t)-1;
    }

    pDiscoveryConfig->FelicaPollCfg.ReqCode = pDiscoveryConfig->FelicaPollCfg.TimeSlotNum = 0;
    memset(pDiscoveryConfig->FelicaPollCfg.SystemCode, 0xFF, sizeof(pDiscoveryConfig->FelicaPollCfg.SystemCode));

    if (RFInterface->RFPowerState & NfcCxPowerRfState_ProximityEnabled)
    {
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableIso14443A = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_A) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableIso14443B = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_B) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableFelica212 = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_F_212) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableFelica424 = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_F_424) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableIso15693 = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_15693) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableNfcActive = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_ACTIVE) != 0;
        pDiscoveryConfig->PollDevInfo.PollCfgInfo.EnableKovio = (RFInterface->uiPollDevInfo & NFC_CX_POLL_NFC_A_KOVIO) != 0;
        pDiscoveryConfig->NfcIP_Mode = RFInterface->uiNfcIP_Mode;
    }

    if (!(RFInterface->RFPowerState & NfcCxPowerRfState_NoListenEnabled))
    {
        if (RFInterface->RFPowerState & NfcCxPowerRfState_StealthListenEnabled)
        {
            // Restrict listening modes to active RF technologies.
            // This will allow us to detect the presence of NFC terminals, which don't support peer-to-peer, without establishing a connection.
            // Ideally we wouldn't allow peer-to-peer connections either. But no such RF mode is avaliable.
            pDiscoveryConfig->NfcIP_Tgt_Mode_Config = RFInterface->uiNfcIP_Tgt_Mode & (phNfc_eP2PTargetNfcActiveATech | phNfc_eP2PTargetNfcActiveFTech);
            pDiscoveryConfig->NfcIP_Tgt_Disable = 0x0;
        }
        else
        {
            if (RFInterface->RFPowerState & NfcCxPowerRfState_ProximityEnabled)
            {
                // Enable all supported listening technologies, both passive and active.
                pDiscoveryConfig->NfcIP_Tgt_Mode_Config = RFInterface->uiNfcIP_Tgt_Mode;
                pDiscoveryConfig->NfcIP_Tgt_Disable = 0x0;
            }

            if (RFInterface->RFPowerState & NfcCxPowerRfState_CardEmulationEnabled)
            {
                // Enable all supported passive listening technologies.
                pDiscoveryConfig->CE_Mode_Config = RFInterface->uiNfcCE_Mode;
                pDiscoveryConfig->PollDevInfo.PollCfgInfo.DisableCardEmulation = 0x0;
            }
        }
    }
}

BOOLEAN
NfcCxRFInterfaceCheckIfWriteTagPublicationsExist(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    return NfcCxNfpInterfaceCheckIfWriteTagPublicationsExist(NfcCxRFInterfaceGetNfpInterface(RFInterface));
}

VOID
NfcCxRFInterfaceP2pConnectionEstablished(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    NfcCxNfpInterfaceHandleP2pConnectionEstablished(NfcCxRFInterfaceGetNfpInterface(RFInterface));
    RFInterface->pLibNfcContext->bIsP2PConnected = TRUE;
}

VOID
NfcCxRFInterfaceP2pConnectionLost(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    if (RFInterface->pLibNfcContext->bIsP2PConnected) {
        NfcCxNfpInterfaceHandleP2pConnectionLost(NfcCxRFInterfaceGetNfpInterface(RFInterface));
        RFInterface->pLibNfcContext->bIsP2PConnected = FALSE;
    }
}

VOID
NfcCxRFInterfaceHandleReceivedNdefMessage(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ RECEIVED_NDEF_MESSAGE_SOURCE /*eSource*/,
    _In_ uint16_t cbRawNdef,
    _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
    )
{
    CNFCProximityBuffer* proxBuffer = new CNFCProximityBuffer();

    if (proxBuffer != NULL)
    {
        HRESULT hr = proxBuffer->InitializeRaw(cbRawNdef, pbRawNdef);

        if (SUCCEEDED(hr)) {
            //
            // Forward the ndef message to the NFP interface
            //
            NfcCxNfpInterfaceHandleReceivedNdefMessage(NfcCxRFInterfaceGetNfpInterface(RFInterface),
                                                        proxBuffer);

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxProximityMessageReceived",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(proxBuffer->GetTnf(), "Tnf"),
                TraceLoggingCharArray((LPCSTR)proxBuffer->GetSubTypeExt(), proxBuffer->GetSubTypeExtSize(), "MessageType"),
                TraceLoggingValue(proxBuffer->GetSubTypeExtSize(), "TypeLength"),
                TraceLoggingValue(proxBuffer->GetPayloadSize(), "PayloadSize"));
        }

        delete proxBuffer;
        proxBuffer = NULL;
    }
}

VOID
NfcCxRFInterfaceHandleReceivedBarcodeMessage(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ uint16_t barcodeLength,
    _In_bytecount_(barcodeLength) PBYTE barcodeBuffer
    )
{
    CNFCProximityBuffer* proxBuffer = new CNFCProximityBuffer();

    if (proxBuffer != NULL)
    {
        HRESULT hr = proxBuffer->InitializeBarcode(barcodeLength, barcodeBuffer);

        if (SUCCEEDED(hr)) {
            //
            // Forward the ndef message to the NFP interface
            //
            NfcCxNfpInterfaceHandleReceivedNdefMessage(NfcCxRFInterfaceGetNfpInterface(RFInterface),
                proxBuffer);

            TraceLoggingWrite(
                g_hNfcCxProvider,
                "NfcCxRFInterfaceHandleReceivedBarcodeMessage",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(proxBuffer->GetPayloadSize(), "PayloadSize"));
        }

        delete proxBuffer;
        proxBuffer = NULL;
    }
}

VOID
NfcCxRFInterfaceHandleSecureElementEvent(
    _In_ PVOID pContext,
    _In_ phLibNfc_eSE_EvtType_t EventType,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo,
    _In_ NFCSTATUS Status
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;
    SECURE_ELEMENT_EVENT_TYPE SEEventType;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "SE event received hSecureElement=%p, EventType=%!phLibNfc_eSE_EvtType_t!",
                            hSecureElement,
                            EventType);

    if (Status != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_WARNING, "Failed with status %!NFCSTATUS!", Status);
        goto Done;
    }

    status = NfcCxRFInterfaceGetSEEvent(EventType, &SEEventType);
    if (!NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_WARNING, "Unsupported EventType=%!phLibNfc_eSE_EvtType_t!", EventType);
        goto Done;
    }

    if ((ExternalReaderDeparture == SEEventType) && (rfInterface->pLibNfcContext->bIsHCEConnected)) {
        NfcCxPostLibNfcThreadMessage(rfInterface, LIBNFC_STATE_HANDLER, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    for (uint8_t i = 0; (i < rfInterface->pLibNfcContext->SECount); i++) {
        if (((SEEventType == ExternalReaderArrival) || (SEEventType == ExternalReaderDeparture) ||
            (rfInterface->pLibNfcContext->SEList[i].hSecureElement == hSecureElement)) &&
            (rfInterface->pLibNfcContext->SEList[i].eSE_ActivationMode == phLibNfc_SE_ActModeOn)) {
            NfcCxSEInterfaceHandleEvent(NfcCxRFInterfaceGetSEInterface(rfInterface),
                                        SEEventType,
                                        &rfInterface->pLibNfcContext->SEList[i],
                                        pSeEvtInfo);
        }
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static void
NfcCxRFInterfaceRemoteDevReceiveCB(
    void*               pContext,
    phNfc_sData_t*      pRecvBufferInfo,
    NFCSTATUS           Status
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS != Status) {
        TRACE_LINE(LEVEL_WARNING, "Failed with status %!NFCSTATUS!", Status);
        goto Done;
    }

    if (0 == pRecvBufferInfo->length) {
        TRACE_LINE(LEVEL_WARNING, "Invalid APDU of size zero!");
        goto Done;
    }

    if ((phLibNfc_SE_Type_DeviceHost != rfInterface->pLibNfcContext->SEList[0].eSE_Type) ||
        (phLibNfc_SE_ActModeOn != rfInterface->pLibNfcContext->SEList[0].eSE_ActivationMode)) {
        TRACE_LINE(LEVEL_WARNING, "Received unexpected APDU, SE type"
            " %!phLibNfc_SE_Type_t!, SE Activation mode %!phLibNfc_eSE_ActivationMode!",
            rfInterface->pLibNfcContext->SEList[0].eSE_Type,
            rfInterface->pLibNfcContext->SEList[0].eSE_ActivationMode);
        goto Done;
    }

    // Send the data buffer to the upper layer
    rfInterface->pLibNfcContext->bIsHCERecv = FALSE;
    NfcCxSEInterfaceHandleHCEPacket(NfcCxRFInterfaceGetSEInterface(rfInterface),
                                    rfInterface->pLibNfcContext->uHCEConnectionId,
                                    pRecvBufferInfo->buffer,
                                    (USHORT) pRecvBufferInfo->length);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxRFInterfaceHandleSEDeviceHostEvent(
    _In_ PVOID pContext,
    _In_ phLibNfc_RemoteDevList_t* pRemoteDevList,
    _In_ uint8_t uiNoOfRemoteDev,
    _In_ NFCSTATUS Status
)
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // We receive this event for the first data packet
    // Validate the status and send it to upper layer
    //
    if (Status != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_WARNING, "Remote device error status %!NFCSTATUS!", Status);
        goto Done;
    }

    if (1 != uiNoOfRemoteDev) {
        TRACE_LINE(LEVEL_WARNING, "Invalid RemoteDev %d", uiNoOfRemoteDev);
        goto Done;
    }

    if (phLibNfc_SE_ActModeOn != rfInterface->pLibNfcContext->SEList->eSE_ActivationMode) {
        TRACE_LINE(LEVEL_INFO, "Device host SE is disabled");
        goto Done;
    }

    //
    // Update connection count and trigger HCE activated Event
    //
    if (!rfInterface->pLibNfcContext->bIsHCEActivated) {
        rfInterface->pLibNfcContext->uHCEConnectionId++;
        rfInterface->pLibNfcContext->bIsHCEActivated = TRUE;

        NfcCxSEInterfaceHandleEvent(NfcCxRFInterfaceGetSEInterface(rfInterface),
                                    HceActivated,
                                    rfInterface->pLibNfcContext->SEList,
                                    NULL);

        NfcCxPostLibNfcThreadMessage(rfInterface, LIBNFC_STATE_HANDLER, NfcCxEventActivated, NULL, NULL, NULL);
    }

    rfInterface->pLibNfcContext->bIsHCERecv = TRUE;
    Status = phLibNfc_RemoteDev_Receive(pRemoteDevList[0].hTargetDev,
                                        NfcCxRFInterfaceRemoteDevReceiveCB,
                                        rfInterface);
    if (NFCSTATUS_PENDING != Status) {
        TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive retruns %!NFCSTATUS!", Status);
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID
NfcCxRFInterfaceSESetDefaultRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface
    )
{
    PNFCCX_LIBNFC_CONTEXT pLibNfcContext = RFInterface->pLibNfcContext;
    phLibNfc_RtngConfig_t *pRtngTable = pLibNfcContext->RtngTable;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    pLibNfcContext->RtngTableCount = 0;
    RtlZeroMemory(pLibNfcContext->RtngTable, sizeof(pLibNfcContext->RtngTable));

    for (uint8_t i = 0; i < pLibNfcContext->SECount; i++) {
        if (pLibNfcContext->SEList[i].eSE_Type != phLibNfc_SE_Type_UICC) {
            continue;
        }

        if (pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.TechnBasedRouting == 0 &&
            pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.ProtocolBasedRouting == 0) {
            NT_ASSERTMSG("The NFCC should support either Technology or Protocol routing to initialize", FALSE);
        }

        if (pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.TechnBasedRouting == 1) {
            pRtngTable[pLibNfcContext->RtngTableCount].hSecureElement = pLibNfcContext->SEList[i].hSecureElement;
            pRtngTable[pLibNfcContext->RtngTableCount].Type = phNfc_LstnModeRtngTechBased;
            pRtngTable[pLibNfcContext->RtngTableCount].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
            pRtngTable[pLibNfcContext->RtngTableCount++].LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = phNfc_RfTechnologiesNfc_A;

            pRtngTable[pLibNfcContext->RtngTableCount].hSecureElement = pLibNfcContext->SEList[i].hSecureElement;
            pRtngTable[pLibNfcContext->RtngTableCount].Type = phNfc_LstnModeRtngTechBased;
            pRtngTable[pLibNfcContext->RtngTableCount].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
            pRtngTable[pLibNfcContext->RtngTableCount++].LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = phNfc_RfTechnologiesNfc_B;

            pRtngTable[pLibNfcContext->RtngTableCount].hSecureElement = pLibNfcContext->SEList[i].hSecureElement;
            pRtngTable[pLibNfcContext->RtngTableCount].Type = phNfc_LstnModeRtngTechBased;
            pRtngTable[pLibNfcContext->RtngTableCount].LstnModeRtngValue.tTechBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
            pRtngTable[pLibNfcContext->RtngTableCount++].LstnModeRtngValue.tTechBasedRtngValue.tRfTechnology = phNfc_RfTechnologiesNfc_F;
        }

        if (pLibNfcContext->sStackCapabilities.psDevCapabilities.RoutingInfo.ProtocolBasedRouting == 1) {
            pRtngTable[pLibNfcContext->RtngTableCount].hSecureElement = pLibNfcContext->SEList[i].hSecureElement;
            pRtngTable[pLibNfcContext->RtngTableCount].Type = phNfc_LstnModeRtngProtocolBased;
            pRtngTable[pLibNfcContext->RtngTableCount].LstnModeRtngValue.tProtoBasedRtngValue.tPowerState.bSwitchedOn = 0x01;
            pRtngTable[pLibNfcContext->RtngTableCount++].LstnModeRtngValue.tProtoBasedRtngValue.tRfProtocol = phNfc_RfProtocolsIsoDepProtocol;
        }

        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static VOID
NfcCxRFInterfaceInitializeCB(
    _In_ VOID* pContext,
    _In_ uint8_t ConfigStatus,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->pLibNfcContext->ConfigStatus = ConfigStatus;

    if (NfcStatus == NFCSTATUS_SUCCESS) {
        if (!IsEqualGUID(SESSION_ID, rfInterface->SessionId)) {
            RtlCopyMemory(&rfInterface->SessionId, &SESSION_ID, sizeof(rfInterface->SessionId));
            NfcCxFdoWritePersistedDeviceRegistrySettings(rfInterface->FdoContext);
        }

        NfcStatus = phLibNfc_Mgt_GetstackCapabilities(&rfInterface->pLibNfcContext->sStackCapabilities, rfInterface->pLibNfcContext);
    }

    status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceInitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_Mgt_Initialize(RFInterface->FdoContext,
                                        RFInterface->pLibNfcContext->eInitType,
                                        &RFInterface->LibConfig,
                                        NfcCxRFInterfaceInitializeCB,
                                        &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceDeinitializeCB(
    _In_ VOID* pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->bIsDiscoveryStarted = FALSE;
    rfInterface->pLibNfcContext->bIsTagPresent = FALSE;

    rfInterface->pLibNfcContext->SECount = 0;
    RtlZeroMemory(rfInterface->pLibNfcContext->SEList, sizeof(rfInterface->pLibNfcContext->SEList));

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, STATUS_SUCCESS, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceDeinitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    // Warning C4302: 'type cast': truncation from 'void *' to 'BOOLEAN'
#pragma warning(suppress:4302)
    BOOLEAN fSkipResetDuringShutdown = (BOOLEAN)Param1;

    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (fSkipResetDuringShutdown) {
        phLibNfc_Mgt_Reset(RFInterface->FdoContext);
        NfcCxRFInterfaceDeinitializeCB(&LibNfcContext, nfcStatus);
    }
    else {
        nfcStatus = phLibNfc_Mgt_DeInitialize(RFInterface->FdoContext,
                                              NfcCxRFInterfaceDeinitializeCB,
                                              &LibNfcContext);
        if (nfcStatus != NFCSTATUS_PENDING) {
            phLibNfc_Mgt_Reset(RFInterface->FdoContext);
            NfcCxRFInterfaceDeinitializeCB(&LibNfcContext, nfcStatus);
        }
    }

    nfcStatus = NFCSTATUS_PENDING;
    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRemoteDevConnectCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_Handle hRemoteDev,
    _In_ phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(status)) {

        phLibNfc_RemoteDevList_t& selectedDevListItem = rfInterface->pLibNfcContext->pRemDevList[rfInterface->pLibNfcContext->SelectedProtocolIndex];

        // We may receive additional information about the remote device during the connection sequence.
        // So make sure our copy of the device information is up to date.
        selectedDevListItem.hTargetDev = hRemoteDev;
        RtlCopyMemory(selectedDevListItem.psRemoteDevInfo,
                        psRemoteDevInfo,
                        sizeof(phLibNfc_sRemoteDevInformation_t));

        if (NfcCxRFInterfaceGetGenericRemoteDevType(psRemoteDevInfo->RemDevType) == phNfc_ePICC_DevType) {
            rfInterface->pLibNfcContext->bIsTagPresent = TRUE;
        }
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceRemoteDevConnect(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_RemoteDev_Connect(RFInterface->pLibNfcContext->pRemDevList[RFInterface->pLibNfcContext->SelectedProtocolIndex].hTargetDev,
                                           NfcCxRFInterfaceRemoteDevConnectCB,
                                           &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    if (NT_SUCCESS(Status))
    {
        TRACE_LINE(LEVEL_INFO, "Set Target Activation To %x", RFInterface->pLibNfcContext->SelectedProtocolIndex);
    }
    else
    {
        TRACE_LINE(LEVEL_INFO, "Set Target Activation Failed for Index %x", RFInterface->pLibNfcContext->SelectedProtocolIndex);
    }


    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRemoteDevDisconnectCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_Handle hRemoteDev,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);

    rfInterface->bIsDiscoveryStarted = rfInterface->pLibNfcContext->eReleaseType != NFC_DISCOVERY_STOP;
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceRemoteDevDisconnect(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_RemoteDev_Disconnect(RFInterface->pLibNfcContext->pRemDevList[RFInterface->pLibNfcContext->SelectedProtocolIndex].hTargetDev,
                                              RFInterface->pLibNfcContext->eReleaseType,
                                              NfcCxRFInterfaceRemoteDevDisconnectCB,
                                              &LibNfcContext);

    if (nfcStatus == NFCSTATUS_TARGET_NOT_CONNECTED) {
        nfcStatus = NFCSTATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        TRACE_LINE(LEVEL_INFO, "Set Target DeActivation To %x", RFInterface->pLibNfcContext->SelectedProtocolIndex);
    }
    else
    {
        TRACE_LINE(LEVEL_INFO, "Set Target DeActivation Failed for Index %x", RFInterface->pLibNfcContext->SelectedProtocolIndex);
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRfDiscoveryConfigCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->bIsDiscoveryStarted = NT_SUCCESS(status);
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceRfDiscoveryConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    NfcCxRFInterfaceGetDiscoveryConfig(RFInterface, &RFInterface->DiscoveryConfig);

    nfcStatus = phLibNfc_Mgt_ConfigureDiscovery(NFC_DISCOVERY_CONFIG,
                                                RFInterface->DiscoveryConfig,
                                                NfcCxRFInterfaceRfDiscoveryConfigCB,
                                                &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRfDiscoveryStopCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->bIsDiscoveryStarted = FALSE;
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceRfDiscoveryStop(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    RtlZeroMemory(&RFInterface->DiscoveryConfig, sizeof(RFInterface->DiscoveryConfig));

    RFInterface->DiscoveryConfig.PollDevInfo.PollCfgInfo.DisableCardEmulation = 0x1;
    RFInterface->DiscoveryConfig.NfcIP_Tgt_Disable = 0x1;

    nfcStatus = phLibNfc_Mgt_ConfigureDiscovery(NFC_DISCOVERY_CONFIG,
                                                RFInterface->DiscoveryConfig,
                                                NfcCxRFInterfaceRfDiscoveryStopCB,
                                                &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTagCheckNdefCB(
    _In_ VOID *                  pContext,
    _In_ phLibNfc_ChkNdef_Info_t NdefInfo,
    _In_ NFCSTATUS               NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfcStatus != NFCSTATUS_TARGET_LOST && PHNFCSTATUS(NfcStatus) != NFCSTATUS_RELEASED) {
        NfcCxRFInterfaceTagConnectionEstablished(rfInterface,
                                                 NdefInfo.NdefCardState == PHLIBNFC_NDEF_CARD_READ_ONLY ?
                                                 DEVICE_ARRIVAL_BIT_READONLY : 0);
    }

    if (NfcStatus == NFCSTATUS_SUCCESS && NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_INVALID) {
        rfInterface->uiActualNdefMsgLength = NdefInfo.ActualNdefMsgLength;
        rfInterface->uiMaxNdefMsgLength = NdefInfo.MaxNdefMsgLength;
        rfInterface->pLibNfcContext->bIsTagNdefFormatted = TRUE;

        TRACE_LINE(LEVEL_INFO, "NdefCardState=%d ActualNdefMsgLength=%d MaxNdefMsgLength=%d",
                                NdefInfo.NdefCardState, NdefInfo.ActualNdefMsgLength, NdefInfo.MaxNdefMsgLength);

        if (NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_READ_ONLY) {
            NfcCxNfpInterfaceHandleWriteableTagEvent(NfcCxRFInterfaceGetNfpInterface(rfInterface), NdefInfo.MaxNdefMsgLength);
            NfcCxNfpInterfaceVerifyAndSendPublication(NfcCxRFInterfaceGetNfpInterface(rfInterface));
        }

        if (NfcCxRFInterfaceCheckIfWriteTagPublicationsExist(rfInterface) == TRUE &&
            NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_READ_ONLY) {
            NfcCxSkipSequence(rfInterface, rfInterface->pSeqHandler, 1); // Skip Read NDEF sequence
        }
    }
    else if (rfInterface->pLibNfcContext->bIsTagConnected) {
        rfInterface->pLibNfcContext->bIsTagNdefFormatted = FALSE;
        if (NfcCxRFInterfaceCheckIfWriteTagPublicationsExist(rfInterface) == TRUE) {
            NfcCxNfpInterfaceVerifyAndSendPublication(NfcCxRFInterfaceGetNfpInterface(rfInterface));
        }
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

static VOID
NfcCxRFInterfaceTagFormatCheckNdefCB(
    _In_ VOID *                  pContext,
    _In_ phLibNfc_ChkNdef_Info_t NdefInfo,
    _In_ NFCSTATUS               NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NfcStatus == NFCSTATUS_SUCCESS && NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_INVALID) {
        rfInterface->uiActualNdefMsgLength = NdefInfo.ActualNdefMsgLength;
        rfInterface->uiMaxNdefMsgLength = NdefInfo.MaxNdefMsgLength;
        rfInterface->pLibNfcContext->bIsTagNdefFormatted = TRUE;

        TRACE_LINE(LEVEL_INFO, "NdefCardState=%d ActualNdefMsgLength=%d MaxNdefMsgLength=%d",
                                NdefInfo.NdefCardState, NdefInfo.ActualNdefMsgLength, NdefInfo.MaxNdefMsgLength);

        if (NdefInfo.NdefCardState != PHLIBNFC_NDEF_CARD_READ_ONLY) {
            NfcCxNfpInterfaceHandleWriteableTagEvent(NfcCxRFInterfaceGetNfpInterface(rfInterface), NdefInfo.MaxNdefMsgLength);
        }
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTagCheckNdef(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    // Warning C4302: 'type cast': truncation from 'void *' to 'BOOLEAN'
#pragma warning(suppress:4302)
    BOOLEAN formatSequence = (BOOLEAN)Param1;

    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_Ndef_CheckNdef(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                        formatSequence ? NfcCxRFInterfaceTagFormatCheckNdefCB : NfcCxRFInterfaceTagCheckNdefCB,
                                        &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceNdefTagWriteCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

#ifdef EVENT_WRITE
    EventWriteRfNdefTagWriteStop(status);
#endif

    rfInterface->pLibNfcContext->bIsTagWriteAttempted = TRUE;
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTagWriteNdef(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (RFInterface->pLibNfcContext->bIsTagWriteAttempted == TRUE) {
        nfcStatus = NFCSTATUS_ABORTED;
        TRACE_LINE(LEVEL_ERROR, "Tag Ndef Write already attempted");
    }
    else {
#ifdef EVENT_WRITE
        EventWriteRfNdefTagWriteStart(RFInterface->sSendBuffer.length);
#endif

        nfcStatus = phLibNfc_Ndef_Write(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                        &RFInterface->sSendBuffer,
                                        NfcCxRFInterfaceNdefTagWriteCB,
                                        &LibNfcContext);
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTagReadNdefCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

#ifdef EVENT_WRITE
    EventWriteRfNdefTagReadStop(status, (USHORT)rfInterface->sNdefMsg.length);
#endif

    if (NT_SUCCESS(status)) {
        NfcCxRFInterfaceHandleReceivedNdefMessage(rfInterface,
                                                  ReceivedNdefFromTag,
                                                  (USHORT)rfInterface->sNdefMsg.length,
                                                  rfInterface->sNdefMsg.buffer);
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    rfInterface->uiActualNdefMsgLength = 0;
    rfInterface->uiMaxNdefMsgLength = 0;

    free(rfInterface->sNdefMsg.buffer);
    rfInterface->sNdefMsg.buffer = NULL;

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTagReadBarcode(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (RFInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phNfc_eKovio_PICC)
    {
        // Skip if already handled (same tag read too quickly)
        if ( (GetTickCount64() - RFInterface->bKovioDetected) > PRESENCE_CHECK_INTERVAL) {
            RFInterface->sBarcodeMsg.length = RFInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemoteDevInfo.Kovio_Info.TagIdLength;
            RFInterface->sBarcodeMsg.buffer = (uint8_t*)malloc(RFInterface->sBarcodeMsg.length);
            if (RFInterface->sBarcodeMsg.buffer)
            {
                RtlCopyMemory(RFInterface->sBarcodeMsg.buffer, RFInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemoteDevInfo.Kovio_Info.TagId, RFInterface->sBarcodeMsg.length);
                NfcCxRFInterfaceHandleReceivedBarcodeMessage(RFInterface, (USHORT)RFInterface->sBarcodeMsg.length, RFInterface->sBarcodeMsg.buffer);
                free(RFInterface->sBarcodeMsg.buffer);
                RFInterface->sBarcodeMsg.buffer = NULL;
                RFInterface->sBarcodeMsg.length = 0;
            }
            else
            {
                RFInterface->sBarcodeMsg.length = 0;
                nfcStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
                TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory for barcode read");
            }
        }
    }
    else
    {
        nfcStatus = NFCSTATUS_INVALID_REMOTE_DEVICE;
        TRACE_LINE(LEVEL_ERROR, "Barcode type not supported for requested device.");
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceTagReadNdef(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    RFInterface->sNdefMsg.buffer = (uint8_t*)malloc(RFInterface->uiActualNdefMsgLength);
    if (RFInterface->sNdefMsg.buffer) {
        RFInterface->sNdefMsg.length = RFInterface->uiActualNdefMsgLength;

#ifdef EVENT_WRITE
        EventWriteRfNdefTagReadStart();
#endif

        nfcStatus = phLibNfc_Ndef_Read(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                       &RFInterface->sNdefMsg,
                                       phLibNfc_Ndef_EBegin,
                                       NfcCxRFInterfaceTagReadNdefCB,
                                       &LibNfcContext);
    }
    else {
        nfcStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory for Ndef read");
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTagConvertReadOnlyCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->pLibNfcContext->bIsTagReadOnlyAttempted = TRUE;
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTagConvertReadOnly(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;
    static uint8_t defaultRawKey[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    phNfc_sData_t defaultKey = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    defaultKey.buffer = defaultRawKey;
    defaultKey.length = sizeof(defaultRawKey);

    if (RFInterface->pLibNfcContext->bIsTagReadOnlyAttempted == TRUE) {
        nfcStatus = NFCSTATUS_ABORTED;
        TRACE_LINE(LEVEL_ERROR, "Tag Convert ReadOnly already attempted");
    }
    else {
        nfcStatus = phLibNfc_ConvertToReadOnlyNdef(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                                   &defaultKey,
                                                   NfcCxRFInterfaceTagConvertReadOnlyCB,
                                                   &LibNfcContext);
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTagFormatNdefCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, (VOID*)TRUE, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTagFormatNdef(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;
    static uint8_t defaultRawKey[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    phNfc_sData_t defaultKey = {0};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    defaultKey.buffer = defaultRawKey;
    defaultKey.length = sizeof(defaultRawKey);

    if (RFInterface->pLibNfcContext->bIsTagNdefFormatted == TRUE) {
        NfcCxSkipSequence(RFInterface, RFInterface->pSeqHandler, 1); // Skip Check NDEF sequence
    }
    else {
        nfcStatus = phLibNfc_RemoteDev_FormatNdef(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                                  &defaultKey,
                                                  NfcCxRFInterfaceTagFormatNdefCB,
                                                  &LibNfcContext);
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTargetTransceiveCB(
    _In_ VOID *             pContext,
    _In_ phLibNfc_Handle    hRemoteDev,
    _In_ phNfc_sData_t*     pResBuffer,
    _In_ NFCSTATUS          NfcStatus
    )
{
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);

    // Note: The handling of 'buffer too small' error case changes depending on which card is communicated with.
    // For some cards, the error code NFCSTATUS_MORE_INFORMATION is returned. Other cards return NFCSTATUS_SUCCESS
    // and set 'pResBuffer->length' to the required buffer size. :-(
    if (NfcStatus == NFCSTATUS_SUCCESS) {
        if (pResBuffer->length > rfInterface->sTransceiveBuffer.sRecvData.length) {
            NfcStatus = NFCSTATUS_BUFFER_TOO_SMALL;
            TRACE_LINE(LEVEL_ERROR, "Receive buffer too small for response");
        }
        // Note: In most cases, we will receive the data in the buffer we provided.
        else if (rfInterface->sTransceiveBuffer.sRecvData.buffer != pResBuffer->buffer) {
            RtlCopyMemory(rfInterface->sTransceiveBuffer.sRecvData.buffer, 
                          pResBuffer->buffer, 
                          pResBuffer->length);
        }

        rfInterface->sTransceiveBuffer.sRecvData.length = pResBuffer->length;
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, NfcCxNtStatusFromNfcStatus(NfcStatus), NULL, NULL);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxRFInterfaceTargetTransceive(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    phLibNfc_RemoteDevList_t* pRemDevList = RFInterface->pLibNfcContext->pRemDevList;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;
    DWORD SelectedProtocol = RFInterface->pLibNfcContext->SelectedProtocolIndex;

    TRACE_LINE(LEVEL_INFO, "SelectedProtocol  is = %x", SelectedProtocol);

    switch (pRemDevList[SelectedProtocol].psRemoteDevInfo->RemDevType)
    {
    case phLibNfc_eISO14443_4A_PICC:
    case phLibNfc_eISO14443_4B_PICC:
        RFInterface->sTransceiveBuffer.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
        break;

    case phLibNfc_eJewel_PICC:
        RFInterface->sTransceiveBuffer.cmd.JewelCmd = phNfc_eJewel_Raw;
        break;

    case phLibNfc_eFelica_PICC:
        RFInterface->sTransceiveBuffer.cmd.FelCmd = phNfc_eFelica_Raw;
        break;

    case phLibNfc_eISO15693_PICC:
        RFInterface->sTransceiveBuffer.cmd.Iso15693Cmd = phNfc_eIso15693_Cmd;
        break;

    case phLibNfc_eMifare_PICC:
        {
            if (pRemDevList->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak == 0x00) {
                RFInterface->sTransceiveBuffer.cmd.MfCmd = phNfc_eMifareRaw;
            }
            else
            {
                static const UINT32 MIN_MIFARE_TRANSCEIVE_SIZE = 2; // 1 byte MfCmd + 1 byte Addr

                if (RFInterface->sTransceiveBuffer.sSendData.length < MIN_MIFARE_TRANSCEIVE_SIZE) {
                    nfcStatus = NFCSTATUS_INVALID_PARAMETER;
                    TRACE_LINE(LEVEL_ERROR, "Payload for Mifare tag is too small");
                    goto Done;
                }

                RFInterface->sTransceiveBuffer.cmd.MfCmd = (phNfc_eMifareCmdList_t)RFInterface->sTransceiveBuffer.sSendData.buffer[0];
                RFInterface->sTransceiveBuffer.addr = RFInterface->sTransceiveBuffer.sSendData.buffer[1];
                RFInterface->sTransceiveBuffer.sSendData.length -= MIN_MIFARE_TRANSCEIVE_SIZE;

                if (RFInterface->sTransceiveBuffer.sSendData.length > 0) {
                    RtlMoveMemory(&RFInterface->sTransceiveBuffer.sSendData.buffer[0],
                                  &RFInterface->sTransceiveBuffer.sSendData.buffer[MIN_MIFARE_TRANSCEIVE_SIZE],
                                  RFInterface->sTransceiveBuffer.sSendData.length);
                }
            }
        }
        break;

        default:
            nfcStatus = NFCSTATUS_NOT_ALLOWED;
            TRACE_LINE(LEVEL_ERROR, "Target type not supported for the transceive command");
            goto Done;
    }

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_RemoteDev_Transceive(RFInterface->pLibNfcContext->pRemDevList[SelectedProtocol].hTargetDev,
                                              &RFInterface->sTransceiveBuffer,
                                              NfcCxRFInterfaceTargetTransceiveCB,
                                              &LibNfcContext);

Done:
    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceTargetPresenceCheckCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, (VOID*)TRUE, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceTargetPresenceCheck(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_RemoteDev_CheckPresence(RFInterface->pLibNfcContext->pRemDevList[RFInterface->pLibNfcContext->SelectedProtocolIndex].hTargetDev,
                                                 NfcCxRFInterfaceTargetPresenceCheckCB,
                                                 &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceESETransceiveSeqCB(
    _In_ VOID* pContext,
    _In_ phLibNfc_Handle hRemoteDev,
    _In_ phNfc_sData_t* pResBuffer,
    _In_ NFCSTATUS NfcStatus
    )
/*++

Routine Description:

    Invoked when the eSE APDU operation has completed.

*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_LIBNFC_REQUEST_CONTEXT requestContext = (PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext;
    PNFCCX_RF_INTERFACE rfInterface = requestContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(hRemoteDev);
    UNREFERENCED_PARAMETER(pResBuffer);

    status = NfcCxNtStatusFromNfcStatus(NfcStatus);

    // Note: Unlike phLibNfc_RemoteDev_Transceive/NfcCxRFInterfaceTargetTransceiveCB, we will always receive NFCSTATUS_MORE_INFORMATION for
    // 'buffer too small' errors. In addition, 'pResBuffer->buffer' and 'rfInterface->SeTransceiveInfo.sRecvData.buffer' will always point
    // to the same memory. (So no need to copy the memory.)
    NT_ASSERT(pResBuffer->buffer == rfInterface->SeTransceiveInfo.sRecvData.buffer);
    if (NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "eSE transceive succeeded. Length=%u", rfInterface->SeTransceiveInfo.sRecvData.length);
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "eSE transceive failed. Status=%!STATUS!", status);
    }

    NfcCxInternalSequence(rfInterface, requestContext->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxRFInterfaceESETransceiveSeq(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS /*Status*/,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
/*++

Routine Description:

    Starts an eSE APDU operation. Function is triggered on the LibNfc thread in response to the
    LIBNFC_EMEBEDDED_SE_TRANSCEIVE message.

*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;
    phLibNfc_Handle hSE_handle = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    for (uint8_t i = 0; i < RFInterface->pLibNfcContext->SECount; i++) {
        if (RFInterface->pLibNfcContext->SEList[i].eSE_Type == phLibNfc_SE_Type_eSE) {
            hSE_handle = RFInterface->pLibNfcContext->SEList[i].hSecureElement;
            break;
        }
    }

    if (hSE_handle == nullptr)
    {
        TRACE_LINE(LEVEL_ERROR, "No eSE found.");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    RFInterface->SeTransceiveInfo.timeout = DEFAULT_HCI_TX_RX_TIME_OUT;

    nfcStatus = phLibNfc_eSE_Transceive(hSE_handle,
                                        &RFInterface->SeTransceiveInfo,
                                        NfcCxRFInterfaceESETransceiveSeqCB,
                                        &LibNfcContext);
    status = NfcCxNtStatusFromNfcStatus(nfcStatus);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static VOID
NfcCxRFInterfaceESEGetATRStringSeqCB(
    _In_ void* pContext,
    _In_ pphNfc_SeAtr_Info_t pResAtrInfo,
    _In_ NFCSTATUS NfcStatus
    )
/*++

Routine Description:

    Invoked when the eSE Get ATR operation has completed.

*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_LIBNFC_REQUEST_CONTEXT requestContext = (PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext;
    PNFCCX_RF_INTERFACE rfInterface = requestContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(pResAtrInfo);

    status = NfcCxNtStatusFromNfcStatus(NfcStatus);

    if (NT_SUCCESS(status)) {
        TRACE_LINE(LEVEL_INFO, "Get eSE ATR was successful. Length = %u.", rfInterface->SeATRInfo.dwLength);
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "Failed to get eSE ATR. Status = %!STATUS!.", status);
    }

    NfcCxInternalSequence(rfInterface, requestContext->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxRFInterfaceESEGetATRStringSeq(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS /*Status*/,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
/*++

Routine Description:

    Starts an eSE Get ATR operation. Function is triggered on the LibNfc thread in response to the
    LIBNFC_EMBEDDED_SE_GET_ATR_STRING message.

*/
{
    NTSTATUS status = STATUS_SUCCESS;
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;
    phLibNfc_Handle hSE_handle = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    for (uint8_t i = 0; i < RFInterface->pLibNfcContext->SECount; i++) {
        if (RFInterface->pLibNfcContext->SEList[i].eSE_Type == phLibNfc_SE_Type_eSE) {
            hSE_handle = RFInterface->pLibNfcContext->SEList[i].hSecureElement;
            break;
        }
    }

    if (hSE_handle == nullptr)
    {
        TRACE_LINE(LEVEL_ERROR, "No eSE found.");
        status = STATUS_INVALID_DEVICE_STATE;
        goto Done;
    }

    nfcStatus = phLibNfc_eSE_GetAtr(hSE_handle, &RFInterface->SeATRInfo, NfcCxRFInterfaceESEGetATRStringSeqCB, &LibNfcContext);
    status = NfcCxNtStatusFromNfcStatus(nfcStatus);

Done:
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceSENtfRegister(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS /*Status*/,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_LIBNFC_CONTEXT pLibNfcContext = RFInterface->pLibNfcContext;
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    phLibNfc_SE_NtfRegister(NfcCxRFInterfaceHandleSecureElementEvent, RFInterface);

    if (NfcCxRFInterfaceIsHCESupported(RFInterface)) {
        pLibNfcContext->SECount = 1;
        pLibNfcContext->SEList[0].eSE_Type = phLibNfc_SE_Type_DeviceHost;
        pLibNfcContext->SEList[0].hSecureElement = NULL;

        if (RFInterface->eDeviceHostInitializationState == phLibNfc_SE_ActModeOn)
        {
            NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

            pLibNfcContext->SEList[0].eSE_ActivationMode = RFInterface->eDeviceHostInitializationState;
            nfcStatus = phLibNfc_CardEmulation_NtfRegister(NfcCxRFInterfaceHandleSEDeviceHostEvent, RFInterface);
            status = NfcCxNtStatusFromNfcStatus(nfcStatus);
        }
        else
        {
            NT_ASSERT(RFInterface->eDeviceHostInitializationState == phLibNfc_SE_ActModeOff);

            pLibNfcContext->SEList[0].eSE_ActivationMode = phLibNfc_SE_ActModeOff;
        }

        RFInterface->eDeviceHostInitializationState = phLibNfc_SE_ActModeOff;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return status;
}

static VOID
NfcCxRFInterfaceSEEnumerateCB(
    _In_ VOID* pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceUpdateSEList(rfInterface);

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceSEEnumerate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_SE_Enumerate(NfcCxRFInterfaceSEEnumerateCB,
                                      &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceSESetModeConfigCB(
    _In_ VOID* pContext,
    _In_ phLibNfc_Handle hSecureElement,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceUpdateSEList(rfInterface);

    phLibNfc_SE_List_t* pSecureElement = NfcCxRFInterfaceGetSecureElementFromHandle(rfInterface, hSecureElement);

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, pSecureElement, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceSESetModeConfig(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    phLibNfc_SE_List_t *pSecureElement = (phLibNfc_SE_List_t*)Param1;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    phLibNfc_eSE_ActivationMode activationMode = RFInterface->SEActivationMode;

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (activationMode == pSecureElement->eSE_ActivationMode)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_INFO, "NFC-EE already has correct activation mode.");
        goto Done;
    }

    if (NULL == pSecureElement->hSecureElement) {
        pSecureElement->eSE_ActivationMode = activationMode;

        if (pSecureElement->eSE_ActivationMode == phLibNfc_SE_ActModeOn) {
            phLibNfc_CardEmulation_NtfRegister(NfcCxRFInterfaceHandleSEDeviceHostEvent, RFInterface);
        }
        else {
            phLibNfc_CardEmulation_NtfRegister(NULL, RFInterface);
        }

        NfcCxRFInterfaceDumpSEList(RFInterface);
        goto Done;
    }

    nfcStatus = phLibNfc_SE_SetMode((phLibNfc_Handle)pSecureElement->hSecureElement,
                                    activationMode,
                                    NfcCxRFInterfaceSESetModeConfigCB,
                                    &LibNfcContext);

    if (NFCSTATUS_SUCCESS == nfcStatus) {
        NfcCxRFInterfaceGetSEList(RFInterface,
                                  RFInterface->pLibNfcContext->SEList,
                                  &RFInterface->pLibNfcContext->SECount);
    }

Done:
    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

void
NfcCxRFInterfaceSESetPowerAndLinkControlCB(
    _In_ void* pContext,
    _In_ NFCSTATUS NfcStatus)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    auto context = reinterpret_cast<PNFCCX_RF_LIBNFC_REQUEST_CONTEXT>(pContext);
    PNFCCX_RF_INTERFACE rfInterface = context->RFInterface;

    NfcCxRFInterfaceUpdateSEList(rfInterface);

    // The NFC Controller will return REJECTED if the Power and Link Control command is not supported or
    // the NFC-EE's power is not managed by the NFC Controller.
    if (NfcStatus == NFCSTATUS_REJECTED)
    {
        // This NFC-EE's power cannot be managed by the NFC Controller.
        // Power and Link Control is an optional feature (as it only improves existing behavior).
        // So it is safe to ignore this problem.
        NfcStatus = NFCSTATUS_SUCCESS;
    }

    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    NfcCxInternalSequence(context->RFInterface, context->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceSESetPowerAndLinkControl(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    phLibNfc_SE_List_t* pSecureElement = (phLibNfc_SE_List_t*)Param1;

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (NULL == pSecureElement->hSecureElement)
    {
        // Skip setting Power and Link Control for Host NFC-EE, as it doesn't make any
        // sense.
        TRACE_LINE(LEVEL_INFO, "Can't set Power and Link Control on host NFC-EE.");
        goto Done;
    }

    phLibNfc_PowerLinkModes_t powerMode = RFInterface->SEPowerAndLinkControl;

    if (pSecureElement->eSE_PowerLinkMode == powerMode)
    {
        // Nothing to do.
        TRACE_LINE(LEVEL_INFO, "NFC-EE already has correct Power and Link Control mode.");
        goto Done;
    }

    nfcStatus = phLibNfc_SE_PowerAndLinkControl(
        pSecureElement->hSecureElement,
        RFInterface->SEPowerAndLinkControl,
        NfcCxRFInterfaceSESetPowerAndLinkControlCB,
        &LibNfcContext);
    if (nfcStatus == NFCSTATUS_FEATURE_NOT_SUPPORTED)
    {
        // It is not an issue if PowerAndLinkControl isn't supported.
        // We can produce a better behavior when it is. But nothing fundamentally breaks when it is not.
        TRACE_LINE(LEVEL_INFO, "Power and Link Control not supported.");
        nfcStatus = NFCSTATUS_SUCCESS;
        goto Done;
    }

Done:
    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRemoteDevSendCB(
    _In_ void* pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;
    NFCSTATUS nfcStatus;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCSTATUS_SUCCESS == NfcStatus) {
        rfInterface->pLibNfcContext->bIsHCERecv = TRUE;
        nfcStatus = phLibNfc_RemoteDev_Receive(rfInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                               NfcCxRFInterfaceRemoteDevReceiveCB,
                                               rfInterface);

        if (NFCSTATUS_PENDING != nfcStatus) {
            TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Receive failed %!NFCSTATUS!", nfcStatus);
        }
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "Remote device send failed %!NFCSTATUS!", NfcStatus);
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, NfcCxNtStatusFromNfcStatus(NfcStatus), NULL, NULL);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS
NfcCxRFInterfaceRemoteDevSend(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    phLibNfc_RemoteDevList_t* pRemDevList = RFInterface->pLibNfcContext->pRemDevList;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if ((phLibNfc_eISO14443_A_PCD != pRemDevList->psRemoteDevInfo->RemDevType) &&
        (phLibNfc_eISO14443_B_PCD != pRemDevList->psRemoteDevInfo->RemDevType)) {
        TRACE_LINE(LEVEL_ERROR, "Invalid remote device %!phNfc_eRFDevType_t!",
                   pRemDevList->psRemoteDevInfo->RemDevType);
        nfcStatus = NFCSTATUS_NOT_ALLOWED;
        goto Done;
    }

    nfcStatus = phLibNfc_RemoteDev_Send(RFInterface->pLibNfcContext->pRemDevList[0].hTargetDev,
                                        &RFInterface->sSendBuffer,
                                        NfcCxRFInterfaceRemoteDevSendCB,
                                        &LibNfcContext);

    if (NFCSTATUS_PENDING != nfcStatus) {
        TRACE_LINE(LEVEL_WARNING, "phLibNfc_RemoteDev_Send failed %!NFCSTATUS!, buffer length %d",
                   nfcStatus,
                   RFInterface->sSendBuffer.length);
        goto Done;
    }

Done:
    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);
    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceDisableSecureElementsCB(
    _In_ VOID* pContext,
    _In_ phLibNfc_Handle /*hSecureElement*/,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceUpdateSEList(rfInterface);

    if (NT_SUCCESS(status)) {
        NfcCxRepeatSequence(rfInterface, rfInterface->pSeqHandler, 1); // Redo sequence
    }

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceDisableSecureElements(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    for (uint8_t i = 0; i < RFInterface->pLibNfcContext->SECount; i++) {
        if ((RFInterface->pLibNfcContext->SEList[i].eSE_ActivationMode == phLibNfc_SE_ActModeOff) ||
            (RFInterface->pLibNfcContext->SEList[i].eSE_Type == phLibNfc_SE_Type_DeviceHost)) {
            continue;
        }

        nfcStatus = phLibNfc_SE_SetMode(RFInterface->pLibNfcContext->SEList[i].hSecureElement,
                                        phLibNfc_SE_ActModeOff,
                                        NfcCxRFInterfaceDisableSecureElementsCB,
                                        &LibNfcContext);
        break;
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceSetDefaultRoutingTableCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    rfInterface->pLibNfcContext->bIsDefaultRtngConfig = TRUE;
    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceSetDefaultRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;
    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    if (!RFInterface->pLibNfcContext->bIsDefaultRtngConfig) {
        NfcCxRFInterfaceSESetDefaultRoutingTable(RFInterface);

        if (RFInterface->pLibNfcContext->RtngTableCount != 0) {
            nfcStatus = phLibNfc_Mgt_ConfigRoutingTable(RFInterface->pLibNfcContext->RtngTableCount,
                                                        RFInterface->pLibNfcContext->RtngTable,
                                                        NfcCxRFInterfaceSetDefaultRoutingTableCB,
                                                        &LibNfcContext);
        }
    }
    else if (RFInterface->pLibNfcContext->ConfigStatus == 1) {
        // The NFCC doesn't persist the configuration upon reset, so we need restore routing table upon reset
        if (RFInterface->pLibNfcContext->RtngTableCount != 0) {
            nfcStatus = phLibNfc_Mgt_ConfigRoutingTable(RFInterface->pLibNfcContext->RtngTableCount,
                                                        RFInterface->pLibNfcContext->RtngTable,
                                                        NfcCxRFInterfaceSetDefaultRoutingTableCB,
                                                        &LibNfcContext);
        }
    }

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceConfigureRoutingTableCB(
    _In_ VOID *pContext,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = NfcCxNtStatusFromNfcStatus(NfcStatus);
    PNFCCX_RF_INTERFACE rfInterface = ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxInternalSequence(rfInterface, ((PNFCCX_RF_LIBNFC_REQUEST_CONTEXT)pContext)->Sequence, status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceConfigureRoutingTable(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    // Warning C4302: 'type cast': truncation from 'void *' to 'uint8_t'
#pragma warning(suppress:4302)
    uint8_t bNumRtngConfigs = (uint8_t)Param1;

    static NFCCX_RF_LIBNFC_REQUEST_CONTEXT LibNfcContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    LibNfcContext.RFInterface = RFInterface;
    LibNfcContext.Sequence = RFInterface->pSeqHandler;

    nfcStatus = phLibNfc_Mgt_ConfigRoutingTable(bNumRtngConfigs,
                                                (phLibNfc_RtngConfig_t*)Param2,
                                                NfcCxRFInterfaceConfigureRoutingTableCB,
                                                &LibNfcContext);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceRemoteDevNtfCB(
    _In_ VOID *pContext,
    _In_ phLibNfc_RemoteDevList_t *psRemoteDevList,
    _In_ uint8_t uNofRemoteDev,
    _In_ NFCSTATUS NfcStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "NfcStatus=%!NFCSTATUS! uNofRemoteDev=%d", NfcStatus, uNofRemoteDev);

    if ((NFCSTATUS_SUCCESS == NfcStatus) ||
        (NFCSTATUS_MULTIPLE_PROTOCOLS == NfcStatus) ||
        (NFCSTATUS_MULTIPLE_TAGS == NfcStatus))
    {
        phLibNfc_RemoteDevList_t *psRemoteDev = psRemoteDevList;

        // Prioritize Peer-to-Peer over anything else (particularly Card Emulation).
        for (uint8_t i = 1; i < uNofRemoteDev; i++) {
            if (psRemoteDevList[i].psRemoteDevInfo->RemDevType == phNfc_eNfcIP1_Target) {
                // Ignore any remote devices in the list that are before the P2P device.
                psRemoteDev = &psRemoteDevList[i];
                uNofRemoteDev -= i;
                break;
            }
        }

        TRACE_LINE(LEVEL_INFO, "RemDevType=%!phNfc_eRFDevType_t!", psRemoteDev->psRemoteDevInfo->RemDevType);

        TraceLoggingWrite(
            g_hNfcCxProvider,
            "NfcCxDeviceType",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingValue((DWORD)(psRemoteDev->psRemoteDevInfo->RemDevType), "DeviceType"));

        status = NfcCxRFInterfaceSetRemoteDevList(rfInterface, psRemoteDev, uNofRemoteDev);

        if (NT_SUCCESS(status)) {
            NfcCxStateInterfaceStateHandler(NfcCxRFInterfaceGetStateInterface(rfInterface), NfcCxEventDiscovered, NULL, NULL, NULL);
        }
        else {
            TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory for remote device list");
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
}

NTSTATUS
NfcCxRFInterfaceRemoteDevNtfRegister(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    NFCSTATUS nfcStatus = NFCSTATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    nfcStatus = phLibNfc_RemoteDev_NtfRegister(&RFInterface->RegInfo, NfcCxRFInterfaceRemoteDevNtfCB, RFInterface);

    Status = NfcCxNtStatusFromNfcStatus(nfcStatus);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID CALLBACK
NfcCxRFInterfaceTagPresenceThread(
    _In_ PTP_CALLBACK_INSTANCE pInstance,
    _In_ PVOID pContext,
    _In_ PTP_WORK /*pWork*/
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    CallbackMayRunLong(pInstance);

    WdfWaitLockAcquire(rfInterface->PresenceCheckLock, NULL); // Ensure single presence check thread

    while (rfInterface->pLibNfcContext->bIsTagPresent && NT_SUCCESS(status))
    {
        if (WaitForSingleObject(rfInterface->hStartPresenceCheck, INFINITE) != WAIT_OBJECT_0)
            break;

        if (rfInterface->pLibNfcContext->bIsTagPresent) {
            Sleep(PRESENCE_CHECK_INTERVAL);
        }

        if (rfInterface->pLibNfcContext->bIsTagPresent &&
            WaitForSingleObject(rfInterface->hStartPresenceCheck, 0) == WAIT_OBJECT_0) {
            status = NfcCxRFInterfaceTargetCheckPresence(rfInterface);
        }
    }

    WdfWaitLockRelease(rfInterface->PresenceCheckLock);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreInitializeCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG Flags,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "Flags=0x%x", Flags);

    if (NT_SUCCESS(Status)) {
        if ((Flags & NFC_CX_SEQUENCE_PRE_INIT_FLAG_SKIP_CONFIG) != 0 && (Flags & NFC_CX_SEQUENCE_PRE_INIT_FLAG_FORCE_CONFIG) != 0) {
            Status = STATUS_INVALID_PARAMETER;
        }
        else if ((Flags & NFC_CX_SEQUENCE_PRE_INIT_FLAG_SKIP_CONFIG) != 0) {
            rfInterface->pLibNfcContext->eInitType = phLibNfc_InitType_SkipConfig;
        }
        else if ((Flags & NFC_CX_SEQUENCE_PRE_INIT_FLAG_FORCE_CONFIG) != 0 || !IsEqualGUID(SESSION_ID, rfInterface->SessionId)) {
            rfInterface->pLibNfcContext->eInitType = phLibNfc_InitType_ForceConfig;
        }
    }

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreInitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    RFInterface->pLibNfcContext->eInitType = phLibNfc_InitType_Default;

    if (RFInterface->SeqHandlers[SequencePreInit] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequencePreInit](RFInterface->FdoContext->Device,
                                                  SequencePreInit,
                                                  NfcCxRFInterfacePreInitializeCB,
                                                  (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostInitializeCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG Flags,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "Flags=0x%x", Flags);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);

    if (Flags & NFC_CX_SEQUENCE_INIT_COMPLETE_FLAG_REDO) {
        Status = NfcCxRFInterfaceDeinitialize(rfInterface, STATUS_SUCCESS, NULL, NULL);

        if (Status == STATUS_PENDING) {
            NfcCxRepeatSequence(rfInterface, rfInterface->pSeqHandler, 3); // Repeat initialization sequence
        }
    }

    if (Status != STATUS_PENDING) {
        NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostInitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceInitComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequenceInitComplete](RFInterface->FdoContext->Device,
                                                       SequenceInitComplete,
                                                       NfcCxRFInterfacePostInitializeCB,
                                                       (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreDeinitializeCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG Flags,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, (VOID*)((Flags & NFC_CX_SEQUENCE_PRE_SHUTDOWN_FLAG_SKIP_RESET) != 0), NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreDeinitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequencePreShutdown] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface, MAX_DEINIT_TIMEOUT);
        RFInterface->SeqHandlers[SequencePreShutdown](RFInterface->FdoContext->Device,
                                                      SequencePreShutdown,
                                                      NfcCxRFInterfacePreDeinitializeCB,
                                                      (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostDeinitializeCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostDeinitialize(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceShutdownComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface, MAX_DEINIT_TIMEOUT);
        RFInterface->SeqHandlers[SequenceShutdownComplete](RFInterface->FdoContext->Device,
                                                           SequenceShutdownComplete,
                                                           NfcCxRFInterfacePostDeinitializeCB,
                                                           (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreRfDiscoveryStartCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreRfDiscoveryStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequencePreRfDiscStart] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequencePreRfDiscStart](RFInterface->FdoContext->Device,
                                                         SequencePreRfDiscStart,
                                                         NfcCxRFInterfacePreRfDiscoveryStartCB,
                                                         (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostRfDiscoveryStartCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostRfDiscoveryStart(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceRfDiscStartComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequenceRfDiscStartComplete](RFInterface->FdoContext->Device,
                                                              SequenceRfDiscStartComplete,
                                                              NfcCxRFInterfacePostRfDiscoveryStartCB,
                                                              (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreRfDiscoveryStopCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreRfDiscoveryStop(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequencePreRfDiscStop] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequencePreRfDiscStop](RFInterface->FdoContext->Device,
                                                        SequencePreRfDiscStop,
                                                        NfcCxRFInterfacePreRfDiscoveryStopCB,
                                                        (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostRfDiscoveryStopCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostRfDiscoveryStop(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceRfDiscStopComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequenceRfDiscStopComplete](RFInterface->FdoContext->Device,
                                                             SequenceRfDiscStopComplete,
                                                             NfcCxRFInterfacePostRfDiscoveryStopCB,
                                                             (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreSEEnumerateCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG Flags,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "Flags=0x%x", Flags);

    if (NT_SUCCESS(Status) && (Flags & NFC_CX_SEQUENCE_PRE_NFCEE_DISC_FLAG_SKIP) != 0) {
        NfcCxSkipSequence(rfInterface, rfInterface->pSeqHandler, 1); // Skip SEEnumerate Sequence
    }

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreSEEnumerate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequencePreNfceeDisc] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequencePreNfceeDisc](RFInterface->FdoContext->Device,
                                                       SequencePreNfceeDisc,
                                                       NfcCxRFInterfacePreSEEnumerateCB,
                                                       (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostSEEnumerateCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostSEEnumerate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceNfceeDiscComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequenceNfceeDiscComplete](RFInterface->FdoContext->Device,
                                                            SequenceNfceeDiscComplete,
                                                            NfcCxRFInterfacePostSEEnumerateCB,
                                                            (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePreRecoveryCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePreRecovery(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequencePreRecovery] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequencePreRecovery](RFInterface->FdoContext->Device,
                                                      SequencePreRecovery,
                                                      NfcCxRFInterfacePreRecoveryCB,
                                                      (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

_Function_class_(EVT_NFC_CX_SEQUENCE_COMPLETION_ROUTINE)
static VOID
NfcCxRFInterfacePostRecoveryCB(
    _In_ WDFDEVICE /*Device*/,
    _In_ NTSTATUS Status,
    _In_ ULONG /*Flags*/,
    _In_ WDFCONTEXT Context
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)Context;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceStopWatchdogTimer(rfInterface);
    NfcCxInternalSequence(rfInterface, rfInterface->pSeqHandler, Status, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
}

NTSTATUS
NfcCxRFInterfacePostRecovery(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    WdfWaitLockAcquire(RFInterface->SequenceLock, NULL);

    if (RFInterface->SeqHandlers[SequenceRecoveryComplete] != NULL) {
        Status = STATUS_PENDING;
        NfcCxRFInterfaceStartWatchdogTimer(RFInterface);
        RFInterface->SeqHandlers[SequenceRecoveryComplete](RFInterface->FdoContext->Device,
                                                           SequenceRecoveryComplete,
                                                           NfcCxRFInterfacePostRecoveryCB,
                                                           (WDFCONTEXT)RFInterface);
    }

    WdfWaitLockRelease(RFInterface->SequenceLock);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static VOID
NfcCxRFInterfaceLibNfcMessageHandler(
    _In_ LPVOID pContext,
    _In_ UINT32 Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    )
{
    PNFCCX_RF_INTERFACE rfInterface = (PNFCCX_RF_INTERFACE)pContext;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NT_ASSERT(NULL != rfInterface);
    NT_ASSERT(NULL != rfInterface->pLibNfcContext);

    TRACE_LINE(LEVEL_INFO, "%!NFCCX_LIBNFC_MESSAGE!: %p, %p", Message, (VOID*)Param1, (VOID*)Param2);

    switch (Message)
    {
    case LIBNFC_INIT:
    case LIBNFC_DEINIT:
    case LIBNFC_DISCOVER_CONFIG:
    case LIBNFC_DISCOVER_STOP:
    case LIBNFC_TARGET_ACTIVATE:
    case LIBNFC_TARGET_DEACTIVATE_SLEEP:
    case LIBNFC_TAG_WRITE:
    case LIBNFC_TAG_CONVERT_READONLY:
    case LIBNFC_TARGET_TRANSCEIVE:
    case LIBNFC_TARGET_SEND:
    case LIBNFC_TARGET_PRESENCE_CHECK:
    case LIBNFC_SE_ENUMERATE:
    case LIBNFC_SE_SET_MODE:
    case LIBNFC_SE_SET_ROUTING_TABLE:
    case LIBNFC_EMEBEDDED_SE_TRANSCEIVE:
    case LIBNFC_EMBEDDED_SE_GET_ATR_STRING:
    case LIBNFC_SNEP_CLIENT_PUT:
        {
            rfInterface->pLibNfcContext->Status = NfcCxStateInterfaceStateHandler(NfcCxRFInterfaceGetStateInterface(rfInterface),
                                                                                  NfcCxRFInterfaceGetEventType(Message),
                                                                                  (VOID*)Message, (VOID*)Param1, (VOID*)Param2);
            TRACE_LINE(LEVEL_VERBOSE, "SetEvent, handle %p", rfInterface->pLibNfcContext->hNotifyCompleteEvent);
            if (!SetEvent(rfInterface->pLibNfcContext->hNotifyCompleteEvent))
            {
                NTSTATUS eventStatus = NTSTATUS_FROM_WIN32(GetLastError());
                TRACE_LINE(LEVEL_ERROR, "SetEvent with handle %p failed, %!STATUS!", rfInterface->pLibNfcContext->hNotifyCompleteEvent, eventStatus);
            }
        }
        break;

    case LIBNFC_STATE_HANDLER:
        NfcCxStateInterfaceStateHandler(NfcCxRFInterfaceGetStateInterface(rfInterface),
                                        (NFCCX_CX_EVENT)Param1, (VOID*)Param2, (VOID*)Param3, (VOID*)Param4);
        break;

    case LIBNFC_SEQUENCE_HANDLER:
        NfcCxSequenceHandler(rfInterface, (PNFCCX_CX_SEQUENCE)Param1, (NTSTATUS)Param2, (VOID*)Param3, (VOID*)Param4);
        break;

    default:
        NT_ASSERTMSG("Invalid Message", FALSE);
        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

static NTSTATUS
NfcCxRFInterfaceInitSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateInitialize(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(NFCCX_IS_USER_EVENT(Event));

    rfInterface->eDeviceHostInitializationState = phLibNfc_SE_ActModeOff;

    if (rfInterface->pLibNfcContext->EnableSEDiscovery) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(InitializeSequence)
            RF_INTERFACE_INITIALIZE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_NTF_REGISTER_SEQUENCE
            LLCP_INTERFACE_CONFIG_SEQUENCE
            RF_INTERFACE_SE_NTF_REGISTER_SEQUENCE
            RF_INTERFACE_SE_ENUMERATE_SEQUENCE
            RF_INTERFACE_SE_DISABLE_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        InitializeSequence[ARRAYSIZE(InitializeSequence)-1].SequenceProcess =
                NfcCxRFInterfaceInitSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, InitializeSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(InitializeSequence)
            RF_INTERFACE_INITIALIZE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_NTF_REGISTER_SEQUENCE
            LLCP_INTERFACE_CONFIG_SEQUENCE
            RF_INTERFACE_SE_NTF_REGISTER_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        InitializeSequence[ARRAYSIZE(InitializeSequence)-1].SequenceProcess =
                NfcCxRFInterfaceInitSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, InitializeSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceShutdownSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrShutdownSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceShutdownSeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateShutdown(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NFCCX_CX_BEGIN_SEQUENCE_MAP(ShutdownSequence)
        RF_INTERFACE_DEINITIALIZE_SEQUENCE
    NFCCX_CX_END_SEQUENCE_MAP()

    if (NFCCX_IS_USER_EVENT(Event)) {
        ShutdownSequence[ARRAYSIZE(ShutdownSequence)-1].SequenceProcess =
            NfcCxRFInterfaceUsrShutdownSeqComplete;
    }
    else {
        ShutdownSequence[ARRAYSIZE(ShutdownSequence)-1].SequenceProcess =
            NfcCxRFInterfaceShutdownSeqComplete;
    }

    NFCCX_INIT_SEQUENCE(rfInterface, ShutdownSequence);
    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceRecoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRecovery(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(NFCCX_IS_INTERNAL_EVENT(Event));

    rfInterface->eDeviceHostInitializationState = phLibNfc_SE_ActModeOff;

    if (rfInterface->pLibNfcContext->EnableSEDiscovery)
    {
        // Recovery needs to reset us to the state expected by the clients.
        // If discovery was enabled and the SE was on, reset the device host to its previous state.
        if (rfInterface->pLibNfcContext->SECount > 0
            && rfInterface->pLibNfcContext->SEList[0].eSE_Type == phLibNfc_SE_Type_DeviceHost
            && rfInterface->pLibNfcContext->SEList[0].eSE_ActivationMode == phLibNfc_SE_ActModeOn)
        {
            rfInterface->eDeviceHostInitializationState = rfInterface->pLibNfcContext->SEList[0].eSE_ActivationMode;
        }

        NFCCX_CX_BEGIN_SEQUENCE_MAP(RecoverySequence)
            RF_INTERFACE_PRE_RECOVERY_SEQUENCE
            RF_INTERFACE_DEINITIALIZE_SEQUENCE
            RF_INTERFACE_INITIALIZE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_NTF_REGISTER_SEQUENCE
            LLCP_INTERFACE_CONFIG_SEQUENCE
            RF_INTERFACE_SE_NTF_REGISTER_SEQUENCE
            RF_INTERFACE_SE_ENUMERATE_SEQUENCE
            RF_INTERFACE_POST_RECOVERY_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        RecoverySequence[ARRAYSIZE(RecoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceRecoverySeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, RecoverySequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(RecoverySequence)
            RF_INTERFACE_PRE_RECOVERY_SEQUENCE
            RF_INTERFACE_DEINITIALIZE_SEQUENCE
            RF_INTERFACE_INITIALIZE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_NTF_REGISTER_SEQUENCE
            LLCP_INTERFACE_CONFIG_SEQUENCE
            RF_INTERFACE_SE_NTF_REGISTER_SEQUENCE
            RF_INTERFACE_POST_RECOVERY_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        RecoverySequence[ARRAYSIZE(RecoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceRecoverySeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, RecoverySequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceStartRfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrStartRfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceStartRfDiscoverySeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfIdle2RfDiscovery(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NFCCX_CX_BEGIN_SEQUENCE_MAP(StartRfDiscoverySequence)
        RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE
    NFCCX_CX_END_SEQUENCE_MAP()

    if (NFCCX_IS_USER_EVENT(Event)) {
        StartRfDiscoverySequence[ARRAYSIZE(StartRfDiscoverySequence)-1].SequenceProcess =
            NfcCxRFInterfaceUsrStartRfDiscoverySeqComplete;
    }
    else {
        StartRfDiscoverySequence[ARRAYSIZE(StartRfDiscoverySequence)-1].SequenceProcess =
            NfcCxRFInterfaceStartRfDiscoverySeqComplete;
    }

    NFCCX_INIT_SEQUENCE(rfInterface, StartRfDiscoverySequence);
    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceStopRfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDiscovery2RfIdle(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NFCCX_CX_BEGIN_SEQUENCE_MAP(StopRfDiscoverySequence)
        RF_INTERFACE_RF_DISCOVERY_STOP_SEQUENCE
    NFCCX_CX_END_SEQUENCE_MAP()

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(NFCCX_IS_USER_EVENT(Event));

    StopRfDiscoverySequence[ARRAYSIZE(StopRfDiscoverySequence)-1].SequenceProcess =
            NfcCxRFInterfaceStopRfDiscoverySeqComplete;

    NFCCX_INIT_SEQUENCE(rfInterface, StopRfDiscoverySequence);
    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceP2PActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrP2PActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceP2PActive2RfIdleSeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTagActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrTagActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceTagActive2RfIdleSeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

static NTSTATUS
NfcCxRFInterfacePCDActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (RFInterface->pLibNfcContext->bIsHCEActivated) {
        RFInterface->pLibNfcContext->bIsHCEActivated = FALSE;
        NfcCxSEInterfacePurgeHCERecvQueue(NfcCxRFInterfaceGetSEInterface(RFInterface));
        NfcCxSEInterfaceHandleEvent(NfcCxRFInterfaceGetSEInterface(RFInterface),
                                    HceDeactivated,
                                    RFInterface->pLibNfcContext->SEList,
                                    NULL);
    }

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrPCDActive2RfIdleSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfacePCDActive2RfIdleSeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfActive2RfIdle(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Target) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PActive2RfIdleSequence)
            LLCP_INTERFACE_DEACTIVATE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_DISCONNECT_STOP_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            P2PActive2RfIdleSequence[ARRAYSIZE(P2PActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrP2PActive2RfIdleSeqComplete;
        }
        else {
            P2PActive2RfIdleSequence[ARRAYSIZE(P2PActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceP2PActive2RfIdleSeqComplete;
        }

        rfInterface->pLibNfcContext->eReleaseType = NFC_DISCOVERY_STOP;

        NFCCX_INIT_SEQUENCE(rfInterface, P2PActive2RfIdleSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Initiator) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PActive2RfIdleSequence)
            LLCP_INTERFACE_DEACTIVATE_SEQUENCE
            RF_INTERFACE_RF_DISCOVERY_STOP_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            P2PActive2RfIdleSequence[ARRAYSIZE(P2PActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrP2PActive2RfIdleSeqComplete;
        }
        else {
            P2PActive2RfIdleSequence[ARRAYSIZE(P2PActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceP2PActive2RfIdleSeqComplete;
        }

        NFCCX_INIT_SEQUENCE(rfInterface, P2PActive2RfIdleSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if ((rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
             (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD)) {

        NFCCX_CX_BEGIN_SEQUENCE_MAP(PCDActive2RfIdleSequence)
            RF_INTERFACE_RF_DISCOVERY_STOP_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            PCDActive2RfIdleSequence[ARRAYSIZE(PCDActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrPCDActive2RfIdleSeqComplete;
        }
        else {
            PCDActive2RfIdleSequence[ARRAYSIZE(PCDActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfacePCDActive2RfIdleSeqComplete;
        }

        NFCCX_INIT_SEQUENCE(rfInterface, PCDActive2RfIdleSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(TagActive2RfIdleSequence)
            RF_INTERFACE_REMOTEDEV_DISCONNECT_STOP_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            TagActive2RfIdleSequence[ARRAYSIZE(TagActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrTagActive2RfIdleSeqComplete;
        }
        else {
            TagActive2RfIdleSequence[ARRAYSIZE(TagActive2RfIdleSequence)-1].SequenceProcess =
                NfcCxRFInterfaceTagActive2RfIdleSeqComplete;
        }

        NfcCxRFInterfaceTagConnectionLost(rfInterface);
        rfInterface->pLibNfcContext->eReleaseType = NFC_DISCOVERY_STOP;

        NFCCX_INIT_SEQUENCE(rfInterface, TagActive2RfIdleSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceP2PActive2RfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrP2PActive2RfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceP2PActive2RfDiscoverySeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTagActive2RfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxRFInterfaceClearRemoteDevList(RFInterface);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceUsrTagActive2RfDiscoverySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;
    Status = NfcCxRFInterfaceTagActive2RfDiscoverySeqComplete(RFInterface, Status, Param1, Param2);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfActive2RfDiscovery(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Target) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PActive2RfDiscoverySequence)
            LLCP_INTERFACE_DEACTIVATE_SEQUENCE
            RF_INTERFACE_REMOTEDEV_DISCONNECT_STOP_SEQUENCE
            RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            P2PActive2RfDiscoverySequence[ARRAYSIZE(P2PActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrP2PActive2RfDiscoverySeqComplete;
        }
        else {
            P2PActive2RfDiscoverySequence[ARRAYSIZE(P2PActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceP2PActive2RfDiscoverySeqComplete;
        }

        rfInterface->pLibNfcContext->eReleaseType = NFC_DISCOVERY_STOP;

        NFCCX_INIT_SEQUENCE(rfInterface, P2PActive2RfDiscoverySequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Initiator) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PActive2RfDiscoverySequence)
            LLCP_INTERFACE_DEACTIVATE_SEQUENCE
            RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            P2PActive2RfDiscoverySequence[ARRAYSIZE(P2PActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrP2PActive2RfDiscoverySeqComplete;
        }
        else {
            P2PActive2RfDiscoverySequence[ARRAYSIZE(P2PActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceP2PActive2RfDiscoverySeqComplete;
        }

        NFCCX_INIT_SEQUENCE(rfInterface, P2PActive2RfDiscoverySequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if ((rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
             (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD)) {

        if (rfInterface->pLibNfcContext->bIsHCEActivated) {
            rfInterface->pLibNfcContext->bIsHCEActivated = FALSE;
            NfcCxSEInterfacePurgeHCERecvQueue(NfcCxRFInterfaceGetSEInterface(rfInterface));
            NfcCxSEInterfaceHandleEvent(NfcCxRFInterfaceGetSEInterface(rfInterface),
                                        HceDeactivated,
                                        rfInterface->pLibNfcContext->SEList,
                                        NULL);
        }
        NfcCxRFInterfaceClearRemoteDevList(rfInterface);
    }
    else {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(TagActive2RfDiscoverySequence)
            RF_INTERFACE_REMOTEDEV_DISCONNECT_STOP_SEQUENCE
            RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        if (NFCCX_IS_USER_EVENT(Event)) {
            TagActive2RfDiscoverySequence[ARRAYSIZE(TagActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceUsrTagActive2RfDiscoverySeqComplete;
        }
        else {
            TagActive2RfDiscoverySequence[ARRAYSIZE(TagActive2RfDiscoverySequence)-1].SequenceProcess =
                NfcCxRFInterfaceTagActive2RfDiscoverySeqComplete;
        }

        NfcCxRFInterfaceTagConnectionLost(rfInterface);
        rfInterface->pLibNfcContext->eReleaseType = NFC_DISCOVERY_STOP;

        NFCCX_INIT_SEQUENCE(rfInterface, TagActive2RfDiscoverySequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceTagDiscoveredSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventActivated, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceP2PDiscoveredSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceBarcodeReadSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    // The tag will keep transmitting so block future attempts until enough time passes
    RFInterface->bKovioDetected = GetTickCount64();

    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDiscovery2RfDiscovered(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(NFCCX_IS_INTERNAL_EVENT(Event));

    // Enable kovio again if some other tag detected
    if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType != phLibNfc_eKovio_PICC) {
        rfInterface->bKovioDetected = 0;
    }

    if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Target) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PDiscoveredSequence)
            RF_INTERFACE_REMOTEDEV_CONNECT_SEQUENCE
            LLCP_INTERFACE_CHECK_SEQUENCE
            LLCP_INTERFACE_ACTIVATE_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        P2PDiscoveredSequence[ARRAYSIZE(P2PDiscoveredSequence)-1].SequenceProcess =
            NfcCxRFInterfaceP2PDiscoveredSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, P2PDiscoveredSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Initiator) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(P2PDiscoveredSequence)
            LLCP_INTERFACE_CHECK_SEQUENCE
            LLCP_INTERFACE_ACTIVATE_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        P2PDiscoveredSequence[ARRAYSIZE(P2PDiscoveredSequence)-1].SequenceProcess =
            NfcCxRFInterfaceP2PDiscoveredSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, P2PDiscoveredSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else if ((rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
             (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD)) {
    }
    else if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eKovio_PICC) {
        // As we already have all that we need keep the logic simple
        NFCCX_CX_BEGIN_SEQUENCE_MAP(TagReadBarcodeSequence)
            RF_INTERFACE_TAG_READ_BARCODE_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        TagReadBarcodeSequence[ARRAYSIZE(TagReadBarcodeSequence) - 1].SequenceProcess =
            NfcCxRFInterfaceBarcodeReadSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, TagReadBarcodeSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(TagDiscoveredSequence)
            RF_INTERFACE_REMOTEDEV_CONNECT_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        TagDiscoveredSequence[ARRAYSIZE(TagDiscoveredSequence)-1].SequenceProcess =
            NfcCxRFInterfaceTagDiscoveredSeqComplete;

        NFCCX_INIT_SEQUENCE(rfInterface, TagDiscoveredSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceTagReadNdefSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (Status != STATUS_LINK_FAILED) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
        SubmitThreadpoolWork(RFInterface->tpTagPrescenceWork);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceSNEPConnectSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTagActivateSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDiscovered2RfDataXchg(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (NFCCX_IS_INTERNAL_EVENT(Event)) {
        if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Target ||
            rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eNfcIP1_Initiator) {

            NFCCX_CX_BEGIN_SEQUENCE_MAP(SNEPConnectSequence)
                SNEP_INTERFACE_CONNECT_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            SNEPConnectSequence[ARRAYSIZE(SNEPConnectSequence)-1].SequenceProcess =
                NfcCxRFInterfaceSNEPConnectSeqComplete;

            NFCCX_INIT_SEQUENCE(rfInterface, SNEPConnectSequence);
            status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        else if ((rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_A_PCD) ||
                 (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eISO14443_B_PCD)) {
        }
        else if (rfInterface->pLibNfcContext->pRemDevList->psRemoteDevInfo->RemDevType == phLibNfc_eKovio_PICC) {
        }
        else {
            NFCCX_CX_BEGIN_SEQUENCE_MAP(TagReadNdefSequence)
                RF_INTERFACE_TAG_READ_NDEF_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            TagReadNdefSequence[ARRAYSIZE(TagReadNdefSequence)-1].SequenceProcess =
                NfcCxRFInterfaceTagReadNdefSeqComplete;

            NFCCX_INIT_SEQUENCE(rfInterface, TagReadNdefSequence);
            status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
    }
    else {
        if (rfInterface->pLibNfcContext->bIsTagPresent) {
            NFCCX_CX_BEGIN_SEQUENCE_MAP(TagActivateSequence)
                RF_INTERFACE_REMOTEDEV_CONNECT_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            TagActivateSequence[ARRAYSIZE(TagActivateSequence)-1].SequenceProcess =
                NfcCxRFInterfaceTagActivateSeqComplete;

            NFCCX_INIT_SEQUENCE(rfInterface, TagActivateSequence);
            status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        else {
            status = STATUS_INVALID_DEVICE_STATE;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceSESetModeSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceSetRoutingTableSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceSEEnumerateSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceESETransceiveSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceESEGetATRStringSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfIdle(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    // Warning C4311: 'type cast': pointer truncation from 'void *' to 'UINT32'
    // Warning C4302: 'type cast': truncation from 'void *' to 'UINT32'
#pragma warning(suppress:4311 4302)
    UINT32 eLibNfcMessage = (UINT32)Param1;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (Event == NfcCxEventConfig)
    {
        switch (eLibNfcMessage)
        {
        case LIBNFC_SE_SET_ROUTING_TABLE:
            {
                NFCCX_CX_BEGIN_SEQUENCE_MAP(SESetRoutingTableSequence)
                    RF_INTERFACE_SE_SET_ROUTING_MODE_SEQUENCE
                NFCCX_CX_END_SEQUENCE_MAP()

                SESetRoutingTableSequence[ARRAYSIZE(SESetRoutingTableSequence)-1].SequenceProcess =
                    NfcCxRFInterfaceSetRoutingTableSeqComplete;

                NFCCX_INIT_SEQUENCE(rfInterface, SESetRoutingTableSequence);
                status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
            }
            break;

        case LIBNFC_SE_ENUMERATE:
            {
                NFCCX_CX_BEGIN_SEQUENCE_MAP(SEEnumerateSequence)
                    RF_INTERFACE_SE_ENUMERATE_SEQUENCE
                    RF_INTERFACE_SE_DISABLE_SEQUENCE
                NFCCX_CX_END_SEQUENCE_MAP()

                SEEnumerateSequence[ARRAYSIZE(SEEnumerateSequence)-1].SequenceProcess =
                    NfcCxRFInterfaceSEEnumerateSeqComplete;

                NFCCX_INIT_SEQUENCE(rfInterface, SEEnumerateSequence);
                status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }
    else if (Event == NfcCxEventSE)
    {
        status = NfcCxRFInterfaceStateSEEvent(rfInterface, eLibNfcMessage, Param2, Param3);
    }
    else {
        NT_ASSERT(Event == NfcCxEventConfigDiscovery || Event == NfcCxEventStopDiscovery);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceRfDiscoveryConfigSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if ((RFInterface->FdoContext->NfcCxClientGlobal->Config.DriverFlags & NFC_CX_DRIVER_DISABLE_RECOVERY_MODE) == 0) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventRecovery, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDiscovery(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;
    phLibNfc_sADD_Cfg_t discoveryConfig = {};

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (Event == NfcCxEventConfigDiscovery)
    {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(RfDiscoveryConfigSequence)
            RF_INTERFACE_RF_DISCOVERY_STOP_SEQUENCE
            RF_INTERFACE_RF_DISCOVERY_CONFIG_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        RfDiscoveryConfigSequence[ARRAYSIZE(RfDiscoveryConfigSequence)-1].SequenceProcess =
                    NfcCxRFInterfaceRfDiscoveryConfigSeqComplete;

        NfcCxRFInterfaceGetDiscoveryConfig(rfInterface, &discoveryConfig);

        if (memcmp(&discoveryConfig, &rfInterface->DiscoveryConfig, sizeof(phLibNfc_sADD_Cfg_t)) != 0) {
            NFCCX_INIT_SEQUENCE(rfInterface, RfDiscoveryConfigSequence);
            status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        else {
            TRACE_LINE(LEVEL_INFO, "No change in discovery configuration");
        }
    }
    else
    {
        // Warning C4311: 'type cast': pointer truncation from 'void *' to 'UINT32'
        // Warning C4302: 'type cast': truncation from 'void *' to 'UINT32'
#pragma warning(suppress:4311 4302)
        UINT32 eLibNfcMessage = (UINT32)Param1;

        NT_ASSERT(Event == NfcCxEventSE);
        status = NfcCxRFInterfaceStateSEEvent(rfInterface, eLibNfcMessage, Param2, Param3);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDiscovered(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    // Warning C4311: 'type cast': pointer truncation from 'void *' to 'UINT32'
    // Warning C4302: 'type cast': truncation from 'void *' to 'UINT32'
#pragma warning(suppress:4311 4302)
    UINT32 eLibNfcMessage = (UINT32)Param1;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(Event == NfcCxEventSE);
    status = NfcCxRFInterfaceStateSEEvent(rfInterface, eLibNfcMessage, Param2, Param3);

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceTagDeactivateSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDataXchg2RfDiscovered(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(Event == NfcCxEventDeactivateSleep);

    if (rfInterface->pLibNfcContext->bIsTagPresent) {
        NFCCX_CX_BEGIN_SEQUENCE_MAP(TagDeactivateSequence)
            RF_INTERFACE_REMOTEDEV_DISCONNECT_SEQUENCE
        NFCCX_CX_END_SEQUENCE_MAP()

        TagDeactivateSequence[ARRAYSIZE(TagDeactivateSequence)-1].SequenceProcess =
                    NfcCxRFInterfaceTagDeactivateSeqComplete;

        rfInterface->pLibNfcContext->eReleaseType = NFC_DEVICE_SLEEP;

        NFCCX_INIT_SEQUENCE(rfInterface, TagDeactivateSequence);
        status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
    }
    else {
        status = STATUS_INVALID_DEVICE_STATE;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

static NTSTATUS
NfcCxRFInterfaceTagWriteNdefSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if (Status == STATUS_LINK_FAILED) {
        // Something is wrong with the RF connection. So let's restart discovery.
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTagConvertReadOnlySeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTargetTransceiveSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else if (Status == STATUS_LINK_FAILED) {
        // Something is wrong with the RF connection. So let's restart discovery.
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceTargetPresenceCheckSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventDeactivated, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceSNEPClientPutSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

static NTSTATUS
NfcCxRFInterfaceRemoteDevSendSeqComplete(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* /*Param1*/,
    _In_opt_ VOID* /*Param2*/
    )
{
    PNFCCX_STATE_INTERFACE stateInterface = NfcCxRFInterfaceGetStateInterface(RFInterface);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    RFInterface->pLibNfcContext->Status = Status;

    if (NT_SUCCESS(Status)) {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventReqCompleted, NULL, NULL, NULL);
    }
    else {
        NfcCxStateInterfaceStateHandler(stateInterface, NfcCxEventFailed, NULL, NULL, NULL);
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, Status);
    return Status;
}

NTSTATUS
NfcCxRFInterfaceStateRfDataXchg(
    _In_ PNFCCX_STATE_INTERFACE StateInterface,
    _In_ NFCCX_CX_EVENT Event,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;

    // Warning C4311: 'type cast': pointer truncation from 'void *' to 'UINT32'
    // Warning C4302: 'type cast': truncation from 'void *' to 'UINT32'
#pragma warning(suppress:4311 4302)
    UINT32 eLibNfcMessage = (UINT32)Param1;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    UNREFERENCED_PARAMETER(Event);

    NT_ASSERT(Event == NfcCxEventDataXchg || Event == NfcCxEventConfig || Event == NfcCxEventSE);

    if (Event == NfcCxEventSE)
    {
        status = NfcCxRFInterfaceStateSEEvent(rfInterface, eLibNfcMessage, Param2, Param3);
    }
    else
    {
        switch (eLibNfcMessage)
        {
        case LIBNFC_TAG_WRITE:
            {
                if (rfInterface->pLibNfcContext->bIsTagPresent) {
                    NFCCX_CX_BEGIN_SEQUENCE_MAP(TagWriteNdefSequence)
                        RF_INTERFACE_TAG_WRITE_NDEF_SEQUENCE
                    NFCCX_CX_END_SEQUENCE_MAP()

                    TagWriteNdefSequence[ARRAYSIZE(TagWriteNdefSequence)-1].SequenceProcess =
                        NfcCxRFInterfaceTagWriteNdefSeqComplete;

                    NFCCX_INIT_SEQUENCE(rfInterface, TagWriteNdefSequence);
                    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
                }
                else {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
            }
            break;

        case LIBNFC_TAG_CONVERT_READONLY:
            {
                if (rfInterface->pLibNfcContext->bIsTagPresent) {
                    NFCCX_CX_BEGIN_SEQUENCE_MAP(TagConvertReadOnlySequence)
                        RF_INTERFACE_TAG_CONVERT_READONLY_SEQUENCE
                    NFCCX_CX_END_SEQUENCE_MAP()

                    TagConvertReadOnlySequence[ARRAYSIZE(TagConvertReadOnlySequence)-1].SequenceProcess =
                        NfcCxRFInterfaceTagConvertReadOnlySeqComplete;

                    NFCCX_INIT_SEQUENCE(rfInterface, TagConvertReadOnlySequence);
                    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
                }
                else {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
            }
            break;

        case LIBNFC_TARGET_TRANSCEIVE:
            {
                if (rfInterface->pLibNfcContext->bIsTagPresent) {
                    NFCCX_CX_BEGIN_SEQUENCE_MAP(TargetTransceiveSequence)
                        RF_INTERFACE_TARGET_TRANSCEIVE_SEQUENCE
                    NFCCX_CX_END_SEQUENCE_MAP()

                    TargetTransceiveSequence[ARRAYSIZE(TargetTransceiveSequence)-1].SequenceProcess =
                        NfcCxRFInterfaceTargetTransceiveSeqComplete;

                    NFCCX_INIT_SEQUENCE(rfInterface, TargetTransceiveSequence);
                    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
                }
                else {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
            }
            break;

        case LIBNFC_TARGET_PRESENCE_CHECK:
            {
                if (rfInterface->pLibNfcContext->bIsTagPresent) {
                    NFCCX_CX_BEGIN_SEQUENCE_MAP(TargetPresenceCheckSequence)
                        RF_INTERFACE_TARGET_PRESENCE_CHECK_SEQUENCE
                    NFCCX_CX_END_SEQUENCE_MAP()

                    TargetPresenceCheckSequence[ARRAYSIZE(TargetPresenceCheckSequence)-1].SequenceProcess =
                        NfcCxRFInterfaceTargetPresenceCheckSeqComplete;

                    NFCCX_INIT_SEQUENCE(rfInterface, TargetPresenceCheckSequence);
                    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
                }
                else {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
            }
            break;

        case LIBNFC_SNEP_CLIENT_PUT:
            {
                if (NfcCxRFInterfaceGetLLCPInterface(rfInterface)->eLinkStatus == phFriNfc_LlcpMac_eLinkActivated) {
                    NFCCX_CX_BEGIN_SEQUENCE_MAP(SNEPClientPutSequence)
                        SNEP_INTERFACE_CLIENT_PUT_SEQUENCE
                    NFCCX_CX_END_SEQUENCE_MAP()

                    SNEPClientPutSequence[ARRAYSIZE(SNEPClientPutSequence)-1].SequenceProcess =
                        NfcCxRFInterfaceSNEPClientPutSeqComplete;

                    NFCCX_INIT_SEQUENCE(rfInterface, SNEPClientPutSequence);
                    status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
                }
                else {
                    status = STATUS_INVALID_DEVICE_STATE;
                }
            }
            break;

        case LIBNFC_TARGET_SEND:
            {
                static NFCCX_CX_SEQUENCE HCESendSequence[] = {
                    { NfcCxRFInterfaceRemoteDevSend, NULL },
                    { NULL, NfcCxRFInterfaceRemoteDevSendSeqComplete }
                };

                NFCCX_INIT_SEQUENCE(rfInterface, HCESendSequence);
                status = NfcCxSequenceHandler(rfInterface, rfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
NfcCxRFInterfaceStateSEEvent(
    _In_ PNFCCX_RF_INTERFACE RfInterface,
    _In_ UINT32 eLibNfcMessage,
    _In_opt_ VOID* Param2,
    _In_opt_ VOID* Param3
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (eLibNfcMessage)
    {
    case LIBNFC_SE_SET_MODE:
        {
            NFCCX_CX_BEGIN_SEQUENCE_MAP(SESetModeSequence)
                RF_INTERFACE_SE_SET_MODE_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            SESetModeSequence[ARRAYSIZE(SESetModeSequence)-1].SequenceProcess =
                NfcCxRFInterfaceSESetModeSeqComplete;

            NFCCX_INIT_SEQUENCE(RfInterface, SESetModeSequence);
            status = NfcCxSequenceHandler(RfInterface, RfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        break;

    case LIBNFC_EMEBEDDED_SE_TRANSCEIVE:
        {
            NFCCX_CX_BEGIN_SEQUENCE_MAP(EmbeddedSEAPDUModeSequence)
                RF_INTERFACE_EMBEDDEDSE_TRANSCEIVE_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            EmbeddedSEAPDUModeSequence[ARRAYSIZE(EmbeddedSEAPDUModeSequence) - 1].SequenceProcess =
                NfcCxRFInterfaceESETransceiveSeqComplete;

            NFCCX_INIT_SEQUENCE(RfInterface, EmbeddedSEAPDUModeSequence);
            status = NfcCxSequenceHandler(RfInterface, RfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        break;

    case LIBNFC_EMBEDDED_SE_GET_ATR_STRING:
        {
            NFCCX_CX_BEGIN_SEQUENCE_MAP(EmbeddedSEGetATRStringSequence)
                RF_INTERFACE_EMBEDDEDSE_GET_ATR_STRING_SEQUENCE
            NFCCX_CX_END_SEQUENCE_MAP()

            EmbeddedSEGetATRStringSequence[ARRAYSIZE(EmbeddedSEGetATRStringSequence) - 1].SequenceProcess =
                NfcCxRFInterfaceESEGetATRStringSeqComplete;

            NFCCX_INIT_SEQUENCE(RfInterface, EmbeddedSEGetATRStringSequence);
            status = NfcCxSequenceHandler(RfInterface, RfInterface->pSeqHandler, STATUS_SUCCESS, Param2, Param3);
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

BOOLEAN
NfcCxRFInterfaceConnChkDiscMode(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    )
{
    return NfcCxPowerShouldEnableDiscovery(StateInterface->FdoContext->RFInterface->RFPowerState);
}

BOOLEAN
NfcCxRFInterfaceConnChkDeviceType(
    _In_ PNFCCX_STATE_INTERFACE StateInterface
    )
{
    PNFCCX_RF_INTERFACE rfInterface = StateInterface->FdoContext->RFInterface;
    return rfInterface->pLibNfcContext->bIsTagPresent;
}
