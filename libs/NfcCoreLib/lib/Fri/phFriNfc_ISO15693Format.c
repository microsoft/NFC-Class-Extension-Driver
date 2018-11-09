/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_ISO15693Format.tmh"

/* State for the format */
#define ISO15693_FORMAT                                 0x01U

/* Bytes per block in the ISO-15693 */
#define ISO15693_BYTES_PER_BLOCK                        0x04U

typedef enum phFriNfc_ISO15693_Command
{
    // Standard command set
    ISO15693_GET_SYSTEM_INFO_CMD = 0x2BU,
    ISO15693_RD_SINGLE_BLK_CMD = 0x20U,
    ISO15693_WR_SINGLE_BLK_CMD = 0x21U,
    ISO15693_RD_MULTIPLE_BLKS_CMD = 0x23U,
    // Extended command set
    ISO15693_EXT_GET_SYSTEM_INFO_CMD = 0x3BU
} phFriNfc_ISO15693_Command_t;

/* Capability Container (CC) */
/* CC BYTE 0 - Magic Number value indicating 1-byte address mode support - 0xE1 */
#define ISO15693_CC_MAGIC_NUM_E1                        0xE1U
/* CC BYTE 0 - Magic Number value indicating 2-byte address mode support - 0xE2 */
#define ISO15693_CC_MAGIC_NUM_E2                        0xE2U
/* CC BYTE 1 - Mapping version and READ WRITE settings 0x40 */
#define ISO15693_CC_VER_RW                              0x40U
/* CC BYTE 2 - max size is calaculated using the byte 3 multiplied by 8 */
#define ISO15693_CC_MULTIPLE_FACTOR                     0x08U
/* CC BYTE 3 - Additional feature information - no additional features by default (0) */
#define ISO15693_CC_ADDITIONAL_FEATURES_NONE            0

/* Inventory command support mask for the CC byte 4 */
#define ISO15693_INVENTORY_CMD_MASK                     0x02U
/* Read MULTIPLE blocks support mask for CC byte 4 */
#define ISO15693_RDMULBLKS_CMD_MASK                     0x01U
/* Flags for the command */
#define ISO15693_FMT_FLAGS                              0x20U

/* Read two blocks */
#define ISO15693_RD_2_BLOCKS                            0x02U

/* TYPE identifier of the NDEF TLV */
#define ISO15693_NDEF_TLV_TYPE_ID                       0x03U
/* Terminator TLV identifier  */
#define ISO15693_TERMINATOR_TLV_ID                      0xFEU

/* UID 7th byte value shall be 0xE0 */
#define ISO15693_7TH_BYTE_UID_VALUE                     0xE0U
#define ISO15693_BYTE_7_INDEX                           0x07U

#define ISO15693_EXTRA_RESPONSE_FLAG                    0x01U

/* GET_SYSTEM_INFO response format
 *bytes*| *description*
(mandatory)
  1     | Information flags
  8     | UID
(per flags)
  1     | DSFID
  1     | AFI
  2,3(*)| VICC memory size
  1     | IC reference
  1     | MOI (memory addressing)
  4     | VICC Commands list
  N     | CSI Information / ignored by NfcCx /

(*) - in case of EXTENDED_GET_SYSTEM_INFO command */
#define ISO15693_GET_SYS_INFO_RESP_MIN_LEN              9

/* GetExtSystemInfo Request full system info parameter flag */
#define ISO15693_EXT_GET_SYSTEM_INFO_ALL_INFO_FLAGS     0x7FU

/* Masks for Info Flags byte */
#define ISO15693_DSFID_MASK                             0x01U
#define ISO15693_AFI_MASK                               0x02U
#define ISO15693_VICC_MEMORY_SIZE_MASK                  0x04U
#define ISO15693_ICREF_MASK                             0x08U
#define ISO15693_CAPABILITY_MASK                        0x20U

/* Field lengths in bytes */
#define ISO15693_DFSID_FIELD_LEN                        1
#define ISO15693_AFI_FIELD_LEN                          1
#define ISO15693_VICC_MEMORY_SIZE_FIELD_LEN             2
#define ISO15693_ICREF_FIELD_LEN                        1
#define ISO15693_CAPABILITY_FIELD_LEN                   4

/* VICC Memory Size field format */
#define ISO15693_VICC_MEMORY_SIZE_FIELD_LEN_EXTENDED    3
#define ISO15693_BLOCK_SIZE_IN_BYTES_FIELD_LEN          1
#define ISO15693_BLOCK_SIZE_IN_BYTES_VALUE_MASK         0x1FU

#define ISO15693_CAPABILITY_MBREAD_MASK                 0x10U

/* MAXimum size of ICODE SLI/X */
#define ISO15693_SLI_X_MAX_SIZE                         112U
/* MAXimum size of ICODE SLI/X - S */
#define ISO15693_SLI_X_S_MAX_SIZE                       160U
/* MAXimum size of ICODE SLI/X - L */
#define ISO15693_SLI_X_L_MAX_SIZE                       32U

typedef enum phFriNfc_ISO15693_FormatSeq
{
    ISO15693_GET_SYS_INFO,
    ISO15693_EXT_GET_SYS_INFO,
    ISO15693_RD_SINGLE_BLK_CHECK,
    ISO15693_WRITE_CC_FMT,
    ISO15693_WRITE_CC_SECOND_BLOCK_FMT,
    ISO15693_WRITE_NDEF_TLV
}phFriNfc_ISO15693_FormatSeq_t;

static
NFCSTATUS
phFriNfc_ISO15693_H_ProFormat (
    phFriNfc_sNdefSmtCrdFmt_t   *psNdefSmtCrdFmt);

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessSystemInfo (
    phFriNfc_sNdefSmtCrdFmt_t   *psNdefSmtCrdFmt,
    uint8_t                     *p_recv_buf,
    uint16_t                    recv_length,
    bool_t                      is_cmd_extended);

static
NFCSTATUS
phFriNfc_ISO15693_H_FmtReadWrite (
    phFriNfc_sNdefSmtCrdFmt_t   *psNdefSmtCrdFmt,
    uint8_t                     command,
    uint8_t                     *p_data,
    uint8_t                     data_length);

static
NFCSTATUS
phFriNfc_ISO15693_H_FmtReadWrite (
    phFriNfc_sNdefSmtCrdFmt_t   *psNdefSmtCrdFmt,
    uint8_t                     command,
    uint8_t                     *p_data,
    uint8_t                     data_length)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    uint8_t                     send_index = 0;
    phHal_sIso15693Info_t       *ps_iso_15693_info =
                                &(psNdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);


    /* set the data for additional data exchange*/
    psNdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    psNdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.NADPresent = 0;
    psNdefSmtCrdFmt->psDepAdditionalInfo.NodeAddress = 0;

    psNdefSmtCrdFmt->SmtCrdFmtCompletionInfo.CompletionRoutine =
                                            phFriNfc_ISO15693_FmtProcess;
    psNdefSmtCrdFmt->SmtCrdFmtCompletionInfo.Context = psNdefSmtCrdFmt;

    *psNdefSmtCrdFmt->SendRecvLength = PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE;

    psNdefSmtCrdFmt->Cmd.Iso15693Cmd = phHal_eIso15693_Cmd;

    *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)ISO15693_FMT_FLAGS;
    if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_iso_15693_info->Uid))
    {
        *(psNdefSmtCrdFmt->SendRecvBuf + send_index) |= (uint8_t)ISO15693_FLAG_PROTOEXT;
    }
    send_index = (uint8_t)(send_index + 1);

    *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)command;
    send_index = (uint8_t)(send_index + 1);

    (void)phOsalNfc_MemCopy ((void *)(psNdefSmtCrdFmt->SendRecvBuf + send_index),
                             (void *)ps_iso_15693_info->Uid, ps_iso_15693_info->UidLength);
    send_index = (uint8_t)(send_index + ps_iso_15693_info->UidLength);

    switch (command)
    {
        case ISO15693_WR_SINGLE_BLK_CMD:
        case ISO15693_RD_MULTIPLE_BLKS_CMD:
        {
            *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)
                        psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block;
            send_index = (uint8_t)(send_index + 1);
            if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_iso_15693_info->Uid))
            {
                *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)
                            ((psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block & 0xFF00) >> 8);
                send_index = (uint8_t)(send_index + 1);
            }

            if (data_length)
            {
                (void)phOsalNfc_MemCopy ((void *)(psNdefSmtCrdFmt->SendRecvBuf + send_index),
                            (void *)p_data, data_length);
                send_index = (uint8_t)(send_index + data_length);
            }
            else
            {
                result = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
            }
            break;
        }

        case ISO15693_RD_SINGLE_BLK_CMD:
        {
            *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)
                        psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block;
            send_index = (uint8_t)(send_index + 1);
            if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_iso_15693_info->Uid))
            {
                *(psNdefSmtCrdFmt->SendRecvBuf + send_index) = (uint8_t)
                            ((psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block & 0xFF00)>> 8);
                send_index = (uint8_t)(send_index + 1);
            }
            break;
        }

        case ISO15693_GET_SYSTEM_INFO_CMD:
        {
            /* Dont do anything */
            break;
        }

        case ISO15693_EXT_GET_SYSTEM_INFO_CMD:
        {
            memmove(psNdefSmtCrdFmt->SendRecvBuf + 3, psNdefSmtCrdFmt->SendRecvBuf + 2,
                    psNdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.UidLength);
            if (data_length == 1)
            {
                (void)phOsalNfc_MemCopy((void *)(psNdefSmtCrdFmt->SendRecvBuf + 2),
                                                 (void *)p_data, data_length);
                send_index = (uint8_t)(send_index + data_length);
            }
            else
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                    NFCSTATUS_INVALID_DEVICE_REQUEST);
            }
            break;
        }

        default:
        {
            result = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT,
                                NFCSTATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }

    psNdefSmtCrdFmt->SendLength = send_index;

    if (!result)
    {
        result = phFriNfc_OvrHal_Transceive(psNdefSmtCrdFmt->LowerDevice,
                                            &psNdefSmtCrdFmt->SmtCrdFmtCompletionInfo,
                                            psNdefSmtCrdFmt->psRemoteDevInfo,
                                            psNdefSmtCrdFmt->Cmd,
                                            &psNdefSmtCrdFmt->psDepAdditionalInfo,
                                            psNdefSmtCrdFmt->SendRecvBuf,
                                            psNdefSmtCrdFmt->SendLength,
                                            psNdefSmtCrdFmt->SendRecvBuf,
                                            psNdefSmtCrdFmt->SendRecvLength);
    }

    return result;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessSystemInfo(
    phFriNfc_sNdefSmtCrdFmt_t   *psNdefSmtCrdFmt,
    uint8_t                     *p_recv_buf,
    uint16_t                    recv_length,
    bool_t                      is_cmd_extended
    )
{
    phFriNfc_ISO15693_AddInfo_t *ps_iso15693_info = &(psNdefSmtCrdFmt->AddInfo.s_iso15693_info);
    phHal_sIso15693Info_t *ps_rem_iso_15693_info = &(psNdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
    uint16_t min_expected_resp_len = ISO15693_GET_SYS_INFO_RESP_MIN_LEN;
    uint8_t recv_index = 0;
    /* VICC Memory size */
    uint16_t no_of_blocks = 0;
    uint8_t no_of_blocks_field_len = is_cmd_extended
                                     ? (ISO15693_VICC_MEMORY_SIZE_FIELD_LEN_EXTENDED - ISO15693_BLOCK_SIZE_IN_BYTES_FIELD_LEN)
                                     : (ISO15693_VICC_MEMORY_SIZE_FIELD_LEN - ISO15693_BLOCK_SIZE_IN_BYTES_FIELD_LEN);
    uint8_t blk_size_in_bytes = 0;
    /* Other fields */
    uint8_t information_flag = 0;
    uint8_t ic_reference = 0;

    information_flag = *p_recv_buf;
    recv_index += 1;

    /* Calculate response length per Information flags */
    if (information_flag & ISO15693_DSFID_MASK)
    {
        min_expected_resp_len += ISO15693_DFSID_FIELD_LEN;
    }

    if (information_flag & ISO15693_AFI_MASK)
    {
        min_expected_resp_len += ISO15693_AFI_FIELD_LEN;
    }

    if (information_flag & ISO15693_VICC_MEMORY_SIZE_MASK)
    {
        min_expected_resp_len += no_of_blocks_field_len + ISO15693_BLOCK_SIZE_IN_BYTES_FIELD_LEN;
    }

    if (information_flag & ISO15693_ICREF_MASK)
    {
        min_expected_resp_len += ISO15693_ICREF_FIELD_LEN;
    }

    if (information_flag & ISO15693_CAPABILITY_MASK)
    {
        min_expected_resp_len += ISO15693_CAPABILITY_FIELD_LEN;
    }

    /* Verify that buffer length is sufficient */
    if (recv_length < min_expected_resp_len
        || 0 != phOsalNfc_MemCompare(ps_rem_iso_15693_info->Uid,
            p_recv_buf + recv_index,
            ps_rem_iso_15693_info->UidLength))
    {
        return PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_DEVICE_REQUEST);
    }

    /* Skip UID memory */
    recv_index += ps_rem_iso_15693_info->UidLength;

    if (information_flag & ISO15693_DSFID_MASK)
    {
        /* Skip DFSID  */
        recv_index += ISO15693_DFSID_FIELD_LEN;
    }

    if (information_flag & ISO15693_AFI_MASK)
    {
        /* Skip AFI  */
        recv_index += ISO15693_AFI_FIELD_LEN;
    }

    if (information_flag & ISO15693_VICC_MEMORY_SIZE_MASK)
    {
        /* get total number of blocks on the card */
        phOsalNfc_MemCopy(&no_of_blocks,
                          p_recv_buf + recv_index,
                          no_of_blocks_field_len);
        no_of_blocks += 1;
        recv_index += no_of_blocks_field_len;

        /* get block size in bytes */
        blk_size_in_bytes = (p_recv_buf[recv_index] & ISO15693_BLOCK_SIZE_IN_BYTES_VALUE_MASK) + 1;
        recv_index += 1;
    }

    if (information_flag & ISO15693_ICREF_MASK)
    {
        ic_reference = p_recv_buf[recv_index];
        recv_index += ISO15693_ICREF_FIELD_LEN;
    }

    if (information_flag & ISO15693_CAPABILITY_MASK)
    {
        // TODO: fix according to field size
        ps_iso15693_info->card_capability = (uint8_t)(*(p_recv_buf + recv_index));
        recv_index += ISO15693_CAPABILITY_FIELD_LEN;
    }

    /* calculate maximum data size in the card */
    uint16_t detected_max_data_size = no_of_blocks * blk_size_in_bytes;

    /* If data about VICC Memory Size isn't available,
       try to detect it for known tags using IC Manufacturer data */
    if (0 == detected_max_data_size)
    {
        switch (ps_rem_iso_15693_info->Uid[ISO15693_UID_BYTE_6])
        {
        case ISO15693_MANUFACTURER_NXP:
        {
            if (ic_reference == 0x03)
            {
                no_of_blocks = 8;
                detected_max_data_size = no_of_blocks * blk_size_in_bytes;
            }
            break;
        }
        case ISO15693_MANUFACTURER_STM:
        {
            switch (ps_rem_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK)
            {
            case ISO15693_UIDBYTE_5_STM_LRIS64K:
                detected_max_data_size = ISO15693_STM_LRIS64K_MAX_SIZE;
                break;
            case ISO15693_UIDBYTE_5_STM_M24LR64R:
            case ISO15693_UIDBYTE_5_STM_M24LR64ER:
                detected_max_data_size = ISO15693_STM_M24LR64X_MAX_SIZE;
                break;
            case ISO15693_UIDBYTE_5_STM_M24LR16ER:
                detected_max_data_size = ISO15693_STM_M24LR16ER_MAX_SIZE;
                break;
            default:
                /* couldn't identify max size */
                break;
            }
            break;
        }
        default:
            break;
        }
    }

    ps_iso15693_info->max_data_size = detected_max_data_size;

    return
        (detected_max_data_size > 0)
        ? NFCSTATUS_SUCCESS
        : PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_DEVICE_REQUEST);
}

static
NFCSTATUS
phFriNfc_ISO15693_H_ProFormat (
    phFriNfc_sNdefSmtCrdFmt_t *psNdefSmtCrdFmt)
{
    NFCSTATUS                       result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693_AddInfo_t     *ps_iso15693_info =
                                    &(psNdefSmtCrdFmt->AddInfo.s_iso15693_info);
    phFriNfc_ISO15693_FormatSeq_t   e_format_seq =
                                    (phFriNfc_ISO15693_FormatSeq_t)
                                    ps_iso15693_info->format_seq;
    phHal_sIso15693Info_t           *ps_rem_iso_15693_info =
                                    &(psNdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
    uint8_t                         command_type = 0;
    uint8_t                         a_send_byte[ISO15693_BYTES_PER_BLOCK] = {0};
    uint8_t                         send_length = 0;
    uint8_t                         send_index = 0;
    uint8_t                         format_complete = FALSE;

    switch (e_format_seq)
    {
        case ISO15693_GET_SYS_INFO:
        {
            if (SUCCEEDED(phFriNfc_ISO15693_H_ProcessSystemInfo(psNdefSmtCrdFmt,
                                                                psNdefSmtCrdFmt->SendRecvBuf + ISO15693_EXTRA_RESPONSE_FLAG,
                                                                *psNdefSmtCrdFmt->SendRecvLength - ISO15693_EXTRA_RESPONSE_FLAG,
                                                                FALSE)))
            {
                /* If memory size was determined,
                   Send the READ SINGLE BLOCK command */
                command_type = ISO15693_RD_SINGLE_BLK_CMD;
                e_format_seq = ISO15693_RD_SINGLE_BLK_CHECK;

                /* Block number 0 to read */
                psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block = 0x00;
            }
            else
            {
                /* If memory size is still unknown,
                   Send the EXTENDED GET SYSTEM INFO command */
                command_type = ISO15693_EXT_GET_SYSTEM_INFO_CMD;
                e_format_seq = ISO15693_EXT_GET_SYS_INFO;
                *a_send_byte = (uint8_t)ISO15693_EXT_GET_SYSTEM_INFO_ALL_INFO_FLAGS;
                send_index = (uint8_t)(send_index + 1);
                send_length = 1;
            }
            break;
        }

        case ISO15693_EXT_GET_SYS_INFO:
        {
            if (SUCCEEDED(phFriNfc_ISO15693_H_ProcessSystemInfo(psNdefSmtCrdFmt,
                                                                psNdefSmtCrdFmt->SendRecvBuf + ISO15693_EXTRA_RESPONSE_FLAG,
                                                                *psNdefSmtCrdFmt->SendRecvLength - ISO15693_EXTRA_RESPONSE_FLAG,
                                                                FALSE)))
            {
                /* Send the READ SINGLE BLOCK COMMAND */
                command_type = ISO15693_RD_SINGLE_BLK_CMD;
                e_format_seq = ISO15693_RD_SINGLE_BLK_CHECK;

                /* Block number 0 to read */
                psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block = 0x00;
            }
            else
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_RECEIVE_LENGTH);
            }
            break;
        }

        case ISO15693_RD_SINGLE_BLK_CHECK:
        {
            /* RESPONSE received for READ SINGLE BLOCK received*/

            /* Check if Card is really fresh
               First 4 bytes must be 0 for fresh card */

            if ((psNdefSmtCrdFmt->AddInfo.s_iso15693_info.current_block == 0x00) &&
                (psNdefSmtCrdFmt->SendRecvBuf[1] != 0x00 ||
                 psNdefSmtCrdFmt->SendRecvBuf[2] != 0x00 ||
                 psNdefSmtCrdFmt->SendRecvBuf[3] != 0x00 ||
                 psNdefSmtCrdFmt->SendRecvBuf[4] != 0x00))
            {
                result = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_FORMAT);
            }
            else
            {
                /* prepare data for writing CC bytes */

                /* CC magic number */
                command_type = ISO15693_WR_SINGLE_BLK_CMD;
                if (ps_iso15693_info->max_data_size / ISO15693_BYTES_PER_BLOCK <= UCHAR_MAX + 1)
                {
                    e_format_seq = ISO15693_WRITE_CC_FMT;

                    *a_send_byte = (uint8_t)ISO15693_CC_MAGIC_NUM_E1;
                    send_index = (uint8_t)(send_index + 1);

                    /* CC Version and read/write access */
                    *(a_send_byte + send_index) = (uint8_t) ISO15693_CC_VER_RW;
                    send_index = (uint8_t)(send_index + 1);

                    /* CC MAX data size, calculated during GET system information */
                    *(a_send_byte + send_index) = (uint8_t)(ps_iso15693_info->max_data_size / ISO15693_CC_MULTIPLE_FACTOR);
                    send_index = (uint8_t)(send_index + 1);

                    switch (ps_iso15693_info->max_data_size)
                    {
                        case ISO15693_SLI_X_MAX_SIZE:
                        {
                            /* For SLI tags : Inventory Page read not supported */
                            *(a_send_byte + send_index) = (uint8_t) ISO15693_RDMULBLKS_CMD_MASK;
                            break;
                        }

                        case ISO15693_SLI_X_S_MAX_SIZE:
                        {
                            /* For SLI - S tags : Read multiple blocks not supported */
                            *(a_send_byte + send_index) = (uint8_t) ISO15693_INVENTORY_CMD_MASK;
                            break;
                        }

                        case ISO15693_SLI_X_L_MAX_SIZE:
                        {
                            /* For SLI - L tags : Read multiple blocks not supported */
                            *(a_send_byte + send_index) = (uint8_t) ISO15693_INVENTORY_CMD_MASK;
                            break;
                        }
                        default:
                        {
                            /* Generic tag: No additional features if tag type was not recognized  */
                            *(a_send_byte + send_index) = (uint8_t) ISO15693_CC_ADDITIONAL_FEATURES_NONE;
                            break;
                        }
                    }
                }
                else
                {
                    e_format_seq = ISO15693_WRITE_CC_SECOND_BLOCK_FMT;

                    *a_send_byte = (uint8_t)ISO15693_CC_MAGIC_NUM_E2;
                    send_index = (uint8_t)(send_index + 1);

                    /* CC Version and read/write access */
                    *(a_send_byte + send_index) = (uint8_t)ISO15693_CC_VER_RW;
                    send_index = (uint8_t)(send_index + 1);

                    *(a_send_byte + send_index) = 0x00;
                    send_index = (uint8_t)(send_index + 1);

                    /* Try best to use the tag capability from Get Extended System Info*/
                    if (ps_iso15693_info->card_capability & ISO15693_CAPABILITY_MBREAD_MASK)
                    {
                        *(a_send_byte + send_index) = ISO15693_CC_USE_MBR;
                    }
                }

                if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_rem_iso_15693_info->Uid))
                {
                    *(a_send_byte + send_index) |= (uint8_t)ISO15693_CC_MEM_EXCEEDED;
                    /* M24LRxxx does not support ISO15693 Get Extended system information command */
                    *(a_send_byte + send_index) |= (uint8_t)ISO15693_CC_USE_MBR;
                    /* Only M24LRIS64K does not support Multi byte read */
                    if ((ps_rem_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_LRIS64K)
                    {
                        *(a_send_byte + send_index) &= ~(ISO15693_CC_USE_MBR);
                    }
                }

                send_index = (uint8_t)(send_index + 1);

                send_length = sizeof (a_send_byte);
            }

            break;
        }

        case ISO15693_WRITE_CC_SECOND_BLOCK_FMT:
        {
            command_type = ISO15693_WR_SINGLE_BLK_CMD;
            e_format_seq = ISO15693_WRITE_CC_FMT;

            ps_iso15693_info->current_block = (uint16_t)
                (ps_iso15693_info->current_block + 1);

            *(a_send_byte + send_index) = 0x00;
            send_index = (uint8_t)(send_index + 1);

            *(a_send_byte + send_index) = 0x00;
            send_index = (uint8_t)(send_index + 1);

            /* CC MAX data size, calculated during GET system information */
            *(a_send_byte + send_index) = (uint8_t)((ps_iso15693_info->max_data_size / ISO15693_CC_MULTIPLE_FACTOR) >> 8);
            send_index = (uint8_t)(send_index + 1);

            *(a_send_byte + send_index) = (uint8_t)(ps_iso15693_info->max_data_size / ISO15693_CC_MULTIPLE_FACTOR);
            send_index = (uint8_t)(send_index + 1);

            send_length = sizeof(a_send_byte);
            break;
        }
        case ISO15693_WRITE_CC_FMT:
        {
            /* CC byte write succcessful.
            Prepare data for NDEF TLV writing */
            command_type = ISO15693_WR_SINGLE_BLK_CMD;
            e_format_seq = ISO15693_WRITE_NDEF_TLV;

            ps_iso15693_info->current_block = (uint16_t)
                        (ps_iso15693_info->current_block + 1);

            /* NDEF TLV - Type byte updated to 0x03 */
            *a_send_byte = (uint8_t)ISO15693_NDEF_TLV_TYPE_ID;
            send_index = (uint8_t)(send_index + 1);

            /* NDEF TLV - Length byte updated to 0 */
            *(a_send_byte + send_index) = 0;
            send_index = (uint8_t)(send_index + 1);

            /* Terminator TLV - value updated to 0xFEU */
            *(a_send_byte + send_index) = (uint8_t)
                            ISO15693_TERMINATOR_TLV_ID;
            send_index = (uint8_t)(send_index + 1);

            send_length = sizeof (a_send_byte);
            break;
        }

        case ISO15693_WRITE_NDEF_TLV:
        {
            /* SUCCESSFUL formatting complete */
            format_complete = TRUE;
            break;
        }

        default:
        {
            result = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT,
                                NFCSTATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }

    if ((!format_complete) && (!result))
    {
        result = phFriNfc_ISO15693_H_FmtReadWrite (psNdefSmtCrdFmt,
                            command_type, a_send_byte, send_length);
    }

    ps_iso15693_info->format_seq = (uint8_t)e_format_seq;
    return result;
}

void
phFriNfc_ISO15693_FmtReset (
    phFriNfc_sNdefSmtCrdFmt_t *psNdefSmtCrdFmt)
{
    /* reset to ISO15693 data structure */
    (void)phOsalNfc_SetMemory((void *)&(psNdefSmtCrdFmt->AddInfo.s_iso15693_info),
                0x00, sizeof (phFriNfc_ISO15693_AddInfo_t));
    psNdefSmtCrdFmt->FmtProcStatus = 0;
}

NFCSTATUS
phFriNfc_ISO15693_Format (
    phFriNfc_sNdefSmtCrdFmt_t *psNdefSmtCrdFmt)
{
    /* The last byte of UID must be 0xE0 if
       detected card is NDEF compliant */
    if (psNdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Uid[ISO15693_BYTE_7_INDEX] == ISO15693_7TH_BYTE_UID_VALUE)
    {
        psNdefSmtCrdFmt->State = ISO15693_FORMAT;

        /* GET system information command to get the card size */
        return phFriNfc_ISO15693_H_FmtReadWrite(psNdefSmtCrdFmt, ISO15693_GET_SYSTEM_INFO_CMD, NULL, 0);
    }

    return PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_DEVICE_REQUEST);
}

void
phFriNfc_ISO15693_FmtProcess (
    void        *pContext,
    NFCSTATUS   Status)
{
    phFriNfc_sNdefSmtCrdFmt_t      *psNdefSmtCrdFmt =
                                    (phFriNfc_sNdefSmtCrdFmt_t *)pContext;

    if((NFCSTATUS_SUCCESS & PHNFCSTBLOWER) == (Status & PHNFCSTBLOWER))
    {
        if (ISO15693_FORMAT == psNdefSmtCrdFmt->State)
        {
            /* Check for further formatting */
            Status = phFriNfc_ISO15693_H_ProFormat (psNdefSmtCrdFmt);
        }
        else
        {
            Status = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT,
                                NFCSTATUS_INVALID_DEVICE_REQUEST);
        }
    }
    else
    {
        Status = PHNFCSTVAL (CID_FRI_NFC_NDEF_SMTCRDFMT,
                            NFCSTATUS_FORMAT_ERROR);
    }

    /* Handle the all the error cases */
    if ((NFCSTATUS_PENDING & PHNFCSTBLOWER) != (Status & PHNFCSTBLOWER))
    {
        /* call respective CR */
        phFriNfc_SmtCrdFmt_HCrHandler (psNdefSmtCrdFmt, Status);
    }
}
