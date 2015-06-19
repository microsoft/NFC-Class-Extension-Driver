/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

typedef enum phLibNfc_Last_Call
{
    ChkNdef   = 0x00,
    NdefRd,
    NdefWr,
    NdefFmt,
    NdefReadOnly,
    RawTrans
} phLibNfc_Last_Call_t;

#define TAG_MIFARE       0x01
#define TAG_FELICA       0x02
#define TAG_JEWEL        0x04
#define TAG_ISO14443_4A  0x08
#define TAG_ISO14443_4B  0x10
#define TAG_NFC_IP1      0x20

#define NDEF_READ_TIMER_TIMEOUT          60U
#define CHK_NDEF_TIMER_TIMEOUT           60U
#define NDEF_SENDRCV_BUF_LEN            252U
#define NDEF_TEMP_RECV_LEN              256U
#define NDEF_MIFARE_UL_LEN               46U
#define NDEF_FELICA_LEN                   0U/*Len to be set when supported*/
#define NDEF_JEWEL_TOPAZ_LEN              0U
#define NDEF_ISO14443_4A_LEN           4096U
#define NDEF_ISO14443_4B_LEN              0U
#define NDEF_MIFARE_4K_LEN             3356U
#define NDEF_MIFARE_1K_LEN              716U
#define MIFARE_STD_DEFAULT_KEY          {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MIFARE_STD_KEY_LEN              6
#define ISO_SAK_VALUE                   0x20U

#define UNKNOWN_BLOCK_ADDRESS           0xFF /**<Unknown BLOCK address>*/
#define SESSION_OPEN                    0x01

extern void phLibNfc_Ndef_Init(void);
extern void phLibNfc_Ndef_DeInit(void);
extern phLibNfc_Ndef_Info_t NdefInfo;
extern phFriNfc_NdefRecord_t *pNdefRecord;
