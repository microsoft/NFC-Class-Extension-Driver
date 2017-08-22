/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_Init.tmh"

static NFCSTATUS phNciNfc_Init(void *pContext);
static NFCSTATUS phNciNfc_ProcessInitRsp(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_ProcessInitRspNci1x(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_ProcessInitRspNci2x(void *pContext, NFCSTATUS wStatus);
static void phNciNfc_DelayForCreditNtfCb(void* pContext, uint8_t bCredits, NFCSTATUS status);
static NFCSTATUS phNciNfc_CompleteInitSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_DelayForResetNtfProc(void* pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_DelayForResetNtfInit(void* pContext);
static NFCSTATUS phNciNfc_DelayForResetNtf(void* pContext);

static NFCSTATUS phNciNfc_InitReset(void *pContext);
static NFCSTATUS phNciNfc_SendReset(void *pContext);

// CORE_RESET_NTF is part of the Reset sequences, but only in NCI2x, so the next element in
// the sequence *must* be phNciNfc_DelayForResetNtf. NCI1x implementation of phNciNfc_ProcessResetRsp
// skips the next step in the sequence assuming that it's phNciNfc_DelayForResetNtf.
static NFCSTATUS phNciNfc_ProcessResetRsp(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_CompleteReleaseSequence(void *pContext, NFCSTATUS wStatus);
static NFCSTATUS phNciNfc_CompleteNfccResetSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_ResetNtfCb(void* pContext, void* pInfo, NFCSTATUS status);

static NFCSTATUS phNciNfc_RegAllNtfs(void* pContext);

/*Global Varibales for Init Sequence Handler*/
phNciNfc_SequenceP_t gphNciNfc_InitSequence[] = {
    {&phNciNfc_DelayForResetNtfInit, &phNciNfc_DelayForResetNtfProc},
    {&phNciNfc_InitReset, &phNciNfc_ProcessResetRsp},
    {&phNciNfc_DelayForResetNtf, &phNciNfc_DelayForResetNtfProc}, // Skipped in NCI1x
    {&phNciNfc_Init, &phNciNfc_ProcessInitRsp},
    {NULL, &phNciNfc_CompleteInitSequence}
};

/*Global Varibales for Release Sequence Handler*/
phNciNfc_SequenceP_t gphNciNfc_ReleaseSequence[] = {
    {&phNciNfc_SendReset, &phNciNfc_ProcessResetRsp},
    {&phNciNfc_DelayForResetNtf, &phNciNfc_DelayForResetNtfProc}, // Skipped in NCI1x
    {NULL, &phNciNfc_CompleteReleaseSequence}
};

/*Global Varibales for Nfcc reset sequence*/
phNciNfc_SequenceP_t gphNciNfc_NfccResetSequence[] = {
    {&phNciNfc_SendReset, &phNciNfc_ProcessResetRsp},
    {&phNciNfc_DelayForResetNtf, &phNciNfc_DelayForResetNtfProc}, // Skipped in NCI1x
    {NULL, &phNciNfc_CompleteNfccResetSequence}
};

/** NCI2.x Core Reset notification min length
    NCI2x Specification Table 5: Control Messages to Reset the NFCC - CORE_RESET_NTF*/
#define PHNCINFC_CORE_RESET_NTF_MIN_LEN_2x       (sizeof(uint8_t) + /* Reset Trigger */\
                                                  sizeof(uint8_t) + /* Configuration Status */\
                                                  sizeof(uint8_t) + /* NCI Version */\
                                                  sizeof(uint8_t) + /* ManufacturerId*/\
                                                  sizeof(uint8_t)   /* Manufacturer Specific Information Length */)

/** Core Reset Rsp min length NCI 1x
    NCI1.x Specification Table8: Control Messages to Initialize the NFCC - CORE_INIT_RSP*/
#define PH_NCINFC_MIN_CORE_INIT_RSP_LEN_1x         (sizeof(uint8_t) +  /* Status */\
                                                    sizeof(phNciNfc_sCoreNfccFeatures_t) + /* NfccFeatures */\
                                                    sizeof(uint8_t) +  /* NoOfRfIfSuprt */\
                                                    sizeof(uint8_t) +  /* MaxLogicalCon */\
                                                    sizeof(uint16_t) + /* RoutingTableSize */\
                                                    sizeof(uint8_t) +  /* CntrlPktPayloadLen */\
                                                    sizeof(uint16_t) + /* MaxSizeLarge */\
                                                    sizeof(uint8_t) +  /* ManufacturerId */\
                                                    sizeof(uint32_t)   /* Manufacturer Specific information */)

/** Core Reset Rsp min length NCI 2x
    NCI2.x Specification Table8: Control Messages to Initialize the NFCC - CORE_INIT_RSP*/
#define PH_NCINFC_MIN_CORE_INIT_RSP_LEN_2x         (sizeof(uint8_t) +  /* Status */\
                                                    sizeof(phNciNfc_sCoreNfccFeatures_t) + /* NfccFeatures */\
                                                    sizeof(uint8_t) +  /* MaxLogicalCon */\
                                                    sizeof(uint16_t) + /* RoutingTableSize */\
                                                    sizeof(uint8_t) +  /* CntrlPktPayloadLen*/\
                                                    sizeof(uint8_t) +  /* DataHCIPktPayloadLen */\
                                                    sizeof(uint8_t) +  /* DataHCINumCredits */\
                                                    sizeof(uint16_t) + /* MaxNFCVFrameSize */\
                                                    sizeof(uint8_t)    /* NoOfRfIfSuprt */)

/** Core Reset notification timeout */
#define PHNCINFC_CORE_RESET_NTF_TIMEOUT_MS        (30)

static NFCSTATUS phNciNfc_Init(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
    TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
    TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
    TxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreInitCmdOid;

    if (PH_NCINFC_VERSION_IS_2x(PHNCINFC_GETNCICONTEXT()))
    {
        TxInfo.Buff = (uint8_t *)phOsalNfc_GetMemory(2);
        TxInfo.wLen = 2;
        phOsalNfc_SetMemory(TxInfo.Buff, 0x00, TxInfo.wLen);
    }
    else
    {
        TxInfo.Buff = (uint8_t *)&pNciContext->tInitInfo.bExtension;
        TxInfo.wLen = 0;
    }

    wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext),
                                    &TxInfo,
                                    &(pNciContext->RspBuffInfo),
                                    PHNCINFC_NCI_INIT_RSP_TIMEOUT,
                                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                                    pContext);
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessInitRsp(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciContext)
    {
        if (PH_NCINFC_VERSION_IS_1x(pNciContext))
        {
            wStatus = phNciNfc_ProcessInitRspNci1x(pContext, Status);
        }
        else if (PH_NCINFC_VERSION_IS_2x(pNciContext))
        {
            wStatus = phNciNfc_ProcessInitRspNci2x(pContext, Status);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessInitRspNci2x(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    phNciNfc_sInitRspParams_t *pInitRsp = NULL;
    pphNciNfc_Context_t pNciContext = pContext;
    phNciNfc_TlvUtilInfo_t tTlvInfo;
    uint8_t Offset = 0;
    uint8_t bCount = 0;
    uint8_t bType = 0;
    uint8_t bLen = 0;
    uint8_t *pValue = NULL;
    uint16_t wTemp = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciContext)
    {
        pInitRsp = &(pNciContext->InitRspParams);
        if (pNciContext->RspBuffInfo.wLen >= PH_NCINFC_MIN_CORE_INIT_RSP_LEN_2x)
        {
            if (pNciContext->RspBuffInfo.pBuff[Offset++] == PH_NCINFC_STATUS_OK)
            {
                wStatus = NFCSTATUS_SUCCESS;

                /*NFCC Features*/
                pInitRsp->NfccFeatures.DiscConfSuprt = pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.RoutingType = pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.PwrOffState = pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.Byte3 = pNciContext->RspBuffInfo.pBuff[Offset++];

                /*Max No of Logical Connection supported*/
                pInitRsp->MaxLogicalCon = pNciContext->RspBuffInfo.pBuff[Offset++];

                /*Routing Table Size*/
                pInitRsp->RoutingTableSize = pNciContext->RspBuffInfo.pBuff[Offset++]; /*LSB*/
                wTemp = pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->RoutingTableSize = (pInitRsp->RoutingTableSize) | (wTemp << 8);

                /*Control Packet Payload Length*/
                pInitRsp->CntrlPktPayloadLen = pNciContext->RspBuffInfo.pBuff[Offset++];

                /*Data Packet Payload Length of the static HCI connection*/
                pInitRsp->DataHCIPktPayloadLen = pNciContext->RspBuffInfo.pBuff[Offset++];

                /*Number of Credits of the Static HCI Connection*/
                pInitRsp->DataHCINumCredits = pNciContext->RspBuffInfo.pBuff[Offset++];

                /*Max NFC-V RF Frame Size*/
                pInitRsp->MaxNFCVFrameSize = pNciContext->RspBuffInfo.pBuff[Offset++];
                wTemp = (uint16_t)pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->MaxNFCVFrameSize = (pInitRsp->MaxNFCVFrameSize) | (wTemp << 8);

                /*number of supported RF interfaces*/
                pInitRsp->NoOfRfIfSuprt = pNciContext->RspBuffInfo.pBuff[Offset++];

                if (pInitRsp->NoOfRfIfSuprt <= PH_NCINFC_CORE_MAX_SUP_RF_INTFS)
                {
                    /*Supported RF Interfaces
                      RfInterfaces in NCI2.0 are coded with x + 2 byte:
                       - 1 byte for Interface
                       - 1 byte for Number of extension
                       - x byte for Extension list
                      Note: We ignore extensions for now */
                    tTlvInfo.pBuffer = &(pNciContext->RspBuffInfo.pBuff[Offset]);
                    tTlvInfo.dwLength = pNciContext->RspBuffInfo.wLen - Offset;
                    tTlvInfo.sizeInfo.dwOffset = 0;
                    for (bCount = 0; bCount < pInitRsp->NoOfRfIfSuprt &&
                                     Offset < pNciContext->RspBuffInfo.wLen &&
                                     wStatus == NFCSTATUS_SUCCESS; bCount++)
                    {
                        /* Parse the buffer */
                        /* The function will return an error status in case of invalid TLV format */
                        wStatus = phNciNfc_TlvUtilsGetNxtTlv(&tTlvInfo, &bType, &bLen, &pValue);
                        Offset += bLen + 2;
                    }

                    if (wStatus == NFCSTATUS_SUCCESS && pInitRsp->DataHCIPktPayloadLen > 0)
                    {
                        /*Note: The HciContext will be created once back in phLibNfc_InitializeProcess*/
                        wStatus = phNciNfc_CreateConn(UNASSIGNED_DESTID, phNciNfc_e_NFCEE);
                        if (wStatus == NFCSTATUS_SUCCESS)
                        {
                            wStatus = phNciNfc_UpdateConnInfo(UNASSIGNED_DESTID, phNciNfc_e_NFCEE,
                                CONNHCITYPE_STATIC,
                                pInitRsp->DataHCINumCredits,
                                pInitRsp->DataHCIPktPayloadLen);

                            if (wStatus == NFCSTATUS_SUCCESS &&
                                pInitRsp->DataHCIPktPayloadLen > 0 &&
                                pInitRsp->DataHCINumCredits == 0)
                            {
                                wStatus = phNciNfc_RegForConnCredits(CONNHCITYPE_STATIC,
                                    &phNciNfc_DelayForCreditNtfCb,
                                    pNciContext, PHNCINFC_MIN_WAITCREDIT_TO);
                            }
                        }
                    }

                    if (wStatus == NFCSTATUS_SUCCESS)
                    {
                        wStatus = phNciNfc_CoreIfSetMaxCtrlPacketSize(&(pNciContext->NciCoreContext),
                            pInitRsp->CntrlPktPayloadLen);
                    }
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Invalid number of supported Rf interfaces");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessInitRspNci1x(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    phNciNfc_sInitRspParams_t *pInitRsp = NULL;
    pphNciNfc_Context_t pNciContext = pContext;
    uint8_t *pBuff;
    uint8_t Offset = 0;
    uint16_t wTemp = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        pInitRsp = &(pNciContext->InitRspParams);
        if (pNciContext->RspBuffInfo.wLen >= PH_NCINFC_MIN_CORE_INIT_RSP_LEN_1x)
        {
            if (pNciContext->RspBuffInfo.pBuff[Offset++] == PH_NCINFC_STATUS_OK)
            {
                wStatus = NFCSTATUS_SUCCESS;

                /*NFCC Features*/
                pInitRsp->NfccFeatures.DiscConfSuprt =  pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.RoutingType =  pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.PwrOffState =  pNciContext->RspBuffInfo.pBuff[Offset++];
                pInitRsp->NfccFeatures.Byte3 =  pNciContext->RspBuffInfo.pBuff[Offset++];

                /*number of supported RF interfaces*/
                pInitRsp->NoOfRfIfSuprt = pNciContext->RspBuffInfo.pBuff[Offset++];

                /* Check RspBuffInfo len is egal to the core reset response min length +
                   No Of Rf IfSuprt */
                if(pInitRsp->NoOfRfIfSuprt <= PH_NCINFC_CORE_MAX_SUP_RF_INTFS &&
                   pNciContext->RspBuffInfo.wLen == pInitRsp->NoOfRfIfSuprt +
                                                    PH_NCINFC_MIN_CORE_INIT_RSP_LEN_1x)
                {
                    /*Supported RF Interfaces */
                    phOsalNfc_MemCopy(pInitRsp->RfInterfaces, &(pNciContext->RspBuffInfo.pBuff[Offset]),
                                                                pInitRsp->NoOfRfIfSuprt);
                    Offset += pInitRsp->NoOfRfIfSuprt;

                    /*Max No of Logical Connection supported*/
                    pInitRsp->MaxLogicalCon = pNciContext->RspBuffInfo.pBuff[Offset++];

                    /*Routing Table Size*/
                    pInitRsp->RoutingTableSize = pNciContext->RspBuffInfo.pBuff[Offset++]; /*LSB*/
                    wTemp = pNciContext->RspBuffInfo.pBuff[Offset++];
                    pInitRsp->RoutingTableSize = (pInitRsp->RoutingTableSize) | (wTemp << 8);

                    /*Control Packet Payload Length*/
                    pInitRsp->CntrlPktPayloadLen = pNciContext->RspBuffInfo.pBuff[Offset++];

                    /*Max Size For Large parameter*/
                    pInitRsp->MaxSizeLarge = pNciContext->RspBuffInfo.pBuff[Offset++]; /*LSB*/
                    wTemp = (uint16_t)pNciContext->RspBuffInfo.pBuff[Offset++];
                    pInitRsp->MaxSizeLarge = (pInitRsp->MaxSizeLarge) | (wTemp << 8);

                    /*Manufacturer ID*/
                    pInitRsp->ManufacturerId = pNciContext->RspBuffInfo.pBuff[Offset++];
                    if(pInitRsp->ManufacturerId != 0x00)
                    {
                        /* Before allocating a new buffer to hold Manufacturer information data,
                           check if a valid one is not already available. */
                        pBuff = pNciContext->InitRspParams.ManufacturerInfo.Buffer;
                        if (pBuff == NULL)
                        {
                            pBuff = (uint8_t *)phOsalNfc_GetMemory(PHNCINFC_CORE_MANUF_INFO_LEN_NCI1x);
                            if (pBuff != NULL)
                            {
                                pNciContext->InitRspParams.ManufacturerInfo.Buffer = pBuff;
                                pNciContext->InitRspParams.ManufacturerInfo.Length =
                                                                    PHNCINFC_CORE_MANUF_INFO_LEN_NCI1x;
                            }
                            else
                            {
                                wStatus = NFCSTATUS_FAILED;
                            }
                        }
                        else if (pNciContext->InitRspParams.ManufacturerInfo.Length !=
                                                                    PHNCINFC_CORE_MANUF_INFO_LEN_NCI1x)
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }

                        if (wStatus == NFCSTATUS_SUCCESS)
                        {
                            /*Decided by Manufacturer*/
                            if (NULL == memcpy(pBuff, &pNciContext->RspBuffInfo.pBuff[Offset],
                                               PHNCINFC_CORE_MANUF_INFO_LEN_NCI1x))
                            {
                                wStatus = NFCSTATUS_FAILED;
                            }
                        }
                    }

                    wStatus = phNciNfc_CoreIfSetMaxCtrlPacketSize(&(pNciContext->NciCoreContext),
                                                                     pInitRsp->CntrlPktPayloadLen);
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Invalid number of supported Rf interfaces");
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static void phNciNfc_DelayForCreditNtfCb(void* pContext, uint8_t bCredits, NFCSTATUS status)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;
    phNciNfc_TransactInfo_t tTranscInfo;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    if ((NFCSTATUS_SUCCESS != status) || (0 == bCredits))
    {
        /* setting generic WaitCreditTimeout event to enable processing of state machine even in failure case */
        wStatus = NFCSTATUS_CREDIT_TIMEOUT;
        PH_LOG_NCI_CRIT_STR("Credits Update from NciNfc_Init Module Failed");
        phNciNfc_Notify(pNciContext, wStatus, NULL);
    }
    else
    {
        if (NULL != pNciContext && PH_NCINFC_VERSION_IS_2x(pNciContext))
        {
            tTranscInfo.pContext = (void*)pNciContext;
            tTranscInfo.pbuffer = (void*)&pNciContext->ResetInfo.ResetTypeRsp;
            tTranscInfo.wLength = sizeof(pNciContext->ResetInfo.ResetTypeRsp);
            phNciNfc_Notify(pNciContext, wStatus, (void *)&tTranscInfo);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_STATE;
            phNciNfc_Notify(pNciContext, wStatus, NULL);
        }
    }

    PH_LOG_NCI_FUNC_EXIT();
}

static void phNciNfc_ResetNtfDelayCb(uint32_t dwTimerId, void *pContext)
{
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;
    /* Internal event is made failed in order to remain in same state */
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(dwTimerId);
    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciContext)
    {
        (void)phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);

        wStatus = phNciNfc_GenericSequence(pNciContext, NULL, wStatus);
        if (wStatus != NFCSTATUS_SUCCESS)
        {
            PH_LOG_NCI_INFO_STR("phNciNfc_GenericSequence returned error %d", wStatus);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

static NFCSTATUS phNciNfc_DelayForResetNtfProc(void* pContext, NFCSTATUS status)
{
    UNUSED(status);
    UNUSED(pContext);
    PH_LOG_NCI_FUNC_ENTRY();
    PH_LOG_NCI_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

static NFCSTATUS phNciNfc_DelayForResetNtfInit(void* pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL == pNciContext)
    {
        wStatus = NFCSTATUS_INVALID_STATE;
    }

    if (wStatus == NFCSTATUS_SUCCESS && 0 == pNciContext->tInitInfo.bSkipRegisterAllNtfs)
    {
        wStatus = phNciNfc_RegAllNtfs(pNciContext);
    }

    if (wStatus == NFCSTATUS_SUCCESS)
    {
        wStatus = phNciNfc_DelayForResetNtf(pContext);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_DelayForResetNtf(void* pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pNciContext)
    {
        PH_LOG_NCI_INFO_STR("Delay to receive Core Reset ntf %d", PHNCINFC_CORE_RESET_NTF_TIMEOUT_MS);

        if (PH_OSALNFC_TIMER_ID_INVALID != pNciContext->dwNtfTimerId)
        {
            PH_LOG_NCI_INFO_STR("Notifications timer already exists. Stopping it...");
            phOsalNfc_Timer_Stop(pNciContext->dwNtfTimerId);
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Creating notifications timer..");
            pNciContext->dwNtfTimerId = phOsalNfc_Timer_Create();
        }

        if (PH_OSALNFC_TIMER_ID_INVALID != pNciContext->dwNtfTimerId)
        {
            wStatus = phOsalNfc_Timer_Start(pNciContext->dwNtfTimerId,
                                            PHNCINFC_CORE_RESET_NTF_TIMEOUT_MS,
                                            &phNciNfc_ResetNtfDelayCb,
                                            (void *)pNciContext);
            if (NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = NFCSTATUS_PENDING;
            }
            else
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
            PH_LOG_NCI_CRIT_STR("Failed to create notification timer instance.");
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_STATE;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_InitReset(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreResetCmdOid;
        TxInfo.Buff = (uint8_t *)&pNciContext->ResetInfo.ResetTypeReq;
        TxInfo.wLen = 1;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext),
                                    &TxInfo,
                                    &(pNciContext->RspBuffInfo),
                                    PHNCINFC_NCI_CMD_RSP_TIMEOUT,
                                    (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence,
                                    pContext);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_ProcessResetRsp(void *pContext, NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    pphNciNfc_Context_t pNciContext = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        if(NFCSTATUS_RESPONSE_TIMEOUT != Status)
        {
            if (pNciContext->RspBuffInfo.wLen == PHNCINFC_CORE_RESET_RSP_LEN_NCI1x)
            {
                /*Check Status Byte*/
                if (pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
                {
                    /* Nfcc supported Nci version */
                    pNciContext->ResetInfo.NciVer = pNciContext->RspBuffInfo.pBuff[1];

                    if(PH_NCINFC_VERSION_IS_1x(pNciContext))
                    {
                        /* Update Reset type */
                        if (pNciContext->RspBuffInfo.pBuff[2] == phNciNfc_ResetType_KeepConfig)
                        {
                            PH_LOG_NCI_INFO_STR("Nfcc reseted to 'phNciNfc_ResetType_KeepConfig'");
                            pNciContext->ResetInfo.ResetTypeRsp = phNciNfc_ResetType_KeepConfig;
                        }
                        else
                        {
                            PH_LOG_NCI_INFO_STR("Nfcc reseted to 'phNciNfc_ResetType_ResetConfig'");
                            pNciContext->ResetInfo.ResetTypeRsp = phNciNfc_ResetType_ResetConfig;
                        }

                        wStatus = NFCSTATUS_SUCCESS;

                        // Next sequence item is NCI2x specific, so we skip it here.
                        phNciNfc_SkipSequenceSeq(pNciContext, pNciContext->pSeqHandler, 1);
                    }
                    else
                    {
                        PH_LOG_NCI_INFO_STR("Unsupported NCI version 0x%02x", pNciContext->ResetInfo.NciVer);
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else if (pNciContext->RspBuffInfo.wLen == PHNCINFC_CORE_RESET_RSP_LEN_NCI2x)
            {
                /*Check Status Byte*/
                if (pNciContext->RspBuffInfo.pBuff[0] == PH_NCINFC_STATUS_OK)
                {
                    if (PH_NCINFC_VERSION_IS_2x(pNciContext))
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                    }
                    else
                    {
                        PH_LOG_NCI_INFO_STR("Unsupported NCI version 0x%02x", pNciContext->ResetInfo.NciVer);
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
        }else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CompleteInitSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    phNciNfc_TransactInfo_t tTranscInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        if (PH_NCINFC_VERSION_IS_1x(pNciCtx))
        {
            tTranscInfo.pContext = (void*)pNciCtx;
            tTranscInfo.pbuffer = (void*)&pNciCtx->ResetInfo.ResetTypeRsp;
            tTranscInfo.wLength = sizeof(pNciCtx->ResetInfo.ResetTypeRsp);
            phNciNfc_Notify(pNciCtx, wStatus, (void *)&tTranscInfo);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_SendReset(void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t TxInfo;
    pphNciNfc_Context_t pNciContext = (pphNciNfc_Context_t)pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciContext)
    {
        phOsalNfc_SetMemory(&TxInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        TxInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        TxInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        TxInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = phNciNfc_e_NciCoreResetCmdOid;
        TxInfo.Buff = (uint8_t *)&pNciContext->ResetInfo.ResetTypeReq;
        TxInfo.wLen = 1;
        wStatus = phNciNfc_CoreIfTxRx(&(pNciContext->NciCoreContext), &TxInfo,
            &(pNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, pContext);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CompleteReleaseSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    pphNciNfc_IfNotificationCb_t pUpperLayerCb = NULL;
    void *pUpperLayerCtx = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        pUpperLayerCb = pNciCtx->IfNtf;
        pUpperLayerCtx = pNciCtx->IfNtfCtx;
        pNciCtx->IfNtf = NULL;
        pNciCtx->IfNtfCtx = NULL;

        wStatus = phNciNfc_ReleaseNciHandle();

        if(NULL != pUpperLayerCb)
        {
            PH_LOG_NCI_INFO_STR("Invoking upper layer call back function");
            pUpperLayerCb(pUpperLayerCtx, wStatus, NULL);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CompleteNfccResetSequence(void *pContext, NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        phNciNfc_Notify(pNciCtx, wStatus,NULL);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_ResetNtfCb(void*     pContext,
                    void *pInfo,
                    NFCSTATUS status)
{
    pphNciNfc_Context_t pNciCtx = (pphNciNfc_Context_t )pContext;
    pphNciNfc_TransactInfo_t pTransInfo = pInfo;
    uint8_t wDataLen;
    uint8_t *pBuff;
    phNciNfc_ResetTrigger_t resetTrigger;
    NFCSTATUS wStatus;

    wStatus  = status;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pNciCtx) && (NULL != pTransInfo))
    {
        /* Reset notification received, take necessary action */
        PH_LOG_NCI_INFO_STR("Received RESET notification from NFCC");

        if (pNciCtx->dwNtfTimerId != 0)
        {
            (void)phOsalNfc_Timer_Stop(pNciCtx->dwNtfTimerId);
        }

        /* Reset Sender statemachine */
        (void)phNciNfc_CoreResetSenderStateMachine(&pNciCtx->NciCoreContext);
        (void)phTmlNfc_WriteAbort(pNciCtx->NciCoreContext.pHwRef);

        if (NULL != pNciCtx->tRegListInfo.pResetNtfCb)
        {
            pNciCtx->tRegListInfo.pResetNtfCb(pNciCtx->tRegListInfo.ResetNtfCtxt,
                eNciNfc_NciResetNtf, pInfo, NFCSTATUS_SUCCESS);
        }

        /* Nci Version is only available in Nci2.0 CORE_RESET_NTF frame.
           Furthermore Nci1.x CORE_RESET_NTF frame is only 2 bytes long
           (Reason code(1 byte) and Configuration status(1 byte).
           We are assuming here that a CORE_RESET_NTF with a size longer
           or equal to PHNCINFC_CORE_RESET_NTF_MIN_LEN_2x(5) is a NCI2x frame.*/
        if (pTransInfo->wLength >= PHNCINFC_CORE_RESET_NTF_MIN_LEN_2x)
        {
            /* Nfcc supported Nci version */
            pNciCtx->ResetInfo.NciVer = pTransInfo->pbuffer[2];
            if (PH_NCINFC_VERSION_IS_2x(pNciCtx))
            {
                /* Update Reset type */
                if (pTransInfo->pbuffer[1] == phNciNfc_ResetType_KeepConfig)
                {
                    PH_LOG_NCI_INFO_STR("Nfcc reseted to 'phNciNfc_ResetType_KeepConfig'");
                    pNciCtx->ResetInfo.ResetTypeRsp = phNciNfc_ResetType_KeepConfig;
                }
                else
                {
                    PH_LOG_NCI_INFO_STR("Nfcc reseted to 'phNciNfc_ResetType_ResetConfig'");
                    pNciCtx->ResetInfo.ResetTypeRsp = phNciNfc_ResetType_ResetConfig;
                }

                /*Manufacturer ID*/
                pNciCtx->InitRspParams.ManufacturerId = pTransInfo->pbuffer[3];
                if (pNciCtx->InitRspParams.ManufacturerId != 0x00)
                {
                    wDataLen = pTransInfo->pbuffer[4];
                    if (wDataLen == pTransInfo->wLength - PHNCINFC_CORE_RESET_NTF_MIN_LEN_2x)
                    {
                        pBuff = pNciCtx->InitRspParams.ManufacturerInfo.Buffer;
                        if (pBuff == NULL)
                        {
                            pBuff = (uint8_t *)phOsalNfc_GetMemory(wDataLen);
                            if (pBuff != NULL)
                            {
                                pNciCtx->InitRspParams.ManufacturerInfo.Buffer = pBuff;
                                pNciCtx->InitRspParams.ManufacturerInfo.Length = wDataLen;
                            }
                            else
                            {
                                wStatus = NFCSTATUS_FAILED;
                            }
                        }

                        if (wStatus == NFCSTATUS_SUCCESS &&
                            pNciCtx->InitRspParams.ManufacturerInfo.Length == wDataLen)
                        {
                            if (NULL == memcpy(pBuff,
                                                &pTransInfo->pbuffer[PHNCINFC_CORE_RESET_NTF_MIN_LEN_2x],
                                                wDataLen))
                            {
                                wStatus = NFCSTATUS_FAILED;
                            }
                        }
                        else
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }
                    }
                }

                resetTrigger = pTransInfo->pbuffer[0];
                if (NFCSTATUS_SUCCESS == wStatus
                    && (resetTrigger == PH_NCINFC_RESETTRIGGER_NFCC_POWER_ON ||
                        resetTrigger == PH_NCINFC_RESETTRIGGER_CMD_RESET_RECEIVED))
                {
                    wStatus = phNciNfc_GenericSequence(pNciCtx, pInfo, status);
                }
                else
                {
                    PH_LOG_NCI_WARN_STR("Unexpected Reset Trigger: %!phNciNfc_ResetTrigger_t!", resetTrigger);
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                /* For now only Nci2x is following this format for CORE_RESET_NTF*/
                PH_LOG_NCI_INFO_STR("Unsupported NCI version 0x%02x", pNciCtx->ResetInfo.NciVer);
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_RegAllNtfs(void*     pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t pCtx = pContext;
    phNciNfc_sCoreHeaderInfo_t tHeaderInfo;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        /*Register Notification function for Rf field info*/
        tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
        tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfFieldInfoNtfOid;
        tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
        wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                            &(tHeaderInfo),
                                            &phNciNfc_RfFieldInfoNtfHandler,
                                            pContext
                                           );
        /*Register Notification function for Rf-Nfcee Action info*/
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Rf-Nfcee Action Notification");
            tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfNfceeActionNtfOid;
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                        &(tHeaderInfo),
                                        &phNciNfc_NfceeActionNtfHandler,
                                        pContext
                                       );
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /*Register Notification function for Rf Deactivate info*/
            tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfDeactivateNtfOid;
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ProcessDeActvNtf,
                                                pContext
                                               );
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Interface Activated Notification");
            /* Register for Interface Activation Notification */
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfIntfActivatedNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ProcessActvNtf,
                                                pContext
                                               );
        }
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Discover Notification");
            /* Register for Interface Activation Notification */
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfDiscoverNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ProcessDiscNtf,
                                                pContext
                                               );
        }
        /* Register for Interface error notification */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Interface Error Notification");
            /* Register for Interface Activation Notification */
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreInterfaceErrNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ProcessIntfErrNtf,
                                                pContext
                                               );
        }
        /* Register for Generic error notification */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Generic Error Notification");
            /* Register for Interface Activation Notification */
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreGenericErrNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ProcessGenericErrNtf,
                                                pContext
                                               );
        }
        /* Register for Reset notification */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Reset Notification");
            /* Register for Interface Activation Notification */
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
            tHeaderInfo.Opcode_ID.OidType.NciCoreNtfOid = phNciNfc_e_NciCoreResetNtfOid;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                                &(tHeaderInfo),
                                                &phNciNfc_ResetNtfCb,
                                                pContext
                                               );
        }
        /*Register for Rf Nfcee Discovery Request Notification */
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_NCI_INFO_STR("Registering for Rf Nfcee Discovery Request Notification");
            tHeaderInfo.Group_ID = phNciNfc_e_CoreRfMgtGid;
            tHeaderInfo.Opcode_ID.OidType.RfMgtNtfOid = phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid;
            tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
            wStatus = phNciNfc_CoreIfRegRspNtf(&(pCtx->NciCoreContext),
                                        &(tHeaderInfo),
                                        &phNciNfc_NfceeDiscReqNtfHandler,
                                        pContext
                                       );
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
