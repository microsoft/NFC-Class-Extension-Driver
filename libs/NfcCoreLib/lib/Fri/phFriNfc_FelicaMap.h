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

/* NDEF Mapping - states of the Finite State machine */
#define PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_WILD_CARD           1 /* Select Wild Card State*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_NDEF_APP            2 /* Select NFC Forum Application State*/
#define PH_FRINFC_NDEFMAP_FELI_STATE_CHK_NDEF                   3 /* Ndef Complient State*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR                    4 /* Read Attribute Information State*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_RD_BLOCK                   5 /* Read Data state*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_WR_BLOCK                   6 /* Write Data State*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN          7 /* Write Attrib Blk for write Begin*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_END            8 /* Write Attrib Blk for write End*/
#define PH_NFCFRI_NDEFMAP_FELI_STATE_WR_EMPTY_MSG               9 /* write Empty Ndef Msg*/


#define PH_NFCFRI_NDEFMAP_FELI_WR_RESP_BYTE                     0x09 /* Write Cmd Response Byte*/
#define PH_NFCFRI_NDEFMAP_FELI_RD_RESP_BYTE                     0x07 /* Read Cmd Response Byte*/

#define PH_NFCFRI_NDEFMAP_FELI_NMAXB                            13 /* Nmaxb Identifier*/
#define PH_NFCFRI_NDEFMAP_FELI_NBC                              14 /* Nbc Identifier*/

#define PH_FRINFC_NDEFMAP_FELI_OP_NONE                          15 /* To Read the attribute information*/
#define PH_FRINFC_NDEFMAP_FELI_WR_ATTR_RD_OP                    16 /* To Read the attribute info. while a WR Operationg*/
#define PH_FRINFC_NDEFMAP_FELI_RD_ATTR_RD_OP                    17 /* To Read the attribute info. while a RD Operationg*/
#define PH_FRINFC_NDEFMAP_FELI_CHK_NDEF_OP                      18 /* To Process the read attribute info. while a ChkNdef Operation*/
#define PH_FRINFC_NDEFMAP_FELI_WR_EMPTY_MSG_OP                  19 /* To Process the Empty NDEF Msg while erasing the NDEF data*/

#define PH_FRINFC_NDEFMAP_FELI_NUM_DEVICE_TO_DETECT             1

#define PH_NFCFRI_NDEFMAP_FELI_RESP_HEADER_LEN                  13 /* To skip response code, IDm, status flgas and Nb*/
#define PH_NFCFRI_NDEFMAP_FELI_VERSION_INDEX                    13 /* Specifies Index of the version in Attribute Resp Buffer*/
#define PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX                    0 /* Specifies Index of the Packet Length*/

/* NFC Device Major and Minor Version numbers*/
/* !!CAUTION!! these needs to be updated periodically.Major and Minor version numbers
   should be compatible to the version number of currently implemented mapping document.
    Example : NFC Device version Number : 1.0 , specifies
              Major VNo is 1,
              Minor VNo is 0 */
#define PH_NFCFRI_NDEFMAP_FELI_NFCDEV_MAJOR_VER_NUM             0x01
#define PH_NFCFRI_NDEFMAP_FELI_NFCDEV_MINOR_VER_NUM             0x00

/* Macros to find major and minor T3T version numbers*/
#define PH_NFCFRI_NDEFMAP_FELI_GET_MAJOR_T3T_VERNO(a)\
do\
{\
    (((a) & (0xf0))>>(4))\
}while (0)

#define PH_NFCFRI_NDEFMAP_FELI_GET_MINOR_T3T_VERNO(a)\
do\
{\
    ((a) & (0x0f))\
}while (0)

#define PRAGMA(X) __pragma(X)

/* Macro for LEN Byte Calculation*/
#define PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(Byte1,Byte2,Byte3,DataLen)\
  PRAGMA(warning(push))         \
  PRAGMA(warning(disable:4127)) \
do\
{ \
    (DataLen) = (Byte1); \
    (DataLen) = (DataLen) << (8);\
    (DataLen) += (Byte2);\
    (DataLen) = (DataLen) << (8);\
    (DataLen) += (Byte3);\
}while(0) \
  PRAGMA(warning(pop))


/* Enum for the data write operations*/
typedef enum
{
    FELICA_WRITE_STARTED,
    FELICA_WRITE_ENDED,
    FELICA_EOF_REACHED_WR_WITH_BEGIN_OFFSET,
    FELICA_EOF_REACHED_WR_WITH_CURR_OFFSET,
    FELICA_RD_WR_EOF_CARD_REACHED,
    FELICA_WRITE_EMPTY_MSG

}phFriNfc_FelicaError_t;


NFCSTATUS
phFriNfc_Felica_RdNdef(
    _Inout_                                                     phFriNfc_NdefMap_t  *NdefMap,
    _Out_writes_bytes_to_(*PacketDataLength, *PacketDataLength) uint8_t             *PacketData,
    _Inout_                                                     uint32_t            *PacketDataLength,
    _In_                                                        uint8_t             Offset
    );


NFCSTATUS
phFriNfc_Felica_WrNdef(
    _Inout_                             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(*PacketDataLength) uint8_t             *PacketData,
    _Inout_                             uint32_t            *PacketDataLength,
    _In_                                uint8_t             Offset
    );


NFCSTATUS
phFriNfc_Felica_EraseNdef(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             *pDummy1,
    _In_    uint32_t            *pDummy2
    );

NFCSTATUS
phFriNfc_Felica_ChkNdef(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

void
phFriNfc_Felica_Process(
    _At_((phFriNfc_NdefMap_t*)Context, _Inout_) void        *Context,
    _In_                                        NFCSTATUS   Status
    );

NFCSTATUS
phFriNfc_Felica_HReset(
    _Inout_ phFriNfc_NdefMap_t      *NdefMap,
    _In_    phNfc_sDevInputParam_t  *pDevInpParam
    );

NFCSTATUS
phFrinfc_Felica_GetContainerSize(
    _In_    const phFriNfc_NdefMap_t    *NdefMap,
    _Out_   uint32_t                    *maxSize,
    _Out_   uint32_t                    *actualSize
    );
