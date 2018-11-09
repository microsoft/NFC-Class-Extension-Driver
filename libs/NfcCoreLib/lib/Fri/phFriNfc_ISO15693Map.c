/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_ISO15693Map.tmh"

typedef enum phFriNfc_eChkNdefSeq
{
    ISO15693_NDEF_TLV_T,
    ISO15693_NDEF_TLV_L,
    ISO15693_NDEF_TLV_V,
    ISO15693_PROP_TLV_L,
    ISO15693_PROP_TLV_V

}phFriNfc_eChkNdefSeq_t;

typedef enum phFriNfc_eWrNdefSeq
{
    ISO15693_RD_BEFORE_WR_NDEF_L_0,
    ISO15693_WRITE_DATA,
    ISO15693_RD_BEFORE_WR_NDEF_L,
    ISO15693_WRITE_NDEF_TLV_L

}phFriNfc_eWrNdefSeq_t;

typedef enum phFriNfc_eRONdefSeq
{
    ISO15693_RD_BEFORE_WR_CC,
    ISO15693_WRITE_CC,
    ISO15693_LOCK_BLOCK

}phFriNfc_eRONdefSeq_t;

/* State Machine declaration */

typedef enum phFriNfc_NdefState
{
    ISO15693_CHECK_NDEF_FIRST_BLOCK,    /* CHECK NDEF state first block */
    ISO15693_CHECK_NDEF_SECOND_BLOCK,   /* CHECK NDEF state second block */
    ISO15693_READ_NDEF,                 /* READ NDEF state */
    ISO15693_WRITE_NDEF,                /* WRITE NDEF state */
    ISO15693_READ_ONLY_NDEF             /* READ ONLY NDEF state */
}phFriNfc_NdefState_t;

/* READ ONLY MASK byte for CC */
#define ISO15693_CC_READ_ONLY_MASK          0x03U
/* CC READ WRITE index */
#define ISO15693_RW_BTYE_INDEX              0x01U
/* LOCK BLOCK command */
#define ISO15693_LOCK_BLOCK_CMD             0x22U

/* EXTENDED NFC-FORUM T5T CMD are based with a 0x10 mask */
#define ISO15693_EXTENDED_CMD_MASK          0x10U

/* CC Bytes Magic numbers */
#define ISO15693_CC_MAGIC_BYTE_E1           0xE1U
#define ISO15693_CC_MAGIC_BYTE_E2           0xE2U
/* Expected mapping version */
#define ISO15693_MAPPING_VERSION            0x01U
/* Major version is in upper 2 bits */
#define ISO15693_MAJOR_VERSION_MASK         0xC0U

/* EXTRA byte in the response */
#define ISO15693_EXTRA_RESP_BYTE            0x01U

/* Maximum card size multiplication factor */
#define ISO15693_MULT_FACTOR                0x08U

/* NIBBLE mask for READ WRITE access */
#define ISO15693_LSB_NIBBLE_MASK            0x0FU
#define ISO15693_RD_WR_PERMISSION           0x00U
#define ISO15693_RD_ONLY_PERMISSION         0x03U

/* RESPONSE length expected for single block READ */
#define ISO15693_SINGLE_BLK_RD_RESP_LEN     0x04U
/* NULL TLV identifier */
#define ISO15693_NULL_TLV_ID                0x00U
/* NDEF TLV, TYPE identifier  */
#define ISO15693_NDEF_TLV_TYPE_ID           0x03U

/* 8 BIT shift */
#define ISO15693_BTYE_SHIFT                 0x08U

/* Proprietary TLV TYPE identifier */
#define ISO15693_PROP_TLV_ID                0xFDU

/* CC SIZE in BYTES */
#define ISO15693_CC_SIZE                    0x04U

/* To get the remaining size in the card.
Inputs are
1. maximum data size
2. block number
3. index of the block number */
#define ISO15693_GET_REMAINING_SIZE(max_data_size, blk, index) \
    (max_data_size - ((blk * ISO15693_BYTES_PER_BLOCK) + index))

#define ISO15693_GET_LEN_FIELD_BLOCK_NO(blk, byte_addr, ndef_size) \
    (((byte_addr + ((ndef_size >= ISO15693_THREE_BYTE_LENGTH_ID) ? 3 : 1)) > \
    (ISO15693_BYTES_PER_BLOCK - 1)) ? (blk + 1) : blk)

#define ISO15693_GET_LEN_FIELD_BYTE_NO(blk, byte_addr, ndef_size) \
    (((byte_addr + ((ndef_size >= ISO15693_THREE_BYTE_LENGTH_ID) ? 3 : 1)) % \
    ISO15693_BYTES_PER_BLOCK))

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessReadOnly (
    phFriNfc_NdefMap_t      *psNdefMap);

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessWriteNdef (
    phFriNfc_NdefMap_t      *psNdefMap);

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessReadNdef (
    phFriNfc_NdefMap_t      *psNdefMap);

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessCheckNdef (
    phFriNfc_NdefMap_t      *psNdefMap);

static
void
phFriNfc_ISO15693_H_Complete (
    phFriNfc_NdefMap_t      *psNdefMap,
    NFCSTATUS               Status);

static
NFCSTATUS
phFriNfc_ISO15693_H_ReadWrite (
    phFriNfc_NdefMap_t      *psNdefMap,
    uint8_t                 command,
    uint8_t                 *p_data,
    uint8_t                 data_length);

static
NFCSTATUS
phFriNfc_ReadRemainingInMultiple (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint32_t            startBlock);

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessWriteNdef (
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);
    phFriNfc_eWrNdefSeq_t       e_wr_ndef_seq = (phFriNfc_eWrNdefSeq_t)
                                psNdefMap->ISO15693Container.ndef_seq;
    uint8_t                     *p_recv_buf = NULL;
    uint8_t                     recv_length = 0;
    uint8_t                     write_flag = FALSE;
    uint8_t                     a_write_buf[ISO15693_BYTES_PER_BLOCK] = {0};
    uint8_t                     remaining_size = 0;

    switch (e_wr_ndef_seq)
    {
        case ISO15693_RD_BEFORE_WR_NDEF_L_0:
        {
            /* L byte is read  */
            p_recv_buf = (psNdefMap->SendRecvBuf + ISO15693_EXTRA_RESP_BYTE);
            recv_length = (uint8_t)
                        (*psNdefMap->SendRecvLength - ISO15693_EXTRA_RESP_BYTE);

            if (ISO15693_SINGLE_BLK_RD_RESP_LEN == recv_length)
            {
                /* Response length is correct */
                uint8_t     byte_index = 0;

                /* Copy the recevied buffer */
                (void)phOsalNfc_MemCopy ((void *)a_write_buf, (void *)p_recv_buf,
                                recv_length);

                byte_index = ISO15693_GET_LEN_FIELD_BYTE_NO(
                        ps_iso_15693_con->ndef_tlv_type_blk,
                        ps_iso_15693_con->ndef_tlv_type_byte,
                        psNdefMap->ApduBufferSize);

                /* Writing length field to 0, Update length field to 0 */
                *(a_write_buf + byte_index) = 0x00;

                if ((ISO15693_BYTES_PER_BLOCK - 1) != byte_index)
                {
                    /* User data is updated in the buffer */
                    byte_index = (uint8_t)(byte_index + 1);
                    /* Block number shall be udate */
                    remaining_size = (ISO15693_BYTES_PER_BLOCK - byte_index);

                    if ((psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex)
                        < remaining_size)
                    {
                        remaining_size = (uint8_t)(psNdefMap->ApduBufferSize -
                                                    psNdefMap->ApduBuffIndex);
                    }

                    /* Go to next byte to fill the write buffer */
                    (void)phOsalNfc_MemCopy ((void *)(a_write_buf + byte_index),
                                (void *)(psNdefMap->ApduBuffer +
                                psNdefMap->ApduBuffIndex), remaining_size);

                    /* Write index updated */
                    psNdefMap->ApduBuffIndex = (uint8_t)(psNdefMap->ApduBuffIndex +
                                                remaining_size);
                }

                /* After this write, user data can be written.
                Update the sequence accordingly */
                e_wr_ndef_seq = ISO15693_WRITE_DATA;
                write_flag = TRUE;
            } /* if (ISO15693_SINGLE_BLK_RD_RESP_LEN == recv_length) */
            else
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_RECEIVE_LENGTH);
            }
            break;
        } /* case ISO15693_RD_BEFORE_WR_NDEF_L_0: */

        case ISO15693_RD_BEFORE_WR_NDEF_L:
        {
            p_recv_buf = (psNdefMap->SendRecvBuf + ISO15693_EXTRA_RESP_BYTE);
            recv_length = (uint8_t)(*psNdefMap->SendRecvLength -
                            ISO15693_EXTRA_RESP_BYTE);

            if (ISO15693_SINGLE_BLK_RD_RESP_LEN == recv_length)
            {
                uint8_t     byte_index = 0;

                (void)phOsalNfc_MemCopy ((void *)a_write_buf, (void *)p_recv_buf,
                                recv_length);

                byte_index = ISO15693_GET_LEN_FIELD_BYTE_NO(
                                ps_iso_15693_con->ndef_tlv_type_blk,
                                ps_iso_15693_con->ndef_tlv_type_byte,
                                psNdefMap->ApduBuffIndex);

                *(a_write_buf + byte_index) = (uint8_t)psNdefMap->ApduBuffIndex;
                e_wr_ndef_seq = ISO15693_WRITE_NDEF_TLV_L;
                write_flag = TRUE;
            }
            else
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_RECEIVE_LENGTH);
            }
            break;
        }

        case ISO15693_WRITE_DATA:
        {
            if ((psNdefMap->ApduBufferSize == psNdefMap->ApduBuffIndex)
                || (ps_iso_15693_con->current_block ==
                    (ps_iso_15693_con->max_data_size / ISO15693_BYTES_PER_BLOCK)))
            {
                ps_iso_15693_con->current_block =
                        ISO15693_GET_LEN_FIELD_BLOCK_NO(
                        ps_iso_15693_con->ndef_tlv_type_blk,
                        ps_iso_15693_con->ndef_tlv_type_byte,
                        psNdefMap->ApduBuffIndex);
                e_wr_ndef_seq = ISO15693_RD_BEFORE_WR_NDEF_L;
            }
            else
            {
                remaining_size = ISO15693_BYTES_PER_BLOCK;

                ps_iso_15693_con->current_block = (uint16_t)
                                    (ps_iso_15693_con->current_block + 1);

                if ((psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex)
                    < remaining_size)
                {
                    remaining_size = (uint8_t)(psNdefMap->ApduBufferSize -
                                                psNdefMap->ApduBuffIndex);
                }

                (void)phOsalNfc_MemCopy ((void *)a_write_buf, (void *)
                                (psNdefMap->ApduBuffer +
                                psNdefMap->ApduBuffIndex), remaining_size);

                psNdefMap->ApduBuffIndex = (psNdefMap->ApduBuffIndex + remaining_size);
                write_flag = TRUE;
            }
            break;
        } /* case ISO15693_WRITE_DATA: */

        case ISO15693_WRITE_NDEF_TLV_L:
        {
            *psNdefMap->WrNdefPacketLength = psNdefMap->ApduBuffIndex;
            ps_iso_15693_con->actual_ndef_size = psNdefMap->ApduBuffIndex;
            break;
        }

        default:
        {
            break;
        }
    } /* switch (e_wr_ndef_seq) */

    if (((0 == psNdefMap->ApduBuffIndex)
        || (*psNdefMap->WrNdefPacketLength != psNdefMap->ApduBuffIndex))
        && (!result))
    {
        if (FALSE == write_flag)
        {
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                    ISO15693_READ_COMMAND, NULL, 0);
        }
        else
        {
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                        ISO15693_WRITE_COMMAND,
                                        a_write_buf, sizeof (a_write_buf));
        }
    }

    psNdefMap->ISO15693Container.ndef_seq = (uint8_t)e_wr_ndef_seq;
    return result;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_ReadWrite (
    phFriNfc_NdefMap_t      *psNdefMap,
    uint8_t                 command,
    uint8_t                 *p_data,
    uint8_t                 data_length)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    uint8_t                     send_index = 0;
    uint8_t                     request_flags = 0;
    phHal_sIso15693Info_t       *ps_iso_15693_info = 
                                &(psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con = &(psNdefMap->ISO15693Container);

    /* set the data for additional data exchange*/
    psNdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    psNdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;
    psNdefMap->psDepAdditionalInfo.NodeAddress = 0;

    psNdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_ISO15693_Process;
    psNdefMap->MapCompletionInfo.Context = psNdefMap;

    *psNdefMap->SendRecvLength = psNdefMap->TempReceiveLength;

    psNdefMap->Cmd.Iso15693Cmd = phHal_eIso15693_Cmd;

    if (ps_iso_15693_info->UidLength != 0)
    {
        request_flags |= ISO15693_FLAG_UID;
    }

    if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_iso_15693_info->Uid))
    {
        request_flags |= ISO15693_FLAG_PROTOEXT;
    }
    else if (ps_iso_15693_con->support_extended_cmd == TRUE)
    {
        /* Force extended command */
        command |= ISO15693_EXTENDED_CMD_MASK;
    }

    if (ISO15693_READ_MULTIPLE_COMMAND == command)
    {
        /* Force data_length to 1 as we are using ISO15693_READ_MULTIPLE_COMMAND */
        data_length = 1;
    }

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)request_flags;
    send_index = (uint8_t)(send_index + 1);

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)command;
    send_index = (uint8_t)(send_index + 1);

    if ((request_flags & ISO15693_FLAG_UID) != 0)
    {
        (void)phOsalNfc_MemCopy ((void *)(psNdefMap->SendRecvBuf + send_index),
            (void *)psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Uid,
            psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.UidLength);
        send_index = (uint8_t)(send_index +
                    psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.UidLength);
    }

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)
                                psNdefMap->ISO15693Container.current_block;
    send_index = (uint8_t)(send_index + 1);
    if ((request_flags & ISO15693_FLAG_PROTOEXT) != 0 ||
        (ISO15693_EXT_READ_MULTIPLE_COMMAND == command) ||
        (ISO15693_EXT_WRITE_COMMAND == command) ||
        (ISO15693_EXT_READ_COMMAND == command))
    {
        *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)
                                ((psNdefMap->ISO15693Container.current_block & 0xFF00) >> 8);
        send_index = (uint8_t)(send_index + 1);
    }

    if ((ISO15693_WRITE_COMMAND == command) ||
        (ISO15693_EXT_WRITE_COMMAND == command) ||
        (ISO15693_READ_MULTIPLE_COMMAND == command) ||
        (ISO15693_EXT_READ_MULTIPLE_COMMAND == command))
    {
        (void)phOsalNfc_MemCopy ((void *)(psNdefMap->SendRecvBuf + send_index),
                    (void *)p_data, data_length);
        send_index = (uint8_t)(send_index + data_length);
    }

    psNdefMap->SendLength = send_index;
    result = phFriNfc_OvrHal_Transceive(psNdefMap->LowerDevice,
                                        &psNdefMap->MapCompletionInfo,
                                        psNdefMap->psRemoteDevInfo,
                                        psNdefMap->Cmd,
                                        &psNdefMap->psDepAdditionalInfo,
                                        psNdefMap->SendRecvBuf,
                                        psNdefMap->SendLength,
                                        psNdefMap->SendRecvBuf,
                                        psNdefMap->SendRecvLength);

    return result;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_Inventory_Page_Read (
    phFriNfc_NdefMap_t      *psNdefMap,
    uint8_t                 command,
    uint32_t                 page,
    uint32_t                 numPages)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    uint8_t                     send_index = 0;
    uint8_t                     request_flags = 0;
    phHal_sIso15693Info_t       *ps_iso_15693_info =
                                &(psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);

    /* set the data for additional data exchange*/
    psNdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    psNdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;
    psNdefMap->psDepAdditionalInfo.NodeAddress = 0;

    psNdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_ISO15693_Process;
    psNdefMap->MapCompletionInfo.Context = psNdefMap;

    *psNdefMap->SendRecvLength = psNdefMap->TempReceiveLength;

    psNdefMap->Cmd.Iso15693Cmd = phHal_eIso15693_Cmd;

    if (ps_iso_15693_info->UidLength != 0)
    {
        request_flags |= ISO15693_FLAG_UID;
    }

    request_flags |= ISO15693_FLAG_IPR;

    *(psNdefMap->SendRecvBuf + send_index) = request_flags;
    send_index = (uint8_t)(send_index + 1);

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)command;
    send_index = (uint8_t)(send_index + 1);

    *(psNdefMap->SendRecvBuf + send_index) = ISO15693_MANUFACTURER_NXP;
    send_index = (uint8_t)(send_index + 1);

    *(psNdefMap->SendRecvBuf + send_index) = 0x40;
    send_index = (uint8_t)(send_index + 1);

    if ((request_flags & ISO15693_FLAG_UID) != 0)
    {
        (void)phOsalNfc_MemCopy ((void *)(psNdefMap->SendRecvBuf + send_index),
            (void *)psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Uid,
            psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.UidLength);
        send_index = (uint8_t)(send_index +
                    psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.UidLength);
    }

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)
                                page;
    send_index = (uint8_t)(send_index + 1);

    *(psNdefMap->SendRecvBuf + send_index) = (uint8_t)
                                numPages;
    send_index = (uint8_t)(send_index + 1);

    psNdefMap->SendLength = send_index;

    result = phFriNfc_OvrHal_Transceive(psNdefMap->LowerDevice,
                                        &psNdefMap->MapCompletionInfo,
                                        psNdefMap->psRemoteDevInfo,
                                        psNdefMap->Cmd,
                                        &psNdefMap->psDepAdditionalInfo,
                                        psNdefMap->SendRecvBuf,
                                        psNdefMap->SendLength,
                                        psNdefMap->SendRecvBuf,
                                        psNdefMap->SendRecvLength);

    return result;
}

static
uint16_t
phFriNfc_ISO15693_Reformat_Pageread_Buffer (
    uint8_t                 *p_recv_buf,
    uint16_t                 recv_length,
    uint8_t                 *p_dst_buf,
    uint16_t                 dst_length)
{
   // Inventory page reads return an extra security byte per page
   // So we need to reformat the returned buffer in memory
    uint32_t i = 0;
    uint16_t reformatted_index = 0;
    while (i < recv_length) {
        // Going for another page of 16 bytes, check for space in dst buffer
        if (reformatted_index + 16 > dst_length) {
            break;
        }
        if (p_recv_buf[i] == 0x0F) {
            // Security, insert 16 0 bytes
            phOsalNfc_SetMemory(&(p_dst_buf[reformatted_index]), 0, 16);
            reformatted_index += 16;
            i++;
        } else {
            // Skip security byte
            i++;
            if (i + 16 <= recv_length) {
                phOsalNfc_MemCopy(&(p_dst_buf[reformatted_index]), &(p_recv_buf[i]), 16);
                reformatted_index += 16;
            } else {
                break;
            }
            i+=16;
        }
    }
    return reformatted_index;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessReadNdef (
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);
    uint16_t                    remaining_data_size = 0;
    uint8_t                     *p_recv_buf =
                                (psNdefMap->SendRecvBuf + ISO15693_EXTRA_RESP_BYTE);
    uint16_t                     recv_length = (uint16_t)
                                (*psNdefMap->SendRecvLength - ISO15693_EXTRA_RESP_BYTE);
    uint8_t                     *reformatted_buf = NULL;
    uint16_t                    reformatted_size = 0;

    if (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR)
    {
        reformatted_buf = (uint8_t*) phOsalNfc_GetMemory(ps_iso_15693_con->max_data_size);
        reformatted_size = phFriNfc_ISO15693_Reformat_Pageread_Buffer(p_recv_buf, recv_length,
                reformatted_buf, ps_iso_15693_con->max_data_size);
        p_recv_buf = reformatted_buf + (ps_iso_15693_con->current_block * ISO15693_BYTES_PER_BLOCK);
        recv_length = reformatted_size - (ps_iso_15693_con->current_block * ISO15693_BYTES_PER_BLOCK);
    }

    if (ps_iso_15693_con->store_length)
    {
        /* Continue Offset option selected
            So stored data already existing,
            copy the information to the user buffer
        */
        if (ps_iso_15693_con->store_length
            <= (psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex))
        {
            /* Stored data length is less than or equal
                to the user expected size */
            (void)phOsalNfc_MemCopy ((void *)(psNdefMap->ApduBuffer +
                        psNdefMap->ApduBuffIndex),
                            (void *)ps_iso_15693_con->store_read_data,
                        ps_iso_15693_con->store_length);

            psNdefMap->ApduBuffIndex = (uint16_t)(psNdefMap->ApduBuffIndex +
                                        ps_iso_15693_con->store_length);

            remaining_data_size = ps_iso_15693_con->store_length;

            ps_iso_15693_con->store_length = 0;
        }
        else
        {
            /* stored length is more than the user expected size */
            remaining_data_size = (uint16_t)(ps_iso_15693_con->store_length -
                                (psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex));

            (void)phOsalNfc_MemCopy ((void *)(psNdefMap->ApduBuffer +
                        psNdefMap->ApduBuffIndex),
                        (void *)ps_iso_15693_con->store_read_data,
                        remaining_data_size);

            /* As stored data is more than the user expected data. So store
                the remaining bytes again into the data structure */
            (void)phOsalNfc_MemCopy ((void *)ps_iso_15693_con->store_read_data,
                        (void *)(ps_iso_15693_con->store_read_data +
                        remaining_data_size),
                        (ps_iso_15693_con->store_length - remaining_data_size));

            psNdefMap->ApduBuffIndex = (uint16_t)(psNdefMap->ApduBuffIndex +
                                        remaining_data_size);

                ps_iso_15693_con->store_length = (uint8_t)
                            (ps_iso_15693_con->store_length - remaining_data_size);
        }
    } /* if (ps_iso_15693_con->store_length) */
    else
    {
        /* Data is read from the card. */
        uint8_t                 byte_index = 0;

        remaining_data_size = ps_iso_15693_con->remaining_size_to_read;

        /* Check if the block number is to read the first VALUE field */
        if (ISO15693_GET_VALUE_FIELD_BLOCK_NO(ps_iso_15693_con->ndef_tlv_type_blk,
                                    ps_iso_15693_con->ndef_tlv_type_byte,
                                    ps_iso_15693_con->actual_ndef_size)
            == ps_iso_15693_con->current_block)
        {
            /* Read from the beginning option selected,
                BYTE number may start from the middle */
            byte_index = (uint8_t)ISO15693_GET_VALUE_FIELD_BYTE_NO(
                            ps_iso_15693_con->ndef_tlv_type_blk,
                            ps_iso_15693_con->ndef_tlv_type_byte,
                            ps_iso_15693_con->actual_ndef_size);
        }

        if ((psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex)
            < remaining_data_size)
        {
                remaining_data_size = (uint8_t)
                                    (recv_length - byte_index);
            /* user input is less than the remaining card size */
            if ((psNdefMap->ApduBufferSize - psNdefMap->ApduBuffIndex)
                    < (uint16_t)remaining_data_size)
            {
                /* user data required is less than the data read */
                remaining_data_size = (uint8_t)(psNdefMap->ApduBufferSize -
                                                psNdefMap->ApduBuffIndex);

                if (0 != (recv_length - (byte_index +
                                remaining_data_size)))
                {
                    /* Store the data for the continue read option */
                    (void)phOsalNfc_MemCopy ((void *)ps_iso_15693_con->store_read_data,
                                    (void *)(p_recv_buf + (byte_index +
                                            remaining_data_size)),
                                            (recv_length - (byte_index +
                                            remaining_data_size)));

                    ps_iso_15693_con->store_length = (uint8_t)
                                        (recv_length - (byte_index +
                                            remaining_data_size));
                }
            }
        }
        else
        {
            /* user data required is equal or greater than the data read */
            if (remaining_data_size > (recv_length - byte_index))
            {
                remaining_data_size = (recv_length - byte_index);
            }
        }

        /* Copy data in the user buffer */
        (void)phOsalNfc_MemCopy ((void *)(psNdefMap->ApduBuffer +
                        psNdefMap->ApduBuffIndex),
                        (void *)(p_recv_buf + byte_index),
                        remaining_data_size);

        /* Update the read index */
        psNdefMap->ApduBuffIndex = (uint16_t)(psNdefMap->ApduBuffIndex +
                                    remaining_data_size);

    } /* else part of if (ps_iso_15693_con->store_length) */

    /* Remaining size is decremented */
    ps_iso_15693_con->remaining_size_to_read = (ps_iso_15693_con->remaining_size_to_read -
                                                remaining_data_size);

    if ((psNdefMap->ApduBuffIndex != psNdefMap->ApduBufferSize)
        && (0 != ps_iso_15693_con->remaining_size_to_read))
    {
        ps_iso_15693_con->current_block = (uint16_t)
            (ps_iso_15693_con->current_block + (recv_length / ISO15693_BYTES_PER_BLOCK));
        /* READ again */
        if ((ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_MBR) ||
            (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR)) {
            result = phFriNfc_ReadRemainingInMultiple(psNdefMap, ps_iso_15693_con->current_block);
        }
        else {
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap, ISO15693_READ_COMMAND,
                                                    NULL, 0);
        }
    }
    else
    {
        /* Read completed, EITHER index has reached to the user size
        OR end of the card is reached
        update the user data structure with read data size */
        *psNdefMap->NumOfBytesRead = psNdefMap->ApduBuffIndex;
    }

    if (reformatted_buf != NULL) {
        phOsalNfc_FreeMemory(reformatted_buf);
    }
    return result;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_CheckCCBytesFirstBlock (
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS               result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t *ps_iso_15693_con =
                            &(psNdefMap->ISO15693Container);
    uint8_t                 recv_index = 0;
    uint8_t                 *p_recv_buf = (psNdefMap->SendRecvBuf + 1);
    phHal_sIso15693Info_t   *ps_iso_15693_info = 
                            &(psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
    uint8_t                  tag_major_version = 0;

    /* expected CC byte : E1 40 "MAX SIZE depends on tag" */
    if (ISO15693_CC_MAGIC_BYTE_E1 == *p_recv_buf || ISO15693_CC_MAGIC_BYTE_E2 == *p_recv_buf)
    {
        /*  magic byte found*/
        recv_index = (uint8_t)(recv_index + 1);
        tag_major_version = (*(p_recv_buf + recv_index) & ISO15693_MAJOR_VERSION_MASK) >> 6;
        if (ISO15693_MAPPING_VERSION >= tag_major_version)
        {
            /* Correct mapping version found */
            switch (*(p_recv_buf + recv_index) & ISO15693_LSB_NIBBLE_MASK)
            {
                case ISO15693_RD_WR_PERMISSION:
                {
                    /* READ/WRITE possible */
                    psNdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_WRITE;
                    break;
                }

                case ISO15693_RD_ONLY_PERMISSION:
                {
                    /* ONLY READ possible, WRITE NOT possible */
                    psNdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_ONLY;
                    break;
                }

                default:
                {
                    result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_NO_NDEF_SUPPORT);
                    break;
                }
            }
            recv_index = (uint8_t)(recv_index + 1);

            if (!result)
            {
                /* Update MAX SIZE */
                uint8_t cc2_value = *(p_recv_buf + recv_index);
                recv_index = (uint8_t)(recv_index + 1);
                ps_iso_15693_con->read_capabilities = (*(p_recv_buf + recv_index));

                if (cc2_value == 0)
                {
                    /* CC is 8 bytes long */
                    /* MLEN is stored in byte 6 and 7 of the CC */
                    ps_iso_15693_con->current_block = ps_iso_15693_con->current_block + 1;
                    /* State update - Read the next block for the T5T_Area */
                    psNdefMap->State = ISO15693_CHECK_NDEF_SECOND_BLOCK;

                    /* Start reading the data on the next block */
                    result = phFriNfc_ISO15693_H_ReadWrite(psNdefMap, ISO15693_READ_COMMAND,
                                                           NULL, 0);
                    return result;
                }
                ps_iso_15693_con->support_extended_cmd = FALSE;

                uint16_t max_data_size = (uint16_t)(cc2_value * ISO15693_MULT_FACTOR);

                if (ISO15693_MANUFACTURER_NXP != ps_iso_15693_info->Uid[ISO15693_UID_BYTE_6]) {
                    ps_iso_15693_con->read_capabilities &= ~(ISO15693_CC_USE_IPR);
                }

                if ((ps_iso_15693_con->read_capabilities & ISO15693_CC_MEM_EXCEEDED)) {
                    ps_iso_15693_con->read_capabilities &= ~(ISO15693_CC_USE_MBR);
                }

                if ((ps_iso_15693_con->read_capabilities & ISO15693_CC_MEM_EXCEEDED) && cc2_value == UCHAR_MAX) {
                    if (ISO15693_MANUFACTURER_STM == ps_iso_15693_info->Uid[ISO15693_UID_BYTE_6]) {
                        if ((ps_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_LRIS64K) /*LRIS64K*/
                            max_data_size = ISO15693_STM_LRIS64K_MAX_SIZE;
                        else if ((ps_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64R ||
                                 (ps_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64ER) /*M24LR64-X*/
                            max_data_size = ISO15693_STM_M24LR64X_MAX_SIZE; 
                        else if ((ps_iso_15693_info->Uid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR16ER) /*M24LR16E-R*/
                            max_data_size = ISO15693_STM_M24LR16ER_MAX_SIZE;
                    }
                }

                ps_iso_15693_con->max_data_size = max_data_size;
            }
        }
        else
        {
            result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_NO_NDEF_SUPPORT);
        }
    }
    else
    {
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_NO_NDEF_SUPPORT);
    }
    return result;
}

static
NFCSTATUS
phFriNfc_ISO15693_H_CheckCCBytesSecondBlock(
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS               result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t *ps_iso_15693_con = &(psNdefMap->ISO15693Container);
    uint8_t                 recv_index = 0;
    uint8_t                 *p_recv_buf = (psNdefMap->SendRecvBuf + 1);

    /* Update MAX SIZE coded on 2 bytes */
    recv_index = (uint8_t)(recv_index + 2);
    uint16_t cc2_value = ((uint16_t)*(p_recv_buf + recv_index)) << 8;
    recv_index = (uint8_t)(recv_index + 1);
    cc2_value |= *(p_recv_buf + recv_index);
    uint16_t max_data_size = (uint16_t)(cc2_value * ISO15693_MULT_FACTOR);
    ps_iso_15693_con->max_data_size = max_data_size;
    ps_iso_15693_con->support_extended_cmd = TRUE;

    return result;
}


static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessCheckNdef (
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS               result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t *ps_iso_15693_con =
                            &(psNdefMap->ISO15693Container);
    phFriNfc_eChkNdefSeq_t  e_chk_ndef_seq = (phFriNfc_eChkNdefSeq_t)
                            psNdefMap->ISO15693Container.ndef_seq;
    uint8_t                 *p_recv_buf =
                            (psNdefMap->SendRecvBuf + ISO15693_EXTRA_RESP_BYTE);
    uint16_t                 recv_length = (uint16_t)
                            (*psNdefMap->SendRecvLength - ISO15693_EXTRA_RESP_BYTE);
    uint8_t                 parse_index = 0;
    static uint16_t         prop_ndef_index = 0;
    uint8_t                 *reformatted_buf = NULL;
    uint16_t                reformatted_size = 0;

    if (0 == ps_iso_15693_con->current_block &&
        psNdefMap->State == ISO15693_CHECK_NDEF_FIRST_BLOCK)
    {
        /* Check CC byte */
        result = phFriNfc_ISO15693_H_CheckCCBytesFirstBlock (psNdefMap);
        parse_index = (uint8_t)(parse_index + recv_length);
    }
    else if (1 == ps_iso_15693_con->current_block &&
             psNdefMap->State == ISO15693_CHECK_NDEF_SECOND_BLOCK)
    {
        /* Retrieve the MBLEN information from the 8 byte CC*/
        result = phFriNfc_ISO15693_H_CheckCCBytesSecondBlock(psNdefMap);
        parse_index = (uint8_t)(parse_index + recv_length);
    }
    else if (1 == ps_iso_15693_con->current_block &&
            (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR))
    {
        reformatted_buf = (uint8_t*) phOsalNfc_GetMemory(ps_iso_15693_con->max_data_size);
        reformatted_size = phFriNfc_ISO15693_Reformat_Pageread_Buffer(p_recv_buf, recv_length,
                reformatted_buf, ps_iso_15693_con->max_data_size);

        // Skip initial CC bytes
        p_recv_buf = reformatted_buf + (ps_iso_15693_con->current_block * ISO15693_BYTES_PER_BLOCK);
        recv_length = reformatted_size - (ps_iso_15693_con->current_block * ISO15693_BYTES_PER_BLOCK);
    }
    else
    {
        /* Proprietary TLVs VALUE can end in between a block,
            so when that block is read, update the parse_index
            with byte address value */
        if (ISO15693_PROP_TLV_V == e_chk_ndef_seq)
        {
            parse_index = ps_iso_15693_con->ndef_tlv_type_byte;
            e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
        }
    }

    while ((parse_index < recv_length)
            && (NFCSTATUS_SUCCESS == result)
            && (ISO15693_NDEF_TLV_V != e_chk_ndef_seq))
    {
        /* Parse
            1. till the received length of the block
            2. till there is no error during parse
            3. till LENGTH field of NDEF TLV is found
        */
        switch (e_chk_ndef_seq)
        {
            case ISO15693_NDEF_TLV_T:
            {
                /* Expected value is 0x03 TYPE identifier
                    of the NDEF TLV */
                prop_ndef_index = 0;
                switch (*(p_recv_buf + parse_index))
                {
                    case ISO15693_NDEF_TLV_TYPE_ID:
                    {
                        /* Update the data structure with the byte address and
                        the block number */
                        ps_iso_15693_con->ndef_tlv_type_byte = parse_index;
                        ps_iso_15693_con->ndef_tlv_type_blk =
                                            ps_iso_15693_con->current_block;
                        e_chk_ndef_seq = ISO15693_NDEF_TLV_L;

                        break;
                    }

                    case ISO15693_NULL_TLV_ID:
                    {
                        /* Dont do any thing, go to next byte */
                        break;
                    }

                    case ISO15693_PROP_TLV_ID:
                    {
                        /* Move the sequence to find the length
                            of the proprietary TLV */
                        e_chk_ndef_seq = ISO15693_PROP_TLV_L;
                        break;
                    }

                    default:
                    {
                        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                            NFCSTATUS_NO_NDEF_SUPPORT);
                        break;
                    }
                } /* switch (*(p_recv_buf + parse_index)) */
                break;
            }

            case ISO15693_PROP_TLV_L:
            {
                /* Length field of the proprietary TLV */
                switch (prop_ndef_index)
                {
                    /* Length field can have 1 or 3 bytes depending
                        on the data size, so check for each index byte */
                    case 0:
                    {
                        /* 1st index of the length field of the TLV */
                        if (0 == *(p_recv_buf + parse_index))
                        {
                            /* LENGTH is 0, not possible, so error */
                            result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                                NFCSTATUS_NO_NDEF_SUPPORT);
                            e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
                        }
                        else
                        {
                            if (ISO15693_THREE_BYTE_LENGTH_ID ==
                                *(p_recv_buf + parse_index))
                            {
                                /* 3 byte LENGTH field identified, so increment the
                                index, so next time 2nd byte is parsed */
                                prop_ndef_index = (uint8_t)(prop_ndef_index + 1);
                            }
                            else
                            {
                                /* 1 byte LENGTH field identified, so "static"
                                index is set to 0 and actual ndef size is
                                copied to the data structure
                                */
                                ps_iso_15693_con->actual_ndef_size =
                                                    *(p_recv_buf + parse_index);
                                e_chk_ndef_seq = ISO15693_PROP_TLV_V;
                                prop_ndef_index = 0;
                            }
                        }
                        break;
                    }

                    case 1:
                    {
                        /* 2nd index of the LENGTH field that is MSB of the length,
                        so the length is left shifted by 8 */
                        ps_iso_15693_con->actual_ndef_size = (uint16_t)
                                        (*(p_recv_buf + parse_index) <<
                                        ISO15693_BTYE_SHIFT);
                        prop_ndef_index = (uint8_t)(prop_ndef_index + 1);
                        break;
                    }

                    case 2:
                    {
                        /* 3rd index of the LENGTH field that is LSB of the length,
                        so the length ORed with the previously stored size */
                        ps_iso_15693_con->actual_ndef_size = (uint16_t)
                                        (ps_iso_15693_con->actual_ndef_size |
                                        *(p_recv_buf + parse_index));

                        e_chk_ndef_seq = ISO15693_PROP_TLV_V;
                        prop_ndef_index = 0;
                        break;
                    }

                    default:
                    {
                        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                            NFCSTATUS_INVALID_DEVICE_REQUEST);
                        break;
                    }
                } /* switch (prop_ndef_index) */

                if ((ISO15693_PROP_TLV_V == e_chk_ndef_seq)
                    && (ISO15693_GET_REMAINING_SIZE(ps_iso_15693_con->max_data_size,
                        ps_iso_15693_con->current_block, parse_index)
                        <= ps_iso_15693_con->actual_ndef_size))
                {
                    /* Check for the length field value has not exceeded the card size,
                    if size is exceeded or then return error */
                    e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
                    result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_NO_NDEF_SUPPORT);
                }
                else
                {
                    uint16_t            prop_byte_addr = 0;

                    /* skip the proprietary TLVs value field */
                    prop_byte_addr = (uint16_t)
                        ((ps_iso_15693_con->current_block * ISO15693_BYTES_PER_BLOCK) +
                        parse_index + ps_iso_15693_con->actual_ndef_size);

                    ps_iso_15693_con->ndef_tlv_type_byte = (uint8_t)(prop_byte_addr %
                                                        ISO15693_BYTES_PER_BLOCK);
                    ps_iso_15693_con->ndef_tlv_type_blk = (uint16_t)(prop_byte_addr /
                                                        ISO15693_BYTES_PER_BLOCK);
                    if (parse_index + ps_iso_15693_con->actual_ndef_size >=
                        recv_length)
                    {
                        parse_index = (uint8_t)recv_length;
                    }
                    else
                    {
                        parse_index = (uint8_t)(parse_index +
                                        ps_iso_15693_con->actual_ndef_size);
                    }

                }
                break;
            } /* case ISO15693_PROP_TLV_L: */

            case ISO15693_PROP_TLV_V:
            {
                uint8_t         remaining_length = (uint8_t)(recv_length -
                                                    parse_index);

                if ((ps_iso_15693_con->actual_ndef_size - prop_ndef_index)
                    > remaining_length)
                {
                    parse_index = (uint8_t)(parse_index + remaining_length);
                    prop_ndef_index = (uint8_t)(prop_ndef_index + remaining_length);
                }
                else if ((ps_iso_15693_con->actual_ndef_size - prop_ndef_index)
                    == remaining_length)
                {
                    parse_index = (uint8_t)(parse_index + remaining_length);
                    e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
                    prop_ndef_index = 0;
                }
                else
                {
                    parse_index = (uint8_t)(parse_index +
                                            (ps_iso_15693_con->actual_ndef_size -
                                            prop_ndef_index));
                    e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
                    prop_ndef_index = 0;
                }
                break;
            } /* case ISO15693_PROP_TLV_V: */

            case ISO15693_NDEF_TLV_L:
            {
                /* Length field of the NDEF TLV */
                switch (prop_ndef_index)
                {
                    /* Length field can have 1 or 3 bytes depending
                        on the data size, so check for each index byte */
                    case 0:
                    {
                        /* 1st index of the length field of the TLV */
                        if (0 == *(p_recv_buf + parse_index))
                        {
                            /* LENGTH is 0, card is in INITILIASED STATE */
                            e_chk_ndef_seq = ISO15693_NDEF_TLV_V;
                            ps_iso_15693_con->actual_ndef_size = 0;
                        }
                        else
                        {
                            prop_ndef_index = (uint8_t)(prop_ndef_index + 1);

                            if (ISO15693_THREE_BYTE_LENGTH_ID ==
                                *(p_recv_buf + parse_index))
                            {
                                /* 3 byte LENGTH field identified */
                                /* next values are the DATA field of the NDEF TLV */
                            }
                            else
                            {
                                /* 1 byte LENGTH field identified, so "static"
                                index is set to 0 and actual ndef size is
                                copied to the data structure
                                */
                                ps_iso_15693_con->actual_ndef_size =
                                                    *(p_recv_buf + parse_index);
                                /* next values are the DATA field of the NDEF TLV */
                                e_chk_ndef_seq = ISO15693_NDEF_TLV_V;
                                prop_ndef_index = 0;
                            }
                        }
                        break;
                    }

                    case 1:
                    {
                        /* 2nd index of the LENGTH field that is MSB of the length,
                        so the length is left shifted by 8 */
                        ps_iso_15693_con->actual_ndef_size = (uint16_t)
                            (*(p_recv_buf + parse_index) <<
                            ISO15693_BTYE_SHIFT);
                        prop_ndef_index = (uint8_t)(prop_ndef_index + 1);
                        break;
                    }

                    case 2:
                    {
                        /* 3rd index of the LENGTH field that is LSB of the length,
                        so the length ORed with the previously stored size */
                        ps_iso_15693_con->actual_ndef_size = (uint16_t)
                            (ps_iso_15693_con->actual_ndef_size |
                            *(p_recv_buf + parse_index));

                        e_chk_ndef_seq = ISO15693_NDEF_TLV_V;
                        prop_ndef_index = 0;
                        break;
                    }

                    default:
                    {
                        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                        break;
                    }
                } /* switch (prop_ndef_index) */

                if ((ISO15693_NDEF_TLV_V == e_chk_ndef_seq)
                    && (ISO15693_GET_REMAINING_SIZE(ps_iso_15693_con->max_data_size,
                        /* parse_index + 1 is done because the data starts from the next index.
                        "MOD" operation is used to know that parse_index >
                        ISO15693_BYTES_PER_BLOCK, then block shall be incremented
                        */
                        (((parse_index + 1) % ISO15693_BYTES_PER_BLOCK) ?
                        ps_iso_15693_con->current_block :
                        ps_iso_15693_con->current_block + 1), ((parse_index + 1) %
                        ISO15693_BYTES_PER_BLOCK))
                        < ps_iso_15693_con->actual_ndef_size))
                {
                    /* Check for the length field value has not exceeded the card size */
                    e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
                    result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_NO_NDEF_SUPPORT);
                }
                else
                {
                    psNdefMap->CardState = (uint8_t)
                                    ((PH_NDEFMAP_CARD_STATE_READ_ONLY
                                    == psNdefMap->CardState) ?
                                    PH_NDEFMAP_CARD_STATE_READ_ONLY :
                                    ((ps_iso_15693_con->actual_ndef_size) ?
                                    PH_NDEFMAP_CARD_STATE_READ_WRITE :
                                    PH_NDEFMAP_CARD_STATE_INITIALIZED));
                }
                break;
            } /* case ISO15693_NDEF_TLV_L: */

            case ISO15693_NDEF_TLV_V:
            {
                break;
            }

            default:
            {
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                break;
            }
        } /* switch (e_chk_ndef_seq) */
        parse_index = (uint8_t)(parse_index + 1);
    } /* while ((parse_index < recv_length)
            && (NFCSTATUS_SUCCESS == result)
            && (ISO15693_NDEF_TLV_V != e_chk_ndef_seq)) */

    if (result)
    {
        /* Error returned while parsing, so STOP read */
        e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
        prop_ndef_index = 0;
    }
    else if (ISO15693_NDEF_TLV_V != e_chk_ndef_seq)
    {
        uint32_t remaining_size = 0;
        /* READ again */
        if (ISO15693_PROP_TLV_V != e_chk_ndef_seq)
        {
            ps_iso_15693_con->current_block = (uint16_t)
                                (ps_iso_15693_con->current_block + 1);
        }
        else
        {
            /* Proprietary TLV detected, so skip the proprietary blocks */
            ps_iso_15693_con->current_block = ps_iso_15693_con->ndef_tlv_type_blk;
        }

        remaining_size = ISO15693_GET_REMAINING_SIZE(ps_iso_15693_con->max_data_size,
                                           ps_iso_15693_con->current_block, 0);
        if (remaining_size > 0)
        {
            if ((ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_MBR) ||
                (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR)) {
                result = phFriNfc_ReadRemainingInMultiple(psNdefMap, ps_iso_15693_con->current_block);
            } else  {
                result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap, ISO15693_READ_COMMAND,
                                                        NULL, 0);
            }
        }
        else
        {
            /* End of card reached, error no NDEF information found */
            e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
            prop_ndef_index = 0;
            /* Error, no size to parse */
            result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_NO_NDEF_SUPPORT);
        }

    }
    else
    {
        /* Successful read with proper NDEF information updated */
        prop_ndef_index = 0;
        e_chk_ndef_seq = ISO15693_NDEF_TLV_T;
        psNdefMap->CardType = (uint8_t)PH_FRINFC_NDEFMAP_ISO15693_CARD;
    }

    psNdefMap->ISO15693Container.ndef_seq = (uint8_t)e_chk_ndef_seq;

    if (reformatted_buf != NULL) {
        phOsalNfc_FreeMemory(reformatted_buf);
    }
    return result;
}

static
void
phFriNfc_ISO15693_H_Complete (
    phFriNfc_NdefMap_t      *psNdefMap,
    NFCSTATUS               Status)
{
    /* set the state back to the RESET_INIT state*/
    psNdefMap->State =  PH_FRINFC_NDEFMAP_STATE_RESET_INIT;

    /* set the completion routine*/
    psNdefMap->CompletionRoutine[psNdefMap->ISO15693Container.cr_index].
        CompletionRoutine (psNdefMap->CompletionRoutine->Context, Status);
}

static
NFCSTATUS
phFriNfc_ISO15693_H_ProcessReadOnly (
    phFriNfc_NdefMap_t      *psNdefMap)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);
    phFriNfc_eRONdefSeq_t       e_ro_ndef_seq = (phFriNfc_eRONdefSeq_t)
                                ps_iso_15693_con->ndef_seq;
    uint8_t                     *p_recv_buf = (psNdefMap->SendRecvBuf +
                                ISO15693_EXTRA_RESP_BYTE);
    uint8_t                     recv_length = (uint8_t)(*psNdefMap->SendRecvLength -
                                ISO15693_EXTRA_RESP_BYTE);
    uint8_t                     a_write_buf[ISO15693_BYTES_PER_BLOCK] = {0};

    switch (e_ro_ndef_seq)
    {
        case ISO15693_RD_BEFORE_WR_CC:
        {
            if (ISO15693_SINGLE_BLK_RD_RESP_LEN == recv_length)
            {
                result = phFriNfc_ISO15693_H_CheckCCBytesFirstBlock (psNdefMap);
                /* Check CC bytes and also the card state for READ ONLY,
                if the card is already read only, then dont continue with
                next operation */
                if ((PH_NDEFMAP_CARD_STATE_READ_ONLY != psNdefMap->CardState)
                    && (!result))
                {
                    /* CC byte read successful */
                (void)phOsalNfc_MemCopy ((void *)a_write_buf, (void *)p_recv_buf,
                                sizeof (a_write_buf));

                    /* Change the read write access to read only */
                *(a_write_buf + ISO15693_RW_BTYE_INDEX) = (uint8_t)
                            (*(a_write_buf + ISO15693_RW_BTYE_INDEX) |
                            ISO15693_CC_READ_ONLY_MASK);

                result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                    ISO15693_WRITE_COMMAND, a_write_buf,
                                sizeof (a_write_buf));

                e_ro_ndef_seq = ISO15693_WRITE_CC;
            }
            }
            break;
        }

        case ISO15693_WRITE_CC:
        {
            /* Write to CC is successful. */
            e_ro_ndef_seq = ISO15693_LOCK_BLOCK;
            /* Start the lock block command to lock the blocks */
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                ISO15693_LOCK_BLOCK_CMD, NULL, 0);
            break;
        }

        case ISO15693_LOCK_BLOCK:
        {
            if (ps_iso_15693_con->current_block ==
                ((ps_iso_15693_con->max_data_size / ISO15693_BYTES_PER_BLOCK) -
                1))
            {
                /* End of card reached, READ ONLY successful */
            }
            else
            {
                /* current block is incremented */
                ps_iso_15693_con->current_block = (uint16_t)
                    (ps_iso_15693_con->current_block + 1);
                /* Lock the current block */
                result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                ISO15693_LOCK_BLOCK_CMD, NULL, 0);
            }
            break;
        }

        default:
        {
            break;
        }
    }

    ps_iso_15693_con->ndef_seq = (uint8_t)e_ro_ndef_seq;
    return result;
}

NFCSTATUS
phFriNfc_ISO15693_ChkNdef (
    phFriNfc_NdefMap_t  *psNdefMap)
{
    NFCSTATUS                       result = NFCSTATUS_SUCCESS;
    phHal_sIso15693Info_t           *ps_iso_15693_info =
                        &(psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);

    /* Update the previous operation with current operation.
        This becomes the previous operation after this execution */
    psNdefMap->PrevOperation = PH_FRINFC_NDEFMAP_CHECK_OPE;
    /* Update the CR index to know from which operation completion
        routine has to be called */
    psNdefMap->ISO15693Container.cr_index = PH_FRINFC_NDEFMAP_CR_CHK_NDEF;
    /* State update */
    psNdefMap->State = ISO15693_CHECK_NDEF_FIRST_BLOCK;
    /* Reset the NDEF sequence */
    psNdefMap->ISO15693Container.ndef_seq = 0;
    psNdefMap->ISO15693Container.current_block = 0;
    psNdefMap->ISO15693Container.actual_ndef_size = 0;
    psNdefMap->ISO15693Container.ndef_tlv_type_blk = 0;
    psNdefMap->ISO15693Container.ndef_tlv_type_byte = 0;
    psNdefMap->ISO15693Container.store_length = 0;
    psNdefMap->ISO15693Container.remaining_size_to_read = 0;
    psNdefMap->ISO15693Container.read_capabilities = 0;
    psNdefMap->ISO15693Container.support_extended_cmd = FALSE;

    if (ISO15693_UIDBYTE_7_VALUE ==
            ps_iso_15693_info->Uid[ISO15693_UID_BYTE_7])
    {
        switch (ps_iso_15693_info->Uid[ISO15693_UID_BYTE_6])
        {
            case ISO15693_MANUFACTURER_NXP:
            {
                /* Check if the card is manufactured by NXP (6th byte
                    index of UID value = 0x04 and the
                    last byte i.e., 7th byte of UID is 0xE0, only then the card detected
                    is NDEF compliant */
                switch (ps_iso_15693_info->Uid[ISO15693_UID_BYTE_5])
                {
                    /* Check for supported tags, by checking the 5th byte index of UID */
                    case ISO15693_UIDBYTE_5_VALUE_SLI_X:
                    {
                        /* ISO 15693 card type is ICODE SLI
                        so maximum size is 112 */
                        break;
                    }

                    case ISO15693_UIDBYTE_5_VALUE_SLI_X_S:
                    {
                        /* ISO 15693 card type is ICODE SLI/X S
                        so maximum size depends on the 4th UID byte index */
                        switch (ps_iso_15693_info->Uid[ISO15693_UID_BYTE_4])
                        {
                            case ISO15693_UIDBYTE_4_VALUE_SLI_X_S:
                            case ISO15693_UIDBYTE_4_VALUE_SLI_X_SHC:
                            case ISO15693_UIDBYTE_4_VALUE_SLI_X_SY:
                            {
                                /* Supported tags are with value (4th byte UID index)
                                of 0x00, 0x80 and 0x40
                                For these cards max size is 160 bytes */
                                break;
                            }

                            default:
                            {
                                /* Tag not supported */
                                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                                break;
                            }
                        }
                        break;
                    }

                    case ISO15693_UIDBYTE_5_VALUE_SLI_X_L:
                    {
                        /* ISO 15693 card type is ICODE SLI/X L
                        so maximum size depends on the 4th UID byte index */
                        switch (ps_iso_15693_info->Uid[ISO15693_UID_BYTE_4])
                        {
                            case ISO15693_UIDBYTE_4_VALUE_SLI_X_L:
                            case ISO15693_UIDBYTE_4_VALUE_SLI_X_LHC:
                            {
                                /* Supported tags are with value (4th byte UID index)
                                of 0x00 and 0x80
                                For these cards max size is 32 bytes */
                                break;
                            }

                            default:
                            {
                                /* Tag not supported */
                                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                                break;
                            }
                        }
                        break;
                    }

                    default:
                    {
                        /* Tag not supported */
                        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                            NFCSTATUS_INVALID_DEVICE_REQUEST);
                        break;
                    }
                }
                break;
            }

            case ISO15693_MANUFACTURER_STM:
            case ISO15693_MANUFACTURER_TI:
            {
                /* Max size will be populated
                        from the capability container */
                break;
            }

            default:
            {
                /* Tag not supported */
                result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                break;
            }
        }
    }
    else
    {
        /* Tag not supported */
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_INVALID_DEVICE_REQUEST);
    }

    if (!result)
    {
        /* Start reading the data */
        result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap, ISO15693_READ_COMMAND,
                                                NULL, 0);
    }


    return result;
}

NFCSTATUS
phFriNfc_ISO15693_RdNdef (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint8_t             *pPacketData,
    uint32_t            *pPacketDataLength,
    uint8_t             Offset)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);

    /* Update the previous operation with current operation.
        This becomes the previous operation after this execution */
    psNdefMap->PrevOperation = PH_FRINFC_NDEFMAP_READ_OPE;
    /* Update the CR index to know from which operation completion
        routine has to be called */
    psNdefMap->ISO15693Container.cr_index = PH_FRINFC_NDEFMAP_CR_RD_NDEF;
    /* State update */
    psNdefMap->State = ISO15693_READ_NDEF;
    /* Copy user buffer to the context */
    psNdefMap->ApduBuffer = pPacketData;
    /* Copy user length to the context */
    psNdefMap->ApduBufferSize = *pPacketDataLength;
    /* Update the user memory size to a context variable */
    psNdefMap->NumOfBytesRead = pPacketDataLength;
    /* Number of bytes read from the card is zero.
    This variable returns the number of bytes read
    from the card. */
    *psNdefMap->NumOfBytesRead = 0;
    /* Index to know the length read */
    psNdefMap->ApduBuffIndex = 0;
    /* Store the offset in the context */
    psNdefMap->Offset = Offset;

    if ((!ps_iso_15693_con->remaining_size_to_read)
        && (!psNdefMap->Offset))
    {
        /* Entire data is already read from the card.
        There is no data to give */
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    else if (0 == ps_iso_15693_con->actual_ndef_size)
    {
        /* Card is NDEF, but no data in the card. */
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_READ_FAILED);
    }
    else if (PH_NDEFMAP_CARD_STATE_INITIALIZED == psNdefMap->CardState)
    {
        /* Card is NDEF, but no data in the card. */
        result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_READ_FAILED);
    }
    else if (psNdefMap->Offset)
    {
        /* BEGIN offset, so reset the remaining read size and
        also the curretn block */
        ps_iso_15693_con->remaining_size_to_read =
                        ps_iso_15693_con->actual_ndef_size;
        ps_iso_15693_con->current_block =
                        ISO15693_GET_VALUE_FIELD_BLOCK_NO(
                        ps_iso_15693_con->ndef_tlv_type_blk,
                        ps_iso_15693_con->ndef_tlv_type_byte,
                        ps_iso_15693_con->actual_ndef_size);

        // Check capabilities
        if ((ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_MBR) ||
            (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR)) {
            result = phFriNfc_ReadRemainingInMultiple(psNdefMap, ps_iso_15693_con->current_block);
        } else  {
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap, ISO15693_READ_COMMAND,
                                                        NULL, 0);
        }
    }
    else
    {
        /* CONTINUE offset */
        if (ps_iso_15693_con->store_length > 0)
        {
            /* Previous read had extra bytes, so data is stored, so give that take
            that data from store. If more data is required, then read remaining bytes */
            result = phFriNfc_ISO15693_H_ProcessReadNdef (psNdefMap);
        }
        else
        {
            ps_iso_15693_con->current_block = (uint16_t)
                                (ps_iso_15693_con->current_block + 1);
            result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                            ISO15693_READ_COMMAND, NULL, 0);
        }
    }

    return result;
}

static
NFCSTATUS
phFriNfc_ReadRemainingInMultiple (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint32_t            startBlock)
{
    NFCSTATUS result = NFCSTATUS_FAILED;
    phFriNfc_ISO15693Cont_t *ps_iso_15693_con = &(psNdefMap->ISO15693Container);
    phHal_sIso15693Info_t   *ps_iso_15693_info =
                            &(psNdefMap->psRemoteDevInfo->RemoteDevInfo.Iso15693_Info);
    uint32_t remaining_size = ISO15693_GET_REMAINING_SIZE(ps_iso_15693_con->max_data_size,
                                           startBlock, 0);
    // Check capabilities
    if (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_MBR) {
        // Multi-page read command
        uint8_t mbread[2];
        uint8_t mbread_len = 1;
        uint32_t nb_blocks = 0;

        /* Compute how many block can be read at a time.
           If we are in NCI2.0 mode, MaxNFCVFrameSize is set and we need to split the packet to MaxNFCVFrameSize - 1.
           In case of M24LR tag, we can read 32 blocks maximum if they are all located in the same sector.
         */
        nb_blocks = ((remaining_size / ISO15693_BYTES_PER_BLOCK) - 1);
        if (ISO15693_PROTOEXT_FLAG_REQUIRED(ps_iso_15693_info->Uid))
        {
            if ((nb_blocks + (ps_iso_15693_con->current_block % ISO15693_STM_M24LR_MAX_BLOCKS_READ_PER_SECTOR)) >= ISO15693_STM_M24LR_MAX_BLOCKS_READ_PER_SECTOR - 1)
            {
                nb_blocks = ISO15693_STM_M24LR_MAX_BLOCKS_READ_PER_SECTOR - (ps_iso_15693_con->current_block % ISO15693_STM_M24LR_MAX_BLOCKS_READ_PER_SECTOR) - 1;
            }
            mbread_len = 2;
        }
        else
        {
            nb_blocks = (remaining_size / ISO15693_BYTES_PER_BLOCK) - 1;
            if (psNdefMap->MaxNFCVFrameSize > 0 &&
                nb_blocks > ((psNdefMap->MaxNFCVFrameSize / ISO15693_BYTES_PER_BLOCK) - 1))
            {
                nb_blocks = (psNdefMap->MaxNFCVFrameSize / ISO15693_BYTES_PER_BLOCK) - 1;
            }

            if (ps_iso_15693_con->support_extended_cmd == TRUE)
            {
                mbread_len = 2;
            }
        }

        mbread[0] = (uint8_t)nb_blocks;
        mbread[1] = (uint8_t)(nb_blocks >> 8);

        result = phFriNfc_ISO15693_H_ReadWrite(psNdefMap, ISO15693_READ_MULTIPLE_COMMAND,
                                               mbread, mbread_len);
    } else if (ps_iso_15693_con->read_capabilities & ISO15693_CC_USE_IPR) {
        uint32_t page = 0;
        uint32_t pagesToRead = (remaining_size / ISO15693_BYTES_PER_BLOCK / 4) - 1;
        if ((remaining_size % (ISO15693_BYTES_PER_BLOCK * ISO15693_BLOCKS_PER_PAGE)) != 0) {
            pagesToRead++;
        }
        result = phFriNfc_ISO15693_H_Inventory_Page_Read (psNdefMap, ICODE_INVENTORY_PAGEREAD_COMMAND,
                                                          page, pagesToRead);
        // Inventory
    } else  {
        result = NFCSTATUS_FAILED;
    }
    return result;
}

NFCSTATUS
phFriNfc_ISO15693_WrNdef (
    phFriNfc_NdefMap_t  *psNdefMap,
    uint8_t             *pPacketData,
    uint32_t            *pPacketDataLength,
    uint8_t             Offset)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);
    uint8_t                     a_write_buf[ISO15693_BYTES_PER_BLOCK] = {0};

    /* Update the previous operation with current operation.
        This becomes the previous operation after this execution */
    psNdefMap->PrevOperation = PH_FRINFC_NDEFMAP_WRITE_OPE;
    /* Update the CR index to know from which operation completion
        routine has to be called */
    psNdefMap->ISO15693Container.cr_index = PH_FRINFC_NDEFMAP_CR_WR_NDEF;
    /* State update */
    psNdefMap->State = ISO15693_WRITE_NDEF;
    /* Copy user buffer to the context */
    psNdefMap->ApduBuffer = pPacketData;
    /* Copy user length to the context */
    psNdefMap->ApduBufferSize = *pPacketDataLength;
    /* Update the user memory size to a context variable */
    psNdefMap->NumOfBytesRead = pPacketDataLength;
    /* Number of bytes written to the card is zero.
    This variable returns the number of bytes written
    to the card. */
    *psNdefMap->WrNdefPacketLength = 0;
    /* Index to know the length read */
    psNdefMap->ApduBuffIndex = 0;
    /* Store the offset in the context */
    psNdefMap->Offset = Offset;

    /* Set the current block correctly to write the length field to 0 */
    ps_iso_15693_con->current_block =
                        ISO15693_GET_LEN_FIELD_BLOCK_NO(
                        ps_iso_15693_con->ndef_tlv_type_blk,
                        ps_iso_15693_con->ndef_tlv_type_byte,
                        *pPacketDataLength);

    if (ISO15693_GET_LEN_FIELD_BYTE_NO(
                        ps_iso_15693_con->ndef_tlv_type_blk,
                        ps_iso_15693_con->ndef_tlv_type_byte,
                        *pPacketDataLength))
    {
        /* Check the byte address to write. If length byte address is in between or
        is the last byte of the block, then READ before write
        reason, write should not corrupt other data
        */
        ps_iso_15693_con->ndef_seq = (uint8_t)ISO15693_RD_BEFORE_WR_NDEF_L_0;
        result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                ISO15693_READ_COMMAND, NULL, 0);
    }
    else
    {
        /* If length byte address is at the beginning of the block then WRITE
        length field to 0 and as also write user DATA */
        ps_iso_15693_con->ndef_seq = (uint8_t)ISO15693_WRITE_DATA;

        /* Length is made 0x00 */
        *a_write_buf = 0x00;

        /* Write remaining data */
        (void)phOsalNfc_MemCopy ((void *)(a_write_buf + 1),
                        (void *)psNdefMap->ApduBuffer,
                        (ISO15693_BYTES_PER_BLOCK - 1));

        /* Write data */
        result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                    ISO15693_WRITE_COMMAND,
                                    a_write_buf, ISO15693_BYTES_PER_BLOCK);

        /* Increment the index to keep track of bytes sent for write */
        psNdefMap->ApduBuffIndex = (uint16_t)(psNdefMap->ApduBuffIndex
                                        + (ISO15693_BYTES_PER_BLOCK - 1));
    }

    return result;
}

NFCSTATUS
phFriNfc_ISO15693_ConvertToReadOnly (
    phFriNfc_NdefMap_t  *psNdefMap)
{
    NFCSTATUS                   result = NFCSTATUS_SUCCESS;
    phFriNfc_ISO15693Cont_t     *ps_iso_15693_con =
                                &(psNdefMap->ISO15693Container);

    psNdefMap->State = ISO15693_READ_ONLY_NDEF;
    /* READ CC bytes */
    ps_iso_15693_con->ndef_seq = (uint8_t)ISO15693_RD_BEFORE_WR_CC;
    ps_iso_15693_con->current_block = 0;

    result = phFriNfc_ISO15693_H_ReadWrite (psNdefMap,
                                    ISO15693_READ_COMMAND, NULL, 0);

    return result;
}


void
phFriNfc_ISO15693_Process (
    void        *pContext,
    NFCSTATUS   Status)
{
    phFriNfc_NdefMap_t      *psNdefMap =
                            (phFriNfc_NdefMap_t *)pContext;

    if ((NFCSTATUS_SUCCESS & PHNFCSTBLOWER) == (Status & PHNFCSTBLOWER))
    {
        switch (psNdefMap->State)
        {
            case ISO15693_CHECK_NDEF_FIRST_BLOCK:
            case ISO15693_CHECK_NDEF_SECOND_BLOCK:
            {
                /* State = CHECK NDEF in progress */
                Status = phFriNfc_ISO15693_H_ProcessCheckNdef(psNdefMap);
                break;
            }

            case ISO15693_READ_NDEF:
            {
                /* State = READ NDEF in progress */
                Status = phFriNfc_ISO15693_H_ProcessReadNdef (psNdefMap);
                break;
            }

            case ISO15693_WRITE_NDEF:
            {
                /* State = WRITE NDEF in progress */
                Status = phFriNfc_ISO15693_H_ProcessWriteNdef (psNdefMap);
                break;
            }

            case ISO15693_READ_ONLY_NDEF:
            {
                /* State = RAD ONLY NDEF in progress */
                Status = phFriNfc_ISO15693_H_ProcessReadOnly (psNdefMap);
                break;
            }

            default:
            {
                break;
            }
        }
    }

    /* Call for the Completion Routine*/
    if (NFCSTATUS_PENDING != Status)
    {
        phFriNfc_ISO15693_H_Complete(psNdefMap, Status);
    }
}

extern
NFCSTATUS phFrinfc_15693_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,
                                   uint32_t *maxSize, uint32_t *actualSize)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NULL != NdefMap) && (NULL != maxSize) &&(NULL != actualSize))
    {
        /*  15693 card */
        *maxSize = NdefMap->ISO15693Container.max_data_size;
        /* In Mifare UL card, the actual size is the length field
        value of the TLV */
        *actualSize = NdefMap->ISO15693Container.actual_ndef_size;
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                        NFCSTATUS_INVALID_PARAMETER);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}
