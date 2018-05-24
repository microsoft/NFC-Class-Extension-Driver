/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_LoglConnMgmt.tmh"

static void phNciNfc_WaitCreditTimerCb(uint32_t TimerId, void *pContext);
static NFCSTATUS phNciNfc_GetConnIndex(uint8_t bConnId, uint8_t *pbConnIdx);

static phNciNfc_LogConnMgmt_Int_t gphNciNfc_ConnMgmtInt;

NFCSTATUS
phNciNfc_LogConnMgmtInit()
{
    NFCSTATUS   status = NFCSTATUS_SUCCESS;

    PH_LOG_NCI_FUNC_ENTRY();

    phOsalNfc_SetMemory(&gphNciNfc_ConnMgmtInt, 0x00, sizeof(phNciNfc_LogConnMgmt_Int_t));

    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].tConn.bConnId =  CONNRFTYPE_STATIC;
    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].tConn.bMaxDpldSize = 0;
    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].tConn.bNumCredits = 0;
    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bDestId = UNASSIGNED_DESTID;
    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bDestType = phNciNfc_e_UNKNOWN_DEST_TYPE;
    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bIfActive = FALSE;

    gphNciNfc_ConnMgmtInt.tConnInfo.bOpenConns = 1;

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

void
phNciNfc_LogConnMgmtDeInit()
{
    if(gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus == 1)
    {
        PH_LOG_NCI_INFO_STR("Stopping Credit await timer..");
        (void)phOsalNfc_Timer_Stop(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId);
        (void)phOsalNfc_Timer_Delete(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId);
    }

    phOsalNfc_SetMemory(&gphNciNfc_ConnMgmtInt, 0x00, sizeof(phNciNfc_LogConnMgmt_Int_t));
}

uint8_t phNciNfc_GetConnCount()
{
    return gphNciNfc_ConnMgmtInt.tConnInfo.bOpenConns;
}

NFCSTATUS
phNciNfc_CreateConn(
                    uint8_t bDestId,
                    phNciNfc_DestType_t bDestType
                    )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bConnIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    for(bConnIdx = 1; bConnIdx < (MAX_LOGICAL_CONNS+1) ; bConnIdx++)
    {
        if(FALSE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bIfActive)
        {
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bDestId = bDestId;
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bDestType = bDestType;
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bIfActive = FALSE;

            wStatus = NFCSTATUS_SUCCESS;
            break;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_CloseConn(
                   uint8_t bConnId
                   )
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bConnIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    if(gphNciNfc_ConnMgmtInt.tConnInfo.bOpenConns > 1)
    {
        for(bConnIdx = 1; bConnIdx < (MAX_LOGICAL_CONNS+1) ; bConnIdx++)
        {
            if(TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bIfActive)
            {
                if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bConnId)
                {
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bConnId = INVALID_CONN_ID;
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bMaxDpldSize = 0;
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits = FLOW_CONTROL_DISABLED;

                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bDestId = UNASSIGNED_DESTID;
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bDestType = phNciNfc_e_UNKNOWN_DEST_TYPE;
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].bIfActive = FALSE;

                    gphNciNfc_ConnMgmtInt.tConnInfo.bOpenConns--;

                    wStatus = NFCSTATUS_SUCCESS;
                    break;
                }
            }
        }
    }
    else
    {
        PH_LOG_NCI_INFO_STR("No connection left to close");
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_GetConnId(
                   void    *pDevHandle,
                   uint8_t *pConnId
                   )
{
    NFCSTATUS   status = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    *pConnId = INVALID_CONN_ID;

    if( (NULL == pConnId) || (NULL == pDevHandle)
      )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    else
    {
        for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
        {
            if(pDevHandle == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].pActvDevHandle)
            {
                if(TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive)
                {
                    *pConnId = gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId;
                    status = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_INFO_STR(" Interface not active for this connection");
                }
                break;
            }
        }
    }
    PH_LOG_NCI_INFO_STR("status = %!NFCSTATUS!", status);
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_GetConnInfo (
                     uint8_t    bDestId,
                     phNciNfc_DestType_t tDestType,
                     uint8_t    *pConnId
                    )
{
    NFCSTATUS   status = NFCSTATUS_FAILED;
    uint8_t     ConnIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == pConnId)
      )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Context Param..");
    }
    else
    {
        for(ConnIdx = 0; ConnIdx < (MAX_LOGICAL_CONNS+1) ; ConnIdx++)
        {
            if(TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bIfActive)
            {
                if( (bDestId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bDestId) &&
                    (tDestType == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bDestType) )
                {
                    *pConnId = gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].tConn.bConnId;
                    status = NFCSTATUS_SUCCESS;
                    break;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_UpdateConnInfo(
                        uint8_t bDestId,
                        phNciNfc_DestType_t tDestType,
                        uint8_t bConnId,
                        uint8_t bInitialCredits,
                        uint8_t bMaxDpldSize
                        )
{
    NFCSTATUS   status = NFCSTATUS_FAILED;
    uint8_t     ConnIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    for(ConnIdx = 1; ConnIdx < (MAX_LOGICAL_CONNS+1) ; ConnIdx++)
    {
        if( (bDestId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bDestId) &&
            (tDestType == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bDestType) )
        {
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].tConn.bConnId = bConnId;
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].tConn.bMaxDpldSize = bMaxDpldSize;
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].tConn.bNumCredits = bInitialCredits;
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[ConnIdx].bIfActive = TRUE;

            gphNciNfc_ConnMgmtInt.tConnInfo.bOpenConns++;
            status = NFCSTATUS_SUCCESS;
            break;
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_UpdateConnDestInfo(uint8_t  bDestId,
                            phNciNfc_DestType_t tDestType,
                            void *pHandle)
{
    NFCSTATUS    wStatus = NFCSTATUS_SUCCESS;
    uint8_t      bConnIdx;
    uint8_t      bConnId;
    PH_LOG_NCI_FUNC_ENTRY();

    if( (phNciNfc_e_NFCEE == tDestType) ||
        (phNciNfc_e_NFCC_LOOPBACK == tDestType) )
    {
        wStatus = phNciNfc_GetConnInfo(bDestId,tDestType,&bConnId);

        if( (NFCSTATUS_SUCCESS != wStatus) ||
            (NFCSTATUS_SUCCESS != phNciNfc_GetConnIndex(bConnId,&bConnIdx)) )
        {
            wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        }
        else
        {
            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].pActvDevHandle = pHandle;
        }
    }
    else if(phNciNfc_e_REMOTE_NFC_ENDPOINT == tDestType && NULL != pHandle)
    {
        pphNciNfc_RemoteDevInformation_t  pActvDev = (pphNciNfc_RemoteDevInformation_t)pHandle;

        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bDestId = pActvDev->bRfDiscId;
        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bDestType = tDestType;
        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].tConn.bMaxDpldSize = pActvDev->bMaxPayLoadSize;
        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].tConn.bNumCredits = pActvDev->bInitialCredit;
        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].bIfActive = TRUE;
        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[CONNRFTYPE_STATIC].pActvDevHandle = pActvDev;
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_CRIT_STR("Invalid parameters supplied: bDestId=%d tDestType=%!phNciNfc_DestType_t! pHandle=%p", bDestId, tDestType, pHandle);
    }

    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_GetConnCredits(
                            uint8_t   bConnId,
                            uint8_t  *pCredits
                            )
{
    NFCSTATUS   wStatus = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == pCredits)
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    else
    {
        for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
        {
            if(bConnId == (gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId) )
            {
                if((TRUE == (gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive)) ||
                    (CONNRFTYPE_STATIC == (gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)))
                {
                    *pCredits = (gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits);
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Interface not active for this connection");
                }
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS phNciNfc_RegForConnCredits(
                                     uint8_t bConnId,
                                     pphNciNfc_ConnCreditsNtf_t pNotify,
                                     void *pContext,
                                     uint32_t CreditTo
                                     )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t     bConnListIdx;
    uint32_t TimerId;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL == pNotify) || (NULL == pContext))
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Caller Layer Param(s) received ..");
    }
    else
    {
        for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
        {
            if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)
            {
                if((TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive) ||
                    (CONNRFTYPE_STATIC == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId))
                {
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR(" Interface not active for this connection");
                    wStatus = NFCSTATUS_FAILED;
                }
                break;
            }
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            gphNciNfc_ConnMgmtInt.bConnId = bConnId;
            gphNciNfc_ConnMgmtInt.pCrdtsNotify = pNotify;
            gphNciNfc_ConnMgmtInt.pContext = pContext;

            TimerId = phOsalNfc_Timer_Create();

            if (PH_OSALNFC_TIMER_ID_INVALID == TimerId)
            {
                PH_LOG_NCI_WARN_STR("Credit wait Timer Create failed!!");
                wStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                PH_LOG_NCI_INFO_STR("Credit wait Timer Created Successfully");

                gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId = TimerId;
                gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus = 0;

                PH_LOG_NCI_INFO_STR("Credit timeout set to %d, for ConnId 0x%x",CreditTo, bConnId);

                wStatus = phOsalNfc_Timer_Start(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId,
                                                CreditTo,
                                                &phNciNfc_WaitCreditTimerCb,
                                                &gphNciNfc_ConnMgmtInt);

                if (NFCSTATUS_SUCCESS == wStatus)
                {
                    PH_LOG_NCI_INFO_STR("Credit wait timer started..");
                    gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus = 1;
                    /* Set the CreditsAwaited flag to indicate credit notif function to call the registered CB */
                    gphNciNfc_ConnMgmtInt.bCreditsAwaited = TRUE;
                }
                else
                {
                     PH_LOG_NCI_WARN_STR("Failed to start credit ntf wait timer!");
                     (void )phOsalNfc_Timer_Delete(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId);
                     gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId = 0;
                     wStatus = NFCSTATUS_FAILED;
                }
            }
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS
phNciNfc_ProcessConnCreditNtf(
                        void               *psContext,
                        void               *pInfo,
                        NFCSTATUS          wStatus
                    )
{
    NFCSTATUS                           status = NFCSTATUS_SUCCESS;
    pphNciNfc_CoreContext_t             pCoreCtx = (pphNciNfc_CoreContext_t)psContext;
    pphNciNfc_TransactInfo_t            pTransInfo = (pphNciNfc_TransactInfo_t)pInfo;
    phNciNfc_CoreGid_t                  tNtfGid;
    phNciNfc_CoreNciCoreNtfOid_t        tNtfOid;
    uint8_t                             bConnIdx;
    uint8_t                             bNewCredits = 0;
    uint8_t                             bNumOfEntries;
    uint8_t                             bEntrIdx;

    UNUSED(psContext);

    PH_LOG_NCI_FUNC_ENTRY();

    if( (NULL == pTransInfo) || (NULL == pCoreCtx) ||
        (NULL == pTransInfo->pbuffer) ||(0 == pTransInfo->wLength)
        || (PH_NCINFC_STATUS_OK != wStatus)
        )
    {
        status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_FAILED);
        PH_LOG_NCI_INFO_STR(" Conn Notification invalid..");
    }
    else
    {
        tNtfGid = pCoreCtx->tReceiveInfo.HeaderInfo.Group_ID;
        tNtfOid = pCoreCtx->tReceiveInfo.HeaderInfo.Opcode_ID.OidType.NciCoreNtfOid;

        if(phNciNfc_e_CoreNciCoreGid == tNtfGid)
        {
            if(phNciNfc_e_NciCoreConnCreditNtfOid == tNtfOid)
            {
                PH_LOG_NCI_INFO_STR(" Core ConnCredits Notification received..");
                bNumOfEntries = pTransInfo->pbuffer[0];

                for(bEntrIdx = 0; bEntrIdx < bNumOfEntries; bEntrIdx++)
                {
                    status = phNciNfc_GetConnIndex(pTransInfo->pbuffer[1], &bConnIdx);

                    if(NFCSTATUS_SUCCESS == status)
                    {
                        gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits += pTransInfo->pbuffer[2];

                        if(gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits > MAX_CREDITS_LIMIT)
                        {
                            gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits = MAX_CREDITS_LIMIT;
                            PH_LOG_NCI_INFO_STR(" Credits Limit xceeded for this Conn, Ignoring notif value ..");
                        }
                        else
                        {
                            PH_LOG_NCI_INFO_STR(" Credits updated for ConnId: 0x%x to 0x%x",
                                gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bConnId,
                                gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits);
                        }

                        bNewCredits = gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnIdx].tConn.bNumCredits;
                    }
                    else
                    {
                        PH_LOG_NCI_INFO_X32MSG( " Failed to get Conn details for index..",bConnIdx);
                    }
                }
            }
            else
            {
                status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
                PH_LOG_NCI_INFO_STR(" Invalid Oid received..");
            }
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
            PH_LOG_NCI_INFO_STR(" Invalid Gid received..");
        }
    }

    PH_LOG_NCI_INFO_STR("Credits awaited: %d, timerstatus: %d",
                        gphNciNfc_ConnMgmtInt.bCreditsAwaited,
                        gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus);

    /* Invoke the registered CB function if waiting for credits */
    if(TRUE == gphNciNfc_ConnMgmtInt.bCreditsAwaited)
    {
        if(gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus == 1)
        {
            PH_LOG_NCI_INFO_STR("Stopping Credit await timer..");

            (void)phOsalNfc_Timer_Stop(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId);
            (void)phOsalNfc_Timer_Delete(gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId);

            gphNciNfc_ConnMgmtInt.tCWTimerIf.TimerStatus = 0;
            gphNciNfc_ConnMgmtInt.tCWTimerIf.dwTimerId = 0;
        }
        gphNciNfc_ConnMgmtInt.bCreditsAwaited = FALSE;

        if(NULL != gphNciNfc_ConnMgmtInt.pCrdtsNotify)
        {
            (void)(gphNciNfc_ConnMgmtInt.pCrdtsNotify)((gphNciNfc_ConnMgmtInt.pContext),bNewCredits,status);
        }
        else
        {
            PH_LOG_NCI_WARN_STR("No CB registered for credits availability!!");
        }
    }

    PH_LOG_NCI_FUNC_EXIT();

    return status;
}

NFCSTATUS
phNciNfc_GetConnMaxPldSz(
                           uint8_t   bConnId,
                           uint8_t *pMaxPldSz
                           )
{
    NFCSTATUS   wStatus = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == pMaxPldSz)
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    else
    {
        for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
        {
            if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)
            {
                if((TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive) ||
                    (CONNRFTYPE_STATIC == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId))
                {
                    *pMaxPldSz = gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bMaxDpldSize;
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_INFO_STR(" Interface not active for this connection");
                }
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS phNciNfc_IncrConnCredits(
                                   uint8_t bConnId,
                                   uint8_t bVal
                                   )
{
    NFCSTATUS   wStatus = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
    {
        if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)
        {
            if((TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive) ||
                (CONNRFTYPE_STATIC == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId))
            {
                gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits += bVal;

                if(gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits > MAX_CREDITS_LIMIT)
                {
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits = MAX_CREDITS_LIMIT;
                    PH_LOG_NCI_WARN_STR(" Credit Limit xceeded for this Conn,Rounding to max value ..");
                }
                wStatus = NFCSTATUS_SUCCESS;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR(" Interface not active for this connection");
            }
            break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

NFCSTATUS phNciNfc_DecrConnCredit(
                                   uint8_t bConnId
                                   )
{
    NFCSTATUS   wStatus = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
    {
        if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)
        {
            if((TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive) ||
                (CONNRFTYPE_STATIC == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId))
            {
                if(gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits > 0)
                {
                    gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bNumCredits -= 1;
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_WARN_STR("No Credits available for this connId!!");
                }
            }
            else
            {
                PH_LOG_NCI_CRIT_STR(" Interface not active for this connection");
            }
            break;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}

static void phNciNfc_WaitCreditTimerCb(uint32_t TimerId, void *pContext)
{
    NFCSTATUS wStatus = NFCSTATUS_RF_TIMEOUT;
    phNciNfc_LogConnMgmt_Int_t *pConnMgmtCtxt = (phNciNfc_LogConnMgmt_Int_t *)pContext;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL != pConnMgmtCtxt)
    {
        if(TimerId == pConnMgmtCtxt->tCWTimerIf.dwTimerId)
        {
            pConnMgmtCtxt->tCWTimerIf.TimerStatus = 0; /* Reset timer status flag */

            (void)phOsalNfc_Timer_Stop(pConnMgmtCtxt->tCWTimerIf.dwTimerId);
            (void)phOsalNfc_Timer_Delete(pConnMgmtCtxt->tCWTimerIf.dwTimerId);
            pConnMgmtCtxt->tCWTimerIf.dwTimerId = 0;
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid wait credit timer ID");
            phOsalNfc_RaiseException(phOsalNfc_e_InternalErr, 1);
        }

        if(TRUE == pConnMgmtCtxt->bCreditsAwaited)
        {
            pConnMgmtCtxt->bCreditsAwaited = FALSE;
        }

        if (NULL != pConnMgmtCtxt->pCrdtsNotify)
        {
            (void)(pConnMgmtCtxt->pCrdtsNotify)(pConnMgmtCtxt->pContext,0,wStatus);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return;
}

static NFCSTATUS phNciNfc_GetConnIndex(uint8_t bConnId, uint8_t *pbConnIdx)
{
    NFCSTATUS   wStatus = NFCSTATUS_FAILED;
    uint8_t     bConnListIdx;

    PH_LOG_NCI_FUNC_ENTRY();

    if(NULL == pbConnIdx)
    {
        wStatus = PHNFCSTVAL(CID_NFC_NCI, NFCSTATUS_INVALID_PARAMETER);
        PH_LOG_NCI_INFO_STR(" Invalid Params..");
    }
    else
    {
        for(bConnListIdx = 0; bConnListIdx < (MAX_LOGICAL_CONNS+1) ; bConnListIdx++)
        {
            if(bConnId == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId)
            {
                if((TRUE == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].bIfActive) ||
                    (CONNRFTYPE_STATIC == gphNciNfc_ConnMgmtInt.tConnInfo.tConnList[bConnListIdx].tConn.bConnId))
                {
                    *pbConnIdx = bConnListIdx;
                    wStatus = NFCSTATUS_SUCCESS;
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR(" Interface not active for this connection");
                }
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();

    return wStatus;
}
