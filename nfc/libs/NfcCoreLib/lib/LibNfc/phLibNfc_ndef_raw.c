/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phLibNfc_Pch.h"

#include "phLibNfc_ndef_raw.h"
#include <phFriNfc_NdefReg.h>
#include <phFriNfc_NdefMap.h>
#include <phFriNfc_MifareStdMap.h>

#include "phLibNfc_ndef_raw.tmh"

#define TOPAZ_NDEF_BITMASK             0x10U
#define TOPAZ_LEN_BITMASK              0x02U
#define TOPAZ_DYNAMIC_LEN              460U
#define TOPAZ_STATIC_CARD_LEN          128U
#define MIFARE_STD_BLOCK_SIZE          0x10U

phLibNfc_Ndef_Info_t NdefInfo;
phFriNfc_NdefRecord_t *pNdefRecord=NULL;

static void phLibNfc_CheckNdefTimerCb(uint32_t timer_id, void *pContext);
static void phLibNfc_Ndef_CheckNdef_Cb(void *pContext, NFCSTATUS status);
static void phLibNfc_Ndef_ChkNdef_Pchk_Cb(void *pContext, NFCSTATUS status);
static void phLibNfc_Ndef_Read_Cb(void* pContext,NFCSTATUS status);
static void phLibNfc_Ndef_Write_Cb(void* pContext,NFCSTATUS status);
static void phLibNfc_Ndef_Format_Cb(void *pContext,NFCSTATUS status);
static void phLibNfc_Ndef_ReadOnly_Cb (void *pContext, NFCSTATUS status);
static void phLibNfc_Ndef_SrchNdefCnt_Cb(void *pContext, NFCSTATUS status);
static void phLibNfc_Ndef_Rtd_Cb( void *CallBackParam);

static void phLibNfc_Reconnect_Mifare_Cb(void *pContext, NFCSTATUS status, void *pInfo);
static void phLibNfc_Reconnect_Mifare_Cb1(void *pContext, phLibNfc_Handle hRemoteDev, phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo, NFCSTATUS Status);

NFCSTATUS phLibNfc_Ndef_Read( phLibNfc_Handle                   hRemoteDevice,
                            phNfc_sData_t                      *psRd,
                            phLibNfc_Ndef_EOffset_t             Offset,
                            pphLibNfc_RspCb_t                   pNdefRead_RspCb,
                            void*                               pContext
                            )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    NFCSTATUS wIntStat = NFCSTATUS_FAILED;
    uint8_t     cr_index = 0;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == psRd) || (NULL == pNdefRead_RspCb)
        || (NULL == psRd->buffer)
        || (0 == psRd->length)
        || (NULL == pContext)
        || (0 == hRemoteDevice))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(0 == gpphLibNfc_Context->Connected_handle)
    {
        RetVal=NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if((TRUE == gpphLibNfc_Context->status.GenCb_pending_status)
            ||(NULL!=gpphLibNfc_Context->CBInfo.pClientRdNdefCb)
            ||(PH_LIBNFC_INTERNAL_CHK_NDEF_NOT_DONE == gpphLibNfc_Context->ndef_cntx.is_ndef))
    {
        RetVal = NFCSTATUS_REJECTED;
    }
    else if(gpphLibNfc_Context->ndef_cntx.is_ndef == 0)
    {
        RetVal = NFCSTATUS_NON_NDEF_COMPLIANT;
    }
    else if((gpphLibNfc_Context->ndef_cntx.is_ndef == 1)
        &&(0 == gpphLibNfc_Context->ndef_cntx.NdefActualSize))
    {
        /*Card is empty- So Returning length as zero*/
        psRd->length = 0;
        RetVal = NFCSTATUS_SUCCESS;
    }
    else
    {
        RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != pNciRemDevHandle))
        {
            RetVal = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == RetVal)
            {
                gpphLibNfc_Context->psRemoteDevList->psRemoteDevInfo->SessionOpened = SESSION_OPEN;
                gpphLibNfc_Context->ndef_cntx.eLast_Call = NdefRd;
                if(((((phNfc_sRemoteDevInformation_t*)hRemoteDevice)->RemDevType ==
                    phNfc_eMifare_PICC) || (((phNfc_sRemoteDevInformation_t*)hRemoteDevice)->RemDevType ==
                    phNfc_eISO14443_3A_PICC)) && (((phNfc_sRemoteDevInformation_t*)
                    hRemoteDevice)->RemoteDevInfo.Iso14443A_Info.Sak != 0)&&
                    ((NULL == gpphLibNfc_Context->psBufferedAuth)
                    ||(phNfc_eMifareAuthentA == gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd)))
                {
                    if((NULL != gpphLibNfc_Context->psBufferedAuth) &&
                        (NULL != gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer))
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer);
                    }
                    if((NULL != gpphLibNfc_Context->psBufferedAuth) &&
                        (NULL != gpphLibNfc_Context->psBufferedAuth->sSendData.buffer))
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sSendData.buffer);
                    }
                    if(NULL != gpphLibNfc_Context->psBufferedAuth)
                    {
                        phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth);
                    }
                    gpphLibNfc_Context->psBufferedAuth = (phLibNfc_sTransceiveInfo_t *)
                                        phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
                    gpphLibNfc_Context->psBufferedAuth->addr = (uint8_t)gpphLibNfc_Context->ndef_cntx.
                                                    psNdefMap->StdMifareContainer.currentBlock;
                    gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd = phNfc_eMifareRead16;
                    gpphLibNfc_Context->psBufferedAuth->sSendData.length = 0;
                    gpphLibNfc_Context->psBufferedAuth->sRecvData.length = MIFARE_STD_BLOCK_SIZE;
                    gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer = (uint8_t *)
                                                phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
                    gpphLibNfc_Context->psBufferedAuth->sSendData.buffer = (uint8_t *)
                                                phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
                }
                gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg = psRd;
                for (cr_index = 0; cr_index < PH_FRINFC_NDEFMAP_CR; cr_index++)
                {
                    RetVal= phFriNfc_NdefMap_SetCompletionRoutine(
                                        gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                        cr_index,
                                        &phLibNfc_Ndef_Read_Cb,
                                        (void *)gpphLibNfc_Context);

                }
                gpphLibNfc_Context->ndef_cntx.NdefContinueRead =(uint8_t) ((phLibNfc_Ndef_EBegin==Offset) ?
                                                        PH_FRINFC_NDEFMAP_SEEK_BEGIN :
                                                        PH_FRINFC_NDEFMAP_SEEK_CUR);
                /* call below layer Ndef Read*/
                RetVal = phFriNfc_NdefMap_RdNdef(gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg->buffer,
                                (uint32_t*)&gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg->length,
                                gpphLibNfc_Context->ndef_cntx.NdefContinueRead);

                RetVal = PHNFCSTATUS(RetVal);
                if(NFCSTATUS_INSUFFICIENT_STORAGE == RetVal)
                {
                    gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg->length = 0;
                    RetVal = NFCSTATUS_SUCCESS;
                }
                if(NFCSTATUS_PENDING == RetVal)
                {
                    gpphLibNfc_Context->CBInfo.pClientRdNdefCb = pNdefRead_RspCb;
                    gpphLibNfc_Context->CBInfo.pClientRdNdefCntx = pContext;
                    gpphLibNfc_Context->status.GenCb_pending_status=TRUE;
                }
                else if (NFCSTATUS_SUCCESS == RetVal)
                {
                    RetVal= NFCSTATUS_SUCCESS;
                }
                else
                {
                    RetVal = NFCSTATUS_FAILED;
                }
            }
            else
            {
                RetVal=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            RetVal=NFCSTATUS_INVALID_HANDLE;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return RetVal;
}

static
void phLibNfc_Ndef_Read_Cb(void* Context,NFCSTATUS status)
{
    NFCSTATUS               RetStatus = NFCSTATUS_SUCCESS,
                            RetVal;
    pphLibNfc_RspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t   *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)Context;
    void                    *pUpperLayerContext=NULL;
    phLibNfc_sRemoteDevInformation_t *ps_rem_dev_info = NULL;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)gpphLibNfc_Context->Connected_handle,
        RetVal = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,
                            &pNciRemDevHandle,
                            PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != ps_rem_dev_info))
        {
            if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
            {
                PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
                RetVal = NFCSTATUS_SHUTDOWN;
            }
            else
            {
                if (NULL != gpphLibNfc_Context->psBufferedAuth)
                {
                    gpphLibNfc_Context->psBufferedAuth->addr = (uint8_t)
                        gpphLibNfc_Context->ndef_cntx.psNdefMap->StdMifareContainer.currentBlock;
                }

                if (NFCSTATUS_FAILED == status ||
                    NFCSTATUS_RF_ERROR == status)
                {
                    /*During Ndef read operation tag was not present in RF
                    field of reader*/
                    RetStatus = status;
                    gpphLibNfc_Context->ndef_cntx.is_ndef = 0;
                    gpphLibNfc_Context->ndef_cntx.psNdefMap->bPrevReadMode = (uint8_t)PH_FRINFC_NDEFMAP_SEEK_INVALID;
                    if (((phNfc_eMifare_PICC == ps_rem_dev_info->RemDevType) ||
                        (phNfc_eISO14443_3A_PICC == ps_rem_dev_info->RemDevType)) &&
                        ((0x08 == (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak & 0x08)) ||
                        (0x01 == (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak))))
                    {
                        /* card type is mifare 1k/4k, then reconnect */
                        RetStatus = phNciNfc_Connect(gpphLibNfc_Context->sHwReference.pNciHandle,
                                    pNciRemDevHandle,
                                    pNciRemDevHandle->eRfIf,
                                    (pphNciNfc_IfNotificationCb_t)
                                    &phLibNfc_Reconnect_Mifare_Cb,
                                    (void *)gpphLibNfc_Context);
                    }
                }
                else if(status == NFCSTATUS_SUCCESS)
                {
                    RetStatus = NFCSTATUS_SUCCESS;
                    gpphLibNfc_Context->ndef_cntx.psNdefMap->bPrevReadMode =
                                gpphLibNfc_Context->ndef_cntx.psNdefMap->bCurrReadMode;
                }
                else
                {
                    gpphLibNfc_Context->ndef_cntx.psNdefMap->bPrevReadMode = (uint8_t)PH_FRINFC_NDEFMAP_SEEK_INVALID;
                    RetStatus = NFCSTATUS_FAILED;
                }
            }
            gpphLibNfc_Context->status.GenCb_pending_status = FALSE;
        }

        // Check if the 'phNciNfc_Connect' call above is pending.
        if(NFCSTATUS_PENDING != RetStatus)
        {
            pClientCb = gpphLibNfc_Context->CBInfo.pClientRdNdefCb;
            pUpperLayerContext = gpphLibNfc_Context->CBInfo.pClientRdNdefCntx;

            gpphLibNfc_Context->CBInfo.pClientRdNdefCb = NULL;
            gpphLibNfc_Context->CBInfo.pClientRdNdefCntx = NULL;

            if (NULL != pClientCb)
            {
                pClientCb(pUpperLayerContext,RetStatus);
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

NFCSTATUS phLibNfc_Ndef_Write(
                            phLibNfc_Handle          hRemoteDevice,
                            phNfc_sData_t           *psWr,
                            pphLibNfc_RspCb_t        pNdefWrite_RspCb,
                            void*                    pContext
                            )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    NFCSTATUS wIntStat = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == psWr) || (NULL == pNdefWrite_RspCb)
        || (NULL == psWr->buffer)
        || (NULL == pContext)
        || (0 ==hRemoteDevice))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(0 == gpphLibNfc_Context->Connected_handle)
    {
        RetVal=NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if((TRUE == gpphLibNfc_Context->status.GenCb_pending_status)||
           (gpphLibNfc_Context->ndef_cntx.is_ndef == PH_LIBNFC_INTERNAL_CHK_NDEF_NOT_DONE))
    {
        RetVal = NFCSTATUS_REJECTED;
    }
    else if(0 == gpphLibNfc_Context->ndef_cntx.is_ndef)
    {
        RetVal = NFCSTATUS_NON_NDEF_COMPLIANT;
    }
    else if(psWr->length > gpphLibNfc_Context->ndef_cntx.NdefLength)
    {
        RetVal = NFCSTATUS_NOT_ENOUGH_MEMORY;
    }
    else
    {
        RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != pNciRemDevHandle))
        {
            RetVal = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == RetVal)
            {
                uint8_t cr_index = 0;
                gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg = psWr;
                gpphLibNfc_Context->ndef_cntx.AppWrLength= psWr->length;
                gpphLibNfc_Context->ndef_cntx.eLast_Call = NdefWr;
                gpphLibNfc_Context->psRemoteDevList->psRemoteDevInfo->SessionOpened
                    = SESSION_OPEN;
                if(((((phNfc_sRemoteDevInformation_t*)hRemoteDevice)->RemDevType ==
                       phNfc_eMifare_PICC) || (((phNfc_sRemoteDevInformation_t*)hRemoteDevice)->RemDevType ==
                       phNfc_eISO14443_3A_PICC)) && (((phNfc_sRemoteDevInformation_t*)
                       hRemoteDevice)->RemoteDevInfo.Iso14443A_Info.Sak != 0)&&
                       ((NULL == gpphLibNfc_Context->psBufferedAuth)
                        ||(phNfc_eMifareAuthentA ==
                           gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd)))
                {
                    if((NULL != gpphLibNfc_Context->psBufferedAuth) &&
                        (NULL != gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer))
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer);
                    }
                    if((NULL != gpphLibNfc_Context->psBufferedAuth) &&
                        (NULL != gpphLibNfc_Context->psBufferedAuth->sSendData.buffer))
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sSendData.buffer);
                    }
                    if(NULL != gpphLibNfc_Context->psBufferedAuth)
                    {
                        phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth);
                    }
                    gpphLibNfc_Context->psBufferedAuth
                        =(phLibNfc_sTransceiveInfo_t *)
                        phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
                    gpphLibNfc_Context->psBufferedAuth->addr =
                     (uint8_t)gpphLibNfc_Context->ndef_cntx.psNdefMap
                     ->StdMifareContainer.currentBlock;
                    gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd = phNfc_eMifareRead16;
                    gpphLibNfc_Context->psBufferedAuth->sSendData.length
                        = 0;
                    gpphLibNfc_Context->psBufferedAuth->sRecvData.length
                        = MIFARE_STD_BLOCK_SIZE;
                    gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer
                        = (uint8_t *)phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
                     gpphLibNfc_Context->psBufferedAuth->sSendData.buffer
                        = (uint8_t *)phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
                }
                for (cr_index = 0; cr_index < PH_FRINFC_NDEFMAP_CR; cr_index++)
                {
                    /* Registering the Completion Routine.*/
                    RetVal= phFriNfc_NdefMap_SetCompletionRoutine(
                                        gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                        cr_index,
                                        &phLibNfc_Ndef_Write_Cb,
                                        (void *)gpphLibNfc_Context);

                }
                if(0 == psWr->length)
                {
                    /* Length of bytes to be written Zero- Erase the Tag  */
                    RetVal = phFriNfc_NdefMap_EraseNdef(gpphLibNfc_Context->ndef_cntx.psNdefMap);
                }
                else
                {
                    gpphLibNfc_Context->ndef_cntx.dwWrLength = psWr->length;
                    /*Write from beginning or current location*/
                    /*Call FRI Ndef Write*/
                    RetVal=phFriNfc_NdefMap_WrNdef(gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                gpphLibNfc_Context->ndef_cntx.psUpperNdefMsg->buffer,
                                (uint32_t*)&gpphLibNfc_Context->ndef_cntx.dwWrLength);
                }
                if(NFCSTATUS_PENDING == RetVal)
                {
                    gpphLibNfc_Context->CBInfo.pClientWrNdefCb = pNdefWrite_RspCb;
                    gpphLibNfc_Context->CBInfo.pClientWrNdefCntx = pContext;
                    gpphLibNfc_Context->status.GenCb_pending_status=TRUE;
                }
                else
                {
                    RetVal = NFCSTATUS_FAILED;
                }
            }
            else
            {
                RetVal=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            RetVal=NFCSTATUS_INVALID_HANDLE;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return RetVal;
}

static
void phLibNfc_Ndef_Write_Cb(void* pContext,NFCSTATUS status)
{

    pphLibNfc_RspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t   *pLibNfc_Ctxt = (phLibNfc_LibContext_t*)pContext;
    void                    *pUpperLayerContext=NULL;
    phLibNfc_sRemoteDevInformation_t   *ps_rem_dev_info = NULL;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    NFCSTATUS RetVal;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)gpphLibNfc_Context->Connected_handle,
        RetVal = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,
                            &pNciRemDevHandle,
                            PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != ps_rem_dev_info))
        {
            if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
            {
                PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
                RetVal = NFCSTATUS_SHUTDOWN;
            }
            else
            {
                if (NULL != gpphLibNfc_Context->psBufferedAuth)
                {
                    gpphLibNfc_Context->psBufferedAuth->addr = (uint8_t)
                        gpphLibNfc_Context->ndef_cntx.psNdefMap->TLVStruct.NdefTLVBlock;
                }
                if (status == NFCSTATUS_FAILED  ||
                    status == NFCSTATUS_RF_ERROR)
                {
                    /*During Ndef write operation tag was not present in RF
                    field of reader*/
                   if (((phNfc_eMifare_PICC == ps_rem_dev_info->RemDevType) ||
                       (phNfc_eISO14443_3A_PICC == ps_rem_dev_info->RemDevType)) &&
                       (0x08 == (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak & 0x08)))
                    {
                        /* card type is mifare 1k/4k, then reconnect */
                        status = phNciNfc_Connect(gpphLibNfc_Context->sHwReference.pNciHandle,
                                    pNciRemDevHandle,
                                    pNciRemDevHandle->eRfIf,
                                    (pphNciNfc_IfNotificationCb_t)
                                    &phLibNfc_Reconnect_Mifare_Cb,
                                    (void *)gpphLibNfc_Context);
                    }
                }
                else if( status== NFCSTATUS_SUCCESS)
                {
                    status = NFCSTATUS_SUCCESS;
                    if(gpphLibNfc_Context->ndef_cntx.AppWrLength >
                                     gpphLibNfc_Context->ndef_cntx.NdefLength)
                    {
                        status = NFCSTATUS_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        pLibNfc_Ctxt->ndef_cntx.psUpperNdefMsg->length = pLibNfc_Ctxt->ndef_cntx.dwWrLength;
                        pLibNfc_Ctxt->ndef_cntx.NdefActualSize = pLibNfc_Ctxt->ndef_cntx.dwWrLength;
                    }
                }
                else
                {
                    status = NFCSTATUS_FAILED;;
                }
            }
            gpphLibNfc_Context->status.GenCb_pending_status = FALSE;
        }

        // Check if the 'phNciNfc_Connect' call above is pending.
        if (NFCSTATUS_PENDING != status)
        {
            pClientCb = gpphLibNfc_Context->CBInfo.pClientWrNdefCb;
            pUpperLayerContext = gpphLibNfc_Context->CBInfo.pClientWrNdefCntx;

            gpphLibNfc_Context->CBInfo.pClientWrNdefCb = NULL;
            gpphLibNfc_Context->CBInfo.pClientWrNdefCntx = NULL;

            if (NULL != pClientCb)
            {
                pClientCb(pUpperLayerContext, status);
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_Ndef_Init(void)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NULL != gpphLibNfc_Context)
    {
        gpphLibNfc_Context->status.GenCb_pending_status = FALSE;

        if(NULL == gpphLibNfc_Context->ndef_cntx.psNdefMap)
        {
            gpphLibNfc_Context->ndef_cntx.psNdefMap = (phFriNfc_NdefMap_t *)
                        phOsalNfc_GetMemory(sizeof(phFriNfc_NdefMap_t));
        }
        if(NULL != gpphLibNfc_Context->ndef_cntx.psNdefMap)
        {
            phOsalNfc_SetMemory(gpphLibNfc_Context->ndef_cntx.psNdefMap,0,sizeof(phFriNfc_NdefMap_t));
            gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen = NDEF_SENDRCV_BUF_LEN;
            gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf =
                    (uint8_t*) phOsalNfc_GetMemory(gpphLibNfc_Context->
                    ndef_cntx.NdefSendRecvLen);

            if(NULL != gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf)
            {
                phOsalNfc_SetMemory(gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf,
                    0,
                    gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen);

                gpphLibNfc_Context->psOverHalCtxt =(phFriNfc_OvrHal_t *)
                    phOsalNfc_GetMemory(sizeof(phFriNfc_OvrHal_t));
            }
        }
        if(NULL == gpphLibNfc_Context->psOverHalCtxt)
        {
            phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,1);
        }
        else
        {
            phOsalNfc_SetMemory(gpphLibNfc_Context->psOverHalCtxt,0,
                sizeof(phFriNfc_OvrHal_t));

            gpphLibNfc_Context->psOverHalCtxt->psHwReference =
                 &gpphLibNfc_Context->sHwReference;
            if(NULL == gpphLibNfc_Context->psDevInputParam )
            {
                gpphLibNfc_Context->psDevInputParam = (phNfc_sDevInputParam_t *)
                    phOsalNfc_GetMemory(sizeof(phNfc_sDevInputParam_t));
            }
            gpphLibNfc_Context->ndef_cntx.is_ndef = PH_LIBNFC_INTERNAL_CHK_NDEF_NOT_DONE;
        }
        if(NULL == gpphLibNfc_Context->ndef_cntx.ndef_fmt)
        {
            gpphLibNfc_Context->ndef_cntx.ndef_fmt = (phFriNfc_sNdefSmtCrdFmt_t *)
                    phOsalNfc_GetMemory(sizeof(phFriNfc_sNdefSmtCrdFmt_t));
        }
        if(NULL != gpphLibNfc_Context->ndef_cntx.ndef_fmt)
        {
            phOsalNfc_SetMemory(gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                            0,
                            sizeof(phFriNfc_sNdefSmtCrdFmt_t));
        }
        else
        {
            phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,1);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

void phLibNfc_Ndef_DeInit(void)
{
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(gpphLibNfc_Context->ndef_cntx.psNdefMap !=NULL)
    {
        if(gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf !=NULL)
        {
            phOsalNfc_FreeMemory(gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf);
            gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf=NULL;
        }
        phOsalNfc_FreeMemory(gpphLibNfc_Context->ndef_cntx.psNdefMap);
        gpphLibNfc_Context->ndef_cntx.psNdefMap =NULL;
    }

    if(NULL != gpphLibNfc_Context->ndef_cntx.ndef_fmt)
    {
        phOsalNfc_FreeMemory(gpphLibNfc_Context->ndef_cntx.ndef_fmt);
        gpphLibNfc_Context->ndef_cntx.ndef_fmt = NULL;
    }

    if(gpphLibNfc_Context->psOverHalCtxt !=NULL)
    {
        phOsalNfc_FreeMemory(gpphLibNfc_Context->psOverHalCtxt);
        gpphLibNfc_Context->psOverHalCtxt =NULL;
    }
    if(gpphLibNfc_Context->psDevInputParam !=NULL)
    {
        phOsalNfc_FreeMemory(gpphLibNfc_Context->psDevInputParam);
        gpphLibNfc_Context->psDevInputParam = NULL;
    }

    if(gpphLibNfc_Context->psBufferedAuth !=NULL)
    {
        if(NULL != gpphLibNfc_Context->psBufferedAuth->sSendData.buffer)
        {
            phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth->sSendData.buffer);
            gpphLibNfc_Context->psBufferedAuth->sSendData.buffer = NULL;
        }

        if(NULL != gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer)
        {
            phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer);
            gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer = NULL;
        }
        phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth);
        gpphLibNfc_Context->psBufferedAuth = NULL;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_Ndef_CheckNdef(phLibNfc_Handle       hRemoteDevice,
                        pphLibNfc_ChkNdefRspCb_t        pCheckNdef_RspCb,
                        void*                           pContext)
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    NFCSTATUS wIntStat = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pCheckNdef_RspCb)||
        (NULL==pContext)||
        (pLibRemDevHandle == 0))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(TRUE == gpphLibNfc_Context->status.GenCb_pending_status)
    {
        RetVal = NFCSTATUS_REJECTED;
    }
    else if(0 == gpphLibNfc_Context->Connected_handle)
    {
        RetVal=NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else
    {
        RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != pNciRemDevHandle))
        {
            RetVal = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == RetVal)
            {
                uint8_t cr_index = 0;
                static uint16_t data_cnt = 0;

                gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen=300;
                gpphLibNfc_Context->ndef_cntx.eLast_Call = ChkNdef;

                /* Resets the component instance */
                RetVal = phFriNfc_NdefMap_Reset( gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                    gpphLibNfc_Context->psOverHalCtxt,
                                    (phLibNfc_sRemoteDevInformation_t *)pLibRemDevHandle,
                                    gpphLibNfc_Context->psDevInputParam,
                                    gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf,
                                    gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen,
                                    gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf,
                                    &(gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen),
                                    &(data_cnt));


                for (cr_index = 0; cr_index < PH_FRINFC_NDEFMAP_CR; cr_index++)
                {
                    /* Register the callback for the check ndef */
                    RetVal = phFriNfc_NdefMap_SetCompletionRoutine(
                                        gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                        cr_index,
                                        &phLibNfc_Ndef_CheckNdef_Cb,
                                        (void *)gpphLibNfc_Context);
                }
                /*call below layer check Ndef function*/
                RetVal = phFriNfc_NdefMap_ChkNdef(gpphLibNfc_Context->ndef_cntx.psNdefMap);
                RetVal = PHNFCSTATUS(RetVal);

                if(RetVal== NFCSTATUS_PENDING)
                {
                    RetVal = NFCSTATUS_PENDING;
                }
                else if((RetVal == NFCSTATUS_FAILED) || (RetVal ==(PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_INVALID_REMOTE_DEVICE))))
                {
                    RetVal= NFCSTATUS_FAILED;
                }
                else
                {
                    if((PH_OSALNFC_TIMER_ID_INVALID == gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id))
                    {
                        gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id =
                          phOsalNfc_Timer_Create();
                    }
                    if((PH_OSALNFC_TIMER_ID_INVALID == gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id))
                    {
                        RetVal = NFCSTATUS_FAILED;
                    }
                    else
                    {
                        (void)phOsalNfc_Timer_Start(gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id,
                                                    CHK_NDEF_TIMER_TIMEOUT,&phLibNfc_CheckNdefTimerCb,NULL);
                        RetVal = NFCSTATUS_PENDING;
                    }
                }
                if(RetVal== NFCSTATUS_PENDING)
                {
                    gpphLibNfc_Context->CBInfo.pClientCkNdefCb = pCheckNdef_RspCb;
                    gpphLibNfc_Context->CBInfo.pClientCkNdefCntx = pContext;
                    gpphLibNfc_Context->status.GenCb_pending_status=TRUE;
                }
            }
            else
            {
                RetVal=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            RetVal=NFCSTATUS_INVALID_HANDLE;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return RetVal;
}

static
void phLibNfc_Ndef_CheckNdef_Cb(void *pContext,NFCSTATUS status)
{
    phLibNfc_ChkNdef_Info_t    Ndef_Info;
    NFCSTATUS RetStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_ChkNdefRspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t           *pLibNfc_Ctxt =
                                    (phLibNfc_LibContext_t *)pContext;
    void                    *pUpperLayerContext=NULL;
    phLibNfc_sRemoteDevInformation_t   *ps_rem_dev_info = NULL;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    NFCSTATUS RetVal;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_INFO_STR("Status = %!NFCSTATUS!", status);

    Ndef_Info.ActualNdefMsgLength = 0;
    Ndef_Info.MaxNdefMsgLength = 0;
    Ndef_Info.NdefCardState = PHLIBNFC_NDEF_CARD_INVALID;

    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)gpphLibNfc_Context->Connected_handle;
        if(NULL != pNciRemDevHandle)
        {
            RetVal = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,
                                            &pNciRemDevHandle,
                                            PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        }
        else
        {
            RetVal = NFCSTATUS_FAILED;
        }

        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != ps_rem_dev_info))
        {
            if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
            {
                PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
                RetVal = NFCSTATUS_SHUTDOWN;
            }
            else
            {
                if(status == NFCSTATUS_SUCCESS)
                {
                    /*Tag is Ndef tag*/
                    gpphLibNfc_Context->ndef_cntx.is_ndef = 1;
                    (void)phFriNfc_NdefMap_GetContainerSize(
                                    pLibNfc_Ctxt->ndef_cntx.psNdefMap,
                                    &(pLibNfc_Ctxt->ndef_cntx.NdefLength),
                                    &(pLibNfc_Ctxt->ndef_cntx.NdefActualSize));
                    /*Get the data size support by particular ndef card */
                    Ndef_Info.ActualNdefMsgLength = pLibNfc_Ctxt->ndef_cntx.NdefActualSize;
                    Ndef_Info.MaxNdefMsgLength = pLibNfc_Ctxt->ndef_cntx.NdefLength;
                    RetStatus =NFCSTATUS_SUCCESS;
                }
                else if (PHNFCSTATUS(status) != NFCSTATUS_MORE_INFORMATION)
                {
                    /*Ndef check Failed.Issue a PresenceChk to ascertain if tag is
                      still in the field*/
                    RetStatus = phLibNfc_RemoteDev_CheckPresence((phLibNfc_Handle)ps_rem_dev_info,
                                                                &phLibNfc_Ndef_ChkNdef_Pchk_Cb,
                                                                (void *)gpphLibNfc_Context);
                    if(NFCSTATUS_PENDING != RetStatus)
                    {
                        gpphLibNfc_Context->ndef_cntx.is_ndef = 0;
                    }
                }
                else
                {
                    RetStatus = NFCSTATUS_FAILED;
                    gpphLibNfc_Context->ndef_cntx.is_ndef = 0;

                    if (((phNfc_eMifare_PICC == ps_rem_dev_info->RemDevType) ||
                        (phNfc_eISO14443_3A_PICC == ps_rem_dev_info->RemDevType)) &&
                        (0x08 == (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak & 0x08)))
                    {
                        /* card type is mifare 1k/4k, then reconnect */
                        RetStatus = phLibNfc_RemoteDev_Connect((phLibNfc_Handle)ps_rem_dev_info,
                                    &phLibNfc_Reconnect_Mifare_Cb1,
                                    (void *)gpphLibNfc_Context);
                    }
                    else
                    {
                       if((phNfc_eJewel_PICC == ps_rem_dev_info->RemDevType)
                           &&(TOPAZ_NDEF_BITMASK &
                           ps_rem_dev_info->RemoteDevInfo.Jewel_Info.HeaderRom0))
                        {
                            gpphLibNfc_Context->ndef_cntx.is_ndef = 1;
                            RetStatus = phFriNfc_NdefMap_GetContainerSize(
                                            pLibNfc_Ctxt->ndef_cntx.psNdefMap,
                                            &(pLibNfc_Ctxt->ndef_cntx.NdefLength),
                                            &(pLibNfc_Ctxt->ndef_cntx.NdefActualSize));
                            /*Get the data size support by particular ndef card */
                            Ndef_Info.ActualNdefMsgLength =
                                pLibNfc_Ctxt->ndef_cntx.NdefActualSize;
                            Ndef_Info.MaxNdefMsgLength
                                = pLibNfc_Ctxt->ndef_cntx.NdefLength
                                = (TOPAZ_LEN_BITMASK &
                                ps_rem_dev_info->RemoteDevInfo.Jewel_Info.HeaderRom0?
                                TOPAZ_DYNAMIC_LEN:TOPAZ_STATIC_CARD_LEN);
                            RetStatus = NFCSTATUS_SUCCESS;
                        }
                    }
                }
            }
            gpphLibNfc_Context->status.GenCb_pending_status = FALSE;
        }
        if((NFCSTATUS_PENDING != RetStatus) && (NULL != ps_rem_dev_info))
        {
            if((((ps_rem_dev_info->RemDevType == phNfc_eMifare_PICC) ||
                 (ps_rem_dev_info->RemDevType == phNfc_eISO14443_3A_PICC))
                && (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak != 0)&&
               ((NULL == gpphLibNfc_Context->psBufferedAuth)
                ||(phNfc_eMifareAuthentA == gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd)))
               )
            {
                if(NULL != gpphLibNfc_Context->psBufferedAuth)
                {
                    if(NULL != gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer)
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer);
                    }
                    if(NULL != gpphLibNfc_Context->psBufferedAuth->sSendData.buffer)
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sSendData.buffer);
                    }
                    phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth);
                }
                gpphLibNfc_Context->psBufferedAuth = (phLibNfc_sTransceiveInfo_t *) phOsalNfc_GetMemory(
                                                        sizeof(phLibNfc_sTransceiveInfo_t));
                gpphLibNfc_Context->psBufferedAuth->addr =
                                (uint8_t)gpphLibNfc_Context->ndef_cntx.psNdefMap->StdMifareContainer.currentBlock;
                gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd = phNfc_eMifareRead16;
                gpphLibNfc_Context->psBufferedAuth->sSendData.length = 0;
                gpphLibNfc_Context->psBufferedAuth->sRecvData.length = MIFARE_STD_BLOCK_SIZE;
                gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer = (uint8_t *)phOsalNfc_GetMemory(
                                        MIFARE_STD_BLOCK_SIZE);
                gpphLibNfc_Context->psBufferedAuth->sSendData.buffer = (uint8_t *)phOsalNfc_GetMemory(
                                        MIFARE_STD_BLOCK_SIZE);
            }
            pClientCb = gpphLibNfc_Context->CBInfo.pClientCkNdefCb;
            pUpperLayerContext = gpphLibNfc_Context->CBInfo.pClientCkNdefCntx;
            gpphLibNfc_Context->CBInfo.pClientCkNdefCb = NULL;
            gpphLibNfc_Context->CBInfo.pClientCkNdefCntx = NULL;
            if(NULL != pClientCb)
            {
                if (!RetStatus)
                {
                    switch (pLibNfc_Ctxt->ndef_cntx.psNdefMap->CardState)
                    {
                        case PH_NDEFMAP_CARD_STATE_INITIALIZED:
                        {
                            Ndef_Info.NdefCardState =
                                            PHLIBNFC_NDEF_CARD_INITIALISED;
                            break;
                        }

                        case PH_NDEFMAP_CARD_STATE_READ_ONLY:
                        {
                            Ndef_Info.NdefCardState =
                                            PHLIBNFC_NDEF_CARD_READ_ONLY;
                            break;
                        }

                        case PH_NDEFMAP_CARD_STATE_READ_WRITE:
                        {
                            Ndef_Info.NdefCardState =
                                            PHLIBNFC_NDEF_CARD_READ_WRITE;
                            break;
                        }

                        default:
                        {
                            Ndef_Info.NdefCardState =
                                            PHLIBNFC_NDEF_CARD_INVALID;
                            break;
                        }
                    }
                }
                pClientCb(pUpperLayerContext,Ndef_Info,RetStatus);
            }
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static void phLibNfc_Ndef_ChkNdef_Pchk_Cb(void   *pContext,
                                NFCSTATUS  status
                                )
{
    phLibNfc_ChkNdef_Info_t    Ndef_Info = {0,0,0};
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    NFCSTATUS RetStatus = NFCSTATUS_SUCCESS;
    pphLibNfc_ChkNdefRspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t           *pLibNfc_Ctxt =
                                    (phLibNfc_LibContext_t *)pContext;
    void                    *pUpperLayerContext=NULL;
    phLibNfc_sRemoteDevInformation_t   *ps_rem_dev_info = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(NFCSTATUS_SUCCESS == status)
    {
        RetStatus = NFCSTATUS_FAILED;
        gpphLibNfc_Context->ndef_cntx.is_ndef = 0;
    }
    else
    {
        RetStatus = NFCSTATUS_TARGET_LOST;
    }
    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        RetVal = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,
                        (phNciNfc_RemoteDevInformation_t **)&gpphLibNfc_Context->Connected_handle,
                        PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != ps_rem_dev_info))
        {
            if((((ps_rem_dev_info->RemDevType == phNfc_eMifare_PICC) ||
                 (ps_rem_dev_info->RemDevType == phNfc_eISO14443_3A_PICC))
                && (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak != 0)&&
                ((NULL == gpphLibNfc_Context->psBufferedAuth)
                ||(phNfc_eMifareAuthentA == gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd)))
                )
            {
                if(NULL != gpphLibNfc_Context->psBufferedAuth)
                {
                    if(NULL != gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer)
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer);
                    }
                    if(NULL != gpphLibNfc_Context->psBufferedAuth->sSendData.buffer)
                    {
                        phOsalNfc_FreeMemory(
                            gpphLibNfc_Context->psBufferedAuth->sSendData.buffer);
                    }
                    phOsalNfc_FreeMemory(gpphLibNfc_Context->psBufferedAuth);
                }
                gpphLibNfc_Context->psBufferedAuth
                    =(phLibNfc_sTransceiveInfo_t *)
                    phOsalNfc_GetMemory(sizeof(phLibNfc_sTransceiveInfo_t));
                gpphLibNfc_Context->psBufferedAuth->addr =
                        (uint8_t)gpphLibNfc_Context->ndef_cntx.psNdefMap
                        ->StdMifareContainer.currentBlock;
                gpphLibNfc_Context->psBufferedAuth->cmd.MfCmd = phNfc_eMifareRead16;
                gpphLibNfc_Context->psBufferedAuth->sSendData.length
                    = 0;
                gpphLibNfc_Context->psBufferedAuth->sRecvData.length
                            = MIFARE_STD_BLOCK_SIZE;
                gpphLibNfc_Context->psBufferedAuth->sRecvData.buffer
                            = (uint8_t *)phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
                gpphLibNfc_Context->psBufferedAuth->sSendData.buffer
                            = (uint8_t *)phOsalNfc_GetMemory(MIFARE_STD_BLOCK_SIZE);
            }
        }
        pClientCb = gpphLibNfc_Context->CBInfo.pClientCkNdefCb;
        pUpperLayerContext = gpphLibNfc_Context->CBInfo.pClientCkNdefCntx;
        gpphLibNfc_Context->CBInfo.pClientCkNdefCb = NULL;
        gpphLibNfc_Context->CBInfo.pClientCkNdefCntx = NULL;
        if(NULL != pClientCb)
        {
            Ndef_Info.NdefCardState = PHLIBNFC_NDEF_CARD_INVALID;
            pClientCb(pUpperLayerContext,Ndef_Info,RetStatus);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return;
}

static void phLibNfc_CheckNdefTimerCb(uint32_t timer_id, void *pContext)
{
    UNUSED(pContext);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    (void)phOsalNfc_Timer_Stop(timer_id);
    (void)phOsalNfc_Timer_Delete(gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id);
    gpphLibNfc_Context->ndef_cntx.Chk_Ndef_Timer_Id = 0x00;
    phLibNfc_Ndef_CheckNdef_Cb((void *)gpphLibNfc_Context,NFCSTATUS_MORE_INFORMATION);
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static void phLibNfc_Reconnect_Mifare_Cb1(void *pContext,
                                   phLibNfc_Handle hRemoteDev,
                                   phLibNfc_sRemoteDevInformation_t    *psRemoteDevInfo,
                                   NFCSTATUS                           Status)
{
    UNUSED(psRemoteDevInfo);
    (void )phLibNfc_Reconnect_Mifare_Cb(pContext,
                                         Status,
                                         (void *)hRemoteDev);

}

static void phLibNfc_Reconnect_Mifare_Cb (void *pContext,
                                          NFCSTATUS status,
                                          void *pInfo)
{
    phLibNfc_ChkNdef_Info_t     Ndef_Info = {0,0,0};
    phLibNfc_LibContext_t       *pLibNfc_Ctxt =
                                (phLibNfc_LibContext_t *)pContext;
    void                        *pUpperLayerContext = NULL;
    pphNciNfc_RemoteDevInformation_t psRemoteDevInfo = (pphNciNfc_RemoteDevInformation_t)pInfo;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    switch(gpphLibNfc_Context->ndef_cntx.eLast_Call)
    {
        case ChkNdef:
        {
            pphLibNfc_ChkNdefRspCb_t    pClientCb=NULL;
            pClientCb = pLibNfc_Ctxt->CBInfo.pClientCkNdefCb;
            pUpperLayerContext = pLibNfc_Ctxt->CBInfo.pClientCkNdefCntx;
            pLibNfc_Ctxt->CBInfo.pClientCkNdefCb = NULL;
            pLibNfc_Ctxt->CBInfo.pClientCkNdefCntx = NULL;
            if (NULL != pClientCb)
            {
                status = (NFCSTATUS_SUCCESS == status?
                        NFCSTATUS_FAILED:NFCSTATUS_TARGET_LOST);
                Ndef_Info.ActualNdefMsgLength = 0;
                Ndef_Info.MaxNdefMsgLength = 0;
                pClientCb(pUpperLayerContext,Ndef_Info,status);
            }
        }
        break;
        case NdefRd:
        {
            pphLibNfc_RspCb_t       pClientCb = pLibNfc_Ctxt->CBInfo.pClientRdNdefCb;
            pUpperLayerContext = pLibNfc_Ctxt->CBInfo.pClientRdNdefCntx;
            pLibNfc_Ctxt->CBInfo.pClientRdNdefCb = NULL;
            pLibNfc_Ctxt->CBInfo.pClientRdNdefCntx = NULL;
            if (NULL != pClientCb)
            {
                status = (NFCSTATUS_SUCCESS == status?
                        NFCSTATUS_FAILED:NFCSTATUS_TARGET_LOST);
                /* call the upper ndef read callback */
                pClientCb(pUpperLayerContext,status);
            }
        }
        break;
        case NdefWr:
        {
            pphLibNfc_RspCb_t       pClientCb =  pLibNfc_Ctxt->CBInfo.pClientWrNdefCb;
            pUpperLayerContext = pLibNfc_Ctxt->CBInfo.pClientWrNdefCntx;
            pLibNfc_Ctxt->CBInfo.pClientWrNdefCb = NULL;
            pLibNfc_Ctxt->CBInfo.pClientWrNdefCntx = NULL;
            if (NULL != pClientCb)
            {
                status = (NFCSTATUS_SUCCESS == status?
                        NFCSTATUS_FAILED:NFCSTATUS_TARGET_LOST);
                pClientCb(pUpperLayerContext,status);
            }
        }
        break;
        case NdefFmt:
        case NdefReadOnly:
        {
            pphLibNfc_RspCb_t       pClientCb =
                           pLibNfc_Ctxt->ndef_cntx.pClientNdefFmtCb;
            pUpperLayerContext= pLibNfc_Ctxt->ndef_cntx.pClientNdefFmtCntx;
            pLibNfc_Ctxt->ndef_cntx.pClientNdefFmtCb = NULL;
            pLibNfc_Ctxt->ndef_cntx.pClientNdefFmtCntx = NULL;
            if (NULL != pClientCb)
            {
                status = (NFCSTATUS_SUCCESS == status?
                        NFCSTATUS_FAILED:NFCSTATUS_TARGET_LOST);
                pClientCb(pUpperLayerContext,status);
            }
        }
        break;
        case RawTrans:
        {
            phNfc_sData_t trans_resp;
            pphLibNfc_TransceiveCallback_t pClientCb =
                           pLibNfc_Ctxt->CBInfo.pClientTransceiveCb;
            trans_resp.length = 0;
            pUpperLayerContext = pLibNfc_Ctxt->CBInfo.pClientTranseCntx;
            pLibNfc_Ctxt->CBInfo.pClientTranseCntx= NULL;
            pLibNfc_Ctxt->CBInfo.pClientTransceiveCb= NULL;
            if (NULL != pClientCb)
            {
                status = (NFCSTATUS_SUCCESS == status?
                        NFCSTATUS_FAILED:NFCSTATUS_TARGET_LOST);
                pClientCb(pUpperLayerContext,
                        (phLibNfc_Handle)psRemoteDevInfo,
                        & trans_resp,
                        status);
            }
        }
        break;
        default:
        {
        }
        break;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_RemoteDev_FormatNdef(phLibNfc_Handle         hRemoteDevice,
                                        phNfc_sData_t*          pScrtKey,
                                        pphLibNfc_RspCb_t       pNdefformat_RspCb,
                                        void*                   pContext
                                        )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    NFCSTATUS wIntStat = NFCSTATUS_FAILED;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
    uint8_t* buffer;
    static uint8_t       mif_std_key[6] ={0},
                         Index = 0;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pContext)
        || (NULL == pNdefformat_RspCb)
        ||(NULL == pScrtKey)
        ||(0 == hRemoteDevice))
    {
        RetVal = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(0 == gpphLibNfc_Context->Connected_handle)
    {
        RetVal = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if((TRUE == gpphLibNfc_Context->status.GenCb_pending_status)||
        (NULL != gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb)
        ||(gpphLibNfc_Context->ndef_cntx.is_ndef == TRUE))
    {
        /*Previous Callback is Pending*/
        RetVal = NFCSTATUS_REJECTED;
    }
    else
    {
        RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != pNciRemDevHandle))
        {
            RetVal = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == RetVal)
            {
                uint8_t   fun_id;
                gpphLibNfc_Context->ndef_cntx.eLast_Call = NdefFmt;
                gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen = NDEF_SENDRCV_BUF_LEN;

                /* Call ndef format reset, this will initialize the ndef
                format structure, and appropriate values are filled */
                RetVal = phFriNfc_NdefSmtCrd_Reset(gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                                    gpphLibNfc_Context->psOverHalCtxt,
                                    (phNfc_sRemoteDevInformation_t*)hRemoteDevice,
                                    gpphLibNfc_Context->psDevInputParam,
                                    gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf,
                                    &(gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen));
                for(fun_id = 0; fun_id < PH_FRINFC_SMTCRDFMT_CR; fun_id++)
                {
                    /* Register for all the callbacks */
                    RetVal = phFriNfc_NdefSmtCrd_SetCR(gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                                fun_id,
                                &phLibNfc_Ndef_Format_Cb,
                                gpphLibNfc_Context);
                }

                /* mif_std_key is required to format the mifare 1k/4k card */
                if (pScrtKey->length > ARRAYSIZE(mif_std_key))
                {
                    PH_LOG_LIBNFC_CRIT_STR("Buffer too small!");
                    RetVal = NFCSTATUS_BUFFER_TOO_SMALL;
                    goto Done;
                }

                buffer = pScrtKey->buffer;
                for (Index =0 ;Index < (pScrtKey->length); Index++ )
                {
                    mif_std_key[Index] = *(buffer++);
                }
                /* Start smart card formatting function   */
                RetVal = phFriNfc_NdefSmtCrd_Format(gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                                                mif_std_key);
                RetVal = PHNFCSTATUS(RetVal);
                if(RetVal== NFCSTATUS_PENDING)
                {
                    gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb = pNdefformat_RspCb;
                    gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx = pContext;
                    gpphLibNfc_Context->status.GenCb_pending_status = TRUE;
                }
                else
                {
                    RetVal = NFCSTATUS_FAILED;
                }
            }
            else
            {
                RetVal=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            RetVal=NFCSTATUS_INVALID_HANDLE;
        }
    }

Done:
    PH_LOG_LIBNFC_FUNC_EXIT();
    return RetVal;
}

NFCSTATUS
phLibNfc_ConvertToReadOnlyNdef (
    phLibNfc_Handle         hRemoteDevice,
    phNfc_sData_t*          pScrtKey,
    pphLibNfc_RspCb_t       pNdefReadOnly_RspCb,
    void*                   pContext
    )
{
    NFCSTATUS           ret_val = NFCSTATUS_FAILED,wIntStat;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *ps_rem_dev_info = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;
    uint8_t             fun_id;
    static uint8_t      mif_std_key[6] ={0},
                        Index = 0;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        ret_val = NFCSTATUS_NOT_INITIALISED;
    }
    else if ((NULL == pContext)
        || (NULL == pNdefReadOnly_RspCb)
        || (0 == hRemoteDevice))
    {
        PH_LOG_LIBNFC_CRIT_STR("Invalid input params");
        ret_val = NFCSTATUS_INVALID_PARAMETER;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        ret_val = NFCSTATUS_SHUTDOWN;
    }
    else if (0 == gpphLibNfc_Context->Connected_handle)
    {
        ret_val = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if ((TRUE == gpphLibNfc_Context->status.GenCb_pending_status)
        || (NULL != gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb)
        || (FALSE == gpphLibNfc_Context->ndef_cntx.is_ndef))
    {
        /* Previous Callback is Pending */
        ret_val = NFCSTATUS_REJECTED;
        PH_LOG_LIBNFC_CRIT_STR("LIbNfc:Previous Callback is Pending");
    }
    else if (PH_NDEFMAP_CARD_STATE_READ_WRITE !=
            gpphLibNfc_Context->ndef_cntx.psNdefMap->CardState)
    {
        /* Tag is in different state */
        ret_val = NFCSTATUS_REJECTED;
    }
    else
    {
        ret_val = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == ret_val) && (NULL != pNciRemDevHandle))
        {
            ret_val = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == ret_val)
            {
                gpphLibNfc_Context->ndef_cntx.eLast_Call = NdefReadOnly;

                switch (ps_rem_dev_info->RemDevType)
                {
                    case phNfc_eMifare_PICC:
                    case phNfc_eISO14443_3A_PICC:
                    case phNfc_eISO14443_4A_PICC:
                    case phNfc_eISO14443_A_PICC:
                    {
                        if (((phNfc_eMifare_PICC == ps_rem_dev_info->RemDevType) ||
                             (phNfc_eISO14443_3A_PICC == ps_rem_dev_info->RemDevType))
                            && (0x00 != ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak))
                        {
                            /* Mifare classic 1k/4k not supported */
                            ret_val = NFCSTATUS_REJECTED;

                            UNUSED(pScrtKey);
                        }
                        else
                        {
                            gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen = NDEF_SENDRCV_BUF_LEN;

                            /* Call ndef format reset, this will initialize the ndef
                            format structure, and appropriate values are filled */
                            ret_val = phFriNfc_NdefSmtCrd_Reset (gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                                                    gpphLibNfc_Context->psOverHalCtxt,
                                                    (phNfc_sRemoteDevInformation_t*)hRemoteDevice,
                                                    gpphLibNfc_Context->psDevInputParam,
                                                    gpphLibNfc_Context->ndef_cntx.psNdefMap->SendRecvBuf,
                                                    &(gpphLibNfc_Context->ndef_cntx.NdefSendRecvLen));

                            for(fun_id = 0; fun_id < PH_FRINFC_SMTCRDFMT_CR; fun_id++)
                            {
                                /* Register for all the callbacks */
                                ret_val = phFriNfc_NdefSmtCrd_SetCR (gpphLibNfc_Context->ndef_cntx.ndef_fmt,
                                                                    fun_id, &phLibNfc_Ndef_ReadOnly_Cb,
                                                                    gpphLibNfc_Context);
                            }

                            /* Start smart card formatting function   */
                            ret_val = phFriNfc_NdefSmtCrd_ConvertToReadOnly (
                                                            gpphLibNfc_Context->ndef_cntx.ndef_fmt);
                            ret_val = PHNFCSTATUS(ret_val);
                        }
                    }
                    break;

                    case phNfc_eJewel_PICC:
                    case phNfc_eISO15693_PICC:
                    {
                        for (fun_id = 0; fun_id < PH_FRINFC_NDEFMAP_CR; fun_id++)
                        {
                            /* Register the callback for the check ndef */
                            ret_val = phFriNfc_NdefMap_SetCompletionRoutine (
                                                gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                                fun_id, &phLibNfc_Ndef_ReadOnly_Cb,
                                                (void *)gpphLibNfc_Context);
                        }

                        /* call below layer check Ndef function */
                        ret_val = phFriNfc_NdefMap_ConvertToReadOnly (
                                                gpphLibNfc_Context->ndef_cntx.psNdefMap);
                        ret_val = PHNFCSTATUS(ret_val);
                        break;
                    }
                    default:
                    {
                        /* Tag not supported */
                        ret_val = NFCSTATUS_REJECTED;
                        break;
                    }
                }
            }
            else
            {
                /*This handle of the device sent by application is not connected */
                ret_val=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            /*This handle of the device sent by application is not connected */
            ret_val=NFCSTATUS_INVALID_HANDLE;
        }

        if (NFCSTATUS_PENDING == ret_val)
        {
            gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb = pNdefReadOnly_RspCb;
            gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx = pContext;

            gpphLibNfc_Context->status.GenCb_pending_status = TRUE;
        }
        else
        {
            ret_val = NFCSTATUS_FAILED;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return ret_val;
}

static
void phLibNfc_Ndef_ReadOnly_Cb (void *p_context,
    NFCSTATUS   status)
{
    NFCSTATUS                       ret_status = NFCSTATUS_SUCCESS;
    pphLibNfc_RspCb_t               p_client_cb = NULL;
    phLibNfc_LibContext_t           *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)p_context;
    void                            *p_upper_layer_ctxt = NULL;
    PH_LOG_LIBNFC_FUNC_ENTRY();
    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
        {
            /*shutdown is pending so issue shutdown*/
            ret_status = NFCSTATUS_SHUTDOWN;
            PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        }
        else
        {
            gpphLibNfc_Context->status.GenCb_pending_status = FALSE;
            if(NFCSTATUS_SUCCESS == status)
            {
                gpphLibNfc_Context->ndef_cntx.psNdefMap->CardState =
                                                PH_NDEFMAP_CARD_STATE_READ_ONLY;
                ret_status = NFCSTATUS_SUCCESS;
            }
            else
            {
                ret_status = NFCSTATUS_FAILED;
            }
        }
        p_client_cb = gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb;
        p_upper_layer_ctxt = gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx;
        gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb = NULL;
        gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx = NULL;
        if (NULL != p_client_cb)
        {
            p_client_cb (p_upper_layer_ctxt, ret_status);
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

NFCSTATUS phLibNfc_Ndef_SearchNdefContent(
                                phLibNfc_Handle                 hRemoteDevice,
                                phLibNfc_Ndef_SrchType_t*       psSrchTypeList,
                                uint8_t                         uNoSrchRecords,
                                pphLibNfc_Ndef_Search_RspCb_t   pNdefNtfRspCb,
                                void *                          pContext
                                )
{
    NFCSTATUS  RetVal =NFCSTATUS_SUCCESS;
    NFCSTATUS wIntStat = NFCSTATUS_FAILED;
    uint32_t Index=0;
    uint8_t     cr_index = 0;
    pphNciNfc_RemoteDevInformation_t pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = (phLibNfc_sRemoteDevInformation_t *)hRemoteDevice;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    wIntStat = phLibNfc_IsInitialised(gpphLibNfc_Context);
    if(NFCSTATUS_SUCCESS != wIntStat)
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack not Initialised");
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
    {
        PH_LOG_LIBNFC_CRIT_STR("LibNfc Stack Shut Down in progress");
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if((NULL == pNdefNtfRspCb) ||
        (NULL == pContext ) ||
        (0 == hRemoteDevice))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if( (NULL != psSrchTypeList) && (0==uNoSrchRecords))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(0 == gpphLibNfc_Context->Connected_handle)
    {
        RetVal = NFCSTATUS_TARGET_NOT_CONNECTED;
    }
    else if((TRUE == gpphLibNfc_Context->status.GenCb_pending_status)
            ||(NULL!=gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb))
    {
        RetVal = NFCSTATUS_REJECTED;
    }
     else
     {
        RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,PH_LIBNFC_INTERNAL_LIBTONCI_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != pNciRemDevHandle))
        {
            RetVal = phLibNfc_ValidateDevHandle((phLibNfc_Handle)pNciRemDevHandle);
            if(NFCSTATUS_SUCCESS == RetVal)
            {
                gpphLibNfc_Context->ndef_cntx.pNdef_NtfSrch_Type = psSrchTypeList;
                if(NULL != psSrchTypeList)
                {
                    /*Maximum records supported*/
                    gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRecords = 255;
                    /*Reset the FRI component to add the Reg type*/
                    RetVal = phFriNfc_NdefReg_Reset(
                                    &(gpphLibNfc_Context->phLib_NdefRecCntx.NdefReg),
                                    gpphLibNfc_Context->phLib_NdefRecCntx.NdefTypes_array,
                                    &(gpphLibNfc_Context->phLib_NdefRecCntx.RecordsExtracted),
                                    &(gpphLibNfc_Context->phLib_NdefRecCntx.CbParam),
                                    gpphLibNfc_Context->phLib_NdefRecCntx.ChunkedRecordsarray,
                                    gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRecords);

                    gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb = phOsalNfc_GetMemory(sizeof(phFriNfc_NdefReg_Cb_t));
                    if(gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb==NULL)
                    {
                        phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,1);
                    }
                    else
                    {
                        gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb->NdefCallback = &phLibNfc_Ndef_Rtd_Cb;
                        /*Copy the TNF types to search in global structure*/
                        gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb->NumberOfRTDs = uNoSrchRecords;
                        for(Index=0;Index<uNoSrchRecords;Index++)
                        {
                            gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb->NdefType[Index] = psSrchTypeList->Type;
                            gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb->Tnf[Index] = psSrchTypeList->Tnf;
                            gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb->NdeftypeLength[Index] = psSrchTypeList->TypeLength;
                            psSrchTypeList++;
                        }
                        /* Add the TNF type to FRI component*/
                        RetVal = phFriNfc_NdefReg_AddCb(&(gpphLibNfc_Context->phLib_NdefRecCntx.NdefReg),
                                                            gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb );
                    }
                }
                gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer =
                    phOsalNfc_GetMemory(gpphLibNfc_Context->ndef_cntx.NdefActualSize);
                gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.length =
                    gpphLibNfc_Context->ndef_cntx.NdefActualSize;
                /*Set Complete routine for NDEF Read*/
                for (cr_index = 0; cr_index < PH_FRINFC_NDEFMAP_CR; cr_index++)
                {
                    RetVal= phFriNfc_NdefMap_SetCompletionRoutine(
                                        gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                        cr_index,
                                        &phLibNfc_Ndef_SrchNdefCnt_Cb,
                                        (void *)gpphLibNfc_Context);
                }
                gpphLibNfc_Context->ndef_cntx.NdefContinueRead = PH_FRINFC_NDEFMAP_SEEK_BEGIN;
                /* call below layer Ndef Read*/
                RetVal = phFriNfc_NdefMap_RdNdef(gpphLibNfc_Context->ndef_cntx.psNdefMap,
                                gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer,
                                (uint32_t*)&gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.length,
                                PH_FRINFC_NDEFMAP_SEEK_BEGIN);
                if(NFCSTATUS_PENDING == RetVal)
                {
                    gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb = pNdefNtfRspCb;
                    gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx = pContext;
                    gpphLibNfc_Context->status.GenCb_pending_status=TRUE;
                }
                else if (NFCSTATUS_SUCCESS == RetVal)
                {
                    RetVal= NFCSTATUS_SUCCESS;
                }
                else
                {
                    RetVal = NFCSTATUS_FAILED;
                }
            }
            else
            {
                RetVal=NFCSTATUS_INVALID_HANDLE;
            }
        }
        else
        {
            RetVal=NFCSTATUS_INVALID_HANDLE;
        }
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
    return RetVal;
}

static
void phLibNfc_Ndef_Format_Cb(void *Context,NFCSTATUS  status)
{
    NFCSTATUS RetStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS RetVal;
    pphLibNfc_RspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t   *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)Context;
    void                    *pUpperLayerContext=NULL;
    phLibNfc_sRemoteDevInformation_t   *ps_rem_dev_info = NULL;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;

    if(pLibNfc_Ctxt != gpphLibNfc_Context)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)gpphLibNfc_Context->Connected_handle;
        RetVal = phLibNfc_MapRemoteDevHandle(&ps_rem_dev_info,
                        &pNciRemDevHandle,
                        PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
        if((NFCSTATUS_SUCCESS == RetVal) && (NULL != ps_rem_dev_info))
        {
            gpphLibNfc_Context->status.GenCb_pending_status = FALSE;
            if(NFCSTATUS_SUCCESS == status)
            {
                RetStatus = NFCSTATUS_SUCCESS;
            }
            else if(PHNFCSTATUS(status)==NFCSTATUS_FAILED)
            {
                RetStatus = NFCSTATUS_FAILED;
                if (((phNfc_eMifare_PICC == ps_rem_dev_info->RemDevType) ||
                    (phNfc_eISO14443_3A_PICC == ps_rem_dev_info->RemDevType)) &&
                    (0x08 == (ps_rem_dev_info->RemoteDevInfo.Iso14443A_Info.Sak & 0x08)))
                {
                    RetStatus = phLibNfc_RemoteDev_Connect((phLibNfc_Handle)ps_rem_dev_info,
                                                         &phLibNfc_Reconnect_Mifare_Cb1,
                                                        (void* )gpphLibNfc_Context );
                }
            }
            else
            {
                /*Target was removed during transaction*/
                RetStatus = NFCSTATUS_FAILED;
            }
        }
        else
        {
            /* Unable to map Nci connected handle to libnfc connected handle */
            RetStatus = NFCSTATUS_FAILED;
        }

        pClientCb = gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb;
        pUpperLayerContext= gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx;
        if(NFCSTATUS_PENDING != RetStatus)
        {
            gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCb = NULL;
            gpphLibNfc_Context->ndef_cntx.pClientNdefFmtCntx = NULL;
            if (NULL != pClientCb)
            {
                pClientCb(pUpperLayerContext,RetStatus);
            }
        }
    }
    return;
}

static
void phLibNfc_Ndef_SrchNdefCnt_Cb(void *context, NFCSTATUS status)
{
    static NFCSTATUS RegPrSt = FALSE;
    uint8_t RegStatus = 0;
    NFCSTATUS RetVal = NFCSTATUS_SUCCESS;
    uint32_t Index = 0;
    phNciNfc_RemoteDevInformation_t *pNciRemDevHandle = NULL;
    phLibNfc_sRemoteDevInformation_t *pLibRemDevHandle = NULL;

    PH_LOG_LIBNFC_FUNC_ENTRY();
    PHNFC_UNUSED_VARIABLE(context);
    if(NULL != gpphLibNfc_Context)
    {
        if(NFCSTATUS_SUCCESS == phLibNfc_IsShutDownInProgress(gpphLibNfc_Context))
        {   /*shutdown called before completion of Ndef read allow
                  shutdown to happen */
            RetVal = NFCSTATUS_SHUTDOWN;
        }
        else if(NFCSTATUS_SUCCESS != status)
        {
            RetVal = status;
        }

        if(NULL != gpphLibNfc_Context->Connected_handle)
        {
            pNciRemDevHandle = (phNciNfc_RemoteDevInformation_t *)gpphLibNfc_Context->Connected_handle;
            RetVal = phLibNfc_MapRemoteDevHandle(&pLibRemDevHandle,&pNciRemDevHandle,
                        PH_LIBNFC_INTERNAL_NCITOLIB_MAP);
            if(NFCSTATUS_SUCCESS != RetVal)
            {
                pLibRemDevHandle = NULL;
            }
        }
        gpphLibNfc_Context->status.GenCb_pending_status = FALSE;

        /* Read is not success send failed to upperlayer Call back*/
        if( RetVal!= NFCSTATUS_SUCCESS )
        {
            if((RetVal!=NFCSTATUS_SHUTDOWN)&& (RetVal!=NFCSTATUS_ABORTED))
            {
                RetVal= NFCSTATUS_FAILED;
            }
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb(
                                gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx,
                                NULL,
                                (phLibNfc_Handle)pLibRemDevHandle,
                                RetVal);
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb = NULL;
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx = NULL;
            return;
        }

        /*Get the Number of records ( If Raw record parameter is null then API gives number of Records*/
        RetVal = phFriNfc_NdefRecord_GetRecords(
                                gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer,
                                gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.length,
                                NULL,
                                gpphLibNfc_Context->phLib_NdefRecCntx.IsChunked,
                                &(gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRawRecords));

        NdefInfo.pNdefMessage = gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer;
        NdefInfo.NdefMessageLengthActual = gpphLibNfc_Context->ndef_cntx.NdefActualSize;
        NdefInfo.NdefMessageLengthMaximum = gpphLibNfc_Context->ndef_cntx.NdefLength;
        NdefInfo.NdefRecordCount =0;

        NdefInfo.pNdefRecord = NULL;
        pNdefRecord = NULL;
        /*Allocate memory to hold the records Read*/
        NdefInfo.pNdefRecord = phOsalNfc_GetMemory
            (sizeof(phFriNfc_NdefRecord_t)* gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRawRecords );
        if(NULL==NdefInfo.pNdefRecord)
        {
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb(
                                gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx,
                                NULL,
                                (phLibNfc_Handle)pLibRemDevHandle,
                                NFCSTATUS_FAILED);
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb = NULL;
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx = NULL;
            return;
        }
        NdefInfo.pNdefRecord->Id = NULL,
        NdefInfo.pNdefRecord->Type = NULL,
        NdefInfo.pNdefRecord->PayloadData = NULL,
        pNdefRecord=NdefInfo.pNdefRecord;
        /*If phLibNfc_Ndef_SearchNdefContent Reg type is NULL return all the Records*/
        if(gpphLibNfc_Context->ndef_cntx.pNdef_NtfSrch_Type==NULL)
        {
            RetVal = phFriNfc_NdefRecord_GetRecords(
                            gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer,
                            gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.length,
                            gpphLibNfc_Context->phLib_NdefRecCntx.RawRecords,
                            gpphLibNfc_Context->phLib_NdefRecCntx.IsChunked,
                            &(gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRawRecords));

            for (Index = 0; Index < gpphLibNfc_Context->phLib_NdefRecCntx.NumberOfRawRecords; Index++)
            {
                RetVal = phFriNfc_NdefRecord_Parse(
                            pNdefRecord,
                            gpphLibNfc_Context->phLib_NdefRecCntx.RawRecords[Index]);
                pNdefRecord++;
                NdefInfo.NdefRecordCount++;
            }
        }
        else
        {
            /* Look for registerd TNF */
            RetVal = phFriNfc_NdefReg_DispatchPacket(
                        &(gpphLibNfc_Context->phLib_NdefRecCntx.NdefReg),
                        gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.buffer,
                        (uint16_t)gpphLibNfc_Context->phLib_NdefRecCntx.ndef_message.length);
            if(NFCSTATUS_SUCCESS != RetVal)
            {
                /*phFriNfc_NdefReg_DispatchPacket is failed call upper layer*/
                gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb(gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx,
                                                        NULL,(phLibNfc_Handle)pLibRemDevHandle,NFCSTATUS_FAILED);
                gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb = NULL;
                gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx = NULL;
                phOsalNfc_FreeMemory(NdefInfo.pNdefRecord);
                NdefInfo.pNdefRecord = NULL;
                pNdefRecord = NULL;
                return;
            }

            while(1 != RegStatus)
            {
                /* Process the NDEF records, If match FOUND we will get Call back*/
                RegStatus = phFriNfc_NdefReg_Process(&(gpphLibNfc_Context->phLib_NdefRecCntx.NdefReg),
                                                    &RegPrSt);
                if(RegPrSt == TRUE)
                {
                    /*  Processing Done */
                    break;
                }
                /*If match found the CbParam will be updated by lower layer, copy the record info*/
                for(Index=0;Index<gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Count;Index++)
                {
                    pNdefRecord->Tnf  = gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].Tnf;
                    pNdefRecord->TypeLength  =
                        gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].TypeLength;
                    pNdefRecord->PayloadLength  =
                        gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].PayloadLength;
                    pNdefRecord->IdLength  =
                        gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].IdLength;
                    pNdefRecord->Flags =
                        gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].Flags;

                    pNdefRecord->Id = phOsalNfc_GetMemory(pNdefRecord->IdLength);
                    pNdefRecord->Type = phOsalNfc_GetMemory(pNdefRecord->TypeLength);
                    pNdefRecord->PayloadData = phOsalNfc_GetMemory(pNdefRecord->PayloadLength);

                    if(NULL != pNdefRecord->Id)
                    {
                        phOsalNfc_MemCopy(pNdefRecord->Id,
                            gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].Id,
                            pNdefRecord->IdLength);
                    }
                    if(NULL != pNdefRecord->PayloadData)
                    {
                        phOsalNfc_MemCopy(pNdefRecord->PayloadData,
                            gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].PayloadData,
                            pNdefRecord->PayloadLength);
                    }
                    if(NULL != pNdefRecord->Type)
                    {
                        phOsalNfc_MemCopy(pNdefRecord->Type,
                            gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Records[Index].Type,
                            pNdefRecord->TypeLength);
                    }
                    pNdefRecord++;
                    NdefInfo.NdefRecordCount++;
                }
            }
        }
        /* If no record found call upper layer with failed status*/
        if(pNdefRecord == NdefInfo.pNdefRecord)
        {
            NdefInfo.NdefRecordCount =0;
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb(
                        gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx,
                        &NdefInfo,(phLibNfc_Handle)pLibRemDevHandle,
                        NFCSTATUS_SUCCESS);
        }
        else
        {
            /*Call upperlayer Call back with match records*/
            gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb(
                        gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx,
                        &NdefInfo,(phLibNfc_Handle)pLibRemDevHandle,
                        NFCSTATUS_SUCCESS);
            /*Remove entry from FRI*/
            RetVal = phFriNfc_NdefReg_RmCb(
                        &(gpphLibNfc_Context->phLib_NdefRecCntx.NdefReg),
                        gpphLibNfc_Context->phLib_NdefRecCntx.NdefCb );
            /*Free the memory*/
            if(gpphLibNfc_Context->ndef_cntx.pNdef_NtfSrch_Type!=NULL)
            {
                pNdefRecord=NdefInfo.pNdefRecord;
                for(Index=0;Index<gpphLibNfc_Context->phLib_NdefRecCntx.CbParam.Count;Index++)
                {
                    if(NULL != pNdefRecord->Id)
                    {
                        phOsalNfc_FreeMemory(pNdefRecord->Id);
                    }
                    if(NULL != pNdefRecord->PayloadData)
                    {
                        phOsalNfc_FreeMemory(pNdefRecord->PayloadData);
                    }
                    if(NULL != pNdefRecord->Type)
                    {
                        phOsalNfc_FreeMemory(pNdefRecord->Type);
                    }
                    pNdefRecord++;
                }
            }
        }
        if(NULL != NdefInfo.pNdefRecord)
        {
            phOsalNfc_FreeMemory(NdefInfo.pNdefRecord);
            NdefInfo.pNdefRecord = NULL;
            pNdefRecord = NULL;
        }

        gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCb = NULL;
        gpphLibNfc_Context->CBInfo.pClientNdefNtfRespCntx = NULL;
    }
    PH_LOG_LIBNFC_FUNC_EXIT();
}

static
void phLibNfc_Ndef_Rtd_Cb( void *CallBackParam)
{
    /*There will be single call back given to all match
      It's processed in phLibNfc_Ndef_SrchNdefCnt_Cb*/
    UNUSED(CallBackParam);
    PH_LOG_LIBNFC_FUNC_ENTRY();
    PH_LOG_LIBNFC_FUNC_EXIT();
}
