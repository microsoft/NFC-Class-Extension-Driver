/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_CoreRecvMgr.h"
#include "phNciNfc_CoreMemMgr.h"

#include "phNciNfc_CoreRecvMgr.tmh"


#define PHNCINFC_CORENTFMGR_DEFAULT_SIZE                (600u)

static NFCSTATUS phNciNfc_CoreGetIndex(uint8_t bGetEmptySlot,
                                       void *pRegList,
                                       pphNciNfc_CoreRegInfo_t pRegInfo,
                                       phNciNfc_NciCoreMsgType_t eMsgType,
                                       uint8_t *bIndex);

static NFCSTATUS
phNciNfc_CoreRegister(void *pRegList,
                      pphNciNfc_CoreRegInfo_t pRegInfo,
                      phNciNfc_NciCoreMsgType_t eMsgType);

static NFCSTATUS
phNciNfc_CoreDeRegister(void *pRegList,
                        pphNciNfc_CoreRegInfo_t pRegInfo,
                        phNciNfc_NciCoreMsgType_t eMsgType);

static NFCSTATUS
phNciNfc_CoreInvokeCb(pphNciNfc_CoreRegInfo_t pRegList,
                      NFCSTATUS wStatus,
                      pphNciNfc_CoreRegInfo_t pRegInfo,
                      phNciNfc_NciCoreMsgType_t eMsgType,
                      phNciNfc_TransactInfo_t *pTransInfo);

NFCSTATUS
phNciNfc_CoreRecvMgrInit(pphNciNfc_CoreContext_t pCoreCtx)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCoreCtx)
    {
        /* Initialize Response, Data and Notification call backs to NULL */
        phOsalNfc_SetMemory(&pCoreCtx->tRspCtx, 0x00, sizeof(phNciNfc_CoreRspRegContext_t));
        phOsalNfc_SetMemory(&pCoreCtx->tNtfCtx, 0x00, sizeof(phNciNfc_CoreNtfRegContext_t));
        phOsalNfc_SetMemory(&pCoreCtx->tDataCtx, 0x00, sizeof(phNciNfc_CoreDataRegContext_t));
        /*Initialise Receive Buffer*/
        pCoreCtx->tReceiveInfo.ListHead.pNext = NULL;
        pCoreCtx->tReceiveInfo.ListHead.tMem.wLen = 0;
        pCoreCtx->tReceiveInfo.wPayloadSize = 0;
        pCoreCtx->tReceiveInfo.wNumOfNodes = 1; /*By default Head is a one node*/
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_CoreRecvMgrRelease(pphNciNfc_CoreContext_t pCoreCtx)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCoreCtx)
    {
        phOsalNfc_SetMemory(&pCoreCtx->tRspCtx, 0x00, sizeof(phNciNfc_CoreRspRegContext_t));
        phOsalNfc_SetMemory(&pCoreCtx->tNtfCtx, 0x00, sizeof(phNciNfc_CoreNtfRegContext_t));
        phOsalNfc_SetMemory(&pCoreCtx->tDataCtx, 0x00, sizeof(phNciNfc_CoreDataRegContext_t));
        wStatus = NFCSTATUS_SUCCESS;
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_CoreRecvMgrRegisterCb(void *pCtx,
                               pphNciNfc_CoreRegInfo_t pRegInfo,
                               phNciNfc_NciCoreMsgType_t eMsgType)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t)pCtx;
    uint8_t bCheckForEntry = 1;
    void *pRegList = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pRegInfo))
    {
        if(NULL != pRegInfo->pNotifyCb)
        {
            wStatus = NFCSTATUS_SUCCESS;
            switch(eMsgType)
            {
                case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
                    pRegList = (void *) pCoreCtx->tRspCtx.aRspRegList;
                    PH_LOG_NCI_INFO_STR("Registering for response message...");

                    /* Over write the existing Rsp entry if any exist */
                    bCheckForEntry = 0;
                    break;

                case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                    pRegList = (void *) pCoreCtx->tNtfCtx.aNtfRegList;
                    PH_LOG_NCI_INFO_STR("Registering for notification message...");

                    /* Whether there are notification registrations already present in the list or not,
                       register for this new notification */
                    bCheckForEntry = 0;
                    break;

                case phNciNfc_e_NciCoreMsgTypeData:
                    pRegList = (void *) pCoreCtx->tDataCtx.aDataRegList;
                    PH_LOG_NCI_INFO_STR("Registering for data message...");
                    /* Over write the existing Rsp entry if any exist */
                    bCheckForEntry = 0;
                    break;

                default:
                    PH_LOG_NCI_WARN_STR("Invalid message type");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    break;
            }
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = phNciNfc_CoreRegister(pRegList,pRegInfo,eMsgType);
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Invalid call back function pointer to register");
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_CoreRecvMgrDeRegisterCb(void *pCtx,
                                 pphNciNfc_CoreRegInfo_t pRegInfo,
                                 phNciNfc_NciCoreMsgType_t eMsgType)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t)pCtx;
    void *pRegList = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pRegInfo))
    {
        if(NULL != pRegInfo->pNotifyCb)
        {
            wStatus = NFCSTATUS_SUCCESS;
            switch(eMsgType)
            {
                case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
                    pRegList = (void *) pCoreCtx->tRspCtx.aRspRegList;
                    PH_LOG_NCI_INFO_STR("De-registering response message call back...");
                    break;

                case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                    pRegList = (void *) pCoreCtx->tNtfCtx.aNtfRegList;
                    PH_LOG_NCI_INFO_STR("De-registering notification message  call back...");
                    break;

                case phNciNfc_e_NciCoreMsgTypeData:
                    pRegList = (void *) pCoreCtx->tDataCtx.aDataRegList;
                    PH_LOG_NCI_INFO_STR("De-registering data message call back...");
                    break;

                default:
                    PH_LOG_NCI_WARN_STR("Invalid message type");
                    wStatus = NFCSTATUS_INVALID_PARAMETER;
                    break;
            }
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                wStatus = phNciNfc_CoreDeRegister(pRegList,pRegInfo,eMsgType);
            }
        }
        else
        {
            PH_LOG_NCI_WARN_STR("Invalid call back function pointer to register");
        }
    }
    else
    {
        PH_LOG_NCI_WARN_STR("Invalid input parameters");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

void
phNciNfc_CoreRecvMgrDeRegisterAll(void *pCtx)
{
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t)pCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCoreCtx)
    {
        /* Clear all response call back function information */
        phOsalNfc_SetMemory(&pCoreCtx->tRspCtx,'\0',sizeof(phNciNfc_CoreRspRegContext_t));
        /* Clear all data call back function information */
        phOsalNfc_SetMemory(&pCoreCtx->tDataCtx,'\0',sizeof(phNciNfc_CoreDataRegContext_t));
        /* Clear all notification call back function information */
        phOsalNfc_SetMemory(&pCoreCtx->tNtfCtx,'\0',sizeof(phNciNfc_CoreNtfRegContext_t));
        PH_LOG_NCI_INFO_STR("All (Rsp/Data/Ntf) call back registrations information cleared!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

void
phNciNfc_CoreRecvMgrDeRegDataCb(void *pCtx, uint8_t bConnId)
{
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t)pCtx;
    uint8_t bIndex;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pCoreCtx)
    {
        for(bIndex =0; bIndex < PHNCINFC_CORE_MAX_DATA_REGS; bIndex++)
        {
            /* Check if a valid entry is present in the list */
            if(NULL != pCoreCtx->tDataCtx.aDataRegList[bIndex].pNotifyCb)
            {
                /* Check if connection is matching */
                if(bConnId == pCoreCtx->tDataCtx.aDataRegList[bIndex].bConnId)
                {
                    pCoreCtx->tDataCtx.aDataRegList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                    pCoreCtx->tDataCtx.aDataRegList[bIndex].bConnId = 0;
                    pCoreCtx->tDataCtx.aDataRegList[bIndex].pContext = NULL;
                    pCoreCtx->tDataCtx.aDataRegList[bIndex].pNotifyCb = NULL;
                    PH_LOG_NCI_INFO_U32MSG("Deregistered data call back fun registered on logical connection",bConnId);
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return ;
}

NFCSTATUS
phNciNfc_CoreRecvManager(void *pCtx,
                         NFCSTATUS wStatus,
                         pphNciNfc_sCoreHeaderInfo_t pHdrInfo,
                         phNciNfc_NciCoreMsgType_t eMsgType)
{
    NFCSTATUS wStat = NFCSTATUS_INVALID_PARAMETER;
    phNciNfc_TransactInfo_t tTransInfo;
    void *pRegList = NULL;
    phNciNfc_CoreRegInfo_t tRegInfo;
    uint16_t wDataLen = 0;
    uint16_t wNumOfNode = 0;
    pphNciNfc_CoreContext_t pCoreCtx = (pphNciNfc_CoreContext_t) pCtx;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pCoreCtx) && (NULL != pHdrInfo))
    {
        wStat = NFCSTATUS_SUCCESS;
        if((phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType) ||
           (phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType))
        {
            tRegInfo.bGid = (uint8_t)pHdrInfo->Group_ID;
            tRegInfo.bOid = (uint8_t)pHdrInfo->Opcode_ID.Val;

            if((phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType))
            {
                PH_LOG_NCI_INFO_STR("Invoke response call back func if registered");
                pRegList = (void *)pCoreCtx->tRspCtx.aRspRegList;
            }
            else
            {
                PH_LOG_NCI_INFO_STR("Invoke notification call back func if registered");
                pRegList =  (void *)pCoreCtx->tNtfCtx.aNtfRegList;
            }
        }
        else if(phNciNfc_e_NciCoreMsgTypeData == eMsgType)
        {
            PH_LOG_NCI_INFO_STR("Invoke data call back func if registered");
            pRegList =  (void *)pCoreCtx->tDataCtx.aDataRegList;
            tRegInfo.bConnId = pHdrInfo->bConn_ID;
        }
        else
        {
            wStat = NFCSTATUS_INVALID_PARAMETER;
            PH_LOG_NCI_CRIT_STR("Invalid message type!");
        }

        if(NFCSTATUS_SUCCESS == wStat)
        {
            /* NOTE: For time being the following operations are being done at this location
            1. Allocation of buffer for storing the extracted data from linked list
            2. Extraction of received payload from linked list
            3. Copy of extracted data to the allocated buffer
            #These operation shall be performed at the Nci module level call back in later point of time
            */
            if(NFCSTATUS_SUCCESS != wStatus)
            {
                /* Response time out has happened */
                tTransInfo.pbuffer = NULL;
                tTransInfo.wLength = 0;
                wStat = NFCSTATUS_SUCCESS;
            }
            else
            {
                tTransInfo.pbuffer = NULL;
                tTransInfo.wLength = 0;
                /* Get size of payload present in linked list */
                wStat = phNciNfc_CoreGetDataLength(pCoreCtx,&wDataLen, &wNumOfNode);
                if(NFCSTATUS_SUCCESS == wStat)
                {
                    PH_LOG_NCI_INFO_U32MSG("Message size received: ",wDataLen);
                    if(wNumOfNode == 1)
                    {
                        /*May need seperate function for getting the pointer value*/
                      tTransInfo.pbuffer = &(pCoreCtx->tReceiveInfo.ListHead.tMem.aBuffer[PHNCINFC_CORE_PKT_HEADER_LEN]);
                      tTransInfo.wLength = wDataLen;
                    }else
                    {
                        /* Allocate memory for storing the payload */
                        tTransInfo.pbuffer = phOsalNfc_GetMemory(wDataLen);
                    }
                    if(NULL != tTransInfo.pbuffer)
                    {
                        tTransInfo.wLength = wDataLen;
                        /* Extract the data from linked list and copy it to the allocated buffer */
                        wStat = phNciNfc_CoreGetData(pCoreCtx,tTransInfo.pbuffer,wDataLen);
                    }
                }
            }

            if(NFCSTATUS_SUCCESS == wStat)
            {
                phNciNfc_PrintPacketDescription(pHdrInfo, tTransInfo.pbuffer, tTransInfo.wLength, pCoreCtx->bLogDataMessages);
                wStat = phNciNfc_CoreInvokeCb(pRegList,wStatus,&tRegInfo,eMsgType,&tTransInfo);
            }
            /* Free memory that had been allocated */
            if((NULL != tTransInfo.pbuffer) && (wNumOfNode > 1))
            {
                phOsalNfc_FreeMemory(tTransInfo.pbuffer);
                tTransInfo.pbuffer = NULL;
                tTransInfo.wLength = 0;
            }
            /* Delete linked list */
            phNciNfc_CoreDeleteList(pCoreCtx);
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

static NFCSTATUS
phNciNfc_CoreInvokeCb(pphNciNfc_CoreRegInfo_t pRegList,
                      NFCSTATUS wStatus,
                      pphNciNfc_CoreRegInfo_t pRegInfo,
                      phNciNfc_NciCoreMsgType_t eMsgType,
                      phNciNfc_TransactInfo_t *pTransInfo)
{
    uint8_t bIndex = 0;
    uint8_t bCount = 0;
    uint8_t bRegExist = 0;
    uint8_t bGetEmptySlot = 0;
    NFCSTATUS wStat = NFCSTATUS_FAILED;
    pphNciNfc_CoreRegRspNtfInfo_t pRegRspNtfList = NULL;
    pphNciNfc_CoreRegDataInfo_t pRegDataList = NULL;
    pphNciNfc_CoreIfNtf_t IfNtfCb = NULL;
    void *pIfNtfCtx = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if((phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType) ||
       (phNciNfc_e_NciCoreMsgTypeData == eMsgType))
    {
        /* Only one entry shall exist per GID and OID or ConnID */
        wStat = phNciNfc_CoreGetIndex(bGetEmptySlot,pRegList,pRegInfo,eMsgType,&bIndex);
        if(NFCSTATUS_SUCCESS == wStat)
        {
            if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)
            {
                pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t) pRegList;
                if(NULL != pRegRspNtfList[bIndex].pNotifyCb)
                {
                    IfNtfCb = pRegRspNtfList[bIndex].pNotifyCb;
                    pIfNtfCtx = pRegRspNtfList[bIndex].pContext;
                    bRegExist = 1;
                    PH_LOG_NCI_INFO_STR("Registered Rsp call back function invoked");
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Registered Rsp call back function invoke failed-Invalid fnc pointer!");
                }
                /* Check if auto de-registration is enabled */
                if((0 != bRegExist) && (pRegRspNtfList[bIndex].bEnabled == PHNCINFC_ENABLE_AUTO_DEREG))
                {
                    pRegRspNtfList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                    pRegRspNtfList[bIndex].bGid = phNciNfc_e_CoreInvalidGid;
                    pRegRspNtfList[bIndex].bOid = phNciNfc_e_NciCoreInvalidOid;
                    pRegRspNtfList[bIndex].pContext = NULL;
                    pRegRspNtfList[bIndex].pNotifyCb = NULL;
                    PH_LOG_NCI_INFO_STR("Response call back de-registered as auto de-register is enabled");
                }
            }
            else
            {
                pRegDataList = (pphNciNfc_CoreRegDataInfo_t) pRegList;
                if(NULL != pRegDataList[bIndex].pNotifyCb)
                {
                    IfNtfCb = pRegDataList[bIndex].pNotifyCb;
                    pIfNtfCtx = pRegDataList[bIndex].pContext;
                    bRegExist = 1;
                    PH_LOG_NCI_INFO_STR("Registered Data call back function invoked");
                }
                else
                {
                    PH_LOG_NCI_CRIT_STR("Registered Data call back function invoke failed-Invalid fnc pointer!");
                }
                /* Check if auto de-registration is enabled */
                if((0 != bRegExist) && (pRegDataList[bIndex].bEnabled == PHNCINFC_ENABLE_AUTO_DEREG))
                {
                    pRegDataList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                    pRegDataList[bIndex].bConnId = 0;
                    pRegDataList[bIndex].pContext = NULL;
                    pRegDataList[bIndex].pNotifyCb = NULL;
                    PH_LOG_NCI_INFO_STR("Data call back de-registered as auto de-register is enabled");
                }
            }
            if(NULL != IfNtfCb)
            {
                IfNtfCb(pIfNtfCtx, pTransInfo, wStatus);
            }
        }
        else
        {
            PH_LOG_NCI_INFO_STR("Response/Data call back not registered");
        }
    }
    else /* phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType */
    {
        pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t) pRegList;
        /* Invoke registered call back functions as multiple registrations can exist */
        for(bCount = 0; bCount < PHNCINFC_CORE_MAX_NTF_REGS; bCount++)
        {
            wStat = phNciNfc_CoreGetIndex(bGetEmptySlot,pRegList,pRegInfo,eMsgType,&bIndex);
            if(NFCSTATUS_SUCCESS == wStat)
            {
                if(NULL != pRegRspNtfList[bIndex].pNotifyCb)
                {
                    pRegRspNtfList[bIndex].pNotifyCb(pRegRspNtfList[bIndex].pContext,pTransInfo,wStatus);
                    bRegExist = 1;
                    PH_LOG_NCI_INFO_STR("Registered Ntf call back function invoked");
                }
                else
                {
                    bRegExist = 0;
                    PH_LOG_NCI_INFO_STR("Registered Ntf call back function invok failed-Invalid fnc pointer");
                }

                /* Check if auto de-registration is enabled */
                if((0 != bRegExist) && (pRegRspNtfList[bIndex].bEnabled == PHNCINFC_ENABLE_AUTO_DEREG))
                {
                    pRegRspNtfList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                    pRegRspNtfList[bIndex].bGid = phNciNfc_e_CoreInvalidGid;
                    pRegRspNtfList[bIndex].bOid = phNciNfc_e_NciCoreInvalidOid;
                    pRegRspNtfList[bIndex].pContext = NULL;
                    pRegRspNtfList[bIndex].pNotifyCb = NULL;
                    PH_LOG_NCI_INFO_STR("Notification call back de-registered as auto de-register is enabled");
                }
                bIndex++;
                if(bIndex == PHNCINFC_CORE_MAX_NTF_REGS)
                {
                    break;
                }
            }
            else
            {
                PH_LOG_NCI_INFO_STR("End of list reached");
                break;
            }
        }
    }

    if(1 == bRegExist)
    {
        wStat = NFCSTATUS_SUCCESS;
    }
    else
    {
        wStat = NFCSTATUS_FAILED;
    }

    PH_LOG_NCI_FUNC_EXIT();
    return wStat;
}

static NFCSTATUS
phNciNfc_CoreRegister(void *pRegList,
                      pphNciNfc_CoreRegInfo_t pRegInfo,
                      phNciNfc_NciCoreMsgType_t eMsgType)
{
    uint8_t bIndex = 0;
    uint8_t bGetEmptySlot = 1;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphNciNfc_CoreRegRspNtfInfo_t pRegRspNtfList = NULL;
    pphNciNfc_CoreRegDataInfo_t pRegDataList = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)
    {
        /* Force registration (over-write resposnse/data call back entry if any exist) */
        bIndex = 0;
        wStatus = NFCSTATUS_SUCCESS;
    }
    else
    {
        /* Get a free slot in the list */
        wStatus = phNciNfc_CoreGetIndex(bGetEmptySlot,pRegList,pRegInfo,eMsgType,&bIndex);
    }
    if(NFCSTATUS_SUCCESS == wStatus)
    {
        switch(eMsgType)
        {
            case phNciNfc_e_NciCoreMsgTypeCntrlRsp:
            case phNciNfc_e_NciCoreMsgTypeCntrlNtf:
                pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t)pRegList;
                pRegRspNtfList[bIndex].bEnabled = pRegInfo->bEnabled;
                pRegRspNtfList[bIndex].bGid = pRegInfo->bGid;
                pRegRspNtfList[bIndex].bOid = pRegInfo->bOid;
                pRegRspNtfList[bIndex].pContext = pRegInfo->pContext;
                pRegRspNtfList[bIndex].pNotifyCb = pRegInfo->pNotifyCb;
                break;

            case phNciNfc_e_NciCoreMsgTypeData:
                pRegDataList = (pphNciNfc_CoreRegDataInfo_t)pRegList;
                pRegDataList[bIndex].bEnabled = pRegInfo->bEnabled;
                pRegDataList[bIndex].bConnId = pRegInfo->bConnId;
                pRegDataList[bIndex].pContext = pRegInfo->pContext;
                pRegDataList[bIndex].pNotifyCb = pRegInfo->pNotifyCb;
                break;
            default:
                break;
        }
        PH_LOG_NCI_INFO_STR("Registration success");
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("No free slots available, registraiton failed!");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phNciNfc_CoreDeRegister(void *pRegList,
                        pphNciNfc_CoreRegInfo_t pRegInfo,
                        phNciNfc_NciCoreMsgType_t eMsgType)
{
    uint8_t bIndex = 0;
    uint8_t bCount = 0;
    uint8_t bGetEmptySlot = 0;
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    pphNciNfc_CoreRegRspNtfInfo_t pRegRspNtfList = NULL;
    pphNciNfc_CoreRegDataInfo_t pRegDataList = NULL;

    PH_LOG_NCI_FUNC_ENTRY();

    if((phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType) ||
       (phNciNfc_e_NciCoreMsgTypeData == eMsgType))
    {
        /* Only one entry can exist per GID and OID or ConnID */
        wStatus = phNciNfc_CoreGetIndex(bGetEmptySlot,pRegList,pRegInfo,eMsgType,&bIndex);
        if(NFCSTATUS_SUCCESS == wStatus)
        {
            if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)
            {
                pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t) pRegList;
                pRegRspNtfList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                pRegRspNtfList[bIndex].bGid = phNciNfc_e_CoreInvalidGid;
                pRegRspNtfList[bIndex].bOid = phNciNfc_e_NciCoreInvalidOid;
                pRegRspNtfList[bIndex].pContext = NULL;
                pRegRspNtfList[bIndex].pNotifyCb = NULL;
                PH_LOG_NCI_INFO_STR("Response call back de-registration success");
            }
            else
            {
                pRegDataList = (pphNciNfc_CoreRegDataInfo_t) pRegList;
                pRegDataList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                pRegDataList[bIndex].bConnId = 0;
                pRegDataList[bIndex].pContext = NULL;
                pRegDataList[bIndex].pNotifyCb = NULL;
                PH_LOG_NCI_INFO_STR("Data call back de-registration success");
            }
        }
        else
        {
            wStatus = NFCSTATUS_NOT_REGISTERED;
            PH_LOG_NCI_INFO_STR("Response/Data call back de-registration failed");
        }
    }
    else /* phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType */
    {
        pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t) pRegList;
        /* Multiple registration can be accepted per registration, so we need to check the entire Notification
           registration list for matching GID, OID and call back function pointer also */
        for(bCount = 0; bCount < PHNCINFC_CORE_MAX_NTF_REGS; bCount++)
        {
            wStatus = phNciNfc_CoreGetIndex(bGetEmptySlot,pRegList,pRegInfo,eMsgType,&bIndex);
            if(NFCSTATUS_SUCCESS == wStatus)
            {
                /* GID and OID are matching, check whether call back function pointer also matchs */
                if(pRegInfo->pNotifyCb == pRegRspNtfList[bIndex].pNotifyCb)
                {
                    /* De-register notification */
                    pRegRspNtfList[bIndex].bEnabled = PHNCINFC_CORE_DISABLE_REG_ENTRY;
                    pRegRspNtfList[bIndex].bGid = phNciNfc_e_CoreInvalidGid;
                    pRegRspNtfList[bIndex].bOid = phNciNfc_e_NciCoreInvalidOid;
                    pRegRspNtfList[bIndex].pContext = NULL;
                    pRegRspNtfList[bIndex].pNotifyCb = NULL;
                    PH_LOG_NCI_INFO_STR("Notification call back de-registration success");
                    break;
                }
                else
                {
                    wStatus = NFCSTATUS_NOT_REGISTERED;
                    /* Call back function pointer match failed, check remaining entries of ntf lists */
                    bIndex++;
                    if(bIndex == PHNCINFC_CORE_MAX_NTF_REGS)
                    {
                        PH_LOG_NCI_INFO_STR("End of list reached1");
                        PH_LOG_NCI_INFO_STR("Notification call back de-registration failed");
                        break;
                    }
                }
            }
            else
            {
                wStatus = NFCSTATUS_NOT_REGISTERED;
                PH_LOG_NCI_INFO_STR("End of list reached2");
                PH_LOG_NCI_INFO_STR("Notification call back de-registration failed");
                break;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS phNciNfc_CoreGetIndex(uint8_t bGetEmptySlot, void *pRegList,
                                       pphNciNfc_CoreRegInfo_t pRegInfo, phNciNfc_NciCoreMsgType_t eMsgType,
                                       uint8_t *bIndex)
{
    NFCSTATUS wStatus = NFCSTATUS_FAILED;
    /* Index to search the list of Callback registered */
    uint8_t bCount;
    uint8_t bMaxEntries = 0;
    pphNciNfc_CoreRegRspNtfInfo_t pRegRspNtfList = NULL;
    pphNciNfc_CoreRegDataInfo_t pRegDataList = NULL;

    PH_LOG_NCI_FUNC_ENTRY();
    if((phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType) ||
       (phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType))
    {
        pRegRspNtfList = (pphNciNfc_CoreRegRspNtfInfo_t) pRegList;

        if(phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)
        {
            bMaxEntries = PHNCINFC_CORE_MAX_RSP_REGS;
        }
        else
        {
            bMaxEntries = PHNCINFC_CORE_MAX_NTF_REGS;
        }
    }
    else /* phNciNfc_e_NciCoreMsgTypeData == eMsgType */
    {
        pRegDataList = (pphNciNfc_CoreRegDataInfo_t) pRegList;
        bMaxEntries = PHNCINFC_CORE_MAX_DATA_REGS;
    }

    for(bCount = (*bIndex); bCount < bMaxEntries; bCount++)
    {
        /* Verify registrations and return index */
        if(0 == bGetEmptySlot)
        {
            /* Incase of Response/Notification */
            if(((phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType) ||
               (phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)) && (NULL != pRegRspNtfList))
            {
                if(pRegRspNtfList[bCount].bEnabled > PHNCINFC_CORE_DISABLE_REG_ENTRY)
                {
                    if((pRegInfo->bGid == pRegRspNtfList[bCount].bGid) &&
                       (pRegInfo->bOid == pRegRspNtfList[bCount].bOid))
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                        /* Return the index at which callback is registered */
                        *bIndex = bCount;
                        break;
                    }
                }
            }
            else /* Incase of Data */
            {
                if((NULL != pRegDataList) &&(pRegDataList[bCount].bEnabled > PHNCINFC_CORE_DISABLE_REG_ENTRY))
                {
                    if(pRegInfo->bConnId == pRegDataList[bCount].bConnId)
                    {
                        wStatus = NFCSTATUS_SUCCESS;
                        /* Return the index at which callback is registered */
                        *bIndex = bCount;
                        break;
                    }
                }
            }
        }
        /* Return index of empty slot */
        else
        {
            /* Incase of Response/Notification */
            if(((phNciNfc_e_NciCoreMsgTypeCntrlNtf == eMsgType) ||
               (phNciNfc_e_NciCoreMsgTypeCntrlRsp == eMsgType)) && (NULL != pRegRspNtfList))
            {
                /* Return an index where a new registration entry can be entered */
                if(PHNCINFC_CORE_DISABLE_REG_ENTRY == pRegRspNtfList[bCount].bEnabled)
                {
                    wStatus = NFCSTATUS_SUCCESS;
                    *bIndex = bCount;
                    break;
                }
            }
            else /* Incase of Data */
            {
                /* Return an index where a new registration entry can be entered */
                if((NULL != pRegDataList) && (PHNCINFC_CORE_DISABLE_REG_ENTRY == pRegDataList[bCount].bEnabled))
                {
                    wStatus = NFCSTATUS_SUCCESS;
                    *bIndex = bCount;
                    break;
                }
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
