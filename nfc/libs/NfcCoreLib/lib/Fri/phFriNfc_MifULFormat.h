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

#define PH_FRINFC_MFUL_FMT_LOCK_BITS_VAL            0x00 /*!< Lock bits block is 2 */

/*
* OTP bytes: Byte 0 is the NDEF magic number 0xE1. Byte 1 is the version number of the Type 2 Tag
* Platform supported. Byte 2 is the memory size of the data area of the tag. Byte 3 indicates the
* read and write access capability of the data area (0x00 means there is no restrictions on read
* and write access). See Section 6.1, NDEF Management, of the NFC Forum Type 2 Tag spec for more
* information.
*/
#define PH_FRINFC_MFUL_FMT_OTP_NDEF_MAGIC_NUMBER_BYTE        0 /*!< OTP byte that contains the NDEF magic number */
#define PH_FRINFC_MFUL_FMT_OTP_TYPE2_TAG_VERSION_NUMBER_BYTE 1 /*!< OTP byte that contains the version of the Type 2 Tag spec implemented */
#define PH_FRINFC_MFUL_FMT_OTP_DATA_AREA_SIZE_BYTE           2 /*!< OTP byte that contains the data area size */
#define PH_FRINFC_MFUL_FMT_OTP_READ_WRITE_ACCESS_BYTE        3 /*!< OTP byte that contains the read/write access of the data area */

#define PH_FRINFC_MFUL_FMT_NDEF_MAGIC_NUMBER        0xE1 /*!< NDEF magic number */
#define PH_FRINFC_MFUL_FMT_TYPE2_TAG_VERSION_NUMBER 0x10 /*!< Version of the NFC Forum Type 2 Tag spec implemented (1.0) */
#define PH_FRINFC_MFUL_FMT_READ_WRITE_ACCESS        0x00 /*!< Read/write access of the data area */
#define PH_FRINFC_MFUL_FMT_DATA_AREA_SIZE           0x06 /*!< Size of data area for Ultralight */
#define PH_FRINFC_MFULC_FMT_DATA_AREA_SIZE          0x12 /*!< Size of data area for Ultralight C */

#define PH_FRINFC_MFUL_FMT_OTP_BYTES_TEMPLATE   { PH_FRINFC_MFUL_FMT_NDEF_MAGIC_NUMBER, PH_FRINFC_MFUL_FMT_TYPE2_TAG_VERSION_NUMBER, 0x00, PH_FRINFC_MFUL_FMT_READ_WRITE_ACCESS } /*!< OTP bytes template for Ultralight. Data area size byte is not set */

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
