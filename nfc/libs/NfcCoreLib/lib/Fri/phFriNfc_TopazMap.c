/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include "phFriNfc_Pch.h"

#include <phFriNfc_TopazMap.h>
#include <phFriNfc_MapTools.h>

#include "phFriNfc_TopazMap.tmh"

#define CC_BLOCK_NUMBER                                         (0x01U)
#define LOCK_BLOCK_NUMBER                                       (0x0EU)

#define LOCK0_BYTE_NUMBER                                       (0x00U)
#define LOCK0_BYTE_VALUE                                        (0xFFU)

#define LOCK1_BYTE_NUMBER                                       (0x01U)
#define LOCK1_BYTE_VALUE                                        (0x7FU)

#define CC_RWA_BYTE_NUMBER                                      (0x03U)
#define CC_READ_ONLY_VALUE                                      (0x0FU)


static NFCSTATUS phFriNfc_Tpz_H_RdBytes(phFriNfc_NdefMap_t *NdefMap,
                                        uint16_t             BlockNo,
                                        uint16_t             ByteNo);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function shall process
* read id command
*/
static NFCSTATUS phFriNfc_Tpz_H_ProReadID(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function shall process
* read all command
*/
static NFCSTATUS phFriNfc_Tpz_H_ProReadAll(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function depends on
* function called by the user
*/
static NFCSTATUS phFriNfc_Tpz_H_CallNxtOp(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function checks the CC
* bytes
*/
static NFCSTATUS phFriNfc_Tpz_H_ChkCCBytes(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function finds
* NDEF TLV
*/
static NFCSTATUS phFriNfc_Tpz_H_findNDEFTLV(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function writes a
* byte into the card
*/
static NFCSTATUS phFriNfc_Tpz_H_WrAByte(phFriNfc_NdefMap_t *NdefMap,
                                        uint16_t             BlockNo,
                                        uint16_t             ByteNo,
                                        uint8_t              ByteVal
                                        );

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function shall process the
* NMN write
*/
static NFCSTATUS phFriNfc_Tpz_H_ProWrNMN(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function writes the length field of
* the NDEF TLV
*/
static NFCSTATUS phFriNfc_Tpz_H_ProWrTLV(phFriNfc_NdefMap_t *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function updates length field
* of the NDEF TLV after complete write.
*/
static NFCSTATUS phFriNfc_Tpz_H_WrLByte(phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function copies the card data
*  to the user buffer
*/
static NFCSTATUS phFriNfc_Tpz_H_CpDataToUsrBuf( phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function shall process the
* written data
*/
static NFCSTATUS phFriNfc_Tpz_H_ProWrUsrData( phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function checks the block
* number is correct or not
*/
static void phFriNfc_Tpz_H_BlkChk(phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function writes the 0th
* byte of block 1 has Zero
*/
static NFCSTATUS phFriNfc_Tpz_H_WrByte0ValE1(phFriNfc_NdefMap_t   *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function calls the
* completion routine
*/
static void phFriNfc_Tpz_H_Complete(phFriNfc_NdefMap_t  *NdefMap,
                                    NFCSTATUS           Status);

/*!
* \brief \copydoc page_ovr Helper function for Topaz check ndef. This function checks
* the CC byte in check ndef function
*/
static NFCSTATUS phFriNfc_Tpz_H_ChkCCinChkNdef(phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz check ndef. This function checks
* the lock bits and set a card state
*/
static void phFriNfc_Tpz_H_ChkLockBits(phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function writes CC bytes or
* type of the TLV
*/
static NFCSTATUS phFriNfc_Tpz_H_WrCCorTLV(phFriNfc_NdefMap_t  *NdefMap);

/*!
* \brief \copydoc page_ovr Helper function for Topaz. This function checks the written
* CC bytes are correct
*/
static NFCSTATUS phFriNfc_Tpz_H_ProCCTLV(phFriNfc_NdefMap_t  *NdefMap);

NFCSTATUS phFriNfc_Topaz_H_Reset(phFriNfc_NdefMap_t *NdefMap,
                               phNfc_sDevInputParam_t *pDevInpParam)
{
    NFCSTATUS Result = NFCSTATUS_SUCCESS;
    UNUSED(pDevInpParam);
    PH_LOG_NDEF_FUNC_ENTRY();
    if(NdefMap == NULL)
    {
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Initialising the Topaz structure variable */
        NdefMap->TopazContainer.CRIndex = PH_FRINFC_NDEFMAP_CR_INVALID_OPE;
        NdefMap->TopazContainer.CurrentBlock = PH_FRINFC_TOPAZ_VAL1;
        NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL0;
        NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_VAL0;
        (void)phOsalNfc_SetMemory(NdefMap->TopazContainer.ReadBuffer, PH_FRINFC_TOPAZ_VAL0,
                    sizeof(NdefMap->TopazContainer.ReadBuffer));
        NdefMap->TopazContainer.ReadWriteCompleteFlag = PH_FRINFC_TOPAZ_FLAG0;
        NdefMap->TopazContainer.RemainingSize = PH_FRINFC_TOPAZ_VAL0;
        (void)phOsalNfc_SetMemory(NdefMap->TopazContainer.UID, PH_FRINFC_TOPAZ_VAL0,
                    sizeof(NdefMap->TopazContainer.UID));
        NdefMap->TopazContainer.Cur_RW_Index=0;
        NdefMap->TopazContainer.ByteRWFrmCard =0;
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

NFCSTATUS phFrinfc_Topaz_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NULL != NdefMap) && (NULL != maxSize) &&(NULL != actualSize))
    {
        /*  Topaz card */
        /*  The integration needs to ensure that the checkNdef
        function has been called before calling this function,
        otherwise NdefMap->CardMemSize will be 0 */
        *maxSize = NdefMap->TopazContainer.RemainingSize;
        /* In Topaz card, the actual size is the length field value of the
        TLV */
        *actualSize = NdefMap->TLVStruct.BytesRemainLinTLV;
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                        NFCSTATUS_INVALID_PARAMETER);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phFrinfc_TopazDynamic_GetContainerSize(const phFriNfc_NdefMap_t *NdefMap,\
                                   uint32_t *maxSize, uint32_t *actualSize)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NULL != NdefMap) && (NULL != maxSize) &&(NULL != actualSize))
    {
        /*  Topaz 512 card */
        /*  The integration needs to ensure that the checkNdef
        function has been called before calling this function,
        otherwise NdefMap->CardMemSize will be 0 */
        *maxSize = NdefMap->TopazContainer.NDEFRWSize;
        /* In Topaz card, the actual size is the length field value of the
        TLV */
        *actualSize = NdefMap->TopazContainer.ActualNDEFMsgSize;
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                        NFCSTATUS_INVALID_PARAMETER);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phFriNfc_TopazMap_ChkNdef( phFriNfc_NdefMap_t     *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_PARAMETER);
    if ( NdefMap != NULL)
    {
        /* Update the previous operation */
        NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_CHECK_OPE;
        /* Update the CR index to know from which operation completion
        routine has to be called */
        NdefMap->TopazContainer.CRIndex = PH_FRINFC_NDEFMAP_CR_CHK_NDEF;
        NdefMap->TopazContainer.CurrentBlock = PH_FRINFC_TOPAZ_VAL1;
        NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL0;

        /* Set card state */
        NdefMap->CardType = PH_FRINFC_NDEFMAP_TOPAZ_CARD;

        /* Change the state to Check Ndef Compliant */
        if(NdefMap->bDtaFlag)
        {
            /* TopazMap is not going to ask anyone for RID/UID and hence copy the lower layer info here */
            phOsalNfc_MemCopy(
                NdefMap->TopazContainer.UID,
                NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid,
                NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.UidLength);
        }

        NdefMap->State = PH_FRINFC_TOPAZ_STATE_READID;
        NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_CHECK_OPE;

        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] = PH_FRINFC_TOPAZ_CMD_READID;

        Result = phFriNfc_Tpz_H_RdBytes(NdefMap, NdefMap->TopazContainer.CurrentBlock,
            NdefMap->TopazContainer.ByteNumber);
    }
    return Result;
}

NFCSTATUS
phFriNfc_TopazMap_ConvertToReadOnly (
    phFriNfc_NdefMap_t          *NdefMap)
{
    NFCSTATUS               result = NFCSTATUS_SUCCESS;

    result = phFriNfc_Tpz_H_WrAByte (NdefMap, CC_BLOCK_NUMBER,
                                    CC_RWA_BYTE_NUMBER, CC_READ_ONLY_VALUE);

    if (NFCSTATUS_PENDING == PHNFCSTATUS(result))
    {
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_WR_CC_BYTE;
    }
    return result;
}

NFCSTATUS phFriNfc_TopazMap_RdNdef( phFriNfc_NdefMap_t                  *NdefMap,
                                        uint8_t                         *PacketData,
                                        uint32_t                        *PacketDataLength,
                                        uint8_t                         Offset)
{
    NFCSTATUS               Result =    NFCSTATUS_SUCCESS;

    /* Copy user buffer to the context */
    NdefMap->ApduBuffer = PacketData;
    /* Copy user length to the context */
    NdefMap->ApduBufferSize = *PacketDataLength;
    /* Update the user memory size to a context variable */
    NdefMap->NumOfBytesRead = PacketDataLength;
    /* Number of bytes read from the card is zero.
    This variable returns the number of bytes read
    from the card. */
    *NdefMap->NumOfBytesRead = PH_FRINFC_TOPAZ_VAL0;
    /* Index to know the length read */
    NdefMap->ApduBuffIndex = PH_FRINFC_TOPAZ_VAL0;
    /* Store the offset in the context */
    NdefMap->Offset = Offset;
    /* Update the CR index to know from which operation completion
    routine has to be called */
    NdefMap->TopazContainer.CRIndex = PH_FRINFC_NDEFMAP_CR_RD_NDEF;

    if( (Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN) || ( NdefMap->PrevOperation ==
        PH_FRINFC_NDEFMAP_WRITE_OPE))
    {
        /* If previous operation is not read then the read shall
        start from BEGIN */
        NdefMap->Offset = PH_FRINFC_NDEFMAP_SEEK_BEGIN;
        /* Initialise current block and byte number */
        NdefMap->TopazContainer.CurrentBlock = PH_FRINFC_TOPAZ_VAL1;
        NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL0;
        /* State has to be changed */
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_READALL;
        NdefMap->TopazContainer.ReadWriteCompleteFlag =
            PH_FRINFC_TOPAZ_FLAG0;
        /* Topaz command = READALL */
        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] = PH_FRINFC_TOPAZ_CMD_READALL;
    }

    NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_READ_OPE;
    /* Offset = Current, but the read has reached the End of Card */
    if( (Offset == PH_FRINFC_NDEFMAP_SEEK_CUR) &&
        (NdefMap->TopazContainer.ReadWriteCompleteFlag ==
        PH_FRINFC_TOPAZ_FLAG1))
    {
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    else
    {
        /* if the offset is begin then call READALL else copy the data
        from the user buffer */
        Result = ((Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN)?
            phFriNfc_Tpz_H_RdBytes(NdefMap,
            NdefMap->TopazContainer.CurrentBlock,
            NdefMap->TopazContainer.ByteNumber):
        phFriNfc_Tpz_H_CpDataToUsrBuf(NdefMap));
    }

    return Result;
}

NFCSTATUS phFriNfc_TopazMap_WrNdef( phFriNfc_NdefMap_t     *NdefMap,
                                   uint8_t                 *PacketData,
                                   uint32_t                *PacketDataLength,
                                   uint8_t                 Offset)
{
    NFCSTATUS                   Result =    NFCSTATUS_SUCCESS;
    uint8_t TempByteVal = 0;
    /* Copy user buffer to the context */
    NdefMap->ApduBuffer = PacketData;
    /* Copy user length to the context */
    NdefMap->ApduBufferSize = *PacketDataLength;
    /* Index to know the length written */
    NdefMap->ApduBuffIndex = PH_FRINFC_TOPAZ_VAL0;
    /* Update the user memory size to a context variable */
    NdefMap->WrNdefPacketLength = PacketDataLength;
    /* Number of bytes written to the card is zero.
    This variable returns the number of bytes written
    to the card. */
    *NdefMap->WrNdefPacketLength = PH_FRINFC_TOPAZ_VAL0;
    /* Update the CR index to know from which operation completion
    routine has to be called */
    NdefMap->TopazContainer.CRIndex = PH_FRINFC_NDEFMAP_CR_WR_NDEF;
    /* Store the offset in the context */
    NdefMap->Offset = Offset;


    if( (Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN) ||
        (NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_READ_OPE))
    {
        NdefMap->Offset = PH_FRINFC_NDEFMAP_SEEK_BEGIN;
        /* Initialise current block and byte number */
        NdefMap->TopazContainer.CurrentBlock = PH_FRINFC_TOPAZ_VAL1;
        NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL0;
        /* State has to be changed */
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_READALL;
        /* Topaz command = READALL */

        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] = PH_FRINFC_TOPAZ_CMD_READALL;

        NdefMap->TopazContainer.ReadWriteCompleteFlag =
            PH_FRINFC_TOPAZ_FLAG0;
        NdefMap->TopazContainer.RemainingSize = NdefMap->CardMemSize;
        TempByteVal = NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL1];
    }
    else
    {
        /* State has to be changed */
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE;
        /* copy the user data to write into the card */
        TempByteVal = NdefMap->ApduBuffer[NdefMap->ApduBuffIndex];
    }

    /* Update the previous operation to write operation */
    NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_WRITE_OPE;
    if((Offset == PH_FRINFC_NDEFMAP_SEEK_CUR) &&
        (NdefMap->TopazContainer.ReadWriteCompleteFlag ==
        PH_FRINFC_TOPAZ_FLAG1))
    {
        /* Offset = Current, but the read has reached the End of Card */
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    else
    {
        /* Check the block */
        phFriNfc_Tpz_H_BlkChk(NdefMap);
        /* if offset is begin then call READALL else start writing */
        Result = ((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN)?
            phFriNfc_Tpz_H_RdBytes(NdefMap, NdefMap->TopazContainer.CurrentBlock,
            NdefMap->TopazContainer.ByteNumber):
            phFriNfc_Tpz_H_WrAByte(NdefMap, NdefMap->TopazContainer.CurrentBlock,
            NdefMap->TopazContainer.ByteNumber,TempByteVal));
    }

    return Result;
}

void phFriNfc_TopazMap_Process( void       *Context,
                               NFCSTATUS   Status)
{

    phFriNfc_NdefMap_t              *psNdefMap = NULL;
    static uint8_t                  written_lock_byte = 0;

    psNdefMap = (phFriNfc_NdefMap_t *)Context;

    if ((Status & PHNFCSTBLOWER) == (NFCSTATUS_SUCCESS & PHNFCSTBLOWER))
    {
        switch (psNdefMap->State)
        {
            case PH_FRINFC_TOPAZ_STATE_WR_CC_BYTE:
            {
                if((CC_READ_ONLY_VALUE == *psNdefMap->SendRecvBuf)
                    && (PH_FRINFC_TOPAZ_VAL1 == *psNdefMap->SendRecvLength))
                {
                    written_lock_byte = 0;

                    *psNdefMap->SendRecvBuf = PH_FRINFC_TOPAZ_CMD_READ;

                    Status = phFriNfc_Tpz_H_RdBytes (psNdefMap, LOCK_BLOCK_NUMBER,
                                                    LOCK0_BYTE_NUMBER);

                    if (NFCSTATUS_PENDING == PHNFCSTATUS(Status))
                    {
                        psNdefMap->State = PH_FRINFC_TOPAZ_STATE_RD_LOCK0_BYTE;
                    }
                }
                else
                {
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_INVALID_RECEIVE_LENGTH);
                }
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_RD_LOCK0_BYTE:
            {
                if (PH_FRINFC_TOPAZ_VAL1 == *psNdefMap->SendRecvLength)
                {
                    Status = phFriNfc_Tpz_H_WrAByte (psNdefMap, LOCK_BLOCK_NUMBER,
                                                LOCK0_BYTE_NUMBER,
                                                LOCK0_BYTE_VALUE);

                    if (NFCSTATUS_PENDING == PHNFCSTATUS(Status))
                    {
                        psNdefMap->State = PH_FRINFC_TOPAZ_STATE_WR_LOCK0_BYTE;
                    }
                }
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_WR_LOCK0_BYTE:
            {
                if((LOCK0_BYTE_VALUE == *psNdefMap->SendRecvBuf)
                    && (PH_FRINFC_TOPAZ_VAL1 == *psNdefMap->SendRecvLength))
                {
                    *psNdefMap->SendRecvBuf = PH_FRINFC_TOPAZ_CMD_READ;

                    Status = phFriNfc_Tpz_H_RdBytes (psNdefMap, LOCK_BLOCK_NUMBER,
                                                    LOCK1_BYTE_NUMBER);

                    if (NFCSTATUS_PENDING == PHNFCSTATUS(Status))
                    {
                        psNdefMap->State = PH_FRINFC_TOPAZ_STATE_RD_LOCK1_BYTE;
                    }
                }
                else
                {
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_INVALID_RECEIVE_LENGTH);
                }
            }

            case PH_FRINFC_TOPAZ_STATE_RD_LOCK1_BYTE:
            {
                if (PH_FRINFC_TOPAZ_VAL1 == *psNdefMap->SendRecvLength)
                {
                    written_lock_byte = (uint8_t)(*psNdefMap->SendRecvBuf | LOCK1_BYTE_VALUE);
                    Status = phFriNfc_Tpz_H_WrAByte (psNdefMap, LOCK_BLOCK_NUMBER,
                                                LOCK1_BYTE_NUMBER,
                                                written_lock_byte);

                    if (NFCSTATUS_PENDING == PHNFCSTATUS(Status))
                    {
                        psNdefMap->State = PH_FRINFC_TOPAZ_STATE_WR_LOCK1_BYTE;
                    }
                }
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_WR_LOCK1_BYTE:
            {
                if((written_lock_byte == *psNdefMap->SendRecvBuf)
                    && (PH_FRINFC_TOPAZ_VAL1 == *psNdefMap->SendRecvLength))
                {
                    written_lock_byte = 0;
                    /* Do nothing */
                }
                else
                {
                    written_lock_byte = 0;
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_INVALID_RECEIVE_LENGTH);
                }
            }
            break;

            case PH_FRINFC_TOPAZ_STATE_WRITE:
            {
                Status = phFriNfc_Tpz_H_ProWrUsrData (psNdefMap);
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_READID:
            {
                Status = phFriNfc_Tpz_H_ProReadID (psNdefMap);
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_READALL:
            {
                Status = phFriNfc_Tpz_H_ProReadAll (psNdefMap);
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_WRITE_NMN:
            {
                Status = phFriNfc_Tpz_H_ProWrNMN (psNdefMap);
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_WRITE_L_TLV:
            {
                Status = phFriNfc_Tpz_H_ProWrTLV (psNdefMap);
                break;
            }

            case PH_FRINFC_TOPAZ_STATE_WR_CC_OR_TLV:
            {
                Status = phFriNfc_Tpz_H_ProCCTLV (psNdefMap);
                break;
            }

            default:
            {
                Status = PHNFCSTVAL (CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_DEVICE_REQUEST);
                break;
            }
        }
    }

    /* Call Completion Routine, if Status != PENDING */
    if (NFCSTATUS_PENDING != Status)
    {
        phFriNfc_Tpz_H_Complete(psNdefMap, Status);
    }
}

static NFCSTATUS phFriNfc_Tpz_H_RdBytes(phFriNfc_NdefMap_t *NdefMap,
                                        uint16_t             BlockNo,
                                        uint16_t            ByteNo)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    uint8_t index = 0;

    /* set the data for additional data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = PH_FRINFC_TOPAZ_VAL0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = PH_FRINFC_TOPAZ_VAL0;
    NdefMap->psDepAdditionalInfo.NodeAddress = PH_FRINFC_TOPAZ_VAL0;

    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_TopazMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Depending on the jewel command, the send length is decided */
    switch(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0])
    {
    case PH_FRINFC_TOPAZ_CMD_READID:

        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        /*Copy command to  Send Buffer*/
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0]  = PH_FRINFC_TOPAZ_CMD_READID;
        index ++;

        /*Copy UID of the tag to  Send Buffer*/
        (void)phOsalNfc_SetMemory(&(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL1]),
        0x00,(0x06));
        index = index + 0x06;

        /* Update the length of the command buffer*/
        NdefMap->SendLength = index;

        /*Copy UID of the tag to  Send Buffer in case of DTA enabled */
        if(NdefMap->bDtaFlag)
        {
            (void)phOsalNfc_MemCopy(&(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL3]),
                &(NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid),
                TOPAZ_UID_LENGTH_FOR_READ_WRITE);
        }
        break;

    case PH_FRINFC_TOPAZ_CMD_READALL:

        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        /*Copy command to  Send Buffer*/
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0]  = PH_FRINFC_TOPAZ_CMD_READALL;
        index ++;

        /*Copy 0x00 to Send Buffer*/
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL1] = 0x00;
        index ++;
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2] = 0x00;
        index ++;

        /*Copy UID of the tag to  Send Buffer*/
        (void)phOsalNfc_MemCopy(&(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL3]),
        &(NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid),
        TOPAZ_UID_LENGTH_FOR_READ_WRITE);

        index = (uint8_t)(index + TOPAZ_UID_LENGTH_FOR_READ_WRITE);

        /* Update the length of the command buffer*/
        NdefMap->SendLength = index;
        break;

    case PH_FRINFC_TOPAZ_CMD_READ:

        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        /*Copy command to  Send Buffer*/
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0]  = PH_FRINFC_TOPAZ_CMD_READ;
        index ++;

        /*Copy Address to  Send Buffer*/
        /* Calculate send length
        7 | 6   5   4   3 | 2   1   0 |
        |    block no   |  byte no  |
        */
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL1] =
                (uint8_t)((BlockNo << PH_FRINFC_TOPAZ_SHIFT3) +
                ByteNo);
        index ++;
        /*Copy 0x00 to  Send Buffer*/
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2] = 0x00;
        index ++;

        /*Copy UID of the tag to  Send Buffer*/
        (void)phOsalNfc_MemCopy(&(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL3]),
        &(NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid),
        TOPAZ_UID_LENGTH_FOR_READ_WRITE);
        index = (uint8_t)(index + TOPAZ_UID_LENGTH_FOR_READ_WRITE);

        /* Update the length of the command buffer*/
        NdefMap->SendLength = index;
        break;

    default:
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_INVALID_DEVICE_REQUEST);
    }

    if(Result == NFCSTATUS_SUCCESS)
    {
        /* Call the Overlapped HAL Transceive function */
        Result = phFriNfc_OvrHal_Transceive(    NdefMap->LowerDevice,
                                                &NdefMap->MapCompletionInfo,
                                                NdefMap->psRemoteDevInfo,
                                                NdefMap->Cmd,
                                                &NdefMap->psDepAdditionalInfo,
                                                NdefMap->SendRecvBuf,
                                                NdefMap->SendLength,
                                                NdefMap->SendRecvBuf,
                                                NdefMap->SendRecvLength);
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_WrAByte(phFriNfc_NdefMap_t *NdefMap,
                                        uint16_t             BlockNo,
                                        uint16_t             ByteNo,
                                        uint8_t              ByteVal
                                        )
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    uint8_t     index = 0;


    PHNFC_UNUSED_VARIABLE(ByteVal);
    /* set the data for additional data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = PH_FRINFC_TOPAZ_VAL0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = PH_FRINFC_TOPAZ_VAL0;
    NdefMap->psDepAdditionalInfo.NodeAddress = PH_FRINFC_TOPAZ_VAL0;

    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_TopazMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Command used to write 1 byte */
    NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;

    /*Copy command to Send Buffer*/
    NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0]  = PH_FRINFC_TOPAZ_CMD_WRITE_1E;
    index ++;

    /*Copy Address to  Send Buffer*/
    /* Calculate send length
    7 | 6   5   4   3 | 2   1   0 |
    |    block no   |  byte no  |
    */
    NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL1] =
            (uint8_t)((BlockNo << PH_FRINFC_TOPAZ_SHIFT3) +
            ByteNo);
    index ++;
    /*Copy Data byte to Send Buffer*/
    NdefMap->SendRecvBuf[index] = ByteVal;
    index ++;

    /*Copy UID of the tag to  Send Buffer*/
    (void)phOsalNfc_MemCopy(&(NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL3]),
      &(NdefMap->psRemoteDevInfo->RemoteDevInfo.Jewel_Info.Uid),
      TOPAZ_UID_LENGTH_FOR_READ_WRITE);
    index = (uint8_t)(index + TOPAZ_UID_LENGTH_FOR_READ_WRITE);

    /* Update the length of the command buffer*/
    NdefMap->SendLength = index;

    /* Call the Overlapped HAL Transceive function */
    Result = phFriNfc_OvrHal_Transceive(    NdefMap->LowerDevice,
                                            &NdefMap->MapCompletionInfo,
                                            NdefMap->psRemoteDevInfo,
                                            NdefMap->Cmd,
                                            &NdefMap->psDepAdditionalInfo,
                                            NdefMap->SendRecvBuf,
                                            NdefMap->SendLength,
                                            NdefMap->SendRecvBuf,
                                            NdefMap->SendRecvLength);

    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProReadID(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);

    if(((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] &
        PH_FRINFC_TOPAZ_HEADROM0_CHK) == PH_FRINFC_TOPAZ_HEADROM0_VAL) &&
        (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL6))
    {
        /* Copy UID to the context, Used when the READ ALL command is used */
        (void)phOsalNfc_MemCopy(NdefMap->TopazContainer.UID,
            &NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2],
            PH_FRINFC_TOPAZ_VAL4);

        /* State has to be changed */
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_READALL;

        /* Topaz command = READALL */
        NdefMap->Cmd.JewelCmd = phNfc_eJewel_Raw;
        NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] = PH_FRINFC_TOPAZ_CMD_READALL;

        /* Read all bytes from the card */
        Result = phFriNfc_Tpz_H_RdBytes(NdefMap, NdefMap->TopazContainer.CurrentBlock,
            NdefMap->TopazContainer.ByteNumber);
    }

    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProReadAll(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    int32_t     memcompare = PH_FRINFC_TOPAZ_VAL0;

    /* Compare the UID of READ ALL command with the stored UID */
    if ((NdefMap->TopazContainer.UID[0] == NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2]) &&
        (NdefMap->TopazContainer.UID[1] == NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2 + 1]) &&
        (NdefMap->TopazContainer.UID[2] == NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2 + 2]) &&
        (NdefMap->TopazContainer.UID[3] == NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL2 + 3]))
    {
        memcompare = PH_FRINFC_TOPAZ_VAL0;
    }
    else
    {
        memcompare = PH_FRINFC_TOPAZ_VAL1;
    }

    if(((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] &
        PH_FRINFC_TOPAZ_HEADROM0_CHK) == PH_FRINFC_TOPAZ_HEADROM0_VAL) &&
        (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_READALL_RESP) &&
        (memcompare == PH_FRINFC_TOPAZ_VAL0))
    {
        /* Copy 96 bytes from the read/write memory space */
        (void)phOsalNfc_MemCopy(NdefMap->TopazContainer.ReadBuffer,
            &NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL10],
            PH_FRINFC_TOPAZ_TOTAL_RWBYTES);

        /* Check the lock bits and set the card state */
        phFriNfc_Tpz_H_ChkLockBits(NdefMap);

        Result = phFriNfc_Tpz_H_CallNxtOp(NdefMap);
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_CallNxtOp(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    /* Depending on the operation (check, read or write ndef), process the
    read data */
    switch(NdefMap->PrevOperation)
    {
    case PH_FRINFC_NDEFMAP_CHECK_OPE:
        /* Check the capabilty container values, according
        to the spec */
        Result = phFriNfc_Tpz_H_ChkCCinChkNdef(NdefMap);

        if (NdefMap->CardState != PH_NDEFMAP_CARD_STATE_INVALID)
        {
            /* Check the spec version */
            Result = phFriNfc_Tpz_H_ChkSpcVer( NdefMap,
                NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL1]);
            /*  Check the CC header size: Only valid ones are
            0x0C for 96 bytes. */
            if ((Result == NFCSTATUS_SUCCESS) &&
                ( NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL2] <=
                PH_FRINFC_TOPAZ_CC_BYTE2_MAX))
            {
                Result = phFriNfc_Tpz_H_findNDEFTLV(NdefMap);

                /* As there is possibility of either having or not having TLV in
                Topaz, no need to send the Actual status to the context.
                   _Analysis_Assume_ to force PreFast to think that NdefMap was left
                in a valid state. */

                _Analysis_assume_(Result == NFCSTATUS_SUCCESS);
                Result = NFCSTATUS_SUCCESS;
            }
        }
        else
        {
            Result = NFCSTATUS_SUCCESS;
            NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INITIALIZED;
            NdefMap->CardMemSize =
            NdefMap->TopazContainer.RemainingSize = (uint16_t)
                        /*
                        4 is decremented from the max size because of the 4 CC bytes
                        2 is decremented because of the NDEF TLV T and L byte
                        to get the actual data size
                        */
                        (PH_FRINFC_TOPAZ_MAX_CARD_SZ - PH_FRINFC_TOPAZ_VAL4 -
                        PH_FRINFC_TOPAZ_VAL2);
        }
        break;

    case PH_FRINFC_NDEFMAP_READ_OPE:
        /* Check the capabilty container values, according
        to the spec */
        Result = phFriNfc_Tpz_H_ChkCCBytes(NdefMap);

        /* If success, find the ndef TLV */
        Result = ((Result != NFCSTATUS_SUCCESS)?
                (PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                NFCSTATUS_INVALID_FORMAT)):
                phFriNfc_Tpz_H_findNDEFTLV(NdefMap));

        if(Result == NFCSTATUS_SUCCESS)
        {
            NdefMap->TopazContainer.ByteNumber += PH_FRINFC_TOPAZ_VAL2;
            /* If success, copy the read bytes to the user buffer */
            Result = phFriNfc_Tpz_H_CpDataToUsrBuf(NdefMap);
        }
        break;

    case PH_FRINFC_NDEFMAP_WRITE_OPE:
    default:
        if((NdefMap->CardState == PH_NDEFMAP_CARD_STATE_READ_WRITE) ||
            (NdefMap->CardState == PH_NDEFMAP_CARD_STATE_INITIALIZED))
        {
            /* Check the capabilty container values, according
            to the spec */
            Result = phFriNfc_Tpz_H_ChkCCBytes(NdefMap);
            if(Result == NFCSTATUS_SUCCESS)
            {
                /* Find the NDEF TLV */
                Result = phFriNfc_Tpz_H_findNDEFTLV(NdefMap);

                /* Write the TLV */
                NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_T_OF_TLV;
            }
            else
            {
                NdefMap->TLVStruct.NdefTLVByte = PH_FRINFC_TOPAZ_VAL4;
                NdefMap->TLVStruct.NdefTLVBlock = PH_FRINFC_TOPAZ_VAL1;
                NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL4;
                /* Write the TLV */
                NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_CC_BYTE0;
            }
            /* Write CC bytes */
            Result = phFriNfc_Tpz_H_WrCCorTLV(NdefMap);
        }
        break;
    }

    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ChkCCBytes(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_NO_NDEF_SUPPORT);

    if(NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL0] ==
        PH_FRINFC_TOPAZ_CC_BYTE0)
    {
        /* Check the spec version */
        Result = phFriNfc_Tpz_H_ChkSpcVer( NdefMap,
            NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL1]);
        /*  Check the CC header size: Only valid ones are
        0x0C for 96 bytes. */
        Result = ((( NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL2] >
                    PH_FRINFC_TOPAZ_CC_BYTE2_MAX) || (Result !=
                    NFCSTATUS_SUCCESS))?
                    (PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                    NFCSTATUS_EOF_NDEF_CONTAINER_REACHED)):
                    Result);

        /* Get the read/write card memory size */
        NdefMap->TopazContainer.RemainingSize =
                NdefMap->CardMemSize = ((Result == NFCSTATUS_SUCCESS)?
                (PH_FRINFC_TOPAZ_MAX_CARD_SZ - PH_FRINFC_TOPAZ_VAL4):
                NdefMap->CardMemSize);

        /* if the call is from write ndef then check for read write access */
        if(((NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_WRITE_OPE) &&
            (NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL3] !=
            PH_FRINFC_TOPAZ_CC_BYTE3_RW) && (Result == NFCSTATUS_SUCCESS)))
        {
            Result = (PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                NFCSTATUS_INVALID_FORMAT));
        }

        /* if the call is from read ndef then check for read only or read write access */
        if(((NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_READ_OPE) &&
            ((NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL3] !=
            PH_FRINFC_TOPAZ_CC_BYTE3_RW) &&
            (NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL3] !=
            PH_FRINFC_TOPAZ_CC_BYTE3_RO))&& (Result == NFCSTATUS_SUCCESS)))
        {
            Result = (PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                NFCSTATUS_INVALID_FORMAT));
        }
    }
    return Result;
}

extern NFCSTATUS phFriNfc_Tpz_H_ChkSpcVer( phFriNfc_NdefMap_t  *NdefMap,
                                          uint8_t             VersionNo)
{
    NFCSTATUS Result = NFCSTATUS_SUCCESS;
    uint8_t TagVerNo = VersionNo;

    /* To remove "warning (VS C4100) : unreferenced formal parameter" */
    PHNFC_UNUSED_VARIABLE(NdefMap);

    if ( TagVerNo == 0 )
    {
        /*Return Status Error “ Invalid Format”*/
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
    }
    else
    {
        /* calculate the major and minor version number of T3VerNo */
        if( (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM ==
            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
            ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM >=
            PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
            (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM ==
            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
            ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM <
            PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))
        {
            Result = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
        }
        else
        {
            if (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM <
                PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo) ) ||
                ( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM >
                PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo)))
            {
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
            }
        }
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_findNDEFTLV(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_NO_NDEF_SUPPORT);
    uint8_t     index = PH_FRINFC_TOPAZ_VAL4;

    /* If remaining size is less than 3 then, there cant be any
    TLV present in the card */
    while((index < PH_FRINFC_TOPAZ_TOTAL_RWBYTES) &&
        (NdefMap->TopazContainer.RemainingSize >= PH_FRINFC_TOPAZ_VAL3))
    {
        switch(NdefMap->TopazContainer.ReadBuffer[index])
        {
        case PH_FRINFC_TOPAZ_NDEF_T:
            /* To get the length field of the TLV */
            index++;
            /* Type and length are not data bytes, so to know the exact
            remaining size in the card, the below operation is done */
            NdefMap->TopazContainer.RemainingSize -= PH_FRINFC_TOPAZ_VAL2;
            /* Set the card state depending on the L value */
            Result = phFriNfc_MapTool_SetCardState(NdefMap,
                (uint32_t)NdefMap->TopazContainer.ReadBuffer[index]);
            /* Check the TLV is correct */
            if((NdefMap->TopazContainer.ReadBuffer[index] >
                NdefMap->TopazContainer.RemainingSize) ||
                ((NdefMap->TopazContainer.ReadBuffer[index] ==
                PH_FRINFC_TOPAZ_VAL0) && (NdefMap->PrevOperation ==
                PH_FRINFC_NDEFMAP_READ_OPE)) || (Result != NFCSTATUS_SUCCESS))
            {
                /* L field value cant be greater than the remaining size, so error */
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_NO_NDEF_SUPPORT);
                /* To break out of the loop */
                index = PH_FRINFC_TOPAZ_TOTAL_RWBYTES;
            }
            else
            {
                /* So remaining size also changes, according to the position of NDEF TLV  */
                NdefMap->TLVStruct.BytesRemainLinTLV =
                    NdefMap->TopazContainer.ReadBuffer[index];

                /* Get the byte number */
                NdefMap->TLVStruct.NdefTLVByte = (uint16_t)((index - PH_FRINFC_TOPAZ_VAL1) %
                                                    PH_FRINFC_TOPAZ_VAL8);
                /* Get the block number */
                NdefMap->TLVStruct.NdefTLVBlock = (uint8_t)(((index - PH_FRINFC_TOPAZ_VAL1) /
                                                    PH_FRINFC_TOPAZ_VAL8) +
                                                    PH_FRINFC_TOPAZ_VAL1);
                /* TLV found flag is set */
                NdefMap->TLVStruct.NdefTLVFoundFlag = PH_FRINFC_TOPAZ_FLAG1;
                /* To know the position of V field in the TLV */
                NdefMap->TopazContainer.ByteNumber = (uint8_t)(((NdefMap->TLVStruct.NdefTLVBlock - 1) * 8) +
                                                    NdefMap->TLVStruct.NdefTLVByte);
                /* To break out of the loop */
                index = PH_FRINFC_TOPAZ_TOTAL_RWBYTES;
                Result = NFCSTATUS_SUCCESS;
            }
            break;

        case PH_FRINFC_TOPAZ_NULL_T:
            /* Null TLV, Skip the TLV */
            NdefMap->TopazContainer.RemainingSize--;
            index++;
            break;

        case PH_FRINFC_TOPAZ_TERM_T:
            /* No more TLV present in the card, so error */
            index = PH_FRINFC_TOPAZ_TOTAL_RWBYTES;
            Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                NFCSTATUS_NO_NDEF_SUPPORT);
            break;

        default:
            /* Go till the length field of the TLV */
            index++;
            /* Type and length is not the data, so to know the exact
            remaining size in the card, the below operation is done */
            NdefMap->TopazContainer.RemainingSize -= PH_FRINFC_TOPAZ_VAL2;
            if(NdefMap->TopazContainer.ReadBuffer[index] >
                NdefMap->TopazContainer.RemainingSize)
            {
                /* L field value cant be greater than the remaining size, so error */
                index = PH_FRINFC_TOPAZ_TOTAL_RWBYTES;
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_NO_NDEF_SUPPORT);
            }
            else
            {
                /* Remaining size of the free space available in the card changes,
                according to the position of NDEF TLV */
                NdefMap->TopazContainer.RemainingSize =
                    NdefMap->TopazContainer.RemainingSize -
                    NdefMap->TopazContainer.ReadBuffer[index];

                /* Get the position of the next TLV */
                index = (uint8_t)(index +
                    (NdefMap->TopazContainer.ReadBuffer[index] +
                     PH_FRINFC_TOPAZ_VAL1));
            }
            break;
        }
    }

    /* If no Ndef TLV found and operation done is read */
    if((NdefMap->TLVStruct.NdefTLVFoundFlag == PH_FRINFC_TOPAZ_FLAG0) &&
        (NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_READ_OPE))
    {
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    if((NdefMap->TLVStruct.NdefTLVFoundFlag == PH_FRINFC_TOPAZ_FLAG0) &&
        ((NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_WRITE_OPE) ||
        (NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_CHECK_OPE)))
    {
        NdefMap->TLVStruct.NdefTLVByte = PH_FRINFC_TOPAZ_VAL4;
        NdefMap->TLVStruct.NdefTLVBlock = PH_FRINFC_TOPAZ_VAL1;
        NdefMap->TopazContainer.ByteNumber = PH_FRINFC_TOPAZ_VAL4;
        NdefMap->TopazContainer.RemainingSize = PH_FRINFC_TOPAZ_TOTAL_RWBYTES1;
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_CpDataToUsrBuf(phFriNfc_NdefMap_t  *NdefMap)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;

    /* Check the the TLV size and the user size */
    if(NdefMap->ApduBufferSize >=
        NdefMap->TLVStruct.BytesRemainLinTLV)
    {
        /* Copy the read bytes to user buffer till the value (V)
        of TLV ends */
        (void)phOsalNfc_MemCopy(NdefMap->ApduBuffer,
            &(NdefMap->TopazContainer.ReadBuffer[
                NdefMap->TopazContainer.ByteNumber]),
                    NdefMap->TLVStruct.BytesRemainLinTLV);

                /* Update the number of read bytes to the user */
                *(NdefMap->NumOfBytesRead) =
                    NdefMap->TLVStruct.BytesRemainLinTLV;
                /* There is no byte to read */
                NdefMap->TopazContainer.ByteNumber =
                                                    PH_FRINFC_TOPAZ_VAL0;
                /* No further read is possible */
                NdefMap->TopazContainer.ReadWriteCompleteFlag =
                                                    PH_FRINFC_TOPAZ_FLAG1;
                /* Remaining size in the card can be greater than length field in
                the TLV */
                NdefMap->TopazContainer.RemainingSize =
                    NdefMap->TopazContainer.RemainingSize -
                                        NdefMap->TLVStruct.BytesRemainLinTLV;
                /* TLV has been completely read, no more bytes to read */
                NdefMap->TLVStruct.BytesRemainLinTLV =
                                                    PH_FRINFC_TOPAZ_VAL0;
    }
    else
    {
        /* Copy read bytes till the user buffer size */

        if((NdefMap->TopazContainer.ByteNumber + NdefMap->ApduBufferSize) > PH_FRINFC_NDEFMAP_TOPAZ_MAX_SIZE)
        {
            PH_LOG_NDEF_CRIT_STR("Buffer too small! "
                                 "ByteNumber: %hu "
                                 "ApduBufferSize: %u",
                                 NdefMap->TopazContainer.ByteNumber,
                                 NdefMap->ApduBufferSize);

            Result = NFCSTATUS_BUFFER_TOO_SMALL;
            goto Done;
        }

#pragma prefast(suppress: __WARNING_BUFFER_COPY_NO_PREDICATE, "ApduBuffer is _Inexpressible_")
        (void)phOsalNfc_MemCopy(NdefMap->ApduBuffer,
                    &(NdefMap->TopazContainer.ReadBuffer[
                    NdefMap->TopazContainer.ByteNumber]),
                    NdefMap->ApduBufferSize);

        /* Update the number of read bytes to the user */
        *(NdefMap->NumOfBytesRead) =
            NdefMap->ApduBufferSize;
        /* Get the next byte number to read */
        NdefMap->TopazContainer.ByteNumber =
            (uint8_t)(NdefMap->TopazContainer.ByteNumber +
                      NdefMap->ApduBufferSize);
        /* Free space left in the card */
        NdefMap->TopazContainer.RemainingSize
            = NdefMap->TopazContainer.RemainingSize
                  - (uint16_t)NdefMap->ApduBufferSize;
        /* Bytes left in the TLV */
        NdefMap->TLVStruct.BytesRemainLinTLV =
            NdefMap->TLVStruct.BytesRemainLinTLV -
                   (uint16_t)NdefMap->ApduBufferSize;
    }

Done:

    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProWrNMN(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    uint8_t     BlockNo = PH_FRINFC_TOPAZ_VAL0,
        ByteNo = PH_FRINFC_TOPAZ_VAL0;

    if((NdefMap->TopazContainer.InternalState ==
        PH_FRINFC_TOPAZ_WR_NMN_0) &&
        (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
        PH_FRINFC_TOPAZ_VAL0) &&
        (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
    {
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE_L_TLV;
        /* Get the L field of the TLV block */
        BlockNo = (uint8_t)(((NdefMap->TLVStruct.NdefTLVByte + PH_FRINFC_TOPAZ_VAL1) >
                            PH_FRINFC_TOPAZ_VAL7)?
                            (NdefMap->TLVStruct.NdefTLVBlock + PH_FRINFC_TOPAZ_VAL1):
                            NdefMap->TLVStruct.NdefTLVBlock);
        /* Get the L byte */
        ByteNo = (uint8_t)((NdefMap->TLVStruct.NdefTLVByte + PH_FRINFC_TOPAZ_VAL1) %
                                PH_FRINFC_TOPAZ_VAL8);


        /* Here the NMN is written 0, so internal state is used */
        NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_L_TLV_0;
        /* Write the length value = 0x00 , Write L field of TLV  = 0  inside this*/
        Result = phFriNfc_Tpz_H_WrAByte( NdefMap, BlockNo, ByteNo,
                                        PH_FRINFC_TOPAZ_VAL0);
    }
    else
    {
        if((NdefMap->TopazContainer.InternalState ==
            PH_FRINFC_TOPAZ_WR_NMN_E1) &&
            (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_CC_BYTE0) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            /* Card state is initialised or invalid */
            NdefMap->CardState = (uint8_t)((NdefMap->CardState ==
                                    PH_NDEFMAP_CARD_STATE_INITIALIZED)?
                                    PH_NDEFMAP_CARD_STATE_READ_WRITE:
                                    NdefMap->CardState);
            /* update the length to the user */
            *NdefMap->WrNdefPacketLength = NdefMap->ApduBuffIndex;
            Result = NFCSTATUS_SUCCESS;
        }
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProWrTLV(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    if((NdefMap->TopazContainer.InternalState ==
        PH_FRINFC_TOPAZ_WR_L_TLV_0) &&
        (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
        PH_FRINFC_TOPAZ_VAL0) &&
        (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
    {
        /* state is writing user data to the card */
        NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE;

        NdefMap->TopazContainer.ByteNumber++;
        /* Check the byte number */
        phFriNfc_Tpz_H_BlkChk(NdefMap);

        /* Write data to the specified location */
        /* Write the data to the card from the user buffer */
        Result = phFriNfc_Tpz_H_WrAByte( NdefMap,
                                        NdefMap->TopazContainer.CurrentBlock,
                                        NdefMap->TopazContainer.ByteNumber,
                                        NdefMap->ApduBuffer[NdefMap->ApduBuffIndex]
                                        );
    }
    else
    {
        if((NdefMap->TopazContainer.InternalState ==
            PH_FRINFC_TOPAZ_WR_L_TLV) &&
            (((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN) &&
            (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            NdefMap->ApduBuffIndex)) ||
            ((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_CUR) &&
            (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            (NdefMap->ApduBuffIndex + NdefMap->TLVStruct.BytesRemainLinTLV)))) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            /* Update the L value in the context */
            NdefMap->TLVStruct.BytesRemainLinTLV =
                ((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN)?
                NdefMap->ApduBuffIndex:
            (NdefMap->TLVStruct.BytesRemainLinTLV + NdefMap->ApduBuffIndex));

            /* Write 0xE1 in block number = 1 and byte number = 0 */
            Result = phFriNfc_Tpz_H_WrByte0ValE1(NdefMap);
        }
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProWrUsrData(phFriNfc_NdefMap_t *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    /* send buffer should be equal to receive buffer */
    if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
        NdefMap->ApduBuffer[NdefMap->ApduBuffIndex]) &&
        (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
    {
        /* Increment the index */
        NdefMap->ApduBuffIndex += PH_FRINFC_TOPAZ_VAL1;

        /* Remaining space left in the card is less by one */
        NdefMap->TopazContainer.RemainingSize -= PH_FRINFC_TOPAZ_VAL1;

        /* Increment the byte number */
        NdefMap->TopazContainer.ByteNumber++;

        /* Check the block number */
        phFriNfc_Tpz_H_BlkChk(NdefMap);

        /* check for the user space or the card size */
        if((NdefMap->ApduBufferSize == NdefMap->ApduBuffIndex) ||
            (NdefMap->TopazContainer.RemainingSize == PH_FRINFC_TOPAZ_VAL0))
        {
            /* Set write complete, if the end of card is reached */
            NdefMap->TopazContainer.ReadWriteCompleteFlag = (uint8_t)
                ((NdefMap->TopazContainer.RemainingSize == PH_FRINFC_TOPAZ_VAL0)?
                PH_FRINFC_TOPAZ_FLAG1:PH_FRINFC_TOPAZ_FLAG0);

            Result = phFriNfc_Tpz_H_WrLByte(NdefMap);
        }
        else
        {
            /* State is continued to be in write */
            NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE;

            /* Write the byte to the specified location , and Byte to write */
            Result = phFriNfc_Tpz_H_WrAByte(NdefMap, NdefMap->TopazContainer.CurrentBlock,
                                            NdefMap->TopazContainer.ByteNumber,
                                            NdefMap->ApduBuffer[NdefMap->ApduBuffIndex]
                                            );
        }
    }
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_WrLByte(phFriNfc_NdefMap_t  *NdefMap)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    uint8_t     BlockNo = PH_FRINFC_TOPAZ_VAL0,
        ByteNo = PH_FRINFC_TOPAZ_VAL0;
    uint8_t TempByteVal = 0;
    BlockNo = (uint8_t)(((NdefMap->TLVStruct.NdefTLVByte +
                            PH_FRINFC_TOPAZ_VAL1) >
                            PH_FRINFC_TOPAZ_VAL7)?
                            (NdefMap->TLVStruct.NdefTLVBlock +
                            PH_FRINFC_TOPAZ_VAL1):
    NdefMap->TLVStruct.NdefTLVBlock);

    ByteNo = (uint8_t)((NdefMap->TLVStruct.NdefTLVByte +
                        PH_FRINFC_TOPAZ_VAL1) % PH_FRINFC_TOPAZ_VAL8);
    /* Update L field */
    NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE_L_TLV;
    /* Internal state for write */
    NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_L_TLV;
    /* Update the length field depending on the offset */
    TempByteVal = (uint8_t)
                    ((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN)?
                    NdefMap->ApduBuffIndex:
                    (NdefMap->ApduBuffIndex +
                    NdefMap->TLVStruct.BytesRemainLinTLV));
    /* Write the L field */
    Result = phFriNfc_Tpz_H_WrAByte(NdefMap, BlockNo, ByteNo,TempByteVal);
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_WrByte0ValE1(phFriNfc_NdefMap_t   *NdefMap)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;

    /* Update L field */
    NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE_NMN;
    /* Internal state for writing 0xE1 */
    NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_NMN_E1;
    /* Update the length field depending on the offset */
    /* Write the L field */
    Result = phFriNfc_Tpz_H_WrAByte(NdefMap, PH_FRINFC_TOPAZ_VAL1,
                                    PH_FRINFC_TOPAZ_VAL0,PH_FRINFC_TOPAZ_CC_BYTE0);
    return Result;
}

static void phFriNfc_Tpz_H_Complete(phFriNfc_NdefMap_t  *NdefMap,
                                    NFCSTATUS           Status)
{
    /* set the state back to the Reset_Init state*/
    NdefMap->State =  PH_FRINFC_NDEFMAP_STATE_RESET_INIT;

    /* set the completion routine*/
    NdefMap->CompletionRoutine[NdefMap->TopazContainer.CRIndex].
        CompletionRoutine(NdefMap->CompletionRoutine->Context, Status);
}

static void phFriNfc_Tpz_H_BlkChk(phFriNfc_NdefMap_t  *NdefMap)
{
    NdefMap->TopazContainer.CurrentBlock =
                    (uint8_t)((NdefMap->TopazContainer.ByteNumber >
                    PH_FRINFC_TOPAZ_VAL7)?
                    (NdefMap->TopazContainer.CurrentBlock +
                    PH_FRINFC_TOPAZ_VAL1):
                    NdefMap->TopazContainer.CurrentBlock);

    NdefMap->TopazContainer.ByteNumber = (uint8_t)(NdefMap->TopazContainer.ByteNumber %
                                        PH_FRINFC_TOPAZ_VAL8);
}

static NFCSTATUS phFriNfc_Tpz_H_ChkCCinChkNdef(phFriNfc_NdefMap_t  *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_NO_NDEF_SUPPORT);

    if(NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL0] ==
        PH_FRINFC_TOPAZ_CC_BYTE0)
    {
        /* Check the most significant nibble of byte 3 (RWA) = 0 */
        Result = (((NdefMap->TopazContainer.ReadBuffer[PH_FRINFC_TOPAZ_VAL3] &
                    PH_FRINFC_TOPAZ_BYTE3_MSB)== PH_FRINFC_TOPAZ_VAL0)?
                    NFCSTATUS_SUCCESS:
                    Result);

        /* Card size is initialised */
        NdefMap->CardMemSize = NdefMap->TopazContainer.RemainingSize =
                            ((Result == NFCSTATUS_SUCCESS)?
                            (PH_FRINFC_TOPAZ_MAX_CARD_SZ - PH_FRINFC_TOPAZ_VAL4):
                            NdefMap->CardMemSize);
    }

    if (NdefMap->CardState != PH_NDEFMAP_CARD_STATE_READ_ONLY)
    {
        NdefMap->CardState = (uint8_t)((Result == NFCSTATUS_SUCCESS)?
                            PH_NDEFMAP_CARD_STATE_INITIALIZED:
                            PH_NDEFMAP_CARD_STATE_INVALID);
    }

    return Result;
}

static void phFriNfc_Tpz_H_ChkLockBits(phFriNfc_NdefMap_t  *NdefMap)
{
    /* Set the card state */
    NdefMap->CardState =  (uint8_t)
        (((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_LOCKBIT_BYTENO_0] ==
        PH_FRINFC_TOPAZ_LOCKBIT_BYTE114) &&
        ((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_LOCKBIT_BYTENO_1] ==
        PH_FRINFC_TOPAZ_LOCKBIT_BYTE115_1) ||
        (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_LOCKBIT_BYTENO_1] ==
        PH_FRINFC_TOPAZ_LOCKBIT_BYTE115_2)))?
        PH_NDEFMAP_CARD_STATE_READ_WRITE:
        PH_NDEFMAP_CARD_STATE_READ_ONLY);

    /* Set the card state from CC bytes */
    if (NdefMap->CardState == PH_NDEFMAP_CARD_STATE_READ_WRITE)
    {
        if (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_CC_BYTENO_3] == PH_FRINFC_TOPAZ_CC_READWRITE)
        {
            NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_WRITE;
        }
        else if (NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_CC_BYTENO_3] == PH_FRINFC_TOPAZ_CC_READONLY)
        {
            NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_ONLY;
        }
        else
        {
            NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
        }
    }
}

static NFCSTATUS phFriNfc_Tpz_H_WrCCorTLV(phFriNfc_NdefMap_t  *NdefMap)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    uint8_t     ByteNo = PH_FRINFC_TOPAZ_VAL0,
        BlockNo = PH_FRINFC_TOPAZ_VAL0;
    uint8_t TempByteVal = 0;
    switch(NdefMap->TopazContainer.InternalState)
    {
    case PH_FRINFC_TOPAZ_WR_CC_BYTE0:
        /* To write the CC bytes */
        TempByteVal = PH_FRINFC_TOPAZ_CC_BYTE0;
        ByteNo = (uint8_t)PH_FRINFC_TOPAZ_VAL0;
        BlockNo = PH_FRINFC_TOPAZ_VAL1;
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE1:
        /* To write the CC bytes */
        TempByteVal = PH_FRINFC_TOPAZ_CC_BYTE1;
        ByteNo = (uint8_t)PH_FRINFC_TOPAZ_VAL1;
        BlockNo = PH_FRINFC_TOPAZ_VAL1;
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE2:
        /* To write the CC bytes */
        TempByteVal = PH_FRINFC_TOPAZ_CC_BYTE2_MAX;
        ByteNo = (uint8_t)PH_FRINFC_TOPAZ_VAL2;
        BlockNo = PH_FRINFC_TOPAZ_VAL1;
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE3:
        /* To write the CC bytes */
        TempByteVal = PH_FRINFC_TOPAZ_CC_BYTE3_RW;
        ByteNo = (uint8_t)PH_FRINFC_TOPAZ_VAL3;
        BlockNo = PH_FRINFC_TOPAZ_VAL1;
        break;

    case PH_FRINFC_TOPAZ_WR_T_OF_TLV:
    default:
        /* To write the NDEF TLV (if not present) */
        TempByteVal = PH_FRINFC_TOPAZ_NDEF_T;
        ByteNo = (uint8_t)NdefMap->TLVStruct.NdefTLVByte;
        BlockNo = NdefMap->TLVStruct.NdefTLVBlock;
        break;
    }
    NdefMap->State = PH_FRINFC_TOPAZ_STATE_WR_CC_OR_TLV;
    Result = phFriNfc_Tpz_H_WrAByte( NdefMap, BlockNo, ByteNo,TempByteVal);
    return Result;
}

static NFCSTATUS phFriNfc_Tpz_H_ProCCTLV(phFriNfc_NdefMap_t  *NdefMap)
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
        NFCSTATUS_INVALID_RECEIVE_LENGTH);
    switch(NdefMap->TopazContainer.InternalState)
    {
    case PH_FRINFC_TOPAZ_WR_CC_BYTE0:
        /* Process the CC byte */
        /* Check the response */
        if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_CC_BYTE0) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_CC_BYTE1;
            Result = phFriNfc_Tpz_H_WrCCorTLV(NdefMap);
        }
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE1:
        /* Process the CC byte */
        /* Check the response */
        if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_CC_BYTE1) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_CC_BYTE2;
            Result = phFriNfc_Tpz_H_WrCCorTLV(NdefMap);
        }
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE2:
        /* Process the CC byte */
        /* Check the response */
        if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_CC_BYTE2_MAX) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_CC_BYTE3;
            Result = phFriNfc_Tpz_H_WrCCorTLV(NdefMap);
        }
        break;

    case PH_FRINFC_TOPAZ_WR_CC_BYTE3:
        /* Process the CC byte */
        /* Check the response */
        if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_CC_BYTE3_RW) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_T_OF_TLV;
            Result = phFriNfc_Tpz_H_WrCCorTLV(NdefMap);
        }
        break;

    case PH_FRINFC_TOPAZ_WR_T_OF_TLV:
    default:
        /* Check the response */
        if((NdefMap->SendRecvBuf[PH_FRINFC_TOPAZ_VAL0] ==
            PH_FRINFC_TOPAZ_NDEF_T) &&
            (*NdefMap->SendRecvLength == PH_FRINFC_TOPAZ_VAL1))
        {
            /* Increment the Byte Number */
            NdefMap->TopazContainer.ByteNumber++;
            /* Check the block and byte number */
            phFriNfc_Tpz_H_BlkChk(NdefMap);
            /* Process the T of NDEF TLV */
            NdefMap->State = PH_FRINFC_TOPAZ_STATE_WRITE_NMN;

            /* Here the NMN is written 0, so internal state is used */
            NdefMap->TopazContainer.InternalState = PH_FRINFC_TOPAZ_WR_NMN_0;

            /*Inside this call Write NMN (block number 1 and byte number 0) = 0 */
            Result = phFriNfc_Tpz_H_WrAByte( NdefMap, PH_FRINFC_TOPAZ_VAL1,
                PH_FRINFC_TOPAZ_VAL0,PH_FRINFC_TOPAZ_VAL0);
        }
        break;
    }
    return Result;
}
