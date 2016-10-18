/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_TopazFormat.tmh"

#define PH_FRINFC_TOPAZ_STATE_WRITE_CC_BYTES         1 /*!< Write CC bytes */
#define PH_FRINFC_TOPAZ_STATE_WRITE_MEM_CTRL_BYTES   2 /*!< Write memory control bytes */
#define PH_FRINFC_TOPAZ_STATE_WRITE_TERM_TLV_BYTES   3 /*!< Write terminator TLV bytes */

static const uint8_t c_staticTopazFormat[] =
{
    PH_FRINFC_TOPAZ_CC_BYTE0, PH_FRINFC_TOPAZ_CC_BYTE1, PH_FRINFC_TOPAZ_CC_BYTE2_STATIC_MAX, PH_FRINFC_TOPAZ_CC_BYTE3_RW, /* CC bytes */
    PH_FRINFC_TOPAZ_TLV_NDEF_T, PH_FRINFC_TOPAZ_TLV_NDEF_L, /* NDEF TLV */
    PH_FRINFC_TOPAZ_TLV_TERM_T /* Terminator TLV */
};

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingCCBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt);

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingMemCtrlBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt);

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingTermTlvBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt);

static NFCSTATUS
phFriNfc_TopazFormat_WriteByte(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt,
                               uint8_t BlockNo,
                               uint8_t ByteNo,
                               uint8_t ByteToWrite);

static NFCSTATUS
phFriNfc_TopazFormat_Transceive(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt,
                                const uint8_t* SendBuffer,
                                uint16_t SendBufferLength);

NFCSTATUS
phFriNfc_TopazFormat_Format(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    if (NdefSmtCrdFmt == NULL)
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        NdefSmtCrdFmt->State = 0;

        phOsalNfc_SetMemory(&NdefSmtCrdFmt->AddInfo.Type1Info,
                            0,
                            sizeof(NdefSmtCrdFmt->AddInfo.Type1Info));

        NdefSmtCrdFmt->AddInfo.Type1Info.CurrentBlock = 0x01;
        NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte = 0;

        wStatus = phFriNfc_TopazFormat_WriteByte(NdefSmtCrdFmt,
                                                 NdefSmtCrdFmt->AddInfo.Type1Info.CurrentBlock,
                                                 NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte,
                                                 c_staticTopazFormat[NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte]);
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

void
phFriNfc_TopazFormat_Process(void* Context,
                             NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt = (phFriNfc_sNdefSmtCrdFmt_t*)Context;

    PH_LOG_NDEF_FUNC_ENTRY();

    if (NdefSmtCrdFmt == NULL)
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else if (wStatus == NFCSTATUS_SUCCESS)
    {
        if (*NdefSmtCrdFmt->SendRecvLength != 1 ||
            NdefSmtCrdFmt->SendRecvBuf[0] != c_staticTopazFormat[NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte])
        {
            PH_LOG_NDEF_WARN_STR("Unexpected response received. Formatting failed");
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_FORMAT_ERROR);
        }
        else
        {
            NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte++;
            if (NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte < ARRAYSIZE(c_staticTopazFormat))
            {
                wStatus = phFriNfc_TopazFormat_WriteByte(NdefSmtCrdFmt,
                                                         NdefSmtCrdFmt->AddInfo.Type1Info.CurrentBlock,
                                                         NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte,
                                                         c_staticTopazFormat[NdefSmtCrdFmt->AddInfo.Type1Info.CurrentByte]);
            }
        }
    }

    /* If status is not pending (i.e. we are waiting for a response from the card), then call the
    completion routine */
    if (wStatus != NFCSTATUS_PENDING)
    {
        phFriNfc_SmtCrdFmt_HCrHandler(NdefSmtCrdFmt, wStatus);
    }

    PH_LOG_NDEF_FUNC_EXIT();
}

NFCSTATUS
phFriNfc_TopazDynamicFormat_Format(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt)
{
    uint8_t write8Cmd[] =
    {
        PH_FRINFC_TOPAZ_CMD_WRITE_E8, /* WRITE-E8 */
        0x01, /* Block 0x1 */
        PH_FRINFC_TOPAZ_CC_BYTE0, PH_FRINFC_TOPAZ_CC_BYTE1, PH_FRINFC_TOPAZ_DYNAMIC_CC_BYTE2_MMSIZE, PH_FRINFC_TOPAZ_CC_BYTE3_RW, /* CC bytes */
        PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_T, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_L, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V0, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V1, /* Lock control bytes */
        0x00, 0x00, 0x00, 0x00 /* UID bytes (need to be copied over) */
    };

    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    if (NdefSmtCrdFmt == NULL)
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Copy the UID to the end of command buffer */
        phOsalNfc_MemCopy(&write8Cmd[10],
                          NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid,
                          TOPAZ_UID_LENGTH);

        NdefSmtCrdFmt->State = PH_FRINFC_TOPAZ_STATE_WRITE_CC_BYTES;
        wStatus = phFriNfc_TopazFormat_Transceive(NdefSmtCrdFmt,
                                                  write8Cmd,
                                                  sizeof(write8Cmd));
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

void
phFriNfc_TopazDynamicFormat_Process(void* Context,
                                    NFCSTATUS Status)
{
    NFCSTATUS wStatus = Status;
    phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt = (phFriNfc_sNdefSmtCrdFmt_t*)Context;

    PH_LOG_NDEF_FUNC_ENTRY();

    if (NdefSmtCrdFmt == NULL)
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_PARAMETER);
    }
    else if (wStatus == NFCSTATUS_SUCCESS)
    {
        switch (NdefSmtCrdFmt->State)
        {
        case PH_FRINFC_TOPAZ_STATE_WRITE_CC_BYTES:
            wStatus = phFriNfc_TopazDynamicFormat_ProcessWritingCCBytes(NdefSmtCrdFmt);
            break;
        case PH_FRINFC_TOPAZ_STATE_WRITE_MEM_CTRL_BYTES:
            wStatus = phFriNfc_TopazDynamicFormat_ProcessWritingMemCtrlBytes(NdefSmtCrdFmt);
            break;
        case PH_FRINFC_TOPAZ_STATE_WRITE_TERM_TLV_BYTES:
            wStatus = phFriNfc_TopazDynamicFormat_ProcessWritingTermTlvBytes(NdefSmtCrdFmt);
            break;
        default:
            wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }

    /* If status is not pending (i.e. we are waiting for a response from the card), then call the
       completion routine */
    if (wStatus != NFCSTATUS_PENDING)
    {
        phFriNfc_SmtCrdFmt_HCrHandler(NdefSmtCrdFmt, wStatus);
    }

    PH_LOG_NDEF_FUNC_EXIT();
}

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingCCBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt)
{
    static const uint8_t c_expectedResponse[] =
    {
        PH_FRINFC_TOPAZ_CC_BYTE0, PH_FRINFC_TOPAZ_CC_BYTE1, PH_FRINFC_TOPAZ_DYNAMIC_CC_BYTE2_MMSIZE, PH_FRINFC_TOPAZ_CC_BYTE3_RW, /* CC bytes */
        PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_T, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_L, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V0, PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V1, /* Lock control bytes */
    };

    uint8_t write8Cmd[] =
    {
        PH_FRINFC_TOPAZ_CMD_WRITE_E8, /* WRITE-E8 */
        0x02, /* Block 0x2 */
        PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V2, /* Lock control bytes */
        PH_FRINFC_TOPAZ_TLV_MEM_CTRL_T, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_L, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V0, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V1, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V2, /* Memory control bytes */
        PH_FRINFC_TOPAZ_TLV_NDEF_T, PH_FRINFC_TOPAZ_TLV_NDEF_L, /* NDEF TLV */
        0x00, 0x00, 0x00, 0x00 /* UID bytes (need to be copied over) */
    };

    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    assert(NdefSmtCrdFmt != NULL);

    if ((*NdefSmtCrdFmt->SendRecvLength != sizeof(c_expectedResponse)) ||
        (0 != phOsalNfc_MemCompare(NdefSmtCrdFmt->SendRecvBuf,
                                   (void*)c_expectedResponse,
                                   sizeof(c_expectedResponse))))
    {
        PH_LOG_NDEF_WARN_STR("Unexpected response received. Formatting failed");
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_FORMAT_ERROR);
    }
    else
    {
        /* Copy the UID to the end of command buffer */
        phOsalNfc_MemCopy(&write8Cmd[10],
                          NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid,
                          TOPAZ_UID_LENGTH);

        NdefSmtCrdFmt->State = PH_FRINFC_TOPAZ_STATE_WRITE_MEM_CTRL_BYTES;
        wStatus = phFriNfc_TopazFormat_Transceive(NdefSmtCrdFmt,
                                                  write8Cmd,
                                                  sizeof(write8Cmd));
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingMemCtrlBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt)
{
    static const uint8_t c_expectedResponse[] =
    {
        PH_FRINFC_TOPAZ_TLV_LOCK_CTRL_V2, /* Lock control bytes */
        PH_FRINFC_TOPAZ_TLV_MEM_CTRL_T, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_L, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V0, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V1, PH_FRINFC_TOPAZ_TLV_MEM_CTRL_V2, /* Memory control bytes */
        PH_FRINFC_TOPAZ_TLV_NDEF_T, PH_FRINFC_TOPAZ_TLV_NDEF_L, /* NDEF TLV */
    };

    uint8_t write8Cmd[] =
    {
        PH_FRINFC_TOPAZ_CMD_WRITE_E8, /* WRITE-E8 */
        0x03, /* Block 0x3 */
        PH_FRINFC_TOPAZ_TLV_TERM_T, /* Terminator TLV */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Rest of data bytes are zeros */
        0x00, 0x00, 0x00, 0x00 /* UID bytes (need to be copied over) */
    };

    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    assert(NdefSmtCrdFmt != NULL);

    if ((*NdefSmtCrdFmt->SendRecvLength != sizeof(c_expectedResponse)) ||
        (0 != phOsalNfc_MemCompare(NdefSmtCrdFmt->SendRecvBuf,
                                   (void*)c_expectedResponse,
                                   sizeof(c_expectedResponse))))
    {
        PH_LOG_NDEF_WARN_STR("Unexpected response received. Formatting failed");
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_FORMAT_ERROR);
    }
    else
    {
        /* Copy the UID to the end of command buffer */
        phOsalNfc_MemCopy(&write8Cmd[10],
                          NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid,
                          TOPAZ_UID_LENGTH);

        NdefSmtCrdFmt->State = PH_FRINFC_TOPAZ_STATE_WRITE_TERM_TLV_BYTES;
        wStatus = phFriNfc_TopazFormat_Transceive(NdefSmtCrdFmt,
                                                  write8Cmd,
                                                  sizeof(write8Cmd));
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phFriNfc_TopazDynamicFormat_ProcessWritingTermTlvBytes(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt)
{
    static const uint8_t c_expectedResponse[] =
    {
        PH_FRINFC_TOPAZ_TLV_TERM_T, /* Terminator TLV */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Rest of data bytes are zeros */
    };

    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    assert(NdefSmtCrdFmt != NULL);

    if ((*NdefSmtCrdFmt->SendRecvLength != sizeof(c_expectedResponse)) ||
        (0 != phOsalNfc_MemCompare(NdefSmtCrdFmt->SendRecvBuf,
                                   (void*)c_expectedResponse,
                                   sizeof(c_expectedResponse))))
    {
        PH_LOG_NDEF_WARN_STR("Unexpected response received. Formatting failed");
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, NFCSTATUS_FORMAT_ERROR);
    }
    else
    {
        PH_LOG_NDEF_INFO_STR("Successfully formatted dynamic Topaz card");
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phFriNfc_TopazFormat_WriteByte(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt,
                               uint8_t BlockNo,
                               uint8_t ByteNo,
                               uint8_t ByteToWrite)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    uint8_t writeCmd[7] = { 0 };

    PH_LOG_NDEF_FUNC_ENTRY();

    assert(NdefSmtCrdFmt != NULL);

    writeCmd[0] = PH_FRINFC_TOPAZ_CMD_WRITE_1E;

    /* Block and byte numbers need to be in the following format:
    7 | 6   5   4   3 | 2   1   0 |
    0 |     Block     |   Byte    |
    */
    writeCmd[1] = ((BlockNo & 0x0F) << 3) | (ByteNo & 0x07);

    writeCmd[2] = ByteToWrite;

    /* Copy the UID to the end of command buffer */
    phOsalNfc_MemCopy(&writeCmd[3],
                      NdefSmtCrdFmt->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid,
                      TOPAZ_UID_LENGTH);

    wStatus = phFriNfc_TopazFormat_Transceive(NdefSmtCrdFmt,
                                              writeCmd,
                                              sizeof(writeCmd));

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

static NFCSTATUS
phFriNfc_TopazFormat_Transceive(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt,
                                const uint8_t* SendBuffer,
                                uint16_t SendBufferLength)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();

    assert(NdefSmtCrdFmt != NULL);
    assert(SendBuffer != NULL);
    assert(SendBufferLength != 0 && SendBufferLength <= PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE);

    /* Fill the send buffer */
    NdefSmtCrdFmt->Cmd.JewelCmd = phNfc_eJewel_Raw;
    phOsalNfc_MemCopy(NdefSmtCrdFmt->SendRecvBuf, SendBuffer, SendBufferLength);
    NdefSmtCrdFmt->SendLength = SendBufferLength;

    /* Set the data for additional data exchange */
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.NADPresent = 0;
    NdefSmtCrdFmt->psDepAdditionalInfo.NodeAddress = 0;

    /* Set the completion routines for the card operations */
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.CompletionRoutine = phFriNfc_NdefSmtCrd_Process;
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.Context = NdefSmtCrdFmt;

    *NdefSmtCrdFmt->SendRecvLength = PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE;

    wStatus = phFriNfc_OvrHal_Transceive(NdefSmtCrdFmt->LowerDevice,
                                         &NdefSmtCrdFmt->SmtCrdFmtCompletionInfo,
                                         NdefSmtCrdFmt->psRemoteDevInfo,
                                         NdefSmtCrdFmt->Cmd,
                                         &NdefSmtCrdFmt->psDepAdditionalInfo,
                                         NdefSmtCrdFmt->SendRecvBuf,
                                         NdefSmtCrdFmt->SendLength,
                                         NdefSmtCrdFmt->SendRecvBuf,
                                         NdefSmtCrdFmt->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}
