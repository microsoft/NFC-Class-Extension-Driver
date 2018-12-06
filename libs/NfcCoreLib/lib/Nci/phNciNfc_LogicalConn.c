/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_Pch.h"

#include "phNciNfc_LogicalConn.tmh"

#define PH_NCINFC_MIN_CREATE_CMD_LEN        (0x02U)

static NFCSTATUS phNciNfc_SendConnOpenCmd (void *psContext);
static NFCSTATUS phNciNfc_LogConnResp(void *psContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_SendConnCloseCmd(void *psContext);
static NFCSTATUS phNciNfc_LogConnCloseResp(void *psContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_CompleteConnSequence(void *pContext, NFCSTATUS wStatus);

static NFCSTATUS phNciNfc_ValidateLogConnInfo(void *psContext, phNciNfc_Dest_Info_t *pDestination);

/*Global variables for Logical Conn Request,Resp & Release Handler */
phNciNfc_SequenceP_t gphNciNfc_LogConnOpenSequence[] = {
    {&phNciNfc_SendConnOpenCmd, &phNciNfc_LogConnResp},
    {NULL, &phNciNfc_CompleteConnSequence}
};

/*Global variables for Logical Conn Request,Resp & Release Handler */
phNciNfc_SequenceP_t gphNciNfc_LogConnCloseSequence[] = {
    {&phNciNfc_SendConnCloseCmd, &phNciNfc_LogConnCloseResp},
    {NULL, &phNciNfc_CompleteConnSequence}
};

NFCSTATUS
phNciNfc_LogConnInit(
                     phNciNfc_Context_t     *psNciContext
                     )
{
    NFCSTATUS    status = NFCSTATUS_SUCCESS;
    phNciNfc_sCoreHeaderInfo_t tNtfInfo;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext)
        )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        PH_LOG_NCI_INFO_STR("Registering for Conn Credits Notification");
        tNtfInfo.bEnabled = PHNCINFC_DISABLE_AUTO_DEREG;
        tNtfInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlNtf;
        tNtfInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        tNtfInfo.Opcode_ID.Val = phNciNfc_e_NciCoreConnCreditNtfOid;

        status = phNciNfc_CoreIfRegRspNtf((void *)&psNciContext->NciCoreContext,
                                 &tNtfInfo,
                                 &phNciNfc_ProcessConnCreditNtf,
                                 (void *)&psNciContext->NciCoreContext);
        if(NFCSTATUS_SUCCESS != status)
        {
            PH_LOG_NCI_INFO_STR("Conn Credits Ntf Registration failed");
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Conn Credits Ntf successfully Registered");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS phNciNfc_ValidateLogConnInfo(
                                       void                    *psContext,
                                       phNciNfc_Dest_Info_t    *pDestination
                                       )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_Context_t psNciCtxt = psContext;
    uint8_t bOpenConns;
    
    /* Validate the destination type,length and parameter type passed
       Number of Destination parameters supported is 1 for RemoteNFC endpoint
       or NFCEE, But it shall be 0 for Loop back mode
       Type of Destination Parameter shall be Either RF discover ID or Nfcee ID
       Length of Destination parameter shall be 0x02 */

    if( (pDestination->tDest <= phNciNfc_e_NFCEE) &&\
        ((0x01 == pDestination->bNumDestParams) ||\
          (0x00 == pDestination->bNumDestParams) && (pDestination->tDest == phNciNfc_e_NFCC_LOOPBACK)) &&\
        ( ((pDestination->tDestParams.bDestParamType <= 0x01)&&\
          (0x02 == pDestination->tDestParams.bDestParamLen)) || (pDestination->tDest == phNciNfc_e_NFCC_LOOPBACK)) )
    {
        bOpenConns = phNciNfc_GetConnCount();
        PH_LOG_NCI_INFO_X32MSG(" Currently number of Open Connections are..",bOpenConns);

        if(bOpenConns <= psNciCtxt->InitRspParams.MaxLogicalCon)
        {
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    return wStatus;
}

NFCSTATUS
phNciNfc_LogConnCreate(
                        void                    *psContext,
                        phNciNfc_Dest_Info_t    *pDestination,
                        pphNciNfc_IfNotificationCb_t    pNotify,
                        void                    *pContext
              )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    uint8_t *pPayload = NULL;
    uint16_t wPayloadLen = 0;
    uint8_t bIndex = 0;
    pphNciNfc_Context_t psNciCtxt = psContext;
    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciCtxt)||\
        (NULL == pDestination) || (NULL == pNotify) )
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        wStatus = phNciNfc_ValidateLogConnInfo(psNciCtxt,pDestination);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            wPayloadLen = PH_NCINFC_MIN_CREATE_CMD_LEN;
            if(phNciNfc_e_NFCC_LOOPBACK != pDestination->tDest)
            {
                wPayloadLen += sizeof(pDestination->tDest);
            }

            pPayload = (uint8_t *) phOsalNfc_GetMemory((uint32_t)wPayloadLen);
            if(NULL != pPayload)
            {
                psNciCtxt->tLogConnCtxt.tDestType = pDestination->tDest;
                pPayload[bIndex++] = pDestination->tDest;
                if(phNciNfc_e_NFCC_LOOPBACK == pDestination->tDest)
                {
                    pPayload[bIndex++] = 0x00;
                    psNciCtxt->tLogConnCtxt.bDestId = PH_NCINFC_LOOPBACK_MODE_DESTID;
                }
                else
                {
                    pPayload[bIndex++] = pDestination->bNumDestParams;
                    pPayload[bIndex++] = pDestination->tDestParams.bDestParamType;
                    pPayload[bIndex++] = pDestination->tDestParams.bDestParamLen;
                    pPayload[bIndex++] = pDestination->tDestParams.bDestParamVal[0];
                    pPayload[bIndex++] = pDestination->tDestParams.bDestParamVal[1];
                    psNciCtxt->tLogConnCtxt.bDestId = pDestination->tDestParams.bDestParamVal[0];
                }

                psNciCtxt->tLogConnCtxt.IfLogConnNtf = pNotify;
                psNciCtxt->tLogConnCtxt.IfLogConnNtfCtx = pContext;
                psNciCtxt->tSendPayload.pBuff = pPayload;
                psNciCtxt->tSendPayload.wPayloadSize = wPayloadLen;

                wStatus = phNciNfc_CreateConn(psNciCtxt->tLogConnCtxt.bDestId,psNciCtxt->tLogConnCtxt.tDestType);
                if(NFCSTATUS_SUCCESS == wStatus)
                {
                    PHNCINFC_INIT_SEQUENCE(psNciCtxt,gphNciNfc_LogConnOpenSequence);

                    wStatus = phNciNfc_GenericSequence((void *)psNciCtxt,NULL,NFCSTATUS_SUCCESS);
                    if(NFCSTATUS_PENDING != wStatus)
                    {
                        PH_LOG_NCI_CRIT_STR("Logical connection Sequence failed!");
                    }
                }
                else
                {
                    wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                    PH_LOG_NCI_INFO_STR("Failed to create entry in logical connection list");
                }
                phNciNfc_FreeSendPayloadBuff(psNciCtxt);
            }
            else
            {
                wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
                PH_LOG_NCI_INFO_STR(" Failed to open logical connection..");
            }
        }
    }
    return wStatus;
}

NFCSTATUS
phNciNfc_LogConnClose(
                        void      *psContext,
                        uint8_t   bDestId,
                        phNciNfc_DestType_t tDestType,
                        pphNciNfc_IfNotificationCb_t    pNotify,
                        void      *pContext
                    )
{
    NFCSTATUS           wStatus = NFCSTATUS_SUCCESS;
    pphNciNfc_Context_t psNciCtxt = psContext;
    uint8_t bConnId = 0x00;
    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == psNciCtxt) || (NULL == pNotify))
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        wStatus = phNciNfc_GetConnInfo(bDestId,tDestType,&bConnId);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            psNciCtxt->tLogConnCtxt.bConnId = bConnId;
            psNciCtxt->tLogConnCtxt.bDestId = bDestId;
            psNciCtxt->tLogConnCtxt.tDestType = tDestType;

            PHNCINFC_INIT_SEQUENCE(psNciCtxt,gphNciNfc_LogConnCloseSequence);

            wStatus = phNciNfc_GenericSequence((void *)psNciCtxt,NULL,NFCSTATUS_SUCCESS);

            if(NFCSTATUS_PENDING == wStatus)
            {
                psNciCtxt->tLogConnCtxt.IfLogConnNtf = pNotify;
                psNciCtxt->tLogConnCtxt.IfLogConnNtfCtx = pContext;
            }
        }
        else
        {
            PH_LOG_NCI_INFO_STR(" Notification ConnClose Request successful..");
            wStatus = NFCSTATUS_SUCCESS;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_SendConnCloseCmd(void *pContext)
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   CmdInfo;
    uint8_t  bConnId;
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)pContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        bConnId = psNciContext->tLogConnCtxt.bConnId;
        PH_LOG_NCI_INFO_STR(" Setting up commandInfo to be sent to lower layer ..");

        phOsalNfc_SetMemory(&CmdInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        CmdInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        CmdInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        CmdInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = \
            (phNciNfc_CoreNciCoreCmdOid_t)phNciNfc_e_NciCoreConnCloseCmdOid;
        CmdInfo.Buff = (uint8_t *)&bConnId;
        CmdInfo.wLen = 0x01;
        (psNciContext->RspBuffInfo.wLen) = 0;
        /* Sending Command to Nci Core */
        status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &CmdInfo,
            &(psNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, pContext);
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static NFCSTATUS
phNciNfc_CompleteConnSequence(void *pContext,
                            NFCSTATUS wStatus)
{
    pphNciNfc_Context_t pNciCtx = pContext;
    pphNciNfc_IfNotificationCb_t pNtfCb;
    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pNciCtx)
    {
        phNciNfc_FreeSendPayloadBuff(pNciCtx);

        if(NULL != pNciCtx->tLogConnCtxt.IfLogConnNtf)
        {
            pNtfCb = pNciCtx->tLogConnCtxt.IfLogConnNtf;
            pNciCtx->tLogConnCtxt.IfLogConnNtf = NULL;
            pNtfCb(pNciCtx->tLogConnCtxt.IfLogConnNtfCtx,wStatus,(void *)&pNciCtx->tLogConnCtxt.bConnId);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phNciNfc_SendConnOpenCmd (
                      void   *psContext
                     )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phNciNfc_CoreTxInfo_t   CmdInfo;
    phNciNfc_Context_t *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == psNciContext)
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        PH_LOG_NCI_INFO_STR(" Setting up commandInfo to be sent to lower layer ..");

        phOsalNfc_SetMemory(&CmdInfo, 0x00, sizeof(phNciNfc_CoreTxInfo_t));
        CmdInfo.tHeaderInfo.eMsgType = phNciNfc_e_NciCoreMsgTypeCntrlCmd;
        CmdInfo.tHeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
        CmdInfo.tHeaderInfo.Opcode_ID.OidType.NciCoreCmdOid = (phNciNfc_CoreNciCoreCmdOid_t)phNciNfc_e_NciCoreConnCreateCmdOid;
        CmdInfo.Buff = (uint8_t *)psNciContext->tSendPayload.pBuff;
        CmdInfo.wLen = psNciContext->tSendPayload.wPayloadSize;
        (psNciContext->RspBuffInfo.wLen) = 0;
        /* Sending Command to Nci Core */
        status = phNciNfc_CoreIfTxRx(&(psNciContext->NciCoreContext), &CmdInfo,
            &(psNciContext->RspBuffInfo), PHNCINFC_NCI_CMD_RSP_TIMEOUT,
            (pphNciNfc_CoreIfNtf_t)&phNciNfc_GenericSequence, psContext);
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_LogConnResp(
                        void         *psContext,
                        NFCSTATUS    wStatus
                    )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    uint8_t                         bIndex = 0x00;
    phNciNfc_Context_t              *psNciContext = (phNciNfc_Context_t *)psContext;
    uint8_t                         bConnId;
    uint8_t                         bNumCredits;
    uint8_t                         bMaxDpldSize;               

    PH_LOG_NCI_FUNC_ENTRY();

    if( NULL == psNciContext )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if( (0 == psNciContext->RspBuffInfo.wLen)
            || (PH_NCINFC_STATUS_OK != wStatus)
            || (NULL == (psNciContext->RspBuffInfo.pBuff))
            )
        {
            /* NOTE:- ( TODO) In this (HW/Board) fail scenario,get the exact status code from response handler
            (or from Nci Context) and use the same in the status below */
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Conn Response invalid..");
        }
        else
        {
            /* Validate the response received */
            if(NFCSTATUS_SUCCESS == (NFCSTATUS)psNciContext->RspBuffInfo.pBuff[bIndex++])
            {
                /* Store the information related to logical connection */
                bMaxDpldSize = psNciContext->RspBuffInfo.pBuff[bIndex++];
                bNumCredits = psNciContext->RspBuffInfo.pBuff[bIndex++];
                bConnId = psNciContext->RspBuffInfo.pBuff[bIndex++];

                psNciContext->tLogConnCtxt.bConnId = bConnId;
                status = phNciNfc_UpdateConnInfo(psNciContext->tLogConnCtxt.bDestId, psNciContext->tLogConnCtxt.tDestType,
                                                 bConnId, bNumCredits, bMaxDpldSize);
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

static
NFCSTATUS
phNciNfc_LogConnCloseResp(
                        void         *psContext,
                        NFCSTATUS    wStatus
                    )
{
    NFCSTATUS                        status = NFCSTATUS_SUCCESS;
    uint8_t                          bIndex = 0x00;
    phNciNfc_Context_t              *psNciContext = (phNciNfc_Context_t *)psContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == psNciContext) )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        if( (0 == psNciContext->RspBuffInfo.wLen)
            || (PH_NCINFC_STATUS_OK != wStatus)
            || (NULL == (psNciContext->RspBuffInfo.pBuff))
            )
        {
            /* NOTE:- ( TODO) In this (HW/Board) fail scenario,get the exact status code from response handler
            (or from Nci Context) and use the same in the status below */
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
            PH_LOG_NCI_INFO_STR(" Conn Response invalid..");
        }
        else
        {
            /* Validate the response received */
            if(NFCSTATUS_SUCCESS == (NFCSTATUS)psNciContext->RspBuffInfo.pBuff[bIndex++])
            {
                status = phNciNfc_CloseConn(psNciContext->tLogConnCtxt.bConnId);
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}
