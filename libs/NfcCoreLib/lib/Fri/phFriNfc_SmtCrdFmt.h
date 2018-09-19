/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcHalTypes2.h>

#define DESFIRE_FMT_EV1

#define PH_FRI_NFC_SMTCRDFMT_NFCSTATUS_FORMAT_ERROR             9
#define PH_FRINFC_SMTCRDFMT_MSTD_DEFAULT_KEYA_OR_KEYB           {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF}
#define PH_FRINFC_SMTCRDFMT_MSTD_MADSECT_KEYA                   {0xA0, 0xA1,0xA2,0xA3,0xA4,0xA5}
#define PH_FRINFC_SMTCRDFMT_NFCFORUMSECT_KEYA                   {0xD3, 0xF7,0xD3,0xF7,0xD3,0xF7}
#define PH_FRINFC_SMTCRDFMT_MSTD_MADSECT_ACCESSBITS             {0x78,0x77,0x88}
#define PH_FRINFC_SMTCRDFMT_MSTD_NFCFORUM_ACCESSBITS            {0x7F,0x07,0x88}
#define PH_FRINFC_SMTCRDFMT_MAX_TLV_TYPE_SUPPORTED              1

#define PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE              252
#define PH_FRINFC_SMTCRDFMT_STATE_RESET_INIT                    1

enum
{
    PH_FRINFC_SMTCRDFMT_MIFARE_UL_CARD,
    PH_FRINFC_SMTCRDFMT_ISO14443_4A_CARD,
    PH_FRINFC_SMTCRDFMT_MFSTD_1K_CRD,
    PH_FRINFC_SMTCRDFMT_MFSTD_4K_CRD,
    PH_FRINFC_SMTCRDFMT_TOPAZ_CARD
};


#define PH_FRINFC_SMTCRDFMT_CR_FORMAT           0
#define PH_FRINFC_SMTCRDFMT_CR_INVALID_OPE      1
#define PH_FRINFC_SMTCRDFMT_CR                  2

typedef struct phFriNfc_Type1_AddInfo
{
    uint8_t CurrentBlock;
    uint8_t CurrentByte;
} phFriNfc_Type1_AddInfo_t;

typedef struct phFriNfc_Type2_AddInfo
{
    uint8_t OTPBytes[4]; /* Stores the CC byte values. For Ex: 0xE1, 0x10, 0x10, 0x00 */
    uint8_t LockBytes[4];
    uint8_t ReadData[16];
    uint8_t ReadDataIndex;
    uint8_t DynLockBytes[4];
    uint8_t BytesLockedPerLockBit;
    uint8_t LockBytesPerPage;
    uint8_t LockByteNumber;
    uint8_t LockBlockNumber;
    uint8_t NoOfLockBits;
    uint8_t DefaultLockBytesFlag;
    uint8_t LockBitsWritten;
    uint8_t CurrentBlock; /* Current block address */
} phFriNfc_Type2_AddInfo_t;

typedef struct phFriNfc_Type4_AddInfo
{
    /* Specifies Keys related to PICC/NFCForum Master Key settings*/
    /* Stores the PICC Master Key/NFC Forum MasterKey*/
    uint8_t PICCMasterKey[16];
    uint8_t NFCForumMasterkey[16];

    /* To create the files follwoiing attributes are required*/
    uint8_t         PrevState;
    uint16_t        FileAccessRights;
    uint32_t        CardSize;
    uint16_t        MajorVersion;
    uint16_t        MinorVersion;

} phFriNfc_Type4_AddInfo_t;

typedef struct phFriNfc_MfStd_AddInfo
{
    /** Device input parameter for poll and connect after failed authentication */
    phNfc_sDevInputParam_t  *DevInputParam;

    /* Stores the Default KeyA and KeyB values*/
    uint8_t     Default_KeyA_OR_B[6];

    /* Key A of MAD sector*/
    uint8_t     MADSect_KeyA[6];

    /* Key A of NFC Forum Sector sector*/
    uint8_t     NFCForumSect_KeyA[6];

    /* Access Bits of MAD sector*/
    uint8_t     MADSect_AccessBits[3];

    /* Access Bits of NFC Forum sector*/
    uint8_t     NFCForumSect_AccessBits[3];

    /* Secret key B to given by the application */
    uint8_t     ScrtKeyB[6];

    /* Specifies the status of the different authentication handled in
    formatting procedure*/
    uint8_t     AuthState;

    /* Stores the current block */
    uint16_t    CurrentBlock;

    /* Stores the current block */
    uint8_t     NoOfDevices;

    /* Store the compliant sectors */
    uint8_t     SectCompl[40];

    /* Flag to know that MAD sector */
    uint8_t     WrMADBlkFlag;

    /* Fill the MAD sector blocks */
    uint8_t     MADSectBlk[80];

    /* Fill the MAD sector blocks */
    uint8_t     UpdMADBlk;
} phFriNfc_MfStd_AddInfo_t;


typedef struct phFriNfc_ISO15693_AddInfo
{
    /* Stores the current block executed */
    uint16_t        current_block;
    /* Sequence executed */
    uint8_t         format_seq;
    /* Maximum data size in the card */
    uint16_t        max_data_size;
    /* Card capability */
    uint32_t        card_capability;
}phFriNfc_ISO15693_AddInfo_t;

typedef struct phFriNfc_sNdefSmtCrdFmt_AddInfo
{
    phFriNfc_Type1_AddInfo_t         Type1Info;
    phFriNfc_Type2_AddInfo_t         Type2Info;
    phFriNfc_Type4_AddInfo_t         Type4Info;
    phFriNfc_MfStd_AddInfo_t         MfStdInfo;
    phFriNfc_ISO15693_AddInfo_t      s_iso15693_info;
}phFriNfc_sNdefSmtCrdFmt_AddInfo_t;

typedef struct phFriNfc_sNdefSmtCrdFmt
{
     /** Pointer to the lower (HAL) instance.*/
    void                                *LowerDevice;

    /** Holds the device additional informations*/
    phHal_sDepAdditionalInfo_t          psDepAdditionalInfo;

    /** Pointer to the Remote Device Information */
    phHal_sRemoteDevInformation_t       *psRemoteDevInfo;

    /** Stores the type of the smart card. */
    uint8_t                             CardType;

    /** Stores operating mode type of the MifareStd. */
    /* phHal_eOpModes_t                    OpModeType[2]; */

     /**< \internal The state of the operation. */
    uint8_t                             State;

    /**< \internal Stores the card state Ex: Blank/Formatted etc. */
    uint8_t                             CardState;

     /**< \internal Completion Routine Context. */
    phFriNfc_CplRt_t                    CompletionRoutine[PH_FRINFC_SMTCRDFMT_CR];

     /**<\internal Holds the completion routine informations of the Smart Card Formatting Layer*/
    phFriNfc_CplRt_t                    SmtCrdFmtCompletionInfo;

     /**<\internal Holds the Command Type(read/write)*/
    phHal_uCmdList_t                    Cmd;

     /**< \internal Holds the length of the received data. */
    uint16_t                            *SendRecvLength;

    /**<\internal Holds the ack of some intial commands*/
    uint8_t                             *SendRecvBuf;

      /**< \internal Holds the length of the data to be sent. */
    uint16_t                            SendLength;

    /**< \internal Stores the output/result of the format procedure. Ex: Formatted Successfully,
             Format Error etc */
    NFCSTATUS                           FmtProcStatus;

    /** Stores Additional Information needed to format the different types of tags*/
    phFriNfc_sNdefSmtCrdFmt_AddInfo_t   AddInfo;

    /*  Stores NDEF message TLV*/
    /* This stores the different TLV messages for the different card types*/
    uint8_t   TLVMsg[PH_FRINFC_SMTCRDFMT_MAX_TLV_TYPE_SUPPORTED][8];


} phFriNfc_sNdefSmtCrdFmt_t;


NFCSTATUS phFriNfc_NdefSmtCrd_Reset(phFriNfc_sNdefSmtCrdFmt_t       *NdefSmtCrdFmt,
                                    void                            *LowerDevice,
                                    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
                                    phHal_sDevInputParam_t          *psDevInputParam,
                                    uint8_t                         *SendRecvBuffer,
                                    uint16_t                        *SendRecvBuffLen);


NFCSTATUS phFriNfc_NdefSmtCrd_SetCR(phFriNfc_sNdefSmtCrdFmt_t     *NdefSmtCrdFmt,
                                    uint8_t                       FunctionID,
                                    pphFriNfc_Cr_t                CompletionRoutine,
                                    void                          *CompletionRoutineContext);


NFCSTATUS phFriNfc_NdefSmtCrd_Format(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt, const uint8_t *ScrtKeyB);


NFCSTATUS phFriNfc_NdefSmtCrd_ConvertToReadOnly(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt);

void phFriNfc_NdefSmtCrd_Process(void        *Context,
                                 NFCSTATUS    Status);

void phFriNfc_SmtCrdFmt_HCrHandler(phFriNfc_sNdefSmtCrdFmt_t  *NdefSmtCrdFmt,
                                   NFCSTATUS            Status);
