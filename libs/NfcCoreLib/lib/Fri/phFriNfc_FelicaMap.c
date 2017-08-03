/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_FelicaMap.tmh"

/* Helpers for Read and updating the attribute informations*/
static
NFCSTATUS
phFriNfc_Felica_HRdAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HNdefRdAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static NFCSTATUS
phFriNfc_Felica_HNdefWrAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HUpdateAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HCalCheckSum(
    _In_reads_bytes_(EndIndex + 1)  const uint8_t   *TempBuffer,
    _In_range_(<=, EndIndex)        uint8_t         StartIndex,
    _In_range_(>=, StartIndex)      uint8_t         EndIndex,
    _In_                            uint16_t        RecvChkSum
    );

/* Helpers for Poll Related Operations*/
static
NFCSTATUS
phFriNfc_Felica_HPollCard(
    _Inout_             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(2) const uint8_t       sysCode[],
    _In_    uint8_t     state
    );

static
NFCSTATUS
phFriNfc_Felica_HUpdateManufIdDetails(
    _In_ const phFriNfc_NdefMap_t *NdefMap
    );

/*Helpers for Reading Operations*/
static
NFCSTATUS
phFriNfc_Felica_HReadData(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             offset
    );

static
uint16_t
phFriNfc_Felica_HGetMaximumBlksToRead(
    _In_ const phFriNfc_NdefMap_t    *NdefMap,
    _In_ uint8_t                     NbcOrNmaxb
    );

static
void
phFriNfc_Felica_HAfterRead_CopyDataToBuff(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HSetTransceiveForRead(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint16_t            TrxLen,
    _In_    uint8_t             Offset
    );

static
uint16_t
phFriNfc_Felica_HSetTrxLen(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint16_t            Nbc
    );

static
NFCSTATUS
phFriNfc_Felica_HChkApduBuff_Size(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

/* Helpers for Writing Operations*/
static
NFCSTATUS
phFriNfc_Felica_HChkAttrBlkForWrOp(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HChkAttrBlkForRdOp(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint32_t            NdefLen
    );

static
NFCSTATUS
phFriNfc_Felica_HUpdateAttrBlkForWrOp(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             isStarted
    );

static
NFCSTATUS
phFriNfc_Felica_HUpdateData(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Felica_HWriteDataBlk(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

/* Write Empty NDEF Message*/
static
NFCSTATUS
phFriNfc_Felica_HWrEmptyMsg(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

/*Helpers for common checks*/
static
NFCSTATUS
phFriNfc_Felica_HCheckManufId(
    _In_ const phFriNfc_NdefMap_t *NdefMap
    );

static
void
phFriNfc_Felica_HCrHandler(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             CrIndex,
    _In_    NFCSTATUS           Status
    );

static
void
phFriNfc_Felica_HInitInternalBuf(
    _Out_writes_bytes_all_(16) uint8_t *Buffer
    );

static
int
phFriNfc_Felica_MemCompare (
    _In_reads_bytes_(n) void            *s1,
    _In_reads_bytes_(n) void            *s2,
    _In_                unsigned int    n
    );

/*!
 * \brief returns maximum number of blocks can be read from the Felica Smart Card.
 *
 * The function is useful in reading of NDEF information from a felica tag.
 */

static
uint16_t
phFriNfc_Felica_HGetMaximumBlksToRead(
    _In_ const phFriNfc_NdefMap_t    *NdefMap,
    _In_ uint8_t                     NbcOrNmaxb
    )
{
   uint16_t    BlksToRead=0;
   uint32_t    DataLen = 0;

   PH_LOG_NDEF_FUNC_ENTRY();

    /* This part of the code is useful if we take account of Nbc blks reading*/
    if ( NbcOrNmaxb == PH_NFCFRI_NDEFMAP_FELI_NBC )
    {
        PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                DataLen);
        /* Calculate Nbc*/
        BlksToRead = (uint16_t) ( ((DataLen % 16) == 0) ? (DataLen >> 4) : ((DataLen >> 4) +1) );


    }
    else if ( NbcOrNmaxb == PH_NFCFRI_NDEFMAP_FELI_NMAXB)
    {
        BlksToRead = NdefMap->FelicaAttrInfo.Nmaxb;
    }
    else
    {
        /* WARNING !!! code should not reach this point*/
        ;
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (BlksToRead);
}

NFCSTATUS
phFriNfc_Felica_RdNdef(
    _Inout_                                                     phFriNfc_NdefMap_t  *NdefMap,
    _Out_writes_bytes_to_(*PacketDataLength, *PacketDataLength) uint8_t             *PacketData,
    _Inout_                                                     uint32_t            *PacketDataLength,
    _In_                                                        uint8_t             Offset
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint32_t  Nbc = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    NdefMap->ApduBufferSize = *PacketDataLength;
    /*Store the packet data buffer*/
    NdefMap->ApduBuffer = PacketData;

    NdefMap->NumOfBytesRead = PacketDataLength ;
    *NdefMap->NumOfBytesRead = 0;
    NdefMap->ApduBuffIndex = 0;

    NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_READ_OPE;
    NdefMap->Felica.Offset = Offset;

    if( ( Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )||( NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_WRITE_OPE))
    {
        NdefMap->Felica.CurBlockNo = 0;
        NdefMap->Felica.OpFlag = PH_FRINFC_NDEFMAP_FELI_RD_ATTR_RD_OP;
        NdefMap->Felica.IntermediateCpyFlag = FALSE;
        NdefMap->Felica.IntermediateCpyLen = 0;
        NdefMap->Felica.Rd_NoBytesToCopy = 0;
        NdefMap->Felica.EofCardReachedFlag= FALSE ;
        NdefMap->Felica.LastBlkReachedFlag = FALSE;
        NdefMap->Felica.CurrBytesRead = 0;

        phFriNfc_Felica_HInitInternalBuf(NdefMap->Felica.Rd_BytesToCopyBuff);

        /* send request to read attribute information*/
        if(NdefMap->bDtaFlag)
        {
            status = phFriNfc_Felica_HNdefRdAttrInfo(NdefMap);
        }
        else
        {
            status = phFriNfc_Felica_HRdAttrInfo(NdefMap);
        }

        /* handle the error in Transc function*/
        if ( (status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
        {
            /* call respective CR */
            phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_CHK_NDEF,status);
        }
    }
    else
    {
         Nbc = phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC);

         /* Offset = Current, but the read has reached the End of NBC Blocks */
        if(( ( Offset == PH_FRINFC_NDEFMAP_SEEK_CUR) && (NdefMap->Felica.CurBlockNo == Nbc)) &&
            (NdefMap->Felica.EofCardReachedFlag == FELICA_RD_WR_EOF_CARD_REACHED ))
        {
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,  NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
        }
        else
        {

            NdefMap->Felica.CurrBytesRead = ((NdefMap->Felica.CurBlockNo * 16)- NdefMap->Felica.Rd_NoBytesToCopy);
            status = phFriNfc_Felica_HReadData(NdefMap,NdefMap->Felica.Offset);

        }
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HReadData(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             offset
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint16_t Nbc=0,TranscLen=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    Nbc = phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC);
    if( ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) > 0) && (NdefMap->Felica.CurBlockNo < Nbc ))
    {
        /* if data is present in the internal buffer*/
        if (NdefMap->Felica.Rd_NoBytesToCopy > 0 )
        {
            /* copy data to external buffer*/
            phFriNfc_Felica_HAfterRead_CopyDataToBuff(NdefMap);
            /*Check the size of user buffer*/
            status = phFriNfc_Felica_HChkApduBuff_Size(NdefMap);
            if ( (status != NFCSTATUS_SUCCESS) && (NdefMap->Felica.IntermediateRdFlag == TRUE ))
            {
                /* set the transc len and call transc cmd*/
                TranscLen = phFriNfc_Felica_HSetTrxLen(NdefMap,Nbc);
                status= phFriNfc_Felica_HSetTransceiveForRead(NdefMap,TranscLen,offset);
            }
            else
            {
                /* Nothing to be done , if IntermediateRdFlag is set to zero*/
                ;
            }
        }
        else
        {
            /* set the transc len and call transc cmd*/
            TranscLen = phFriNfc_Felica_HSetTrxLen(NdefMap,Nbc);
            status= phFriNfc_Felica_HSetTransceiveForRead(NdefMap,TranscLen,offset);
        }
    }
    else
    {
        /* Chk the Buffer size*/
        status = phFriNfc_Felica_HChkApduBuff_Size(NdefMap);
        if ( (status != NFCSTATUS_SUCCESS) && (NdefMap->Felica.IntermediateRdFlag == TRUE ))
        {
            TranscLen = phFriNfc_Felica_HSetTrxLen(NdefMap,Nbc);
            status= phFriNfc_Felica_HSetTransceiveForRead(NdefMap,TranscLen,offset);
        }
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
uint16_t
phFriNfc_Felica_HSetTrxLen(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint16_t            Nbc
    )
{
    uint16_t TranscLen = 0,BlocksToRead=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    if( ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)% 16) == 0)
    {
        BlocksToRead = (uint16_t)( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)/16 );
    }
    else
    {
        BlocksToRead = (uint16_t)(((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)/16) +1);
    }
    if ( (BlocksToRead > Nbc) ||( (BlocksToRead) > ( Nbc - NdefMap->Felica.CurBlockNo)) )
    {
        BlocksToRead = Nbc - NdefMap->Felica.CurBlockNo;
    }


    if ( BlocksToRead >= NdefMap->FelicaAttrInfo.Nbr)
    {
        if( NdefMap->FelicaAttrInfo.Nbr < Nbc )
        {
            TranscLen =  NdefMap->FelicaAttrInfo.Nbr*16;
        }
        else
        {
            TranscLen = Nbc*16;
            NdefMap->Felica.LastBlkReachedFlag =1;
        }
    }
    else
    {
        if (BlocksToRead <= Nbc )
        {
            if ( ( BlocksToRead * 16) == ((Nbc *16) -  (NdefMap->Felica.CurBlockNo * 16)))
            {
                NdefMap->Felica.LastBlkReachedFlag =1;

            }
            TranscLen = BlocksToRead*16;

        }
        else
        {
            TranscLen = Nbc*16;
        }
    }
    /* As Cur Blk changes, to remember the exact len what we had set
    in the begining of each read operation*/
    NdefMap->Felica.TrxLen = TranscLen;

    PH_LOG_NDEF_FUNC_EXIT();
    return (TranscLen);
}

static
void
phFriNfc_Felica_HAfterRead_CopyDataToBuff(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    uint8_t ResetFlag = FALSE, ExtrBytesToCpy = FALSE;
    uint16_t Nbc=0;
    uint32_t DataLen=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    Nbc = phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC );

    PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                DataLen);
    /* Internal Buffer has some old read bytes to cpy to user buffer*/
    if( NdefMap->Felica.Rd_NoBytesToCopy > 0 )
    {
        if ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) < NdefMap->Felica.Rd_NoBytesToCopy )
        {
                NdefMap->Felica.Rd_NoBytesToCopy -= (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);

                if (NdefMap->Felica.IntermediateCpyFlag == TRUE )
                {
                    /*Copy data from the internal buffer to user buffer*/
                     (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                    (&(NdefMap->Felica.Rd_BytesToCopyBuff[NdefMap->Felica.IntermediateCpyLen])),
                                    (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));



                    /* Store number of bytes copied frm internal buffer to User Buffer */
                    NdefMap->Felica.IntermediateCpyLen += (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                    NdefMap->Felica.IntermediateCpyFlag = 1;

                    /* check do we reach len bytes any chance*/
                    PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                    NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                    NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                    DataLen);
                    /* Internal buffer has zero bytes for copy operation*/
                    if ( NdefMap->Felica.Rd_NoBytesToCopy == 0)
                    {
                        NdefMap->Felica.EofCardReachedFlag =FELICA_RD_WR_EOF_CARD_REACHED;
                    }
                }
                else
                {
                    /*Copy data from the internal buffer to apdu buffer*/
                     (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                    NdefMap->Felica.Rd_BytesToCopyBuff,
                                    (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));
                }
                NdefMap->ApduBuffIndex += (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);

        }
        else if ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) == NdefMap->Felica.Rd_NoBytesToCopy )
        {
            if ( NdefMap->Felica.IntermediateCpyFlag == TRUE )
            {
                /*Copy data internal buff to apdubuffer*/
                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                               (&(NdefMap->Felica.Rd_BytesToCopyBuff[NdefMap->Felica.IntermediateCpyLen])),
                               NdefMap->Felica.Rd_NoBytesToCopy);
            }
            else
            {
                /*Copy data internal buff to apdubuffer*/
                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                NdefMap->Felica.Rd_BytesToCopyBuff,
                                NdefMap->Felica.Rd_NoBytesToCopy);
            }

            /*increment the index,internal buffer len*/
            NdefMap->ApduBuffIndex += NdefMap->Felica.Rd_NoBytesToCopy;
            NdefMap->Felica.Rd_NoBytesToCopy -= (uint8_t)(NdefMap->ApduBuffIndex);

            /* To reset the parameters*/
            ResetFlag = TRUE;
        }
        else
        {
            /* Extra Bytes to Copy from internal buffer to external buffer*/
            if ( NdefMap->Felica.IntermediateCpyFlag == TRUE )
            {
                /*Copy data internal buff to apdubuffer*/
                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                               (&(NdefMap->Felica.Rd_BytesToCopyBuff[NdefMap->Felica.IntermediateCpyLen])),
                               NdefMap->Felica.Rd_NoBytesToCopy);
            }
            else
            {
                /*Copy data internal buff to apdubuffer*/
                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                NdefMap->Felica.Rd_BytesToCopyBuff,
                                NdefMap->Felica.Rd_NoBytesToCopy);
            }
            /*increment the index*/
            NdefMap->ApduBuffIndex += NdefMap->Felica.Rd_NoBytesToCopy;

            /* To reset the parameters*/
            ResetFlag = TRUE;
        }
    }/*End of Internal Buffer has some old read bytes to cpy to user buffer*/
    else
    {
        /* check if last block is reached*/
        if ( ((NdefMap->Felica.LastBlkReachedFlag == 1) && (( NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= 16)) )
        {
            /* greater than 16 but less than the data len size*/
            if (( NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= DataLen)
            {
                NdefMap->Felica.CurrBytesRead = (uint16_t)((DataLen) - (NdefMap->Felica.CurrBytesRead +
                                                    NdefMap->ApduBuffIndex));

                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                               (&(NdefMap->SendRecvBuf[13])),
                                NdefMap->Felica.CurrBytesRead);

                NdefMap->ApduBuffIndex += NdefMap->Felica.CurrBytesRead;
                if ( NdefMap->ApduBuffIndex == DataLen)
                {
                    ResetFlag = TRUE;
                }
            }
            else
            {
                /* need to check exact no. of bytes to copy to buffer*/
                if( ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) <= NdefMap->Felica.TrxLen )||
                    ((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) <= DataLen ))
                {

                    ExtrBytesToCpy = TRUE;
                }
                else
                {
                    NdefMap->Felica.Rd_NoBytesToCopy = (uint8_t)(16-(( Nbc * 16) - (DataLen)));

                    if ( NdefMap->Felica.Rd_NoBytesToCopy > (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex))
                    {
                        /*Reduce already copied bytes from the internal buffer*/
                        NdefMap->Felica.Rd_NoBytesToCopy -= (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                        ExtrBytesToCpy = TRUE;
                    }
                    else
                    {
                        ExtrBytesToCpy = FALSE;
                    }
                }
                if ( ExtrBytesToCpy == TRUE )
                {
                    NdefMap->Felica.CurrBytesRead = (uint16_t)((DataLen)- (NdefMap->Felica.CurrBytesRead +
                                                    NdefMap->ApduBuffIndex));

                    if(NdefMap->Felica.CurrBytesRead <
                        (uint16_t)(NdefMap->ApduBufferSize -
                        NdefMap->ApduBuffIndex))
                    {
                         (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                       (&(NdefMap->SendRecvBuf[13])),
                                       NdefMap->Felica.CurrBytesRead);
                    }
                    else
                    {
                         (void)phOsalNfc_MemCopy( (&( NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                       (&( NdefMap->SendRecvBuf[13])),
                                        (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));
                    }

                    if ( NdefMap->Felica.LastBlkReachedFlag == 1 )
                    {
                        NdefMap->Felica.Rd_NoBytesToCopy =
                                    (uint8_t)((NdefMap->Felica.CurrBytesRead >
                                    (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex))?
                                    (NdefMap->Felica.CurrBytesRead -
                                    (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)):
                                    0);

                        ResetFlag = ((NdefMap->Felica.Rd_NoBytesToCopy == 0)?TRUE:FALSE);

                    }
                    else
                    {
                        NdefMap->Felica.Rd_NoBytesToCopy = (uint8_t)( NdefMap->Felica.TrxLen - (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));
                    }

                    /* Copy remained bytes back into internal buffer*/
                     (void)phOsalNfc_MemCopy(   NdefMap->Felica.Rd_BytesToCopyBuff,
                                  (&(NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_RESP_HEADER_LEN+(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)])),
                                     NdefMap->Felica.Rd_NoBytesToCopy);

                    /* set the intermediate flag : This flag remembers that there are still X no. bytes remained in
                    Internal Buffer Ex: User has given only one byte buffer,needs to cpy one byte at a time*/
                    NdefMap->Felica.IntermediateCpyFlag = TRUE;

                    NdefMap->ApduBuffIndex += ((NdefMap->Felica.CurrBytesRead <
                                                (uint16_t)(NdefMap->ApduBufferSize -
                                                NdefMap->ApduBuffIndex))?
                                                NdefMap->Felica.CurrBytesRead:
                                                (uint16_t)(NdefMap->ApduBufferSize -
                                                NdefMap->ApduBuffIndex));
                }
                else
                {
                    /*Copy data from the internal buffer to user buffer*/
                    (void)phOsalNfc_MemCopy( (&( NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                  (&( NdefMap->SendRecvBuf[13])),
                                   NdefMap->Felica.Rd_NoBytesToCopy);

                    NdefMap->ApduBuffIndex += NdefMap->Felica.Rd_NoBytesToCopy;
                    ResetFlag = TRUE;

                }
            }

        }
        else
        {
            if ((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) < NdefMap->Felica.TrxLen )
            {
                /* Calculate exactly remained bytes to copy to internal buffer and set it*/
                if ( NdefMap->Felica.LastBlkReachedFlag == 1)
                {
                    NdefMap->Felica.Rd_NoBytesToCopy = (uint8_t)(16-(( Nbc * 16) - DataLen));

                    if ( NdefMap->Felica.Rd_NoBytesToCopy > (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex))
                    {
                        /*Reduce already copied bytes from the internal buffer*/
                        NdefMap->Felica.Rd_NoBytesToCopy -= (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                        ExtrBytesToCpy = TRUE;
                    }
                }
                else
                {
                    NdefMap->Felica.Rd_NoBytesToCopy = (uint8_t)(NdefMap->Felica.TrxLen - (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));
                    ExtrBytesToCpy = TRUE;
                }
                if ( ExtrBytesToCpy == TRUE )
                {
                    /*Copy the read data from trx buffer to apdu of size apdu*/
                     (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                   (&(NdefMap->SendRecvBuf[13])),
                                    NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);

                    /*copy bytesToCopy to internal buffer*/
                     (void)phOsalNfc_MemCopy( NdefMap->Felica.Rd_BytesToCopyBuff,
                         (&(NdefMap->SendRecvBuf[13+(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)])),
                                    NdefMap->Felica.Rd_NoBytesToCopy);

                    NdefMap->Felica.IntermediateCpyFlag = TRUE;
                    NdefMap->ApduBuffIndex += (uint16_t)NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex;
                }
                else
                {
                    /*Copy data from the internal buffer to user buffer*/
                     (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                   (&(NdefMap->SendRecvBuf[13])),
                                    NdefMap->Felica.Rd_NoBytesToCopy);

                    NdefMap->ApduBuffIndex += NdefMap->Felica.Rd_NoBytesToCopy;
                    ResetFlag = TRUE;

                }
                if ( DataLen <= (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) )
                {
                        NdefMap->Felica.EofCardReachedFlag =FELICA_RD_WR_EOF_CARD_REACHED;
                }
                else
                {
                ;
                }
            }
            else if ((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) == NdefMap->Felica.TrxLen )
            {
                /*Copy exactly remained last bytes to user buffer and increment the index*/
                /*13 : 1+12 : 1st byte entire pkt length + 12 bytes to skip manuf details*/
                 (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                               (&(NdefMap->SendRecvBuf[13])),
                               (NdefMap->Felica.TrxLen ));

                NdefMap->ApduBuffIndex += NdefMap->Felica.TrxLen;
            }
            else
            {
                if ((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) > NdefMap->Felica.TrxLen )
                {
                        /*Copy the data to apdu buffer and increment the index */
                         (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                       (&(NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_RESP_HEADER_LEN])),
                                        NdefMap->Felica.TrxLen);

                        NdefMap->ApduBuffIndex += (uint16_t)NdefMap->Felica.TrxLen;
                }
            }
        }
    }
    if ( ResetFlag == TRUE )
    {
        /* reset the internal buffer variables*/
        NdefMap->Felica.Rd_NoBytesToCopy =0;
        NdefMap->Felica.IntermediateCpyLen =0;
        NdefMap->Felica.IntermediateCpyFlag =FALSE;
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return;
}

static
NFCSTATUS
phFriNfc_Felica_HChkApduBuff_Size(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint8_t ResetFlag = FALSE;
    uint32_t Nbc = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    Nbc = phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC);

    /* set status to Success : User Buffer is full and Curblk < nmaxb*/
    if ( (( NdefMap->ApduBufferSize-NdefMap->ApduBuffIndex )== 0) &&
         (NdefMap->Felica.CurBlockNo < Nbc ))
    {
        status = PHNFCSTVAL(CID_NFC_NONE,
                                       NFCSTATUS_SUCCESS);
        /*Reset the index, internal buffer counters back to zero*/
        *NdefMap->NumOfBytesRead = NdefMap->ApduBuffIndex;
        NdefMap->ApduBuffIndex = 0;

    }/*if( (NdefMap->ApduBufferSize-NdefMap->ApduBuffIndex )== 0 && NdefMap->Felica.CurBlockNo < NdefMap->FelicaAttrInfo.Nmaxb )*/
    else
    {
        if (( ( NdefMap->ApduBufferSize-NdefMap->ApduBuffIndex )== 0) &&
             (NdefMap->Felica.CurBlockNo == Nbc ))
        {
            status = PHNFCSTVAL(CID_NFC_NONE,
                                       NFCSTATUS_SUCCESS);

            ResetFlag = ((NdefMap->Felica.Rd_NoBytesToCopy > 0 )?
                            FALSE:
                            TRUE);
            if( ResetFlag== FALSE)
            {
                *NdefMap->NumOfBytesRead = NdefMap->ApduBuffIndex;
                /*Reset the index, internal buffer counters back to zero*/
                NdefMap->ApduBuffIndex = 0;
            }
        }
        else
        {
            /* reached reading all the blks available in the card: set EOF flag*/
            if ( NdefMap->ApduBuffIndex == (Nbc*16))
            {
                status = PHNFCSTVAL(CID_NFC_NONE,
                                    NFCSTATUS_SUCCESS);
                ResetFlag = TRUE;
            }
            else
            {
                if ((NdefMap->ApduBufferSize-NdefMap->ApduBuffIndex )> 0 )
                {
                    if ( NdefMap->Felica.CurBlockNo == Nbc )
                    {
                        /* bytes pending in internal buffer , No Space in User Buffer*/
                        if ( NdefMap->Felica.Rd_NoBytesToCopy > 0)
                        {
                            if ( NdefMap->Felica.EofCardReachedFlag == TRUE )
                            {
                                status = PHNFCSTVAL(CID_NFC_NONE,
                                NFCSTATUS_SUCCESS);
                                *NdefMap->NumOfBytesRead = NdefMap->ApduBuffIndex;
                                NdefMap->ApduBuffIndex=0;
                            }
                            else
                            {
                                phFriNfc_Felica_HAfterRead_CopyDataToBuff(NdefMap);
                                if( NdefMap->Felica.Rd_NoBytesToCopy > 0 )
                                {
                                    status = PHNFCSTVAL(CID_NFC_NONE,
                                        NFCSTATUS_SUCCESS);
                                    *NdefMap->NumOfBytesRead = NdefMap->ApduBuffIndex;
                                    NdefMap->ApduBuffIndex=0;
                                }
                                else
                                {
                                    /* EOF Card Reached set the internal EOF Flag*/
                                    status = PHNFCSTVAL(CID_NFC_NONE,
                                       NFCSTATUS_SUCCESS);

                                    ResetFlag = TRUE;
                                }
                            }
                        }
                        /* All bytes from internal buffer are copied and set eof flag*/
                        else
                        {
                               status = PHNFCSTVAL(CID_NFC_NONE,
                                      NFCSTATUS_SUCCESS);
                               ResetFlag = TRUE;
                         }
                    }
                    else
                    {
                        /* This flag is set to ensure that, need of Read Opearation
                        we completed coying the data from internal buffer to external buffer
                        left some more bytes,in User bufer so initiate the read operation */
                        NdefMap->Felica.IntermediateRdFlag = TRUE;
                    }
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_NONE,
                                       NFCSTATUS_SUCCESS);
                }
            }
        }
        if ( ResetFlag == TRUE)
        {
            *NdefMap->NumOfBytesRead = NdefMap->ApduBuffIndex;
                                /*Reset the index, internal buffer counters back to zero*/
                                NdefMap->ApduBuffIndex = 0;
                                NdefMap->Felica.Rd_NoBytesToCopy=0;
                                NdefMap->Felica.EofCardReachedFlag=FELICA_RD_WR_EOF_CARD_REACHED;

        }

    }

    PH_LOG_NDEF_FUNC_EXIT();
    return( status);
}

static
NFCSTATUS
phFriNfc_Felica_HSetTransceiveForRead(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint16_t            TrxLen,
    _In_    uint8_t             Offset
    )
{
    NFCSTATUS TrxStatus =  NFCSTATUS_PENDING;
    uint16_t BufIndex=0,i=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* set the felica cmd */
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /*Change the state to Read */
    NdefMap->State = PH_NFCFRI_NDEFMAP_FELI_STATE_RD_BLOCK;

    /* set the complition routines for the mifare operations */
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /* pkt len : updated at the end*/
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;
    BufIndex ++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x06;
    BufIndex++;

    /* IDm - Manufacturer Id : 8bytes*/
    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                  (void * )(&(NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm)),8);

    BufIndex+=8;

    /*Number of Services (n=1 ==> 0x80)*/
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;
    BufIndex++;

    /*Service Code List*/
    NdefMap->SendRecvBuf[BufIndex]    =   0x0B;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;
    BufIndex++;

    /*Number of Blocks to read*/
    NdefMap->SendRecvBuf[BufIndex]    =  (uint8_t)(TrxLen/16);
    BufIndex++;
    /* Set the Blk numbers as per the offset set by the user : Block List*/
    if ( Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )
    {
        for ( i=0;i<(TrxLen/16);i++)
        {
            /*1st Service Code list : byte 1*/
            NdefMap->SendRecvBuf[BufIndex]    =   0x80;
            BufIndex++;

            /* No. Of Blocks*/
            NdefMap->SendRecvBuf[BufIndex]    =   (uint8_t)(i + 1);
            BufIndex++;
        }
    }
    else
    {
        for ( i= 1;i<=(TrxLen/16);i++)
        {
            /*1st Service Code list : byte 1*/
            NdefMap->SendRecvBuf[BufIndex]    =   0x80;
            BufIndex++;

            /* No. Of Blocks*/
            NdefMap->SendRecvBuf[BufIndex]    =   (uint8_t)(NdefMap->Felica.CurBlockNo + i);
            BufIndex++;
        }
    }

    /* len of entire pkt*/
    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX]          =  (uint8_t) BufIndex;

    /* Set the Pkt Len*/
    NdefMap->SendLength = BufIndex;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    TrxStatus = phFriNfc_OvrHal_Transceive(NdefMap->LowerDevice,
                                        &NdefMap->MapCompletionInfo,
                                        NdefMap->psRemoteDevInfo,
                                        NdefMap->Cmd,
                                        &NdefMap->psDepAdditionalInfo,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendLength,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (TrxStatus);
}

NFCSTATUS
phFriNfc_Felica_WrNdef(
    _Inout_                             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(*PacketDataLength) uint8_t             *PacketData,
    _Inout_                             uint32_t            *PacketDataLength,
    _In_                                uint8_t             Offset
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    PH_LOG_NDEF_FUNC_ENTRY();

    NdefMap->ApduBufferSize = *PacketDataLength;
    /*Store the packet data buffer*/
    NdefMap->ApduBuffer = PacketData;

    /* To Update the Acutal written bytes to context*/
    NdefMap->WrNdefPacketLength = PacketDataLength;
    *NdefMap->WrNdefPacketLength = 0;


    NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_WRITE_OPE;
    NdefMap->Felica.Offset = Offset;

    NdefMap->Felica.OpFlag = PH_FRINFC_NDEFMAP_FELI_WR_ATTR_RD_OP;

    if(NdefMap->bDtaFlag)
    {
        status = phFriNfc_Felica_HNdefWrAttrInfo(NdefMap);
    }
    else
    {
        status = phFriNfc_Felica_HRdAttrInfo(NdefMap);
    }

    /* handle the error in Transc function*/
    if ( (status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
    {
        /* call respective CR */
        phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_CHK_NDEF,status);
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

NFCSTATUS
phFriNfc_Felica_EraseNdef(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             *pDummy1,
    _In_    uint32_t            *pDummy2
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    static uint32_t PktDtLength =0;

    PH_LOG_NDEF_FUNC_ENTRY();
    UNUSED(pDummy1);
    UNUSED(pDummy2);
    if ( NdefMap->CardState == PH_NDEFMAP_CARD_STATE_INVALID )
    {
        /*  Card is in invalid state, cannot have any read/write
        operations*/
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
            NFCSTATUS_INVALID_FORMAT);
    }
    else if ( NdefMap->CardState == PH_NDEFMAP_CARD_STATE_READ_ONLY )
    {
        /*Can't write to the card :No Grants */
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
            NFCSTATUS_WRITE_FAILED);
        /* set the no. bytes written is zero*/
        NdefMap->WrNdefPacketLength = &PktDtLength;
        *NdefMap->WrNdefPacketLength = 0;
    }
    else
    {

        /* set the Operation*/
        NdefMap->Felica.OpFlag = PH_FRINFC_NDEFMAP_FELI_WR_EMPTY_MSG_OP;

        if(NdefMap->bDtaFlag)
        {
            status = phFriNfc_Felica_HNdefWrAttrInfo(NdefMap);
        }
        else
        {
            status = phFriNfc_Felica_HRdAttrInfo(NdefMap);
        }
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

NFCSTATUS
phFriNfc_Felica_HReset(
    _Inout_ phFriNfc_NdefMap_t      *NdefMap,
    _In_    phNfc_sDevInputParam_t  *pDevInpParam
    )
{
    NFCSTATUS Result = NFCSTATUS_SUCCESS;
    uint8_t index=0;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(NdefMap == NULL)
    {
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        NdefMap->FelicaPollDetails.DevInputParam = pDevInpParam;
        /* Initialise/reset the different felica structure variables*/
        /* Reset all the felica Basic staruture variables*/
        NdefMap->Felica.CurBlockNo = 0;

        for(index = 0;index<PH_FRINFC_NDEFMAP_FELICA_BLOCK_SIZE;index++)
        {
            NdefMap->Felica.Rd_BytesToCopyBuff[index] = 0;
            NdefMap->Felica.Wr_RemainedBytesBuff[index] = 0;
        }
        NdefMap->Felica.Rd_NoBytesToCopy = 0;
        NdefMap->Felica.Wr_BytesRemained = 0;

        /* Reset all the felica attribute information staruture variables*/
        for(index = 0;index<PH_FRINFC_NDEFMAP_FELICA_ATTR_NDEF_DATA_LEN;index++)
        {
            NdefMap->FelicaAttrInfo.LenBytes[index] = 0;
        }

        NdefMap->FelicaAttrInfo.Nmaxb = 0;
        NdefMap->FelicaAttrInfo.Nbr = 0;
        NdefMap->FelicaAttrInfo.Nbw= 0;
        NdefMap->FelicaAttrInfo.RdWrFlag = 0;
        NdefMap->FelicaAttrInfo.WriteFlag = 0;
        NdefMap->Felica.CurrBytesRead=0;

        /* Reset all the felica manufacture details staruture variables*/
        for(index = 0;index<PH_FRINFC_NDEFMAP_FELICA_MANUF_ID_DATA_LEN;index++)
        {
            NdefMap->FelicaManufDetails.ManufID[index] = 0;
            NdefMap->FelicaManufDetails.ManufParameter[index] = 0;
        }
        NdefMap->Felica.NoBlocksWritten=0;
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

NFCSTATUS
phFrinfc_Felica_GetContainerSize(
    _In_    const phFriNfc_NdefMap_t    *NdefMap,
    _Out_   uint32_t                    *maxSize,
    _Out_   uint32_t                    *actualSize
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NULL != NdefMap) && (NULL != maxSize) &&(NULL != actualSize))
    {
        /*  Felica card */
        *maxSize = NdefMap->FelicaAttrInfo.Nmaxb * 0x10;

        /* In Felica Card, actual size is calculated using the Length Bytes */
        *actualSize = NdefMap->FelicaAttrInfo.LenBytes[0];
        *actualSize = *actualSize << 8;
        *actualSize += NdefMap->FelicaAttrInfo.LenBytes[1];
        *actualSize = *actualSize << 8;
        *actualSize += NdefMap->FelicaAttrInfo.LenBytes[2];
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                        NFCSTATUS_INVALID_PARAMETER);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return wStatus;
}

static
NFCSTATUS
phFriNfc_Felica_HUpdateAttrBlkForWrOp(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             isStarted
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;

    uint16_t ChkSum=0,index=0;
    uint8_t BufIndex=0, ErrFlag = FALSE;
    uint32_t TotNoWrittenBytes=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Write Operation : Begin/End Check*/

    NdefMap->State =
    (( isStarted == FELICA_WRITE_STARTED )?
    PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN:
    PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_END);

    if( ( NdefMap->State == PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN)||
        ( NdefMap->State == PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_END) )
    {

        /* Set the Felica Cmd*/
        NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

        /* 1st byte represents the length of the cmd packet*/
        NdefMap->SendRecvBuf[BufIndex] = 0x00;
        BufIndex++;

        /* Write/Update command code*/
        NdefMap->SendRecvBuf[BufIndex] = 0x08;
        BufIndex++;

        /* IDm - Manufacturer Id : 8bytes*/
        (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                    (void*)(&(NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm)),8);

        BufIndex+=8;
        NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Services (n=1 ==> 0x80)*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x09;  /*  Service Code List*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  Service Code List*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Blocks to Write*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x80;  /*  1st Block Element : byte 1*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  1st Block Element : byte 2, block 1*/
        BufIndex++;

        /* Fill Attribute Blk Information*/
        NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Version;
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Nbr;
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Nbw;
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)((NdefMap->FelicaAttrInfo.Nmaxb) >> 8);
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)((NdefMap->FelicaAttrInfo.Nmaxb) & (0x00ff));
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
        BufIndex++;

        if (isStarted == FELICA_WRITE_STARTED )
        {
            NdefMap->SendRecvBuf[BufIndex]  = 0x0F; /* Write Flag Made On*/
            BufIndex++;

            NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.RdWrFlag; /* Read write flag*/
            BufIndex++;

            /* Len Bytes*/
            NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.LenBytes[0];
            BufIndex++;

            NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.LenBytes[1];
            BufIndex++;

            NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.LenBytes[2];
            BufIndex++;
        }
        else
        {
            /* Case: Previous Write Operation failed and integration context continues with write
            operation with offset set to Current. In this case, if we find Internal Bytes remained in the
            felica context is true(>0) and current block number is Zero. Then we shouldn't allow the module
            to write the data to card, as this is a invalid case*/
            if ( (NdefMap->Felica.Wr_BytesRemained > 0) && (NdefMap->Felica.CurBlockNo == 0))
            {
                status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                        NFCSTATUS_INVALID_PARAMETER);
                ErrFlag = TRUE;
            }
            else
            {

                NdefMap->SendRecvBuf[BufIndex]  = 0x00; /* Write Flag Made Off*/
                BufIndex++;

                NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.RdWrFlag; /* Read write flag*/
                BufIndex++;

                if ( NdefMap->Felica.Wr_BytesRemained > 0 )
                {
                    TotNoWrittenBytes = ( (NdefMap->Felica.CurBlockNo *16)- (16 - (NdefMap->Felica.Wr_BytesRemained)));
                }
                else
                {
                    TotNoWrittenBytes = ( NdefMap->Felica.CurBlockNo *16);

                }

                /* Update Len Bytes*/
                NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)(( TotNoWrittenBytes & 0x00ff0000) >> 16);
                BufIndex++;

                NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)((TotNoWrittenBytes & 0x0000ff00) >> 8);
                BufIndex++;

                NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)(TotNoWrittenBytes & 0x000000ff);
                BufIndex++;
            }
        }

        if ( ErrFlag != TRUE )
        {
            /* check sum update*/
            for ( index = 16 ; index < 30 ; index ++)
            {
                ChkSum += NdefMap->SendRecvBuf[index];
            }

            /* fill check sum in command pkt*/
            NdefMap->SendRecvBuf[BufIndex] = (uint8_t)(ChkSum >> 8);
            BufIndex++;

            NdefMap->SendRecvBuf[BufIndex] = (uint8_t )(ChkSum & 0x00ff);
            BufIndex++;

            /* update length of the cmd pkt*/
            NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;

            *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

            /* Update the Send Len*/
            NdefMap->SendLength = BufIndex;

            /*set the completion routines for the desfire card operations*/
            NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;//phFriNfc_NdefMap_Process;
            NdefMap->MapCompletionInfo.Context = NdefMap;

            /*set the additional informations for the data exchange*/
            NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
            NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

            /*Call the Overlapped HAL Transceive function */
            status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                                &NdefMap->MapCompletionInfo,
                                                NdefMap->psRemoteDevInfo,
                                                NdefMap->Cmd,
                                                &NdefMap->psDepAdditionalInfo,
                                                NdefMap->SendRecvBuf,
                                                NdefMap->SendLength,
                                                NdefMap->SendRecvBuf,
                                                NdefMap->SendRecvLength);
        }
    }
    else
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                      NFCSTATUS_INVALID_PARAMETER);

    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HWrEmptyMsg(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;

    uint16_t ChkSum=0,index=0;
    uint8_t BufIndex=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Write Operation : To Erase the present NDEF Data*/

    NdefMap->State = PH_NFCFRI_NDEFMAP_FELI_STATE_WR_EMPTY_MSG;

    /* Set the Felica Cmd*/
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /* 1st byte represents the length of the cmd packet*/
    NdefMap->SendRecvBuf[BufIndex] = 0x00;
    BufIndex++;

    /* Write/Update command code*/
    NdefMap->SendRecvBuf[BufIndex] = 0x08;
    BufIndex++;

    /* IDm - Manufacturer Id : 8bytes*/
    (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                (void*)(&(NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm)),8);

    BufIndex+=8;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Services (n=1 ==> 0x80)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x09;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Blocks to Write*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x80;  /*  1st Block Element : byte 1*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  1st Block Element : byte 2, block 1*/
    BufIndex++;

    /* Fill Attribute Blk Information*/
    NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Version;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Nbr;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.Nbw;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)((NdefMap->FelicaAttrInfo.Nmaxb) >> 8);
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = (uint8_t)((NdefMap->FelicaAttrInfo.Nmaxb) & (0x00ff));
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00; /*RFU*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.WriteFlag;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = NdefMap->FelicaAttrInfo.RdWrFlag; /* Read write flag*/
    BufIndex++;

    /* Len Bytes are set to 0 : Empty Msg*/
    NdefMap->SendRecvBuf[BufIndex]  = 0x00;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00;
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]  = 0x00;
    BufIndex++;

    /* check sum update*/
    for ( index = 16 ; index < 30 ; index ++)
    {
        ChkSum += NdefMap->SendRecvBuf[index];
    }

    /* fill check sum in command pkt*/
    NdefMap->SendRecvBuf[BufIndex] = (uint8_t)(ChkSum >> 8);
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex] = (uint8_t )(ChkSum & 0x00ff);
    BufIndex++;

    /* update length of the cmd pkt*/
    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Update the Send Len*/
    NdefMap->SendLength = BufIndex;

    /*set the completion routines for the desfire card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;//phFriNfc_NdefMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /*Call the Overlapped HAL Transceive function */
    status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                        &NdefMap->MapCompletionInfo,
                                        NdefMap->psRemoteDevInfo,
                                        NdefMap->Cmd,
                                        &NdefMap->psDepAdditionalInfo,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendLength,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HChkAttrBlkForWrOp(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint32_t DataLen=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /*check RW Flag Access Rights*/
    /* set to read only cannot write*/
    if ( NdefMap->FelicaAttrInfo.RdWrFlag == 0x00)

    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                       NFCSTATUS_INVALID_DEVICE_REQUEST);
    }
    else
    {
        if ( ( NdefMap->Felica.Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN) ||
                ( ( NdefMap->PrevOperation == PH_FRINFC_NDEFMAP_READ_OPE) &&
                   (NdefMap->Felica.Offset != PH_FRINFC_NDEFMAP_SEEK_BEGIN ) ))
        {
            /* check allready written number of bytes and apdu buffer size*/
            if (NdefMap->ApduBufferSize > (uint32_t)(NdefMap->FelicaAttrInfo.Nmaxb *16))
            {
                NdefMap->Felica.EofCardReachedFlag = FELICA_EOF_REACHED_WR_WITH_BEGIN_OFFSET;
            }
            else
            {
                NdefMap->Felica.EofCardReachedFlag = FALSE;
            }


            /* reset the internal variables initiate toupdate the attribute blk*/
            NdefMap->Felica.Wr_BytesRemained = 0;
            NdefMap->Felica.CurBlockNo = 0;
            NdefMap->Felica.NoBlocksWritten = 0;
            phFriNfc_Felica_HInitInternalBuf(NdefMap->Felica.Wr_RemainedBytesBuff);
            status= phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_STARTED);

        }
        else
        {
            if (NdefMap->Felica.Offset == PH_FRINFC_NDEFMAP_SEEK_CUR )
            {
                /* Calculate the Allready Written No. Of Blocks*/
                PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                DataLen);

                if (( NdefMap->ApduBufferSize + (DataLen )) >
                        (uint32_t)( NdefMap->FelicaAttrInfo.Nmaxb *16))
                {
                    if(( DataLen ) ==  (uint32_t)(NdefMap->FelicaAttrInfo.Nmaxb *16) )
                    {
                        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                     NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
                    }
                    else
                    {

                        NdefMap->Felica.EofCardReachedFlag =FELICA_EOF_REACHED_WR_WITH_CURR_OFFSET;
                        NdefMap->ApduBuffIndex =0;
                        NdefMap->Felica.NoBlocksWritten = 0;
                        status= phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_STARTED);
                    }
                }
                else
                {
                    NdefMap->ApduBuffIndex =0;
                    NdefMap->Felica.NoBlocksWritten = 0;
                    status= phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_STARTED);
                }
            }/*if (NdefMap->Felica.Offset == PH_FRINFC_NDEFMAP_SEEK_CUR )*/
        }
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HChkAttrBlkForRdOp(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint32_t            NdefLen
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;

    PH_LOG_NDEF_FUNC_ENTRY();

    /*check WR Flag Access Rights*/
    /* set to still writing data state only cannot Read*/
    if ( NdefMap->FelicaAttrInfo.WriteFlag == 0x0F )
    {
         status =  PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_READ_FAILED);
         /* As we are not able to continue with reading data
            bytes read set to zero*/
         *NdefMap->NumOfBytesRead = 0;
    }
    else
    {
        status = phFriNfc_MapTool_SetCardState( NdefMap,NdefLen);
        if ( status == NFCSTATUS_SUCCESS)
        {
            /* Read data From the card*/
            status = phFriNfc_Felica_HReadData(NdefMap,NdefMap->Felica.Offset);
        }
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HUpdateData(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    uint8_t BufIndex=0,
            i=0,
            BlkNo=0,
            PadBytes=0,
            CurBlk=1,
            NoOfBlks=0,
            NbwCheck=0,
            TotNoBlks=0;

    uint32_t BytesRemainedInCard=0,
             BytesRemained=0,
             TotNoWrittenBytes=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    if( (NdefMap->ApduBufferSize -  NdefMap->ApduBuffIndex) > 0 )
    {
        /* Prepare the write cmd pkt for felica*/
        /* 1st byte represents the length of the cmd packet*/
        NdefMap->SendRecvBuf[BufIndex] = 0x00;
        BufIndex++;

        /* Write/Update command code*/
        NdefMap->SendRecvBuf[BufIndex] = 0x08;
        BufIndex++;

        /* IDm - Manufacturer Id : 8bytes*/
        (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                     (&(NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm)),
                      8);

        BufIndex+=8;
        NdefMap->SendRecvBuf[BufIndex]    =   0x01;   /*  Number of Services (n=1 ==> 0x80)*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x09;   /*  Service Code List*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   0x00;   /* Service Code List*/
        BufIndex++;

        if ( NdefMap->Felica.EofCardReachedFlag == FELICA_EOF_REACHED_WR_WITH_BEGIN_OFFSET)
        {
            /* check for the eof card reached flag.Need to write only mamximum bytes(memory)to card.
            Used when, offset set to begin case*/
            BytesRemainedInCard= ( (NdefMap->FelicaAttrInfo.Nmaxb*16) - (NdefMap->Felica.CurBlockNo * 16));
        }
        else
        {
            /* Offset : Cuurent*/
            if ( NdefMap->Felica.EofCardReachedFlag == FELICA_EOF_REACHED_WR_WITH_CURR_OFFSET )
            {
                  /* caculate previously written Ndef blks*/
                 (void)phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC);

                 if ( NdefMap->Felica.Wr_BytesRemained  )
                 {
                    TotNoWrittenBytes = ( (NdefMap->Felica.CurBlockNo *16)- (16 - (NdefMap->Felica.Wr_BytesRemained)));
                 }
                 else
                 {
                    TotNoWrittenBytes = ( NdefMap->Felica.CurBlockNo *16);
                 }
                 /* Determine exactly, how many bytes we can write*/
                 BytesRemainedInCard = (NdefMap->FelicaAttrInfo.Nmaxb*16 - (TotNoWrittenBytes));
            }

        }
        /* Write Data Pending in the Internal Buffer*/
        if(NdefMap->Felica.Wr_BytesRemained > 0)
        {
            /* update the number of blocks to write with the block list elements*/
            /* Total Number of blocks to write*/
            NdefMap->SendRecvBuf[BufIndex]    =   0;
            BufIndex++;

            /* Update this Total no. Bloks later*/
            NoOfBlks = BufIndex;

            /* As we are writing atleast one block*/
            TotNoBlks = 1;

            /* check do we have some extra bytes to write? in User Buffer*/
            if ( NdefMap->ApduBufferSize >(uint32_t) (16 - NdefMap->Felica.Wr_BytesRemained))
            {
                /* Have we reached EOF?*/
                if ( NdefMap->Felica.EofCardReachedFlag )
                {
                    BytesRemained = BytesRemainedInCard;
                }
                else
                {
                    /* This value tells how many extra bytes we can write other than internal buffer bytes*/
                    BytesRemained = (uint8_t)NdefMap->ApduBufferSize - (16 - NdefMap->Felica.Wr_BytesRemained);
                }

                if ( BytesRemained )
                {
                    /* Not reached EOF*/
                    if (!NdefMap->Felica.EofCardReachedFlag)
                    {
                       /* Calculate How many blks we need to write*/
                        BlkNo =((uint8_t)( BytesRemained )/16);

                        /* check blocks to write exceeds nbw*/
                        if ( BlkNo >= NdefMap->FelicaAttrInfo.Nbw )
                        {
                            BlkNo = NdefMap->FelicaAttrInfo.Nbw;
                            /* No. Blks to write are more than Nbw*/
                            NbwCheck = 1;
                        }
                        else
                        {
                             if  ((( BytesRemained %16) == 0)&& (BlkNo == 0 ))
                            {
                                BlkNo=1;
                            }
                        }
                        /* check do we need pad bytes?*/
                        if( (!NbwCheck && (uint8_t)( BytesRemained)%16) != 0)
                        {
                            BlkNo++;
                            PadBytes = (BlkNo * 16) - (uint8_t)( BytesRemained);
                            NdefMap->Felica.PadByteFlag = TRUE;
                            NdefMap->Felica.NoBlocksWritten = BlkNo;
                            TotNoBlks += BlkNo;

                        }
                        else
                        {
                            if ( NbwCheck )
                            {
                                /* as we have to write only 8 blocks and already we have pad bytes so we have
                                to strat from previous block*/
                                TotNoBlks += BlkNo - 1;
                                NdefMap->Felica.NoBlocksWritten = TotNoBlks-1;
                            }
                            else
                            {
                                if ( !(BytesRemained - (16 -NdefMap->Felica.Wr_BytesRemained)== 0 ))
                                {
                                    TotNoBlks += BlkNo;
                                }
                                else
                                {

                                }
                                 if ( NdefMap->Felica.PadByteFlag )
                                {
                                    NdefMap->Felica.NoBlocksWritten = TotNoBlks-1;

                                }
                            }
                        }
                    }
                    else
                    {
                         /* we have reached the eof card & hv bytes to write*/
                        BlkNo =(uint8_t)(( BytesRemained - ((16 -NdefMap->Felica.Wr_BytesRemained)) )/16);

                        /* check are we exceeding the NBW limit, while a write?*/
                        if ( BlkNo >= NdefMap->FelicaAttrInfo.Nbw )
                        {
                            BlkNo = NdefMap->FelicaAttrInfo.Nbw;

                            /* No. Blks to write are more than Nbw*/
                            NbwCheck = 1;

                        }
                        else
                        {
                            if  ((( BytesRemained %16) == 0)&& (BlkNo == 0 ))
                            {
                                BlkNo=1;
                            }
                        }

                        /*check Total how many blocks to write*/
                        if(((!NbwCheck) &&( BytesRemained- (16 - NdefMap->Felica.Wr_BytesRemained))%16) != 0)
                        {
                            BlkNo++;
                            PadBytes = (BlkNo * 16) - (uint8_t)( BytesRemained);
                            NdefMap->Felica.PadByteFlag = TRUE;
                            NdefMap->Felica.NoBlocksWritten = BlkNo;
                            TotNoBlks += BlkNo;

                        }
                        else
                        {
                            if ( NbwCheck )
                            {
                                /* as we have to write only 8 blocks and already we have pad bytes so we have
                                to strat from previous last block*/
                                TotNoBlks += BlkNo - 1;
                                NdefMap->Felica.NoBlocksWritten = TotNoBlks-1;
                            }
                            else
                            {
                                /* we need to write only one block ( bytesremanind + internal buffer size = 16)*/
                                if ( !(BytesRemained - (16 -NdefMap->Felica.Wr_BytesRemained)== 0 ))
                                {
                                    TotNoBlks += BlkNo;
                                }
                                else
                                {
                                    ;/* we are not incrementing the Total no. of blocks to write*/
                                }

                                if ( NdefMap->Felica.PadByteFlag )
                                {
                                    NdefMap->Felica.NoBlocksWritten = TotNoBlks -1;

                                }
                            }
                        }
                    }
                }/*if ( BytesRemained )*/
                else
                {
                   ; /*Nothing to process here*/
                }
            }/*if ( NdefMap->ApduBufferSize >(uint32_t) (16 - NdefMap->Felica.Wr_BytesRemained))*/
            else
            {
                /* No new blks to write*/
                NdefMap->Felica.NoBlocksWritten = 0;
            }
            /* Prepare the Blk List for Write Operation*/
            /* Block List for NBw : 1st byte : two byte len list: 2nd byte is the block number*/
            for ( i=0; i< TotNoBlks; i++)
            {
                NdefMap->SendRecvBuf[BufIndex]    =   0x80;
                BufIndex++;
                /* remember the previous Blk no and continue from there*/
                if ( NdefMap->Felica.PadByteFlag == TRUE )
                {
                    NdefMap->SendRecvBuf[BufIndex]    =   NdefMap->Felica.CurBlockNo + i;
                    BufIndex++;
                }
                else
                {
                    CurBlk = NdefMap->Felica.CurBlockNo +1;
                    NdefMap->SendRecvBuf[BufIndex]    =   CurBlk + i;
                    BufIndex++;
                }
            }
            /* Copy relevant data to Transc buffer*/
            if((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= (uint32_t)(16 - NdefMap->Felica.Wr_BytesRemained))
            {

                /*Copy the Remained bytes from the internal buffer to  trxbuffer */
                (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                                NdefMap->Felica.Wr_RemainedBytesBuff,
                                NdefMap->Felica.Wr_BytesRemained);

                /*Increment the buff index*/
                BufIndex += NdefMap->Felica.Wr_BytesRemained;


                /*append copy 16-bytesToPad to trxBuffer*/
                (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                               (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                (16 - NdefMap->Felica.Wr_BytesRemained));

                /* Update Number Of Bytes Writtened*/
                NdefMap->NumOfBytesWritten = 16 - NdefMap->Felica.Wr_BytesRemained;

                /* increment the index*/
                BufIndex += 16 - NdefMap->Felica.Wr_BytesRemained;

                if ( BytesRemained )
                {
                    if (!NdefMap->Felica.EofCardReachedFlag)
                    {
                        /* check nbw limit*/
                        if ( NbwCheck != 1 )
                        {
                            /* Copy Extra Bytes other than the internal buffer bytes*/
                            (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                                        (&(NdefMap->ApduBuffer[(16 - NdefMap->Felica.Wr_BytesRemained)])),
                                        (NdefMap->ApduBufferSize - (16 - NdefMap->Felica.Wr_BytesRemained)));


                            /* Update Number Of Bytes Writtened*/
                            NdefMap->NumOfBytesWritten += (uint16_t)(NdefMap->ApduBufferSize - (16 - NdefMap->Felica.Wr_BytesRemained));

                            BufIndex += (uint8_t)(NdefMap->ApduBufferSize - (16 - NdefMap->Felica.Wr_BytesRemained));

                            if ( PadBytes )
                            {
                                for(i= 0; i< PadBytes; i++)
                                {
                                    NdefMap->SendRecvBuf[BufIndex] =0x00;
                                    BufIndex++;
                                }
                                /* no of bytes remained copy*/
                                NdefMap->Felica.Wr_BytesRemained = (uint8_t)(16 - PadBytes);

                                /*copy the data to internal buffer : Bytes remained*/
                                (void)phOsalNfc_MemCopy( NdefMap->Felica.Wr_RemainedBytesBuff,
                                              (&( NdefMap->ApduBuffer[(NdefMap->ApduBufferSize - NdefMap->Felica.Wr_BytesRemained)])),
                                              ( NdefMap->Felica.Wr_BytesRemained));
                            }
                            else
                            {
                                /* No Bytes in Internal buffer*/
                                NdefMap->Felica.Wr_BytesRemained = 0;
                            }

                        }
                        else
                        {

                            /*Copy Nbw*16 bytes of data to the trx buffer*/
                             (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                                           (&(NdefMap->ApduBuffer[(16 - NdefMap->Felica.Wr_BytesRemained)])),
                                            (NdefMap->FelicaAttrInfo.Nbw - 1) * 16);

                            /* increment the Buffindex*/
                            BufIndex += ((NdefMap->FelicaAttrInfo.Nbw - 1 )*16);

                            NdefMap->Felica.Wr_BytesRemained = 0;
                            NdefMap->NumOfBytesWritten+= ((NdefMap->FelicaAttrInfo.Nbw -1)*16);
                            NdefMap->Felica.PadByteFlag =FALSE;
                        }
                    }/*if (!NdefMap->Felica.EofCardReachedFlag)*/
                    else
                    {
                         /* check nbw limit*/
                        if ( NbwCheck != 1 )
                        {
                            if(NdefMap->ApduBufferSize < BytesRemained)
                            {
                                PH_LOG_FRI_CRIT_STR("Incorrect buffer size! "
                                                    "ApduBufferSize: %u "
                                                    "BytesRemained: %u",
                                                    NdefMap->ApduBufferSize,
                                                    BytesRemained);

                                status = NFCSTATUS_BUFFER_TOO_SMALL;
                                goto Done;
                            }

                            /* handle EOF card reached case*/
                            (void)phOsalNfc_MemCopy( (&(NdefMap->SendRecvBuf[BufIndex])),
                                       (&(NdefMap->ApduBuffer[(16 - NdefMap->Felica.Wr_BytesRemained)])),
                                       ( BytesRemained - ((16 -NdefMap->Felica.Wr_BytesRemained) )));

                            /* Update Number Of Bytes Writtened*/
                            NdefMap->NumOfBytesWritten += (uint16_t)( BytesRemained - (16 -NdefMap->Felica.Wr_BytesRemained));

                            BufIndex += (uint8_t)( BytesRemained - (16 -NdefMap->Felica.Wr_BytesRemained));

                            if ( PadBytes )
                            {
                                for(i= 0; i< PadBytes; i++)
                                {
                                    NdefMap->SendRecvBuf[BufIndex] =0x00;
                                    BufIndex++;
                                }

                                /*no of bytes remained copy*/
                                NdefMap->Felica.Wr_BytesRemained = (uint8_t)(16 - PadBytes);

                                /*copy the data to internal buffer : Bytes remained*/
                                (void)phOsalNfc_MemCopy(NdefMap->Felica.Wr_RemainedBytesBuff,
                                             (&(NdefMap->ApduBuffer[(NdefMap->ApduBufferSize - NdefMap->Felica.Wr_BytesRemained)])),
                                             (NdefMap->Felica.Wr_BytesRemained));

                            }
                            else
                            {
                                NdefMap->Felica.Wr_BytesRemained = 0;
                            }
                        }
                        else
                        {
                            if(NdefMap->ApduBufferSize < ((16u - NdefMap->Felica.Wr_BytesRemained) + (NdefMap->FelicaAttrInfo.Nbw - 1)*16u))
                            {
                                PH_LOG_FRI_CRIT_STR("Incorrect buffer size! "
                                                    "ApduBufferSize: %u",
                                                    NdefMap->ApduBufferSize);

                                status = NFCSTATUS_BUFFER_TOO_SMALL;
                                goto Done;
                            }

                            /*Copy Nbw*16 bytes of data to the trx buffer*/
                            (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                                       (&(NdefMap->ApduBuffer[(16 - NdefMap->Felica.Wr_BytesRemained)])),
                                       (NdefMap->FelicaAttrInfo.Nbw - 1) * 16);

                            /* increment the Buffindex*/
                            BufIndex += ((NdefMap->FelicaAttrInfo.Nbw - 1 )*16);

                            NdefMap->Felica.Wr_BytesRemained = 0;
                            NdefMap->NumOfBytesWritten+= ((NdefMap->FelicaAttrInfo.Nbw -1)*16);

                            NdefMap->Felica.PadByteFlag =FALSE;
                        }
                    }
                }/*if ( BytesRemained )*/
                else
                {
                    NdefMap->Felica.Wr_BytesRemained = 0;
                }
                /* Update Total No. of blocks writtened*/
                NdefMap->SendRecvBuf[NoOfBlks -1 ]=TotNoBlks;
            }/*if((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= (uint32_t)(16 - NdefMap->Felica.Wr_BytesRemained))*/
            else
            {
                /*copy the internal buffer data to   trx buffer*/
                (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                              NdefMap->Felica.Wr_RemainedBytesBuff,
                             (NdefMap->Felica.Wr_BytesRemained));

                /* increment the index*/
                BufIndex+=NdefMap->Felica.Wr_BytesRemained;

                /*append the apdusize data to the trx buffer*/

                if(NdefMap->ApduBuffIndex != 0)
                {
                    PH_LOG_FRI_CRIT_STR("Incorrect buffer size! "
                                        "ApduBuffIndex: %hu",
                                        NdefMap->ApduBuffIndex);
                    status = NFCSTATUS_BUFFER_TOO_SMALL;
                    goto Done;
                }

                (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                            (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                            NdefMap->ApduBufferSize);

                 /* Index increment*/
                BufIndex+= (uint8_t)NdefMap->ApduBufferSize;

                /* Tells how many bytes present in the internal buffer*/
                BytesRemained = NdefMap->Felica.Wr_BytesRemained + NdefMap->ApduBufferSize;

                PadBytes = (uint8_t)(16-BytesRemained);

                /* Pad empty bytes with Zeroes to complete 16 bytes*/
                for(i= 0; i< PadBytes; i++)
                {
                    NdefMap->SendRecvBuf[BufIndex] =0x00;
                    BufIndex++;
                }

                /* Update Number Of Bytes Writtened*/
                NdefMap->NumOfBytesWritten = (uint16_t)NdefMap->ApduBufferSize;

                /* Flag set to understand that , we have received less no. of bytes than
                present in the internal buffer*/
                NdefMap->Felica.IntermediateWrFlag = TRUE;

                if ( NdefMap->Felica.PadByteFlag )
                {
                    NdefMap->Felica.NoBlocksWritten = 0;
                }
            }

            NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;
            NdefMap->SendLength = BufIndex;
            /* Update Total No. of blocks writtened*/
            NdefMap->SendRecvBuf[NoOfBlks -1 ]=TotNoBlks;
        }
        else
        {
            /*Fresh write, starting from a new block*/
            if ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= (uint32_t)(16* NdefMap->FelicaAttrInfo.Nbw ))
            {
                /* check for the card size and write Nbw Blks*/
                if ( NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo >= NdefMap->FelicaAttrInfo.Nbw)
                {
                    /* update the number of blocks to write with the block list elements*/
                    /* Total Number of blocks to write*/
                    NdefMap->SendRecvBuf[BufIndex]    =   NdefMap->FelicaAttrInfo.Nbw;
                    BufIndex++;

                    /* Block List for NBw : 1st byte : two byte len list: 2nd byte is the block number*/
                    for ( i=1; i<= NdefMap->FelicaAttrInfo.Nbw; i++)
                    {
                        NdefMap->SendRecvBuf[BufIndex]    =   0x80;
                        BufIndex++;

                        NdefMap->SendRecvBuf[BufIndex]    =   NdefMap->Felica.CurBlockNo + i;
                        BufIndex++;
                    }
                    /*Copy Nbw*16 bytes of data to the trx buffer*/

                    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                                 (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                 NdefMap->FelicaAttrInfo.Nbw * 16);

                    /* increment the Buffindex*/
                    BufIndex += (NdefMap->FelicaAttrInfo.Nbw*16);

                    /* update the length*/
                    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;

                    NdefMap->Felica.Wr_BytesRemained = 0;
                    NdefMap->NumOfBytesWritten = (NdefMap->FelicaAttrInfo.Nbw*16);
                    NdefMap->Felica.NoBlocksWritten = NdefMap->FelicaAttrInfo.Nbw;

                    /* update the Send length*/
                    NdefMap->SendLength = BufIndex;

                    NdefMap->Felica.PadByteFlag = FALSE;
                }/*if ( NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo >= NdefMap->FelicaAttrInfo.Nbw)*/
                else
                {
                    /* we need to write less than nbw blks*/
                    /* update the number of blocks to write with the block list elements*/
                    /* Total Number of blocks to write*/
                    NdefMap->SendRecvBuf[BufIndex] = (uint8_t)( NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo);
                    BufIndex++;

                    /* Block List for NBw : 1st byte : two byte len list: 2nd byte is the block number*/
                    for ( i=1; i<= (NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo); i++)
                    {
                        NdefMap->SendRecvBuf[BufIndex]    =   0x80;
                        BufIndex++;
                        NdefMap->SendRecvBuf[BufIndex]    =   NdefMap->Felica.CurBlockNo + i;
                        BufIndex++;
                    }

                    /*Copy Nbw*16 bytes of data to the trx buffer*/
                    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                                 (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                 (NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo)*16);

                    /* increment the Buffindex*/
                    BufIndex += (uint8_t)((NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo )*16);

                    /* update the length*/
                    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;

                    NdefMap->NumOfBytesWritten = ((NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo)*16);
                    NdefMap->Felica.NoBlocksWritten = (uint8_t)(NdefMap->FelicaAttrInfo.Nmaxb - NdefMap->Felica.CurBlockNo);

                    /* update the Send length*/
                    NdefMap->SendLength = BufIndex;

                    NdefMap->Felica.PadByteFlag =FALSE;
                    NdefMap->Felica.Wr_BytesRemained = 0;
                }
            }
            else
            {
                /*chk eof reached*/
                if ( NdefMap->Felica.EofCardReachedFlag)
                {
                    BlkNo =((uint8_t)(BytesRemainedInCard )/16);
                    if(((uint8_t)( BytesRemainedInCard )%16) != 0)
                    {
                        BlkNo++;
                        PadBytes = ((BlkNo * 16) - (uint8_t)(BytesRemainedInCard ));
                        NdefMap->Felica.PadByteFlag = TRUE;
                    }
                }
                else
                {

                    BlkNo =((uint8_t)( NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)/16);
                    if(((uint8_t)( NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex)%16) != 0)
                    {
                        BlkNo++;
                        PadBytes = (BlkNo * 16) - (uint8_t)( NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                        NdefMap->Felica.PadByteFlag = TRUE;

                    }


                }

                /* update the number of blocks to write with the block list elements*/
                /* Total Number of blocks to write*/
                NdefMap->SendRecvBuf[BufIndex]    =  BlkNo;
                BufIndex++;

                NdefMap->Felica.NoBlocksWritten = BlkNo;

                /* Block List for NBw : 1st byte : two byte len list: 2nd byte is the block number*/
                for ( i=0; i< BlkNo; i++)
                {
                    NdefMap->SendRecvBuf[BufIndex]    =   0x80;
                    BufIndex++;
                    {
                        CurBlk = NdefMap->Felica.CurBlockNo +1;
                        NdefMap->SendRecvBuf[BufIndex]    =   CurBlk + i;
                        BufIndex++;
                    }
                }
                if ( NdefMap->Felica.EofCardReachedFlag )
                {
                    /*Copy last data to the trx buffer*/
                    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                                 (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                 BytesRemainedInCard );

                    /* increment the bufindex and bytes written*/
                    BufIndex += (uint8_t )BytesRemainedInCard ;
                    NdefMap->NumOfBytesWritten = (uint16_t)BytesRemainedInCard ;
                }
                else
                {
                    /*Copy data to the trx buffer*/
                    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                                 (&(NdefMap->ApduBuffer[NdefMap->ApduBuffIndex])),
                                 (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex));

                    /* increment the bufindex and bytes written*/
                    BufIndex += (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                    NdefMap->NumOfBytesWritten = (uint8_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
                }
                if ( PadBytes )
                {
                    for(i= 0; i< PadBytes; i++)
                    {
                        NdefMap->SendRecvBuf[BufIndex] =0x00;
                        BufIndex++;
                    }
                    /*no of bytes remained copy*/
                    NdefMap->Felica.Wr_BytesRemained = (uint8_t)(16 - PadBytes);

                    if ( NdefMap->Felica.EofCardReachedFlag )
                    {
                        /*copy the data to internal buffer : Bytes remained*/
                        (void)phOsalNfc_MemCopy(NdefMap->Felica.Wr_RemainedBytesBuff,
                                    (&(NdefMap->ApduBuffer[((BytesRemainedInCard - (BytesRemainedInCard % 16)))])),
                                    ( NdefMap->Felica.Wr_BytesRemained));

                    }
                    else
                    {
                        /*copy the data to internal buffer : Bytes remained*/
                        (void)phOsalNfc_MemCopy( NdefMap->Felica.Wr_RemainedBytesBuff,
                                       (&(NdefMap->ApduBuffer[((NdefMap->ApduBufferSize - NdefMap->Felica.Wr_BytesRemained))])),
                                        ( NdefMap->Felica.Wr_BytesRemained));

                    }
                }/*if ( PadBytes )*/
                else
                {
                    NdefMap->Felica.Wr_BytesRemained = 0;
                    NdefMap->Felica.PadByteFlag = FALSE;
                }
                /* update the pkt len*/
                NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX] = BufIndex;
                NdefMap->SendLength = BufIndex;
            }
        }/* else of if ( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= (uint32_t)(16* NdefMap->FelicaAttrInfo.Nbw )) */
        status = phFriNfc_Felica_HWriteDataBlk(NdefMap);
    }
    else
    {
        /*0 represents the write operation ended*/
        status = phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_ENDED);
    }

Done:

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HWriteDataBlk(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    PH_LOG_NDEF_FUNC_ENTRY();

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /*Set the ISO14434 command*/
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /* set the state*/
    NdefMap->State = PH_NFCFRI_NDEFMAP_FELI_STATE_WR_BLOCK;

    /* set send receive length*/
    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

     /*Call the Overlapped HAL Transceive function */
    status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                         &NdefMap->MapCompletionInfo,
                                         NdefMap->psRemoteDevInfo,
                                         NdefMap->Cmd,
                                         &NdefMap->psDepAdditionalInfo,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendLength,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

NFCSTATUS
phFriNfc_Felica_ChkNdef(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint8_t sysCode[2];

    PH_LOG_NDEF_FUNC_ENTRY();

    /* set the system code for selecting the wild card*/
    sysCode[0] = 0x12;
    sysCode[1] = 0xFC;

    status = phFriNfc_Felica_HPollCard( NdefMap,sysCode,PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_NDEF_APP);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);

}

static
NFCSTATUS
phFriNfc_Felica_HPollCard(
    _Inout_             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(2) const uint8_t       sysCode[],
    _In_    uint8_t     state
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    PH_LOG_NDEF_FUNC_ENTRY();

    /*Format the Poll Packet for selecting the system code passed as parameter */
    NdefMap->SendRecvBuf[0] = 0x06;
    NdefMap->SendRecvBuf[1] = 0x00;
    NdefMap->SendRecvBuf[2] = sysCode[0];
    NdefMap->SendRecvBuf[3] = sysCode[1];

    if(NdefMap->bDtaFlag)
    {
        NdefMap->SendRecvBuf[4] = 0x00;
        NdefMap->SendRecvBuf[5] = 0x00;
    }
    else
    {
        NdefMap->SendRecvBuf[4] = 0x01;
        NdefMap->SendRecvBuf[5] = 0x03;
    }

    NdefMap->SendLength = 6;

     /*set the completion routines for the felica card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*Set Ndef State*/
    NdefMap->State = state;

    /* set the felica cmd */
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    status = phFriNfc_OvrHal_Transceive(NdefMap->LowerDevice,
                                        &NdefMap->MapCompletionInfo,
                                        NdefMap->psRemoteDevInfo,
                                        NdefMap->Cmd,
                                        &NdefMap->psDepAdditionalInfo,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendLength,
                                        NdefMap->SendRecvBuf,
                                        NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HUpdateManufIdDetails(
    _In_ const phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Get the details from Poll Response packet */
    if ( ((NdefMap->bDtaFlag) && (*NdefMap->SendRecvLength >= 18U) ) ||
        (*NdefMap->SendRecvLength >= 20U) )
    {
        (void)phOsalNfc_MemCopy(  (uint8_t *)NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,
                       (uint8_t *)&NdefMap->SendRecvBuf[2], 8);
        (void)phOsalNfc_MemCopy(  (uint8_t *)NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm,
                       (uint8_t *)&NdefMap->SendRecvBuf[10], 8);

        if (NdefMap->bDtaFlag )
        {
            NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[1] = 0x12;
            NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[0] = 0xFC;
        }
        else
        {
            NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[1] = NdefMap->SendRecvBuf[18];
            NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[0] = NdefMap->SendRecvBuf[19];
        }
        NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDmLength = 8;

        /* copy the IDm and PMm in Manufacture Details Structure*/
        (void)phOsalNfc_MemCopy( (uint8_t *)(NdefMap->FelicaManufDetails.ManufID),
                      (uint8_t *)NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,
                      8);
        (void)phOsalNfc_MemCopy( (uint8_t *)(NdefMap->FelicaManufDetails.ManufParameter),
                      (uint8_t *)NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm,
                      8);
        if((NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[1] == 0x12)
            && (NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode[0] == 0xFC))
        {
            status = PHNFCSTVAL(CID_NFC_NONE, NFCSTATUS_SUCCESS);
        }
        else
        {
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                NFCSTATUS_NO_NDEF_SUPPORT);
        }
    }
    else
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_NO_NDEF_SUPPORT);
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

void
phFriNfc_Felica_Process(
    _At_((phFriNfc_NdefMap_t*)Context, _Inout_) void        *Context,
    _In_                                        NFCSTATUS   Status
    )
{
    uint8_t  CRFlag = FALSE;
    uint16_t RecvTxLen = 0,
             BytesToRecv = 0,
             Nbc = 0;
    uint32_t TotNoWrittenBytes = 0,
             NDEFLen=0;

    /*Set the context to Map Module*/
    phFriNfc_NdefMap_t      *NdefMap = (phFriNfc_NdefMap_t *)Context;

    PH_LOG_NDEF_FUNC_ENTRY();

    if ( Status == NFCSTATUS_SUCCESS )
    {
        switch (NdefMap->State)
        {
            case PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_NDEF_APP:

                    /* check the ndef compliency with the system code reecived in the RemoteDevInfo*/
                    Status = phFriNfc_Felica_HUpdateManufIdDetails(NdefMap);

                    if (Status == NFCSTATUS_SUCCESS)
                    {
                        /* Mantis ID : 645*/
                        /* set the operation type to Check ndef type*/
                        NdefMap->Felica.OpFlag = PH_FRINFC_NDEFMAP_FELI_CHK_NDEF_OP;
                        Status = phFriNfc_Felica_HRdAttrInfo(NdefMap);
                        /* handle the error in Transc function*/
                        if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                        {
                            CRFlag = TRUE;
                        }
                    }
                    else
                    {
                        CRFlag = TRUE;
                    }
                    if ( CRFlag == TRUE )
                    {
                        /* call respective CR */
                        phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_CHK_NDEF,Status);

                    }
                break;

            case PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR:
             /* check for the status flag1 and status flag2for the successful read operation*/
                if ( NdefMap->SendRecvBuf[10] == 0x00)
                {
                    /* check the Manuf Id in the receive buffer*/
                    Status = phFriNfc_Felica_HCheckManufId(NdefMap);
                    if ( Status == NFCSTATUS_SUCCESS)
                    {
                        /* Update the Attribute Information in to the context structure*/
                        Status = phFriNfc_Felica_HUpdateAttrInfo(NdefMap);
                        if ( Status == NFCSTATUS_SUCCESS )
                        {
                            PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                NDEFLen);

                            if ( NdefMap->Felica.OpFlag == PH_FRINFC_NDEFMAP_FELI_WR_ATTR_RD_OP )
                            {
                                /* Proceed With Write Functinality*/
                                Status = phFriNfc_Felica_HChkAttrBlkForWrOp(NdefMap);
                                /* handle the error in Transc function*/
                                if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                {
                                    /* call respective CR */
                                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                                }
                            }
                            else if( NdefMap->Felica.OpFlag == PH_FRINFC_NDEFMAP_FELI_RD_ATTR_RD_OP )
                            {
                                /* Proceed With Read Functinality*/
                                Status = phFriNfc_Felica_HChkAttrBlkForRdOp(NdefMap,NDEFLen);
                                if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                {
                                    /* call respective CR */
                                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_RD_NDEF,Status);
                                }
                            }
                            else if( NdefMap->Felica.OpFlag == PH_FRINFC_NDEFMAP_FELI_CHK_NDEF_OP )
                            {

                                Status = phFriNfc_MapTool_SetCardState( NdefMap,
                                                                        NDEFLen);
                                /* check status value*/
                                NdefMap->CardType = PH_FRINFC_NDEFMAP_FELICA_SMART_CARD;
                                /*reset the buffer index*/
                                NdefMap->ApduBuffIndex = 0;
                                /* set the Next operation Flag to indicate need of reading attribute information*/
                                NdefMap->Felica.OpFlag = PH_FRINFC_NDEFMAP_FELI_OP_NONE;
                                /* call respective CR */
                                phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_CHK_NDEF,Status);
                            }
                            else if ( NdefMap->Felica.OpFlag == PH_FRINFC_NDEFMAP_FELI_WR_EMPTY_MSG_OP )
                            {
                                /* Proceed With Write Functinality*/
                                Status = phFriNfc_Felica_HWrEmptyMsg(NdefMap);
                                /* handle the error in Transc function*/
                                if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                {
                                    /* call respective CR */
                                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_ERASE_NDEF,Status);
                                }
                            }
                            else
                            {

                                /* invalid operation occured*/
                                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                        NFCSTATUS_INVALID_DEVICE_REQUEST);
                                CRFlag =TRUE ;
                            }
                        }
                        else
                        {
                            CRFlag =TRUE ;
                        }
                    }
                    else
                    {
                        CRFlag =TRUE ;
                    }
                }
                else
                {
                    CRFlag =TRUE;
                    /*handle the  Error case*/
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                        NFCSTATUS_READ_FAILED);
                }
                if ( CRFlag == TRUE )
                {
                    /* call respective CR */
                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_RD_NDEF,Status);
                }
                break;

            case PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN:
                    /* chk the status flags 1 and 2*/
                    if ( NdefMap->SendRecvBuf[10] == 0x00 )
                    {
                        /* Update Data Call*/
                        Status =phFriNfc_Felica_HUpdateData(NdefMap);
                        if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                        {
                            /* call respective CR */
                            phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                        }
                    }
                    else
                    {
                        /*handle the  Error case*/
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                            NFCSTATUS_WRITE_FAILED);
                        phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);

                    }
                break;
            case PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_END:

                    /* chk the status flags 1 and 2*/
                    if ( NdefMap->SendRecvBuf[10] == 0x00)
                    {
                        /* Entire Write Operation is complete*/
                        Status = PHNFCSTVAL(CID_NFC_NONE,\
                                            NFCSTATUS_SUCCESS);
                    }
                    else
                    {
                         /*handle the  Error case*/
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                            NFCSTATUS_WRITE_FAILED);
                    }
                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                break;

            case PH_NFCFRI_NDEFMAP_FELI_STATE_WR_EMPTY_MSG :

                    /* chk the status flags 1 and 2*/
                    if ( NdefMap->SendRecvBuf[10] == 0x00)
                    {
                        /* Entire Write Operation is complete*/
                        Status = PHNFCSTVAL(CID_NFC_NONE,\
                                            NFCSTATUS_SUCCESS);
                    }
                    else
                    {
                         /*handle the  Error case*/
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                            NFCSTATUS_WRITE_FAILED);
                    }
                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                break;

            case PH_NFCFRI_NDEFMAP_FELI_STATE_WR_BLOCK :
                if(NdefMap->SendRecvBuf[1] == PH_NFCFRI_NDEFMAP_FELI_WR_RESP_BYTE )
                {
                    /* chk the status flags 1 and 2*/
                    if ( NdefMap->SendRecvBuf[10] == 0x00 )
                    {
                        /* This is used when we have bytes less than 16 bytes*/
                        if ( NdefMap->Felica.IntermediateWrFlag == TRUE )
                        {
                            /* after Successful write copy the last writtened bytes back to the
                            internal buffer*/
                            (void)phOsalNfc_MemCopy( (&(NdefMap->Felica.Wr_RemainedBytesBuff[NdefMap->Felica.Wr_BytesRemained])),
                                            NdefMap->ApduBuffer,
                                            NdefMap->NumOfBytesWritten);

                            NdefMap->Felica.Wr_BytesRemained +=
                           (uint8_t)( NdefMap->NumOfBytesWritten);

                            /* Increment the Send Buffer index */
                            NdefMap->ApduBuffIndex +=
                                NdefMap->NumOfBytesWritten;

                            *NdefMap->WrNdefPacketLength = NdefMap->ApduBuffIndex;
                            NdefMap->Felica.IntermediateWrFlag = FALSE;
                            /* Call Update Data()*/
                            Status = phFriNfc_Felica_HUpdateData(NdefMap);
                        }
                        else
                        {
                            /* update the index and bytes writtened*/
                            NdefMap->ApduBuffIndex += NdefMap->NumOfBytesWritten;
                            *NdefMap->WrNdefPacketLength = NdefMap->ApduBuffIndex;
                            if ( NdefMap->Felica.EofCardReachedFlag )
                            {
                                if ( NdefMap->Felica.CurBlockNo < NdefMap->FelicaAttrInfo.Nmaxb)
                                {
                                    NdefMap->Felica.CurBlockNo += NdefMap->Felica.NoBlocksWritten;
                                }
                                if (( NdefMap->Felica.CurBlockNo == NdefMap->FelicaAttrInfo.Nmaxb) &&
                                 ( NdefMap->ApduBuffIndex == (NdefMap->FelicaAttrInfo.Nmaxb*16)))
                                {
                                    NdefMap->Felica.EofCardReachedFlag = FELICA_RD_WR_EOF_CARD_REACHED ;
                                    /*0 represents the write ended*/
                                    Status = phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_ENDED);
                                    if( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                    {
                                        /* call respective CR */
                                        phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                                    }
                                }
                                else
                                {
                                    PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES(NdefMap->FelicaAttrInfo.LenBytes[0],
                                                                NdefMap->FelicaAttrInfo.LenBytes[1],
                                                                NdefMap->FelicaAttrInfo.LenBytes[2],
                                                                TotNoWrittenBytes);
                                    if ( ( NdefMap->Felica.CurBlockNo == NdefMap->FelicaAttrInfo.Nmaxb) &&
                                        ((TotNoWrittenBytes + NdefMap->ApduBuffIndex) == (uint32_t)(NdefMap->FelicaAttrInfo.Nmaxb*16)))
                                    {
                                        NdefMap->Felica.EofCardReachedFlag =FELICA_RD_WR_EOF_CARD_REACHED;
                                        /*0 represents the write ended*/
                                        Status = phFriNfc_Felica_HUpdateAttrBlkForWrOp(NdefMap,FELICA_WRITE_ENDED);
                                        if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                        {
                                            /* call respective CR */
                                            phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                                        }
                                    }
                                    else
                                    {
                                        /* Call Update Data()*/
                                        Status = phFriNfc_Felica_HUpdateData(NdefMap);
                                        if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                        {
                                            /* call respective CR */
                                            phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                NdefMap->Felica.CurBlockNo += NdefMap->Felica.NoBlocksWritten;
                                /* Call Update Data()*/
                                Status = phFriNfc_Felica_HUpdateData(NdefMap);
                                if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                                {
                                    /* call respective CR */
                                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                                }
                            }
                        }
                    }
                    else
                    {
                        /*handle the  Error case*/
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                            NFCSTATUS_WRITE_FAILED);
                        CRFlag = TRUE;

                    }
                }
                else
                {
                   /*return Error "Invalid Write Response Code"*/
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                       NFCSTATUS_WRITE_FAILED);
                    CRFlag = TRUE;

                }
                if ( CRFlag == TRUE )
                {
                    /* Reset following parameters*/
                    NdefMap->ApduBuffIndex=0;
                    NdefMap->Felica.Wr_BytesRemained = 0;
                    NdefMap->ApduBufferSize = 0;
                    phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_WR_NDEF,Status);
                }

                break;

            case    PH_NFCFRI_NDEFMAP_FELI_STATE_RD_BLOCK :

            /* check the Manuf Id in the receive buffer*/
            Status = phFriNfc_Felica_HCheckManufId(NdefMap);
            if ( Status == NFCSTATUS_SUCCESS )
            {
                if(NdefMap->SendRecvBuf[1] == PH_NFCFRI_NDEFMAP_FELI_RD_RESP_BYTE )
                {
                    /* calculate the Nmaxb*/
                    Nbc = phFriNfc_Felica_HGetMaximumBlksToRead(NdefMap,PH_NFCFRI_NDEFMAP_FELI_NBC);
                    /*get Receive length from the card for corss verifications*/
                    RecvTxLen= phFriNfc_Felica_HSetTrxLen(NdefMap,Nbc);
                    BytesToRecv = NdefMap->SendRecvBuf[12]*16;

                    /* chk the status flags 1 */
                    if ( NdefMap->SendRecvBuf[10] == 0x00)
                    {
                        if ( RecvTxLen == BytesToRecv)
                        {
                            NdefMap->Felica.CurBlockNo += (uint8_t)(RecvTxLen/16);
                            phFriNfc_Felica_HAfterRead_CopyDataToBuff(NdefMap);
                            Status = phFriNfc_Felica_HReadData(NdefMap,PH_FRINFC_NDEFMAP_SEEK_CUR);
                            /* handle the error in Transc function*/
                            if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                            {
                                CRFlag =TRUE;
                            }
                        }
                        else
                        {
                            CRFlag =TRUE;
                            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                                NFCSTATUS_INVALID_RECEIVE_LENGTH);
                            /*set the buffer index back to zero*/
                            NdefMap->ApduBuffIndex = 0;
                            NdefMap->Felica.Rd_NoBytesToCopy = 0;
                        }
                    }
                    else
                    {
                        NdefMap->ApduBuffIndex=0;
                        /*handle the  Error case*/
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                                            NFCSTATUS_READ_FAILED);
                        CRFlag =TRUE;
                    }
                }
                else
                {
                    CRFlag =TRUE;
                    NdefMap->ApduBuffIndex=0;
                    /*return Error "Invalid Read Response Code"*/
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                NFCSTATUS_READ_FAILED);
                }
            }
            else
            {
                CRFlag =TRUE;
            }
            if ( CRFlag ==TRUE )
            {
                /* call respective CR */
                phFriNfc_Felica_HCrHandler(NdefMap,PH_FRINFC_NDEFMAP_CR_RD_NDEF,Status);
            }
            break;

        default:
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                               NFCSTATUS_INVALID_DEVICE_REQUEST);
            phFriNfc_Felica_HCrHandler(NdefMap, PH_FRINFC_NDEFMAP_CR_INVALID_OPE, Status);
            break;


        }
    }
    else
    {
        /* Call CR for unknown Error's*/
        switch ( NdefMap->State)
        {
            case PH_FRINFC_NDEFMAP_FELI_STATE_CHK_NDEF :
            case PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_WILD_CARD :
            case PH_NFCFRI_NDEFMAP_FELI_STATE_SELECT_NDEF_APP :
            case PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR :
                phFriNfc_Felica_HCrHandler(NdefMap, PH_FRINFC_NDEFMAP_CR_CHK_NDEF,
                                            Status);
                break;
            case PH_NFCFRI_NDEFMAP_FELI_STATE_WR_BLOCK :
            case PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN :
            case PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_END :
                phFriNfc_Felica_HCrHandler(NdefMap, PH_FRINFC_NDEFMAP_CR_WR_NDEF,
                                            Status);
                break;
            case PH_NFCFRI_NDEFMAP_FELI_STATE_RD_BLOCK :
                phFriNfc_Felica_HCrHandler(NdefMap, PH_FRINFC_NDEFMAP_CR_RD_NDEF,
                                            Status);
                break;
            default :
                /*set the invalid state*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_DEVICE_REQUEST);
                phFriNfc_Felica_HCrHandler(NdefMap, PH_FRINFC_NDEFMAP_CR_INVALID_OPE, Status);
                break;
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
}

/*!
 * \brief Prepares Cmd Pkt for reading attribute Blk information.
 */
static
NFCSTATUS
phFriNfc_Felica_HRdAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint8_t BufIndex = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Set the Felica Cmd*/
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /* 1st byte represents the length of the cmd packet*/
    NdefMap->SendRecvBuf[BufIndex] = 0x00;
    BufIndex++;

    /* Read/check command code*/
    NdefMap->SendRecvBuf[BufIndex] = 0x06;
    BufIndex++;

    /* IDm - Manufacturer Id : 8bytes*/
    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                (void * )&NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,8);

    BufIndex+=8;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Services (n=1 ==> 0x80)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x0B;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Blocks to read)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x80;  /*  1st Block Element : byte 1*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  1st Block Element : byte 2, block 1*/
    BufIndex++;

    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX]             =   BufIndex;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Update the Send Len*/
    NdefMap->SendLength = BufIndex;

    /* Change the state to  PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR */
    NdefMap->State =  PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR;

    /*set the completion routines for the desfire card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;//phFriNfc_NdefMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*Call the Overlapped HAL Transceive function */
    status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                         &NdefMap->MapCompletionInfo,
                                         NdefMap->psRemoteDevInfo,
                                         NdefMap->Cmd,
                                         &NdefMap->psDepAdditionalInfo,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendLength,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);

}

/*!
 * \brief Prepares Cmd Pkt for reading attribute Blk information.
 */
static
NFCSTATUS
phFriNfc_Felica_HNdefRdAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint8_t BufIndex = 0;
    uint8_t bIndex = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Set the Felica Cmd*/
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /* 1st byte represents the length of the cmd packet*/
    NdefMap->SendRecvBuf[BufIndex] = 0x00;
    BufIndex++;

    /* Read/check command code*/
    NdefMap->SendRecvBuf[BufIndex] = 0x06;
    BufIndex++;

    /* IDm - Manufacturer Id : 8bytes*/
    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                (void * )&NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,8);

    BufIndex+=8;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Services (n=1 ==> 0x80)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x09;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x0F;  /*  Number of Blocks to read*/
    BufIndex++;

    for(bIndex = 1; bIndex <= 0xF;bIndex++)
    {
        NdefMap->SendRecvBuf[BufIndex]    =   0x80;  /*  1st Block Element : byte 1*/
        BufIndex++;

        NdefMap->SendRecvBuf[BufIndex]    =   bIndex;  /*  1st Block Element : byte 2, block 1*/
        BufIndex++;
    }

    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX]             =   BufIndex;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Update the Send Len*/
    NdefMap->SendLength = BufIndex;

    /* Change the state to  PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR */
    NdefMap->State =  PH_NFCFRI_NDEFMAP_FELI_STATE_RD_ATTR;

    /*set the completion routines for the desfire card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;//phFriNfc_NdefMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*Call the Overlapped HAL Transceive function */
    status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                         &NdefMap->MapCompletionInfo,
                                         NdefMap->psRemoteDevInfo,
                                         NdefMap->Cmd,
                                         &NdefMap->psDepAdditionalInfo,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendLength,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);

}

/*!
 * \brief Prepares Cmd Pkt for writing attribute Blk information.
 */
static
NFCSTATUS
phFriNfc_Felica_HNdefWrAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint8_t BufIndex = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* Set the Felica Cmd*/
    NdefMap->Cmd.FelCmd = phNfc_eFelica_Raw;

    /*set the additional informations for the data exchange*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /* 1st byte represents the length of the cmd packet*/
    NdefMap->SendRecvBuf[BufIndex] = 0x00;
    BufIndex++;

    /* Read/check command code*/
    NdefMap->SendRecvBuf[BufIndex] = 0x08;
    BufIndex++;

    /* IDm - Manufacturer Id : 8bytes*/
    (void)phOsalNfc_MemCopy((&(NdefMap->SendRecvBuf[BufIndex])),
                (void * )&NdefMap->psRemoteDevInfo->RemoteDevInfo.Felica_Info.IDm,8);

    BufIndex+=8;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Services (n=1 ==> 0x80)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x09;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  Service Code List*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  Number of Blocks to read)*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x80;  /*  1st Block Element : byte 1*/
    BufIndex++;

    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  1st Block Element : byte 2, block 1*/
    BufIndex++;

    /* Write 16 Bytes of Attribute info */
    NdefMap->SendRecvBuf[BufIndex]    =   0x10;  /*  1st Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x0F;  /*  2nd Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x0C;  /*  3rd Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  4th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x93;  /*  5th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  6th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  7th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  8th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  9th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x0F;  /*  10th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  11th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  12th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x00;  /*  13th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0xF0;  /*  14th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0x01;  /*  15th Byte */
    BufIndex++;
    NdefMap->SendRecvBuf[BufIndex]    =   0xBD;  /*  16th Byte */
    BufIndex++;

    NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_PKT_LEN_INDEX]             =   BufIndex;

    *NdefMap->SendRecvLength = NdefMap->TempReceiveLength;

    /* Update the Send Len*/
    NdefMap->SendLength = BufIndex;

    /* Change the state to  PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN */
    NdefMap->State =  PH_NFCFRI_NDEFMAP_FELI_STATE_ATTR_BLK_WR_BEGIN;

    /*set the completion routines for the desfire card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Felica_Process;//phFriNfc_NdefMap_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /*Call the Overlapped HAL Transceive function */
    status = phFriNfc_OvrHal_Transceive( NdefMap->LowerDevice,
                                         &NdefMap->MapCompletionInfo,
                                         NdefMap->psRemoteDevInfo,
                                         NdefMap->Cmd,
                                         &NdefMap->psDepAdditionalInfo,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendLength,
                                         NdefMap->SendRecvBuf,
                                         NdefMap->SendRecvLength);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

/*!
 * \brief Validated manufacturer Details, during the read/write operations.
 */

static
NFCSTATUS
phFriNfc_Felica_HCheckManufId(
    _In_ const phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;

    uint8_t result = 0;

    PH_LOG_NDEF_FUNC_ENTRY();

    /* check the stored manufacture id with the received manufacture id*/
    result = (uint8_t)(phFriNfc_Felica_MemCompare( (void *)(&(NdefMap->SendRecvBuf[2])),
                               (void *)NdefMap->FelicaManufDetails.ManufID,
                               8));

    if ( result != 0)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_REMOTE_DEVICE);

    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);

    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Felica_HCalCheckSum(
    _In_reads_bytes_(EndIndex + 1)  const uint8_t   *TempBuffer,
    _In_range_(<=, EndIndex)        uint8_t         StartIndex,
    _In_range_(>=, StartIndex)      uint8_t         EndIndex,
    _In_                            uint16_t        RecvChkSum
    )
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    uint16_t CheckSum=0,
             BufIndex=0;

    PH_LOG_NDEF_FUNC_ENTRY();

    for(BufIndex = StartIndex;BufIndex <=EndIndex;BufIndex++)
    {
        CheckSum += TempBuffer[BufIndex];
    }
    if( RecvChkSum != CheckSum )
    {
           Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_INVALID_FORMAT);
    }

    PH_LOG_NDEF_FUNC_EXIT();
    return (Result);
}

/*!
 * \brief On successful read attribute blk information, this function validates and stores the
 * Attribute informations in to the context.
 */
static
NFCSTATUS
phFriNfc_Felica_HUpdateAttrInfo(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS status = NFCSTATUS_PENDING;
    uint8_t   CRFlag = FALSE,
              Nmaxb1, Nmaxb2 = 0,
              ChkSum1 = 0, ChkSum2=0;

    uint16_t  Nmaxblk = 0,
              RecvChkSum=0,
              NdefBlk = 0;
    uint32_t  DataLen =0;


    /* Validate T3VNo and NFCDevVNo */
    status = phFriNfc_MapTool_ChkSpcVer(NdefMap,
                                        PH_NFCFRI_NDEFMAP_FELI_VERSION_INDEX);
    if ( status != NFCSTATUS_SUCCESS )
    {
        CRFlag = TRUE;
    }
    else
    {
        /* get the Nmaxb from the receive buffer*/
        Nmaxb1 = NdefMap->SendRecvBuf[16];
        Nmaxb2 = NdefMap->SendRecvBuf[17];

        Nmaxblk = (((uint16_t)Nmaxb1 << 8) | (Nmaxb2 & 0x00ff));

        if ( Nmaxblk != 0 )
        {
            /* check the Nbr against the Nmaxb*/
            if ( NdefMap->SendRecvBuf[14] > Nmaxblk )
            {
                CRFlag = TRUE;
            }
            else
            {
                /*check Nbw > Nmaxb*/
                /*check the write flag validity*/
                /*check for the RFU bytes validity*/
                 if ( (NdefMap->SendRecvBuf[15] > Nmaxblk) ||
                     ((NdefMap->SendRecvBuf[22] != 0x00) && (NdefMap->SendRecvBuf[22] !=0x0f ))||
                     ( (NdefMap->SendRecvBuf[23] != 0x00) && (NdefMap->SendRecvBuf[23] !=0x01 ))||
                     ( NdefMap->SendRecvBuf[18] != 0x00) ||
                     ( NdefMap->SendRecvBuf[19] != 0x00) ||
                     ( NdefMap->SendRecvBuf[20] != 0x00) ||
                     ( NdefMap->SendRecvBuf[21] != 0x00))

                {
                    CRFlag = TRUE;
                }
                else
                {
                    /* check the validity of the actual ndef data len*/
                    PH_NFCFRI_NDEFMAP_FELI_CAL_LEN_BYTES( NdefMap->SendRecvBuf[24],
                                                          NdefMap->SendRecvBuf[25],
                                                          NdefMap->SendRecvBuf[26],
                                                          DataLen);


                    /* Calculate Nbc*/
                    NdefBlk = (uint16_t )((( DataLen % 16) == 0 ) ? (DataLen >> 4) : ((DataLen >> 4) +1));

                    /* check Nbc against Nmaxb*/
                    if ((NdefBlk > Nmaxblk))
                    {
                        CRFlag = TRUE;
                    }
                    else
                    {
                        /*Store the attribute information in phFriNfc_Felica_AttrInfo*/
                        NdefMap->FelicaAttrInfo.Version = NdefMap->SendRecvBuf[PH_NFCFRI_NDEFMAP_FELI_VERSION_INDEX];
                        NdefMap->FelicaAttrInfo.Nbr = NdefMap->SendRecvBuf[14];
                        NdefMap->FelicaAttrInfo.Nbw = NdefMap->SendRecvBuf[15];

                        NdefMap->FelicaAttrInfo.Nmaxb = Nmaxblk;

                        NdefMap->FelicaAttrInfo.WriteFlag = NdefMap->SendRecvBuf[22];
                        NdefMap->FelicaAttrInfo.RdWrFlag = NdefMap->SendRecvBuf[23];

                        /* Get CheckSum*/
                        ChkSum1 = NdefMap->SendRecvBuf[27];
                        ChkSum2 = NdefMap->SendRecvBuf[28];

                        RecvChkSum = (((uint16_t)ChkSum1 << 8) | (ChkSum2 & 0x00ff));

                        /* Check the check sum validity?*/
                        status = phFriNfc_Felica_HCalCheckSum(NdefMap->SendRecvBuf,
                                                              PH_NFCFRI_NDEFMAP_FELI_VERSION_INDEX,
                                                              26,
                                                              RecvChkSum);
                        if ( status != NFCSTATUS_SUCCESS )
                        {
                            CRFlag = TRUE;
                        }
                        else
                        {
                            /*check RW Flag Access Rights*/
                            /* set to read only cannot write*/
                            if ( NdefMap->FelicaAttrInfo.RdWrFlag == 0x00 )
                            {
                                NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_ONLY;
                            }
                            else if ( NdefMap->FelicaAttrInfo.RdWrFlag == 0x01 ) // additional check for R/W access
                            {
                                NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_WRITE;
                            }
                            else // otherwise invalid
                            {
                                NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
                            }

                            NdefMap->FelicaAttrInfo.LenBytes[0] = NdefMap->SendRecvBuf[24];
                            NdefMap->FelicaAttrInfo.LenBytes[1] = NdefMap->SendRecvBuf[25];
                            NdefMap->FelicaAttrInfo.LenBytes[2] = NdefMap->SendRecvBuf[26];
                            status = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
                        }
                    }
                }
            }
        }
        else
        {
            CRFlag = TRUE;
        }
    }
    if ( (status == NFCSTATUS_INVALID_FORMAT ) && (CRFlag == TRUE ))
    {
        NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
    }
    if ( CRFlag == TRUE )
    {
        /*Return Status Error “ Invalid Format”*/
        status =  PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
    }

    return (status);
}

static
void
phFriNfc_Felica_HCrHandler(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             CrIndex,
    _In_    NFCSTATUS           Status
    )
{
    /* set the state back to the Reset_Init state*/
    NdefMap->State =  PH_FRINFC_NDEFMAP_STATE_RESET_INIT;

    /* set the completion routine*/
    NdefMap->CompletionRoutine[CrIndex].
        CompletionRoutine(NdefMap->CompletionRoutine->Context, Status);
}

static
void
phFriNfc_Felica_HInitInternalBuf(
    _Out_writes_bytes_all_(16) uint8_t *Buffer
    )
{
    uint8_t index=0;

    for( index = 0; index< 16 ; index++)
    {
        Buffer[index] = 0;
    }
}

static
int
phFriNfc_Felica_MemCompare (
    _In_reads_bytes_(n) void            *s1,
    _In_reads_bytes_(n) void            *s2,
    _In_                unsigned int    n
    )
{
    int8_t   diff = 0;
    int8_t *char_1  =(int8_t *)s1;
    int8_t *char_2  =(int8_t *)s2;
    if(NULL == s1 || NULL == s2)
    {
        PH_LOG_NDEF_CRIT_STR("NULL pointer passed to memcompare");
    }
    else
    {
        for(;((n>0)&&(diff==0));n--,char_1++,char_2++)
        {
            diff = *char_1 - *char_2;
        }
    }
    return (int)diff;
}
