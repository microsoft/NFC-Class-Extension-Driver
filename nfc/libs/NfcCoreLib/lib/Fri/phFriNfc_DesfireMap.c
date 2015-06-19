/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_DesfireMap.h"

#include "phFriNfc_DesfireMap.tmh"

#ifdef DESFIRE_EV1
    #define DESFIRE_EV1_P2_OFFSET_VALUE                         (0x0CU)
#endif /* #ifdef DESFIRE_EV1 */

static
NFCSTATUS
phFriNfc_Desfire_SelectSmartTag(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_SelectFile(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_ReadBinary(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_UpdateBinary(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_Update_SmartTagCapContainer(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );


/* Completion Helper*/
static
void
phFriNfc_Desfire_HCrHandler(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    NFCSTATUS           Status
    );

/* Calculates the Le Bytes for Read Operation*/
static
uint32_t
phFriNfc_Desfire_HGetLeBytes(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desf_HChkAndParseTLV(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             BuffIndex
    );

static
NFCSTATUS
phFriNfc_Desfire_HSetGet_NLEN(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_HProcReadData(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap
    );

static
void
phFriNfc_Desfire_HChkNDEFFileAccessRights(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    );

static
NFCSTATUS
phFriNfc_Desfire_HSendTransCmd(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             SendRecvLen
    );


NFCSTATUS
phFriNfc_Desfire_RdNdef(
    _Inout_                                                     phFriNfc_NdefMap_t  *NdefMap,
    _Out_writes_bytes_to_(*PacketDataLength, *PacketDataLength) uint8_t             *PacketData,
    _Inout_                                                     uint32_t            *PacketDataLength,
    _In_                                                        uint8_t             Offset
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    PH_LOG_NDEF_FUNC_ENTRY();
    NdefMap->ApduBufferSize = *PacketDataLength;
    /*  To return actual number of bytes read to the caller */
    NdefMap->NumOfBytesRead = PacketDataLength ;
    *NdefMap->NumOfBytesRead = 0;

    /* store the offset in to map context*/
    NdefMap->Offset = Offset;

    if( (Offset == PH_FRINFC_NDEFMAP_SEEK_CUR) &&
        (*NdefMap->DataCount == NdefMap->DesfireCapContainer.NdefDataLen))
    {
        /*  No space on card for Reading : we have already
        reached the end of file !
        Offset is set to Continue Operation */
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    else
    {

        /* reset the inter flag*/
        NdefMap->DesfireCapContainer.IsNlenPresentFlag = 0;
        NdefMap->DesfireCapContainer.SkipNlenBytesFlag = 0;

        /*  Set the desfire read operation  */
        NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_READ_OP;

        /*  Save the packet data buffer address in the context  */
        NdefMap->ApduBuffer = PacketData;

        NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_READ_OPE;

#ifdef DESFIRE_EV1
        /* Select smart tag operation. First step for the read operation. */
        if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == NdefMap->CardType)
        {
            status = phFriNfc_Desfire_SelectFile(NdefMap);
        }
        else
#endif /* #ifdef DESFIRE_EV1 */
        {
            status = phFriNfc_Desfire_SelectSmartTag(NdefMap);
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

/*!
* \brief Initiates Writing of NDEF information to the Remote Device.
*
* The function initiates the writing of NDEF information to a Remote Device.
* It performs a reset of the state and starts the action (state machine).
* A periodic call of the \ref phFriNfcNdefMap_Process has to be done once the action
* has been triggered.
*/

NFCSTATUS
phFriNfc_Desfire_WrNdef(
    _Inout_                             phFriNfc_NdefMap_t  *NdefMap,
    _In_reads_bytes_(*PacketDataLength) uint8_t             *PacketData,
    _Inout_                             uint32_t            *PacketDataLength,
    _In_                                uint8_t             Offset
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;

    NdefMap->ApduBufferSize = *PacketDataLength;
    NdefMap->WrNdefPacketLength = PacketDataLength;
    PH_LOG_NDEF_FUNC_ENTRY();
    /*  Now, let's initialize     *NdefMap->WrNdefPacketLength    to zero.
    In case we get an error, this will be correctly set to "no byte written".
    In case there is no error, this will be updated later on, in the _process function.
    */
    *NdefMap->WrNdefPacketLength =   0;

    /*  we have write access. */
    if( *NdefMap->DataCount >= PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE)
    {
        /*  No space on card for writing : we have already
        reached the end of file !
        Offset is set to Continue Operation */
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
    }
    else
    {
        /*  Adapt the nb of bytes that the user would like to write */

        /*set the defire write operation*/
        NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_WRITE_OP;
        NdefMap->PrevOperation = PH_FRINFC_NDEFMAP_WRITE_OPE;
        NdefMap->Offset = Offset;

        /*Store the packet data buffer*/
        NdefMap->ApduBuffer = PacketData;

#ifdef DESFIRE_EV1
        if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == NdefMap->CardType)
        {
            status = phFriNfc_Desfire_SelectFile(NdefMap);
        }
        else
#endif /* #ifdef DESFIRE_EV1 */
        {
            /* Select smart tag operation. First step for the write operation. */
            status = phFriNfc_Desfire_SelectSmartTag (NdefMap);
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

NFCSTATUS
phFriNfc_Desfire_ChkNdef(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS    status = NFCSTATUS_PENDING;
    PH_LOG_NDEF_FUNC_ENTRY();

#ifdef DESFIRE_EV1
    /* Reset card type */
    NdefMap->CardType = 0;
#endif /* #ifdef DESFIRE_EV1 */
    /*Set the desfire operation flag*/
    NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP;

    /*Call Select Smart tag Functinality*/
    status = phFriNfc_Desfire_SelectSmartTag (NdefMap);

    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}


static
NFCSTATUS
phFriNfc_Desf_HChkAndParseTLV(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             BuffIndex
    )
{
    /*
     * TODO: Annotate valid range for BuffIndex
     */

    NFCSTATUS    status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NdefMap->SendRecvBuf[BuffIndex] <= 0x03) ||
        (NdefMap->SendRecvBuf[BuffIndex] >= 0x06) )
    {
        status =  PHNFCSTVAL(   CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_NO_NDEF_SUPPORT);
    }
    else
    {
        /* check for the type of TLV*/
        NdefMap->TLVFoundFlag =
            ((NdefMap->SendRecvBuf[BuffIndex] == 0x04)?
            PH_FRINFC_NDEFMAP_DESF_NDEF_CNTRL_TLV:
            PH_FRINFC_NDEFMAP_DESF_PROP_CNTRL_TLV);

        status =  PHNFCSTVAL(   CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_SUCCESS);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

static
NFCSTATUS
phFriNfc_Desfire_HSetGet_NLEN(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS    status = NFCSTATUS_PENDING;
    PH_LOG_NDEF_FUNC_ENTRY();
    if ( PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP == NdefMap->DespOpFlag)
    {

        /*Call Select Smart tag Functinality*/
            status = phFriNfc_Desfire_SelectSmartTag (NdefMap);
    }
    else
    {

        /* Get the Data Count and set it to NoOfBytesWritten
        Update the NLEN using Transceive cmd*/

        /*Form the packet for the update binary command*/
        NdefMap->SendRecvBuf[0] = 0x00;
        NdefMap->SendRecvBuf[1] = 0xD6;

        /* As we need to set the NLEN @ first 2 bytes of NDEF File*/
        /* set the p1/p2 offsets */
        NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
        NdefMap->SendRecvBuf[3] = 0x00; /* p2 */

        /* Set only two bytes as NLEN*/
        NdefMap->SendRecvBuf[4] = 0x02;

        /* update NLEN */
        NdefMap->SendRecvBuf[5] = (uint8_t)(*NdefMap->DataCount >> PH_FRINFC_NDEFMAP_DESF_SHL8);
        NdefMap->SendRecvBuf[6] = (uint8_t)(*NdefMap->DataCount & (0x00ff));

        NdefMap->SendLength = 0x07 ;

        /* Change the state to Write */
        NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_END;

        status =  phFriNfc_Desfire_HSendTransCmd(NdefMap,PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}


static
NFCSTATUS
phFriNfc_Desfire_HProcReadData(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap
    )
{
    NFCSTATUS   Result = NFCSTATUS_PENDING;
    uint32_t    BufferSize = 0;
    uint8_t     BufIndex=0;
    uint16_t    SizeToCpy=0;
    uint16_t    SendRecvBufSize;
    PH_LOG_NDEF_FUNC_ENTRY();
    /* Need to check the Actual Ndef Length before copying the data to buffer*/
    /* Only NDEF data should be copied , rest all the data should be ignored*/
    /* Ex : Ndef File Size 50 bytes , but only 5 bytes(NLEN) are relavent to NDEF data*/
    /* component should only copy 5 bytes to user buffer*/

    /*  Data has been read successfully in the TRX buffer. */
    /*  copy it to the user buffer. */

    /* while copying need check the offset if its begin need to skip the first 2 bytes
    while copying. If its current no need to skip the first 2 bytes*/

    if ( NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )
    {
        BufIndex = (uint8_t)(( NdefMap->DesfireCapContainer.IsNlenPresentFlag == 1 )?
            0:PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES);

        /* Update the latest NLEN to context*/
        NdefMap->DesfireCapContainer.NdefDataLen = ((*NdefMap->DataCount == 0)?
            ( (((uint16_t)NdefMap->SendRecvBuf[
                PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_FIRST_INDEX])<<8)+ \
                    NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_SECOND_INDEX]):
                NdefMap->DesfireCapContainer.NdefDataLen);

        /* Decide how many byes to be copied into user buffer: depending upon the actual NDEF
           size need to copy the content*/
        if ( (NdefMap->DesfireCapContainer.NdefDataLen) <= (*NdefMap->SendRecvLength - \
            (PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET + BufIndex)))
        {
            SizeToCpy = NdefMap->DesfireCapContainer.NdefDataLen;
        }
        else
        {
            SizeToCpy = ((*NdefMap->SendRecvLength)-(PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET+BufIndex));
        }

        /* Check do we have Ndef Data len > 0 present in the card.If No Ndef Data
        present in the card , set the card state to Initalised and set an Error*/
        if ( NdefMap->DesfireCapContainer.NdefDataLen == 0x00 )
        {
            NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INITIALIZED;
            Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_EOF_NDEF_CONTAINER_REACHED);
            phFriNfc_Desfire_HCrHandler(NdefMap,Result);
        }
        else
        {
            if((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) < SizeToCpy)
            {
                PH_LOG_FRI_CRIT_STR("Not enough space in ApduBuffer! "
                                    "ApduBufferSize: %u "
                                    "ApduBuffIndex: %hu "
                                    "SizeToCpy: %hu",
                                    NdefMap->ApduBufferSize,
                                    NdefMap->ApduBuffIndex,
                                    SizeToCpy);

                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_BUFFER_TOO_SMALL);
                phFriNfc_Desfire_HCrHandler(NdefMap,Result);
                goto Done;
            }

            (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[
            NdefMap->ApduBuffIndex])),
            (&(NdefMap->SendRecvBuf[BufIndex])),
            (SizeToCpy));

            /*  Increment the Number of Bytes Read, which will be returned to the caller. */
            *NdefMap->NumOfBytesRead = (uint32_t)(*NdefMap->NumOfBytesRead + SizeToCpy);

            /*update the data count*/
            *NdefMap->DataCount = (uint16_t)(*NdefMap->DataCount + SizeToCpy);

            /*update the buffer index of the apdu buffer*/
            NdefMap->ApduBuffIndex = (uint16_t)(NdefMap->ApduBuffIndex + SizeToCpy);
        }
    }
    else
    {
            /* to avoid the length of the NDEF File*/
            SendRecvBufSize = *NdefMap->SendRecvLength - PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET;

            if((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) < SendRecvBufSize)
            {
                PH_LOG_FRI_CRIT_STR("Not enough space in ApduBuffer! "
                                    "ApduBufferSize: %u "
                                    "ApduBuffIndex: %hu "
                                    "SendRecvBufSize: %hu",
                                    NdefMap->ApduBufferSize,
                                    NdefMap->ApduBuffIndex,
                                    SendRecvBufSize);

                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_BUFFER_TOO_SMALL);
                phFriNfc_Desfire_HCrHandler(NdefMap,Result);
                goto Done;
            }

            (void)phOsalNfc_MemCopy( (&(NdefMap->ApduBuffer[
            NdefMap->ApduBuffIndex])),
                (NdefMap->SendRecvBuf),
                SendRecvBufSize);

            /*  Increment the Number of Bytes Read, which will be returned to the caller. */
            *NdefMap->NumOfBytesRead += SendRecvBufSize;

            /*update the data count*/
            *NdefMap->DataCount += SendRecvBufSize;

            /*update the buffer index of the apdu buffer*/
            NdefMap->ApduBuffIndex += SendRecvBufSize;
    }

    /*  check whether we still have to read some more data. */
    if (*NdefMap->DataCount < NdefMap->DesfireCapContainer.NdefDataLen )
    {
        /*  we have some bytes to read. */

        /*  Now check, we still have bytes left in the user buffer. */
        BufferSize = NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex;
        if(BufferSize != 0)
        {
            /* Before read need to set the flag to intimate the module to
            dont skip the first 2 bytes as we are in mode reading next
            continues available bytes, which will not contain the NLEN
            information in the begining part that is 2 bytes*/
            NdefMap->DesfireCapContainer.IsNlenPresentFlag = 1;
            /* Read Operation is not complete */
            Result = phFriNfc_Desfire_ReadBinary( NdefMap );
            /* handle the error in Transc function*/
            if  ( (Result & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER) )
            {
                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Result);
            }
        }
        else
        {
            /*  There are some more bytes to read, but
            no space in the user buffer */
            Result = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
            NdefMap->ApduBuffIndex =0;
            /* call respective CR */
            phFriNfc_Desfire_HCrHandler(NdefMap,Result);
        }
    }
    else
    {
        if (*NdefMap->DataCount == NdefMap->DesfireCapContainer.NdefDataLen )
        {
            /*  we have read all the bytes available in the card. */
            Result = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
        }
        else
        {
            /*  The control should not come here. */
            /*  we have actually read more byte than available in the card. */
            NdefMap->PrevOperation = 0;
            Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                NFCSTATUS_FAILED);
        }


        NdefMap->ApduBuffIndex = 0;

        /* call respective CR */
        phFriNfc_Desfire_HCrHandler(NdefMap,Result);
    }

Done:

    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

void
phFriNfc_Desfire_Process(
    _At_((phFriNfc_NdefMap_t*)Context, _Inout_) void        *Context,
    _In_                                        NFCSTATUS   Status
    )
{
    /*Set the context to Map Module*/
    phFriNfc_NdefMap_t  *NdefMap = (phFriNfc_NdefMap_t *)Context;
    uint8_t             ErrFlag = 0;
    uint16_t            NLength = 0,
                        SendRecLen=0;
    uint32_t            BytesRead = 0;

    PH_LOG_NDEF_FUNC_ENTRY();
    /* Fix for 0000255/0000257:[gk] MAP:Handling HAL Errors */
    if ( Status == NFCSTATUS_SUCCESS )
    {
        switch (NdefMap->State)
        {
#ifdef DESFIRE_EV1
            case PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG_EV1:
            {
                if(( NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength) - 2)] ==
                    PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                    (NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength) - 1)] ==
                    PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
                {
                    NdefMap->CardType = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1;

                    Status = phFriNfc_Desfire_SelectFile(NdefMap);

                    /* handle the error in Transc function*/
                    if ((Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                    {
                        /* call respective CR */
                        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                    }
                }
                else
                {
                    NdefMap->CardType = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD;
                    /* The card is not the new desfire, so send select smart tag command
                    of the old desfire */
                    Status = phFriNfc_Desfire_SelectSmartTag (NdefMap);
                }
                break;
            }
#endif /* #ifdef DESFIRE_EV1 */

        case PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG:
#ifdef DESFIRE_EV1
            if(( NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength) - 2)] ==
                    PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                    (NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength) - 1)] ==
                    PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
#else
            if(( NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW1_INDEX] ==
                PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW2_INDEX] ==
                PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
#endif /* #ifdef DESFIRE_EV1 */
            {
                Status = phFriNfc_Desfire_SelectFile(NdefMap);

                /* handle the error in Transc function*/
                if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                {
                    /* call respective CR */
                    phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                }
            }
            else
            {
                /*Error " Smart Tag Functionality Not Supported"*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                    NFCSTATUS_SMART_TAG_FUNC_NOT_SUPPORTED);
#ifdef DESFIRE_EV1
                NdefMap->CardType = 0;
#endif /* #ifdef DESFIRE_EV1 */

                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Status);

            }

            break;

        case PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_FILE :

            if(( NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW1_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW2_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
            {
                /*check for the which operation */
                if( (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_READ_OP) ||
                    (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP) ||
                    (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP ))
                {
                    /* call for read binary operation*/
                    Status = phFriNfc_Desfire_ReadBinary(NdefMap);

                    /* handle the error in Transc function*/
                    if  ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER) )
                    {
                        /* call respective CR */
                        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                    }
                }
                /*its a write Operation*/
                else if(NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_WRITE_OP )
                {
                    Status = phFriNfc_Desfire_UpdateBinary (NdefMap);
                    /* handle the error in Transc function*/
                    if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                    {
                        /* call respective CR */
                        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                    }
                }
                else
                {
                    /* unknown/invalid desfire operations*/
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                        NFCSTATUS_INVALID_REMOTE_DEVICE);

                    /* call respective CR */
                    phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                }
            }
            else
            {
                /*return Error " Select File Operation Failed"*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                    NFCSTATUS_INVALID_REMOTE_DEVICE);

                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            }
            break;

        case PH_FRINFC_NDEFMAP_DESF_STATE_READ_CAP_CONT:
            if( (NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength)-2)] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[(*(NdefMap->SendRecvLength)-1)] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
            {
                /*  Read successful. */
                /*Update the smart tag capability container*/
                Status = phFriNfc_Desfire_Update_SmartTagCapContainer(NdefMap);

                if ( Status == NFCSTATUS_SUCCESS)
                {
                    NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP;
#ifdef DESFIRE_EV1
                    if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == NdefMap->CardType)
                    {
                        Status = phFriNfc_Desfire_SelectFile(NdefMap);
                    }
                    else
#endif /* #ifdef DESFIRE_EV1 */
                    {
                        Status = phFriNfc_Desfire_HSetGet_NLEN(NdefMap);
                    }
                    /* handle the error in Transc function*/
                    if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
                    {
                        /* call respective CR */
                        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                    }
                }
                else
                {
                    /* call respective CR */
                    phFriNfc_Desfire_HCrHandler(NdefMap,Status);

                }

            }
            else
            {
                /*return Error " Capability Container Not Found"*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                    NFCSTATUS_INVALID_REMOTE_DEVICE);
                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            }
            break;

        case  PH_FRINFC_NDEFMAP_DESF_STATE_READ_BIN:

            /* Check how many bytes have been read/returned from the card*/
            BytesRead = phFriNfc_Desfire_HGetLeBytes(NdefMap);

            /* set the send recev len*/
            SendRecLen = *NdefMap->SendRecvLength - (PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET );
            if ( (NdefMap->DesfireCapContainer.SkipNlenBytesFlag  == 1) && ((BytesRead == 1) || (BytesRead == 2 )))
            {
                BytesRead += PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES; /* to take care of first 2 len bytes*/
            }
            else
            {
                /* Nothing to process*/
            }
            /* Read More Number Of Bytes than Expected*/
            if ( ( BytesRead == SendRecLen ) &&
                ((NdefMap->SendRecvBuf[(*NdefMap->SendRecvLength-2)] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[(*NdefMap->SendRecvLength-1)] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE)))

            {
                /* this is to check the card state in first Read Operation*/
                if ( NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP )
                {
                    /* check the actual length of the ndef data : NLEN*/
                    NLength = ( (((uint16_t)NdefMap->SendRecvBuf[0])<<PH_FRINFC_NDEFMAP_DESF_SHL8)+ \
                        NdefMap->SendRecvBuf[1]);
                    if (( NLength > PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE )||
                        ( NLength == 0xFFFF))
                    {
                        ErrFlag = 1;
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_NO_NDEF_SUPPORT);
                    }
                    else
                    {
                        /* Store the NLEN into the context  */
                        NdefMap->DesfireCapContainer.NdefDataLen = NLength;

                        Status = phFriNfc_MapTool_SetCardState( NdefMap,
                            NLength);
                        if ( Status == NFCSTATUS_SUCCESS )
                        {
                            /*Set the card type to Desfire*/
#ifndef DESFIRE_EV1
                            NdefMap->CardType = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD;
#endif /* #ifdef DESFIRE_EV1 */
                            /*Set the state to specify True for Ndef Compliant*/
                            NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_CHK_NDEF;

                            /*set the data count back to zero*/;
                            *NdefMap->DataCount = 0;
                            /*set the apdu buffer index to zero*/
                            NdefMap->ApduBuffIndex = 0;
                            /* Set the Operationg flag to Complete check NDEF Operation*/
                            NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP;

                        }
                         /* call respective CR */
                        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                    }/* End ofNdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP*/
                }
                else if ( NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_READ_OP )
                {
                    phFriNfc_Desfire_HProcReadData(NdefMap);
                }
                else
                {
                    /* Invalid Desfire Operation  */
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                        NFCSTATUS_INVALID_REMOTE_DEVICE);
                    phFriNfc_Desfire_HCrHandler(NdefMap,Status);
                }

            }
            else
            {
                /*handle the  Error case*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                    NFCSTATUS_READ_FAILED);
                ErrFlag =1;
            }
            if( ErrFlag == 1)
            {
                *NdefMap->DataCount = 0;

                /*set the buffer index back to zero*/
                NdefMap->ApduBuffIndex = 0;

                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            }

            break;

        case  PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_BEGIN:
            if( (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW1_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW2_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
            {
                /*  Write operation was successful. */
                /*  NdefMap->NumOfBytesWritten have been written on to the card.
                Update the DataCount and the ApduBufferIndex */
                *NdefMap->DataCount = (uint16_t)(*NdefMap->DataCount +
                                                NdefMap->NumOfBytesWritten);
                NdefMap->ApduBuffIndex = (uint16_t)(NdefMap->ApduBuffIndex +
                                                NdefMap->NumOfBytesWritten);

                /*  Update the user-provided buffer size to write */
                *NdefMap->WrNdefPacketLength += NdefMap->NumOfBytesWritten;

                /*  Call Upadte Binary function to check if some more bytes are to be written. */
                Status = phFriNfc_Desfire_UpdateBinary( NdefMap );
            }
            else
            {
                /*handle the  Error case*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                    NFCSTATUS_WRITE_FAILED);

                /*set the buffer index back to zero*/
                NdefMap->ApduBuffIndex = 0;

                /* call respective CR */
                phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            }
            break;
        case PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_END :
            if((NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW1_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW1_BYTE) &&
                (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_SW2_INDEX] == PH_FRINFC_NDEFMAP_DESF_RAPDU_SW2_BYTE))
            {
                /*  Updating NLEN operation was successful. */
                /* Entire Write Operation is complete*/
                /*  Reset the relevant parameters. */
                Status = PHNFCSTVAL(CID_NFC_NONE,\
                    NFCSTATUS_SUCCESS);

                /* set the state & Data len into context*/
                NdefMap->CardState = (uint8_t)((NdefMap->CardState ==
                                            PH_NDEFMAP_CARD_STATE_INITIALIZED)?
                                            PH_NDEFMAP_CARD_STATE_READ_WRITE :
                                            NdefMap->CardState);

                NdefMap->DesfireCapContainer.NdefDataLen = (uint16_t)(*NdefMap->WrNdefPacketLength);
            }
            else
            {
                NdefMap->PrevOperation = 0;
                /*handle the  Error case*/
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                    NFCSTATUS_WRITE_FAILED);
            }

            /*set the buffer index back to zero*/
            NdefMap->ApduBuffIndex = 0;

            /* call respective CR */
            phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            break;

        default:
            /*define the invalid state*/
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,\
                NFCSTATUS_INVALID_DEVICE_REQUEST);
            phFriNfc_Desfire_HCrHandler(NdefMap,Status);
            break;
        }
    }
    else
    {
        /* call respective CR */
        phFriNfc_Desfire_HCrHandler(NdefMap,Status);
    }
    PH_LOG_NDEF_FUNC_EXIT();

    UNREFERENCED_PARAMETER(Status);
}

NFCSTATUS
phFriNfc_DesfCapCont_HReset(
    _Inout_ phFriNfc_NdefMap_t      *NdefMap,
    _In_    phNfc_sDevInputParam_t  *pDevInpParam
    )
{
    NFCSTATUS Result = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    UNUSED(pDevInpParam);
    if(NdefMap == NULL)
    {
        Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Initialise/reset the desfire capability contatiner structure variables*/
        NdefMap->DesfireCapContainer.DesfVersion = 0;
        NdefMap->DesfireCapContainer.NdefMsgFid  = 0;
        NdefMap->DesfireCapContainer.NdefFileSize = 0;
        NdefMap->DesfireCapContainer.MaxCmdSize  = 0;
        NdefMap->DesfireCapContainer.MaxRespSize = 0;
        NdefMap->DesfireCapContainer.ReadAccess  = 0;
        NdefMap->DesfireCapContainer.WriteAccess = 0;
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

NFCSTATUS
phFrinfc_Desfire_GetContainerSize(
    _In_    const phFriNfc_NdefMap_t    *NdefMap,
    _Out_   uint32_t                    *maxSize,
    _Out_   uint32_t                    *actualSize
    )
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if((NULL != NdefMap) && (NULL != maxSize) &&(NULL != actualSize))
    {
        /*  Desfire card */
        /*  The integration needs to ensure that the checkNdef
        function has been called before calling this function,
        otherwise NdefMap->DesfireCapContainer.NdefFileSize
        will be 0 */
        /* -2 bytes represents the size field bytes*/
        *maxSize = NdefMap->DesfireCapContainer.NdefFileSize - 2;
        /* In Desfire card, the actual size cant be calculated so
        the actual size is given as 0xFFFFFFFF */
        *actualSize = NdefMap->DesfireCapContainer.NdefDataLen;
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
phFriNfc_Desfire_SelectSmartTag(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS                   status = NFCSTATUS_PENDING;
#ifdef DESFIRE_EV1
    uint8_t                     card_type = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1;
#endif /* #ifdef DESFIRE_EV1 */
    PH_LOG_NDEF_FUNC_ENTRY();
    /*form the packet for Select smart tag command*/
    NdefMap->SendRecvBuf[0]  = 0x00; /* cls */
    NdefMap->SendRecvBuf[1]  = 0xa4; /* ins */
    NdefMap->SendRecvBuf[2]  = 0x04; /* p1 */
    NdefMap->SendRecvBuf[3]  = 0x00; /* p2 */
    NdefMap->SendRecvBuf[4]  = 0x07; /* lc */

    /* next 7 bytes specify the DF Name*/
    NdefMap->SendRecvBuf[5]  = 0xd2;
    NdefMap->SendRecvBuf[6]  = 0x76;
    NdefMap->SendRecvBuf[7]  = 0x00;
    NdefMap->SendRecvBuf[8]  = 0x00;
    NdefMap->SendRecvBuf[9]  = 0x85;
    NdefMap->SendRecvBuf[10] = 0x01;

#ifdef DESFIRE_EV1

    switch (NdefMap->DespOpFlag)
    {
        case PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP:
        {
            /* First select the smart tag using the new desfire EV1 and increment the
                "sel_index" and if it fails then try the old desfire select smart tag
                command */
            if (0 == NdefMap->CardType)
            {
                /* p2
                NdefMap->SendRecvBuf[3]  = DESFIRE_EV1_P2_OFFSET_VALUE; */
                NdefMap->SendRecvBuf[11] = 0x01;
                /* Le */
                NdefMap->SendRecvBuf[12] = 0x00;
                NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG_EV1;
                card_type = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1;
            }
            else
            {
                NdefMap->SendRecvBuf[3]  = 0x00; /* p2 */
                NdefMap->SendRecvBuf[11] = 0x00;
                NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG;
                card_type = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD;
            }
            break;
        }

        case PH_FRINFC_NDEFMAP_DESF_READ_OP:
        default :
        {
            if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == NdefMap->CardType)
            {
                NdefMap->SendRecvBuf[11] = 0x01;
                NdefMap->SendRecvBuf[12] = 0x00;
                NdefMap->State = (uint8_t)PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG_EV1;
                card_type = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1;
            }
            else
            {
                NdefMap->SendRecvBuf[11] = 0x00;
                NdefMap->State = (uint8_t)PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG;
                card_type = PH_FRINFC_NDEFMAP_ISO14443_4A_CARD;
            }
            break;
        }
    }

#else /* #ifdef DESFIRE_EV1 */

    NdefMap->SendRecvBuf[11] = 0x00;

#endif /* #ifdef DESFIRE_EV1 */

    /*Set the Send length*/
    NdefMap->SendLength = PH_FRINFC_NDEFMAP_DESF_CAPDU_SMARTTAG_PKT_SIZE;
#ifdef DESFIRE_EV1

    if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == card_type)
    {
        /* Send length is updated for the NEW DESFIRE EV1 */
        NdefMap->SendLength = (uint16_t)(NdefMap->SendLength + 1);
    }

#else
    /* Change the state to Select Smart Tag */
    NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_SMART_TAG;
#endif /* #ifdef DESFIRE_EV1 */

    status = phFriNfc_Desfire_HSendTransCmd(NdefMap,PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET);
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

static
NFCSTATUS
phFriNfc_Desfire_SelectFile(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS                   status = NFCSTATUS_PENDING;
    PH_LOG_NDEF_FUNC_ENTRY();

    /* check for the invalid/unknown desfire operations*/
    if ((NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP)&& \
        (NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_READ_OP)&&\
        ( NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_WRITE_OP) &&
        ( NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP))
    {
        status  =   PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, NFCSTATUS_INVALID_REMOTE_DEVICE);
    }
    else
    {

        /*  Set the command*/
        //NdefMap->Cmd.Iso144434Cmd = phHal_eIso14443_4_CmdListTClCmd;

        /*  Form the packet for select file command either for the
        Check Ndef/Read/Write functionalities*/
        if ( (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_READ_OP) &&
            (NdefMap->bDtaFlag))
        {
            NdefMap->SendRecvBuf[0] = 0x00; /* cls */
            NdefMap->SendRecvBuf[1] = 0xB0; /* ins */
            NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
            NdefMap->SendRecvBuf[3] = 0x02; /* p2 */
            NdefMap->SendRecvBuf[4] = (uint8_t)NdefMap->DesfireCapContainer.NdefDataLen; /* Ndef Len */
        }
        else if ( (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_WRITE_OP) &&
            (NdefMap->bDtaFlag) )
        {
            NdefMap->SendRecvBuf[0] = 0x00; /* cls */
            NdefMap->SendRecvBuf[1] = 0xD6; /* ins */
            NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
            NdefMap->SendRecvBuf[3] = 0x00; /* p2 */
            NdefMap->SendRecvBuf[4] = 0x05; /* lc */
            NdefMap->SendRecvBuf[5] = 0x00; /* Nlen */
            NdefMap->SendRecvBuf[6] = 0x03; /* Nlen */
            NdefMap->SendRecvBuf[7] = 0xD0; /* Ndef Message */
            NdefMap->SendRecvBuf[8] = 0x00; /* Ndef Message */
            NdefMap->SendRecvBuf[9] = 0x00; /* Ndef Message */
        }
        else
        {
            NdefMap->SendRecvBuf[0] = 0x00; /* cls */
            NdefMap->SendRecvBuf[1] = 0xa4; /* ins */
            NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
            NdefMap->SendRecvBuf[3] = 0x00; /* p2 */
            NdefMap->SendRecvBuf[4] = 0x02; /* lc */
        }

#ifdef DESFIRE_EV1
        if (PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1 == NdefMap->CardType)
        {
            if(!NdefMap->bDtaFlag)
            {
                NdefMap->SendRecvBuf[3]  = DESFIRE_EV1_P2_OFFSET_VALUE; /* p2 */
            }
            else if ((NdefMap->bDtaFlag) &&
                (NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_READ_OP) &&
                (NdefMap->DespOpFlag != PH_FRINFC_NDEFMAP_DESF_WRITE_OP))
            {
                NdefMap->SendRecvBuf[3]  = DESFIRE_EV1_P2_OFFSET_VALUE; /* p2 */
            }
            else
            {
                /* Do nothing for other operations in DTA mode */
            }
        }
#endif /* #ifdef DESFIRE_EV1 */

        if ( (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP))

        {
            /* cap container file identifier*/
            NdefMap->SendRecvBuf[5] = 0xe1;
            NdefMap->SendRecvBuf[6] = 0x03;
        }
        /* Mantis entry 0394 fixed */
        else 
        {
            if ( ( NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_WRITE_OP) &&
                (NdefMap->bDtaFlag) )
            {
                /* Do nothing on DTA mode */
            }
            else
            {
                NdefMap->SendRecvBuf[5] = (uint8_t)((NdefMap->DesfireCapContainer.NdefMsgFid) >> PH_FRINFC_NDEFMAP_DESF_SHL8);
                NdefMap->SendRecvBuf[6] = (uint8_t)((NdefMap->DesfireCapContainer.NdefMsgFid) & (0x00ff));
            }
        }
        /*Set the Send length*/
        NdefMap->SendLength = PH_FRINFC_NDEFMAP_DESF_CAPDU_SELECT_FILE_PKT_SIZE;

        /* Change the state to Select File */
        NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_SELECT_FILE;

        status = phFriNfc_Desfire_HSendTransCmd(NdefMap,PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET);

    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}

static
NFCSTATUS
phFriNfc_Desfire_ReadBinary(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS    status = NFCSTATUS_PENDING;
    uint32_t     BytesToRead = 0;
    uint8_t      BufIndex=0,OperFlag=0;
    uint16_t     DataCnt=0;

    PH_LOG_NDEF_FUNC_ENTRY();
    /* to read the capability container data*/
    if (NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP )
    {
        /*specifies capability container shall be read*/
        NdefMap->SendRecvBuf[0] = 0x00;
        NdefMap->SendRecvBuf[1] = 0xb0;
        NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
        NdefMap->SendRecvBuf[3] = 0x00; /* p2 */
        NdefMap->SendRecvBuf[4] = 0x0F; /* le */

        NdefMap->SendLength = PH_FRINFC_NDEFMAP_DESF_CAPDU_READ_BIN_PKT_SIZE;

        /* Change the state to Cap Container Read */
        NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_READ_CAP_CONT;

        /* set the send receive buffer length*/
        OperFlag = 1;
    }
    /*desfire file read operation*/
    else
    {
        NdefMap->SendRecvBuf[0] = 0x00;
        NdefMap->SendRecvBuf[1] = 0xb0;

        /*TBD the NLEN bytes*/
        if( *NdefMap->DataCount == 0 )
        {
            /* first read */
            /* set the offset p1 and p2*/
            NdefMap->SendRecvBuf[2] = 0;
            NdefMap->SendRecvBuf[3] = 0;
        }
        else
        {
            /* as the p1 of the 8bit is 0, p1 and p2 are used to store the
            ofset value*/
            DataCnt = *NdefMap->DataCount;
            DataCnt += PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES;
            NdefMap->SendRecvBuf[2] = (uint8_t)((DataCnt)>> PH_FRINFC_NDEFMAP_DESF_SHL8);
            NdefMap->SendRecvBuf[3] = (uint8_t)((DataCnt)& (0x00ff));
        }
        /* calculate the Le Byte*/
        BytesToRead = phFriNfc_Desfire_HGetLeBytes(NdefMap);

        if ( NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )
        {
            /* BufIndex represents the 2 NLEN bytes and decides about the presence of
            2 bytes NLEN data*/

            BufIndex = (uint8_t)(( NdefMap->DesfireCapContainer.SkipNlenBytesFlag == 1 ) ?
            PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES:0);

            if( ((BytesToRead == 1) || (BytesToRead == 2)) && (NdefMap->DesfireCapContainer.SkipNlenBytesFlag == 1))
            {
                BytesToRead += BufIndex;
            }
        }

        /* set the Le byte*/
        /* This following code is true for get nlen  and current offset set*/
        NdefMap->SendRecvBuf[4]=(uint8_t) BytesToRead  ;

        /* Change the state to Read */
        NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_READ_BIN;

        /*set the send length*/
        NdefMap->SendLength = PH_FRINFC_NDEFMAP_DESF_CAPDU_READ_BIN_PKT_SIZE;
        OperFlag = 2;
    }

    if (OperFlag == 1 )
    {
        status = phFriNfc_Desfire_HSendTransCmd(NdefMap,PH_FRINFC_NDEFMAP_MAX_SEND_RECV_BUF_SIZE);
    }
    else
    {
        status = phFriNfc_Desfire_HSendTransCmd(NdefMap,(uint8_t)(BytesToRead +PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET));
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}

static
NFCSTATUS
phFriNfc_Desfire_UpdateBinary(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    NFCSTATUS   status = NFCSTATUS_PENDING;
    uint16_t    noOfBytesToWrite = 0, DataCnt=0,
        index=0;

    PH_LOG_NDEF_FUNC_ENTRY();
    /*  Do we have space in the file to write? */
    if ( (*NdefMap->DataCount < PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE) &&
        (NdefMap->ApduBuffIndex < NdefMap->ApduBufferSize))
    {
        /*  Yes, we have some bytes to write */
        /*  Check and set the card memory size , if user sent bytes are more than the
        card memory size*/
        if( (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >\
            (uint16_t)(PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE - *NdefMap->DataCount))
        {
            NdefMap->ApduBufferSize =( (PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE) - (*NdefMap->DataCount + NdefMap->ApduBuffIndex));
        }

        /*  Now, we have space in the card to write the data, */
        /*Form the packet for the update binary command*/
        NdefMap->SendRecvBuf[0] = 0x00;
        NdefMap->SendRecvBuf[1] = 0xD6;

        if( *NdefMap->DataCount == 0)
        {
            /* set the p1/p2 offsets */
            NdefMap->SendRecvBuf[2] = 0x00; /* p1 */
            NdefMap->SendRecvBuf[3] = 0x00; /* p2 */
            NdefMap->DesfireCapContainer.SkipNlenBytesFlag = 0;
        }
        else
        {
            /*  as the p1 of the 8bit is 0, p1 and p2 are used to store the
            ofset value*/
            /*  This sets card offset in a card for a write operation. + 2 is
            added as first 2 offsets represents the size of the NDEF Len present
            in the file*/

            DataCnt = *NdefMap->DataCount;
            DataCnt += PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES;
            NdefMap->SendRecvBuf[2] = (uint8_t)((DataCnt)>> PH_FRINFC_NDEFMAP_DESF_SHL8);
            NdefMap->SendRecvBuf[3] = (uint8_t)((DataCnt)& (0x00ff));
            /* No need to attach 2 NLEN bytes at the begining.
            as we have already attached in the first write operation.*/
            NdefMap->DesfireCapContainer.SkipNlenBytesFlag = 1;

        }

        /*  Calculate the bytes to write */
        if( (NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= (uint32_t)( NdefMap->DesfireCapContainer.MaxCmdSize ))
        {
            noOfBytesToWrite = ( ( NdefMap->DesfireCapContainer.SkipNlenBytesFlag == 1) ?
                NdefMap->DesfireCapContainer.MaxCmdSize :
            (NdefMap->DesfireCapContainer.MaxCmdSize - PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES));
        }
        else
        {
            /*  Read only till the available buffer space */
            noOfBytesToWrite = (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
        }

        if ( NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )
        {
            if ( NdefMap->DesfireCapContainer.SkipNlenBytesFlag == 1 )
            {
                index = 5;
                /* To Specify the NDEF data len written : updated at the of write cycle*/
                NdefMap->SendRecvBuf[4] = (uint8_t)noOfBytesToWrite;
            }
            else
            {
                /* Leave space to update NLEN */
                NdefMap->SendRecvBuf[5] = 0x00;
                NdefMap->SendRecvBuf[6] = 0x00;
                index =7;
                /* To Specify the NDEF data len written : updated at the of write cycle*/
                NdefMap->SendRecvBuf[4] = (uint8_t)noOfBytesToWrite + PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES;
            }

            /* copy the data to SendRecvBuf from the apdu buffer*/
            (void)phOsalNfc_MemCopy( &NdefMap->SendRecvBuf[index],
                &NdefMap->ApduBuffer[NdefMap->ApduBuffIndex],
                noOfBytesToWrite);
            NdefMap->SendLength = (noOfBytesToWrite + index);
        }
        else
        {
            NdefMap->SendRecvBuf[4] = (uint8_t)noOfBytesToWrite;

            /* copy the data to SendRecvBuf from the apdu buffer*/
            (void)phOsalNfc_MemCopy( &NdefMap->SendRecvBuf[5],
                &NdefMap->ApduBuffer[NdefMap->ApduBuffIndex],
                noOfBytesToWrite);
            NdefMap->SendLength = (noOfBytesToWrite + 5);
        }

        /*  Store the number of bytes being written in the context structure, so that
        the parameters can be updated, after a successful write operation. */
        NdefMap->NumOfBytesWritten = noOfBytesToWrite;

        /* Change the state to Write */
        NdefMap->State = PH_FRINFC_NDEFMAP_DESF_STATE_UPDATE_BIN_BEGIN;

        status = phFriNfc_Desfire_HSendTransCmd(NdefMap,PH_FRINFC_NDEFMAP_DESF_RESP_OFFSET);

    }   /*  if(NdefMap->ApduBuffIndex < NdefMap->ApduBufferSize) */
    else
    {
        if ( (*NdefMap->DataCount == PH_NFCFRI_NDEFMAP_DESF_NDEF_FILE_SIZE) ||
            (NdefMap->ApduBuffIndex == NdefMap->ApduBufferSize))
        {
            /* The NdefMap->DespOpFlag = PH_FRINFC_NDEFMAP_DESF_SET_LEN_OP is not
                required, because the DespOpFlag shall be WRITE_OP
                */
            /* Update the NLEN Bytes*/
            status = phFriNfc_Desfire_HSetGet_NLEN(NdefMap);
        }
        else
        {
            /*  The control should not come here.
            wrong internal calculation.
            we have actually written more than the space available
            in the card ! */
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                NFCSTATUS_FAILED);
            /*  Reset the relevant parameters. */
            NdefMap->ApduBuffIndex = 0;
            NdefMap->PrevOperation = 0;

            /* call respective CR */
            phFriNfc_Desfire_HCrHandler(NdefMap,status);
        }

    }
    PH_LOG_NDEF_FUNC_EXIT();
    return status;
}


static
void
phFriNfc_Desfire_HChkNDEFFileAccessRights(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    PH_LOG_NDEF_FUNC_ENTRY();
    if ( (NdefMap->DesfireCapContainer.ReadAccess  == 0x00) &&
        (NdefMap->DesfireCapContainer.WriteAccess == 0x00 ))
    {
        /* Set the card state to Read/write State*/
        /* This state can be either INITIALISED or READWRITE. but default
        is INITIALISED */
        NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_WRITE;

    }
    else if((NdefMap->DesfireCapContainer.ReadAccess  == 0x00) &&
        (NdefMap->DesfireCapContainer.WriteAccess == 0xFF ))
    {
        /* Set the card state to Read Only State*/
        NdefMap->CardState = PH_NDEFMAP_CARD_STATE_READ_ONLY;
    }
    else
    {
        /* Set the card state to invalid State*/
        NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
    }
    PH_LOG_NDEF_FUNC_EXIT();
}

static
NFCSTATUS
phFriNfc_Desfire_Update_SmartTagCapContainer(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    uint16_t    CapContSize = 0,
        /* this is initalised 2 because CCLEN includes the field size bytes i.e 2bytes*/
        CCLen= 0;
    uint16_t    max_rsp_size;
    uint16_t    max_cmd_size;
    uint8_t     ErrFlag = 0;

    NFCSTATUS status= NFCSTATUS_SUCCESS;

    PH_LOG_NDEF_FUNC_ENTRY();
    /*Check the Size of Cap Container */
    CapContSize = ( (((uint16_t)NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_FIRST_INDEX])<<8)+ \
        NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_CCLEN_BYTE_SECOND_INDEX]);

    CCLen += 2;

    if ( (CapContSize < 0x0f) || (CapContSize == 0xffff))
    {
        ErrFlag =1;
    }
    else
    {
        /*Version : Smart Tag Spec version */
        /* check for the validity of Major and Minor Version numbers*/
        status = phFriNfc_MapTool_ChkSpcVer (   NdefMap,
            PH_FRINFC_NDEFMAP_DESF_VER_INDEX);
        if ( status  !=  NFCSTATUS_SUCCESS )
        {
            ErrFlag =1;
        }
        else
        {
            CCLen += 1;

            /*Get Response APDU data size
            to check the integration s/w response size*/
            max_rsp_size =
                 ((((uint16_t)NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_MLE_BYTE_FIRST_INDEX]) << 8)\
                + NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_MLE_BYTE_SECOND_INDEX]);
            NdefMap->DesfireCapContainer.MaxRespSize =
                            ((max_rsp_size > PHNFC_MAX_DATASIZE)?
                            (PHNFC_MAX_DATASIZE) : max_rsp_size);
            /*Get Command APDU data size*/
            max_cmd_size =
                ((((uint16_t)NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_MLC_BYTE_FIRST_INDEX])<<8)\
                + NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_MLC_BYTE_SECOND_INDEX]);

            NdefMap->DesfireCapContainer.MaxCmdSize =
                            ((max_cmd_size > PHNFC_MAX_DATASIZE)?
                            (PHNFC_MAX_DATASIZE): max_cmd_size);

            /* Check for the Validity of Cmd & Resp Size*/
            /* check the Validity of the Cmd Size*/
            if( (NdefMap->DesfireCapContainer.MaxRespSize < 0x0f) ||
                ( NdefMap->DesfireCapContainer.MaxCmdSize == 0x00))
            {
                ErrFlag=1;

            }
            else
            {
                CCLen += 4;

                /* Check and Parse the TLV structure */
                /* In future this chk can be extended to Propritery TLV */
                //status = phFriNfc_ChkAndParseTLV(NdefMap);
                status = phFriNfc_Desf_HChkAndParseTLV(NdefMap,PH_FRINFC_NDEFMAP_DESF_TLV_INDEX);
                if ( (status == NFCSTATUS_SUCCESS) && (NdefMap->TLVFoundFlag == PH_FRINFC_NDEFMAP_DESF_NDEF_CNTRL_TLV))
                {
                    CCLen += 1;

                    /* check the TLV length*/
                    if ( (( NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_TLV_LEN_INDEX]) > 0x00 ) &&
                        (( NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_TLV_LEN_INDEX]) <= 0xFE )&&
                        (( NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_TLV_LEN_INDEX]) == 0x06 ))
                    {
                        CCLen +=1;
                        /* store the contents in to the container structure*/
                        NdefMap->DesfireCapContainer.NdefMsgFid = ( (((uint16_t)NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILEID_BYTE_FIRST_INDEX])<<PH_FRINFC_NDEFMAP_DESF_SHL8)+ \
                            NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILEID_BYTE_SECOND_INDEX]);

                        CCLen +=2;

                        /* Invalid Msg File Id : User Can't Have read/write Opeartion*/
                        if (    (NdefMap->DesfireCapContainer.NdefMsgFid == 0xFFFF) ||
                            (NdefMap->DesfireCapContainer.NdefMsgFid == 0xE102) ||
                            (NdefMap->DesfireCapContainer.NdefMsgFid == 0xE103) ||
                            (NdefMap->DesfireCapContainer.NdefMsgFid == 0x3F00) ||
                            (NdefMap->DesfireCapContainer.NdefMsgFid == 0x3FFF ) )
                        {

                            ErrFlag=1;
                        }
                        else
                        {
                            /*Get Ndef Size*/
                            NdefMap->DesfireCapContainer.NdefFileSize =
                                ((((uint16_t)NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILESZ_BYTE_FIRST_INDEX])<<8)
                                | (NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILESZ_BYTE_SECOND_INDEX] & 0x00ff));


                            /*Check Ndef Size*/
                            /* TBD : Do we need to minus 2 bytes of size it self?*/
                            if ( ((NdefMap->DesfireCapContainer.NdefFileSize -2) <= 0x0004 ) ||
                                ((NdefMap->DesfireCapContainer.NdefFileSize -2) == 0xFFFD ) )
                            {
                                ErrFlag=1;
                            }
                            else
                            {
                                CCLen +=2;

                                /*Ndef File Read Access*/
                                NdefMap->DesfireCapContainer.ReadAccess = NdefMap->\
                                    SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILERD_ACCESS_INDEX] ;

                                /*Ndef File Write Access*/
                                NdefMap->DesfireCapContainer.WriteAccess = NdefMap->SendRecvBuf[PH_FRINFC_NDEFMAP_DESF_NDEF_FILEWR_ACCESS_INDEX];

                                CCLen +=2;

                                phFriNfc_Desfire_HChkNDEFFileAccessRights(NdefMap);
                            }
                        }
                    }
                    else
                    {

                        /* TLV Lenth is of two byte value
                        TBD: As the length of TLV is fixed for 6 bytes. We need not
                        handle the 2 byte value*/
                    }
                }
                else
                {
                    if ( NdefMap->TLVFoundFlag == PH_FRINFC_NDEFMAP_DESF_PROP_CNTRL_TLV )
                    {
                        /*TBD: To Handle The Proprietery TLV*/
                    }
                    else
                    {
                        /*Invalid T found case*/
                        ErrFlag =1;
                    }
                }
                /* check for the entire LENGTH Validity
                CCLEN + TLV L value == CCLEN*/
                if ( CapContSize < CCLen )
                {
                    ErrFlag=1;
                }

            }
        }
    }
    if( ErrFlag == 1 )
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
            NFCSTATUS_NO_NDEF_SUPPORT);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return ( status );
}


static
uint32_t
phFriNfc_Desfire_HGetLeBytes(
    _Inout_ phFriNfc_NdefMap_t *NdefMap
    )
{
    /*Represents the LE byte*/
    uint16_t BytesToRead =0;

    PH_LOG_NDEF_FUNC_ENTRY();
    if ( NdefMap->DespOpFlag == PH_FRINFC_NDEFMAP_DESF_GET_LEN_OP )
    {
        BytesToRead = PH_FRINFC_NDEFMAP_DESF_NLEN_SIZE_IN_BYTES;
         NdefMap->DesfireCapContainer.SkipNlenBytesFlag =0;
    }
    else
    {

        /* Calculate Le bytes : No of bytes to read*/
        /* Check for User Apdu Buffer Size and Msg Size of Desfire Capability container */
        if((NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex) >= NdefMap->DesfireCapContainer.MaxRespSize)
        {
            /*  We have enough buffer space to read the whole capability container
            size bytes
            Now, check do we have NdefMap->DesfireCapContainer.MaxRespSize to read ? */

            BytesToRead = (((NdefMap->DesfireCapContainer.NdefDataLen - *NdefMap->DataCount) >=
                NdefMap->DesfireCapContainer.MaxRespSize) ?
                NdefMap->DesfireCapContainer.MaxRespSize :
            (NdefMap->DesfireCapContainer.NdefDataLen -
                *NdefMap->DataCount));
        }
        else
        {
            /*  Read only till the available buffer space */
            BytesToRead = (uint16_t)(NdefMap->ApduBufferSize - NdefMap->ApduBuffIndex);
            if(BytesToRead >= (uint16_t)(NdefMap->DesfireCapContainer.NdefDataLen - *NdefMap->DataCount))
            {
                BytesToRead = (NdefMap->DesfireCapContainer.NdefDataLen - *NdefMap->DataCount);
            }
        }

        NdefMap->DesfireCapContainer.SkipNlenBytesFlag =
            (uint8_t)(((NdefMap->Offset == PH_FRINFC_NDEFMAP_SEEK_BEGIN )&&( *NdefMap->DataCount == 0 )) ?
            1 : 0);

    }
    PH_LOG_NDEF_FUNC_EXIT();
    return (BytesToRead);
}

static
void
phFriNfc_Desfire_HCrHandler(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    NFCSTATUS           Status
    )
{
    /* set the state back to the Reset_Init state*/
    NdefMap->State =  PH_FRINFC_NDEFMAP_STATE_RESET_INIT;

    PH_LOG_NDEF_FUNC_ENTRY();
    switch(NdefMap->DespOpFlag)
    {
        /* check which routine has the problem and set the CR*/
    case PH_FRINFC_NDEFMAP_DESF_NDEF_CHK_OP :
        /* set the completion routine*/
        NdefMap->CompletionRoutine[PH_FRINFC_NDEFMAP_CR_CHK_NDEF].\
            CompletionRoutine(NdefMap->CompletionRoutine->Context,\
            Status);
        break;

    case PH_FRINFC_NDEFMAP_DESF_READ_OP :
        /* set the completion routine*/
        NdefMap->CompletionRoutine[PH_FRINFC_NDEFMAP_CR_RD_NDEF].\
            CompletionRoutine(NdefMap->CompletionRoutine->Context,\
            Status);
        break;

    case PH_FRINFC_NDEFMAP_DESF_WRITE_OP :
        /* set the completion routine*/
        NdefMap->CompletionRoutine[PH_FRINFC_NDEFMAP_CR_WR_NDEF].\
            CompletionRoutine(NdefMap->CompletionRoutine->Context,\
            Status);
        break;

    default :
        /* set the completion routine*/
        NdefMap->CompletionRoutine[PH_FRINFC_NDEFMAP_CR_INVALID_OPE].\
            CompletionRoutine(NdefMap->CompletionRoutine->Context,\
            Status);
        break;

    }
    PH_LOG_NDEF_FUNC_EXIT();
}


static
NFCSTATUS
phFriNfc_Desfire_HSendTransCmd(
    _Inout_ phFriNfc_NdefMap_t  *NdefMap,
    _In_    uint8_t             SendRecvLen
    )
{
    NFCSTATUS status =  NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    /* set the command type*/
    NdefMap->Cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;

    /* set the Additional Info*/
    NdefMap->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefMap->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /*set the completion routines for the desfire card operations*/
    NdefMap->MapCompletionInfo.CompletionRoutine = phFriNfc_Desfire_Process;
    NdefMap->MapCompletionInfo.Context = NdefMap;

    /* set the receive length */
    *NdefMap->SendRecvLength = ((uint16_t)(SendRecvLen));


    /*Call the Overlapped HAL Transceive function */
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
