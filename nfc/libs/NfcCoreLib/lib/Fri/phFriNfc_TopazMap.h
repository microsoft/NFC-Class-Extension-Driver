/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phFriNfc.h>
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phFriNfc_NdefMap.h>

#define TOPAZ_UID_LENGTH                              4

#define PH_FRINFC_TOPAZ_STATE_READ                    1   /*!< Read State */
#define PH_FRINFC_TOPAZ_STATE_WRITE                   2   /*!< Write is going on*/
#define PH_FRINFC_TOPAZ_STATE_CHK_NDEF                3   /*!< Check Ndef is going on */
#define PH_FRINFC_TOPAZ_STATE_READALL                 5   /*!< Read all under progress */
#define PH_FRINFC_TOPAZ_STATE_WRITE_NMN               6   /*!< Write ndef magic number */
#define PH_FRINFC_TOPAZ_STATE_WRITE_L_TLV             7   /*!< Write length field of TLV */
#define PH_FRINFC_TOPAZ_STATE_WR_CC_OR_TLV            8   /*!< Write CC or NDEF TLV */
#define PH_FRINFC_TOPAZ_STATE_WR_CC_BYTE              9   /*!< READ ONLY state */
#define PH_FRINFC_TOPAZ_STATE_RD_LOCK0_BYTE           10  /*!< read Lock byte 0 state */
#define PH_FRINFC_TOPAZ_STATE_WR_LOCK0_BYTE           11  /*!< write Lock byte 0 state */
#define PH_FRINFC_TOPAZ_STATE_RD_LOCK1_BYTE           12  /*!< read Lock byte 1 state */
#define PH_FRINFC_TOPAZ_STATE_WR_LOCK1_BYTE           13  /*!< write Lock byte 1 state */

#define PH_FRINFC_TOPAZ_CC_BYTE0                    0xE1 /*!< Capability container byte 0 = 0xE1 (NMN) */
#define PH_FRINFC_TOPAZ_CC_BYTE1                    0x10 /*!< Capability container byte 1 = 0x10 (version number) */
#define PH_FRINFC_TOPAZ_CC_BYTE2_STATIC_MAX         0x0E /*!< Capability container byte 2 = 0x0E (Total free space
                                                            in the card) for Topaz card with static memory layout */
#define PH_FRINFC_TOPAZ_CC_BYTE3_RW                 0x00 /*!< Capability container byte 3 = 0x00 for
                                                                  READ WRITE/INITIALISED card state */
#define PH_FRINFC_TOPAZ_CC_BYTE3_RO                 0x0F /*!< Capability container byte 3 = 0x0F for
                                                                  READ only card state */

#define PH_FRINFC_TOPAZ_FLAG0                       0 /*!< Flag value = 0 */
#define PH_FRINFC_TOPAZ_FLAG1                       1 /*!< Flag value = 1 */

enum
{
    PH_FRINFC_TOPAZ_WR_CC_BYTE0,                  /*!< CC Byte 0 = 0xE1 ndef magic number */
    PH_FRINFC_TOPAZ_WR_CC_BYTE1,                  /*!< CC Byte 1 = 0x10 version number */
    PH_FRINFC_TOPAZ_WR_CC_BYTE2,                  /*!< CC Byte 2 = 0x0C space in the data area */
    PH_FRINFC_TOPAZ_WR_CC_BYTE3,                  /*!< CC Byte 3 = 0x00 read write access */
    PH_FRINFC_TOPAZ_WR_T_OF_TLV,                  /*!< CC Byte 3 = 0x00 read write access */
    PH_FRINFC_TOPAZ_WR_NMN_0,                     /*!< NMN = 0x00 */
    PH_FRINFC_TOPAZ_WR_NMN_E1,                    /*!< NMN = 0xE1 */
    PH_FRINFC_TOPAZ_WR_L_TLV_0,                   /*!< L field of TLV = 0 */
    PH_FRINFC_TOPAZ_WR_L_TLV,                     /*!< To update the L field */
    PH_FRINFC_TOPAZ_DYNAMIC_INIT_CHK_NDEF,    /*!< Internal state to represent the  parsing of card to locate Ndef TLV*/
    PH_FRINFC_TOPAZ_DYNAMIC_INIT_FIND_NDEF_TLV
};

/* Refer to section 2.4, TLV Blocks, of the NFC Forum Type 1 Tag spec */
#define PH_FRINFC_TOPAZ_TLV_NULL_T               0x00 /*!< Null TLV value = 0x00 */
#define PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_T          0x01 /*!< Lock control TLV = 0x01 */
#define PH_FRINFC_TOPAZ_TLV_MEM_CTRL_T           0x02 /*!< Reserved memory control TLV = 0x02 */
#define PH_FRINFC_TOPAZ_TLV_NDEF_T               0x03 /*!< NDEF TLV = 0x03 */
#define PH_FRINFC_TOPAZ_TLV_PROP_T               0xFD /*!< Proprietary TLV = 0xFD */
#define PH_FRINFC_TOPAZ_TLV_TERM_T               0xFE /*!< Terminator TLV = 0xFE */

#define PH_FRINFC_TOPAZ_TLV_NDEF_L               0x00 /*!< Length of intial NDEF TLV */
#define PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_L          0x03 /*!< Length of lock control TLV */
#define PH_FRINFC_TOPAZ_TLV_MEM_CTRL_L           0x03 /*!< Length of memory control TLV */

#define PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V0         0xF0 /*!< Value 0 of lock control TLV */
#define PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V1         0x10 /*!< Value 1 of lock control TLV */
#define PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V2         0x33 /*!< Value 2 of lock control TLV */

#define PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V0          0xF2 /*!< Value 0 of memory control TLV */
#define PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V1          0x06 /*!< Value 1 of memory control TLV */
#define PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V2          0x03 /*!< Value 2 of memory control TLV */

#define PH_FRINFC_TOPAZ_MAX_CARD_SZ              0x60 /*!< Send Length for Read Ndef */
#define PH_FRINFC_TOPAZ_WR_A_BYTE                0x02 /*!< Send Length for Write Ndef */
#define PH_FRINFC_TOPAZ_SEND_BUF_READ            0x01 /*!< Send Length for Read Ndef */
#define PH_FRINFC_TOPAZ_HEADROM0_VAL             0x11 /*!< Header rom byte 0 value of static card */
#define PH_FRINFC_TOPAZ_READID_RESP_SIZE         6    /*!< Size of READ ID response */
#define PH_FRINFC_TOPAZ_READALL_RESP_SIZE        122  /*!< Size of READ ALL response */
#define PH_FRINFC_TOPAZ_TOTAL_RWBYTES            0x60 /*!< Total number of raw Bytes that can
                                                            be read or written to the card 96 bytes */
#define PH_FRINFC_TOPAZ_TOTAL_RWBYTES1           0x5A /*!< Total number of bytes that can be read
                                                            or written 90 bytes */
#define PH_FRINFC_TOPAZ_BYTE3_MSB                0xF0 /*!< most significant nibble of byte 3(RWA) shall be
                                                            0 */
#define PH_FRINFC_TOPAZ_LOCKBIT_BYTE114          0x01 /*!< lock bits value of byte 104 */
#define PH_FRINFC_TOPAZ_LOCKBIT_BYTE115_1        0x60 /*!< lock bits value of byte 105 */
#define PH_FRINFC_TOPAZ_LOCKBIT_BYTE115_2        0xE0 /*!< lock bits value of byte 105 */
#define PH_FRINFC_TOPAZ_LOCKBIT_BYTENO_0         114  /*!< lock bits byte number 104 */
#define PH_FRINFC_TOPAZ_LOCKBIT_BYTENO_1         115  /*!< lock bits byte number 105 */
#define PH_FRINFC_TOPAZ_CC_BYTENO_3              13   /*! Lock status according to CC bytes */
#define PH_FRINFC_TOPAZ_CC_READWRITE             0x00 /*! Lock status according to CC bytes */
#define PH_FRINFC_TOPAZ_CC_READONLY              0x0F /*! Lock status according to CC bytes */

/**Topaz static commands*/
#define PH_FRINFC_TOPAZ_CMD_READID               0x78U
#define PH_FRINFC_TOPAZ_CMD_READALL              0x00U
#define PH_FRINFC_TOPAZ_CMD_READ                 0x01U
#define PH_FRINFC_TOPAZ_CMD_WRITE_1E             0x53U
#define PH_FRINFC_TOPAZ_CMD_WRITE_1NE            0x1AU

/**Topaz Dynamic commands*/
#define PH_FRINFC_TOPAZ_CMD_RSEG                 0x10U
#define PH_FRINFC_TOPAZ_CMD_READ8                0x02U
#define PH_FRINFC_TOPAZ_CMD_WRITE_E8             0x54U
#define PH_FRINFC_TOPAZ_CMD_WRITE_NE8            0x1BU

enum
{
    PH_FRINFC_TOPAZ_VAL0,
    PH_FRINFC_TOPAZ_VAL1,
    PH_FRINFC_TOPAZ_VAL2,
    PH_FRINFC_TOPAZ_VAL3,
    PH_FRINFC_TOPAZ_VAL4,
    PH_FRINFC_TOPAZ_VAL5,
    PH_FRINFC_TOPAZ_VAL6,
    PH_FRINFC_TOPAZ_VAL7,
    PH_FRINFC_TOPAZ_VAL8,
    PH_FRINFC_TOPAZ_VAL9,
    PH_FRINFC_TOPAZ_VAL10,
    PH_FRINFC_TOPAZ_VAL11,
    PH_FRINFC_TOPAZ_VAL12,
    PH_FRINFC_TOPAZ_VAL13,
    PH_FRINFC_TOPAZ_VAL14,
    PH_FRINFC_TOPAZ_VAL15,
    PH_FRINFC_TOPAZ_VAL16,
    PH_FRINFC_TOPAZ_VAL17,
    PH_FRINFC_TOPAZ_VAL18
};

NFCSTATUS phFriNfc_Topaz_H_Reset(phFriNfc_NdefMap_t *NdefMap,
                               phNfc_sDevInputParam_t *pDevInpParam);

/**< This function shall calculate Topaz container size */
NFCSTATUS phFriNfc_Topaz_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,
                                          uint32_t *maxSize,
                                          uint32_t *actualSize);

NFCSTATUS
phFriNfc_TopazMap_ConvertToReadOnly (
    phFriNfc_NdefMap_t          *NdefMap);

NFCSTATUS phFriNfc_TopazMap_RdNdef( phFriNfc_NdefMap_t  *NdefMap,
                                    uint8_t             *PacketData,
                                    uint32_t            *PacketDataLength,
                                    uint8_t             Offset);

NFCSTATUS phFriNfc_TopazMap_WrNdef( phFriNfc_NdefMap_t  *NdefMap,
                                    uint8_t             *PacketData,
                                    uint32_t            *PacketDataLength,
                                    uint8_t             Offset);

NFCSTATUS phFriNfc_TopazMap_ChkNdef(phFriNfc_NdefMap_t  *NdefMap);

extern NFCSTATUS phFriNfc_Tpz_H_ChkSpcVer( phFriNfc_NdefMap_t  *NdefMap,
                                          uint8_t             VersionNo);

void phFriNfc_TopazMap_Process( void        *Context,
                                NFCSTATUS   Status);

#define PH_FRINFC_TOPAZ_DYNAMIC_CC_BYTE2_MMSIZE                 0x3F  /*!< Capability container byte 2 = 0x3F (Total free space
                                                                        in the card) for Topaz card with dynamic memory layout */
#define PH_FRINFC_TOPAZ_DYNAMIC_HEADROM0_VAL                    0x12  /*!< Header rom byte 0 value of dynamic card */

#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_0                      0x01 /*!< Value of lock byte 0 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_1                      0xE0 /*!< Value of lock byte 1 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_2TO7                   0x00 /*!< Value of lock bytes 2-7 */

#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_0                112  /*!< Lock bits: Blocks 00-07 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_1                113  /*!< Lock bits: Blocks 08-0F */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_2                122  /*!< Lock bits: Blocks 10-17 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_3                123  /*!< Lock bits: Blocks 18-1F */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_4                124  /*!< Lock bits: Blocks 20-27 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_5                125  /*!< Lock bits: Blocks 28-2F */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_6                126  /*!< Lock bits: Blocks 30-37 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_7                127  /*!< Lock bits: Blocks 38-3F */

#define PH_FRINFC_TOPAZ_DYNAMIC_SEGMENT0                        0x00 /*!< 00000000 : 0th segment */
#define PH_FRINFC_TOPAZ_DYNAMIC_READSEG_RESP                    0x80

enum
{
    NULL_TLV,
    LOCK_TLV,
    MEM_TLV,
    NDEF_TLV,
    PROP_TLV,
    TERM_TLV,
    INVALID_TLV,
    VALID_TLV,
    TLV_NOT_FOUND
};

/**< This function shall calculate Topaz dynamic container size */
NFCSTATUS phFriNfc_TopazDynamic_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,
                                                 uint32_t *maxSize,
                                                 uint32_t *actualSize);

NFCSTATUS phFriNfc_TopazDynamicMap_RdNdef( phFriNfc_NdefMap_t  *NdefMap,
                                    uint8_t             *PacketData,
                                    uint32_t            *PacketDataLength,
                                    uint8_t             Offset);

NFCSTATUS
phFriNfc_TopazDynamicMap_WrNdef(
    _Inout_                             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(*PacketDataLength) uint8_t             *PacketData,
    _Inout_                             uint32_t            *PacketDataLength,
    _In_                                uint8_t             Offset
    );

NFCSTATUS phFriNfc_TopazDynamicMap_ChkNdef(    phFriNfc_NdefMap_t  *NdefMap);

void phFriNfc_TopazDynamicMap_Process( void        *Context,
                                NFCSTATUS   Status);

NFCSTATUS
phFriNfc_TopazDynamicMap_ConvertToReadOnly(
    phFriNfc_NdefMap_t     *psNdefMap);
