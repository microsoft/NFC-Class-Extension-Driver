/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h" 

#include "phNciNfc_CoreUtils.tmh"

static NFCSTATUS phNciNfc_CoreUtilsValidateRspPktOID(
                                            uint8_t bGID,
                                            uint8_t bOID
                                           );

static NFCSTATUS phNciNfc_CoreUtilsValidateNtfPktOID(
                                            uint8_t bGID,
                                            uint8_t bOID
                                           );

NFCSTATUS phNciNfc_CoreUtilsValidateGID(uint8_t bGID)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;

    PH_LOG_NCI_FUNC_ENTRY();
    switch(bGID)
    {
        case phNciNfc_e_CoreNciCoreGid:
        case phNciNfc_e_CoreRfMgtGid:
        case phNciNfc_e_CoreNfceeMgtGid:
        case phNciNfc_e_CorePropGid:
            wStatus = NFCSTATUS_SUCCESS;
            break;
        default:
            break;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phNciNfc_CoreUtilsValidateCtrlPktOID(uint8_t bMT, uint8_t bGID, uint8_t bOID)
{
    NFCSTATUS bRetNfcStat;

    PH_LOG_NCI_FUNC_ENTRY();
    /* If received packet is a response packet */
    if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == bMT)
    {
        bRetNfcStat = phNciNfc_CoreUtilsValidateRspPktOID(bGID, bOID);
    }
    /* If received packet is a notification packet (bMT == phNciNfc_e_NciCoreMsgTypeCntrlNtf) */
    else
    {
        bRetNfcStat = phNciNfc_CoreUtilsValidateNtfPktOID(bGID, bOID);
    }
    PH_LOG_NCI_FUNC_EXIT();
    return  bRetNfcStat;
}

static NFCSTATUS phNciNfc_CoreUtilsValidateRspPktOID(uint8_t bGID, uint8_t bOID)
{
    NFCSTATUS bRetNfcStat = NFCSTATUS_FAILED;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check whether the OID of the received packet is within the
     * allowed range w.r.t GID of the response packet
     */
    if(bGID == phNciNfc_e_CoreNciCoreGid)
    {
        switch(bOID)
        {
            case phNciNfc_e_NciCoreResetRspOid:
            case phNciNfc_e_NciCoreInitRspOid:
            case phNciNfc_e_NciCoreSetConfigRspOid:
            case phNciNfc_e_NciCoreGetConfigRspOid:
            case phNciNfc_e_NciCoreDhConnRspOid:
            case phNciNfc_e_NciCoreConnCloseRspOid:
            case phNciNfc_e_NciCoreSetPowerSubStateRspOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if(phNciNfc_e_CoreRfMgtGid == bGID)
    {
        switch(bOID)
        {
            case phNciNfc_e_RfMgtRfDiscoverMapRspOid:
            case phNciNfc_e_RfMgtRfSetRoutingRspOid:
            case phNciNfc_e_RfMgtRfGetRoutingRspOid:
            case phNciNfc_e_RfMgtRfDiscoverRspOid:
            case phNciNfc_e_RfMgtRfDiscSelectRspOid:
            case phNciNfc_e_RfMgtRfDeactivateRspOid:
            case phNciNfc_e_RfMgtRfT3tPollingRspOid:
            case phNciNfc_e_RfMgtRfParamUpdateRspOid:
            case phNciNfc_e_RfMgtRfIsoDepPresChkRspOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if(phNciNfc_e_CoreNfceeMgtGid == bGID)
    {
        switch(bOID)
        {
            case phNciNfc_e_NfceeMgtNfceeDiscRspOid:
            case phNciNfc_e_NfceeMgtModeSetRspOid:
            case phNciNfc_e_NfceeMgtPowerAndLinkCtrlRspOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if (phNciNfc_e_CorePropGid == bGID)
    {
        switch(bOID)
        {
            case phNciNfc_e_CorePropSetPwrModeRspOid:
            case phNciNfc_e_CorePropEnableExtnRspOid:
            case phNciNfc_e_CorePropIsoDepChkPresRspOid:
            case phNciNfc_e_CorePropDhListenFilterRspOid:
            case  phNciNfc_e_CorePropTestSwpRspOid:
            case phNciNfc_e_CorePropTestPrbsRspOid:
            case phNciNfc_e_CorePropTestAntennaRspOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
             break;
         }
    }
    else
    {
        /* do nothing */
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bRetNfcStat;
}

static NFCSTATUS phNciNfc_CoreUtilsValidateNtfPktOID(uint8_t bGID, uint8_t bOID)
{
    NFCSTATUS bRetNfcStat = NFCSTATUS_FAILED;

    PH_LOG_NCI_FUNC_ENTRY();
    /* Check whether the OID of the received packet is within the
     * allowed range w.r.t GID of the notification packet
     */
    if(bGID == phNciNfc_e_CoreNciCoreGid)
    {
        switch(bOID)
        {
            case phNciNfc_e_NciCoreResetNtfOid:
            case phNciNfc_e_NciCoreConnCreditNtfOid:
            case phNciNfc_e_NciCoreGenericErrNtfOid:
            case phNciNfc_e_NciCoreInterfaceErrNtfOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if(bGID == phNciNfc_e_CoreRfMgtGid)
    {
        switch(bOID)
        {
            case phNciNfc_e_RfMgtRfGetListenModeRoutingNtfOid:
            case phNciNfc_e_RfMgtRfDiscoverNtfOid:
            case phNciNfc_e_RfMgtRfIntfActivatedNtfOid:
            case phNciNfc_e_RfMgtRfDeactivateNtfOid:
            case phNciNfc_e_RfMgtRfFieldInfoNtfOid:
            case phNciNfc_e_RfMgtRfT3tPollingNtfOid:
            case phNciNfc_e_RfMgtRfNfceeActionNtfOid:
            case phNciNfc_e_RfMgtRfNfceeDiscoveryReqNtfOid:
            case phNciNfc_e_RfMgtRfIsoDepPresChkNtfOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if(bGID == phNciNfc_e_CoreNfceeMgtGid)
    {
        switch(bOID)
        {
            case phNciNfc_e_NfceeMgtNfceeDiscNtfOid:
            case phNciNfc_e_NfceeMgtModeSetNtfOid:
            case phNciNfc_e_NfceeMgtStatusNtfOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else if(bGID == phNciNfc_e_CorePropGid)
    {
        switch(bOID)
        {
            case phNciNfc_e_CorePropIsoDepPrsnChkNtfOid:
            case phNciNfc_e_CorePropTestSwpNtfOid:
            case phNciNfc_e_CorePropTagDetectorNtfOid:
                bRetNfcStat = NFCSTATUS_SUCCESS;
                break;
            default:
                PH_LOG_NCI_CRIT_STR("Unknown OID received");
                break;
        }
    }
    else
    {
        /* (Proprietary - yet to be implemented) */
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bRetNfcStat;
}

NFCSTATUS phNciNfc_CoreUtilsUpdatePktInfo(pphNciNfc_CoreContext_t pContext,
                                                    uint8_t *pBuff, uint16_t wLength)
{
    uint8_t bMsgType = 0;
    uint8_t bGid = 0;
    uint8_t bOid = 0;
    uint8_t bConn_ID = 0;
    uint8_t bPayloadLen = 0;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();

    if((NULL != pContext) && (NULL != pBuff))
    {
        /* Verify payload length */
        bPayloadLen = PHNCINFC_CORE_GET_LENBYTE(pBuff);
        if((PHNCINFC_CORE_PKT_HEADER_LEN + bPayloadLen) != wLength)
        {
            PH_LOG_NCI_CRIT_STR("Incorrect payload length");
            wStatus = PH_NCINFC_STATUS_INVALID_PKT_LEN;
        }
        else
        {
            wStatus = NFCSTATUS_SUCCESS;
        }

        if(NFCSTATUS_SUCCESS == wStatus)
        {
            /* Get the Message Type from the received packet */
            bMsgType = PHNCINFC_CORE_GET_MT(pBuff);
            switch(bMsgType)
            {
                case phNciNfc_e_NciCoreMsgTypeData:
                {
                    bConn_ID = (uint8_t) PHNCINFC_CORE_GET_CONNID(pBuff);
                    /* Update packet info with connection id */
                    pContext->tReceiveInfo.HeaderInfo.bConn_ID = bConn_ID;
                    pContext->tReceiveInfo.HeaderInfo.Group_ID = phNciNfc_e_CoreNciCoreGid;
                    pContext->tReceiveInfo.HeaderInfo.Opcode_ID.Val = 0;
                    wStatus = PH_NCINFC_STATUS_OK;
                }
                break;
                case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
                case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                {
                    /* Get GID and OID of the received packet */
                    bGid = PHNCINFC_CORE_GET_GID(pBuff);
                    bOid = PHNCINFC_CORE_GET_OID(pBuff);

                    /* Validate GID */
                    wStatus  = phNciNfc_CoreUtilsValidateGID(bGid);
                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        /* Validate for Oid */
                        wStatus = phNciNfc_CoreUtilsValidateCtrlPktOID(bMsgType, bGid, bOid);
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("Unknown GID received");
                    }

                    if(NFCSTATUS_SUCCESS == wStatus)
                    {
                        /* Update GID and OID values in packet info structure */
                        pContext->tReceiveInfo.HeaderInfo.Group_ID = (phNciNfc_CoreGid_t)bGid;
                        pContext->tReceiveInfo.HeaderInfo.Opcode_ID.Val = bOid;
                        pContext->tReceiveInfo.HeaderInfo.bConn_ID = 0;
                    }
                }
                break;
                default:
                {
                    PH_LOG_NCI_CRIT_STR("Invalid message type");
                    wStatus = PH_NCINFC_STATUS_INVALID_MT;
                }
                break;
            }
        }
        /* Update packet information */
        if(PH_NCINFC_STATUS_OK == wStatus)
        {
            /*Update message type*/
            pContext->tReceiveInfo.HeaderInfo.eMsgType = (phNciNfc_NciCoreMsgType_t)bMsgType;
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
