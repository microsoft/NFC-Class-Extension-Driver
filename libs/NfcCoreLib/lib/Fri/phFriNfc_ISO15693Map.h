/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#define ISO15693_BYTES_PER_BLOCK            0x04U
#define ISO15693_BLOCKS_PER_PAGE            0x04U
/* 3 BYTE value identifier for NDEF TLV */
#define ISO15693_THREE_BYTE_LENGTH_ID       0xFFU

/* Command identifiers */
#define ISO15693_READ_COMMAND               0x20U
#define ISO15693_WRITE_COMMAND              0x21U
#define ISO15693_READ_MULTIPLE_COMMAND      0x23U
#define ISO15693_EXT_READ_COMMAND           0x30U
#define ISO15693_EXT_WRITE_COMMAND          0x31U
#define ISO15693_EXT_READ_MULTIPLE_COMMAND  0x33U

#define ICODE_INVENTORY_PAGEREAD_COMMAND    0xB0U

/* REQUEST flags */
#define ISO15693_FLAG_HIGH_DATARATE         0x02U
#define ISO15693_FLAG_IPR                   0x04U
#define ISO15693_FLAG_PROTOEXT              0x08U
#define ISO15693_FLAG_UID                   0x20U

/* CC indicating tag is capable of multi-block read */
#define ISO15693_CC_USE_MBR                 0x01U
/* CC indicating tag is capable of inventory page read */
#define ISO15693_CC_USE_IPR                 0x02U
/* CC indicating tag memory size exceeds the CC2 field */
#define ISO15693_CC_MEM_EXCEEDED            0x04U

/* UID bytes to differentiate ICODE cards */
#define ISO15693_UID_BYTE_4                 0x04U
#define ISO15693_UID_BYTE_5                 0x05U
#define ISO15693_UID_BYTE_6                 0x06U
#define ISO15693_UID_BYTE_7                 0x07U

/* UID 7th byte value shall be 0xE0 */
#define ISO15693_UIDBYTE_7_VALUE            0xE0U

/* UID 6th byte manufacturer identifier */
#define ISO15693_MANUFACTURER_STM           0x02U
#define ISO15693_MANUFACTURER_NXP           0x04U
#define ISO15693_MANUFACTURER_TI            0x07U

/* UID value for SL2 ICS20, SL2S2002 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X      0x01U

/* UID value for SL2 ICS53, SL2 ICS54, SL2S5302 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X_S    0x02U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_S    0x00U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_SHC  0x80U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_SY   0x40U

/* UID value for SL2 ICS50, SL2 ICS51, SL2S5002 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X_L    0x03U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_L    0x00U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_LHC  0x80U

/* UID value for LRIS64K, M24LR04E-R, M24LR16E-R, M24LR64E-R */
#define ISO15693_UIDBYTE_5_STM_MASK         0xFC
#define ISO15693_UIDBYTE_5_STM_LRIS64K      0x44
#define ISO15693_UIDBYTE_5_STM_M24LR64R     0x2C
#define ISO15693_UIDBYTE_5_STM_M24LR64ER    0x5C
#define ISO15693_UIDBYTE_5_STM_M24LR16ER    0x4C

#define ISO15693_STM_LRIS64K_MAX_SIZE       8192
#define ISO15693_STM_M24LR16ER_MAX_SIZE     2048
#define ISO15693_STM_M24LR64X_MAX_SIZE      8192

/* Get the NDEF TLV VALUE field block and byte address */
#define ISO15693_GET_VALUE_FIELD_BLOCK_NO(blk, byte_addr, ndef_size) \
    (((byte_addr + 1 + ((ndef_size >= ISO15693_THREE_BYTE_LENGTH_ID) ? 3 : 1)) > \
    (ISO15693_BYTES_PER_BLOCK - 1)) ? (blk + 1) : blk)

#define ISO15693_GET_VALUE_FIELD_BYTE_NO(blk, byte_addr, ndef_size) \
    (((byte_addr + 1 + ((ndef_size >= ISO15693_THREE_BYTE_LENGTH_ID) ? 3 : 1)) % \
    ISO15693_BYTES_PER_BLOCK))

/* Check if protocol extension bit is needed in the request flag */
#define ISO15693_PROTOEXT_FLAG_REQUIRED(pUid) \
    ((ISO15693_MANUFACTURER_STM == pUid[ISO15693_UID_BYTE_6]) && \
        ((pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_LRIS64K || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64R || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64ER || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR16ER))

#define ISO15693_STM_M24LR_MAX_BLOCKS_READ_PER_SECTOR 0x20U

NFCSTATUS
phFriNfc_ISO15693_RdNdef (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint8_t             *pPacketData,
    uint32_t            *pPacketDataLength,
    uint8_t             Offset);

NFCSTATUS
phFriNfc_ISO15693_WrNdef (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint8_t             *pPacketData,
    uint32_t            *pPacketDataLength,
    uint8_t             Offset);

NFCSTATUS
phFriNfc_ISO15693_ChkNdef (
    phFriNfc_NdefMap_t  *psNdefMap);

void
phFriNfc_ISO15693_Process (
    void        *pContext,
    NFCSTATUS   Status);

NFCSTATUS
phFriNfc_ISO15693_ConvertToReadOnly (
    phFriNfc_NdefMap_t  *psNdefMap);

extern
NFCSTATUS phFrinfc_15693_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,
                                   uint32_t *maxSize, uint32_t *actualSize);
