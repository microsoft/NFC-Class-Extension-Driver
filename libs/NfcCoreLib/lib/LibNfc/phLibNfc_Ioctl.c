/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_Ioctl.tmh"

static phLibNfc_Ioctl_Cntx_t gphLibNfc_IoctlCtx;

static void phLibNfc_Ioctl_Mgmt_CB(void *context, phNfc_sData_t *pOutData, NFCSTATUS status);

static NFCSTATUS phLibNfc_IoctlSetRfCfgCmd(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_IoctlSetRfConfig(uint8_t *pInBuffer, uint32_t Size);
static NFCSTATUS phLibNfc_IoctlSetRfCfgProc(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_IoctlSetRfCfgComplete(void* pContext,NFCSTATUS status,void* pInfo);
static NFCSTATUS phLibNfc_DtaEnableSetConfig(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_DtaDisableSetConfig(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_DtaSetConfigCb(void* pContext, NFCSTATUS status, void* pInfo);
static NFCSTATUS phLibNfc_DtaSetConfigComplete(void* pContext,NFCSTATUS status,void* pInfo);

static phLibNfc_Sequence_t gphLibNfc_IoctlSetRfConfig[] = {
    {&phLibNfc_IoctlSetRfCfgCmd, &phLibNfc_IoctlSetRfCfgProc},
    {NULL, &phLibNfc_IoctlSetRfCfgComplete}
};

static phLibNfc_Sequence_t gphLibNfc_DtaEnableSequence[] = {
    {&phLibNfc_DtaEnableSetConfig, &phLibNfc_DtaSetConfigCb},
    {NULL, &phLibNfc_DtaSetConfigComplete}
};

static phLibNfc_Sequence_t gphLibNfc_DtaDisableSequence[] = {
    {&phLibNfc_DtaDisableSetConfig, &phLibNfc_DtaSetConfigCb},
    {NULL, &phLibNfc_DtaSetConfigComplete}
};

NFCSTATUS phLibNfc_Mgt_IoCtl(void*                      pDriverHandle,
                             uint16_t                   IoctlCode,
                             phNfc_sData_t*             pInParam,
                             phNfc_sData_t*             pOutParam,
                             pphLibNfc_IoctlCallback_t  pIoCtl_Rsp_cb,
                             void*                      pContext
                            )
{
    NFCSTATUS      wStatus = NFCSTATUS_SUCCESS;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    UNUSED(pInParam);

    if((0 == IoctlCode) || (NULL == pIoCtl_Rsp_cb) ||
      (NULL == pContext) || (NULL == pDriverHandle))
    {
        wStatus = NFCSTATUS_INVALID_PARAMETER;
        goto Done;
    }

    gphLibNfc_IoctlCtx.CliRspCb = pIoCtl_Rsp_cb;
    gphLibNfc_IoctlCtx.pCliCntx = pContext;
    gphLibNfc_IoctlCtx.pOutParam = pOutParam;
    gphLibNfc_IoctlCtx.IoctlCode = IoctlCode;

    switch(IoctlCode)
    {
        case PHLIBNFC_ENABLE_DTA_MODE:
        {
            if(NULL != pLibContext)
            {
                pLibContext->bDtaFlag = 1;
                pLibContext->ndef_cntx.psNdefMap->bDtaFlag = 1;
                pLibContext->llcp_cntx.sLlcpContext.bDtaFlag = 1;

                PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_DtaEnableSequence);
                wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
            }
        }
        break;
        case PHLIBNFC_DISABLE_DTA_MODE:
        {
            if(NULL != pLibContext)
            {
                pLibContext->bDtaFlag = 0;
                pLibContext->ndef_cntx.psNdefMap->bDtaFlag = 0;
                pLibContext->llcp_cntx.sLlcpContext.bDtaFlag = 0;

                PHLIBNFC_INIT_SEQUENCE(pLibContext, gphLibNfc_DtaDisableSequence);
                wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
            }
        }
        break;
        case PHLIBNFC_SET_RAW_CONFIG:
        {
            if((NULL == pInParam) || (NULL == pInParam->buffer) ||
               (pInParam->length < PHLIBNFC_INPUT_BUFF_MIN_LEN))
            {
                /* Invalid input parameters, can not perform Rf configuration */
                wStatus = NFCSTATUS_INVALID_PARAMETER;
            }
            else
            {
                wStatus = phLibNfc_IoctlSetRfConfig(pInParam->buffer,pInParam->length);
            }
        }
        break;
        default:
        {
            /* Do nothing */
            wStatus = NFCSTATUS_INVALID_PARAMETER;
        }
        break;
    }

Done:
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static void phLibNfc_Ioctl_Mgmt_CB(void          *context,
                              phNfc_sData_t *pOutData,
                              NFCSTATUS      status)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phLibNfc_Ioctl_Cntx_t *pIoctlCtx = (phLibNfc_Ioctl_Cntx_t *)context;

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pIoctlCtx) && (pIoctlCtx == &gphLibNfc_IoctlCtx))
    {
        switch(pIoctlCtx->IoctlCode)
        {
            case PHLIBNFC_ENABLE_DTA_MODE:
            {
                wStatus = status;
                PH_LOG_LIBNFC_INFO_STR("Ioctl DTA mode Complete");
            }
            break;
            case PHLIBNFC_DISABLE_DTA_MODE:
            {
                wStatus = status;
                PH_LOG_LIBNFC_INFO_STR("Ioctl DTA mode Complete");
            }
            break;
            case PHLIBNFC_SET_RAW_CONFIG:
            {
                wStatus = status;
                PH_LOG_LIBNFC_INFO_STR("Ioctl set raw config complete");
            }
            break;
            default:
            {
            }
            break;
        }
        if(NULL != pIoctlCtx->CliRspCb)
        {
            pIoctlCtx->CliRspCb(pIoctlCtx->pCliCntx,pOutData,wStatus);
            pIoctlCtx->CliRspCb = NULL;
            pIoctlCtx->pCliCntx = NULL;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();

    return ;
}

NFCSTATUS
phLibNfc_IoctlSetRfCfgCmd(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    UNUSED(pInfo);
    UNUSED(status);
    UNUSED(pContext);

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if(NULL != pLibContext)
    {
        wStatus = phNciNfc_SetConfigRaw(pLibContext->sHwReference.pNciHandle,
                                        pLibContext->tRfRawConfig.buffer,
                                        (uint16_t) pLibContext->tRfRawConfig.length,
                                        (pphNciNfc_IfNotificationCb_t)&phLibNfc_InternalSequence,
                                        pLibContext);
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phLibNfc_IoctlSetRfCfgProc(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    UNUSED(pContext);
    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS
phLibNfc_IoctlSetRfCfgComplete(void* pContext,NFCSTATUS status,void* pInfo)
{
    UNUSED(pInfo);
    UNUSED(pContext);
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if (NULL != pLibContext)
    {
        /* Invoke upper layer with proper status */
        phLibNfc_Ioctl_Mgmt_CB(&gphLibNfc_IoctlCtx,NULL,status);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();

    return status;
}

static
NFCSTATUS phLibNfc_IoctlSetRfConfig(uint8_t *pInBuffer, uint32_t Size)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    PH_LOG_LIBNFC_FUNC_ENTRY();

    if((NULL != pInBuffer) && (0 != Size) && (NULL != pLibContext))
    {
        pLibContext->tRfRawConfig.length = (uint16_t) Size;
        pLibContext->tRfRawConfig.buffer = pInBuffer;

        /* Initialize the sequence and start */
        PHLIBNFC_INIT_SEQUENCE(pLibContext,gphLibNfc_IoctlSetRfConfig);

        wStatus = phLibNfc_SeqHandler(pLibContext,NFCSTATUS_SUCCESS,NULL);
    }

    PH_LOG_LIBNFC_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS phLibNfc_DtaEnableSetConfig(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;
    
    UNUSED(pCtx);
    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS phLibNfc_DtaDisableSetConfig(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_Context_t pCtx = (pphLibNfc_Context_t ) pContext;

    UNUSED(pCtx);
    UNUSED(status);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phLibNfc_DtaSetConfigCb(void* pContext, NFCSTATUS status, void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    UNUSED(pContext);
    UNUSED(pInfo);

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wStatus = status;
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        PH_LOG_LIBNFC_INFO_STR("Request Successful");
    }
    else
    {
        wStatus = NFCSTATUS_FAILED;
        PH_LOG_LIBNFC_CRIT_STR("Request Failed!!");
    }

    PH_LOG_LIBNFC_FUNC_EXIT();

    return wStatus;
}

static
NFCSTATUS
phLibNfc_DtaSetConfigComplete(void* pContext,NFCSTATUS status,void* pInfo)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    phLibNfc_LibContext_t* pLibContext = phLibNfc_GetContext();

    UNUSED(pInfo);
    UNUSED(pContext);

    if(NULL != pLibContext)
    {
        if(NFCSTATUS_SUCCESS == status)
        {
            wStatus = NFCSTATUS_SUCCESS;
        }

        phLibNfc_Ioctl_Mgmt_CB(&gphLibNfc_IoctlCtx,NULL,wStatus);
    }
    return wStatus;
}
