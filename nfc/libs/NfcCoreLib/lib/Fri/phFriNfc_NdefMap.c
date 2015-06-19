/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_NdefMap.tmh"

#define PH_FRINFC_MAP_MIFAREUL_SUPPORT      (1U)
#define PH_FRINFC_MAP_DESFIRE_SUPPORT       (1U)
#define PH_FRINFC_MAP_FELICA_SUPPORT        (1U)
#define PH_FRINFC_MAP_TOPAZ_SUPPORT         (1U)
#define PH_FRINFC_MAP_TOPAZ_DYN_SUPPORT     (1U)
#define PH_FRINFC_MAP_MIFARESTD_SUPPORT     (1U)
#define PH_FRINFC_MAP_ISO15693_SUPPORT      (1U)

/* Total number of NDEF Erase supported tags */
#define PH_FRINFC_MAP_ERASE_NDEF            (7U)

/* Total number of NDEF Reset functions */
#define PH_FRINFC_MAP_RESET_NDEF            (5U)

/* Total number of NDEF Capability container functions */
#define PH_FRINFC_MAP_CAPCONTAINER_NDEF     (6U)

#define PH_FRINFC_MAP_CHECK_NONE            (0U)    /* No Check is needed */
#define PH_FRINFC_MAP_CHECK_SAK             (1U)    /* Incase of Mifare/Desfire family, this check is used to
                                                       identify the type of tag */
#define PH_FRINFC_MAP_CHECK_HEADERROM       (2U)    /* Incase of Topaz/Jewel family, this check is used to
                                                       identify the type of tag */

#define PH_FRINFC_MAP_MIFUL_SAK             (0x00)  /* Sak would be '0' incase of Mifare UL or ULC */
#define PH_FRINFC_MAP_DESFIRE_SAK           (0x20)  /* Sak would be '0' incase of Mifare UL or ULC */
#define PH_FRINFC_MAP_MFC1K_SAK             (0x08)  /* Sak would be '0x08' incase of Mifare Classic 1K */
#define PH_FRINFC_MAP_MFC1K_SAK_INFINEON    (0x88)  /* Sak would be '0x88' incase of Mifare Classic 1K infineon */
#define PH_FRINFC_MAP_MFC1K_CLASSIC_SAK     (0x01)  /* Sak would be '0x1' incase of old Mifare Classic 1K*/
#define PH_FRINFC_MAP_MFC4K_SAK             (0x18)  /* Sak would be '0x18' incase of Mifare Classic 4K */
#define PH_FRINFC_MAP_MFCMINI_SAK           (0x09)  /* Sak would be '0x09' incase of Mifare Classic Mini */
/* Ndef write request flag */
#define PH_FRINFC_NDEF_WRITE_REQ            (1U)

/* Ndef read request flag */
#define PH_FRINFC_NDEF_READ_REQ             (0U)

/* Invalid Ndef card not support flag */
#define PH_FRINFC_NOT_SUPPORTED             (1U)
/* Invalid Ndef card support flag */
#define PH_FRINFC_SUPPORTED                 (0U)

/* Total number of NDEF supported tags */
#define PH_FRINFC_REMDEVICE_UNSUPPORTED     (0xFF)

/* Generic Erase function index */
#define PH_FRINFC_GENERIC_ERASE             (0x00)

/* Felica Erase function index */
#define PH_FRINFC_FELICA_ERASE              (0x01)

/**< Mask to obtain the target type from SEL_RES (Sak) byte */
#define PH_FRINFC_SAK_CONFIG_MASK           (0x64U)

/**< Mask to obtain the target type from SEL_RES (Sak) byte */
#define PH_FRINFC_SAK_CONFIG_MASKMFC        (0x18U)

#include <phFriNfc_MifareULMap.h>
#include <phFriNfc_DesfireMap.h>
#include <phFriNfc_FelicaMap.h>
#include <phFriNfc_TopazMap.h>
#include <phFriNfc_MifareStdMap.h>
#include <phFriNfc_ISO15693Map.h>
#include <phFriNfc_OvrHal.h>

/* Check Ndef function prototype */
typedef NFCSTATUS (*phFriNfc_ChkNdef)(phFriNfc_NdefMap_t *);

/* Read Ndef function prototype */
typedef NFCSTATUS (*phFriNfc_RdNdef)(phFriNfc_NdefMap_t  *,uint8_t *,uint32_t *,uint8_t );

/* Write Ndef function prototype */
typedef NFCSTATUS (*phFriNfc_WrNdef)(phFriNfc_NdefMap_t  *,uint8_t *,uint32_t *,uint8_t );

/* Erase Ndef function prototype */
typedef NFCSTATUS (*phFriNfc_NdefErase)(phFriNfc_NdefMap_t *,uint8_t *,uint32_t *);

/* Reset Ndef function prototype */
typedef NFCSTATUS (*phFriNfc_Ndef_HReset)(phFriNfc_NdefMap_t *,phNfc_sDevInputParam_t *);

/* Get container size function prototype */
typedef NFCSTATUS (*phFrinfc_Ndef_GetContainerSize)(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize);

/* Sturcture holding tag specific Ndef function pointers */
typedef struct phFriNfc_NdefFunc
{
    phNfc_eRemDevType_t eRemoteDevType;               /**<Remote device type*/
    uint8_t             bInternalInfo;                /**<Used to indicate whether to check SAK or Header */
    uint8_t             bInfo;                        /**<Holds the SAK or Header */
    uint8_t             bSupported;                   /**<1: Ndef supported; 0: Ndef not supported*/
    phFriNfc_ChkNdef    pCheckNdef;                   /**<Function pointer to tag specific Check Ndef function*/
    phFriNfc_RdNdef     pReadNdef;                    /**<Function pointer to tag specific Read Ndef function*/
    phFriNfc_WrNdef     pWriteNdef;                   /**<Function pointer to tag specific Write Ndef function*/
    phFriNfc_NdefErase  pNdefErase;                   /**<Ndef Erase function pointer*/
    phFrinfc_Ndef_GetContainerSize pNdefCapContainer; /**<Ndef Reset function pointer*/
}phFriNfc_NdefFunc_t;

/* Sturcture holds tag specific Reset Ndef function pointers */
typedef struct phFriNfc_NdefResetInfo
{
    uint8_t bSupported;               /**<1: Ndef supported; 0: Ndef not supported*/
    phFriNfc_Ndef_HReset pNdefReset;  /**<Ndef Reset function pointer*/
}phFriNfc_NdefResetInfo_t;

static const phFriNfc_NdefFunc_t phFriNfc_NdefFunc[] =
{
    /**/                      /**/                           /**/                                  /**/                             /*Check Ndef*/                     /*Read Ndef*/                     /*Write Ndef*/                    /*Erase Ndef*/             /*Container Capabilites*/
    {phNfc_eMifare_PICC,      PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MIFUL_SAK,              PH_FRINFC_MAP_MIFAREUL_SUPPORT,  &phFriNfc_MifareUL_ChkNdef,        &phFriNfc_MifareUL_RdNdef,        &phFriNfc_MifareUL_WrNdef,        phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareUL_GetContainerSize},
    {phNfc_eISO14443_3A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MIFUL_SAK,              PH_FRINFC_MAP_MIFAREUL_SUPPORT,  &phFriNfc_MifareUL_ChkNdef,        &phFriNfc_MifareUL_RdNdef,        &phFriNfc_MifareUL_WrNdef,        phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareUL_GetContainerSize},
    {phNfc_eMifare_PICC,      PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_SAK,              PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eMifare_PICC,      PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_SAK_INFINEON,     PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eMifare_PICC,      PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_CLASSIC_SAK,      PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eISO14443_3A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_CLASSIC_SAK,      PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eISO14443_3A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_SAK,              PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eISO14443_3A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC1K_SAK_INFINEON,     PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eMifare_PICC,      PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC4K_SAK,              PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},
    {phNfc_eISO14443_3A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_MFC4K_SAK,              PH_FRINFC_MAP_MIFARESTD_SUPPORT, &phFriNfc_MifareStdMap_ChkNdef,    &phFriNfc_MifareStdMap_RdNdef,    &phFriNfc_MifareStdMap_WrNdef,    phFriNfc_NdefMap_WrNdef,   &phFrinfc_MifareClassic_GetContainerSize},

    {phNfc_eISO14443_B_PICC,  PH_FRINFC_MAP_CHECK_NONE,      PH_FRINFC_MAP_CHECK_NONE,             PH_FRINFC_MAP_DESFIRE_SUPPORT,   &phFriNfc_Desfire_ChkNdef,         &phFriNfc_Desfire_RdNdef,         &phFriNfc_Desfire_WrNdef,         phFriNfc_NdefMap_WrNdef,   &phFrinfc_Desfire_GetContainerSize},
    {phNfc_eISO14443_4B_PICC, PH_FRINFC_MAP_CHECK_NONE,      PH_FRINFC_MAP_CHECK_NONE,             PH_FRINFC_MAP_DESFIRE_SUPPORT,   &phFriNfc_Desfire_ChkNdef,         &phFriNfc_Desfire_RdNdef,         &phFriNfc_Desfire_WrNdef,         phFriNfc_NdefMap_WrNdef,   &phFrinfc_Desfire_GetContainerSize},

    {phNfc_eISO14443_A_PICC,  PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_DESFIRE_SAK,            PH_FRINFC_MAP_DESFIRE_SUPPORT,   &phFriNfc_Desfire_ChkNdef,         &phFriNfc_Desfire_RdNdef,         &phFriNfc_Desfire_WrNdef,         phFriNfc_NdefMap_WrNdef,   &phFrinfc_Desfire_GetContainerSize},
    {phNfc_eISO14443_4A_PICC, PH_FRINFC_MAP_CHECK_SAK,       PH_FRINFC_MAP_DESFIRE_SAK,            PH_FRINFC_MAP_DESFIRE_SUPPORT,   &phFriNfc_Desfire_ChkNdef,         &phFriNfc_Desfire_RdNdef,         &phFriNfc_Desfire_WrNdef,         phFriNfc_NdefMap_WrNdef,   &phFrinfc_Desfire_GetContainerSize},

    {phNfc_eFelica_PICC,      PH_FRINFC_MAP_CHECK_NONE,      PH_FRINFC_MAP_CHECK_NONE,             PH_FRINFC_MAP_FELICA_SUPPORT,    &phFriNfc_Felica_ChkNdef,          &phFriNfc_Felica_RdNdef,          &phFriNfc_Felica_WrNdef,          phFriNfc_Felica_EraseNdef, &phFrinfc_Felica_GetContainerSize},

    {phNfc_eJewel_PICC,       PH_FRINFC_MAP_CHECK_HEADERROM, PH_FRINFC_TOPAZ_HEADROM0_VAL,         PH_FRINFC_MAP_TOPAZ_SUPPORT,     &phFriNfc_TopazMap_ChkNdef,        &phFriNfc_TopazMap_RdNdef,        &phFriNfc_TopazMap_WrNdef,        phFriNfc_NdefMap_WrNdef,   &phFrinfc_Topaz_GetContainerSize},
    {phNfc_eJewel_PICC,       PH_FRINFC_MAP_CHECK_HEADERROM, PH_FRINFC_TOPAZ_DYNAMIC_HEADROM0_VAL, PH_FRINFC_MAP_TOPAZ_DYN_SUPPORT, &phFriNfc_TopazDynamicMap_ChkNdef, &phFriNfc_TopazDynamicMap_RdNdef, &phFriNfc_TopazDynamicMap_WrNdef, phFriNfc_NdefMap_WrNdef,   &phFrinfc_TopazDynamic_GetContainerSize},
    {phNfc_eISO15693_PICC,    PH_FRINFC_MAP_CHECK_NONE,      PH_FRINFC_MAP_CHECK_NONE,             PH_FRINFC_MAP_ISO15693_SUPPORT,  &phFriNfc_ISO15693_ChkNdef,        &phFriNfc_ISO15693_RdNdef,        &phFriNfc_ISO15693_WrNdef,        phFriNfc_NdefMap_WrNdef,   &phFrinfc_15693_GetContainerSize}
};

/* Total number of NDEF supported tags */
uint8_t phFriNfc_NdefFuncCount = ARRAYSIZE(phFriNfc_NdefFunc);

static const phFriNfc_NdefResetInfo_t phFriNfc_NdefReset[PH_FRINFC_MAP_RESET_NDEF] =
{
    {PH_FRINFC_MAP_MIFAREUL_SUPPORT, &phFriNfc_MifareUL_H_Reset},
    {PH_FRINFC_MAP_MIFARESTD_SUPPORT,&phFriNfc_MifareStdMap_H_Reset},
    {PH_FRINFC_MAP_DESFIRE_SUPPORT,  &phFriNfc_DesfCapCont_HReset},
    {PH_FRINFC_MAP_FELICA_SUPPORT,   &phFriNfc_Felica_HReset},
    {PH_FRINFC_MAP_TOPAZ_SUPPORT,    &phFriNfc_Topaz_H_Reset}
};

/**< This function shall take Remote device type as input and returns the Ndef index */
_Post_satisfies_((return < phFriNfc_NdefFuncCount) || (return == PH_FRINFC_REMDEVICE_UNSUPPORTED))
static
uint8_t
phFriNfc_GetNdefFuncIndex(
    _In_ phFriNfc_NdefMap_t *pNdefMap
    );

/**< This function is a common function which validates NdefRd and NdefWr parameters */
static NFCSTATUS phFriNfc_ValidateParams(uint8_t             *PacketData,
                                       uint32_t            *PacketDataLength,
                                       uint8_t             Offset,
                                       phFriNfc_NdefMap_t  *pNdefMap,
                                       uint8_t bNdefReq);

NFCSTATUS phFriNfc_NdefMap_Reset(   phFriNfc_NdefMap_t              *NdefMap,
                                 void                            *LowerDevice,
                                 phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
                                 phNfc_sDevInputParam_t          *psDevInputParam,
                                 uint8_t                         *TrxBuffer,
                                 uint16_t                        TrxBufferSize,
                                 uint8_t                         *ReceiveBuffer,
                                 uint16_t                        *ReceiveLength,
                                 uint16_t                        *DataCount)
{
    NFCSTATUS   status = NFCSTATUS_SUCCESS;
    uint8_t     index;
    PH_LOG_NDEF_FUNC_ENTRY();
    if ((ReceiveLength == NULL) || (NdefMap == NULL) || (psRemoteDevInfo == NULL) ||
        (TrxBuffer == NULL) || (TrxBufferSize == 0)  || (LowerDevice == NULL) ||
        (*ReceiveLength == 0) || (ReceiveBuffer == NULL) || (DataCount == NULL) ||
        (psDevInputParam == NULL) ||
        (*ReceiveLength < PH_FRINFC_NDEFMAP_MAX_SEND_RECV_BUF_SIZE ))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Initialise the state to Init */
        NdefMap->State = PH_FRINFC_NDEFMAP_STATE_RESET_INIT;

        for(index = 0;index<PH_FRINFC_NDEFMAP_CR;index++)
        {
            /* Initialise the NdefMap Completion Routine to Null */
            NdefMap->CompletionRoutine[index].CompletionRoutine = NULL;
            /* Initialise the NdefMap Completion Routine context to Null  */
            NdefMap->CompletionRoutine[index].Context = NULL;
        }

        /* Lower Device(Always Overlapped HAL Struct initialised in application
        is registred in NdefMap Lower Device) */
        NdefMap->LowerDevice = LowerDevice;

        /* Remote Device info received from Manual Device Discovery is registered here */
        NdefMap->psRemoteDevInfo = psRemoteDevInfo;

        /* Transfer Buffer registered */
        NdefMap->ApduBuffer = TrxBuffer;

        /* Set the MaxApduBufferSize */
        NdefMap->ApduBufferSize = TrxBufferSize;

        /* Set APDU Buffer Index */
        NdefMap->ApduBuffIndex = 0;

        /* Register Transfer Buffer Length */
        NdefMap->SendLength = 0;

        /* Register Receive Buffer */
        NdefMap->SendRecvBuf = ReceiveBuffer;

        /* Register Receive Buffer Length */
        NdefMap->SendRecvLength = ReceiveLength;

        /* Register Temporary Receive Buffer Length */
        NdefMap->TempReceiveLength = *ReceiveLength;

        /* Register Data Count variable and set it to zero */
        NdefMap->DataCount = DataCount;
        *NdefMap->DataCount = 0;

        /* Reset the PageOffset */
        NdefMap->Offset = 0;

        /* Reset the NumOfBytesRead*/
        NdefMap->NumOfBytesRead = 0;

        /* Reset the NumOfBytesWritten*/
        NdefMap->NumOfBytesWritten = 0;

        /* Reset the Card Type */
        NdefMap->CardType = 0;

        /* Reset the Memory Card Size*/
        NdefMap->CardMemSize = 0;

        /* Reset the Previous Operation*/
        NdefMap->PrevOperation = 0;

        /* Reset the Previous Read mode*/
        NdefMap->bPrevReadMode = (uint8_t)PH_FRINFC_NDEFMAP_SEEK_INVALID;

        /* Reset the Current Read mode*/
        NdefMap->bCurrReadMode = (uint8_t)PH_FRINFC_NDEFMAP_SEEK_INVALID;

        /* Reset the Desfire Operation Flag*/
        NdefMap->DespOpFlag = 0;

        /* Clear Card type */
        NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;

        /* Reset MapCompletion Info*/
        NdefMap->MapCompletionInfo.CompletionRoutine = NULL;
        NdefMap->MapCompletionInfo.Context = NULL;

        /*  Reset the ReadingForWriteOperation flag. */
        NdefMap->ReadingForWriteOperation = 0;  /*  FALSE  */

        /* Reset all supported card information */
        for(index = 0; index < PH_FRINFC_MAP_RESET_NDEF; index++)
        {
            if((1 == phFriNfc_NdefReset[index].bSupported) &&
                    (NULL != phFriNfc_NdefReset[index].pNdefReset))
            {
                status = phFriNfc_NdefReset[index].pNdefReset(NdefMap,psDevInputParam);
            }
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

NFCSTATUS phFriNfc_NdefMap_SetCompletionRoutine(phFriNfc_NdefMap_t     *NdefMap,
                                                uint8_t                 FunctionID,
                                                pphFriNfc_Cr_t          CompletionRoutine,
                                                void                   *CompletionRoutineContext)
{
    NFCSTATUS   status = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();
    if ( ( NdefMap == NULL ) || (FunctionID >= PH_FRINFC_NDEFMAP_CR) ||
        ( CompletionRoutine == NULL) || (CompletionRoutineContext == NULL))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Register the application callback with the NdefMap Completion Routine */
        NdefMap->CompletionRoutine[FunctionID].CompletionRoutine = CompletionRoutine;

        /* Register the application context with the NdefMap Completion Routine context */
        NdefMap->CompletionRoutine[FunctionID].Context = CompletionRoutineContext;
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_NdefMap_RdNdef(  phFriNfc_NdefMap_t  *NdefMap,
                                  uint8_t             *PacketData,
                                  uint32_t            *PacketDataLength,
                                  uint8_t             Offset)
{
    uint8_t bNdefIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;
    NFCSTATUS   status;
    PH_LOG_NDEF_FUNC_ENTRY();

    /* check for validity of input parameters*/
    status = phFriNfc_ValidateParams(PacketData,PacketDataLength,Offset,NdefMap,PH_FRINFC_NDEF_READ_REQ);
    if((NFCSTATUS_SUCCESS == status) && (NULL != NdefMap))
        {
        /* Update current read mode */
            NdefMap->bCurrReadMode = Offset;
        /* Get Ndef Read function index */
            bNdefIndex = phFriNfc_GetNdefFuncIndex(NdefMap);
            if(PH_FRINFC_REMDEVICE_UNSUPPORTED != bNdefIndex)
            {
            /* Check whether Ndef read is suported or not */
                if((1 == phFriNfc_NdefFunc[bNdefIndex].bSupported) &&
                    (NULL != phFriNfc_NdefFunc[bNdefIndex].pReadNdef))
                {
                    status = phFriNfc_NdefFunc[bNdefIndex].pReadNdef(NdefMap,
                                            PacketData,
                                            PacketDataLength,
                                            Offset);
                }
                else
                {
                    PH_LOG_NDEF_CRIT_STR("Ndef not supported for this remote device");
                    status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
            }
            else
            {
                PH_LOG_NDEF_CRIT_STR("No Ndef Read is found for the current remote device");
                /* No Ndef Read is found for the current remote device */
                status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_REMOTE_DEVICE);
            }
        }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_NdefMap_WrNdef(  phFriNfc_NdefMap_t  *NdefMap,
                                  uint8_t             *PacketData,
                                  uint32_t            *PacketDataLength)
{
    NFCSTATUS   status;
    uint8_t bNdefIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;

    PH_LOG_NDEF_FUNC_ENTRY();
    /* check for validity of input parameters*/
    status = phFriNfc_ValidateParams(PacketData,PacketDataLength,0,NdefMap,PH_FRINFC_NDEF_WRITE_REQ);
    if((NFCSTATUS_SUCCESS == status) && (NULL != NdefMap))
        {
        /* Clear data count (since NdefWr always writes from the beginning) */
            NdefMap->ApduBuffIndex = 0;
            *NdefMap->DataCount = 0;

            NdefMap->WrNdefPacketLength =   PacketDataLength;
            bNdefIndex = phFriNfc_GetNdefFuncIndex(NdefMap);
            if(PH_FRINFC_REMDEVICE_UNSUPPORTED != bNdefIndex)
            {
                if((1 == phFriNfc_NdefFunc[bNdefIndex].bSupported) &&
                    (NULL != phFriNfc_NdefFunc[bNdefIndex].pWriteNdef))
                {
                    status = phFriNfc_NdefFunc[bNdefIndex].pWriteNdef(NdefMap,
                                                        PacketData,
                                                        PacketDataLength,
                                                        PH_FRINFC_NDEFMAP_SEEK_BEGIN);
                }
                else
                {
                    PH_LOG_NDEF_CRIT_STR("Ndef not supported for this remote device");
                    status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
            }
            else
            {
                status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                    NFCSTATUS_INVALID_REMOTE_DEVICE);
            }
        }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_NdefMap_ChkNdef( phFriNfc_NdefMap_t     *NdefMap)
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint8_t bNdefIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;

    PH_LOG_NDEF_FUNC_ENTRY();
    /*  Check for ndefmap context and relevant state. Else return error*/
    if ( NdefMap == NULL )
    {
        PH_LOG_NDEF_WARN_STR("Invalid input parameter");
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( (NdefMap->State !=  PH_FRINFC_NDEFMAP_STATE_RESET_INIT) ||
            (NdefMap->psRemoteDevInfo->SessionOpened != 0x01 ) )
            /*  Harsha: If SessionOpened is not 1, this means that connect has not happened */
        {
            PH_LOG_NDEF_WARN_STR("Invalid device request");
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_DEVICE_REQUEST);
        }
        else if ( (NdefMap->CompletionRoutine->CompletionRoutine == NULL) || (NdefMap->CompletionRoutine->Context == NULL ))
        {
            PH_LOG_NDEF_WARN_STR("Invalid completeion routine or context");
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
        }
        else
        {
            bNdefIndex = phFriNfc_GetNdefFuncIndex(NdefMap);
            if(PH_FRINFC_REMDEVICE_UNSUPPORTED != bNdefIndex)
            {
                if((1 == phFriNfc_NdefFunc[bNdefIndex].bSupported) &&
                    (NULL != phFriNfc_NdefFunc[bNdefIndex].pCheckNdef))
                {
                    status = phFriNfc_NdefFunc[bNdefIndex].pCheckNdef(NdefMap);
                }
                else
                {
                    PH_LOG_NDEF_CRIT_STR("Ndef not supported for this remote device");
                    status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                }
            }
            else
            {
                PH_LOG_NDEF_CRIT_STR("Invalid Remote device");
                status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                    NFCSTATUS_INVALID_REMOTE_DEVICE);
            }
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

_Post_satisfies_((return < phFriNfc_NdefFuncCount) || (return == PH_FRINFC_REMDEVICE_UNSUPPORTED))
static
uint8_t
phFriNfc_GetNdefFuncIndex(
    _In_ phFriNfc_NdefMap_t *pNdefMap
    )
{
    phNfc_eRemDevType_t eRemDevType;
    uint8_t bIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;
    uint8_t bCount = 0;
    uint8_t bSak;

    PH_LOG_NDEF_FUNC_ENTRY();
    if(NULL != pNdefMap)
    {
        eRemDevType = pNdefMap->psRemoteDevInfo->RemDevType;
        for(bCount = 0; bCount < phFriNfc_NdefFuncCount; bCount++)
        {
            /* Check if remote device matches */
            if(eRemDevType == phFriNfc_NdefFunc[bCount].eRemoteDevType)
            {
                if(PH_FRINFC_MAP_CHECK_SAK == phFriNfc_NdefFunc[bCount].bInternalInfo)
                {
                    /* Sak check is need only if remote device belongs to Type 2 tag platform (Mifare)
                       or Type 4 tag platform (Desfire) */
                    bSak = pNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak;

                    if(phNfc_eMifare_PICC == pNdefMap->psRemoteDevInfo->RemDevType ||
                       phNfc_eISO14443_3A_PICC == pNdefMap->psRemoteDevInfo->RemDevType )
                    {
                        /* Mask the Sak (SEL_RES) for the required bits */
                        if(bSak == PH_FRINFC_MAP_MFC1K_SAK ||
                           bSak == PH_FRINFC_MAP_MFC4K_SAK ||
                           bSak == PH_FRINFC_MAP_MFCMINI_SAK)
                        {
                            bSak &= (uint8_t)PH_FRINFC_SAK_CONFIG_MASKMFC;
                        }
                    }
                    else
                    {
                        /* Mask the Sak (SEL_RES) for the required bits */
                        bSak &= (uint8_t)PH_FRINFC_SAK_CONFIG_MASK;
                    }
                    /* Check 'Sak' for identifying the type of remte device */
                    if(bSak == phFriNfc_NdefFunc[bCount].bInfo)
                    {
                        bIndex = bCount;
                        break;
                    }
                }
                else if(PH_FRINFC_MAP_CHECK_HEADERROM == phFriNfc_NdefFunc[bCount].bInternalInfo)
                {
                    if(phFriNfc_NdefFunc[bCount].bInfo ==
                       pNdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.HeaderRom0)
                    {
                        bIndex = bCount;
                        break;
                    }
                }
                else if(PH_FRINFC_MAP_CHECK_NONE == phFriNfc_NdefFunc[bCount].bInternalInfo)
                {
                    bIndex = bCount;
                    break;
                }
                else
                {
                    /* Continue checking the list */
                }
            }
        }
    }
    if(PH_FRINFC_REMDEVICE_UNSUPPORTED == bIndex)
    {
        PH_LOG_NDEF_CRIT_STR("Invalid Remote device");
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return bIndex;
}

static NFCSTATUS phFriNfc_ValidateParams(uint8_t             *PacketData,
                                       uint32_t            *PacketDataLength,
                                       uint8_t             Offset,
                                       phFriNfc_NdefMap_t  *pNdefMap,
                                       uint8_t bNdefReq)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(pNdefMap != NULL)
    {
        if((PacketData        == NULL) ||
           (PacketDataLength  == NULL) ||
           (*PacketDataLength == 0) ||
           (pNdefMap->CompletionRoutine->CompletionRoutine == NULL) ||
           (pNdefMap->CompletionRoutine->Context == NULL))
        {
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
        }
        else if(pNdefMap->CardState == PH_NDEFMAP_CARD_STATE_INVALID)
        {
            /*  Card is in invalid state, cannot have any read/write operations*/
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
        }
        else if(PH_FRINFC_NDEF_READ_REQ == bNdefReq)
        {
            if((Offset != PH_FRINFC_NDEFMAP_SEEK_CUR) && (Offset != PH_FRINFC_NDEFMAP_SEEK_BEGIN))
            {
                wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
            }
            else if(pNdefMap->CardState == PH_NDEFMAP_CARD_STATE_INITIALIZED)
            {
                /*  Can't read any data from the card:TLV length is zero*/
                wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
                pNdefMap->NumOfBytesRead = PacketDataLength;
                *pNdefMap->NumOfBytesRead = 0;
            }
            else if((pNdefMap->PrevOperation == PH_FRINFC_NDEFMAP_WRITE_OPE) &&
                    (Offset != PH_FRINFC_NDEFMAP_SEEK_BEGIN ))
            {
                /* If previous operation was Ndef write and current operation is Read ndef from offset */
                wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_DEVICE_REQUEST);
            }
            else
            {
                /* If the offset is 1 (PH_FRINFC_NDEFMAP_SEEK_BEGIN), reset everything and start
                   reading from the first Page of the card.
                   else if offset is 0 (PH_FRINFC_NDEFMAP_SEEK_CUR), continue reading
                   No need to reset the parameters.  */
                if(Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN)
                {
                    pNdefMap->ApduBuffIndex = 0;
                    *pNdefMap->DataCount = 0;
                }
                else /* Offset == PH_FRINFC_NDEFMAP_SEEK_CUR */
                {
                    /* Ndef Read operation with mode PH_FRINFC_NDEFMAP_SEEK_CURR mode is
                       allowed only if pervious operation is Ndef Read with PH_FRINFC_NDEFMAP_SEEK_BEGIN mode */
                    if((pNdefMap->bPrevReadMode == PH_FRINFC_NDEFMAP_SEEK_BEGIN) ||
                        (pNdefMap->bPrevReadMode == PH_FRINFC_NDEFMAP_SEEK_CUR))
                    {
                        /* Continue */
                    }
                    else
                    {
                        /* If previous operation was not Ndef read from beginning */
                        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_DEVICE_REQUEST);
                    }
                }
            }
        }
        else if(PH_FRINFC_NDEF_WRITE_REQ == bNdefReq)
        {
            if(pNdefMap->CardState == PH_NDEFMAP_CARD_STATE_READ_ONLY)
            {
                /*Can't write to the card :No Grants */
                wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_NOT_ALLOWED);

                /* set the no. bytes written is zero*/
                pNdefMap->WrNdefPacketLength = PacketDataLength;
                *pNdefMap->WrNdefPacketLength = 0;
            }
        }
        else
        {
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phFriNfc_NdefMap_EraseNdef(phFriNfc_NdefMap_t *NdefMap)
    {
    NFCSTATUS   status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_REMOTE_DEVICE);
    uint8_t bIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;
    static uint8_t     PktData[3] = PH_FRINFC_NDEFMAP_EMPTY_NDEF_MSG;
    static uint32_t    PacketDataLength = sizeof(PktData);

    PH_LOG_NDEF_FUNC_ENTRY();
    if (NdefMap == NULL )
    {
        /*  Invalid input parameter error   */
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Get index to the Erase function that needs to be used */
        bIndex = phFriNfc_GetNdefFuncIndex(NdefMap);
        if(PH_FRINFC_REMDEVICE_UNSUPPORTED != bIndex)
        {
            if((1 == phFriNfc_NdefFunc[bIndex].bSupported) &&
                (NULL != phFriNfc_NdefFunc[bIndex].pNdefErase))
            {
                status =  phFriNfc_NdefFunc[bIndex].pNdefErase(NdefMap,PktData,&PacketDataLength);
            }
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS phFriNfc_NdefMap_GetContainerSize(phFriNfc_NdefMap_t *NdefMap,uint32_t *maxSize, uint32_t *actualSize)
{
    NFCSTATUS   wStatus = NFCSTATUS_INVALID_REMOTE_DEVICE;
    uint8_t bIndex = PH_FRINFC_REMDEVICE_UNSUPPORTED;

    PH_LOG_NDEF_FUNC_ENTRY();
    if( (NdefMap == NULL) || (maxSize == NULL) || (actualSize == NULL))
    {
        /* Invalid input parameter error */
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Get index to the Capability container function that needs to be used */
        bIndex = phFriNfc_GetNdefFuncIndex(NdefMap);
        if(PH_FRINFC_REMDEVICE_UNSUPPORTED != bIndex)
        {
            if((1 == phFriNfc_NdefFunc[bIndex].bSupported) &&
                (NULL != phFriNfc_NdefFunc[bIndex].pNdefCapContainer))
            {
                if(NULL != phFriNfc_NdefFunc[bIndex].pNdefCapContainer)
                {
                    wStatus =  phFriNfc_NdefFunc[bIndex].pNdefCapContainer(NdefMap,maxSize,actualSize);
                }
            }
            else
            {
                wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                NFCSTATUS_INVALID_REMOTE_DEVICE);
            }
        }
        else
        {
            /* If card type is not found */
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                            NFCSTATUS_INVALID_REMOTE_DEVICE);
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phFriNfc_NdefMap_ConvertToReadOnly (
    phFriNfc_NdefMap_t          *NdefMap)
{
    NFCSTATUS   result = NFCSTATUS_PENDING;


    /*  Check for ndefmap context and relevant state. Else return error*/
    if (NULL == NdefMap)
    {
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else if ((NdefMap->CompletionRoutine->CompletionRoutine == NULL)
        || (NdefMap->CompletionRoutine->Context == NULL))
    {
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        switch (NdefMap->CardType)
        {
            case PH_FRINFC_NDEFMAP_TOPAZ_CARD:
            {
                result = phFriNfc_TopazMap_ConvertToReadOnly (NdefMap);
                break;
            }

            case PH_FRINFC_NDEFMAP_TOPAZ_DYNAMIC_CARD:
            {
                result = phFriNfc_TopazDynamicMap_ConvertToReadOnly (NdefMap);
                break;
            }
#if 0
            case PH_FRINFC_NDEFMAP_ISO15693_CARD:
            {
                result = phFriNfc_ISO15693_ConvertToReadOnly (NdefMap);
                break;
            }
#endif
            default:
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_REMOTE_DEVICE);
                break;
            }
        }
    }
    return result;
}
