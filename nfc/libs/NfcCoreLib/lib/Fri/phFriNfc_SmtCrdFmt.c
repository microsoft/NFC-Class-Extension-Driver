/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_DesfireFormat.h"

#include "phFriNfc_SmtCrdFmt.tmh"

void phFriNfc_SmtCrdFmt_HCrHandler(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt,
                                   NFCSTATUS                 Status)
{
    PH_LOG_NDEF_FUNC_ENTRY();
    NdefSmtCrdFmt->State = PH_FRINFC_SMTCRDFMT_STATE_RESET_INIT;
    NdefSmtCrdFmt->CompletionRoutine[PH_FRINFC_SMTCRDFMT_CR_FORMAT].CompletionRoutine(NdefSmtCrdFmt->CompletionRoutine->Context,
                                                                                      Status);
    PH_LOG_NDEF_FUNC_EXIT();
}

NFCSTATUS phFriNfc_NdefSmtCrd_Reset(phFriNfc_sNdefSmtCrdFmt_t       *NdefSmtCrdFmt,
                                    void                            *LowerDevice,
                                    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
                                    phHal_sDevInputParam_t          *psDevInputParam,
                                    uint8_t                         *SendRecvBuffer,
                                    uint16_t                        *SendRecvBuffLen)
{
    NFCSTATUS   result = NFCSTATUS_SUCCESS;
    uint8_t     index;
    PH_LOG_NDEF_FUNC_ENTRY();
    if (    (SendRecvBuffLen == NULL) || (NdefSmtCrdFmt == NULL) || (psRemoteDevInfo == NULL) ||
            (SendRecvBuffer == NULL) ||  (LowerDevice == NULL) ||
            (*SendRecvBuffLen == 0) ||  (psDevInputParam == NULL) ||
            (*SendRecvBuffLen < PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE) )
    {
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Initialise the state to Init */
        NdefSmtCrdFmt->State = PH_FRINFC_SMTCRDFMT_STATE_RESET_INIT;

        for(index = 0; index < PH_FRINFC_SMTCRDFMT_CR; index++)
        {
            /* Initialise the NdefMap Completion Routine to Null */
            NdefSmtCrdFmt->CompletionRoutine[index].CompletionRoutine = NULL;
            /* Initialise the NdefMap Completion Routine context to Null  */
            NdefSmtCrdFmt->CompletionRoutine[index].Context = NULL;
        }

        /* Lower Device(Always Overlapped HAL Struct initialised in application
            is registred in NdefMap Lower Device) */
        NdefSmtCrdFmt->LowerDevice = LowerDevice;

        /* Remote Device info received from Manual Device Discovery is registered here */
        NdefSmtCrdFmt->psRemoteDevInfo = psRemoteDevInfo;

        /* Trx Buffer registered */
        NdefSmtCrdFmt->SendRecvBuf = SendRecvBuffer;

        /* Trx Buffer Size */
        NdefSmtCrdFmt->SendRecvLength = SendRecvBuffLen;

        /* Register Transfer Buffer Length */
        NdefSmtCrdFmt->SendLength = 0;

        /* Initialise the Format status flag*/
        NdefSmtCrdFmt->FmtProcStatus = 0;

        /* Reset the Card Type */
        NdefSmtCrdFmt->CardType = 0;

        /* Reset MapCompletion Info*/
        NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.CompletionRoutine = NULL;
        NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.Context = NULL;

        phFriNfc_MfUL_Reset(NdefSmtCrdFmt);

        /*Reset Desfire Cap Container elements*/
        phFriNfc_Desfire_Reset(NdefSmtCrdFmt);

        /*Reset Mifare Standard Container elements*/
        NdefSmtCrdFmt->AddInfo.MfStdInfo.DevInputParam = psDevInputParam;
        phFriNfc_MfStd_Reset(NdefSmtCrdFmt);

        phFriNfc_ISO15693_FmtReset(NdefSmtCrdFmt);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return (result);

}

NFCSTATUS phFriNfc_NdefSmtCrd_SetCR(phFriNfc_sNdefSmtCrdFmt_t     *NdefSmtCrdFmt,
                                    uint8_t                       FunctionID,
                                    pphFriNfc_Cr_t                CompletionRoutine,
                                    void                          *CompletionRoutineContext)
{
    NFCSTATUS   status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if ((NdefSmtCrdFmt == NULL) || (FunctionID >= PH_FRINFC_SMTCRDFMT_CR) ||
        (CompletionRoutine == NULL) || (CompletionRoutineContext == NULL))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Register the application callback with the NdefMap Completion Routine */
        NdefSmtCrdFmt->CompletionRoutine[FunctionID].CompletionRoutine = CompletionRoutine;

        /* Register the application context with the NdefMap Completion Routine context */
        NdefSmtCrdFmt->CompletionRoutine[FunctionID].Context = CompletionRoutineContext;
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_NdefSmtCrd_ConvertToReadOnly(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{
    NFCSTATUS   result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                  NFCSTATUS_INVALID_PARAMETER);
    uint8_t     sak = 0;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NdefSmtCrdFmt != NULL)
        && (NdefSmtCrdFmt->CompletionRoutine->CompletionRoutine != NULL)
        && (NdefSmtCrdFmt->CompletionRoutine->Context != NULL))
    {
        sak = NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak;
        switch (NdefSmtCrdFmt->psRemoteDevInfo->RemDevType)
        {
            case phNfc_eMifare_PICC:
            case phNfc_eISO14443_3A_PICC:
            {
                if (0x00 == sak)
                {
                    result = phFriNfc_MfUL_ConvertToReadOnly (NdefSmtCrdFmt);
                }
                else
                {
                    /* MIFARE classic 1k/4k is not supported */
                    result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
                break;
            }
            case phHal_eISO14443_A_PICC:
            case phHal_eISO14443_4A_PICC:
            {
                result = phFriNfc_Desfire_ConvertToReadOnly (NdefSmtCrdFmt);
                break;
            }
            default :
            {
                /*  Remote device is not recognised.
                Probably not NDEF compliant */
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_INVALID_REMOTE_DEVICE);
                break;
            }
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return result;
}

NFCSTATUS phFriNfc_NdefSmtCrd_Format(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt,
                                     const uint8_t *ScrtKeyB)
{
    /* Component ID needs to be changed */
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_INVALID_PARAMETER);
    uint8_t     sak = 0;
    PH_LOG_NDEF_FUNC_ENTRY();
    UNUSED(ScrtKeyB);
    /* Check for the correct context structure */
    if((NdefSmtCrdFmt != NULL) &&
        (NdefSmtCrdFmt->CompletionRoutine->CompletionRoutine != NULL) &&
        (NdefSmtCrdFmt->CompletionRoutine->Context != NULL))
    {
        /* SAK (Select response) */
        sak = NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak;

        PH_LOG_NDEF_INFO_STR("RemDevType = %d, SAK = 0x%02X",
                             NdefSmtCrdFmt->psRemoteDevInfo->RemDevType,
                             sak);

        /* Depending on the Opmodes, call the respective card functions */
        switch (NdefSmtCrdFmt->psRemoteDevInfo->RemDevType)
        {
            case phNfc_eMifare_PICC:
            case phNfc_eISO14443_3A_PICC:
                /* Remote device is Mifare card. Check for Mifare
                   NDEF compliance */
                if(0x00 == sak)
                {
                    /* The SAK/Sel_Res says the card is of the type
                       Mifare UL */
                    NdefSmtCrdFmt->CardType = PH_FRINFC_SMTCRDFMT_MIFARE_UL_CARD;
                    if (NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength == 7 &&
                    NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Uid[0] == 0x04)
                    {
                        Result = phFriNfc_MfUL_Format( NdefSmtCrdFmt);
                    }
                    else
                    {
                        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                    }
                }
                else if((0x08 == (sak & 0x18)) ||
                        (0x18 == (sak & 0x18)) ||
                        (0x01 == sak))
                {
                    NdefSmtCrdFmt->CardType = (uint8_t)
                        (((sak & 0x18) == 0x08) ?
                        PH_FRINFC_SMTCRDFMT_MFSTD_1K_CRD :
                        PH_FRINFC_SMTCRDFMT_MFSTD_4K_CRD);

                    /* The SAK/Sel_Res says the card is of the type
                       Mifare standard */
                    Result = phFriNfc_MfStd_Format( NdefSmtCrdFmt, ScrtKeyB);
                }
                else
                {
                    /* Invalid Mifare card, as the remote device
                       info - opmode says its a Mifare card but,
                       The SAK/Sel_Res is wrong */
                    Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
                break;

            case phNfc_eISO14443_A_PICC:
            case phNfc_eISO14443_4A_PICC:
                /* Remote device is Desfire card. Check for Desfire
                   NDEF compliancy */
                if(0x20 == (sak & 0xFF))
                {
                    NdefSmtCrdFmt->CardType = PH_FRINFC_SMTCRDFMT_ISO14443_4A_CARD;
                    /* The SAK/Sel_Res says the card is of the type
                       ISO14443_4A */

                    Result = phFriNfc_Desfire_Format(NdefSmtCrdFmt);
                }
                else
                {
                    /* Invalid Desfire card, as the remote device
                       info - opmode says its a desfire card but,
                       The SAK/Sel_Res is wrong */
                    Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
                break;

            case phNfc_eJewel_PICC:
                if (NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0 == PH_FRINFC_TOPAZ_HEADROM0_VAL)
                {
                    Result = phFriNfc_TopazFormat_Format(NdefSmtCrdFmt);
                }
                else if (NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0 == PH_FRINFC_TOPAZ_DYNAMIC_HEADROM0_VAL)
                {
                    Result = phFriNfc_TopazDynamicFormat_Format(NdefSmtCrdFmt);
                }
                else
                {
                    Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_REMOTE_DEVICE);
                    PH_LOG_NDEF_WARN_STR("Unknown header ROM 0 byte: 0x%02X",
                                         NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0);
                }
                break;

            case phNfc_eISO15693_PICC:
                Result = phFriNfc_ISO15693_Format(NdefSmtCrdFmt);
                break;

            default:
                /* Remote device is not recognised.
                   Probably not NDEF compliant */
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_INVALID_REMOTE_DEVICE);
                PH_LOG_NDEF_WARN_STR("Invalid remote device or Remote device not found");
                break;
        }
    }
    else
    {
        PH_LOG_NDEF_WARN_STR("Invalid input parameters");
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

void phFriNfc_NdefSmtCrd_Process(void         *Context,
                                 NFCSTATUS    Status)
{
    PH_LOG_NDEF_FUNC_ENTRY();
    PH_LOG_NDEF_INFO_STR("Status = %!NFCSTATUS!", Status);

    if (Context != NULL)
    {
        phFriNfc_sNdefSmtCrdFmt_t  *NdefSmtCrdFmt = (phFriNfc_sNdefSmtCrdFmt_t *)Context;

        switch (NdefSmtCrdFmt->psRemoteDevInfo->RemDevType)
        {
            case phNfc_eMifare_PICC:
            case phNfc_eISO14443_3A_PICC:
                if((NdefSmtCrdFmt->CardType == PH_FRINFC_SMTCRDFMT_MFSTD_1K_CRD) ||
                    (NdefSmtCrdFmt->CardType == PH_FRINFC_SMTCRDFMT_MFSTD_4K_CRD))
                {
                    phFriNfc_MfStd_Process(NdefSmtCrdFmt, Status);
                }
                else
                {
                    phFriNfc_MfUL_Process(NdefSmtCrdFmt, Status);
                }
                break;

            case phNfc_eISO14443_A_PICC:
            case phNfc_eISO14443_4A_PICC:
                phFriNfc_Desf_Process(NdefSmtCrdFmt, Status);
                break;

            case phNfc_eJewel_PICC:
                if (NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0 == PH_FRINFC_TOPAZ_HEADROM0_VAL)
                {
                    phFriNfc_TopazFormat_Process(NdefSmtCrdFmt, Status);
                }
                else if (NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0 == PH_FRINFC_TOPAZ_DYNAMIC_HEADROM0_VAL)
                {
                    phFriNfc_TopazDynamicFormat_Process(NdefSmtCrdFmt, Status);
                }
                else
                {
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_REMOTE_DEVICE);
                    PH_LOG_NDEF_WARN_STR("Unknown header ROM 0 byte: 0x%02X",
                                         NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0);
                }
                break;

            case phNfc_eISO15693_PICC:
                phFriNfc_ISO15693_FmtProcess(NdefSmtCrdFmt, Status);
                break;

            default:
                /* Remote device opmode not recognised.
                   Probably not NDEF compliant */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_INVALID_REMOTE_DEVICE);
                /* Set the state back to the Reset_Init state */
                NdefSmtCrdFmt->State = PH_FRINFC_SMTCRDFMT_STATE_RESET_INIT;

                /* Set the completion routine */
                NdefSmtCrdFmt->CompletionRoutine[PH_FRINFC_SMTCRDFMT_CR_INVALID_OPE].CompletionRoutine(NdefSmtCrdFmt->CompletionRoutine->Context,
                                                                                                       Status);
                break;
        }
    }
    else
    {
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                            NFCSTATUS_INVALID_PARAMETER);
    }

    PH_LOG_NDEF_FUNC_EXIT();
}
