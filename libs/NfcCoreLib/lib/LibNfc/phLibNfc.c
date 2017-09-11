/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Deactivate.h"
#include "phLibNfc_Type1Tag.h"
#include "phLibNfc_Iso15693.h"

#include "phLibNfc.tmh"

#define PHLIBNFC_DATE_RATE_FACTOR       (0x40U) /**<Data rate conversion factor*/

#define MIFARE_READ16_NUMBLOCKS         (0x01U) /**< Mifare UL & standard read16 num of blocks */
#define MIFARE_WRITE_ACK                (0x0AU) /**< Success acknowledge state for mifare write command */
#define MIFARE_BLKPERSECTOR             (0x04U) /**< Number of blocks per sector for Mifare Classic Tag*/

#define PHLIBNFC_4ARATS_START_BYTE      (0xE0U) /**< Start Byte in RATS request */
#define PHLIBNFC_4ARATS_FSD_VAL1        (0x08U) /**< Preferred FSDI value1 for RATS cmd request */
#define PHLIBNFC_4ARATS_FSD_VAL2        (0x0FU) /**< Preferred FSDI value2 for RATS cmd request */

#define PHLIBNFC_FELICA_REQRES_RESP_LEN (0x0BU) /**< Payload length for response of RequestResp command */

static NFCSTATUS phLibNfc_MifareULSendGetVersionCmd(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULProcessGetVersionResp(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULSendAuthenticateCmd(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULProcessAuthenticateResp(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULDeactivateCard(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULProcessDeactivateCard(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULConnectCard(void *pContext, NFCSTATUS status, void *pInfo);
static NFCSTATUS phLibNfc_MifareULProcessConnectCard(void *pContext, NFCSTATUS status, void *pInfo);

static NFCSTATUS phLibNfc_SendT3tPollCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_T3tCmdResp(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_T3tConnectComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_T3tChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo);

static NFCSTATUS phLibNfc_FelicaReqResCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_FelicaReqResResp(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_FelicaChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo);

static NFCSTATUS phLibNfc_SendIsoDepPresChkCmd(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_IsoDepPresChkCmdResp(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_IsoDepChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_IsoDepConnectComplete(void *pContext,NFCSTATUS status,void *pInfo);

static NFCSTATUS phLibNfc_SendRATSReq(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ProcessATSRsp(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ReActivateComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ReActivateMFCComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_ReActivateMFCComplete1(void *pContext,NFCSTATUS status,void *pInfo);

static NFCSTATUS phLibNfc_P2pActivateSeqComplete(void *pContext,NFCSTATUS status,void *pInfo);
static NFCSTATUS phLibNfc_RequestMoreInfo(void* pContext,phLibNfc_Event_t TrigEvent,NFCSTATUS wStatus);

static NFCSTATUS phLibNfc_MifareMap(phLibNfc_sTransceiveInfo_t* pTransceiveInfo, pphNciNfc_TransceiveInfo_t pMappedTranscvIf);
static NFCSTATUS phLibNfc_Iso15693Map(phLibNfc_sTransceiveInfo_t* pTransceiveInfo, pphNciNfc_TransceiveInfo_t pMappedTranscvIf);
static NFCSTATUS phLibNfc_JewelMap(phLibNfc_sTransceiveInfo_t* pTransceiveInfo, pphNciNfc_TransceiveInfo_t pMappedTranscvIf);

static NFCSTATUS phLibNfc_MapRemoteDevA(phLibNfc_sRemoteDevInformation_t *pLibNfcDeviceInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);
static NFCSTATUS phLibNfc_MapRemoteDevAJewel(phNfc_sJewelInfo_t *RemoteDevInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);
static NFCSTATUS phLibNfc_MapRemoteDevB(phNfc_sIso14443BInfo_t *RemoteDevInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);
static NFCSTATUS phLibNfc_MapRemoteDevFelica(phNfc_sFelicaInfo_t *RemoteDevInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);
static NFCSTATUS phLibNfc_MapRemoteDevNfcIp1(phNfc_sNfcIPInfo_t *pRemoteDevInfo, phNciNfc_NfcIPInfo_t *pNciRemoteDevInfo, phNciNfc_RfTechMode_t eRfTechMode);
static NFCSTATUS phLibNfc_MapRemoteDevIso15693(phNfc_sIso15693Info_t *pRemoteDevInfo, phNciNfc_Iso15693Info_t *pNciRemoteDevInfo);
static NFCSTATUS phLibNfc_MapRemoteDevKovio(phNfc_sKovioInfo_t *RemoteDevInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);

static NFCSTATUS phLibNfc_ChkAuthCmdMFC(void *pContext, phLibNfc_sTransceiveInfo_t* pTransceiveInfo);
static NFCSTATUS phLibNfc_InternalTransceive(phLibNfc_Handle hRemoteDevice, phLibNfc_sTransceiveInfo_t* psTransceiveInfo, pphLibNfc_TransceiveCallback_t pTransceive_RspCb, void* pContext);
static NFCSTATUS phLibNfc_InternalPresenceCheck(phLibNfc_Handle hRemoteDevice, pphLibNfc_RspCb_t pPresenceChk_RspCb, void* pContext);

static NFCSTATUS phLibNfc_GetRemoteDevInfo(phLibNfc_sRemoteDevInformation_t *pLibNfcRemdevHandle, phLibNfc_sRemoteDevInformation_t **pRemoteDevInfo);
static NFCSTATUS phLibNfc_ParseDiscActivatedRemDevInfo(phLibNfc_sRemoteDevInformation_t *pLibNfcDeviceInfo, pphNciNfc_RemoteDevInformation_t pNciDevInfo);
static void phLibNfc_ListenModeDeactvNtfHandler(pphLibNfc_LibContext_t pLibNfcHandle);

static void phLibNfc_ShutdownCb(void* pContext, NFCSTATUS status, void* pInfo);
static void phLibNfc_ChkPresence_Trcv_Cb(void *context, phLibNfc_Handle hRemoteDev, phNfc_sData_t *response, NFCSTATUS status);
static void phLibNfc_RemoteDev_Connect_Cb(void *pContext, NFCSTATUS status, void *pInfo);
static void phLibNfc_RemoteDev_ChkPresenceTimer_Cb(_In_ void* pLibContext);

static void phLibNfc_ClearNdefInfo(phLibNfc_NdefInfo_t *pNdefCtx);
static bool_t phLibNfc_CallBacksPending(pphLibNfc_LibContext_t pContext);
static void phLibNfc_Invoke_Pending_Cb(void * pContext, NFCSTATUS status);
static void phLibNfc_CalSectorAddress(uint8_t *Sector_Address);
static void phLibNfc_RemoteDev_ClearInfo(void );

static void phLibNfc_PrintRemoteDevInfo(phLibNfc_RemoteDevList_t *psRemoteDevList , uint32_t Num_Dev);
static void phLibNfc_PrintRemDevInfoNFCA(phNfc_sIso14443AInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoNFCB(phNfc_sIso14443BInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoJewel(phNfc_sJewelInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoNFCF(phNfc_sFelicaInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoNFCIP1(phNfc_sNfcIPInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoISO15693(phNfc_sIso15693Info_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoEPCGEN2(phNfc_sEpcGenInfo_t *phLibNfc_RemoteDevInfo);
static void phLibNfc_PrintRemDevInfoKovio(phNfc_sKovioInfo_t *phLibNfc_RemoteDevInfo);

/* Deactivate Mifare classic to sleep + again select same tag in case of check presence */
phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeq2[] = {
    {&phLibNfc_SendDeactSleepCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_SendSelectCmd1, &phLibNfc_SelectCmdResp},
    {NULL, &phLibNfc_ReActivateMFCComplete1}
};

/* Deactivate Mifare classic to sleep + again select same tag for reactivation sequence*/
static phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeq[] = {
    {&phLibNfc_SendDeactSleepCmd, &phLibNfc_ProcessDeactResp},
    {&phLibNfc_SendSelectCmd1, &phLibNfc_SelectCmdResp},
    {NULL, &phLibNfc_ReActivateMFCComplete}
};

/* Deactivate Mifare classic to sleep + again select same tag in case of check presence send only select command*/
phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeq2Select[] = {
    {&phLibNfc_SendSelectCmd1, &phLibNfc_SelectCmdResp},
    {NULL, &phLibNfc_ReActivateMFCComplete1}
};

/* Deactivate Mifare classic to sleep + again select same tag for reactivation sequence send only select command*/
static phLibNfc_Sequence_t gphLibNfc_ReActivate_MFCSeqSelect[] = {
    {&phLibNfc_SendSelectCmd1, &phLibNfc_SelectCmdResp},
    {NULL, &phLibNfc_ReActivateMFCComplete}
};

/* Distinguish Mifare Ultralight card between Ultralight, Ultralight C, and Ultralight EV1*/
static phLibNfc_Sequence_t gphLibNfc_DistinguishMifareUL[] = {
    {&phLibNfc_MifareULSendGetVersionCmd, &phLibNfc_MifareULProcessGetVersionResp},
    {&phLibNfc_MifareULDeactivateCard, &phLibNfc_MifareULProcessDeactivateCard},
    {&phLibNfc_MifareULConnectCard, &phLibNfc_MifareULProcessConnectCard},
    {&phLibNfc_MifareULSendAuthenticateCmd, &phLibNfc_MifareULProcessAuthenticateResp},
    {&phLibNfc_MifareULDeactivateCard, &phLibNfc_MifareULProcessDeactivateCard},
    {&phLibNfc_MifareULConnectCard, &phLibNfc_MifareULProcessConnectCard},
    {NULL, &phLibNfc_ReqInfoComplete}
};

/* Felica Request-Response command is sent to check presence of the card*/
phLibNfc_Sequence_t gphLibNfc_Felica_CheckPresSeq[] = {
    {&phLibNfc_FelicaReqResCmd, &phLibNfc_FelicaReqResResp},
    {NULL, &phLibNfc_FelicaChkPresComplete}
};

/* T3t poll command is sent to connect/check presence of the card*/
static phLibNfc_Sequence_t gphLibNfc_T3t_CheckPresSeq[] = {
    {&phLibNfc_SendT3tPollCmd, &phLibNfc_T3tCmdResp},
    {NULL, &phLibNfc_T3tChkPresComplete}
};

/* T3t poll command is sent to connect/check presence of the card*/
static phLibNfc_Sequence_t gphLibNfc_Felica_ConnectSeq[] = {
    {&phLibNfc_SendT3tPollCmd, &phLibNfc_T3tCmdResp},
    {NULL, &phLibNfc_T3tConnectComplete}
};

/* IsoDep poll command is sent to connect/check presence of the card*/
phLibNfc_Sequence_t gphLibNfc_IsoDep_CheckPresSeq[] = {
    {&phLibNfc_SendIsoDepPresChkCmd, &phLibNfc_IsoDepPresChkCmdResp},
    {NULL, &phLibNfc_IsoDepChkPresComplete}
};

/* IsoDep R-NAK command is sent to connect/check presence of the card*/
static phLibNfc_Sequence_t gphLibNfc_IsoDep_ConnectSeq[] = {
    {&phLibNfc_SendIsoDepPresChkCmd, &phLibNfc_IsoDepPresChkCmdResp},
    {NULL, &phLibNfc_IsoDepConnectComplete}
};

pphLibNfc_LibContext_t gpphLibNfc_Context = NULL;

inline pphLibNfc_LibContext_t phLibNfc_GetContext()
{
    return gpphLibNfc_Context;
}

static inline void phLibNfc_SetContext(_In_opt_ pphLibNfc_LibContext_t pLibContext)
{
    gpphLibNfc_Context = pLibContext;
}

NFCSTATUS phLibNfc_Mgt_Initialize(
                              _In_ void * pDriverHandle,
                              _In_ phLibNfc_InitType_t eInitType,
                              _In_ pphLibNfc_sConfig_t psConfig,
                              _In_ pphLibNfc_InitCallback_t pInitCb,
                              _In_ void *pContext
                              )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventInit;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    
    if((NULL == pDriverHandle)||(NULL == pInitCb))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NULL == pLibContext)
    {
        /* Initialize the Lib context */
        pLibContext=(pphLibNfc_LibContext_t)phOsalNfc_GetMemory(
            (uint32_t)sizeof(phLibNfc_LibContext_t));
        
        if(NULL == pLibContext)
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed to allocate memory, Insufficient Resources");
            wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            phLibNfc_SetContext(pLibContext);
            phOsalNfc_SetMemory((void *)pLibContext,0x00,(
                                    (uint32_t)sizeof(phLibNfc_LibContext_t)));

            pLibContext->tADDconfig.PollDevInfo.PollCfgInfo.DisableCardEmulation = 1;
            pLibContext->tADDconfig.NfcIP_Tgt_Disable = 1;
            pLibContext->sHwReference.pDriverHandle = pDriverHandle;
            pLibContext->eInitType = eInitType;
            phOsalNfc_MemCopy(&pLibContext->Config, psConfig, sizeof(*psConfig));

            wStatus = phLibNfc_InitStateMachine(pLibContext);

            if(wStatus==NFCSTATUS_SUCCESS)
            {
                wStatus = phLibNfc_StateHandler(pLibContext,
                                                TrigEvent,
                                                (void *)pDriverHandle,
                                                NULL,
                                                NULL);
                if(NFCSTATUS_PENDING == wStatus)
                {
                    pLibContext->CBInfo.pClientInitCb=pInitCb;
                    pLibContext->CBInfo.pClientInitCntx=pContext;
                    phLibNfc_Ndef_Init();
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("Mgt initialize not returned NFCSTATUS_PENDING");
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Initialisation of Libnfc state machine failed.Deinitialising stack");
            }

            if(NFCSTATUS_PENDING != wStatus)
            {
                if(pLibContext != NULL)
                {
                    phOsalNfc_FreeMemory(pLibContext);
                    phLibNfc_SetContext(NULL);
                    pLibContext=NULL;
                }
            }
        }
    }
    else if (NULL != pLibContext->CBInfo.pClientInitCb)
    {
        PH_LOG_LIBNFC_WARN_STR("Libnfc Stack busy - Init callback pending");
        wStatus = NFCSTATUS_BUSY;
    }
    else
    {
        PH_LOG_LIBNFC_WARN_STR("Stack already initialized");
        wStatus = NFCSTATUS_ALREADY_INITIALISED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Mgt_DeInitialize (
                            _In_ void *pDriverHandle,
                            _In_ pphLibNfc_RspCb_t pDeInitCb,
                            _In_ void *pContext
                            )
{
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReset;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pDriverHandle);

    if(NULL == pLibContext)
    {
        wStatus = NFCSTATUS_NOT_INITIALISED;
        PH_LOG_LIBNFC_CRIT_STR("Stack is not Initialised");
    }
    else
    {
        wStatus = phLibNfc_StatePrepareShutdown(pLibContext);
    }

    if(NFCSTATUS_SUCCESS == wStatus)
    {

        wStatus = phLibNfc_StateHandler(pLibContext,TrigEvent,
                                        NULL,NULL,NULL);

        if (wStatus == NFCSTATUS_PENDING)
        {
            PH_LOG_LIBNFC_INFO_STR("Shutdown in Progress...");
            pLibContext->CBInfo.pClientShutdownCb = pDeInitCb;
            pLibContext->CBInfo.pClientShtdwnCntx = pContext;
        }

        if((NULL == pDeInitCb) || (NFCSTATUS_PENDING != wStatus))
        {
            PH_LOG_LIBNFC_INFO_STR("No Callback delete LibNfc Context");

            (void)phNciNfc_Reset(pLibContext->sHwReference.pNciHandle,
                                 phNciNfc_NciReset_Mgt_Reset,NULL,NULL);

            phLibNfc_ClearLibContext(pLibContext);
            pLibContext = NULL;

            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_ShutdownCb(void *pContext,NFCSTATUS status,void *pInfo)
{
    pphLibNfc_RspCb_t           pClientCb=NULL;
    void                        *pUpperLayerContext=NULL;
    pphLibNfc_LibContext_t      pLibContext=(pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t            TrigEvent = phLibNfc_EventReqCompleted;
    NFCSTATUS                   wStatus=NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS != status)
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer Reset Failed");
    }

    if((pLibContext == phLibNfc_GetContext()) && (NULL != pLibContext))
    {
        pClientCb = pLibContext->CBInfo.pClientShutdownCb;
        pUpperLayerContext = pLibContext->CBInfo.pClientShtdwnCntx;

        wStatus = phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            phLibNfc_ClearLibContext(pLibContext);
            pLibContext = NULL;

            if(pClientCb!=NULL)
            {
                PH_LOG_LIBNFC_INFO_STR("Invoking callback function, wStatus = %!NFCSTATUS!", wStatus);
                (*pClientCb)(pUpperLayerContext,wStatus);
            }
        }else
        {
            PH_LOG_LIBNFC_CRIT_STR("State Machine has rejected the event!!!");
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Libnfc context is Null, Can't do anyhting");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_GetTechModeAndRfInterface(pphNciNfc_RemoteDevInformation_t pRemDevInfo, uint8_t *pTechMode, uint8_t *pRfInterface)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(NULL != pRemDevInfo && NULL != pTechMode && NULL != pRfInterface)
    {
        switch (pRemDevInfo->eRFTechMode)
        {
        case phNciNfc_NFCA_Poll:
        case phNciNfc_NFCA_Active_Poll:
        case phNciNfc_NFCB_Poll:
        case phNciNfc_NFCF_Poll:
        case phNciNfc_NFCF_Active_Poll:
        case phNciNfc_NFCISO15693_Poll:
        case phNciNfc_NFCA_Kovio_Poll:
            *pTechMode = 1; /*Poll Mode*/
            break;

        case phNciNfc_NFCA_Listen:
        case phNciNfc_NFCA_Active_Listen:
        case phNciNfc_NFCB_Listen:
        case phNciNfc_NFCF_Listen:
        case phNciNfc_NFCF_Active_Listen:
            {
                *pTechMode = 0; /*Listen Mode*/

                switch (pRemDevInfo->eRfIf)
                {
                case  phNciNfc_e_RfInterfacesNFCDEP_RF:
                    *pRfInterface = 0; /*NFC DEP interface*/
                    break;
                case  phNciNfc_e_RfInterfacesISODEP_RF:
                    *pRfInterface = 1; /*ISO DEP interface*/
                    break;
                default:
                    *pRfInterface = 1; /*Frame RF Interface or any other interface*/
                    break;
                }
            }
            break;

        default:
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            break;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    return wStatus;
}

NFCSTATUS phLibNfc_CheckPcdDevice(pphNciNfc_RemoteDevInformation_t pRemDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    if(NULL != pRemDevInfo)
    {
        if((pRemDevInfo->eRfIf == phNciNfc_e_RfInterfacesISODEP_RF ) \
        && (pRemDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol ) \
        && ((pRemDevInfo->eRFTechMode == phNciNfc_NFCA_Listen)|| \
            (pRemDevInfo->eRFTechMode == phNciNfc_NFCB_Listen)))
        {
            wStatus = NFCSTATUS_SUCCESS;
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
    return wStatus;
}

void phLibNfc_DeActvNtfRegister_Resp_Cb (
    void*      pContext,
    phNciNfc_NotificationType_t eNtfType,
    pphNciNfc_NotificationInfo_t pDevInfo,
    NFCSTATUS status
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL == pLibContext) || (NULL == pDevInfo))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Params received!!");
    }
    else
    {
        /* Clearing the stored 1st HCE Data buffer, buffer length
           and the first buffer stored flag during de-act */
        if(NULL != pLibContext->HCE_FirstBuffer.buffer)
        {
            phOsalNfc_FreeMemory(pLibContext->HCE_FirstBuffer.buffer);
            pLibContext->HCE_FirstBuffer.buffer = NULL;
            pLibContext->HCE_FirstBuffer.length = 0;
            pLibContext->HCE_FirstBuf = 0;
        }

        if (pLibContext != phLibNfc_GetContext())
        {
            PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned invalid LibNfc context");
        }
        else
        {
            if((NFCSTATUS_TARGET_LOST == status) && (eNciNfc_NciRfDeActvNtf == eNtfType))
            {
                TrigEvent = phLibNfc_EventDeactivated;

                /* If deactivation reason is Rf Link loss (taking this as priority),
                   update the state machine transition flag to 'phLibNfc_StateTransitionComplete'*/
                PH_LOG_LIBNFC_INFO_STR("eRfDeactvReason -> %!phNciNfc_DeActivateReason_t!", pDevInfo->tRfDeactvInfo.eRfDeactvReason);

                if((phNciNfc_e_RfLinkLoss == pDevInfo->tRfDeactvInfo.eRfDeactvReason) ||
                   (phNciNfc_e_EndPoint == pDevInfo->tRfDeactvInfo.eRfDeactvReason))
                {
                    pLibContext->StateContext.Flag = phLibNfc_StateTransitionComplete;
                    PH_LOG_LIBNFC_INFO_STR("State machine flag: %!phLibNfc_TransitionFlag!", pLibContext->StateContext.Flag);
                }

                /* Update the discovery disconnect mode based on deactivation type */
                if(phNciNfc_e_IdleMode == (pDevInfo->tRfDeactvInfo.eRfDeactvType))
                {
                    pLibContext->DiscDisconnMode = NFC_INTERNAL_STOP_DISCOVERY;
                }
                else if(phNciNfc_e_DiscMode == (pDevInfo->tRfDeactvInfo.eRfDeactvType))
                {
                    pLibContext->DiscDisconnMode = NFC_INTERNAL_CONTINUE_DISCOVERY;
                }
                else
                {
                    pLibContext->DiscDisconnMode = NFC_DISCONN_INVALID_RELEASE_TYPE;
                }

                /* If we are in Listen mode, deactivate notification should be handled */
                phLibNfc_ListenModeDeactvNtfHandler(pLibContext);
                wStatus = phLibNfc_StateHandler(pLibContext, TrigEvent,
                                        (void *)pLibContext->DiscDisconnMode,
                                        &(pDevInfo->tRfDeactvInfo), NULL);

                PH_LOG_LIBNFC_WARN_STR("State machine has returned %!NFCSTATUS!", wStatus);
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Irrelevant notification received!!");
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_ResetNtfHandler(
    void*      pContext,
    phNciNfc_NotificationType_t eNtfType,
    pphNciNfc_NotificationInfo_t pResetInfo,
    NFCSTATUS status)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReseted;

    UNUSED(eNtfType);
    UNUSED(status);
    UNUSED(pResetInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    PH_LOG_LIBNFC_CRIT_STR("LibNfc Reset Handler: Received Reset Ntf from NFCC");

    (void)phLibNfc_ClearAllDeferredEvents(pLibContext);
    pLibContext->StateContext.Flag = phLibNfc_StateTransitionComplete;

    (void)phTmlNfc_IoCtl(pLibContext->sHwReference.pDriverHandle, phTmlNfc_e_ResetDevice);
    phLibNfc_Invoke_Pending_Cb(pLibContext, NFCSTATUS_BOARD_COMMUNICATION_ERROR);

    (void)phLibNfc_StateHandler(pLibContext, TrigEvent, pResetInfo, NULL, NULL);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_GenericErrorHandler(
    void*      pContext,
    phNciNfc_NotificationType_t eNtfType,
    pphNciNfc_NotificationInfo_t pGenericErrInfo,
    NFCSTATUS status)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(eNtfType);
    UNUSED(status);
    if((NULL == pLibContext) || (pLibContext != phLibNfc_GetContext()) ||
        (NULL == pGenericErrInfo))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Params received!!");
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Generic error received: %!phNciNfc_GenericErrCode_t!",
                               pGenericErrInfo->tGenericErrInfo.eGenericErrInfo);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_NotificationRegister_Resp_Cb(
    void* pContext,
    phNciNfc_NotificationType_t eNtfType,
    pphNciNfc_NotificationInfo_t pDevInfo,
    NFCSTATUS status)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventInvalid;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    UNUSED(eNtfType);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    pLibContext->bCall_RegisterListner_Cb = PH_LIBNFC_INTERNAL_CALL_CB_TRUE;
    
    if(PHNFCSTATUS(status) == NFCSTATUS_FAILED)
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned NFCSTATUS_FAILED");
        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReDiscSeqWithDeactAndDisc);
        wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
    }
    else if(pLibContext != phLibNfc_GetContext() )
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned invalid LibNfc context");
        wStatus = NFCSTATUS_FAILED;
    }
    else
    {
        pLibContext->bDiscoverInProgress = 0;
        pLibContext->bPcdConnected = FALSE;
        phLibNfc_RemoteDev_ClearInfo();
        phLibNfc_ClearNdefInfo(&pLibContext->ndef_cntx);
        pLibContext->pInfo = pDevInfo->pDiscoveryInfo;
        /*This flag is checked in phLibNfc_ConnChkDevType function in order to move from
        discovery to discovered state in case of multiple discovery notification*/
        pLibContext->bTotalNumDev = pDevInfo->pDiscoveryInfo->dwNumberOfDevices;
        if(status == NFCSTATUS_MULTIPLE_TAGS)
        {
            PH_LOG_LIBNFC_INFO_STR("Lower layer has returned status NFCSTATUS_MULTIPLE_TAGS");
            TrigEvent = phLibNfc_EventDeviceDiscovered;
        }
        else if(status == NFCSTATUS_SINGLE_TAG_ACTIVATED)
        {
            PH_LOG_LIBNFC_INFO_STR("Lower layer has returned status NFCSTATUS_SINGLE_TAG_ACTIVATED");
            wStatus = phLibNfc_GetTechModeAndRfInterface(pDevInfo->pDiscoveryInfo->pRemDevList[0], &(pLibContext->bTechMode), &(pLibContext->bRfInterface));
            if(wStatus == NFCSTATUS_SUCCESS)
            {
                /*Check the listen phase and RF interface(ISO-DEP) and send  the PCD_EventPCDActivated */
                wStatus = phLibNfc_CheckPcdDevice(pDevInfo->pDiscoveryInfo->pRemDevList[0]);
                if(wStatus == NFCSTATUS_SUCCESS)
                {
                    TrigEvent = phLibNfc_EventPCDActivated;
                }
                else
                {
                    TrigEvent = phLibNfc_EventDeviceActivated;
                }
            }
        }
        else
        {
            TrigEvent = phLibNfc_EventInvalid;
        }

        /* Check whether Type1 and ISO15693 tags are discovered, If yes, then send retrieve
           the UID & Header ROM info of tags */
        wStatus = phLibNfc_RequestMoreInfo(pContext,TrigEvent,status);
        if(wStatus != NFCSTATUS_PENDING)
        {
            (void)phLibNfc_ProcessDevInfo(pLibContext, TrigEvent, pDevInfo->pDiscoveryInfo,status);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_ProcessDevInfo(void* pContext, phLibNfc_Event_t TrigEvent,
                             pphNciNfc_DeviceInfo_t pDiscoveryInfo,NFCSTATUS wStatus)
{
    NFCSTATUS wProcStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_NtfRegister_RspCb_t pClientCb =\
        pLibContext->CBInfo.pClientNtfRegRespCB;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NFCSTATUS_SUCCESS == wStatus) ||
        (NFCSTATUS_SINGLE_TAG_ACTIVATED == wStatus) ||
        (NFCSTATUS_MULTIPLE_TAGS == wStatus))
    {
        wProcStatus = phLibNfc_StateHandler(pLibContext, TrigEvent, pDiscoveryInfo, NULL, NULL);
        if(wProcStatus == NFCSTATUS_BUSY)
        {
            wStatus = NFCSTATUS_BUSY;
        }
        else if(wProcStatus != NFCSTATUS_SUCCESS)
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
        }

        PH_LOG_LIBNFC_INFO_STR("State machine has returned %!NFCSTATUS!", wStatus);

        if((pLibContext->bCall_RegisterListner_Cb == \
            PH_LIBNFC_INTERNAL_CALL_CB_TRUE) && (NFCSTATUS_FAILED != wStatus))
        {
            if((pLibContext->dev_cnt > 0) && (NFCSTATUS_SUCCESS == wStatus))
            {
                phLibNfc_PrintRemoteDevInfo(pLibContext->psRemoteDevList,\
                                            pLibContext->dev_cnt);
            }

            if ((NULL != pClientCb) && (pLibContext->dev_cnt > 0))
            {
                PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback function ");
                pClientCb((void*)pLibContext->CBInfo.pClientNtfRegRespCntx,
                          pLibContext->psRemoteDevList,
                          pLibContext->dev_cnt,
                          PHNFCSTATUS(wStatus));
            }
        }
        else if(wStatus == NFCSTATUS_FAILED)
        {
            PH_LOG_LIBNFC_CRIT_STR("NCI layer has returned NFCSTATUS_FAILED, Restart discovery ");
            PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReDiscSeqWithDeact);
            wProcStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
        }
        else
        {
            pLibContext->bCall_RegisterListner_Cb = PH_LIBNFC_INTERNAL_CALL_CB_TRUE;
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("NCI layer has returned NFCSTATUS_FAILED, Restart discovery ");
        PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_ReDiscSeqWithDeactAndDisc);
        wProcStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static NFCSTATUS phLibNfc_RequestMoreInfo(void* pContext,\
                                        phLibNfc_Event_t TrigEvent,\
                                        NFCSTATUS status)
{
    uint8_t bIndex = 0;
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_DeviceInfo_t pDeviceInfo = pLibContext->pInfo;
    if( (NFCSTATUS_SINGLE_TAG_ACTIVATED == wStatus) ||\
        (NFCSTATUS_MULTIPLE_TAGS == wStatus) )
    {
        if((pDeviceInfo->dwNumberOfDevices == 1) && (NULL != pDeviceInfo->pRemDevList[bIndex]))
        {
            if((phNciNfc_e_RfProtocolsT1tProtocol == pDeviceInfo->pRemDevList[bIndex]->eRFProtocol)&&\
               (0x00 == pDeviceInfo->pRemDevList[bIndex]->tRemoteDevInfo.Jewel_Info.UidLength) )
            {
                PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_T1tGetUidSequence);
                pLibContext->DiscTagTrigEvent = TrigEvent;
                /* Re-Using bLastCmdSent to store the index at which Jewel info is stored */
                pLibContext->bLastCmdSent = bIndex;
                wStatus = phLibNfc_SeqHandler(pContext,NFCSTATUS_SUCCESS,NULL);
            }
            else if ((phNciNfc_e_RfProtocolsT2tProtocol == pDeviceInfo->pRemDevList[bIndex]->eRFProtocol) && \
                (PHLIBNFC_MIFAREUL_SAK == pDeviceInfo->pRemDevList[bIndex]->tRemoteDevInfo.Iso14443A_Info.Sak))
            {
                PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_DistinguishMifareUL);
                pLibContext->DiscTagTrigEvent = TrigEvent;
                wStatus = phLibNfc_SeqHandler(pContext, NFCSTATUS_SUCCESS, NULL);
            }
            else if((phNciNfc_NFCISO15693_Poll == pDeviceInfo->pRemDevList[bIndex]->eRFTechMode)&&\
                   (0x00 == pDeviceInfo->pRemDevList[bIndex]->tRemoteDevInfo.Iso15693_Info.Afi))
            {
                PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_Iso15693GetSysInfoSeq);
                pLibContext->DiscTagTrigEvent = TrigEvent;
                wStatus = phLibNfc_SeqHandler(pContext,NFCSTATUS_SUCCESS,NULL);
            }
        }
    }
    return wStatus;
}

NFCSTATUS phLibNfc_StateDiscoveryEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3)
{
    NFCSTATUS wStatus=*((NFCSTATUS *)pParam3);
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PHNFC_UNUSED_VARIABLE(pParam1);
    PHNFC_UNUSED_VARIABLE(pParam2);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if (pLibContext->bDiscovery_Notify_Enable == 0x01)
    {
        pLibContext->bDiscovery_Notify_Enable = 0x00;
        pLibContext->bCall_RegisterListner_Cb = PH_LIBNFC_INTERNAL_CALL_CB_FALSE;

        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_DeactivateSequence);
        wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
    }

    phLibNfc_RemoteDev_ClearInfo();

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_StateDiscoveredEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3)
{
    uint8_t bIndex = 0;
    uint8_t bDeviceIndex1 = 0;
    uint8_t bDeviceIndex2 = 0;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_DeviceInfo_t pNciDevInfo = (pphNciNfc_DeviceInfo_t )pLibContext->pInfo;

    PHNFC_UNUSED_VARIABLE(pParam1);
    PHNFC_UNUSED_VARIABLE(pParam2);
    PHNFC_UNUSED_VARIABLE(pParam3);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (pLibContext->dev_cnt >= 1)
    {
        if (pLibContext->psRemoteDevInfo == NULL)
        {
            pLibContext->psRemoteDevInfo = (phLibNfc_sRemoteDevInformation_t *)phOsalNfc_GetMemory(
                (uint32_t)(sizeof(phLibNfc_sRemoteDevInformation_t) * (pLibContext->dev_cnt)));

            if(pLibContext->psRemoteDevInfo == NULL)
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to allocate memory, Insufficient Resources");
                wStatus= NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                phOsalNfc_SetMemory((void*)pLibContext->psRemoteDevInfo,0x00,
                                    (sizeof(phLibNfc_sRemoteDevInformation_t )*(pLibContext->dev_cnt)));
            }
         }
         else
         {
            /*Free previously allocated memory and allocate new memory*/
            phOsalNfc_FreeMemory(pLibContext->psRemoteDevInfo);
            pLibContext->Connected_handle = NULL;

            pLibContext->psRemoteDevInfo =(phLibNfc_sRemoteDevInformation_t *)phOsalNfc_GetMemory(\
                (uint32_t)(sizeof(phLibNfc_sRemoteDevInformation_t )*(pLibContext->dev_cnt)) );

            if(pLibContext->psRemoteDevInfo == NULL)
            {
                PH_LOG_LIBNFC_CRIT_STR("Failed to allocate memory, Insufficient Resources");
                wStatus= NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                phOsalNfc_SetMemory((void*)pLibContext->psRemoteDevInfo,0x00,
                                    (sizeof(phLibNfc_sRemoteDevInformation_t )*(pLibContext->dev_cnt)));
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            PH_LOG_LIBNFC_INFO_STR("pLibContext->dev_cnt = %u, pNciDevInfo->dwNumberOfDevices = %u",
                                   pLibContext->dev_cnt,
                                   pNciDevInfo->dwNumberOfDevices);
            for(bIndex = 0; bIndex < pLibContext->dev_cnt; bIndex++)
            {
                for(bDeviceIndex1 = 0; bDeviceIndex1 < pNciDevInfo->dwNumberOfDevices; bDeviceIndex1++)
                {
                    if((pphNciNfc_RemoteDevInformation_t)pLibContext->Disc_handle[bIndex] == pNciDevInfo->pRemDevList[bDeviceIndex1])
                    {
                        if(pNciDevInfo->dwNumberOfDevices == 1)
                        {
                            pLibContext->DummyConnect_handle = pNciDevInfo->pRemDevList[bDeviceIndex1];
                        }

                        pLibContext->psRemoteDevList[bDeviceIndex2].psRemoteDevInfo = &pLibContext->psRemoteDevInfo[bDeviceIndex2];
                        pLibContext->psRemoteDevList[bDeviceIndex2].hTargetDev = (phLibNfc_Handle)pLibContext->psRemoteDevList[bDeviceIndex2].psRemoteDevInfo;
                        pLibContext->Map_Handle[bDeviceIndex2].pLibNfc_RemoteDev_List = &pLibContext->psRemoteDevInfo[bDeviceIndex2];
                        pLibContext->Map_Handle[bDeviceIndex2].pNci_RemoteDev_List = pNciDevInfo->pRemDevList[bDeviceIndex1];

                        PH_LOG_LIBNFC_INFO_STR("bIndex = %u, bDeviceIndex1 = %u, bDeviceIndex2 = %u", bIndex, bDeviceIndex1, bDeviceIndex2);
                        wStatus = phLibNfc_ParseDiscActivatedRemDevInfo(pLibContext->psRemoteDevList[bDeviceIndex2].psRemoteDevInfo,
                                                                        pNciDevInfo->pRemDevList[bDeviceIndex1]);
                        if(wStatus != NFCSTATUS_SUCCESS)
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }
                        else
                        {
                            bDeviceIndex2++;
                        }
                        break;
                    }
                }
            }
        }
    }
    /*If the dev_cnt is 0, then it is NFCEE Direct interface.
      Before moving to the StateSEListen return success*/
    else
    {
        pLibContext->dev_cnt = 0;
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_StateDiscoveredExit(void *pContext, void *pParam1, void *pParam2, void *pParam3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    PHNFC_UNUSED_VARIABLE(pParam1);
    PHNFC_UNUSED_VARIABLE(pParam2);
    PHNFC_UNUSED_VARIABLE(pParam3);

    /* Clear the mifare auth info since it would still be around if we deactivated to sleep and then
       selected the device again. This will ensure the presence check isn't using previous auth data */
    phLibNfc_MfcAuthInfo_Clear(pLibContext);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_StateSEListenEntry(void *pContext, void *pParam1, void *pParam2, void *pParam3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_uSeEvtInfo_t tSeEvtInfo = {0};

    PH_LOG_LIBNFC_FUNC_ENTRY();

    PHNFC_UNUSED_VARIABLE(pParam1);
    PHNFC_UNUSED_VARIABLE(pParam2);
    PHNFC_UNUSED_VARIABLE(pParam3);

    if(!pLibContext->bPcdConnected)
    {
        pLibContext->bPcdConnected = TRUE;
        if(NULL != pLibContext->CBInfo.pSeListenerNtfCb)
        {
            PH_LOG_LIBNFC_INFO_STR("Invoking pSeListenerNtfCb");
            pLibContext->CBInfo.pSeListenerNtfCb(pLibContext->CBInfo.pSeListenerCtxt,
                                                 phLibNfc_eSE_EvtFieldOn,
                                                 NULL,
                                                 &tSeEvtInfo,
                                                 NFCSTATUS_SUCCESS);
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS   phLibNfc_RemoteDev_NtfRegister(
    phLibNfc_Registry_Info_t*       pRegistryInfo,
    phLibNfc_NtfRegister_RspCb_t    pNotificationHandler,
    void*                           pContext
    )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack no Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pNotificationHandler )||
       (NULL== pRegistryInfo))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter");
        wStatus= NFCSTATUS_INVALID_PARAMETER;
    }
    else if (pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("Shutdown in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        PH_LOG_LIBNFC_INFO_STR("Registering Notification Handler");

        (void )phOsalNfc_MemCopy(&(pLibContext->RegNtfType),
                                 pRegistryInfo,
                                 sizeof(phLibNfc_Registry_Info_t ));

        pLibContext->CBInfo.pClientNtfRegRespCB = pNotificationHandler;
        pLibContext->CBInfo.pClientNtfRegRespCntx = pContext;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_RemoteDev_Connect_Cb(void *pContext,\
                                          NFCSTATUS status,\
                                          void  *pInfo)
{
    NFCSTATUS wStatus = status;
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;
    pphLibNfc_ConnectCallback_t ps_client_con_cb = NULL;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(pLibContext == phLibNfc_GetContext())
    {
        if(NULL == pInfo)
        {
            PH_LOG_LIBNFC_CRIT_STR("Buffer passed by Lower layer is NULL");
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pNciRemoteDevHandle =(pphNciNfc_RemoteDevInformation_t)pInfo;

            /* Copy the Remote device address as connected handle*/
            pLibContext->Connected_handle = (void *)pInfo;
            pLibContext->DummyConnect_handle = (void *)pInfo;

            wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,\
                                                  &pNciRemoteDevHandle,\
                                                  PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapping on NCI RemoteDev Handle to LibNfc RemoteDev Handle Failed");
                wStatus = NFCSTATUS_FAILED;
            }
            else
            {
                /*In case of multiple discovery ,after connect received notification contain extra information
                overwrite the previous information with this new information*/
                wStatus = phLibNfc_ParseDiscActivatedRemDevInfo(pLibRemoteDevHandle,\
                                                               (pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle);

                if(NFCSTATUS_SUCCESS != wStatus)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Getting LibNfc RemoteDev Info by using LibNfc RemoteDev Handle Failed");
                    wStatus = NFCSTATUS_FAILED;
                }
                else
                {
                    pLibRemoteDevHandle->SessionOpened = pNciRemoteDevHandle->SessionOpened;
                }
            }
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        wRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);

        if(NFCSTATUS_SUCCESS != wRetVal)
        {
            wStatus = NFCSTATUS_FAILED;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            ps_client_con_cb = pLibContext->CBInfo.pClientConnectCb;
            if (NULL != pLibContext->CBInfo.pClientConnectCb)
            {
                pLibContext->CBInfo.pClientConnectCb = NULL;
                PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
                ps_client_con_cb(pLibContext->CBInfo.pClientConCntx,\
                                 (phLibNfc_Handle)pLibRemoteDevHandle,\
                                 pLibRemoteDevHandle,\
                                 wStatus);
            }
        }
        else
        {
            /* if(PHNFCSTATUS(status)==NFCSTATUS_INVALID_REMOTE_DEVICE) */
            /* If remote device is invalid return as TARGET LOST to upper layer*/
            /* If error code is other than SUCCESS return NFCSTATUS_TARGET_LOST */

            PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned NFCSTATUS_FAILED");
            wStatus = NFCSTATUS_FAILED;

            pLibContext->Connected_handle = NULL;
            pInfo = NULL;

            ps_client_con_cb = pLibContext->CBInfo.pClientConnectCb;
            if (NULL != pLibContext->CBInfo.pClientConnectCb)
            {
                pLibContext->CBInfo.pClientConnectCb = NULL;
                PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
                ps_client_con_cb(pLibContext->CBInfo.pClientConCntx,\
                                 (phLibNfc_Handle)pInfo,\
                                 pLibRemoteDevHandle,\
                                 wStatus);
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned Invalid LibNfc context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static NFCSTATUS phLibNfc_P2pActivateSeqComplete(void *pContext,\
                                          NFCSTATUS status,\
                                          void  *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pContext)
    {
        phLibNfc_RemoteDev_Connect_Cb(pContext,status,pInfo);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

void phLibNfc_RemoteDev_ConnectTimer_Cb(_In_ void *pContext)
{
    NFCSTATUS bRetVal;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_ConnectCallback_t    pClientConnectCb;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevInfo=NULL;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle=NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    phLibNfc_LibContext_t* pLibContext = (phLibNfc_LibContext_t*)pContext;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(pLibContext == phLibNfc_GetContext())
    {
        pClientConnectCb = pLibContext->CBInfo.pClientConnectCb;
        bRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);

        if(bRetVal == NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_INFO_STR("State machine has returned NFCSTATUS_SUCCESS");
            pNciRemoteDevHandle =(pphNciNfc_RemoteDevInformation_t)pLibContext->DummyConnect_handle;

            bRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(bRetVal != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapping of Nci RemoteDev Handle to LibNfc RemoteDev handle Failed");
                bRetVal = NFCSTATUS_FAILED;
            }
            else
            {
                bRetVal = phLibNfc_GetRemoteDevInfo(pLibRemoteDevHandle,&pLibRemoteDevInfo);
                if(bRetVal!=NFCSTATUS_SUCCESS)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Getting LibNfc RemoteDev Info by using LibNfc Handle Failed");
                    bRetVal = NFCSTATUS_FAILED;
                }
                else
                {
                    pLibContext->Connected_handle = pNciRemoteDevHandle;
                    pLibRemoteDevInfo->SessionOpened = pNciRemoteDevHandle->SessionOpened;
                }
            }
        }
        else
        {
            bRetVal=NFCSTATUS_FAILED;
        }

        if(pClientConnectCb!=NULL)
        {
            pLibContext->CBInfo.pClientConnectCb = NULL;
            PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
            (pClientConnectCb)((void *)pLibContext->CBInfo.pClientConCntx,
                               (phLibNfc_Handle)pLibRemoteDevHandle,
                               pLibRemoteDevInfo,
                               bRetVal);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned invalid LibNfc context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_RemoteDev_Connect(phLibNfc_Handle                hRemoteDevice,
                                     pphLibNfc_ConnectCallback_t    pNotifyConnect_RspCb,
                                     void*                          pContext
    )
{
    NFCSTATUS wRetVal = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevInfo;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    phNciNfc_RfInterfaces_t eRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventActivate;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    wRetVal = phLibNfc_IsInitialised(pLibContext);

    if(NFCSTATUS_SUCCESS != wRetVal)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wRetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if (pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        wRetVal= NFCSTATUS_SHUTDOWN;
    }
    else if((NULL == pNotifyConnect_RspCb)
            || (NULL == (void*)hRemoteDevice))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input Parameter");
        wRetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pLibRemoteDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
        wRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if(wRetVal != NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_CRIT_STR("Mapping of LibNfc RemoteDev Handle to NCI RemoteDev Handle Failed");
            wRetVal= NFCSTATUS_INVALID_HANDLE;
        }
        else
        {
            wRetVal = phLibNfc_ValidateNciHandle(pNciRemoteDevHandle);

            if(wRetVal != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("Validation of NCI RemoteDev Handle Failed");
                wRetVal= NFCSTATUS_INVALID_HANDLE;
            }
            else if (NULL != pLibContext->Connected_handle)
            {
                PH_LOG_LIBNFC_CRIT_STR("Connected to a RemoteDev Handle");
                wRetVal = NFCSTATUS_FAILED;
            }
            else
            {
                pNciRemoteDevInfo = (pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle;
                wRetVal = phLibNfc_GetTechModeAndRfInterface(pNciRemoteDevInfo,\
                                               &(pLibContext->bTechMode),&(pLibContext->bRfInterface));

                /* FIXME:- added assignment Below as a workaround for connected_handle problem during connect processing */
                pLibContext->Connected_handle = pNciRemoteDevInfo;

                if(NFCSTATUS_SUCCESS == wRetVal)
                {
                    switch(pNciRemoteDevHandle->eRFProtocol)
                    {
                    case phNciNfc_e_RfProtocolsIsoDepProtocol:
                        eRfInterface = phNciNfc_e_RfInterfacesISODEP_RF;
                        break;
                    case phNciNfc_e_RfProtocolsNfcDepProtocol:
                        eRfInterface = phNciNfc_e_RfInterfacesNFCDEP_RF;
                        break;
                    case phNciNfc_e_RfProtocolsMifCProtocol:
                        eRfInterface = phNciNfc_e_RfInterfacesTagCmd_RF;
                        break;
                    default:
                        eRfInterface = phNciNfc_e_RfInterfacesFrame_RF;
                        break;
                    }

                    wRetVal = phLibNfc_StateHandler(pLibContext,\
                                                    TrigEvent,\
                                                    (void *)pNciRemoteDevHandle,\
                                                    (void *)eRfInterface,\
                                                    (void *)pLibRemoteDevHandle);

                    if(wRetVal== NFCSTATUS_PENDING)
                    {
                        PH_LOG_LIBNFC_INFO_STR("State Machine has returned NFCSTATUS_PENDING");
                        /* If HAL Connect is pending update the LibNFC state machine
                           and store the CB pointer and Context,
                           mark the General CB pending status is TRUE*/
                        pLibContext->CBInfo.pClientConnectCb = pNotifyConnect_RspCb;
                        pLibContext->CBInfo.pClientConCntx = pContext;
                    }
                    else if((PHNFCSTATUS(wRetVal) == NFCSTATUS_INVALID_PARAMETER )||
                            (PHNFCSTATUS(wRetVal) == NFCSTATUS_BUSY) ||
                            (PHNFCSTATUS(wRetVal) == NFCSTATUS_SHUTDOWN))
                    {
                        PH_LOG_LIBNFC_INFO_X32MSG("State Machine has returned ",wRetVal);
                    }
                    else
                    {
                        PH_LOG_LIBNFC_INFO_STR("State Machine has returned NFCSTATUS_FAILED");
                        wRetVal = NFCSTATUS_FAILED;
                    }
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wRetVal;
}

NFCSTATUS phLibNfc_ValidateNciHandle(pphNciNfc_RemoteDevInformation_t psRemoteDevHandle)
{
    NFCSTATUS wStatus=NFCSTATUS_FAILED;
    uint8_t bIndex;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(psRemoteDevHandle!=NULL)
    {
        for(bIndex=0;bIndex<MAX_REMOTE_DEVICES;bIndex++)
        {
            if(pLibContext->Disc_handle[bIndex] == psRemoteDevHandle)
            {
                wStatus = NFCSTATUS_SUCCESS;
                break;
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_GetRemoteDevInfo(phLibNfc_sRemoteDevInformation_t *pLibNfcRemdevHandle,\
                                           phLibNfc_sRemoteDevInformation_t **pRemoteDevInfo)
{
    uint8_t bIndex;
    NFCSTATUS wStatus=NFCSTATUS_FAILED;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    if (pLibNfcRemdevHandle != NULL)
    {
        for (bIndex = 0 ;bIndex < pLibContext->dev_cnt; bIndex++)
        {
            if(pLibNfcRemdevHandle == (phLibNfc_sRemoteDevInformation_t *)pLibContext->psRemoteDevList[bIndex].hTargetDev)
            {
                *pRemoteDevInfo = pLibContext->psRemoteDevList[bIndex].psRemoteDevInfo;
                wStatus = NFCSTATUS_SUCCESS;
                break;
            }
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_MapRemoteDevHandle(phLibNfc_sRemoteDevInformation_t **Libnfc_RemoteDevHandle,phNciNfc_RemoteDevInformation_t **Nci_RemoteDevHandle,uint8_t flag)
{
    uint8_t bIndex;
    NFCSTATUS wStatus=NFCSTATUS_INVALID_HANDLE;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(flag == PH_LIBNFC_INTERNAL_LIBTONCI_MAP)
    {
        if(NULL != Libnfc_RemoteDevHandle)
        {
            for(bIndex=0;bIndex<pLibContext->dev_cnt;bIndex++)
            {
                if(pLibContext->Map_Handle[bIndex].pLibNfc_RemoteDev_List == *Libnfc_RemoteDevHandle)
                {
                    *Nci_RemoteDevHandle = pLibContext->Map_Handle[bIndex].pNci_RemoteDev_List;
                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }
    }
    else
    {
        if(*Nci_RemoteDevHandle != NULL)
        {
            for(bIndex=0;bIndex<pLibContext->dev_cnt;bIndex++)
            {
                if(pLibContext->Map_Handle[bIndex].pNci_RemoteDev_List == *Nci_RemoteDevHandle)
                {
                   *Libnfc_RemoteDevHandle = pLibContext->Map_Handle[bIndex].pLibNfc_RemoteDev_List;
                    wStatus=NFCSTATUS_SUCCESS;
                    break;
                 }
             }
        }
        else
        {
            wStatus=NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_RemoteDev_Transceive(phLibNfc_Handle                 hRemoteDevice,
                                        phLibNfc_sTransceiveInfo_t*     psTransceiveInfo,
                                        pphLibNfc_TransceiveCallback_t  pTransceive_RspCb,
                                        void*                           pContext
    )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phLibNfc_InternalTransceive(hRemoteDevice,
                                psTransceiveInfo,
                                pTransceive_RspCb,
                                pContext);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InternalTransceive(phLibNfc_Handle                 hRemoteDevice,
                                             phLibNfc_sTransceiveInfo_t*     psTransceiveInfo,
                                             pphLibNfc_TransceiveCallback_t  pTransceive_RspCb,
                                             void*                           pContext
    )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventTransceive;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if( (NULL == (void *)hRemoteDevice) || (NULL == psTransceiveInfo)
             || (NULL == pTransceive_RspCb) || (NULL == pContext))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameters");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pLibRemoteDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;

        wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,
                                              PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wStatus = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemoteDevHandle);

            /* TBD:- Mapping from LibNfc Rem device handle to Nci Rem device
               and pass the mapped handle to State Handler below */

            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = phLibNfc_StateHandler(pLibContext,
                                                TrigEvent,
                                                psTransceiveInfo,
                                                NULL,
                                                NULL);

                if(NFCSTATUS_PENDING == wStatus)
                {
                    /* Store the Callback and context in LibContext structure*/
                    pLibContext->CBInfo.pClientTranscvCb = pTransceive_RspCb;
                    pLibContext->CBInfo.pClientTranscvCntx = pContext;

                    /* store transceive info from user for later use during cb */
                    pLibContext->tTranscvBuff = psTransceiveInfo->sRecvData;
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("State machine returned status other than PENDING: %!NFCSTATUS!", wStatus);
                    PH_LOG_LIBNFC_CRIT_U32MSG("psTransceiveInfo->cmd.Iso15693Cmd", psTransceiveInfo->cmd.Iso15693Cmd);
                    PH_LOG_LIBNFC_CRIT_U32MSG("psTransceiveInfo->sSendData.length", psTransceiveInfo->sSendData.length);
                }
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapped handle validation failed!");
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("Failed to map handle");
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_RemoteDev_Disconnect( phLibNfc_Handle                  hRemoteDevice,
                                         phLibNfc_eReleaseType_t          ReleaseType,
                                         pphLibNfc_DisconnectCallback_t   pDscntCallback,
                                         void*                            pContext
    )
{
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevInfo;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDeActivate;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    phNfc_eDiscAndDisconnMode_t DisconnType;
    void* param1 = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wRetVal = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wRetVal)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        wRetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pDscntCallback)||(hRemoteDevice == 0))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid parameters passed by upper layer");
        wRetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        wRetVal= NFCSTATUS_SHUTDOWN;
    }
    else if(pLibContext->Connected_handle==0)
    {
        PH_LOG_LIBNFC_CRIT_STR("Target not connected");
        wRetVal = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if(ReleaseType == NFC_INVALID_RELEASE_TYPE)
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid Release type passed ");
        wRetVal = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pLibRemoteDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;

        wRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if(wRetVal != NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_CRIT_STR("Mapping of LibNfc RemoteDev Handle to NCI RemoteDev handle Failed");
            wRetVal= NFCSTATUS_INVALID_HANDLE;
        }
        else
        {
            /* The given handle is not the connected handle return NFCSTATUS_INVALID_HANDLE*/
            wRetVal = phLibNfc_ValidateNciHandle(pNciRemoteDevHandle);

            if(wRetVal != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("Validation of  NCI RemoteDev handle Failed");
                wRetVal= NFCSTATUS_INVALID_HANDLE;
            }
            else
            {
                /* Map actual release type to internal release type */
                switch(ReleaseType)
                {
                case NFC_DISCOVERY_STOP:
                    DisconnType = NFC_DISCONN_STOP_DISCOVERY;
                    break;

                case NFC_DISCOVERY_CONTINUE:
                    {
                        DisconnType = NFC_DISCONN_CONTINUE_DISCOVERY;

                        if(NULL == pLibContext->Connected_handle && pLibContext->dev_cnt > 1)
                        {
                            DisconnType = NFC_DISCONN_RESTART_DISCOVERY;
                        }
                        else
                        {
                            if(NULL != pLibRemoteDevHandle && pLibRemoteDevHandle->SessionOpened != 1)
                            {
                                DisconnType = NFC_DISCONN_RESTART_DISCOVERY;
                            }
                        }
                    }
                    break;

                case NFC_DISCOVERY_RESTART:
                    DisconnType = NFC_DISCONN_RESTART_DISCOVERY;
                    break;
                case NFC_DEVICE_SLEEP:
                    TrigEvent = phLibNfc_EventDeActivateSleep;
                    DisconnType = NFC_DISCONN_STOP_DISCOVERY;
                    break;
                default:
                    DisconnType = NFC_DISCONN_STOP_DISCOVERY;
                    break;
                }

                pLibContext->DiscDisconnMode = DisconnType;
                pNciRemoteDevInfo = (pphNciNfc_RemoteDevInformation_t )pNciRemoteDevHandle;
                pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;

                /*Call the NCI Disconnect */
                /*If remote device is a P2P Target, raise the priority of disconnect*/
                if(phNciNfc_eNfcIP1_Target == pNciRemoteDevInfo->RemDevType)
                {
                    PH_LOG_LIBNFC_INFO_STR("P2P Target detected as remote device type,try priority disconnect");
                    /* Raise the priority of Disconnect functionality */
                    (void)phLibNfc_EnablePriorityDiscDiscon((void *)pLibContext);
                    phNciNfc_AbortDataTransfer(pLibContext->sHwReference.pNciHandle);
                }
                wRetVal = phLibNfc_StateHandler(pLibContext,
                                                TrigEvent,
                                                (void *)DisconnType,
                                                param1,
                                                NULL);

                if( NFCSTATUS_PENDING == PHNFCSTATUS(wRetVal))
                {
                    PH_LOG_LIBNFC_INFO_STR("State machine has returned NFCSTATUS_PENDING");
                    pLibContext->CBInfo.pClientDisConnectCb = pDscntCallback;
                    pLibContext->CBInfo.pClientDConCntx = pContext;
                    /* Enable flag which specifies that the upper layer call back funciton
                    is stored in 'pClientDisConnectCb' */
                }
                else if ((NFCSTATUS_BUSY == PHNFCSTATUS(wRetVal))||
                         (NFCSTATUS_SHUTDOWN == PHNFCSTATUS(wRetVal))||
                         (NFCSTATUS_INVALID_PARAMETER == PHNFCSTATUS(wRetVal)))
                {
                    PH_LOG_LIBNFC_CRIT_X32MSG("State machine has returned ",wRetVal);
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                }
                else
                {
                    PH_LOG_LIBNFC_CRIT_STR("State machine has returned NFCSTATUS_FAILED");
                    /*If lower layer returns other than pending
                      (internal error codes) return NFCSTATUS_FAILED */
                    wRetVal = NFCSTATUS_FAILED;
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wRetVal;
}

NFCSTATUS phLibNfc_RemoteDev_NtfUnregister(void)
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);
    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack no Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    else if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("Shutdown in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        wStatus = phNciNfc_DeregisterNotification(pLibContext->sHwReference.pNciHandle,
                                                  phNciNfc_e_RegisterTagDiscovery);
        pLibContext->CBInfo.pClientNtfRegRespCB = NULL;
        pLibContext->CBInfo.pClientNtfRegRespCntx =NULL;

        PH_LOG_LIBNFC_CRIT_STR("Unregister Notification Handler");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

void phLibNfc_ConnectExtensionFelica_Cb(void *   pContext,\
                                     NFCSTATUS status,\
                                     void* pInfo)
{
    NFCSTATUS bRetVal;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_ConnectCallback_t    pClientConnectCb;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevInfo=NULL;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle=NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    phLibNfc_LibContext_t* pLibContext = (phLibNfc_LibContext_t*)pContext;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(pLibContext == phLibNfc_GetContext())
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;

        pClientConnectCb = pLibContext->CBInfo.pClientConnectCb;
        bRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);

        if(bRetVal == NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_INFO_STR("State machine has returned NFCSTATUS_SUCCESS");
            pNciRemoteDevHandle =(pphNciNfc_RemoteDevInformation_t)pLibContext->DummyConnect_handle;
            bRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(bRetVal != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapping of Nci RemoteDev Handle to LibNfc RemoteDev handle Failed");
                bRetVal = NFCSTATUS_FAILED;
            }
            else
            {
                bRetVal = phLibNfc_GetRemoteDevInfo(pLibRemoteDevHandle,&pLibRemoteDevInfo);
                if(bRetVal != NFCSTATUS_SUCCESS)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Getting LibNfc RemoteDev Info by using LibNfc Handle Failed");
                    bRetVal = NFCSTATUS_FAILED;
                }
                else
                {
                    pLibContext->Connected_handle = pNciRemoteDevHandle;
                    pLibRemoteDevInfo->SessionOpened = pNciRemoteDevHandle->SessionOpened;
                }
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("State machine update failed");
            bRetVal = NFCSTATUS_FAILED;
        }

        if(pClientConnectCb!=NULL)
        {
            if(NFCSTATUS_SUCCESS != status)
            {
                status = NFCSTATUS_FAILED;
            }
            pLibContext->CBInfo.pClientConnectCb = NULL;
            PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
            (pClientConnectCb)((void *)pLibContext->CBInfo.pClientConCntx,
                               (phLibNfc_Handle)pLibRemoteDevHandle,
                               pLibRemoteDevInfo,
                               status);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned invalid LibNfc context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_RemoteDev_CheckPresence( phLibNfc_Handle     hRemoteDevice,
                                            pphLibNfc_RspCb_t   pPresenceChk_RspCb,
                                            void*               pContext)
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle = NULL;
    phLibNfc_DummyInfo_t Info;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventDummy;
    phLibNfc_sTransceiveInfo_t tTransceiveInfo;
    uint16_t wRetVal;
    uint8_t bFlag = 0; /* Set this flag to '0' incase of check presence */

    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);

    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_CRIT_STR("Libnfc Stack is not initialised");
        wStatus=NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pContext) || (NULL == pPresenceChk_RspCb)
        || (hRemoteDevice == 0))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid parameters passed by upper layer");
        wStatus= NFCSTATUS_INVALID_PARAMETER;
    }
    else if( pLibContext->Connected_handle == NULL)
    {
        PH_LOG_LIBNFC_CRIT_STR("No target is connected");
        wStatus = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else
    {
        pLibRemoteDevHandle =( phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
        wStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,
                                              &pNciRemoteDevHandle,
                                              PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if(wStatus != NFCSTATUS_SUCCESS)
        {
            PH_LOG_LIBNFC_CRIT_STR("Mapping of LibNfc RemoteDev Handle to NCI RemoteDev Handle Failed");
            wStatus= NFCSTATUS_INVALID_HANDLE;
        }
        else
        {
            wStatus = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemoteDevHandle);
            if(wStatus != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("NCI RemoteDev Handle is not same as Connected Handle");
                wStatus= NFCSTATUS_INVALID_HANDLE;
            }
            else
            {
                /* Skip presence check for DTA and NFC-DEP */
                if((phNciNfc_e_RfProtocolsNfcDepProtocol == pNciRemoteDevHandle->eRFProtocol)
                    || (pLibContext->bDtaFlag))
                {
                    wRetVal = phOsalNfc_QueueDeferredCallback(
                                  phLibNfc_RemoteDev_ChkPresenceTimer_Cb,
                                  pLibContext);

                    if(wRetVal == 0x00)
                    {
                        wStatus = NFCSTATUS_PENDING;
                        PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                        pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                        pLibContext->CBInfo.pClientPresChkCntx = pContext;
                    }
                    else
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else if(phNciNfc_e_RfProtocolsT3tProtocol == pNciRemoteDevHandle->eRFProtocol)
                {
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;
                    Info.Evt = phLibNfc_DummyEventFelicaChkPresExtn;
                    wStatus = phLibNfc_StateHandler(pLibContext,
                                                    TrigEvent,
                                                    (void *)&bFlag,
                                                    &Info,
                                                    (void *)hRemoteDevice);
                    if(NFCSTATUS_PENDING == wStatus)
                    {
                        pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                        pLibContext->CBInfo.pClientPresChkCntx = pContext;
                    }
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                }
                else if((phNciNfc_e_RfProtocolsIsoDepProtocol == pNciRemoteDevHandle->eRFProtocol) &&
                        (pLibContext->Config.bIsoDepPresChkCmd == 1))
                {
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;
                    Info.Evt = phLibNfc_DummyEventIsoDepChkPresExtn;
                    /* Using state machine */
                    wStatus = phLibNfc_StateHandler(pLibContext,
                                                    TrigEvent,
                                                    (void *)&bFlag,
                                                    &Info,
                                                    NULL);
                    if(NFCSTATUS_PENDING == wStatus)
                    {
                        pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                        pLibContext->CBInfo.pClientPresChkCntx = pContext;
                    }
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                }
                else if(NFCSTATUS_SUCCESS == phLibNfc_ChkMfCTag(pNciRemoteDevHandle))
                {
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_INPROGRESS;
                    Info.Evt = phLibNfc_DummyEventChkPresMFC;
                    wStatus = phLibNfc_StateHandler(pLibContext,
                                                    TrigEvent,
                                                    (void *)&bFlag,
                                                    &Info,
                                                    (void *)&tTransceiveInfo);
                    if(NFCSTATUS_PENDING == wStatus)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                        pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                        pLibContext->CBInfo.pClientPresChkCntx = pContext;
                    }
                    else
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                    pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
                }
                else
                {
                    /* Using internal presence check procedure */
                    wStatus = phLibNfc_InternalPresenceCheck(hRemoteDevice, pPresenceChk_RspCb, (void *)pContext);
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_InternalPresenceCheck(phLibNfc_Handle     hRemoteDevice,
                                                pphLibNfc_RspCb_t    pPresenceChk_RspCb,
                                                void*                pContext)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    phLibNfc_sRemoteDevInformation_t *pLibNfc_RemoteDevInfo =NULL ;
    phLibNfc_sTransceiveInfo_t TransceiveInfo;
    static uint8_t bRATSRespBuff[19];
    static uint8_t b4ACmdBuff[] = {0x00,0xA4,0x00,0x00,0x02,0x11,0x11}; /*wrong pkt for check presence*/
    static uint8_t b4ARespBuff[2];
    static uint8_t bJewelCmdBuff[] = {0x78,0x00,0x00,0x00,0x00,0x00,0x00}; /*Send RID for check presence*/
    static uint8_t bJewelRespBuff[6];
    static uint8_t bFelicaCmdBuff[0x10] = {0x10,  /*SoD Byte = PayloadLen + 1*/
                                           0x06,  /*Check Cmd Code - 1 byte*/
                                           0x01,0x27,0x00,0x5D,0x1A,0x0B,0xA1,0xAD,  /*IDm - 8 bytes from SENSF_RESP*/
                                           0x01,  /*No.of services - 1 byte*/
                                           0x0B,0x00,  /*Service Code List - (2 * No. of services) bytes*/
                                           0x01,  /*/ No. of Blocks - 1 byte*/
                                           0x80,  /*Byte0 of 2 byte BlockList => 1(Len)|000(Access Mode)|0000(SCLO)*/
                                           0x05   /*Byte1 of 2 byte BlockList => 0x05 (BlockNumber)*/
                                          };
    static uint8_t bFelicaRespBuff[27];
    static uint8_t bCommonCmdBuff[3] = {0};
    static uint8_t bCommonRespBuff[68]; /* Max 528 bits can be received */
    uint8_t bIndex=0;
    uint8_t bIndex1=0;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL == pContext) || (NULL == pPresenceChk_RspCb)
        || (hRemoteDevice == 0) )
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid parameters passed");
        wStatus= NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        pLibNfc_RemoteDevInfo = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
        if((phNfc_eMifare_PICC == pLibNfc_RemoteDevInfo->RemDevType) ||
           (phNfc_eISO14443_3A_PICC == pLibNfc_RemoteDevInfo->RemDevType))
        {
            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.addr = 0x02;
            TransceiveInfo.NumBlock = 1;
            TransceiveInfo.cmd.MfCmd = phNfc_eMifareRead16;
            /*Use memory allocated after receiving activated notification - ALLOCATE memory and deallocate during disconnect or deinit*/
            TransceiveInfo.sRecvData.buffer = bRATSRespBuff;
            TransceiveInfo.sRecvData.length = 19;
            TransceiveInfo.sSendData.buffer = NULL;
            TransceiveInfo.sSendData.length = 0;
            wStatus = phLibNfc_InternalTransceive(hRemoteDevice,
                                                &TransceiveInfo,
                                                &phLibNfc_ChkPresence_Trcv_Cb,
                                                pLibContext);

            if(NFCSTATUS_PENDING == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                pLibContext->CBInfo.pClientPresChkCntx = pContext;
            }
            else if ((NFCSTATUS_BUSY == wStatus) ||
                     (NFCSTATUS_SHUTDOWN == wStatus)||
                     (NFCSTATUS_INVALID_PARAMETER == wStatus )||
                     (NFCSTATUS_INVALID_HANDLE == wStatus)||
                     (NFCSTATUS_TARGET_NOT_CONNECTED == wStatus))
            {
                PH_LOG_LIBNFC_INFO_X32MSG("Return status received from lower layer",wStatus);
            }
            else
            {
                /* If return value is internal error(other than pending ) return NFCSTATUS_FAILED*/
                PH_LOG_LIBNFC_CRIT_STR("Return status NFCSTATUS_FAILED received from lower layer");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if((phNfc_eISO14443_4A_PICC == pLibNfc_RemoteDevInfo->RemDevType)||\
                (phNfc_eISO14443_4B_PICC == pLibNfc_RemoteDevInfo->RemDevType))
        {
            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
            /*Use memory allocated after receiving activated notification - ALLOCATE memory and deallocate during disconnect or deinit*/
            TransceiveInfo.sRecvData.buffer = b4ARespBuff;
            TransceiveInfo.sRecvData.length = sizeof(b4ARespBuff);
            TransceiveInfo.sSendData.buffer = b4ACmdBuff;
            TransceiveInfo.sSendData.length = sizeof(b4ACmdBuff);
            wStatus =phLibNfc_InternalTransceive(hRemoteDevice,
                                                &TransceiveInfo,
                                                &phLibNfc_ChkPresence_Trcv_Cb,
                                                pLibContext);

            if(NFCSTATUS_PENDING == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                pLibContext->CBInfo.pClientPresChkCntx = pContext;
            }
            else
            {
                /* If return value is internal error(other than pending ) return NFCSTATUS_FAILED*/
                PH_LOG_LIBNFC_CRIT_STR("Return status NFCSTATUS_FAILED received from lower layer");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eJewel_PICC == pLibNfc_RemoteDevInfo->RemDevType)
        {
            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.cmd.JewelCmd = phNfc_eJewel_Raw;
            TransceiveInfo.sRecvData.buffer = bJewelRespBuff;
            TransceiveInfo.sRecvData.length = sizeof(bJewelRespBuff);
            TransceiveInfo.sSendData.buffer = bJewelCmdBuff;
            TransceiveInfo.sSendData.length = sizeof(bJewelCmdBuff);
            wStatus =phLibNfc_InternalTransceive(hRemoteDevice,
                                                &TransceiveInfo,
                                                &phLibNfc_ChkPresence_Trcv_Cb,
                                                pLibContext);

            if(NFCSTATUS_PENDING == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                pLibContext->CBInfo.pClientPresChkCntx = pContext;
            }
            else
            {
                /* If return value is internal error(other than pending ) return NFCSTATUS_FAILED*/
                PH_LOG_LIBNFC_CRIT_STR("Return status NFCSTATUS_FAILED received from lower layer");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eFelica_PICC == pLibNfc_RemoteDevInfo->RemDevType)
        {
             for(bIndex = 2,bIndex1 = 0 ; bIndex1 < 8; bIndex++ ,bIndex1++)
            {
                bFelicaCmdBuff[bIndex] = pLibNfc_RemoteDevInfo->RemoteDevInfo.Felica_Info.IDm[bIndex1];
            }

            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.cmd.FelCmd = phNfc_eFelica_Raw;
            TransceiveInfo.sRecvData.buffer = bFelicaRespBuff;
            TransceiveInfo.sRecvData.length = sizeof(bFelicaRespBuff);
            TransceiveInfo.sSendData.buffer = bFelicaCmdBuff;
            TransceiveInfo.sSendData.length = sizeof(bFelicaCmdBuff);
            wStatus =phLibNfc_InternalTransceive(hRemoteDevice,
                                                &TransceiveInfo,
                                                &phLibNfc_ChkPresence_Trcv_Cb,
                                                pLibContext);

            if(NFCSTATUS_PENDING == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                pLibContext->CBInfo.pClientPresChkCntx = pContext;
            }
            else
            {
                /* If return value is internal error(other than pending ) return NFCSTATUS_FAILED*/
                PH_LOG_LIBNFC_CRIT_STR("Return status NFCSTATUS_FAILED received from lower layer");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else if(phNfc_eISO15693_PICC == pLibNfc_RemoteDevInfo->RemDevType)
        {
            /* Sending Ack command */
            bCommonCmdBuff[0] = 0x02;
            bCommonCmdBuff[1] = 0x20;
            bCommonCmdBuff[2] = 0x00;

            phOsalNfc_SetMemory(&TransceiveInfo, 0x00, sizeof(phLibNfc_sTransceiveInfo_t));
            TransceiveInfo.cmd.Iso15693Cmd = phNfc_eIso15693_Raw;

            TransceiveInfo.sSendData.buffer = bCommonCmdBuff;
            TransceiveInfo.sSendData.length = sizeof(bCommonCmdBuff);
            TransceiveInfo.sRecvData.buffer = bCommonRespBuff;
            TransceiveInfo.sRecvData.length = sizeof(bCommonRespBuff);
            wStatus =phLibNfc_InternalTransceive(hRemoteDevice,
                                                &TransceiveInfo,
                                                &phLibNfc_ChkPresence_Trcv_Cb,
                                                pLibContext);
            if(NFCSTATUS_PENDING == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Return status NFCSTATUS_PENDING received from lower layer");
                pLibContext->CBInfo.pClientPresChkCb = pPresenceChk_RspCb;
                pLibContext->CBInfo.pClientPresChkCntx = pContext;
            }
            else
            {
                /* If return value is internal error(other than pending ) return NFCSTATUS_FAILED*/
                PH_LOG_LIBNFC_CRIT_STR("Return status NFCSTATUS_FAILED received from lower layer");
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            PH_LOG_LIBNFC_CRIT_STR("RemoteDev is other than Mifare PICC or SAK byte is 0");
            wStatus = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

void phLibNfc_IsoDepFelicaPresChk_Cb(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    void *pUpperLayerContext = NULL;
    pphLibNfc_RspCb_t pClientCb = NULL;
    phLibNfc_LibContext_t *pLibContext = (phLibNfc_LibContext_t *)pContext;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL == pLibContext) ||
      (pLibContext != pLibContext))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed by lower layer");
        wStatus = NFCSTATUS_FAILED;
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);

    }
    else
    {
        pLibContext->bSkipTransceive = PH_LIBNFC_INTERNAL_COMPLETE;
        (void)phLibNfc_StateHandler(pLibContext, TrigEvent, NULL, NULL, NULL);
        pClientCb = pLibContext->CBInfo.pClientPresChkCb;
        pUpperLayerContext = pLibContext->CBInfo.pClientPresChkCntx;
        pLibContext->CBInfo.pClientPresChkCntx = NULL;
        pLibContext->CBInfo.pClientPresChkCb =NULL;

        if(NFCSTATUS_RESPONSE_TIMEOUT == wStatus)
        {
            PH_LOG_LIBNFC_CRIT_STR("NFCC not responded for Iso-Dep pres chk command");
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Target is still in the Rf field");
            }
            else if(NFCSTATUS_TARGET_LOST == wStatus)
            {
                PH_LOG_LIBNFC_INFO_STR("Target is no more in the Rf field");
            }
            else if(NFCSTATUS_REJECTED == wStatus)
            {
                PH_LOG_LIBNFC_WARN_STR("Nfcc rejected pres chk command sent");
                wStatus = NFCSTATUS_FAILED;
            }
            else
            {
                PH_LOG_LIBNFC_WARN_STR("pres chk failed");
                wStatus = NFCSTATUS_TARGET_LOST;
            }
        }

        if(NULL != pClientCb)
        {
            PH_LOG_LIBNFC_INFO_STR("Invoke the upper layer callback function");
            pClientCb(pUpperLayerContext,wStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static void phLibNfc_RemoteDev_ChkPresenceTimer_Cb(_In_ void *pLibContext)
{
    uint8_t bDummyData = 1;
    phNfc_sData_t tResData;
    tResData.buffer = &bDummyData;
    tResData.length = 1;
    tResData.buffer = NULL;
    tResData.length = 0;
    phLibNfc_RemoteDev_ChkPresence_Cb(pLibContext,&tResData,NFCSTATUS_SUCCESS);
}

void phLibNfc_RemoteDev_ChkPresence_Cb(void     *pContext,
                                       phNfc_sData_t*      pResBuffer,
                                       NFCSTATUS wStatus)
{
    void                    *pUpperLayerContext=NULL;
    pphLibNfc_RspCb_t        pClientCb=NULL;
    phLibNfc_LibContext_t* pLibContext = (phLibNfc_LibContext_t*)pContext;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL == pContext) ||
      (pLibContext != phLibNfc_GetContext()))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc context passed by lower layer");
        wStatus = NFCSTATUS_FAILED;
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);

    }
    else
    {
        pClientCb = pLibContext->CBInfo.pClientPresChkCb ;
        pUpperLayerContext = pLibContext->CBInfo.pClientPresChkCntx;
        pLibContext->CBInfo.pClientPresChkCntx = NULL;
        pLibContext->CBInfo.pClientPresChkCb =NULL;

        if(NFCSTATUS_RESPONSE_TIMEOUT == wStatus)
        {
            PH_LOG_LIBNFC_WARN_STR("Target Lost!!");
            wStatus= NFCSTATUS_TARGET_LOST;
        }
        else
        {
            if((NFCSTATUS_SUCCESS == wStatus) && (NULL != pResBuffer))
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned status NFCSTATUS_SUCCESS");

                if((phNfc_eISO14443_4A_PICC == (pLibContext->psRemoteDevInfo->RemDevType)) ||
                   (phNfc_eISO14443_4B_PICC == (pLibContext->psRemoteDevInfo->RemDevType)))
                {
                    if(pResBuffer->length > 0x00)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
                    }
                    else
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else if(phNfc_eJewel_PICC == (pLibContext->psRemoteDevInfo->RemDevType))
                {
                    if(pResBuffer->length > 0x00)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
                    }
                    else
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                        wStatus = NFCSTATUS_TARGET_LOST;
                    }
                }
                else if(phNfc_eFelica_PICC == (pLibContext->psRemoteDevInfo->RemDevType))
                {
                    if(pResBuffer->length > 0x00)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
                    }
                    else
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                        wStatus = NFCSTATUS_TARGET_LOST;
                    }
                }
                else if((phNfc_eMifare_PICC == pLibContext->psRemoteDevInfo->RemDevType) ||
                        (phNfc_eISO14443_3A_PICC == pLibContext->psRemoteDevInfo->RemDevType))
                {
                    if(pLibContext->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak == PHLIBNFC_MIFAREUL_SAK)
                    {
                        if(pResBuffer->length > 0)
                        {
                            PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
                        }
                        else
                        {
                            PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                            wStatus = NFCSTATUS_FAILED;
                        }
                    }
                }
                else if(phNfc_eISO15693_PICC == (pLibContext->psRemoteDevInfo->RemDevType))
                {
                    if(pResBuffer->length > 0)
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned valid BuffLen");
                    }
                    else
                    {
                        PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Invalid BuffLen!!");
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else if(phNfc_eNfcIP1_Target == (pLibContext->psRemoteDevInfo->RemDevType))
                {
                        wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                }
            }
            else
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned Fail status!!");
                wStatus = NFCSTATUS_FAILED;
            }
        }
    }

    if(NULL != pClientCb)
    {
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            wStatus = NFCSTATUS_TARGET_LOST;
        }

        PH_LOG_LIBNFC_INFO_STR("Invoke the upper layer callback function");
        pClientCb(pUpperLayerContext,wStatus);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static void phLibNfc_ChkPresence_Trcv_Cb(
                            void *context,
                            phLibNfc_Handle hRemoteDev,
                            phNfc_sData_t *response,
                            NFCSTATUS status)
{
    UNUSED(hRemoteDev);
    UNUSED(response);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    phLibNfc_RemoteDev_ChkPresence_Cb(context,response,status);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

NFCSTATUS phLibNfc_Mgt_GetstackCapabilities(phLibNfc_StackCapabilities_t* phLibNfc_StackCapabilities,
                                            void*                         pContext)
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pLibContext = phLibNfc_GetContext();
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    wStatus = phLibNfc_IsInitialised(pLibContext);

    if(NFCSTATUS_SUCCESS != wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Stack is not Initialised");
        wStatus = NFCSTATUS_NOT_INITIALISED;
    }
    /*Check application has sent the valid parameters*/
    else if(NULL == phLibNfc_StackCapabilities)
    {
        PH_LOG_LIBNFC_INFO_STR("Invalid parameters passed");
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(pLibContext->StateContext.TrgtState == phLibNfc_StateReset)
    {
        PH_LOG_LIBNFC_INFO_STR("Shutdown in progress");
        wStatus = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        /* Device Capabilities*/
        phLibNfc_StackCapabilities->psDevCapabilities.NciVersion =
            pLibContext->tNfccFeatures.NciVer;
        
        phLibNfc_StackCapabilities->psDevCapabilities.ManufacturerId =
            pLibContext->tNfccFeatures.ManufacturerId;
        
        phLibNfc_StackCapabilities->psDevCapabilities.ManufactureInfo.Length =
            pLibContext->tNfccFeatures.ManufactureInfo.Length;
        phLibNfc_StackCapabilities->psDevCapabilities.ManufactureInfo.Buffer =
            pLibContext->tNfccFeatures.ManufactureInfo.Buffer;

        phLibNfc_StackCapabilities->psDevCapabilities.PowerStateInfo.SwitchOffState =
            pLibContext->tNfccFeatures.PowerStateInfo.SwitchOffState;
        phLibNfc_StackCapabilities->psDevCapabilities.PowerStateInfo.BatteryOffState =
            pLibContext->tNfccFeatures.PowerStateInfo.BatteryOffState;

        phLibNfc_StackCapabilities->psDevCapabilities.RoutingInfo.AidBasedRouting =
            pLibContext->tNfccFeatures.RoutingInfo.AidBasedRouting;
       phLibNfc_StackCapabilities->psDevCapabilities.RoutingInfo.ProtocolBasedRouting =
            pLibContext->tNfccFeatures.RoutingInfo.ProtocolBasedRouting;
       phLibNfc_StackCapabilities->psDevCapabilities.RoutingInfo.TechnBasedRouting =
            pLibContext->tNfccFeatures.RoutingInfo.TechnBasedRouting;

        phLibNfc_StackCapabilities->psDevCapabilities.RoutingTableSize =
            pLibContext->tNfccFeatures.RoutingTableSize;

        /* Tag Format Capabilities*/
        phLibNfc_StackCapabilities->psFormatCapabilities.Desfire = TRUE;
        phLibNfc_StackCapabilities->psFormatCapabilities.FeliCa = FALSE;
        phLibNfc_StackCapabilities->psFormatCapabilities.ISO14443_4A = FALSE;
        phLibNfc_StackCapabilities->psFormatCapabilities.ISO14443_4B = FALSE;
        phLibNfc_StackCapabilities->psFormatCapabilities.ISO15693 = FALSE;
        phLibNfc_StackCapabilities->psFormatCapabilities.Jewel = FALSE;
        phLibNfc_StackCapabilities->psFormatCapabilities.MifareStd = TRUE;
        phLibNfc_StackCapabilities->psFormatCapabilities.MifareUL = TRUE;
        phLibNfc_StackCapabilities->psFormatCapabilities.MifareULC = TRUE;
        phLibNfc_StackCapabilities->psFormatCapabilities.Kovio = TRUE;

        /* Tag Mapping Capabilities */
        phLibNfc_StackCapabilities->psMappingCapabilities.Desfire = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.FeliCa= TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.ISO14443_4A = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.ISO14443_4B = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.ISO15693 = FALSE;
        phLibNfc_StackCapabilities->psMappingCapabilities.Jewel = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.MifareStd = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.MifareUL = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.MifareULC = TRUE;
        phLibNfc_StackCapabilities->psMappingCapabilities.Kovio = TRUE;

        /* Emulation support protocols */
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.Bprime = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.EPCGEN2 = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.Felica = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.HID = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.ISO14443_4A = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.ISO14443_4B = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.ISO15693 = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.Jewel = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.Kovio = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.MifareStd = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.MifareUL = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.EmulationSupProtocol.NFC = FALSE;

        /* Reader support Protocols*/
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.Bprime = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.EPCGEN2 = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.Felica = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.HID = FALSE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.ISO14443_4A = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.ISO14443_4B = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.ISO15693 = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.Jewel = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.Kovio = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.MifareStd = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.MifareUL = TRUE;
        phLibNfc_StackCapabilities->psDevCapabilities.ReaderSupProtocol.NFC = TRUE;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Mgt_Reset(void*  pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_NOT_INITIALISED;
    pphLibNfc_LibContext_t pLibContext = phLibNfc_GetContext();
    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pContext);

    if(NULL != pLibContext)
    {
        /* Use LibNfc state machine here */
        wStatus = phNciNfc_Reset(pLibContext->sHwReference.pNciHandle,
                                 phNciNfc_NciReset_Mgt_Reset,NULL,NULL);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            phLibNfc_ClearLibContext(pLibContext);
            pLibContext = NULL;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ParseDiscActivatedRemDevInfo(phLibNfc_sRemoteDevInformation_t *pLibNfcDeviceInfo,
                                                       pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pNciDevInfo != NULL)
    {
        PH_LOG_LIBNFC_INFO_STR(
            "eRFTechMode = 0x%02X, eRFProtocol = 0x%02X, RemDevType = %d",
            pNciDevInfo->eRFTechMode,
            pNciDevInfo->eRFProtocol,
            pNciDevInfo->RemDevType);

        switch(pNciDevInfo->eRFTechMode)
        {
            case phNciNfc_NFCA_Poll:
            case phNciNfc_NFCA_Kovio_Poll:
            case phNciNfc_NFCA_Active_Poll:
            {
                if((pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsT2tProtocol) ||
                   (pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsMifCProtocol))
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eMifare_PICC;

                    /* Set Mifare Ultralight type and data area size */
                    pLibNfcDeviceInfo->RemoteDevInfo.Iso14443A_Info.ULType = pNciDevInfo->tRemoteDevInfo.Iso14443A_Info.ULType;
                    pLibNfcDeviceInfo->RemoteDevInfo.Iso14443A_Info.DataAreaSize = pNciDevInfo->tRemoteDevInfo.Iso14443A_Info.DataAreaSize;

                    wStatus = phLibNfc_MapRemoteDevA(pLibNfcDeviceInfo,
                                                     pNciDevInfo);
                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsT1tProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eJewel_PICC;

                    wStatus=phLibNfc_MapRemoteDevAJewel(&pLibNfcDeviceInfo->RemoteDevInfo.Jewel_Info,
                                                   pNciDevInfo);
                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eISO14443_4A_PICC;

                    wStatus=phLibNfc_MapRemoteDevA(pLibNfcDeviceInfo,
                                                   pNciDevInfo);
                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsNfcDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eNfcIP1_Target;

                    wStatus=phLibNfc_MapRemoteDevNfcIp1(&pLibNfcDeviceInfo->RemoteDevInfo.NfcIP_Info,
                                                        &pNciDevInfo->tRemoteDevInfo.NfcIP_Info,
                                                        pNciDevInfo->eRFTechMode);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsKovioProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eKovio_PICC;
                    wStatus = phLibNfc_MapRemoteDevKovio(&pLibNfcDeviceInfo->RemoteDevInfo.Kovio_Info, pNciDevInfo);
                    if (wStatus != NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus = NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCB_Poll:
            {
                if(pNciDevInfo->eRFProtocol== phNciNfc_e_RfProtocolsIsoDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eISO14443_4B_PICC;

                    wStatus = phLibNfc_MapRemoteDevB(&pLibNfcDeviceInfo->RemoteDevInfo.Iso14443B_Info,
                                                   pNciDevInfo);


                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCF_Poll:
            case phNciNfc_NFCF_Active_Poll:
            {
                if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsNfcDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eNfcIP1_Target;

                    wStatus=phLibNfc_MapRemoteDevNfcIp1(&pLibNfcDeviceInfo->RemoteDevInfo.NfcIP_Info,
                                                        &pNciDevInfo->tRemoteDevInfo.NfcIP_Info,
                                                        pNciDevInfo->eRFTechMode);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsT3tProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eFelica_PICC;

                    wStatus=phLibNfc_MapRemoteDevFelica(&pLibNfcDeviceInfo->RemoteDevInfo.Felica_Info,
                                                        pNciDevInfo);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCISO15693_Poll:
            {
                if(pNciDevInfo->eRFTechMode == phNciNfc_NFCISO15693_Poll)
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eISO15693_PICC;

                    wStatus=phLibNfc_MapRemoteDevIso15693(&pLibNfcDeviceInfo->RemoteDevInfo.Iso15693_Info,
                                                          &pNciDevInfo->tRemoteDevInfo.Iso15693_Info);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCA_Listen:
            case phNciNfc_NFCA_Active_Listen:
            {
                if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsNfcDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eNfcIP1_Initiator;

                    wStatus=phLibNfc_MapRemoteDevNfcIp1(&pLibNfcDeviceInfo->RemoteDevInfo.NfcIP_Info,
                                                        &pNciDevInfo->tRemoteDevInfo.NfcIP_Info,
                                                        pNciDevInfo->eRFTechMode);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eISO14443_A_PCD;
                    /*Register for the callback to receive the first data buffer
                    Once the first data buffer arrives, the HCE activation is intimated to DH*/
                    wStatus = phLibNfc_RegisterForHceActivation();
                    if(wStatus == NFCSTATUS_PENDING)
                    {
                        wStatus = phLibNfc_MapRemoteDevCeHost(&pLibNfcDeviceInfo->RemoteDevInfo.Iso14443A_Info,
                                                              pNciDevInfo);
                        if(wStatus!=NFCSTATUS_SUCCESS)
                        {
                            pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                            wStatus=NFCSTATUS_FAILED;
                        }
                    }
                    else
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCB_Listen:
            {
                if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsIsoDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eISO14443_B_PCD;

                    /*Register for the callback to receive the first data buffer
                    Once the first data buffer arrives, the HCE activation is intimated to DH*/
                    wStatus = phLibNfc_RegisterForHceActivation();

                    if(wStatus == NFCSTATUS_PENDING)
                    {
                        wStatus = phLibNfc_MapRemoteDevB(&pLibNfcDeviceInfo->RemoteDevInfo.Iso14443B_Info,
                                                       pNciDevInfo);
                        if(wStatus!=NFCSTATUS_SUCCESS)
                        {
                            pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                            wStatus=NFCSTATUS_FAILED;
                        }
                    }
                    else
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            case phNciNfc_NFCF_Listen:
            case phNciNfc_NFCF_Active_Listen:
            {
                if(pNciDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsNfcDepProtocol)
                {
                    pLibNfcDeviceInfo->RemDevType=phNfc_eNfcIP1_Initiator;

                    wStatus=phLibNfc_MapRemoteDevNfcIp1(&pLibNfcDeviceInfo->RemoteDevInfo.NfcIP_Info,
                                                        &pNciDevInfo->tRemoteDevInfo.NfcIP_Info,
                                                        pNciDevInfo->eRFTechMode);

                    if(wStatus!=NFCSTATUS_SUCCESS)
                    {
                        pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                        wStatus=NFCSTATUS_FAILED;
                    }
                }
                else
                {
                    pLibNfcDeviceInfo->RemDevType = phNfc_eInvalid_DevType;
                    wStatus=NFCSTATUS_FAILED;
                }
            }
            break;
            default:
            {
                pLibNfcDeviceInfo->RemDevType=phNfc_eInvalid_DevType;
            }
            break;
        }
    }
    else
    {
        wStatus=NFCSTATUS_INVALID_PARAMETER;
         PH_LOG_LIBNFC_CRIT_STR("Invalid Input info!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_MapRemoteDevA(phLibNfc_sRemoteDevInformation_t   *pLibNfcDeviceInfo,
                       pphNciNfc_RemoteDevInformation_t   pNciDevInfo)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    uint8_t bIndex = 0x00;
    phNciNfc_Iso14443AInfo_t tNciRemoteDevInfo;
    phNfc_sIso14443AInfo_t   *pRemoteDevInfo = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pNciDevInfo) && (NULL != pLibNfcDeviceInfo))
    {
        pRemoteDevInfo = &pLibNfcDeviceInfo->RemoteDevInfo.Iso14443A_Info;

        wStatus = phLibNfc_MapBitRate((phNciNfc_BitRates_t)pNciDevInfo->bTransBitRate,
                                      (phNfc_eDataRate_t *)&pRemoteDevInfo->MaxDataRate);

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            tNciRemoteDevInfo = pNciDevInfo->tRemoteDevInfo.Iso14443A_Info;

            /*Mapped SAK info from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            if(0 != tNciRemoteDevInfo.bSelResRespLen)
            {
                pRemoteDevInfo->Sak=tNciRemoteDevInfo.Sak;
            }
            pRemoteDevInfo->UidLength=tNciRemoteDevInfo.UidLength;

            /*Mapped UID from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            if(tNciRemoteDevInfo.UidLength > 0)
            {
                phOsalNfc_MemCopy(pRemoteDevInfo->Uid,\
                                  &tNciRemoteDevInfo.Uid[bIndex],\
                                  (uint32_t)pRemoteDevInfo->UidLength);
            }

            if((tNciRemoteDevInfo.bRatsRespLen > 0x04) &&
                (tNciRemoteDevInfo.bRatsRespLen <= (PHNFC_MAX_ATR_LENGTH+4) ))
            {
                pRemoteDevInfo->AppDataLength=tNciRemoteDevInfo.bRatsRespLen-4;
                if(0 != pRemoteDevInfo->AppDataLength)
                {
                    if(pRemoteDevInfo->AppDataLength > (PH_NCINFCTYPES_MAX_HIST_BYTES + bIndex))
                    {
                        PH_LOG_LIBNFC_CRIT_STR("Buffer too small! "
                                               "AppDataLength: %u",
                                               (uint32_t)(pRemoteDevInfo->AppDataLength));
                        wStatus = NFCSTATUS_BUFFER_TOO_SMALL;
                        goto Done;
                    }

                    /*Mapped AppData from Nci RemoteDev Info to LibNfc RemoteDev Info*/
                    phOsalNfc_MemCopy(pRemoteDevInfo->AppData,\
                        &tNciRemoteDevInfo.tRatsResp.bHistByte[bIndex],\
                        (uint32_t)pRemoteDevInfo->AppDataLength);
                }
            }

            pRemoteDevInfo->Fwi_Sfgt=tNciRemoteDevInfo.Fwi_Sfgt;

            /*Mapped ATQA from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            pRemoteDevInfo->AtqA[bIndex] = tNciRemoteDevInfo.bSensResResp[bIndex];
            pRemoteDevInfo->AtqA[bIndex+1] = tNciRemoteDevInfo.bSensResResp[bIndex+1];
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }

Done:

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevB(phNfc_sIso14443BInfo_t     *RemoteDevInfo,
                                        pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    uint8_t bIndex;
    uint8_t bIndex1;
    phNciNfc_Iso14443BInfo_t tNciRemoteDevInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(RemoteDevInfo!=NULL && pNciDevInfo!=NULL)
    {
        /*Mapped Bit Rate from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        wStatus = phLibNfc_MapBitRate((phNciNfc_BitRates_t)pNciDevInfo->bTransBitRate,
                                      (phNfc_eDataRate_t *)&RemoteDevInfo->MaxDataRate);

        if(wStatus != NFCSTATUS_SUCCESS)
        {
            wStatus = NFCSTATUS_FAILED;
        }
        else
        {
            tNciRemoteDevInfo = pNciDevInfo->tRemoteDevInfo.Iso14443B_Info;

            /*Mapped Afi from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            RemoteDevInfo->Afi = tNciRemoteDevInfo.Afi;

            /*Mapped Pupi length from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            bIndex = 0;
            bIndex1 = 0;
            phOsalNfc_MemCopy(&RemoteDevInfo->AtqB.AtqResInfo.Pupi[bIndex],\
                              &tNciRemoteDevInfo.aSensBResp[bIndex1],\
                              PHNFC_PUPI_LENGTH);

            /*Mapped AppData from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            bIndex = 0;
            bIndex1 = PHNFC_PUPI_LENGTH;
            phOsalNfc_MemCopy(&RemoteDevInfo->AtqB.AtqResInfo.AppData[bIndex],\
                              &tNciRemoteDevInfo.aSensBResp[bIndex1],\
                              PHNFC_APP_DATA_B_LENGTH);

            /*Mapped ProtInfo from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            bIndex = 0x00;
            bIndex1 = PHNFC_PUPI_LENGTH + PHNFC_APP_DATA_B_LENGTH;
            phOsalNfc_MemCopy(&RemoteDevInfo->AtqB.AtqResInfo.ProtInfo[bIndex],\
                              &tNciRemoteDevInfo.aSensBResp[bIndex1],\
                              PHNFC_PROT_INFO_B_LENGTH);

            /*Mapped HiLayerRespLength from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            RemoteDevInfo->HiLayerRespLength = tNciRemoteDevInfo.HiLayerRespLength;

            /*Mapped HiLayerResp from Nci RemoteDev Info to LibNfc RemoteDev Info*/
            bIndex = 0x00;
            phOsalNfc_MemCopy(&RemoteDevInfo->HiLayerResp[bIndex],\
                              &tNciRemoteDevInfo.HiLayerResp[bIndex],\
                              PHNFC_MAX_ATR_LENGTH);
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevAJewel(phNfc_sJewelInfo_t     *RemoteDevInfo,
                                             pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    uint8_t bIndex = 0x00;
    phNciNfc_JewelInfo_t tNciRemoteDevInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(RemoteDevInfo!=NULL && pNciDevInfo!=NULL)
    {
        tNciRemoteDevInfo = pNciDevInfo->tRemoteDevInfo.Jewel_Info;

        /*Mapped UID length from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        RemoteDevInfo->UidLength=tNciRemoteDevInfo.UidLength;

        /*Mapped UID from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        phOsalNfc_MemCopy(&RemoteDevInfo->Uid[bIndex],\
                          &tNciRemoteDevInfo.Uid[bIndex],\
                          (uint32_t)RemoteDevInfo->UidLength);

        RemoteDevInfo->HeaderRom0 = pNciDevInfo->tRemoteDevInfo.Jewel_Info.HeaderRom0;
        RemoteDevInfo->HeaderRom1 = pNciDevInfo->tRemoteDevInfo.Jewel_Info.HeaderRom1;

    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevIso15693(phNfc_sIso15693Info_t   *pRemoteDevInfo,
                                               phNciNfc_Iso15693Info_t *pNciRemoteDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pLibContext) && (NULL != pRemoteDevInfo) &&
       (NULL != pNciRemoteDevInfo))
    {
        if(PH_NCINFCTYPES_15693_UID_LENGTH == pNciRemoteDevInfo->UidLength)
        {
            PH_LOG_LIBNFC_WARN_STR("Valid ISO15693 UID length received");
        }
        else
        {
            PH_LOG_LIBNFC_WARN_STR("Invalid ISO15693 UID length");
        }
        phOsalNfc_MemCopy(pRemoteDevInfo->Uid,pNciRemoteDevInfo->Uid,
                          pNciRemoteDevInfo->UidLength);
        pRemoteDevInfo->UidLength = pNciRemoteDevInfo->UidLength;
        pRemoteDevInfo->Afi = pNciRemoteDevInfo->Afi;
        pRemoteDevInfo->Dsfid = pNciRemoteDevInfo->Dsfid;
        pRemoteDevInfo->Flags = pNciRemoteDevInfo->Flags;
        PH_LOG_LIBNFC_INFO_HEXDATA("Received UID",pRemoteDevInfo->Uid,pRemoteDevInfo->UidLength);
        PH_LOG_LIBNFC_INFO_X32MSG("UID length",pRemoteDevInfo->UidLength);
        PH_LOG_LIBNFC_INFO_X32MSG("AFI",pRemoteDevInfo->Afi);
        PH_LOG_LIBNFC_INFO_X32MSG("DSFID",pRemoteDevInfo->Dsfid);
        PH_LOG_LIBNFC_INFO_X32MSG("Flags",pRemoteDevInfo->Flags);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevKovio(phNfc_sKovioInfo_t *RemoteDevInfo,
                                            pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phNciNfc_KovioInfo_t pRemoteDevInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pNciDevInfo) && (NULL != RemoteDevInfo))
    {
        pRemoteDevInfo = pNciDevInfo->tRemoteDevInfo.Kovio_Info;
        RemoteDevInfo->TagIdLength = pRemoteDevInfo.TagIdLength;
        phOsalNfc_MemCopy(&RemoteDevInfo->TagId, pRemoteDevInfo.TagId, RemoteDevInfo->TagIdLength);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevNfcIp1(phNfc_sNfcIPInfo_t     *pRemoteDevInfo,
                                             phNciNfc_NfcIPInfo_t   *pNciRemoteDevInfo,
                                             phNciNfc_RfTechMode_t  eRfTechMode)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    uint8_t bOptionalParams = 0;
    phNfc_eDataRate_t eDataRate;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pLibContext) && (NULL != pRemoteDevInfo) &&
       (NULL != pNciRemoteDevInfo))
    {
        wStatus = phLibNfc_MapBitRate(pNciRemoteDevInfo->Nfcip_Datarate,
                                      (phNfc_eDataRate_t *)&eDataRate);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            switch(eRfTechMode)
            {
                case phNciNfc_NFCA_Active_Poll:
                case phNciNfc_NFCF_Active_Poll:
                case phNciNfc_NFCA_Active_Listen:
                case phNciNfc_NFCF_Active_Listen:
                    pRemoteDevInfo->Nfcip_Active = 1;
                    break;
                default:
                    pRemoteDevInfo->Nfcip_Active = pNciRemoteDevInfo->Nfcip_Active;
                    break;
            }
            pRemoteDevInfo->Nfcip_Datarate = eDataRate;

            /* Based on the device type and active technology and mode, map the NfcIp parameters */
            switch(eRfTechMode)
            {
                case phNciNfc_NFCA_Poll:
                case phNciNfc_NFCA_Active_Poll:
                {
                    pRemoteDevInfo->NFCID_Length = pNciRemoteDevInfo->NFCID_Length;
                    if(0 != pRemoteDevInfo->NFCID_Length)
                    {
                        phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                        phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->NFCID,
                                          pRemoteDevInfo->NFCID_Length);
                    }
                    pRemoteDevInfo->SenseRes[0] = 0;
                    pRemoteDevInfo->SenseRes[1] = 0;
                    if((0 != pNciRemoteDevInfo->SensResLength) &&
                       (PH_NCINFCTYPES_ATQA_LENGTH == pNciRemoteDevInfo->SensResLength))
                    {
                        phOsalNfc_MemCopy(pRemoteDevInfo->SenseRes,
                                          pNciRemoteDevInfo->SensRes,
                                          PH_NCINFCTYPES_ATQA_LENGTH);
                    }
                    pRemoteDevInfo->SelRes = 0;
                    if(0 != pNciRemoteDevInfo->SelResLen)
                    {
                        pRemoteDevInfo->SelRes = pNciRemoteDevInfo->SelRes;
                    }
                    if((0 != pNciRemoteDevInfo->bATRInfo_Length) &&
                       (PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET <= pNciRemoteDevInfo->bATRInfo_Length))
                    {
                        bOptionalParams = pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_MAX_NFCID3_SIZE +\
                                                                      PH_NCINFCTYPES_DID_LEN + \
                                                                      PH_NCINFCTYPES_SEND_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_RECV_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_TIMEOUT_LEN];
                        pRemoteDevInfo->MaxFrameLength = (((bOptionalParams >> \
                                                            PH_NCINFCTYPES_MAXFRAMELEN) & PH_NCINFCTYPES_MAXFRAMELEN_BITMASK) * \
                                                          PHLIBNFC_DATE_RATE_FACTOR);
                        if(PH_NCINFCTYPES_MAXFRAMELEN_MAX == pRemoteDevInfo->MaxFrameLength)
                        {
                            /* Maximum payload size can be of size 254 (ref: [DIGITAL] ATR_RES parameter) */
                            pRemoteDevInfo->MaxFrameLength -= PH_NCINFCTYPES_MAXFRAMELEN_LIMITER;
                        }
                        phOsalNfc_SetMemory(pRemoteDevInfo->ATRInfo,0,PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH);
                        pRemoteDevInfo->ATRInfo_Length = 0;
                        /* Chech if general bytes are present in ATR_RES */
                        if((bOptionalParams & PH_NCINFCTYPES_GENERALBYTES_SUPP) &&
                            (pNciRemoteDevInfo->bATRInfo_Length > PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET))
                        {
                            /* General bytes length */
                            pRemoteDevInfo->ATRInfo_Length =
                                (pNciRemoteDevInfo->bATRInfo_Length - PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET);
                            phOsalNfc_MemCopy(pRemoteDevInfo->ATRInfo,
                                              &pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET],
                                              pRemoteDevInfo->ATRInfo_Length);
                        }
                        /* If NfcId1 is not received through techn spec parameters, extracting them
                           from Actv parameters*/
                        if(0 == pRemoteDevInfo->NFCID_Length)
                        {
                            pRemoteDevInfo->NFCID_Length = PH_NCINFCTYPES_MAX_NFCID3_SIZE;
                            phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,
                                              PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                            phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->aAtrInfo,
                                              PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                        }
                    }
                }
                break;
                case phNciNfc_NFCF_Poll:
                case phNciNfc_NFCF_Active_Poll:
                {
                    pRemoteDevInfo->NFCID_Length = pNciRemoteDevInfo->NFCID_Length;
                    if(0 != pRemoteDevInfo->NFCID_Length)
                    {
                        phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                        phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->NFCID,
                                          pRemoteDevInfo->NFCID_Length);
                    }
                    if((0 != pNciRemoteDevInfo->bATRInfo_Length) &&
                       (PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET <= pNciRemoteDevInfo->bATRInfo_Length))
                    {
                        /* NfcId2 not received through techn spec parameters, extracting it from
                           Actv parameters*/
                        if(0 == pRemoteDevInfo->NFCID_Length)
                        {
                            pRemoteDevInfo->NFCID_Length = PH_NCINFCTYPES_MAX_NFCID2_SIZE;
                            phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                            phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->aAtrInfo,
                                              PH_NCINFCTYPES_MAX_NFCID2_SIZE);
                        }
                        bOptionalParams = pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_MAX_NFCID3_SIZE +\
                                                                      PH_NCINFCTYPES_DID_LEN + \
                                                                      PH_NCINFCTYPES_SEND_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_RECV_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_TIMEOUT_LEN];
                        pRemoteDevInfo->MaxFrameLength = (((bOptionalParams >> \
                                                            PH_NCINFCTYPES_MAXFRAMELEN) & PH_NCINFCTYPES_MAXFRAMELEN_BITMASK) * \
                                                          PHLIBNFC_DATE_RATE_FACTOR);
                        if(PH_NCINFCTYPES_MAXFRAMELEN_MAX == pRemoteDevInfo->MaxFrameLength)
                        {
                            /* Maximum payload size can be of size 254 (ref: [DIGITAL] ATR_RES parameter) */
                            pRemoteDevInfo->MaxFrameLength -= PH_NCINFCTYPES_MAXFRAMELEN_LIMITER;
                        }
                        phOsalNfc_SetMemory(pRemoteDevInfo->ATRInfo,0,PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH);
                        pRemoteDevInfo->ATRInfo_Length = 0;
                        /* Chech if general bytes are present in ATR_RES */
                        if((bOptionalParams & PH_NCINFCTYPES_GENERALBYTES_SUPP) &&
                            (pNciRemoteDevInfo->bATRInfo_Length > PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET))
                        {
                            /* General bytes length */
                            pRemoteDevInfo->ATRInfo_Length =
                                (pNciRemoteDevInfo->bATRInfo_Length - PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET);
                            phOsalNfc_MemCopy(pRemoteDevInfo->ATRInfo,
                                              &pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_ATR_RES_GENBYTES_OFFSET],
                                              pRemoteDevInfo->ATRInfo_Length);
                        }
                    }
                }
                break;
                case phNciNfc_NFCA_Listen:
                case phNciNfc_NFCA_Active_Listen:
                {
                    if((0 != pNciRemoteDevInfo->bATRInfo_Length) &&
                       (PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET <= pNciRemoteDevInfo->bATRInfo_Length))
                    {
                        bOptionalParams = pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_MAX_NFCID3_SIZE +\
                                                                      PH_NCINFCTYPES_DID_LEN + \
                                                                      PH_NCINFCTYPES_SEND_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_RECV_BIT_RATE_LEN];
                        pRemoteDevInfo->MaxFrameLength = (((bOptionalParams >> \
                                                            PH_NCINFCTYPES_MAXFRAMELEN) & PH_NCINFCTYPES_MAXFRAMELEN_BITMASK) * \
                                                          PHLIBNFC_DATE_RATE_FACTOR);
                        if(PH_NCINFCTYPES_MAXFRAMELEN_MAX == pRemoteDevInfo->MaxFrameLength)
                        {
                            /* Maximum payload size can be of size 254 (ref: [DIGITAL] ATR_REQ parameter) */
                            pRemoteDevInfo->MaxFrameLength -= PH_NCINFCTYPES_MAXFRAMELEN_LIMITER;
                        }
                        phOsalNfc_SetMemory(pRemoteDevInfo->ATRInfo,0,PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH);
                        pRemoteDevInfo->ATRInfo_Length = 0;
                        /* Chech if general bytes are present in ATR_RES */
                        if((bOptionalParams & PH_NCINFCTYPES_GENERALBYTES_SUPP) &&
                            (pNciRemoteDevInfo->bATRInfo_Length > PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET))
                        {
                            /* General bytes length */
                            pRemoteDevInfo->ATRInfo_Length =
                                (pNciRemoteDevInfo->bATRInfo_Length - PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET);
                            phOsalNfc_MemCopy(pRemoteDevInfo->ATRInfo,
                                              &pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET],
                                              pRemoteDevInfo->ATRInfo_Length);
                        }
                        /* As length of NfcID1 length is not mentioned incase of using Nfc-A Listen,
                           copying all 10 bytes */
                        pRemoteDevInfo->NFCID_Length = PH_NCINFCTYPES_MAX_NFCID3_SIZE;
                        phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,
                                          PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                        phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->aAtrInfo,
                                          PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                     }
                }
                break;
                case phNciNfc_NFCF_Listen:
                case phNciNfc_NFCF_Active_Listen:
                {
                    pRemoteDevInfo->NFCID_Length = pNciRemoteDevInfo->NFCID_Length;
                    if(0 != pRemoteDevInfo->NFCID_Length)
                    {
                        phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,
                                          PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                        phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->NFCID,
                                          pRemoteDevInfo->NFCID_Length);
                    }

                    if((0 != pNciRemoteDevInfo->bATRInfo_Length) &&
                       (PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET <= pNciRemoteDevInfo->bATRInfo_Length))
                    {
                        /* NfcId2 not received through techn spec parameters, extracting it from
                           Actv parameters*/
                        if(0 == pRemoteDevInfo->NFCID_Length)
                        {
                            pRemoteDevInfo->NFCID_Length = PH_NCINFCTYPES_MAX_NFCID2_SIZE;
                            phOsalNfc_SetMemory(pRemoteDevInfo->NFCID,0,PH_NCINFCTYPES_MAX_NFCID3_SIZE);
                            phOsalNfc_MemCopy(pRemoteDevInfo->NFCID,pNciRemoteDevInfo->aAtrInfo,
                                              PH_NCINFCTYPES_MAX_NFCID2_SIZE);
                        }
                        bOptionalParams = pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_MAX_NFCID3_SIZE +\
                                                                      PH_NCINFCTYPES_DID_LEN + \
                                                                      PH_NCINFCTYPES_SEND_BIT_RATE_LEN + \
                                                                      PH_NCINFCTYPES_RECV_BIT_RATE_LEN];
                        pRemoteDevInfo->MaxFrameLength = (((bOptionalParams >> \
                                                            PH_NCINFCTYPES_MAXFRAMELEN) & PH_NCINFCTYPES_MAXFRAMELEN_BITMASK) * \
                                                          PHLIBNFC_DATE_RATE_FACTOR);
                        if(PH_NCINFCTYPES_MAXFRAMELEN_MAX == pRemoteDevInfo->MaxFrameLength)
                        {
                            /* Maximum payload size can be of size 254 (ref: [DIGITAL] ATR_REQ parameter) */
                            pRemoteDevInfo->MaxFrameLength -= PH_NCINFCTYPES_MAXFRAMELEN_LIMITER;
                        }
                        phOsalNfc_SetMemory(pRemoteDevInfo->ATRInfo,0,PH_LIBNFC_INTERNAL_MAX_ATR_LENGTH);
                        pRemoteDevInfo->ATRInfo_Length = 0;
                        /* Chech if general bytes are present in ATR_RES */
                        if((bOptionalParams & PH_NCINFCTYPES_GENERALBYTES_SUPP) &&
                            (pNciRemoteDevInfo->bATRInfo_Length > PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET))
                        {
                            pRemoteDevInfo->ATRInfo_Length =
                                (pNciRemoteDevInfo->bATRInfo_Length - PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET);
                            phOsalNfc_MemCopy(pRemoteDevInfo->ATRInfo,
                                              &pNciRemoteDevInfo->aAtrInfo[PH_NCINFCTYPES_ATR_REQ_GENBYTES_OFFSET],
                                              pRemoteDevInfo->ATRInfo_Length);
                        }
                    }
                }
                break;
                default:
                {
                    wStatus = NFCSTATUS_FAILED;
                }
                break;
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
        PH_LOG_LIBNFC_CRIT_STR("Invalid input parameter!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MapRemoteDevFelica(phNfc_sFelicaInfo_t     *RemoteDevInfo,
                                             pphNciNfc_RemoteDevInformation_t pNciDevInfo)
{
    NFCSTATUS wStatus=NFCSTATUS_SUCCESS;
    uint8_t bIndex = 0;
    phNciNfc_FelicaInfo_t tNciRemoteDevInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(RemoteDevInfo!=NULL && pNciDevInfo!=NULL)
    {
        tNciRemoteDevInfo = pNciDevInfo->tRemoteDevInfo.Felica_Info;

        /*Mapped UID length from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        RemoteDevInfo->IDmLength = PH_NCINFCTYPES_FEL_ID_LEN;

        /*Use the aSensFResp array for obtaining the IDm(index 0 to index 7 in aSensFResp)*/
        /*Mapped IDm from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        phOsalNfc_MemCopy(RemoteDevInfo->IDm,&tNciRemoteDevInfo.aSensFResp[bIndex],
            PH_NCINFCTYPES_FEL_ID_LEN);
        bIndex += PH_NCINFCTYPES_FEL_ID_LEN;

        /*Use the aSensFResp array for obtaining PMm(index 8 to index 15 in aSensFResp)*/
        /*Mapped PMm from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        phOsalNfc_MemCopy(RemoteDevInfo->PMm,&tNciRemoteDevInfo.aSensFResp[bIndex],
            PH_NCINFCTYPES_FEL_PM_LEN);
        bIndex += PH_NCINFCTYPES_FEL_PM_LEN;

        /*Mapped SystemCode from Nci RemoteDev Info to LibNfc RemoteDev Info*/
        phOsalNfc_MemCopy(RemoteDevInfo->SystemCode,&tNciRemoteDevInfo.aSensFResp[bIndex],
            PHNFC_FEL_SYS_CODE_LEN);
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    return wStatus;
}
void phLibNfc_TranscvCb(void*   pContext,
                               NFCSTATUS status,
                               pphNciNfc_Data_t    pInfo
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS wIntStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext=NULL;
    pphLibNfc_TransceiveCallback_t pClientCb=NULL;
    void *pUpperLayerContext=NULL;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    pphNciNfc_RemoteDevInformation_t pConnectedNciHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    pLibContext  = (pphLibNfc_LibContext_t)pContext;

    if((NULL == pLibContext) || 
       (NULL == pInfo))
    {
        status = NFCSTATUS_FAILED;
    }
    else
    {
        if(phLibNfc_GetContext() == pLibContext)
        {
            pClientCb = pLibContext->CBInfo.pClientTranscvCb;
            pUpperLayerContext = pLibContext->CBInfo.pClientTranscvCntx;

            pConnectedNciHandle = (pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle ;

            if((NFCSTATUS_SUCCESS != status) &&
               (NULL != pConnectedNciHandle) &&
               (NFCSTATUS_SUCCESS == phLibNfc_ChkMfCTag(pConnectedNciHandle)))
            {
                /*Reset flag*/
                /* Reactivate sequence for Mifare classic tag if command is failed */
                pLibContext->tSelInf.eRfIf = phNciNfc_e_RfInterfacesTagCmd_RF;

                /*If this flag is set,then change reactivation sequence*/
                if(pLibContext->bReactivation_Flag == PH_LIBNFC_REACT_ONLYSELECT)
                {
                    PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReActivate_MFCSeqSelect);
                }
                else
                {
                    PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_ReActivate_MFCSeq);
                }

                (void)phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
            }
            else
            {
                if(NFCSTATUS_SUCCESS == status)
                {
                    wStatus = phLibNfc_VerifyResponse(pInfo,
                        (pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle);
                    if (NFCSTATUS_SUCCESS != wStatus)
                    {
                        PH_LOG_LIBNFC_WARN_STR("Invalid response: %!NFCSTATUS!", wStatus);
                    }
                }
                else if (NFCSTATUS_RESPONSE_TIMEOUT == status ||
                    NFCSTATUS_RF_ERROR == status)
                {
                    PH_LOG_LIBNFC_WARN_STR("Target Lost!");
                    wStatus = NFCSTATUS_RF_ERROR;
                }
                else if (NFCSTATUS_INSUFFICIENT_RESOURCES == status )
                {
                    PH_LOG_LIBNFC_WARN_STR("Insufficient resources!");
                    wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    wStatus = NFCSTATUS_FAILED;
                }

                if(NFCSTATUS_PENDING != wStatus)
                {
                    phLibNfc_UpdateEvent(PHNFCSTATUS(status),&TrigEvent);
                    wIntStatus = phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

                    if(wIntStatus == NFCSTATUS_SUCCESS)
                    {
                        pNciRemoteDevHandle =(pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle;
                        wIntStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);

                        if(wIntStatus != NFCSTATUS_SUCCESS)
                        {
                            wStatus = NFCSTATUS_FAILED;
                        }
                    }
                    else
                    {
                        wStatus = NFCSTATUS_FAILED;
                    }
                    pLibContext->CBInfo.pClientTranscvCb = NULL;
                    pLibContext->CBInfo.pClientTranscvCntx = NULL;

                    if((NULL != pClientCb))
                    {
                        if((NFCSTATUS_SUCCESS == wStatus) && (0 != pInfo->wLen) && (NULL != pInfo->pBuff))
                        {
                            (pLibContext->tTranscvBuff.length) = pInfo->wLen;
                            phOsalNfc_MemCopy((pLibContext->tTranscvBuff.buffer),pInfo->pBuff,pInfo->wLen);
                            (*pClientCb)(pUpperLayerContext,(phLibNfc_Handle)pLibRemoteDevHandle,
                                &(pLibContext->tTranscvBuff),wStatus);
                        }
                        else if(0 == pInfo->wLen)
                        {
                            pLibContext->tTranscvBuff.buffer = pInfo->pBuff;
                            pLibContext->tTranscvBuff.length = pInfo->wLen;
                            (*pClientCb)(pUpperLayerContext,(phLibNfc_Handle)pLibRemoteDevHandle,
                                &(pLibContext->tTranscvBuff),wStatus);
                        }
                    }
                    else
                    {
                        if(TRUE == (pLibContext->tSelInf.bSelectInpAvail))
                        {
                            (void)phLibNfc_SeqHandler(pContext,status,pInfo);
                        }
                    }
                }
            }
        }
        else
        {
            /* Invalid Context received form lower layer, Lower Layer might not be functioning properly */
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

NFCSTATUS
phLibNfc_NciTranscv( void*                               pNciHandle,
                     void*                               pDevicehandle,
                     pphNciNfc_TransceiveInfo_t         psTransceiveInfo,
                     void*                               pContext)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if( (NULL == pNciHandle) || (NULL == pDevicehandle) ||
        (NULL == psTransceiveInfo) ||
        (NULL == pContext) )
    {
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        status = phNciNfc_Transceive(pNciHandle,
                                     pDevicehandle,
                                     psTransceiveInfo,
                                     &phLibNfc_TranscvCb,
                                     pContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

NFCSTATUS phLibNfc_MapCmds(phNciNfc_RFDevType_t RemDevType,
                           phLibNfc_sTransceiveInfo_t*     pTransceiveInfo,
                           pphNciNfc_TransceiveInfo_t   pMappedTranscvIf
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pTransceiveInfo) && (NULL != pMappedTranscvIf))
    {
        pMappedTranscvIf->wTimeout = pTransceiveInfo->timeout;

        switch(RemDevType)
        {
            case phNciNfc_eMifareUL_PICC:
            case phNciNfc_eMifare1k_PICC:
            case phNciNfc_eMifare4k_PICC:
            case phNciNfc_eMifareMini_PICC:
            case phNciNfc_eISO14443_3A_PICC:
            {
                status = phLibNfc_MifareMap(pTransceiveInfo,pMappedTranscvIf);
                break;
            }
            case phNciNfc_eISO14443_4A_PICC:
            case phNciNfc_eISO14443_4B_PICC:
            {
                if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                    (0 != (pTransceiveInfo->sSendData.length)) &&
                    (NULL != pTransceiveInfo->sRecvData.buffer) &&
                    (0 != (pTransceiveInfo->sRecvData.length)) &&
                    (phNfc_eIso14443_4_Raw == pTransceiveInfo->cmd.Iso144434Cmd)
                    )
                {
                    (pMappedTranscvIf->uCmd.T4TCmd) = phNciNfc_eT4TRaw;
                    pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                    pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                    pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                    pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                }
                else
                {
                    status = NFCSTATUS_INVALID_PARAMETER;
                }
                break;
            }
            case phNciNfc_eFelica_PICC:
            {
                if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                    (0 != (pTransceiveInfo->sSendData.length)) &&
                    (NULL != pTransceiveInfo->sRecvData.buffer) &&
                    (0 != (pTransceiveInfo->sRecvData.length)) &&
                    (phNfc_eFelica_Raw == pTransceiveInfo->cmd.FelCmd)
                    )
                {
                    (pMappedTranscvIf->uCmd.T3TCmd) = phNciNfc_eT3TRaw;
                    pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                    pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                    pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                    pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                }
                else
                {
                    status = NFCSTATUS_INVALID_PARAMETER;
                }
                break;
            }
            case phNciNfc_eJewel_PICC:
            {
                status = phLibNfc_JewelMap(pTransceiveInfo,pMappedTranscvIf);
                break;
            }
            case phNciNfc_eNfcIP1_Target:
            {
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                break;
            }
            case phNciNfc_eISO15693_PICC:
            {
                status = phLibNfc_Iso15693Map(pTransceiveInfo,pMappedTranscvIf);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_MifareMap(phLibNfc_sTransceiveInfo_t*    pTransceiveInfo,
                                    pphNciNfc_TransceiveInfo_t   pMappedTranscvIf
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t bBuffIdx = 0;
    uint8_t bSectorNumber;
    uint8_t bKey = 0;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    switch(pTransceiveInfo->cmd.MfCmd)
    {
        case phNfc_eMifareRead16:
        {
            if( (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length)))
            {
                if(NULL != pLibContext)
                {
                    pLibContext->aSendBuff[bBuffIdx++] = phNfc_eMifareRead16;
                    pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->addr;

                    (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                    (pMappedTranscvIf->tSendData.wLen) = bBuffIdx;
                    (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                    (pMappedTranscvIf->tRecvData.pBuff) = pTransceiveInfo->sRecvData.buffer;
                    (pMappedTranscvIf->tRecvData.wLen) = (uint16_t)pTransceiveInfo->sRecvData.length;
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareReadN:
        {
            if( (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length)))
            {
                (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TreadN;
                (pMappedTranscvIf->bAddr) = pTransceiveInfo->addr;

                (pMappedTranscvIf->bNumBlock) = pTransceiveInfo->NumBlock;
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareWrite16:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)))
            {
                if(NULL != pLibContext)
                {
                    pLibContext->aSendBuff[bBuffIdx++] = phNfc_eMifareWrite16;
                    pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->addr;
                    phOsalNfc_MemCopy(&(pLibContext->aSendBuff[bBuffIdx]),pTransceiveInfo->sSendData.buffer,
                                      (pTransceiveInfo->sSendData.length));

                    (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                    (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx + (pTransceiveInfo->sSendData.length));
                    (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                    (pMappedTranscvIf->tRecvData.pBuff) = pLibContext->aRecvBuff;
                    (pMappedTranscvIf->tRecvData.wLen) = sizeof(pLibContext->aRecvBuff);
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareWrite4:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)))
            {
                if(NULL != pLibContext)
                {
                    pLibContext->aSendBuff[bBuffIdx++] = phNfc_eMifareWrite4;
                    pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->addr;
                    phOsalNfc_MemCopy(&(pLibContext->aSendBuff[bBuffIdx]),pTransceiveInfo->sSendData.buffer,
                                      (pTransceiveInfo->sSendData.length));

                    (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                    (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx + (pTransceiveInfo->sSendData.length));
                    (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                    (pMappedTranscvIf->tRecvData.pBuff) = pLibContext->aRecvBuff;
                    (pMappedTranscvIf->tRecvData.wLen) = sizeof(pLibContext->aRecvBuff);
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareWriteN:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)))
            {
                (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TWriteN;
                (pMappedTranscvIf->bAddr) = pTransceiveInfo->addr;
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareRaw:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)) &&
                (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length))
                )
            {
                (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
            break;
        }
        case phNfc_eMifareAuthentA:
        case phNfc_eMifareAuthentB:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length))
                )
            {
                if(NULL != pLibContext)
                {
                    status = phLibNfc_ChkAuthCmdMFC(pLibContext, pTransceiveInfo);

                    if(NFCSTATUS_SUCCESS == status)
                    {
                        bKey = bKey | PHLIBNFC_MFC_EMBEDDED_KEY;

                        bSectorNumber = pTransceiveInfo->addr;
                        phLibNfc_CalSectorAddress(&bSectorNumber);

                        if (phNfc_eMifareAuthentB == pTransceiveInfo->cmd.MfCmd) {
                            bKey = bKey | PH_LIBNFC_ENABLE_KEY_B;
                        }

                        /*For creating extension command header pTransceiveInfo's MfCmd get used*/
                        pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->cmd.MfCmd;
                        pLibContext->aSendBuff[bBuffIdx++] = bSectorNumber;
                        pLibContext->aSendBuff[bBuffIdx++] = bKey;

                        /* Copy the Dynamic key passed */
                        phOsalNfc_MemCopy(&pLibContext->aSendBuff[bBuffIdx],
                                          &pTransceiveInfo->sSendData.buffer[PHLIBNFC_MFCUIDLEN_INAUTHCMD],
                                          PHLIBNFC_MFC_AUTHKEYLEN);

                        bBuffIdx = bBuffIdx + PHLIBNFC_MFC_AUTHKEYLEN;

                        (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                        (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx /*+ (pTransceiveInfo->sSendData.length)*/);

                        pMappedTranscvIf->uCmd.T2TCmd = phNciNfc_eT2TAuth;
                        pMappedTranscvIf->bAddr = bSectorNumber;
                        pMappedTranscvIf->bNumBlock = pTransceiveInfo->NumBlock;
                        pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                        pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                    }
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        break;
        case phNfc_eMifareAuthKeyNumA: /**< prop - Authenticate Mifare Block with Key Type A using Key number*/
        case phNfc_eMifareAuthKeyNumB:
        {
            if(NULL != pLibContext)
            {
                bSectorNumber = pTransceiveInfo->addr;
                phLibNfc_CalSectorAddress(&bSectorNumber);

                if (phNfc_eMifareAuthKeyNumB == pTransceiveInfo->cmd.MfCmd) {
                    bKey = bKey | PH_LIBNFC_ENABLE_KEY_B;
                }

                /*For creating extension command header pTransceiveInfo's MfCmd get used*/
                pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->cmd.MfCmd;
                pLibContext->aSendBuff[bBuffIdx++] = bSectorNumber;
                pLibContext->aSendBuff[bBuffIdx++] = bKey | pTransceiveInfo->bKeyNum;

                (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx /*+ (pTransceiveInfo->sSendData.length)*/);

                pMappedTranscvIf->uCmd.T2TCmd = phNciNfc_eT2TAuth;
                pMappedTranscvIf->bAddr = bSectorNumber;
                pMappedTranscvIf->bNumBlock = pTransceiveInfo->NumBlock;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
            }
        }
        break;
        case phNfc_eMifareSectorSel:
        {
            /*There is no need to pass send buffer and receive buffer for sector select command
              Only block address is sufficient*/
            if( (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length)))
            {
                 pMappedTranscvIf->uCmd.T2TCmd = phNciNfc_eT2TSectorSel;
                 pMappedTranscvIf->bAddr = pTransceiveInfo->addr;

                 pMappedTranscvIf->bNumBlock = pTransceiveInfo->NumBlock;
                 pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                 pMappedTranscvIf->tSendData.wLen= (uint16_t)pTransceiveInfo->sSendData.length;
                 pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                 pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        break;
        case phNfc_eMifareInc:
        case phNfc_eMifareDec:
        case phNfc_eMifareRestore:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != pTransceiveInfo->sSendData.length))
            {
                if(NULL != pLibContext)
                {
                     pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->cmd.MfCmd;
                     pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->addr;

                     phOsalNfc_MemCopy(&(pLibContext->aSendBuff[bBuffIdx]),\
                                        (pTransceiveInfo->sSendData.buffer),\
                                        (pTransceiveInfo->sSendData.length));

                    (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                    (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx + (pTransceiveInfo->sSendData.length));
                    (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                    (pMappedTranscvIf->tRecvData.pBuff) = pTransceiveInfo->sRecvData.buffer;
                    (pMappedTranscvIf->tRecvData.wLen) = (uint16_t)pTransceiveInfo->sRecvData.length;
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        break;
        case phNfc_eMifareTransfer:
        {
            if((NULL != pTransceiveInfo->sRecvData.buffer) &&
               (0 != (pTransceiveInfo->sRecvData.length)))
            {
                if(NULL != pLibContext)
                {
                     pLibContext->aSendBuff[bBuffIdx++] = phNfc_eMifareTransfer;
                     pLibContext->aSendBuff[bBuffIdx++] = pTransceiveInfo->addr;

                    (pMappedTranscvIf->tSendData.pBuff) = pLibContext->aSendBuff;
                    (pMappedTranscvIf->tSendData.wLen) = (uint16_t)(bBuffIdx);
                    (pMappedTranscvIf->uCmd.T2TCmd) = phNciNfc_eT2TRaw;
                    (pMappedTranscvIf->tRecvData.pBuff) = pTransceiveInfo->sRecvData.buffer;
                    (pMappedTranscvIf->tRecvData.wLen) = (uint16_t)pTransceiveInfo->sRecvData.length;
                }
            }
            else
            {
                status = NFCSTATUS_INVALID_PARAMETER;
            }
        }
        break;
        default:
        {
            PH_LOG_LIBNFC_CRIT_STR("Unknown Mifare command: 0x%02X", pTransceiveInfo->cmd.MfCmd);
            status = NFCSTATUS_INVALID_PARAMETER;
            break;
        }
    }

    if((NULL != pLibContext) && (status == NFCSTATUS_SUCCESS))
    {
        pLibContext->bLastCmdSent = (pMappedTranscvIf->uCmd.T2TCmd);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_Iso15693Map(phLibNfc_sTransceiveInfo_t*    pTransceiveInfo,
                                      pphNciNfc_TransceiveInfo_t   pMappedTranscvIf)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    switch(pTransceiveInfo->cmd.Iso15693Cmd)
    {
        case phNfc_eIso15693_Raw:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)) &&
                (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length))
                )
            {
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        }
        case phNfc_eIso15693_Cmd:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)) &&
                (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length))
                )
            {
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                /* Modify the Request Flags to enable High data rate bit */
                pMappedTranscvIf->tSendData.pBuff[0] = \
                    (pMappedTranscvIf->tSendData.pBuff[0] | PH_LIBNFC_15693_ENABLE_HIGH_DATARATE);
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        }
        default:
        {
            PH_LOG_LIBNFC_CRIT_STR("Error: Iso15693 command not defined");
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_JewelMap(phLibNfc_sTransceiveInfo_t*    pTransceiveInfo,
                                   pphNciNfc_TransceiveInfo_t   pMappedTranscvIf
    )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    switch(pTransceiveInfo->cmd.JewelCmd)
    {
        case phNfc_eJewel_WriteN:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)))
            {
                (pMappedTranscvIf->uCmd.T1TCmd) = phNciNfc_eT1TWriteN;
                (pMappedTranscvIf->bAddr) = pTransceiveInfo->addr;
                (pMappedTranscvIf->bNumBlock) = pTransceiveInfo->NumBlock;
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        }
        case phNfc_eJewel_Raw:
        {
            if( (NULL != pTransceiveInfo->sSendData.buffer) &&
                (0 != (pTransceiveInfo->sSendData.length)) &&
                (NULL != pTransceiveInfo->sRecvData.buffer) &&
                (0 != (pTransceiveInfo->sRecvData.length))
                )
            {
                (pMappedTranscvIf->uCmd.T1TCmd) = phNciNfc_eT1TRaw;
                pMappedTranscvIf->tSendData.pBuff = pTransceiveInfo->sSendData.buffer;
                pMappedTranscvIf->tRecvData.pBuff = pTransceiveInfo->sRecvData.buffer;
                pMappedTranscvIf->tSendData.wLen = (uint16_t)pTransceiveInfo->sSendData.length;
                pMappedTranscvIf->tRecvData.wLen = (uint16_t)pTransceiveInfo->sRecvData.length;
                wStatus = NFCSTATUS_SUCCESS;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_ClearNdefInfo(phLibNfc_NdefInfo_t *pNdefCtx)
{
    if(NULL != pNdefCtx)
    {
        /* Clear the Check Ndef status flag */
        pNdefCtx->is_ndef = PH_LIBNFC_INTERNAL_CHK_NDEF_NOT_DONE;
    }
    return;
}

NFCSTATUS phLibNfc_VerifyResponse(pphNciNfc_Data_t pTransactInfo,
                                  pphNciNfc_RemoteDevInformation_t NciRemoteDevHandle)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL == pTransactInfo)
    {
        status = NFCSTATUS_INVALID_PARAMETER;
    }
    else
    {
        switch(NciRemoteDevHandle->RemDevType)
        {
            case phNciNfc_eJewel_PICC:
            case phNciNfc_eFelica_PICC:
            {
                if(0x00 == pTransactInfo->wLen)
                {
                    status = NFCSTATUS_RESPONSE_TIMEOUT;
                }
                else
                {
                    status = NFCSTATUS_SUCCESS;
                }
            }
            break;
            default:
            {
                switch(pLibContext->bLastCmdSent)
                {
                    case phNfc_eMifareWrite4:
                    case phNfc_eMifareWrite16:
                    {
                        if((1 == pTransactInfo->wLen) && 
                           (MIFARE_WRITE_ACK == pTransactInfo->pBuff[0]))
                        {
                            status = NFCSTATUS_SUCCESS;
                            pTransactInfo->wLen = 0;
                            pTransactInfo->pBuff[0] = 0x00;
                        }
                        break;
                    }
                    default:
                    {
                        status = NFCSTATUS_SUCCESS;
                        break;
                    }
                }
            }
            break;
        }
    }
    if(NULL != pLibContext)
    {
        if(NFCSTATUS_PENDING != status)
        {
            /* Todo :- Update/Add a generic cmd type below */
            pLibContext->bLastCmdSent = phNfc_eMifareInvalidCmd;
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

NFCSTATUS phLibNfc_Idle2InitTransition(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtx = pContext;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        PHLIBNFC_INIT_SEQUENCE(pCtx,gphLibNfc_InitializeSequence);
        wStatus = phLibNfc_SeqHandler(pContext,NFCSTATUS_SUCCESS,NULL);
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Init2Reset(void *pContext, void *Param1, void *Param2, void *Param3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = pContext;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL!=pCtx)
    {
        wStatus = phNciNfc_Reset(pCtx->sHwReference.pNciHandle,
                                 phNciNfc_NciReset_DeInit_KeepConfig,
                                 &phLibNfc_ShutdownCb,
                                 (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Actv2Reset(void *pContext, void *Param1, void *Param2, void *Param3)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = pContext;
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL!=pCtx)
    {
        wStatus = phNciNfc_Reset(pCtx->sHwReference.pNciHandle,
                                 phNciNfc_NciReset_DeInit_KeepConfig,
                                 &phLibNfc_ShutdownCb,
                                 (void *)pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Actv2Idle(void *pContext, void *Param1, void *Param2, void *Param3)
{
    UNUSED(pContext);
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phLibNfc_Discovered2Discovery(void *pContext, void *Param1, void *Param2, void *Param3)
{
    UNUSED(pContext);
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phLibNfc_Discovered2Transceive(void *pContext, void *Param1, void *Param2, void *Param3)
{
    pphLibNfc_Context_t pCtxt = (pphLibNfc_Context_t )pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevInfo=(pphNciNfc_RemoteDevInformation_t)Param1;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint16_t wRetVal;

    // Warning C4305: 'type cast': truncation from 'void *' to 'phNciNfc_RfInterfaces_t'
#pragma warning(suppress:4305)
    phNciNfc_RfInterfaces_t eRfInterface = (phNciNfc_RfInterfaces_t)Param2;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if((NULL != pCtxt) &&(NULL != pRemoteDevInfo) && (NULL != Param3))
    {
        /*Check if device is already activated*/
        if(pRemoteDevInfo->SessionOpened == 1)
        {
            if(phNciNfc_e_RfProtocolsT3tProtocol == pRemoteDevInfo->eRFProtocol)
            {
                PHLIBNFC_INIT_SEQUENCE(pCtxt,gphLibNfc_Felica_ConnectSeq);
                wStatus = phLibNfc_SeqHandler(pCtxt,wStatus,NULL);
            }
            else if((phNciNfc_e_RfProtocolsIsoDepProtocol == pRemoteDevInfo->eRFProtocol) &&
                    (1 == pCtxt->Config.bIsoDepPresChkCmd))
            {
                PHLIBNFC_INIT_SEQUENCE(pCtxt,gphLibNfc_IsoDep_ConnectSeq);
                wStatus = phLibNfc_SeqHandler(pCtxt,wStatus,NULL);
            }
            else if((phNciNfc_e_RfProtocolsNfcDepProtocol == pRemoteDevInfo->eRFProtocol) ||
                    (NFCSTATUS_SUCCESS == phLibNfc_ChkMfCTag(pRemoteDevInfo)) ||
                    pCtxt->bDtaFlag)
            {
                wRetVal = phOsalNfc_QueueDeferredCallback(phLibNfc_RemoteDev_ConnectTimer_Cb,
                                                          pCtxt);
                if(wRetVal == 0x00)
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
                wStatus = phLibNfc_InternConnect(pContext,Param1,Param3);
            }
        }
        else
        {
            /*Call Nci connect to connect to target*/
            wStatus = phNciNfc_Connect(pCtxt->sHwReference.pNciHandle,
                                       (pphNciNfc_RemoteDevInformation_t)Param1,
                                       eRfInterface,
                                       &phLibNfc_RemoteDev_Connect_Cb,
                                       (void *)pCtxt);
        }
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phLibNfc_Send2Discovered(void *pContext, void *Param1, void *Param2, void *Param3)
{
    UNUSED(pContext);
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phLibNfc_Recv2Discovered(void *pContext, void *Param1, void *Param2, void *Param3)
{
    UNUSED(pContext);
    UNUSED(Param1);
    UNUSED(Param2);
    UNUSED(Param3);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return NFCSTATUS_SUCCESS;
}

void phLibNfc_UpdateEvent(NFCSTATUS wStatus,phLibNfc_Event_t *pTrigEvent)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    switch(wStatus)
    {
    case NFCSTATUS_BOARD_COMMUNICATION_ERROR:
        *pTrigEvent = phLibNfc_EventBoardError;
        break;
    case NFCSTATUS_RESPONSE_TIMEOUT:
        *pTrigEvent = phLibNfc_EventTimeOut;
        break;
    case NFCSTATUS_FAILED:
        *pTrigEvent = phLibNfc_EventFailed;
        break;
    case NFCSTATUS_SUCCESS:
        *pTrigEvent = phLibNfc_EventReqCompleted;
        break;
    default:
        break;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_ValidateDevHandle(phLibNfc_Handle    hRemoteDevice)
{
    NFCSTATUS wStatus;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibContext->Connected_handle == NULL)
    {
        PH_LOG_LIBNFC_CRIT_STR("Target not connected");
        wStatus = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if(pLibContext->Connected_handle == (void *)hRemoteDevice)
    {
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

void phLibNfc_MfcAuthInfo_Clear(pphLibNfc_LibContext_t pLibContext)
{
    pphLibNfc_LibContext_t pLibCtx = NULL;
    if(NULL != pLibContext)
    {
        pLibCtx = (pphLibNfc_LibContext_t) pLibContext;
        pLibCtx->tMfcInfo.cmd = phNfc_eMifareInvalidCmd;
        pLibCtx->tMfcInfo.addr= 0 ;
        pLibCtx->tMfcInfo.key = 0;
        phOsalNfc_SetMemory(pLibCtx->tMfcInfo.MFCKey, 0x00, sizeof(pLibCtx->tMfcInfo.MFCKey));
    }
    return ;
}

static void phLibNfc_RemoteDev_ClearInfo(void )
{
    uint8_t bIndex;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    if(pLibContext != NULL)
    {
        phLibNfc_MfcAuthInfo_Clear(pLibContext);
        pLibContext->Connected_handle = NULL;
        pLibContext->DummyConnect_handle = NULL;

        for(bIndex =0 ;bIndex<MAX_REMOTE_DEVICES;bIndex++)
        {
            pLibContext->Disc_handle[bIndex]=NULL;
        }

        if(pLibContext->psRemoteDevList[0].psRemoteDevInfo != NULL)
        {
             phOsalNfc_FreeMemory((void *)pLibContext->psRemoteDevList[0].psRemoteDevInfo);
        }

        for(bIndex=0;bIndex<MAX_REMOTE_DEVICES;bIndex++)
        {
            pLibContext->psRemoteDevList[bIndex].hTargetDev=(phLibNfc_Handle)NULL;
            pLibContext->psRemoteDevList[bIndex].psRemoteDevInfo=NULL;
        }

        for(bIndex = 0;bIndex<MAX_REMOTE_DEVICES;bIndex++)
        {
            pLibContext->Map_Handle[bIndex].pLibNfc_RemoteDev_List=NULL;
            pLibContext->Map_Handle[bIndex].pNci_RemoteDev_List=NULL;
        }

        pLibContext->pInfo=NULL;
        pLibContext->psRemoteDevInfo=NULL;

        pLibContext->dev_cnt=0;
        pLibContext->bTotalNumDev=0;   /*Total number of discovery notifications*/
        pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_DEACTSELECT;

        phOsalNfc_SetMemory((void *)(&pLibContext->DiscoverdNtfType),0x00,sizeof(phLibNfc_Discover_Info_t));
    }
}

NFCSTATUS 
phLibNfc_MapBitRate(
    _In_ phNciNfc_BitRates_t NciBitRate, 
    _Out_ phNfc_eDataRate_t *pLibNfcBitRate
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibNfcBitRate)
    {
        switch(NciBitRate)
        {
        case phNciNfc_e_BitRate106:
            *pLibNfcBitRate = phNfc_eDataRate_106;
            break;
        case phNciNfc_e_BitRate212:
            *pLibNfcBitRate = phNfc_eDataRate_212;
            break;
        case phNciNfc_e_BitRate424:
            *pLibNfcBitRate = phNfc_eDataRate_424;
            break;
        case phNciNfc_e_BitRate848:
            *pLibNfcBitRate = phNfc_eDataRate_848;
            break;
        case phNciNfc_e_BitRate1696:
            *pLibNfcBitRate = phNfc_eDataRate_1696;
            break;
        case phNciNfc_e_BitRate3392:
            *pLibNfcBitRate = phNfc_eDataRate_3392;
            break;
        case phNciNfc_e_BitRate26:
            *pLibNfcBitRate = phNfc_eDataRate_26;
            break;
        default:
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            break;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_Invoke_Pending_Cb(void * pContext, NFCSTATUS status)
{
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_ChkNdef_Info_t tNdef_Info;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibContext)
    {
        if(NULL != pLibContext->CBInfo.pClientInitCb)
        {
            pLibContext->CBInfo.pClientInitCb(pLibContext->CBInfo.pClientInitCntx, pLibContext->eConfigStatus, status);
            pLibContext->CBInfo.pClientInitCb = NULL;
            pLibContext->CBInfo.pClientInitCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientConnectCb)
        {
            pLibContext->CBInfo.pClientConnectCb(pLibContext->CBInfo.pClientConCntx,NULL,NULL,status);
            pLibContext->CBInfo.pClientConnectCb = NULL;
            pLibContext->CBInfo.pClientConCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientTranscvCb)
        {
            pLibContext->CBInfo.pClientTranscvCb(pLibContext->CBInfo.pClientTranscvCntx, NULL, NULL,
                                                status);
            pLibContext->CBInfo.pClientTranscvCb = NULL;
            pLibContext->CBInfo.pClientTranscvCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientDisConnectCb)
        {
            pLibContext->CBInfo.pClientDisConnectCb(pLibContext->CBInfo.pClientDConCntx, NULL, status);
            pLibContext->CBInfo.pClientDisConnectCb = NULL;
            pLibContext->CBInfo.pClientDConCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientDisConfigCb)
        {
            pLibContext->CBInfo.pClientDisConfigCb(pLibContext->CBInfo.pClientDisCfgCntx, status);
            pLibContext->CBInfo.pClientDisConfigCb = NULL;
            pLibContext->CBInfo.pClientDisCfgCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientCkNdefCb)
        {
            tNdef_Info.ActualNdefMsgLength = 0;
            tNdef_Info.MaxNdefMsgLength = 0;
            tNdef_Info.NdefCardState = PHLIBNFC_NDEF_CARD_INVALID;

            pLibContext->CBInfo.pClientCkNdefCb(pLibContext->CBInfo.pClientCkNdefCntx,tNdef_Info, status);
            pLibContext->CBInfo.pClientCkNdefCb = NULL;
            pLibContext->CBInfo.pClientCkNdefCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientTransceiveCb)
        {
            pLibContext->CBInfo.pClientTransceiveCb(pLibContext->CBInfo.pClientTranseCntx, NULL,NULL, status);
            pLibContext->CBInfo.pClientTransceiveCb = NULL;
            pLibContext->CBInfo.pClientTranseCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientRdNdefCb)
        {
            pLibContext->CBInfo.pClientRdNdefCb(pLibContext->CBInfo.pClientRdNdefCntx, status);
            pLibContext->CBInfo.pClientRdNdefCb = NULL;
            pLibContext->CBInfo.pClientRdNdefCntx= NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientWrNdefCb)
        {
            pLibContext->CBInfo.pClientWrNdefCb(pLibContext->CBInfo.pClientWrNdefCntx, status);
            pLibContext->CBInfo.pClientWrNdefCb = NULL;
            pLibContext->CBInfo.pClientWrNdefCntx= NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientNdefNtfRespCb)
        {
            pLibContext->CBInfo.pClientNdefNtfRespCb(pLibContext->CBInfo.pClientNdefNtfRespCntx, NULL,NULL,status);
            pLibContext->CBInfo.pClientNdefNtfRespCb = NULL;
            pLibContext->CBInfo.pClientNdefNtfRespCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientPresChkCb)
        {
            pLibContext->CBInfo.pClientPresChkCb(pLibContext->CBInfo.pClientPresChkCntx, status);
            pLibContext->CBInfo.pClientPresChkCb = NULL;
            pLibContext->CBInfo.pClientPresChkCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientRoutingCfgCb)
        {
            pLibContext->CBInfo.pClientRoutingCfgCb(pLibContext->CBInfo.pClientRoutingCfgCntx, status);
            pLibContext->CBInfo.pClientRoutingCfgCb = NULL;
            pLibContext->CBInfo.pClientRoutingCfgCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientNfcIpCfgCb)
        {
            pLibContext->CBInfo.pClientNfcIpCfgCb(pLibContext->CBInfo.pClientNfcIpCfgCntx, status);
            pLibContext->CBInfo.pClientNfcIpCfgCb = NULL;
            pLibContext->CBInfo.pClientNfcIpCfgCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientNfcIpTxCb)
        {
            pLibContext->CBInfo.pClientNfcIpTxCb(pLibContext->CBInfo.pClientNfcIpTxCntx, status);
            pLibContext->CBInfo.pClientNfcIpTxCb = NULL;
            pLibContext->CBInfo.pClientNfcIpTxCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pClientNfcIpRxCb)
        {
            pLibContext->CBInfo.pClientNfcIpRxCb(pLibContext->CBInfo.pClientNfcIpRxCntx, NULL, status);
            pLibContext->CBInfo.pClientNfcIpRxCb = NULL;
            pLibContext->CBInfo.pClientNfcIpRxCntx = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pSeSetModeCb)
        {
            pLibContext->CBInfo.pSeSetModeCb(pLibContext->CBInfo.pSeSetModeCtxt, NULL, status);
            pLibContext->CBInfo.pSeSetModeCb = NULL;
            pLibContext->CBInfo.pSeSetModeCtxt = NULL;
        }
        else if(NULL != pLibContext->CBInfo.pNFCEEDiscoveryCb)
        {
            pLibContext->CBInfo.pNFCEEDiscoveryCb(pLibContext->CBInfo.pNFCEEDiscoveryCntx, status);
            pLibContext->CBInfo.pNFCEEDiscoveryCb = NULL;
            pLibContext->CBInfo.pNFCEEDiscoveryCntx = NULL;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_GetConnectedHandle(phLibNfc_Handle *pHandle)
{
    phLibNfc_sRemoteDevInformation_t *pRemDev = NULL;
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    if(NULL != pLibContext)
    {
        wStatus = phLibNfc_MapRemoteDevHandle(&pRemDev,
                                                (phNciNfc_RemoteDevInformation_t**)&pLibContext->Connected_handle,
                                                PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            *pHandle = (phLibNfc_Handle)pRemDev;
        }
        else
        {
            *pHandle = NULL;
        }
    }
    return wStatus;
}

NFCSTATUS phLibNfc_ChkMfCTag(pphNciNfc_RemoteDevInformation_t RemoteDevInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    uint8_t bSak = 0x00;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != RemoteDevInfo) &&\
       (RemoteDevInfo->eRFTechMode == phNciNfc_NFCA_Poll) &&\
       (RemoteDevInfo->eRFProtocol == phNciNfc_e_RfProtocolsMifCProtocol))
    {
        bSak = RemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.Sak;

        switch(bSak)
        {
        case PHLIBNFC_MFC1K_SAK:
        case PHLIBNFC_MFC4K_SAK:
        case PHLIBNFC_MFCMINI_SAK:
        case PHLIBNFC_MFC1K_WITHSAK1:
        case PHLIBNFC_MFC1K_WITHSAK88:
        case PHLIBNFC_MFC4K_WITHSAK98:
        case PHLIBNFC_MFC4K_WITHSAKB8:
        case PHLIBNFC_MFC1K_WITHSAK28:
        case PHLIBNFC_MFC4K_WITHSAK38:
            wStatus = NFCSTATUS_SUCCESS;
            break;

        default:
            wStatus = NFCSTATUS_FAILED;
            break;
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_ListenModeDeactvNtfHandler(pphLibNfc_LibContext_t pLibNfcHandle)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pLibNfcHandle)
    {
        if(pLibNfcHandle->dev_cnt > 0)
        {
            PH_LOG_LIBNFC_INFO_U32MSG("pLibNfcHandle->dev_cnt", pLibNfcHandle->dev_cnt);
            if(phNfc_eNfcIP1_Initiator == pLibNfcHandle->psRemoteDevList->psRemoteDevInfo->RemDevType)
            {
                PH_LOG_LIBNFC_INFO_STR("RemDevType -> phNfc_eNfcIP1_Initiator");
                if(NULL != pLibNfcHandle->CBInfo.pClientNfcIpRxCb)
                {
                    PH_LOG_LIBNFC_INFO_STR("pClientNfcIpRxCb is valid, invoking...");
                    /* Receive in progress, Notify application through Receive call back function */
                    pLibNfcHandle->CBInfo.pClientNfcIpRxCb(
                            pLibNfcHandle->CBInfo.pClientNfcIpRxCntx,NULL,NFCSTATUS_TARGET_LOST);
                    pLibNfcHandle->CBInfo.pClientNfcIpRxCb = NULL;
                    pLibNfcHandle->CBInfo.pClientNfcIpRxCntx = NULL;
                }
                else if(NULL != pLibNfcHandle->CBInfo.pClientNfcIpTxCb)
                {
                    PH_LOG_LIBNFC_INFO_STR("pClientNfcIpTxCb is valid, invoking...");
                    /* Send in progress, Notify application through Send call back function */
                    pLibNfcHandle->CBInfo.pClientNfcIpTxCb(
                        pLibNfcHandle->CBInfo.pClientNfcIpTxCntx,NFCSTATUS_TARGET_LOST);
                    pLibNfcHandle->CBInfo.pClientNfcIpTxCb = NULL;
                    pLibNfcHandle->CBInfo.pClientNfcIpTxCntx = NULL;
                }
                /* Notify application through Register listener call back function */
                else if(NULL != pLibNfcHandle->CBInfo.pClientNtfRegRespCB)
                {
                    PH_LOG_LIBNFC_INFO_STR("No Rx or Tx call backs found, invoking pClientNtfRegRespCB");
                    /*Only incase case of Register listner, return status should be NFCSTATUS_DESELECTED*/
                    pLibNfcHandle->CBInfo.pClientNtfRegRespCB(pLibNfcHandle->CBInfo.pClientNtfRegRespCntx,
                                NULL, 0, NFCSTATUS_DESELECTED);
                }
            }
            else
            {
                PH_LOG_LIBNFC_INFO_U32MSG("pLibNfcHandle->dev_cnt == 1 & SE",pLibNfcHandle->dev_cnt);
                if(NULL != pLibNfcHandle->CBInfo.pClientNtfRegRespCB)
                {
                    PH_LOG_LIBNFC_INFO_STR("Invoking pClientNtfRegRespCB");
                    pLibNfcHandle->CBInfo.pClientNtfRegRespCB((void*)pLibNfcHandle->CBInfo.pClientNtfRegRespCntx,
                              pLibNfcHandle->psRemoteDevList,
                              pLibNfcHandle->dev_cnt,
                              NFCSTATUS_DESELECTED);
                }
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

/*This function shall send the RATS request to activate the target at 4A level */
static NFCSTATUS
phLibNfc_SendRATSReq(void *pContext,NFCSTATUS status,void *pInfo)
{
    pphLibNfc_LibContext_t pLibCtx = (pphLibNfc_LibContext_t)pContext;
    phNciNfc_TransceiveInfo_t tTrancvIf;
    uint8_t bRATSCmdBuff[2];
    static uint8_t bRATSRespBuff[20];
    uint8_t bDidVal = 0;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    UNUSED(pInfo);

    if(NULL != pLibCtx)
    {
        bRATSCmdBuff[0] = PHLIBNFC_4ARATS_START_BYTE;
        /* Considering only 1 active target,DID value of 0 is used currently for RATS request
           [TODO] could be extended to use any number between 0 & 14 as defined by NFC DIGITAL */
        bRATSCmdBuff[1] = ((PHLIBNFC_4ARATS_FSD_VAL1 << 4) | bDidVal);

        tTrancvIf.tSendData.pBuff = bRATSCmdBuff;
        tTrancvIf.tSendData.wLen = sizeof(bRATSCmdBuff);
        tTrancvIf.tRecvData.pBuff = bRATSRespBuff;
        tTrancvIf.tRecvData.wLen = sizeof(bRATSRespBuff);
        tTrancvIf.wTimeout = 500;

        status = phLibNfc_NciTranscv(pLibCtx->sHwReference.pNciHandle,
                              pLibCtx->Connected_handle,
                              &tTrancvIf,
                              (void *)pLibCtx);
    }
    else
    {
        status = NFCSTATUS_INVALID_PARAMETER;
        PH_LOG_LIBNFC_INFO_STR("Invalid context Param!!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS
phLibNfc_ProcessATSRsp(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_LibContext_t pLibCtx = (pphLibNfc_LibContext_t)pContext;
    pphNciNfc_Data_t pRspBuff = (pphNciNfc_Data_t) pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NFCSTATUS_SUCCESS == wStatus)
    {
        if(NULL != pLibCtx && ((NULL != pRspBuff->pBuff) && (0 != pRspBuff->wLen)))
        {
            PH_LOG_LIBNFC_INFO_X32MSG("Received ATS response of length",pRspBuff->wLen);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
            PH_LOG_LIBNFC_CRIT_STR("Invalid Param(s) received!!");
        }
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
        PH_LOG_LIBNFC_CRIT_STR("RATS request failed!!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_ReActivateComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS   wRemDevInfoStat = NFCSTATUS_FAILED, wStatus = status;
    NFCSTATUS    wRetVal = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;
    pphLibNfc_ConnectCallback_t   ps_client_con_cb;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevInfo=NULL;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle=NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if( pLibContext == phLibNfc_GetContext())
    {
        (pLibContext->tSelInf.bSelectInpAvail) = FALSE;
        if(pInfo != NULL)
        {
            if (NFCSTATUS_SUCCESS == PHNFCSTATUS(status))
            {
                PH_LOG_LIBNFC_INFO_STR("Lower layer has returned NFCSTATUS_SUCCESS");
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Buffer passed by Lower layer is NULL");
                wStatus = NFCSTATUS_FAILED;
            }
        }

        if(NFCSTATUS_SUCCESS != wStatus)
        {
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_ONLYSELECT;
        }
        else
        {
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_DEACTSELECT;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        wRetVal = phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

        if(NFCSTATUS_SUCCESS != wRetVal)
        {
            wStatus = NFCSTATUS_FAILED;
        }

        if(NULL != pLibContext->Connected_handle)
        {
            pNciRemoteDevHandle = pLibContext->Connected_handle;
            wRemDevInfoStat = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(wRemDevInfoStat != NFCSTATUS_SUCCESS)
            {
                PH_LOG_LIBNFC_CRIT_STR("Mapping on NCI RemoteDev Handle to LibNfc RemoteDev Handle Failed");
                wRemDevInfoStat = NFCSTATUS_FAILED;
            }
            else
            {
                wRemDevInfoStat = phLibNfc_GetRemoteDevInfo(pLibRemoteDevHandle,&pLibRemoteDevInfo);
                if(wRemDevInfoStat != NFCSTATUS_SUCCESS)
                {
                    PH_LOG_LIBNFC_CRIT_STR("Getting LibNfc RemoteDev Info by using LibNfc RemoteDev Handle Failed");
                    wRemDevInfoStat = NFCSTATUS_FAILED;
                }
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if((NFCSTATUS_SUCCESS == wRemDevInfoStat) && (NULL != pLibRemoteDevHandle) && (NULL != pNciRemoteDevHandle))
            {
                pLibRemoteDevHandle->SessionOpened = pNciRemoteDevHandle->SessionOpened;
            }
            else
            {
                wStatus = wRemDevInfoStat;
            }

            ps_client_con_cb = pLibContext->CBInfo.pClientConnectCb;
            if (NULL != pLibContext->CBInfo.pClientConnectCb)
            {
                pLibContext->CBInfo.pClientConnectCb = NULL;
                PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
                ps_client_con_cb(pLibContext->CBInfo.pClientConCntx,
                                 (phLibNfc_Handle)pLibRemoteDevHandle,
                                 pLibRemoteDevInfo,
                                 wStatus);
            }
        }
        else
        {
            /* if(PHNFCSTATUS(status)==NFCSTATUS_INVALID_REMOTE_DEVICE) */
            /* If remote device is invalid return as TARGET LOST to upper layer*/
            /* If error code is other than SUCCESS return NFCSTATUS_TARGET_LOST */

            PH_LOG_LIBNFC_CRIT_STR("phLibNfc_ReActivateComplete: Lower layer has returned NFCSTATUS_FAILED");
            wStatus = NFCSTATUS_FAILED;
            pInfo = NULL;

            /*In case of Reactivation failed then If user if execute Deactivate with Discovery
            the NCI state machine will reject that,In order to prevent this make session opened as 0
            and then in discovery check if this session opened is 0
            then call deactivate to idle then discovery*/
            if((NFCSTATUS_SUCCESS == wRemDevInfoStat) && (NULL != pLibRemoteDevHandle))
            {
                pLibRemoteDevHandle->SessionOpened = 0;
            }

            ps_client_con_cb = pLibContext->CBInfo.pClientConnectCb;
            if (NULL != pLibContext->CBInfo.pClientConnectCb)
            {
                pLibContext->CBInfo.pClientConnectCb = NULL;
                PH_LOG_LIBNFC_INFO_STR("Invoking upper layer callback");
                ps_client_con_cb(pLibContext->CBInfo.pClientConCntx,
                                 (phLibNfc_Handle)pInfo,
                                 pLibRemoteDevInfo,
                                 wStatus);

            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Lower layer has returned Null LibNfc context");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_ReActivateMFCComplete1(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    NFCSTATUS wRetVal = NFCSTATUS_SUCCESS;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;
    uint8_t bDummyData = 1;
    phNfc_sData_t tResData;
    tResData.buffer = &bDummyData;
    tResData.length = 1;
    UNUSED(pInfo);
    tResData.buffer = NULL;
    tResData.length = 0;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if( pLibContext == phLibNfc_GetContext())
    {
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_DEACTSELECT;
            PH_LOG_LIBNFC_INFO_STR("Reactivation of Mifare classic Success");
        }
        else
        {
            /*If reactivation failed set this flag*/
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_ONLYSELECT;
            /*In case of check presense FAILED wstatus is RESPONSE_TIMEOUT*/
            wStatus = NFCSTATUS_RESPONSE_TIMEOUT;
            PH_LOG_LIBNFC_CRIT_STR("Reactivation of Mifare classic failed!");
        }

         phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
         wRetVal = phLibNfc_StateHandler((pphLibNfc_LibContext_t)pContext, TrigEvent, NULL, NULL, NULL);

         if(NFCSTATUS_SUCCESS != wRetVal)
         {
             wStatus = NFCSTATUS_FAILED;
         }
         phLibNfc_RemoteDev_ChkPresence_Cb(pContext,&tResData,wStatus);
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid LibNfc Context passed by lower layer");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phLibNfc_ReActivateMFCComplete(void *pContext,NFCSTATUS wStatus,void *pInfo)
{
    NFCSTATUS wIntStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t)pContext;
    pphLibNfc_TransceiveCallback_t pClientCb=NULL;
    void *pUpperLayerContext=NULL;
    phNfc_sData_t tResBuffer;
    phLibNfc_Event_t TrigEvent = phLibNfc_EventReqCompleted;
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle;
    phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle=NULL;
    uint8_t bDummyData = 1;
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        /*If Reactivation Failed set this flag*/
        if(NFCSTATUS_SUCCESS != wStatus)
        {
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_ONLYSELECT;
        }
        else
        {
            pLibContext->bReactivation_Flag = PH_LIBNFC_REACT_DEACTSELECT;
        }

        phLibNfc_UpdateEvent(PHNFCSTATUS(wStatus),&TrigEvent);
        wIntStatus = phLibNfc_StateHandler(pLibContext, TrigEvent, pInfo, NULL, NULL);

        if( (NFCSTATUS_SUCCESS == wIntStatus) && (NFCSTATUS_SUCCESS == wStatus))
        {
            pNciRemoteDevHandle =(pphNciNfc_RemoteDevInformation_t)pLibContext->Connected_handle;

            wIntStatus = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,&pNciRemoteDevHandle,PH_LIBNFC_INTERNAL_NCITOLIB_MAP);

            if(wIntStatus != NFCSTATUS_SUCCESS)
            {
                wStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            wStatus = NFCSTATUS_FAILED;
        }

        pClientCb = pLibContext->CBInfo.pClientTranscvCb;
        pUpperLayerContext = pLibContext->CBInfo.pClientTranscvCntx;
        pLibContext->CBInfo.pClientTranscvCb = NULL;
        pLibContext->CBInfo.pClientTranscvCntx = NULL;
        if(NULL != pClientCb)
        {
            phOsalNfc_SetMemory((void*)&tResBuffer,0x00,sizeof(phNfc_sData_t));

            /*Reactivation is SUCCESS but previous transceive operation is FAILED*/
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = NFCSTATUS_FAILED;
            }

            tResBuffer.buffer = &bDummyData;
            tResBuffer.length = 1;

            (*pClientCb)(pUpperLayerContext,(phLibNfc_Handle)pLibRemoteDevHandle,
                     &tResBuffer,wStatus);
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid libnfc context received from lower layer!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_MifareULDistinguishSequence(void *pContext, NFCSTATUS status, pphNciNfc_Data_t pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NFCSTATUS_SUCCESS != status)
    {
        PH_LOG_LIBNFC_WARN_STR("Mifare Ultralight command failed. This may be expected in order to distinguish card: %!NFCSTATUS!",
                               status);

        /* Set pInfo->pBuff to NULL so that process function knows the command failed */
        if (NULL != pInfo)
        {
            pInfo->pBuff = NULL;
            pInfo->wLen = 0;
        }
    }

    /* Pass NFCSTATUS_SUCCESS to phLibNfc_SeqHandler so that it will not skip to the end of the sequence */
    phLibNfc_SeqHandler(pContext, NFCSTATUS_SUCCESS, pInfo);

    PH_LOG_LIBNFC_FUNC_EXIT();
}

static NFCSTATUS phLibNfc_MifareULSendGetVersionCmd(void *pContext, NFCSTATUS status, void *pInfo)
{
    static const uint8_t c_getVersionCmd[] = { 0x60 };
    static const uint16_t c_timeoutMillis = 50;

    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_TransceiveInfo_t transceiveInfo = { 0 };
    phNciNfc_DeviceInfo_t* pDeviceInfo = NULL;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pCtx)
    {
        phOsalNfc_MemCopy(pCtx->aSendBuff, c_getVersionCmd, sizeof(c_getVersionCmd));
        transceiveInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
        transceiveInfo.tSendData.pBuff = pCtx->aSendBuff;
        transceiveInfo.tSendData.wLen = sizeof(c_getVersionCmd);
        transceiveInfo.tRecvData.pBuff = pCtx->aRecvBuff;
        transceiveInfo.tRecvData.wLen = sizeof(pCtx->aRecvBuff);
        transceiveInfo.wTimeout = c_timeoutMillis;

        pDeviceInfo = (phNciNfc_DeviceInfo_t*)pCtx->pInfo;
        wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                      pDeviceInfo->pRemDevList[0],
                                      &transceiveInfo,
                                      &phLibNfc_MifareULDistinguishSequence,
                                      pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULProcessGetVersionResp(void *pContext, NFCSTATUS status, void *pInfo)
{
    static const uint16_t c_expectedGetVersionResponseLength = 8;
    static const int c_dataAreaSizeIndex = 6;
    static const uint8_t c_mf0Ul11DataAreaSizeByte = 0x0B; /* Data area size byte for MF0UL11 Ultralight EV1 card */
    static const uint8_t c_mf0Ul11DataAreaSize = 48; /* Data area size for MF0UL11 Ultralight EV1 card */

    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t* pRecvData = (phNciNfc_Data_t*)pInfo;
    phNciNfc_DeviceInfo_t* pDeviceInfo = NULL;
    phNciNfc_RemoteDevInformation_t* pRemoteDevInfo = NULL;
    uint8_t dataAreaSizeByte = 0;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == status))
    {
        if ((NULL != pRecvData) &&
            (NULL != pRecvData->pBuff) &&
            (pRecvData->wLen == c_expectedGetVersionResponseLength))
        {
            PH_LOG_LIBNFC_INFO_STR("GET_VERSION response received. Card is Ultralight EV1 card");
            pDeviceInfo = (phNciNfc_DeviceInfo_t*)pCtx->pInfo;
            pRemoteDevInfo = pDeviceInfo->pRemDevList[0];
            pRemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.ULType = phNfc_eMifareULType_UltralightEV1;

            /* Get data area size from the GET_VERSION response */
            dataAreaSizeByte = pRecvData->pBuff[c_dataAreaSizeIndex];

            /* If the data area size byte equals the one for MF0UL11, then don't do the normal
               processing of the byte, since that will waste 16 bytes of the already-limited 48
               bytes of the card. Instead, in this case, just hardcode the data area size. */
            if (dataAreaSizeByte == c_mf0Ul11DataAreaSizeByte)
            {
                pRemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.DataAreaSize = c_mf0Ul11DataAreaSize;
            }
            else
            {
                pRemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.DataAreaSize = 1 << (dataAreaSizeByte >> 1);
            }

            /* Skip sending AUTHENTICATE command since we know the card is Ultralight EV1 */
            phLibNfc_SkipSequenceSeq(pCtx, gphLibNfc_DistinguishMifareUL, 3);
        }
        else
        {
            PH_LOG_LIBNFC_INFO_STR("GET_VERSION command failed. Try sending AUTHENTICATE command");
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULSendAuthenticateCmd(void *pContext, NFCSTATUS status, void *pInfo)
{
    static const uint8_t c_authenticateCmd[] = { 0x1A, 0x00 };
    static const uint16_t c_timeoutMillis = 50;

    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_TransceiveInfo_t transceiveInfo = { 0 };
    phNciNfc_DeviceInfo_t* pDeviceInfo = NULL;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pCtx)
    {
        phOsalNfc_MemCopy(pCtx->aSendBuff, c_authenticateCmd, sizeof(c_authenticateCmd));
        transceiveInfo.uCmd.T2TCmd = phNciNfc_eT2TRaw;
        transceiveInfo.tSendData.pBuff = pCtx->aSendBuff;
        transceiveInfo.tSendData.wLen = sizeof(c_authenticateCmd);
        transceiveInfo.tRecvData.pBuff = pCtx->aRecvBuff;
        transceiveInfo.tRecvData.wLen = sizeof(pCtx->aRecvBuff);
        transceiveInfo.wTimeout = c_timeoutMillis;

        pDeviceInfo = (phNciNfc_DeviceInfo_t*)pCtx->pInfo;
        wStatus = phNciNfc_Transceive(pCtx->sHwReference.pNciHandle,
                                      pDeviceInfo->pRemDevList[0],
                                      &transceiveInfo,
                                      &phLibNfc_MifareULDistinguishSequence,
                                      pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULProcessAuthenticateResp(void *pContext, NFCSTATUS status, void *pInfo)
{
    static const uint16_t c_expectedAuthenticateResponseLength = 9;

    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_Data_t* pRecvData = (phNciNfc_Data_t*)pInfo;
    phNciNfc_DeviceInfo_t* pDeviceInfo = NULL;
    phNciNfc_RemoteDevInformation_t* pRemoteDevInfo = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if ((NULL != pCtx) && (NFCSTATUS_SUCCESS == status))
    {
        pDeviceInfo = (phNciNfc_DeviceInfo_t*)pCtx->pInfo;
        pRemoteDevInfo = pDeviceInfo->pRemDevList[0];

        if ((NULL != pRecvData) &&
            (NULL != pRecvData->pBuff) &&
            (pRecvData->wLen == c_expectedAuthenticateResponseLength))
        {
            PH_LOG_LIBNFC_INFO_STR("AUTHENTICATE response received. Card is Ultralight C card");
            pRemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.ULType = phNfc_eMifareULType_UltralightC;
        }
        else
        {
            PH_LOG_LIBNFC_INFO_STR("AUTHENTICATE command failed. Card is regular Ultralight card");
            pRemoteDevInfo->tRemoteDevInfo.Iso14443A_Info.ULType = phNfc_eMifareULType_Ultralight;
        }
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULDeactivateCard(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pCtx)
    {
        wStatus = phNciNfc_Deactivate(pCtx->sHwReference.pNciHandle,
                                      phNciNfc_e_SleepMode,
                                      &phLibNfc_InternalSequence,
                                      pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULProcessDeactivateCard(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS wStatus = status;

    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    PH_LOG_LIBNFC_INFO_STR("Status = %!NFCSTATUS!", status);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULConnectCard(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t)pContext;
    phNciNfc_DeviceInfo_t* pDeviceInfo = NULL;

    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pCtx)
    {
        pDeviceInfo = (phNciNfc_DeviceInfo_t*)pCtx->pInfo;
        wStatus = phNciNfc_Connect(pCtx->sHwReference.pNciHandle,
                                   pDeviceInfo->pRemDevList[0],
                                   phNciNfc_e_RfInterfacesFrame_RF,
                                   &phLibNfc_InternalSequence,
                                   pCtx);
    }
    else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_MifareULProcessConnectCard(void *pContext, NFCSTATUS status, void *pInfo)
{
    NFCSTATUS wStatus = status;

    UNUSED(pContext);
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();

    PH_LOG_LIBNFC_INFO_STR("Status = %!NFCSTATUS!", status);

    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_FelicaReqResCmd(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = status;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t) pContext;
    phNciNfc_TransceiveInfo_t tFelicaInfo;
    phLibNfc_sRemoteDevInformation_t *pRemoteDevInfo = (phLibNfc_sRemoteDevInformation_t *)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pCtx) && (NULL != pInfo))
    {
        phOsalNfc_SetMemory(&tFelicaInfo, 0x00, sizeof(phNciNfc_TransceiveInfo_t));

        pCtx->pInfo = pInfo;

        pCtx->aSendBuff[0] = pRemoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength + 2;
        pCtx->aSendBuff[1] = 0x04; /* Request-Response command */
        
        phOsalNfc_MemCopy(&pCtx->aSendBuff[2],
                          pRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,
                          pRemoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength);
        
        tFelicaInfo.uCmd.T3TCmd = phNciNfc_eT3TRaw;
        tFelicaInfo.tSendData.pBuff = pCtx->aSendBuff;
        tFelicaInfo.tSendData.wLen = pRemoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength + 2;
        tFelicaInfo.tRecvData.pBuff = pCtx->aRecvBuff;
        tFelicaInfo.tRecvData.wLen = (uint16_t)sizeof(pCtx->aRecvBuff);
        tFelicaInfo.wTimeout = 300;

        wStatus = phNciNfc_Transceive((void *)pCtx->sHwReference.pNciHandle,
                                      (void *)(pCtx->Connected_handle),
                                      &tFelicaInfo,
                                      (pphNciNfc_TransreceiveCallback_t)&phLibNfc_InternalSeq,
                                      (void *)pContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_FelicaReqResResp(void *pContext,NFCSTATUS status,void *pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphLibNfc_LibContext_t pLibContext  = (pphLibNfc_LibContext_t)pContext;
    phLibNfc_sRemoteDevInformation_t *hRemoteDevInfo = NULL;
    pphNciNfc_Data_t pRecvData = (pphNciNfc_Data_t)pInfo;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        hRemoteDevInfo = pLibContext->pInfo;
        if((NFCSTATUS_SUCCESS == status) && (NULL != pInfo))
        {
            pRecvData = (pphNciNfc_Data_t)pInfo;
            if((NULL != hRemoteDevInfo) && (NULL != pRecvData->pBuff) &&\
                (PHLIBNFC_FELICA_REQRES_RESP_LEN == pRecvData->wLen) )
            {
                if(PHLIBNFC_FELICA_REQRES_RESP_LEN == pRecvData->pBuff[0])
                {
                    if(!phOsalNfc_MemCompare(&pRecvData->pBuff[2],
                                             hRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,
                                             hRemoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength))
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                    }
                }
            }else
            {
                PH_LOG_LIBNFC_CRIT_STR("Felica Invalid response buffer or response length");
            }
        }else
        {
            PH_LOG_LIBNFC_CRIT_STR("Felica Request Response no response received");
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_FelicaChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    if(NFCSTATUS_SUCCESS == status)
    {
        (void)phLibNfc_IsoDepFelicaPresChk_Cb(pContext,status,pInfo);
    }
    else
    {
        /* Launch T3t check presence sequence */
        PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_T3t_CheckPresSeq);
        status = phLibNfc_SeqHandler(pLibContext, NFCSTATUS_SUCCESS, pInfo);
        if(NFCSTATUS_PENDING != status)
        {
            status = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_SendT3tPollCmd(void *pContext,
                                  NFCSTATUS wStatus,
                                  void *pInfo
                                 )
{
    pphLibNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevNciHandle = NULL ;
    phNciNfc_SensFReqParams_t tSensFReq;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        pRemoteDevNciHandle = pCtx->Connected_handle;
        if(NULL != pRemoteDevNciHandle)
        {
            phOsalNfc_MemCopy(tSensFReq.bSysCode, pCtx->tADDconfig.FelicaPollCfg.SystemCode, sizeof(tSensFReq.bSysCode));
            tSensFReq.bReqCode = pCtx->tADDconfig.FelicaPollCfg.ReqCode;
            tSensFReq.bTimeSlotNum = pCtx->tADDconfig.FelicaPollCfg.TimeSlotNum;
            wStatus = phNciNfc_T3TPollReq(pCtx->sHwReference.pNciHandle,\
                                            &tSensFReq,\
                                            (pphNciNfc_IfNotificationCb_t) &phLibNfc_InternalSequence,
                                             pCtx);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_T3tCmdResp(void *pContext,
                                 NFCSTATUS wStatus,
                                 void *pInfo
                                 )
{
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pInfo;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
    UNUSED(pContext);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Command success");

        if(NULL != pNciRemoteDevHandle)
        {
            if(pLibContext->Connected_handle == pNciRemoteDevHandle)
            {
                PH_LOG_LIBNFC_INFO_STR("Valid remoteDev Handle!!");
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid remoteDev Handle!!");
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Command failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_T3tConnectComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_INFO_STR("Completing T3t Connect sequence");
    (void)phLibNfc_ConnectExtensionFelica_Cb(pContext,status,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static
NFCSTATUS phLibNfc_T3tChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_INFO_STR("Completing T3t Connect sequence");
    (void)phLibNfc_IsoDepFelicaPresChk_Cb(pContext,status,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_SendIsoDepPresChkCmd(void *pContext,
                                              NFCSTATUS wStatus,
                                              void *pInfo
                                              )
{
    pphLibNfc_Context_t pCtx = pContext;
    pphNciNfc_RemoteDevInformation_t pRemoteDevNciHandle = NULL;
    UNUSED(pInfo);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != pCtx)
    {
        pRemoteDevNciHandle = pCtx->Connected_handle;
        if(NULL != pRemoteDevNciHandle)
        {
            wStatus = phNciNfc_IsoDepPresenceChk(pCtx->sHwReference.pNciHandle,\
                                                 (pphNciNfc_IfNotificationCb_t) &phLibNfc_InternalSequence,
                                                 pCtx);
        }
        else
        {
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
    }else
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_IsoDepPresChkCmdResp(void *pContext,
                                               NFCSTATUS wStatus,
                                               void *pInfo
                                               )
{
    pphNciNfc_RemoteDevInformation_t pNciRemoteDevHandle = (pphNciNfc_RemoteDevInformation_t)pInfo;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();
    UNUSED(pContext);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Command success");

        if(NULL != pNciRemoteDevHandle)
        {
            if(pLibContext->Connected_handle == pNciRemoteDevHandle)
            {
                PH_LOG_LIBNFC_INFO_STR("Valid remoteDev Handle!!");
            }
            else
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid remoteDev Handle!!");
            }
        }
    }
    else
    {
        PH_LOG_LIBNFC_CRIT_STR("Command failed!");
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phLibNfc_IsoDepConnectComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_INFO_STR("Completing IsoDep Connect sequence");
    (void)phLibNfc_ConnectExtensionFelica_Cb(pContext,status,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static NFCSTATUS phLibNfc_IsoDepChkPresComplete(void *pContext,NFCSTATUS status,void *pInfo)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_INFO_STR("Completing IsoDep Connect sequence");
    (void)phLibNfc_IsoDepFelicaPresChk_Cb(pContext,status,pInfo);
    PH_LOG_LIBNFC_FUNC_EXIT();
    return status;
}

static void phLibNfc_CalSectorAddress(uint8_t *Sector_Address)
{
    uint8_t BlockNumber = 0x00;

    if(NULL != Sector_Address)
    {
        BlockNumber = *Sector_Address;
        if(BlockNumber >= PHLIBNFC_MIFARESTD4K_BLK128)
        {
            *Sector_Address = (uint8_t)(PHLIBNFC_MIFARESTD_SECTOR_NO32 +
                              ((BlockNumber - PHLIBNFC_MIFARESTD4K_BLK128)/
                               PHLIBNFC_MIFARESTD_BLOCK_BYTES));
        }
        else
        {
            *Sector_Address = BlockNumber/MIFARE_BLKPERSECTOR;
        }
    }
}

static
NFCSTATUS phLibNfc_ChkAuthCmdMFC(void *pContext,
                                 phLibNfc_sTransceiveInfo_t* pTransceiveInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext = (pphLibNfc_LibContext_t )pContext;
    phNciNfc_RemoteDevInformation_t *phNciRemoteDev;
    uint8_t bUID[PH_NCINFCTYPES_MAX_UID_LENGTH];
    int32_t wStat;
    uint8_t bUidIndex = 0;
    uint8_t bUidLength = 0;

    if(NULL != pTransceiveInfo &&
       NULL != pTransceiveInfo->sSendData.buffer &&
       0 != pTransceiveInfo->sSendData.length &&
       NULL != pLibContext &&
       NULL != pLibContext->Connected_handle)
    {
        if((pTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentA ||
           pTransceiveInfo->cmd.MfCmd == phNfc_eMifareAuthentB ) &&
           pTransceiveInfo->sSendData.length == (PHLIBNFC_MFCUIDLEN_INAUTHCMD + PHLIBNFC_MFC_AUTHKEYLEN ))
        {
            phNciRemoteDev = (phNciNfc_RemoteDevInformation_t *)pLibContext->Connected_handle;
            bUidLength = phNciRemoteDev->tRemoteDevInfo.Iso14443A_Info.UidLength;

            if(PHLIBNFC_MFCWITH_7BYTEUID == bUidLength)
            {
                bUidIndex = PHLIBNFC_MFCUIDINDEX_7BYTEUID;
            }

            _Analysis_assume_(bUidIndex < bUidLength);

            phOsalNfc_MemCopy(bUID,&phNciRemoteDev->tRemoteDevInfo.Iso14443A_Info.Uid[bUidIndex],(bUidLength - bUidIndex));
            wStat = phOsalNfc_MemCompare(bUID,pTransceiveInfo->sSendData.buffer,PHLIBNFC_MFCUIDLEN_INAUTHCMD);

            if(wStat != 0)
            {
                PH_LOG_LIBNFC_CRIT_STR("Invalid send payload buffer");
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
    return wStatus;
}

void phLibNfc_ClearLibContext(pphLibNfc_LibContext_t pLibContext)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext &&
        pLibContext == phLibNfc_GetContext() )
    {
        pLibContext->bPcdConnected = FALSE;

        (void)phOsalNfc_Timer_Delete(pLibContext->WdTimerId);
        pLibContext->WdTimerId = 0;

        (void)phOsalNfc_Timer_Delete(pLibContext->dwHciTimerId);
        pLibContext->dwHciTimerId = 0;

        phLibNfc_Invoke_Pending_Cb(pLibContext, NFCSTATUS_SHUTDOWN);
        (void)phLibNfc_Ndef_DeInit();

        phLibNfc_RemoteDev_ClearInfo();

        pLibContext->sHwReference.pNciHandle = NULL;

        if(NULL != pLibContext->psBufferedAuth)
        {
            if(NULL != pLibContext->psBufferedAuth->sRecvData.buffer)
            {
                phOsalNfc_FreeMemory(
                    pLibContext->psBufferedAuth->sRecvData.buffer);
                pLibContext->psBufferedAuth->sRecvData.buffer = NULL;
            }
            if(NULL != pLibContext->psBufferedAuth->sSendData.buffer)
            {
                phOsalNfc_FreeMemory(
                    pLibContext->psBufferedAuth->sSendData.buffer);
                pLibContext->psBufferedAuth->sSendData.buffer = NULL;
            }
            phOsalNfc_FreeMemory(pLibContext->psBufferedAuth);
            pLibContext->psBufferedAuth = NULL;
        }
        if(NULL != pLibContext->ndef_cntx.psNdefMap)
        {
            if(NULL != pLibContext->ndef_cntx.psNdefMap->SendRecvBuf)
            {
                phOsalNfc_FreeMemory(pLibContext->ndef_cntx.psNdefMap->SendRecvBuf);
                pLibContext->ndef_cntx.psNdefMap->SendRecvBuf = NULL;
            }
            phOsalNfc_FreeMemory(pLibContext->ndef_cntx.psNdefMap);
            pLibContext->ndef_cntx.psNdefMap = NULL;
        }
        if(NULL != pLibContext->psOverHalCtxt)
        {
            phOsalNfc_FreeMemory(pLibContext->psOverHalCtxt);
            pLibContext->psOverHalCtxt = NULL;
        }
        if(NULL != pLibContext->psDevInputParam)
        {
            phOsalNfc_FreeMemory(pLibContext->psDevInputParam);
            pLibContext->psDevInputParam = NULL;
        }
        if(NULL != pLibContext->ndef_cntx.ndef_fmt)
        {
            phOsalNfc_FreeMemory(pLibContext->ndef_cntx.ndef_fmt);
            pLibContext->ndef_cntx.ndef_fmt = NULL;
        }
        if(NULL != pLibContext->phLib_NdefRecCntx.NdefCb)
        {
            phOsalNfc_FreeMemory(pLibContext->phLib_NdefRecCntx.NdefCb);
            pLibContext->phLib_NdefRecCntx.NdefCb = NULL;
        }
        if(NULL != pLibContext->phLib_NdefRecCntx.ndef_message.buffer)
        {
            phOsalNfc_FreeMemory(pLibContext->phLib_NdefRecCntx.ndef_message.buffer);
            pLibContext->phLib_NdefRecCntx.ndef_message.buffer = NULL;
        }

        phLibNfc_HciDeInit();

        phOsalNfc_FreeMemory(pLibContext);
        phLibNfc_SetContext(NULL);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();
    return ;
}

NFCSTATUS phLibNfc_GetNciHandle (phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle,
                                 pphNciNfc_RemoteDevInformation_t *ppNciRemoteDevHandle,
                                 pphLibNfc_Context_t pLibContext)
{
    NFCSTATUS wRetVal = NFCSTATUS_FAILED;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pLibContext) &&
       (pLibContext == phLibNfc_GetContext()) &&
       (NULL != pLibRemoteDevHandle))
    {
        wRetVal = phLibNfc_MapRemoteDevHandle(&pLibRemoteDevHandle,ppNciRemoteDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if(NFCSTATUS_SUCCESS != wRetVal)
        {
            PH_LOG_LIBNFC_CRIT_STR("Mapping of LibNfc RemoteDev Handle to NCI RemoteDev Handle Failed");
            wRetVal = NFCSTATUS_INVALID_HANDLE;
        }
    }
    else
    {
        wRetVal= NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_ENTRY();

    return wRetVal;
}

NFCSTATUS phLibNfc_VerifyRemDevHandle(phLibNfc_sRemoteDevInformation_t *pLibRemoteDevHandle,
                                      pphLibNfc_Context_t pLibContext)
{
    uint8_t bIndex;
    NFCSTATUS wRetVal = NFCSTATUS_FAILED;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pLibContext) &&
       (pLibContext == phLibNfc_GetContext()) &&
       (NULL != pLibRemoteDevHandle) &&
       (pLibContext->dev_cnt > 0))
    {
        for(bIndex = 0 ; bIndex < pLibContext->dev_cnt ; bIndex++)
        {
            if((NULL != pLibContext->Map_Handle[bIndex].pLibNfc_RemoteDev_List ) &&
               (pLibContext->Map_Handle[bIndex].pLibNfc_RemoteDev_List == pLibRemoteDevHandle))
            {
                wRetVal = NFCSTATUS_SUCCESS;
                break;
            }
        }
    }
    else
    {
        wRetVal= NFCSTATUS_INVALID_PARAMETER;
    }

    PH_LOG_LIBNFC_FUNC_ENTRY();

    return wRetVal;
}

static bool_t phLibNfc_CallBacksPending(pphLibNfc_LibContext_t pLibContext)
{
    bool_t bStatus = FALSE;

    /* Check validity of all available call back functions */
    if(NULL != pLibContext->CBInfo.pClientInitCb ||
       NULL != pLibContext->CBInfo.pClientShutdownCb ||
       NULL != pLibContext->CBInfo.pClientConnectCb ||
       NULL != pLibContext->CBInfo.pClientTranscvCb ||
       NULL != pLibContext->CBInfo.pClientDisConnectCb ||
       NULL != pLibContext->CBInfo.pClientDisConfigCb ||
       NULL != pLibContext->CBInfo.pClientCkNdefCb ||
       NULL != pLibContext->CBInfo.pClientTransceiveCb ||
       NULL != pLibContext->CBInfo.pClientRdNdefCb ||
       NULL != pLibContext->CBInfo.pClientWrNdefCb ||
       NULL != pLibContext->CBInfo.pClientNdefNtfRespCb ||
       NULL != pLibContext->CBInfo.pClientPresChkCb ||
       NULL != pLibContext->CBInfo.pClientRoutingCfgCb ||
       NULL != pLibContext->CBInfo.pClientLlcpCheckRespCb ||
       NULL != pLibContext->CBInfo.pClientLlcpLinkCb ||
       NULL != pLibContext->CBInfo.pClientLlcpDiscoveryCb ||
       NULL != pLibContext->CBInfo.pClientNfcIpCfgCb ||
       NULL != pLibContext->CBInfo.pClientNfcIpTxCb ||
       NULL != pLibContext->CBInfo.pClientNfcIpRxCb ||
       NULL != pLibContext->CBInfo.pSeSetModeCb ||
       NULL != pLibContext->CBInfo.pNFCEEDiscoveryCb)
    {
        bStatus = TRUE;
    }
    return bStatus;
}

static void phLibNfc_PrintRemoteDevInfo(phLibNfc_RemoteDevList_t *psRemoteDevList , uint32_t Num_Dev)
{
    uint8_t bIndex = 0x00;

    for(bIndex = 0; bIndex < Num_Dev; bIndex ++)
    {
        if((NULL != psRemoteDevList) &&
           (NULL != psRemoteDevList[bIndex].hTargetDev )&&
           (NULL != psRemoteDevList[bIndex].psRemoteDevInfo ) &&
           (Num_Dev > 0))
        {
            PH_LOG_LIBNFC_INFO_STR("========= Remote Device Info =========");
            PH_LOG_LIBNFC_INFO_STR("Remote Device Type: %!phNfc_eRFDevType_t!", psRemoteDevList[bIndex].psRemoteDevInfo->RemDevType);

            switch(psRemoteDevList[bIndex].psRemoteDevInfo->RemDevType)
            {
                case phNfc_eISO14443_3A_PICC:
                case phNfc_eMifare_PICC:
                case phNfc_eISO14443_4A_PICC:
                case phNfc_eISO14443_A_PICC:
                    phLibNfc_PrintRemDevInfoNFCA(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info);
                    break;
                case phNfc_eISO14443_B_PICC:
                case phNfc_eISO14443_4B_PICC:
                    phLibNfc_PrintRemDevInfoNFCB(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info);
                    break;
                case phNfc_eFelica_PICC:
                    phLibNfc_PrintRemDevInfoNFCF(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Felica_Info);
                    break;
                case phNfc_eJewel_PICC:
                    phLibNfc_PrintRemDevInfoJewel(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Jewel_Info);
                    break;
                case phNfc_eISO15693_PICC:
                    phLibNfc_PrintRemDevInfoISO15693(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
                    break;
                case phNfc_eEpcGen2_PICC:
                    phLibNfc_PrintRemDevInfoEPCGEN2(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.EpcGen_Info);
                    break;
                case phNfc_eNfcIP1_Target:
                case phNfc_eNfcIP1_Initiator:
                    phLibNfc_PrintRemDevInfoNFCIP1(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.NfcIP_Info);
                    break;
                case phNfc_eKovio_PICC:
                    phLibNfc_PrintRemDevInfoKovio(&psRemoteDevList[bIndex].psRemoteDevInfo->RemoteDevInfo.Kovio_Info);
                    break;
                default:
                    break;
            }
        }
    }
}

static void phLibNfc_PrintRemDevInfoNFCA(phNfc_sIso14443AInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
       NULL != phLibNfc_RemoteDevInfo->AppData &&
       NULL != phLibNfc_RemoteDevInfo->Uid &&
       NULL != phLibNfc_RemoteDevInfo->AtqA)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("UID LENGTH :",phLibNfc_RemoteDevInfo->UidLength);
        if(phLibNfc_RemoteDevInfo->UidLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("UID :",phLibNfc_RemoteDevInfo->Uid,phLibNfc_RemoteDevInfo->UidLength);
        }

        PH_LOG_LIBNFC_INFO_X32MSG("APP DATA LENGTH :",phLibNfc_RemoteDevInfo->AppDataLength);
        if(phLibNfc_RemoteDevInfo->AppDataLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("APP DATA :",phLibNfc_RemoteDevInfo->AppData,phLibNfc_RemoteDevInfo->AppDataLength);
        }

        PH_LOG_LIBNFC_INFO_X32MSG("SAK :",phLibNfc_RemoteDevInfo->Sak);
        PH_LOG_LIBNFC_INFO_HEXDATA("AtqA :",phLibNfc_RemoteDevInfo->AtqA,PHNFC_ATQA_LENGTH);
        PH_LOG_LIBNFC_INFO_X32MSG("MaxDataRate :",phLibNfc_RemoteDevInfo->MaxDataRate);
        PH_LOG_LIBNFC_INFO_X32MSG("FWI_SFGT :",phLibNfc_RemoteDevInfo->Fwi_Sfgt);
    }
}

static void phLibNfc_PrintRemDevInfoJewel(phNfc_sJewelInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
       NULL != phLibNfc_RemoteDevInfo->Uid)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("HeaderRom0:",phLibNfc_RemoteDevInfo->HeaderRom0);
        PH_LOG_LIBNFC_INFO_X32MSG("HeaderRom1:",phLibNfc_RemoteDevInfo->HeaderRom1);
        PH_LOG_LIBNFC_INFO_X32MSG("Uid length :",phLibNfc_RemoteDevInfo->UidLength);

        if(phLibNfc_RemoteDevInfo->UidLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("Uid:",phLibNfc_RemoteDevInfo->Uid,phLibNfc_RemoteDevInfo->UidLength);
        }
    }
}

static void phLibNfc_PrintRemDevInfoNFCB(phNfc_sIso14443BInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
       NULL != phLibNfc_RemoteDevInfo->AtqB.AtqRes &&
       NULL != phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.Pupi &&
       NULL != phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.ProtInfo &&
       NULL != phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.AppData &&
       NULL != phLibNfc_RemoteDevInfo->HiLayerResp)
    {
        PH_LOG_LIBNFC_INFO_HEXDATA("PUPI:",phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.Pupi,PHNFC_PUPI_LENGTH);
        PH_LOG_LIBNFC_INFO_HEXDATA("AppData:",phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.AppData,PHNFC_APP_DATA_B_LENGTH);
        PH_LOG_LIBNFC_INFO_HEXDATA("ProtInfo:",phLibNfc_RemoteDevInfo->AtqB.AtqResInfo.ProtInfo,PHNFC_PROT_INFO_B_LENGTH);
        PH_LOG_LIBNFC_INFO_X32MSG("HiLayerRespLength:",phLibNfc_RemoteDevInfo->HiLayerRespLength);

        if(phLibNfc_RemoteDevInfo->HiLayerRespLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("HiLayerResp :",phLibNfc_RemoteDevInfo->HiLayerResp,phLibNfc_RemoteDevInfo->HiLayerRespLength);
        }
        PH_LOG_LIBNFC_INFO_X32MSG("Afi :",phLibNfc_RemoteDevInfo->Afi);
        PH_LOG_LIBNFC_INFO_X32MSG("MaxDataRate :",phLibNfc_RemoteDevInfo->MaxDataRate);
    }
}

static void phLibNfc_PrintRemDevInfoNFCF(phNfc_sFelicaInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
        NULL != phLibNfc_RemoteDevInfo->IDm &&
        NULL != phLibNfc_RemoteDevInfo->PMm &&
        NULL != phLibNfc_RemoteDevInfo->SystemCode)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("IDmLength:",phLibNfc_RemoteDevInfo->IDmLength);
        if(phLibNfc_RemoteDevInfo->IDmLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("ID:",phLibNfc_RemoteDevInfo->IDm,phLibNfc_RemoteDevInfo->IDmLength);
        }
        PH_LOG_LIBNFC_INFO_HEXDATA("PMm:",phLibNfc_RemoteDevInfo->PMm,PHNFC_FEL_PM_LEN);
        PH_LOG_LIBNFC_INFO_HEXDATA("SystemCode:",phLibNfc_RemoteDevInfo->SystemCode,PHNFC_FEL_SYS_CODE_LEN);
    }
}

static void phLibNfc_PrintRemDevInfoNFCIP1(phNfc_sNfcIPInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
        NULL != phLibNfc_RemoteDevInfo->ATRInfo &&
        NULL != phLibNfc_RemoteDevInfo->NFCID &&
        NULL != phLibNfc_RemoteDevInfo->SenseRes)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("NFCID Length:",phLibNfc_RemoteDevInfo->NFCID_Length);
        if(phLibNfc_RemoteDevInfo->NFCID_Length > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("NFCID:",phLibNfc_RemoteDevInfo->NFCID,phLibNfc_RemoteDevInfo->NFCID_Length);
        }

        PH_LOG_LIBNFC_INFO_X32MSG("ATRInfo Length:",phLibNfc_RemoteDevInfo->ATRInfo_Length);
        if(phLibNfc_RemoteDevInfo->ATRInfo_Length > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("ATRInfo:",phLibNfc_RemoteDevInfo->ATRInfo,phLibNfc_RemoteDevInfo->ATRInfo_Length);
        }

        PH_LOG_LIBNFC_INFO_X32MSG("SelRes:",phLibNfc_RemoteDevInfo->SelRes);
        PH_LOG_LIBNFC_INFO_HEXDATA("SenseRes:",phLibNfc_RemoteDevInfo->SenseRes,PHNFC_ATQA_LENGTH);
        PH_LOG_LIBNFC_INFO_X32MSG("Nfcip_Active:",phLibNfc_RemoteDevInfo->Nfcip_Active);
        PH_LOG_LIBNFC_INFO_X32MSG("MaxFrameLength:",phLibNfc_RemoteDevInfo->MaxFrameLength);
        PH_LOG_LIBNFC_INFO_X32MSG("Nfcip_Datarate:",phLibNfc_RemoteDevInfo->Nfcip_Datarate);
    }
}

static void phLibNfc_PrintRemDevInfoISO15693(phNfc_sIso15693Info_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo &&
       NULL != phLibNfc_RemoteDevInfo->Uid)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("UidLength :",phLibNfc_RemoteDevInfo->UidLength);
        if(phLibNfc_RemoteDevInfo->UidLength > 0)
        {
            PH_LOG_LIBNFC_INFO_HEXDATA("Uid:",phLibNfc_RemoteDevInfo->Uid,phLibNfc_RemoteDevInfo->UidLength);
        }
        PH_LOG_LIBNFC_INFO_X32MSG("Afi:",phLibNfc_RemoteDevInfo->Afi);
        PH_LOG_LIBNFC_INFO_X32MSG("Dsfid:",phLibNfc_RemoteDevInfo->Dsfid);
        PH_LOG_LIBNFC_INFO_X32MSG("Flags:",phLibNfc_RemoteDevInfo->Flags);
    }
}

static void phLibNfc_PrintRemDevInfoEPCGEN2(phNfc_sEpcGenInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("hAccessHandle:",phLibNfc_RemoteDevInfo->hAccessHandle);
        PH_LOG_LIBNFC_INFO_X32MSG("bEpcLength:",phLibNfc_RemoteDevInfo->ProtocolControl.bEpcLength);
        PH_LOG_LIBNFC_INFO_X32MSG("bRecomissionStatus:",phLibNfc_RemoteDevInfo->ProtocolControl.bRecomissionStatus);
        PH_LOG_LIBNFC_INFO_X32MSG("bUmi:",phLibNfc_RemoteDevInfo->ProtocolControl.bUmi);
        PH_LOG_LIBNFC_INFO_X32MSG("wNsi:",phLibNfc_RemoteDevInfo->ProtocolControl.wNsi);
        PH_LOG_LIBNFC_INFO_X32MSG("wStoredCrc:",phLibNfc_RemoteDevInfo->wStoredCrc);
        PH_LOG_LIBNFC_INFO_X32MSG("wXpc_W1:",phLibNfc_RemoteDevInfo->wXpc_W1);
    }
}

static void phLibNfc_PrintRemDevInfoKovio(phNfc_sKovioInfo_t *phLibNfc_RemoteDevInfo)
{
    if(NULL != phLibNfc_RemoteDevInfo)
    {
        PH_LOG_LIBNFC_INFO_X32MSG("TAG LENGTH :", phLibNfc_RemoteDevInfo->TagIdLength);
        PH_LOG_LIBNFC_INFO_HEXDATA("APP DATA :", phLibNfc_RemoteDevInfo->TagId, phLibNfc_RemoteDevInfo->TagIdLength);
    }
}
