/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phFriNfc.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phNfcHalTypes2.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phFriNfc_NdefMap.h>

#define PH_FRINFC_MFUL_FMT_RESET_INIT               0 /*!< Reset state */
#define PH_FRINFC_MFUL_FMT_RD_16BYTES               1 /*!< Read 16 bytes */
#define PH_FRINFC_MFUL_FMT_WR_OTPBYTES              2 /*!< Write OTP bytes */
#define PH_FRINFC_MFUL_FMT_WR_TLV                   3 /*!< Write TLV */
#define PH_FRINFC_MFUL_FMT_WR_TLV1                  4 /*!< Write TLV (second part) */
#define PH_FRINFC_MFUL_FMT_RO_RD_16BYTES            5 /*!< Read only the tag */
#define PH_FRINFC_MFUL_FMT_RO_WR_LOCK_BYTES         6 /*!< Write lock bytes to make the tag Read only */
#define PH_FRINFC_MFUL_FMT_RO_WR_OTP_BYTES          7 /*!< Write OTP bytes to make the tag Read only */
#define PH_FRINFC_MFUL_FMT_RO_RD_DYN_LOCK_BYTES     8 /*!< Read default dynamic lock bytes address */
#define PH_FRINFC_MFUL_FMT_RO_WR_DYN_LOCK_BYTES     9 /*!< Write default dynamic lock bytes address */
#define PH_FRINFC_MFUL_FMT_RO_PARSE_NDEF            10 /*!< Write default dynamic lock bytes address */
#define PH_FRINFC_MFUL_FMT_RO_NDEF_PARSE_RD_BYTES   12 /*!< Read bytes from the card for parsing NDEF */

#define PH_FRINFC_MFUL_FMT_LOCK_BITS_VAL            0x00   /*!< Lock bits block is 2 */

#define PH_FRINFC_MFULC_FMT_OTP_BYTES               {0xE1, 0x10, 0x12, 0x00}   /*!< OTP bytes macro */
#define PH_FRINFC_MFUL_FMT_OTP_BYTES                {0xE1, 0x10, 0x06, 0x00}   /*!< OTP bytes macro */

enum
{
    PH_FRINFC_MFUL_FMT_VAL_0,
    PH_FRINFC_MFUL_FMT_VAL_1,
    PH_FRINFC_MFUL_FMT_VAL_2,
    PH_FRINFC_MFUL_FMT_VAL_3,
    PH_FRINFC_MFUL_FMT_VAL_4,
    PH_FRINFC_MFUL_FMT_VAL_5,
    PH_FRINFC_MFUL_FMT_VAL_6,
    PH_FRINFC_MFUL_FMT_VAL_7
};

#define PH_FRINFC_MFUL_FMT_NON_NDEF_COMPL       0   /*!< Card is not ndef compliant */
#define PH_FRINFC_MFUL_FMT_NDEF_COMPL           1   /*!< Card is ndef compliant */

#define PH_FRINFC_MFUL_FMT_MAX_RECV_LENGTH      252 /*!< Maximum receive length */
#define PH_FRINFC_MFUL_FMT_WR_SEND_LENGTH       5   /*!< Send length for write */
#define PH_FRINFC_MFUL_FMT_MAX_BLK              16  /*!< Maximum blocks */


void
phFriNfc_MfUL_Reset(
    phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

NFCSTATUS
phFriNfc_MfUL_Format(
    phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

NFCSTATUS
phFriNfc_MfUL_ConvertToReadOnly (
    phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);


void phFriNfc_MfUL_Process(void            *Context,
                            NFCSTATUS       Status);
