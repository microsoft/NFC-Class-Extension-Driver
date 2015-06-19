/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

#include <phFriNfc.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc_NdefMap.h>

#define PH_FRINFC_NDEFMAP_DESF_READ_OP                              2  /*!< Desfire Operation Flag is Read */
#define PH_FRINFC_NDEFMAP_DESF_WRITE_OP                             3  /*!< Desfire Operation Flag is Write */
#define PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP                          4  /*!< Desfire Operation Flag is Check Ndef */
#define PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP                           5
#define PH_FRINFC_NDEFMAP_DESF_SET_LEN_OP                           6
#define PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET                          2  /*!< Two Status Flag at the end of the
                                                                            Receive buffer*/
#define PH_FRINFC_NDEFMAP_DESF_CAPDU_SMARTTAG_PKT_SIZE              12 /*!< Send Length for Smart Tag function*/
#define PH_FRINFC_NDEFMAP_DESF_CAPDU_SELECT_FILE_PKT_SIZE           7  /*!< Send Length for Select File function */
#define PH_FRINFC_NDEFMAP_DESF_CAPDU_READ_BIN_PKT_SIZE              5  /*!< Send Length for Reading a Packet */

#ifdef DESFIRE_EV1
    #define PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG_EV1       4     /*!< Selection of Smart Tag is going on for Desfire EV1 */
#endif /* #ifdef DESFIRE_EV1 */

#define PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG               5     /*!< Selection of Smart Tag is going on */
#define PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_FILE                    6     /*!< Selecting a file to read/write */
#define PH_FRINFC_NDEFMAP_DESF_STATE_READ_CAP_CONT                  7     /*!< Reading a capability container */
#define PH_FRINFC_NDEFMAP_DESF_STATE_READ_BIN                       8     /*!< Reading from the card */
#define PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_BEGIN               60    /*!< Writing to the card */
#define PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_END                 61    /*!< Writing to the card */

#define PH_FRINFC_NDEFMAP_DESF_STATE_CHK_NDEF                       10    /*!< Check Ndef is in progress */
#define PH_FRINFC_NDEFMAP_DESF_TLV_INDEX                            7     /*!< Specifies the index of TLV Structure */
#define PH_FRINFC_NDEFMAP_DESF_NDEF_CNTRL_TLV                       0x04  /*!< Specifies the NDEF File Cntrl TLV */
#define PH_FRINFC_NDEFMAP_DESF_PROP_CNTRL_TLV                       0x05  /*!< Specifies the Propreitary File Cntrl TLV */

/* Following Constants are used to navigate the Capability Container(CC)*/

/*!< Following two indexes represents the CCLEN in CC*/
#define PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_FIRST_INDEX               0
#define PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_SECOND_INDEX              1

/*!< Specifies the index of the Mapping Version in CC */
#define PH_FRINFC_NDEFMAP_DESF_VER_INDEX                            2

/*!< Following two indexes represents the MLe bytes in CC*/
#define PH_FRINFC_NDEFMAP_DESF_MLE_BYTE_FIRST_INDEX                 3
#define PH_FRINFC_NDEFMAP_DESF_MLE_BYTE_SECOND_INDEX                4

/*!< Following two indexes represents the MLc bytes in CC*/
#define PH_FRINFC_NDEFMAP_DESF_MLC_BYTE_FIRST_INDEX                 5
#define PH_FRINFC_NDEFMAP_DESF_MLC_BYTE_SECOND_INDEX                6

/*!< Specifies the index of the TLV in CC */
#define PH_FRINFC_NDEFMAP_DESF_TLV_INDEX                            7

/*!< Specifies the index of the TLV  length in CC */
#define PH_FRINFC_NDEFMAP_DESF_TLV_LEN_INDEX                        8

/*!< Following two indexes represents the NDEF file identifier in CC*/
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILEID_BYTE_FIRST_INDEX         9
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILEID_BYTE_SECOND_INDEX        10

/*!< Following two indexes represents the NDEF file size in CC */
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILESZ_BYTE_FIRST_INDEX         11
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILESZ_BYTE_SECOND_INDEX        12

/*!< Specifies the index of the NDEF file READ access byte in CC */
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILERD_ACCESS_INDEX             13

/*!< Specifies the index of the NDEF file WRITE access byte in CC */
#define PH_FRINFC_NDEFMAP_DESF_NDEF_FILEWR_ACCESS_INDEX             14


/* Macros to find Maximum NDEF File Size*/
#define PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE                       (NdefMap->DesfireCapContainer.NdefFileSize - 2)
/* Specifies the size of the NLEN Bytes*/
#define PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES                    2


/* Following constants are used with buffer index's*/
#define PH_FRINFC_NDEFMAP_DESF_SW1_INDEX            0
#define PH_FRINFC_NDEFMAP_DESF_SW2_INDEX            1


/* Following constants are used for SW1 SW2 status codes*/
#define PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE                    0x90
#define PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE                    0x00


/* Following constatnts for shift bytes*/
#define PH_FRINFC_NDEFMAP_DESF_SHL8                            8


#define PH_FRINFC_DESF_GET_VER_CMD                          0x60
#define PH_FRINFC_DESF_NATIVE_CLASS_BYTE                    0x90
#define PH_FRINFC_DESF_NATIVE_OFFSET_P1                     0x00
#define PH_FRINFC_DESF_NATIVE_OFFSET_P2                     0x00
#define PH_FRINFC_DESF_NATIVE_GETVER_RESP                   0xAF

typedef enum
{
    PH_FRINFC_DESF_STATE_GET_UID,
    PH_FRINFC_DESF_STATE_GET_SW_VERSION,
    PH_FRINFC_DESF_STATE_GET_HW_VERSION

}phFriNfc_eMapDesfireState;

typedef enum
{
    PH_FRINFC_DESF_IDX_0,
    PH_FRINFC_DESF_IDX_1,
    PH_FRINFC_DESF_IDX_2,
    PH_FRINFC_DESF_IDX_3,
    PH_FRINFC_DESF_IDX_4,
    PH_FRINFC_DESF_IDX_5

}phFriNfc_eMapDesfireId;

#define PH_FRINFC_DESF_ISO_NATIVE_WRAPPER() \
    do \
{\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_0] = PH_FRINFC_DESF_NATIVE_CLASS_BYTE;\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_2] = PH_FRINFC_DESF_NATIVE_OFFSET_P1;\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_3] = PH_FRINFC_DESF_NATIVE_OFFSET_P2;\
    switch(NdefMap->State)\
{\
    case PH_FRINFC_DESF_STATE_GET_HW_VERSION :\
    case PH_FRINFC_DESF_STATE_GET_SW_VERSION :\
    case PH_FRINFC_DESF_STATE_GET_UID :\
    if ( NdefMap->State == PH_FRINFC_DESF_STATE_GET_HW_VERSION  )\
{\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_1] = PH_FRINFC_DESF_GET_VER_CMD;\
}\
        else\
{\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_1] = 0xAF;\
}\
    NdefMap->SendRecvBuf[PH_FRINFC_DESF_IDX_4] = 0x00;\
    NdefMap->SendLength = PH_FRINFC_DESF_IDX_5;\
    break;\
    default :\
    break;\
}\
} while(0)\


NFCSTATUS
phFriNfc_Desfire_RdNdef(
    _Inout_                                                     phFriNfc_NdefMap_t  *NdefMap,
    _Out_writes_bytes_to_(*PacketDataLength, *PacketDataLength) uint8_t             *PacketData,
    _Inout_                                                     uint32_t            *PacketDataLength,
    _In_                                                        uint8_t             Offset
    );

NFCSTATUS
phFriNfc_Desfire_WrNdef(
    _Inout_                             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(*PacketDataLength) uint8_t             *PacketData,
    _Inout_                             uint32_t            *PacketDataLength,
    _In_                                uint8_t             Offset
    );

NFCSTATUS
phFriNfc_Desfire_ChkNdef(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

void
phFriNfc_Desfire_Process(
    _At_((phFriNfc_NdefMap_t*)Context, _Inout_) void        *Context,
    _In_                                        NFCSTATUS   Status
    );

NFCSTATUS
phFriNfc_DesfCapCont_HReset(
    _Inout_ phFriNfc_NdefMap_t      *NdefMap,
    _In_    phNfc_sDevInputParam_t  *pDevInpParam
    );

NFCSTATUS
phFrinfc_Desfire_GetContainerSize(
    _In_    const phFriNfc_NdefMap_t    *NdefMap,
    _Out_   uint32_t                    *maxSize,
    _Out_   uint32_t                    *actualSize
    );
