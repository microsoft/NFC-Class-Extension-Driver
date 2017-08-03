/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

enum
{
    PH_FRINFC_DESF_STATE_CREATE_AID = 0,
    PH_FRINFC_DESF_STATE_SELECT_APP = 1,
    PH_FRINFC_DESF_STATE_CREATE_CCFILE = 2,
    PH_FRINFC_DESF_STATE_CREATE_NDEFFILE = 3,
    PH_FRINFC_DESF_STATE_WRITE_CC_FILE = 4,
    PH_FRINFC_DESF_STATE_WRITE_NDEF_FILE = 5,
    PH_FRINFC_DESF_STATE_DISCON = 6,
    PH_FRINFC_DESF_STATE_CON = 7,
    PH_FRINFC_DESF_STATE_POLL = 8,
    PH_FRINFC_DESF_STATE_GET_UID = 9,
    PH_FRINFC_DESF_STATE_GET_SW_VERSION = 10,
    PH_FRINFC_DESF_STATE_GET_HW_VERSION = 11,

#ifdef DESFIRE_FMT_EV1
    PH_FRINFC_DESF_STATE_RO_SELECT_APP_EV1 = 100,
#endif /* #ifdef DESFIRE_FMT_EV1 */

    PH_FRINFC_DESF_STATE_RO_SELECT_APP = 101,
    PH_FRINFC_DESF_STATE_RO_SELECT_CC_FILE = 102,
    PH_FRINFC_DESF_STATE_RO_READ_CC_FILE = 103,
    PH_FRINFC_DESF_STATE_RO_UPDATE_CC_FILE = 104,

    /* following are used in the ISO wrapper commands*/
    PH_FRINFC_DESF_CREATEAPP_CMD = 0,
    PH_FRINFC_DESF_SELECTAPP_CMD = 1,
    PH_FRINFC_DESF_CREATECC_CMD = 2,
    PH_FRINFC_DESF_CREATENDEF_CMD = 3,
    PH_FRINFC_DESF_WRITECC_CMD = 4,
    PH_FRINFC_DESF_WRITECC_CMD_READ_ONLY = 20,
    PH_FRINFC_DESF_WRITENDEF_CMD = 5,
    PH_FRINFC_DESF_GET_HW_VERSION_CMD = 6,
    PH_FRINFC_DESF_GET_SW_VERSION_CMD = 7,
    PH_FRINFC_DESF_GET_UID_CMD = 8,
    PH_FRINFC_DESF_WRITENDEF_CMD_SNLEN = 15,
    PH_FRINFC_DESF_WRITECC_CMD_SNLEN = 28,
    PH_FRINFC_DESF_CREATECCNDEF_CMD_SNLEN = 13,
    PH_FRINFC_DESF_SELECTAPP_CMD_SNLEN = 9,
    PH_FRINFC_DESF_CREATEAPP_CMD_SNLEN = 11,
    PH_FRINFC_DESF_NATIVE_OFFSET_P1 = 0x00,
    PH_FRINFC_DESF_NATIVE_OFFSET_P2 = 0x00,
    PH_FRINFC_DESF_NATIVE_LE_BYTE = 0x00,
    PH_FRINFC_DESF_NATIVE_CRAPP_WRDT_LEN = 5,
    PH_FRINFC_DESF_NATIVE_SLAPP_WRDT_LEN = 3,
    PH_FRINFC_DESF_NATIVE_CRCCNDEF_WRDT_LEN = 7,
    PH_FRINFC_DESF_NATIVE_WRCC_WRDT_LEN = 22,
    PH_FRINFC_DESF_NATIVE_WRNDEF_WRDT_LEN = 9
};


/* CC File contents*/
#define  PH_FRINFC_DESF_CCFILE_BYTES                    {0x00,0x0f,0x10,0x00,0x3B,0x00,0x34,0x04,0x06,0xE1,0x04,0x04,0x00,0x00,0x00 }
#define  PH_FRINFC_DESF_NDEFFILE_BYTES                  {0x00,0x00}
#define  PH_FRINFC_DESF_PICC_MASTER_KEY                 {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }
#define  PH_FRINFC_DESF_NFCFORUM_APP_KEY                {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }
#define  PH_FRINFC_DESF_COMM_SETTINGS                   0x00
#define  PH_FRINFC_DESF_CREATE_DATA_FILE_CMD            0xCD
#define  PH_FRINFC_DESF_NATIVE_CLASS_BYTE               0x90

/* Constant defined to specify the NFC Forum Application ID : 0xEEEE10*/
/* This is defined in order to support to N/W Byte order style : LSB : : MSB*/
#define PH_FRINFC_DESF_FIRST_AID_BYTE                   0x10
#define PH_FRINFC_DESF_SEC_AID_BYTE                     0xEE
#define PH_FRINFC_DESF_THIRD_AID_BYTE                   0xEE


/* Create File command constants*/
#define  PH_FRINFC_DESF_CREATE_AID_CMD                  0xCA

/* Specifies the NFC Forum App Number of Keys*/
#define  PH_FRINFC_DESF_NFCFORUM_APP_NO_OF_KEYS         0x01

#define  PH_FRINFC_DESF_SLECT_APP_CMD                   0x5A

#define  PH_FRINFC_DESF_GET_VER_CMD                     0x60


#define  PH_FRINFC_DESF_NATIVE_RESP_BYTE1               0x91
#define  PH_FRINFC_DESF_NATIVE_RESP_BYTE2               0x00

/* Create CC File Commands*/
#define  PH_FRINFC_DESF_CC_FILE_ID                      0x03
#define  PH_FRINFC_DESF_CC_FILE_SIZE                    0x0F
#define  PH_FRINFC_DESF_FIRST_BYTE_CC_ACCESS_RIGHTS     0x00
#define  PH_FRINFC_DESF_SEC_BYTE_CC_ACCESS_RIGHTS       0xE0


/* Create NDEF File Commands*/
#define  PH_FRINFC_DESF_NDEF_FILE_ID                    0x04
#define  PH_FRINFC_DESF_NDEF_FILE_SIZE                  0x04
#define  PH_FRINFC_DESF_FIRST_BYTE_NDEF_ACCESS_RIGHTS   0xE0
#define  PH_FRINFC_DESF_SEC_BYTE_NDEF_ACCESS_RIGHTS     0xEE


/* Write/Read Data commands/constants*/
#define  PH_FRINFC_DESF_WRITE_CMD                       0x3D

/* PICC additional frame response*/
#define  PH_FRINFC_DESF_PICC_ADDI_FRAME_RESP            0xAF

/* Response for PICC native DESFire wrapper cmd*/
#define  PH_FRINFC_DESF_NAT_WRAP_FIRST_RESP_BYTE        0x91
#define  PH_FRINFC_DESF_NAT_WRAP_SEC_RESP_BYTE          0x00

/* DESFire4 Major/Minor versions*/
#define  PH_FRINFC_DESF4_MAJOR_VERSION                  0x00
#define  PH_FRINFC_DESF4_MINOR_VERSION                  0x06

/* DESFire4 memory size*/
#define  PH_FRINFC_DESF4_MEMORY_SIZE                    0xEDE

enum
{
    PH_SMTCRDFMT_DESF_VAL0 = 0,
    PH_SMTCRDFMT_DESF_VAL1 = 1,
    PH_SMTCRDFMT_DESF_VAL2 = 2,
    PH_SMTCRDFMT_DESF_VAL3 = 3,
    PH_SMTCRDFMT_DESF_VAL4 = 4,
    PH_SMTCRDFMT_DESF_VAL14 = 14,
    PH_SMTCRDFMT_DESF_VAL15 = 15
};

void
phFriNfc_Desfire_Reset(
    _Inout_ phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt
    );

NFCSTATUS
phFriNfc_Desfire_Format(
    _Inout_ phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt
    );

NFCSTATUS
phFriNfc_Desfire_ConvertToReadOnly(
    _Inout_ phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt
    );

void
phFriNfc_Desf_Process(
    _At_((phFriNfc_sNdefSmtCrdFmt_t*)Context, _Inout_)  void        *Context,
    _In_                                                NFCSTATUS   Status
    );
