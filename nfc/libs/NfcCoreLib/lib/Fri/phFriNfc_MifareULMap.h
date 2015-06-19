/*
 *          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

#include <phFriNfc.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phFriNfc_NdefMap.h>

#define PH_FRINFC_NDEFMAP_MFUL_STATE_READ                        1   /*!< Read State */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_WRITE                       2   /*!< Write is going on*/
#define PH_FRINFC_NDEFMAP_MFUL_STATE_CHK_NDEF_COMP               3   /*!< Check Ndef is going on */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_FND_NDEF_COMP               4   /*!< to find the NDEF TLV */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_TERM_TLV                    5   /*!< to write the terminator TLV */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_WR_LEN_TLV                  6   /*!< Write L value of TLV */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_CHK_1         7   /*!< to send sector select command 1 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_CHK_2         8   /*!< to send sector select command 2 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_RESET_1       9   /*!< to send sector select command 1 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_RESET_2       10   /*!< to send sector select command 2 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_READ_1        11   /*!< to send sector select command 1 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_READ_2        12   /*!< to send sector select command 2 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_WRITE_1       13   /*!< to send sector select command 1 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_WRITE_2       14   /*!< to send sector select command 2 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_RW_1       15   /*!< to send sector select command 1 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_RW_2      16   /*!< to send sector select command 2 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_WRITE_INIT_1       17   /*!< to send sector select command 1 for resetting sector 0 */
#define PH_FRINFC_NDEFMAP_MFUL_STATE_SELECT_SECTOR_WRITE_INIT_2      18   /*!< to send sector select command 2 for resetting sector 0 */

#define PH_FRINFC_NDEFMAP_MFUL_CC_BYTE0                 0xE1 /*!< Capability container byte 0 = 0xE1 */
#define PH_FRINFC_NDEFMAP_MFUL_CC_BYTE1                 0x10 /*!< Capability container byte 1 = 0x10 */
#define PH_FRINFC_NDEFMAP_MFUL_CC_BYTE2                 0x06 /*!< Capability container byte 2 = 0x06 */
#define PH_FRINFC_NDEFMAP_MFUL_CC_BYTE3_RW              0x00 /*!< Capability container byte 3 = 0x00 for
                                                                  READ WRITE/INITIALISED card state*/
#define PH_FRINFC_NDEFMAP_MFUL_CC_BYTE3_RO              0x0F /*!< Capability container byte 3 = 0x0F for
                                                                  READ only card state*/

#define PH_FRINFC_NDEFMAP_MFUL_FLAG0                    0 /*!< Flag value = 0 */
#define PH_FRINFC_NDEFMAP_MFUL_FLAG1                    1 /*!< Flag value = 1 */

#define PH_FRINFC_NDEFMAP_MFUL_SHIFT8                   8 /*!< Flag value = 0 */

#define PH_FRINFC_NDEFMAP_MFUL_NDEFTLV_T                0x03 /*!< Type value of TLV = 0x03 */
#define PH_FRINFC_NDEFMAP_MFUL_NDEFTLV_L                0x00 /*!< Length value of TLV = 0x00 */
#define PH_FRINFC_NDEFMAP_MFUL_NDEFTLV_LFF              0xFF /*!< Length value of TLV = 0xFF */
#define PH_FRINFC_NDEFMAP_MFUL_TERMTLV                  0xFE /*!< Terminator TLV value = 0xFE */
#define PH_FRINFC_NDEFMAP_MFUL_NULLTLV                  0x00 /*!< Null TLV value = 0x00 */
#define PH_FRINFC_NDEFMAP_MFUL_LOCK_CTRL_TLV            0x01 /*!< Lock Control TLV value = 0x01 */
#define PH_FRINFC_NDEFMAP_MFUL_MEM_CTRL_TLV             0x02 /*!< Memory Control TVL value = 0x02 */
#define PH_FRINFC_NDEFMAP_MFUL_PROPRIETRY_TLV           0xFD /*!< Proprietry TVL value = 0xFD */

#define PH_FRINFC_NDEFMAP_MFUL_WR_A_BLK                 0x05 /*!< Send Length for Write Ndef */
#define PH_FRINFC_NDEFMAP_MFUL_MAX_SEND_BUF_TO_READ     0x01 /*!< Send Length for Read Ndef */
#define PH_FRINFC_NDEFMAP_MFUL_CHECK_RESP               0x04 /*!< Value of the Sense Response for Mifare UL */
#define PH_FRINFC_NDEFMAP_MFUL_OTP_OFFSET               3    /*!< To initialise the Offset */
#define PH_FRINFC_NDEFMAP_MFUL_MUL8                     8    /*!< Multiply by 8 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL0                     0    /*!< Value 0 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL1                     1    /*!< Value 1 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL2                     2    /*!< Value 2 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL3                     3    /*!< Value 3 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL4                     4    /*!< Value 4 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL5                     5    /*!< Value 5 */
#define PH_FRINFC_NDEFMAP_MFUL_VAL64                    64    /*!< Value 64 */
#define PH_FRINFC_NDEFMAP_MFUL_BYTE0                    0x00 /*!< Byte number 0 */
#define PH_FRINFC_NDEFMAP_MFUL_BYTE1                    0x01 /*!< Byte number 1 */
#define PH_FRINFC_NDEFMAP_MFUL_BYTE2                    0x02 /*!< Byte number 2 */
#define PH_FRINFC_NDEFMAP_MFUL_BYTE3                    0x03 /*!< Byte number 3 */
#define PH_FRINFC_NDEFMAP_MFUL_BYTE4                    0x04 /*!< Byte number 4 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK0                   0x00 /*!< Block number 0 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK1                   0x01 /*!< Block number 1 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK2                   0x02 /*!< Block number 2 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK3                   0x03 /*!< Block number 3 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK4                   0x04 /*!< Block number 4 */
#define PH_FRINFC_NDEFMAP_MFUL_BLOCK5                   0x05 /*!< Block number 5 */

#define PH_FRINFC_NDEFMAP_MFUL_RDBYTES_16               0x10 /*!< Read Bytes 16 */
#define PH_FRINFC_NDEFMAP_STMFUL_MAX_CARD_SZ            48   /*!< For static maximum memory size is 48 bytes */
#define PH_FRINFC_NDEFMAP_MFUL_WR_BUF_STR               0x04 /*!< To store the block of data written to the card */


NFCSTATUS phFriNfc_MifareUL_H_Reset(phFriNfc_NdefMap_t *NdefMap,
                                    phNfc_sDevInputParam_t *pDevInpParam);

NFCSTATUS phFriNfc_MifareUL_RdNdef( phFriNfc_NdefMap_t  *NdefMap,
                                    uint8_t             *PacketData,
                                    uint32_t            *PacketDataLength,
                                    uint8_t             Offset);

NFCSTATUS phFriNfc_MifareUL_WrNdef( phFriNfc_NdefMap_t  *NdefMap,
                                    uint8_t             *PacketData,
                                    uint32_t            *PacketDataLength,
                                    uint8_t             Offset);

NFCSTATUS phFriNfc_MifareUL_ChkNdef(    phFriNfc_NdefMap_t  *NdefMap);

void phFriNfc_MifareUL_Process( void        *Context,
                                NFCSTATUS   Status);

NFCSTATUS phFrinfc_MifareUL_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize);
