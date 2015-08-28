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

#define TOPAZ_UID_LENGTH_FOR_READ_WRITE                     0x04U

#define PH_FRINFC_TOPAZ_STATE_READ                    1   /*!< Read State */
#define PH_FRINFC_TOPAZ_STATE_WRITE                   2   /*!< Write is going on*/
#define PH_FRINFC_TOPAZ_STATE_CHK_NDEF                3   /*!< Check Ndef is going on */
#define PH_FRINFC_TOPAZ_STATE_READID                  4   /*!< Read Id under progress */
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
#define PH_FRINFC_TOPAZ_CC_BYTE2_MAX                0x0E /*!< Capability container byte 2 = 0x0E (Total free space
                                                            in the card) */
#define PH_FRINFC_TOPAZ_CC_BYTE3_RW                 0x00 /*!< Capability container byte 3 = 0x00 for
                                                                  READ WRITE/INITIALISED card state */
#define PH_FRINFC_TOPAZ_CC_BYTE3_RO                 0x0F /*!< Capability container byte 3 = 0x0F for
                                                                  READ only card state */

#define PH_FRINFC_TOPAZ_FLAG0                       0 /*!< Flag value = 0 */
#define PH_FRINFC_TOPAZ_FLAG1                       1 /*!< Flag value = 1 */

#define PH_FRINFC_TOPAZ_SHIFT3                      3 /*!< Shift by 3 bits */

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

#define PH_FRINFC_TOPAZ_NULL_T                   0x00 /*!< Null TLV value = 0x00 */
#define PH_FRINFC_TOPAZ_LOCK_CTRL_T              0x01 /*!< Lock TLV = 0x01 */
#define PH_FRINFC_TOPAZ_MEM_CTRL_T               0x02 /*!< Memory TLV = 0x02 */
#define PH_FRINFC_TOPAZ_NDEF_T                   0x03 /*!< NDEF TLV = 0x03 */
#define PH_FRINFC_TOPAZ_PROP_T                   0xFD /*!< NDEF TLV = 0xFD */
#define PH_FRINFC_TOPAZ_TERM_T                   0xFE /*!< Terminator TLV value = 0xFE */

#define PH_FRINFC_TOPAZ_NDEFTLV_L                0x00 /*!< Length value of TLV = 0x00 */
#define PH_FRINFC_TOPAZ_NDEFTLV_LFF              0xFF /*!< Length value of TLV = 0xFF */
#define PH_FRINFC_TOPAZ_MAX_CARD_SZ              0x60 /*!< Send Length for Read Ndef */

#define PH_FRINFC_TOPAZ_WR_A_BYTE                0x02 /*!< Send Length for Write Ndef */
#define PH_FRINFC_TOPAZ_SEND_BUF_READ            0x01 /*!< Send Length for Read Ndef */
#define PH_FRINFC_TOPAZ_HEADROM0_CHK             0xFF /*!< To check the header rom byte 0 */
#define PH_FRINFC_TOPAZ_HEADROM0_VAL             0x11 /*!< Header rom byte 0 value of static card */
#define PH_FRINFC_TOPAZ_READALL_RESP             0x7A /*!< Response of the read all command 122 bytes */
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
#define PH_FRINFC_TOPAZ_CC_READWRITE             0x00     /*! Lock status according to CC bytes */
#define PH_FRINFC_TOPAZ_CC_READONLY              0x0F     /*! Lock status according to CC bytes */

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
NFCSTATUS phFrinfc_Topaz_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize);

/**< This function shall calculate Topaz dynamic container size */
NFCSTATUS phFrinfc_TopazDynamic_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize);

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

#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_WRITE_COMPLETE            11  /*!< Write Operation Complete */
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_READ                      12
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_RD_CCBLK                  13
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_INIT_RD_CCBLK             14
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_INIT_WR                   15
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_WRITE_LEN                 16
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_FIND_NDEF_TLV             17
#define PH_FRINFC_TOPAZ_DYNAMI_FOUND_RESERV_AREA                18
#define PH_FRINFC_TOPAZ_DYNAMIC_NOT_FOUND_RESERV_AREA           19
#define PH_FRINFC_TOPAZ_DYNAMIC_PROCESS_CHK_NDEF                20
#define PH_FRINFC_TOPAZ_DYNAMIC_FIND_NDEF_TLV                   21
#define PH_FRINFC_TOPAZ_DYNAMIC_INIT_RD_NDEF                    22
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_WR_MEM_TLV                23
#define PH_FRINFC_TOPAZ_DYNAMIC_STATE_WR_LOCK_TLV               24

#define PH_FRINFC_TOPAZ_DYNAMIC_CC_BYTE2_MMSIZE                 0x3F  /*!< Capability container byte 2 = 0x3F (Total free space
                                                                        in the card) */
#define PH_FRINFC_TOPAZ_DYNAMIC_HEADROM0_VAL                    0x12  /*!< Header rom byte 0 value of dynamic card */

#define PH_FRINFC_TOPAZ_DYNAMIC_TOTAL_RWBYTES                   0x1CC /*!< Total number of raw Bytes that can
                                                                        be read or written to the card 460 bytes
                                                                        460 = 512 - 6 bloks * 8(48)( this includes 2 bytes of null byte in 02 block)
                                                                        - 4 bytes ( NDEF TLV )*/
#define PH_FRINFC_TOPAZ_DYNAMIC_MAX_CARD_SZ                     0x1E0 /*!< Card size */
#define PH_FRINFC_TOPAZ_DYNAMIC_MX_ONEBYTE_TLV_SIZE             0xFF  /*!< MAX size supported in one byte length TLV*/
#define PH_FRINFC_TOPAZ_DYNAMIC_MAX_DATA_SIZE_TO_WRITE          0xE6  /*!< MAX size supported by HAL if the data size > 255*/

#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_0                      0x00 /*!< lock bits value of byte 104 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_1                      0x00 /*!< lock bits value of byte 105 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBYTE_2TO7                   0x00 /*!< lock bits value of byte 105 */

#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_0                112  /*!< lock bits byte number 104:Blk0-7 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_1                113  /*!< lock bits byte number 105:Blk08-F */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_2                122  /*!< lock bits byte number 124:Blk10-17 */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_3                123  /*!< lock bits byte number 125:Blk18-1F */
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_4                124  /*!< lock bits byte number 126:Blk20-27*/
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_5                125  /*!< lock bits byte number 127:Blk28-2F*/
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_6                126  /*!< lock bits byte number 128:Blk30-37*/
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCKBIT_BYTENO_7                127  /*!< lock bits byte number 128:Blk30-37*/
#define PH_FRINFC_TOPAZ_DYNAMIC_CC_BYTENO_3                     11   /*! Lock status according to CC bytes */

#define PH_FRINFC_TOPAZ_DYNAMIC_SEGMENT0                        0x00  /*!< 00000000 : 0th segment */
#define PH_FRINFC_TOPAZ_DYNAMIC_READSEG_RESP                    0x80

#define PH_FRINFC_TOPAZ_DYNAMIC_MAX_BYTES_TO_READ_IN_ONEB_LTLV_FSEG             78
#define PH_FRINFC_TOPAZ_DYNAMIC_MAX_BYTES_TO_READ_IN_THREEB_LTLV_FSEG           76

#define PH_FRINFC_TOPAZ_DYNAMIC_MAX_DATA_SIZE                                   PHNFC_MAX_DATASIZE
#define PH_FRINFC_TOPAZ_DYNAMIC_FSEG_BYTE_COUNT                                 104
#define PH_FRINFC_TOPAZ_DYNAMIC_SEG_BYTE_COUNT                                  128
#define PH_FRINFC_TOPAZ_DYNAMIC_CC_BLK_SIZE                                     18
#define PH_FRINFC_TOPAZ_DYNAMIC_CC_BLK_ADDRESS                                  8
#define PH_FRINFC_TOPAZ_DYNAMIC_UID_BLK_ADDRESS                                 0
#define PH_FRINFC_TOPAZ_DYNAMIC_LOCK_BYTE_SIZE                                  24
#define PH_FRINFC_TOPAZ_DYNAMIC_FSEG_TOT_DATA_BYTES                             120

#define PH_FRINFC_TOPAZ_DYNAMIC_DATA_BYTE_COUNT_OF_FSEG_IN_ONEB_LTLV_FSEG       26
#define PH_FRINFC_TOPAZ_DYNAMIC_DATA_BYTE_COUNT_OF_FSEG_IN_THREEB_LTLV_FSEG     28

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
phFriNfc_TopazDynamicMap_ConvertToReadOnly (
    phFriNfc_NdefMap_t     *psNdefMap);

