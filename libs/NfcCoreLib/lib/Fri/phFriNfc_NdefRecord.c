/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

static uint8_t phFriNfc_NdefRecord_NdefFlag(uint8_t Flags,uint8_t Mask);
static uint8_t phFriNfc_NdefRecord_RecordFlag ( uint8_t* pRecord);
static uint8_t phFriNfc_NdefRecord_TypeNameFormat ( uint8_t* pRecord);

static NFCSTATUS phFriNfc_NdefRecord_RecordIDCheck (uint8_t*       pRecord,
                                                    uint8_t*       pTypeLength,
                                                    uint8_t*       pTypeLengthByte,
                                                    uint8_t*       pPayloadLengthByte,
                                                    uint32_t*       pPayloadLength,
                                                    uint8_t*       pIDLengthByte,
                                                    uint8_t*   pIDLength
                                                   );


 NFCSTATUS phFriNfc_NdefRecord_GetRecords(  uint8_t     *Buffer,
                                            uint32_t    BufferLength,
                                            uint8_t     *RawRecords[],
                                            uint8_t     IsChunked[],
                                            uint32_t    *NumberOfRawRecords)
{
    NFCSTATUS   Status = NFCSTATUS_SUCCESS;
    uint8_t     PayloadLengthByte = 0,
                TypeLengthByte = 0,
                TypeLength = 0,
                IDLengthByte = 0,
                NoOfRecordsReturnFlag = 0,
                IDLength = 0;
    uint32_t    Count = 0,
                PayloadLength = 0,
                BytesTraversed = 0;

    /*  Validate the input parameters */
    if (Buffer == NULL || BufferLength == 0 || NumberOfRawRecords == NULL)
    {
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_PARAMETER);
        return Status;
    }

    if((*NumberOfRawRecords) > 0)
    {
        /*  The number of caller-provided array positions for the array IsChunked
            has to be the same as NumberOfRawRecords. Hence,
            if NumberOfRawRecords > 0, the array IsChunked cannot be null */
        if(IsChunked == NULL)
        {
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                NFCSTATUS_INVALID_PARAMETER);
            return Status;
        }
    }

    /* Check Raw Records input is NULL and Number of Raw records is 0*/
    if ( RawRecords == NULL || *NumberOfRawRecords == 0)
    {
        /*  This flag is set, to return only number of records
            this is done when the Raw Records is NULL or
            Number of Raw records is 0 */
        NoOfRecordsReturnFlag = 1;
    }

    /* Check for the MB bit*/
    if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_MB) !=
            PH_FRINFC_NDEFRECORD_FLAGS_MB )
    {
        /* MB  Error */
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);

        /*  Number of valid records found in the message is 0 */
        *NumberOfRawRecords = 0;
        return Status;
    }

    /* Check for Tnf bits 0x07 is reserved for future use */
    if ((*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) ==
        PH_FRINFC_NDEFRECORD_TNF_RESERVED)
    {
        /* TNF 07  Error */
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);
        /*  Number of valid records found in the message is 0 */
        *NumberOfRawRecords = 0;
        return Status;
    }

    /* Check the First Record(MB = 0) for TNF = 0x06(Unchanged) */
    if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_MB) == PH_FRINFC_NDEFRECORD_FLAGS_MB &&
        (*Buffer & PH_FRINFC_NDEFRECORD_TNF_UNCHANGED) == PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
    {
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);
        /*  Number of valid records found in the message is 0 */
        *NumberOfRawRecords = 0;
        return Status;
    }

    /* First Record i.e., MB = 1, TNF != 0x05 and TypeLength = 0 */
    if ( (*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_MB) == PH_FRINFC_NDEFRECORD_FLAGS_MB &&
         (*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) != PH_FRINFC_NDEFRECORD_TNF_UNKNOWN &&
         (*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) != PH_FRINFC_NDEFRECORD_TNF_EMPTY &&
         *(Buffer + 1) == 0)
    {
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);
        /*  Number of valid records found in the message is 0  */
        *NumberOfRawRecords = 0;
        return Status;
    }

    /* Check till Buffer Length exceeds */
    while ( BytesTraversed < BufferLength )
    {
        if (Buffer == NULL)
        {
            break;
        }

        /* For Each Record Check whether it contains the ME bit set and CF bit Set
            if YES return ERROR*/
        if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_CF) ==
                    PH_FRINFC_NDEFRECORD_FLAGS_CF  &&
            (*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_ME) ==
             PH_FRINFC_NDEFRECORD_FLAGS_ME)
        {
            /* CF and ME Error */
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                    NFCSTATUS_INVALID_FORMAT);
            break;
        }

        if (NoOfRecordsReturnFlag == 0)
        {
            /*  Harsha: Fix for 0000241: [gk], NDEF Tools: GetRecords() overshoots
                a given array boundary if the number of records != 0. */
            /*  Actual Number of Records should not exceed Number of records
                required by caller*/
            if(Count >= *NumberOfRawRecords)
            {
                break;
            }
            /* To fix the mantis entry 0388 */
            if((Buffer != NULL)&&(RawRecords!=NULL))/*QMOR FIX*/
            {
                RawRecords[Count] = Buffer;
            }
            else
            {
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                    NFCSTATUS_INVALID_PARAMETER);
                break;
            }
        }

        /* To Calculate the IDLength and PayloadLength for
            short or normal record */
        Status = phFriNfc_NdefRecord_RecordIDCheck (    Buffer,
                                                        &TypeLength,
                                                        &TypeLengthByte,
                                                        &PayloadLengthByte,
                                                        &PayloadLength,
                                                        &IDLengthByte,
                                                        &IDLength);
        if (Status != NFCSTATUS_SUCCESS)
        {
            break;
        }

        /* Check for the Chunk Flag */
        if (NoOfRecordsReturnFlag == 0)
        {
            /*  If NoOfRecordsReturnFlag = 0, that means we have enough space  */
            /*  in the array IsChunked, to write  */
            if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_CF) ==
                PH_FRINFC_NDEFRECORD_FLAGS_CF)
            {
                IsChunked [Count] = PHFRINFCNDEFRECORD_CHUNKBIT_SET;
            }
            else
            {
                IsChunked [Count] = PHFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO;
            }
        }

        /* Check the record is not the first record */
        if (Count > 0)
        {
            /* Not a first record, if chunk record is present and IL bit is set
                also if the MB bit is set */
            if(((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_CF) == PH_FRINFC_NDEFRECORD_FLAGS_CF &&
                (*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_IL) == PH_FRINFC_NDEFRECORD_FLAGS_IL &&
                (*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) == PH_FRINFC_NDEFRECORD_TNF_UNCHANGED) ||
                (*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_MB) == PH_FRINFC_NDEFRECORD_FLAGS_MB)
            {
                /* IL or MB Error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                    NFCSTATUS_INVALID_FORMAT);
                break;
            }

            /* Check for the Chunk Flag */
            if (NoOfRecordsReturnFlag == 0)
            {
                /*  If NoOfRecordsReturnFlag = 0, that means the array IsChunked
                    contains valid values. So, cannot check the value
                    of IsChunked if NoOfRecordsReturnFlag = 1.  */

                /*  Check whether the previous record has the chunk flag and
                    TNF of present record is not 0x06 */
                if (IsChunked [Count - 1] == PHFRINFCNDEFRECORD_CHUNKBIT_SET &&
                    (*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) !=
                    PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
                {
                    /* CF or TNF  Error */
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                        NFCSTATUS_INVALID_FORMAT);
                    break;
                }

                /*  Check whether the previous record doesnot have the chunk flag and
                    TNF of present record is 0x06 */
                if (IsChunked [Count - 1] == PHFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO &&
                    (*Buffer & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) ==
                    PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
                {
                    /* CF or TNF  Error */
                    Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                        NFCSTATUS_INVALID_FORMAT);
                    break;
                }

                /* Check for the last chunk */
                if (IsChunked [Count - 1] == PHFRINFCNDEFRECORD_CHUNKBIT_SET &&
                    IsChunked [Count] == PHFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO)
                {
                    /* Check for the TypeLength, IDLength = 0 */
                    if (TypeLength != 0 || IDLength != 0)
                    {
                        /* last chunk record Error */
                        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                            NFCSTATUS_INVALID_FORMAT);
                        break;
                    }
                }
            }   /*  if (NoOfRecordsReturnFlag == 0)  */
        }   /*  if (Count > 0)  */

        /*  Calculate the bytes already traversed. */
        BytesTraversed = (BytesTraversed + PayloadLengthByte + IDLengthByte + TypeLength
                         + IDLength + TypeLengthByte + PayloadLength
                         + PH_FRINFC_NDEFRECORD_BUF_INC1);

        if(BytesTraversed == BufferLength)
        {
            /*  We have reached the last record, and everything is fine.  */
            /*  Check for the ME Byte */
            if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_ME) ==
                PH_FRINFC_NDEFRECORD_FLAGS_ME)
            {
                Count++;
                break;
            }
            else
            {
                /* Each message must have ME flag in the last record, Since
                ME is not set raise an error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                NFCSTATUS_INVALID_FORMAT);
                break;
            }
        }
       else
        {
            /* Buffer Overshoot: Inconsistency in the message length
              and actual value of the bytes in the message detected.
              Report error.*/
            if(BytesTraversed > BufferLength)
            {
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                NFCSTATUS_INVALID_FORMAT);
                break;
            }
        }
        /*  For Each Record Check whether it contains the ME bit set
            if YES return*/
        if ((*Buffer & PH_FRINFC_NDEFRECORD_FLAGS_ME) ==
            PH_FRINFC_NDEFRECORD_FLAGS_ME)
        {
            Count++;
            break;
        }

        /* +1 is for first byte */
        Buffer = (Buffer + PayloadLengthByte + IDLengthByte + TypeLength
                 + TypeLengthByte + IDLength + PayloadLength
                 + PH_FRINFC_NDEFRECORD_BUF_INC1);

        /*  Increment the number of valid records found in the message  */
        Count++;
    }

    /*  Whatever is the error, update the NumberOfRawRecords with the number
        of proper records found till the error was detected in the message. */
    *NumberOfRawRecords = Count;
    return Status;
}

static uint8_t phFriNfc_NdefRecord_NdefFlag(uint8_t Flags,uint8_t Mask)
{
    uint8_t check_flag = 0x00;
    check_flag = Flags & Mask;
    return check_flag;
}

uint32_t phFriNfc_NdefRecord_GetLength(phFriNfc_NdefRecord_t *Record)
{
    uint32_t RecordLength=1;
    uint8_t  FlagCheck=0;

    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Tnf,PH_FRINFC_NDEFRECORD_TNFBYTE_MASK);
    /* Type length is present only for following TNF
                    PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN
                    PH_FRINFC_NDEFRECORD_TNF_MEDIATYPE
                    PH_FRINFC_NDEFRECORD_TNF_ABSURI
                    PH_FRINFC_NDEFRECORD_TNF_NFCEXT
    */

    /* ++ is for the Type Length Byte */
    RecordLength++;
    if( FlagCheck != PH_FRINFC_NDEFRECORD_TNF_EMPTY &&
        FlagCheck != PH_FRINFC_NDEFRECORD_TNF_UNKNOWN &&
        FlagCheck != PH_FRINFC_NDEFRECORD_TNF_UNCHANGED )
    {
        RecordLength += Record->TypeLength;
    }

    /* to check if payloadlength is 8bit or 32bit*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_SR);
    if(FlagCheck!=0)
    {
        /* ++ is for the Payload Length Byte */
        RecordLength++;/* for short record*/
    }
    else
    {
        /* + PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE is for the Payload Length Byte */
        RecordLength += PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;/* for normal record*/
    }

    /* for non empty record */
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Tnf,PH_FRINFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck != PH_FRINFC_NDEFRECORD_TNF_EMPTY)
    {
        RecordLength += Record->PayloadLength;
    }

    /* ID and IDlength are present only if IL flag is set*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_IL);
    if(FlagCheck!=0)
    {
        RecordLength +=Record->IdLength;
        /* ++ is for the ID Length Byte */
        RecordLength ++;
    }
    return RecordLength;
}

/*!
 *
 *  Extract a specific NDEF record from the data, provided by the caller. The data is a buffer holding
 *  at least the entire NDEF record (received via the NFC link, for example).
 *
 * \param[out] Record               The NDEF record structure. The storage for the structure has to be provided by the
 *                                  caller matching the requirements for \b Extraction, as described in the compound
 *                                  documentation.
 * \param[in]  RawRecord            The Pointer to the buffer, selected out of the array returned by
 *                                  the \ref phFriNfc_NdefRecord_GetRecords function.
 *
 * \retval NFCSTATUS_SUCCESS                Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function is invalid.
 *
 * \note There are some caveats:
 *       - The "RawRecord" Data buffer must exist at least as long as the function execution time plus the time
 *         needed by the caller to evaluate the extracted information. No copying of the contained data is done.
 *       - Using the "RawRecord" and "RawRecordMaxSize" parameters the function internally checks whether the
 *         data to extract are within the bounds of the buffer.
 *
 *
 */
NFCSTATUS phFriNfc_NdefRecord_Parse(phFriNfc_NdefRecord_t *Record,
                                    uint8_t               *RawRecord)
{
    NFCSTATUS       Status = NFCSTATUS_SUCCESS;
    uint8_t         PayloadLengthByte = 0,
                    TypeLengthByte = 0,
                    TypeLength = 0,
                    IDLengthByte = 0,
                    IDLength = 0,
                    Tnf     =   0;
    uint32_t        PayloadLength = 0;

    if (Record == NULL || RawRecord == NULL)
    {
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_PARAMETER);
    }

    else
    {

        /* Calculate the Flag Value */
        Record->Flags = phFriNfc_NdefRecord_RecordFlag ( RawRecord);

        /* Calculate the Type Namr format of the record */
        Tnf = phFriNfc_NdefRecord_TypeNameFormat( RawRecord);
        if(Tnf != 0xFF)
        {
            Record->Tnf = Tnf;
            /* To Calculate the IDLength and PayloadLength for short or normal record */
            Status = phFriNfc_NdefRecord_RecordIDCheck (    RawRecord,
                                                            &TypeLength,
                                                            &TypeLengthByte,
                                                            &PayloadLengthByte,
                                                            &PayloadLength,
                                                            &IDLengthByte,
                                                            &IDLength);
            Record->TypeLength = TypeLength;
            Record->PayloadLength = PayloadLength;
            Record->IdLength = IDLength;
            RawRecord = (RawRecord +  PayloadLengthByte + IDLengthByte + TypeLengthByte + PH_FRINFC_NDEFRECORD_BUF_INC1);
            Record->Type = RawRecord;

            RawRecord = (RawRecord + Record->TypeLength);

            if (Record->IdLength != 0)
            {
                Record->Id = RawRecord;
            }

            RawRecord = RawRecord + Record->IdLength;
            Record->PayloadData = RawRecord;
        }
        else
        {
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                NFCSTATUS_INVALID_PARAMETER);
        }
    }
    return Status;
}

 NFCSTATUS phFriNfc_NdefRecord_Generate(phFriNfc_NdefRecord_t *Record,
                                        uint8_t               *Buffer,
                                        uint32_t               MaxBufferSize,
                                        uint32_t              *BytesWritten)
{
    uint8_t     FlagCheck,
                TypeCheck=0,
                *temp,
                i;
    uint32_t    i_data=0;

    if(Record==NULL ||Buffer==NULL||BytesWritten==NULL||MaxBufferSize == 0)
    {
        return (PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD, NFCSTATUS_INVALID_PARAMETER));
    }

    if (Record->Tnf == PH_FRINFC_NDEFRECORD_TNF_RESERVED)
    {
        return (PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD, NFCSTATUS_INVALID_FORMAT));
    }

     /* calculate the length of the record and check with the buffersize if it exceeds return */
    i_data=phFriNfc_NdefRecord_GetLength(Record);
    if(i_data > MaxBufferSize)
    {
        return (PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD, NFCSTATUS_BUFFER_TOO_SMALL));
    }
    *BytesWritten = i_data;

    /*fill the first byte of the message(all the flags) */
    /*increment the buffer*/
    *Buffer = ( (Record->Flags & PH_FRINFC_NDEFRECORD_FLAG_MASK) | (Record->Tnf & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK));
    Buffer++;

    /* check the TypeNameFlag for PH_FRINFC_NDEFRECORD_TNF_EMPTY */
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Tnf,PH_FRINFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == PH_FRINFC_NDEFRECORD_TNF_EMPTY)
    {
        /* fill the typelength idlength and payloadlength with zero(empty message)*/
        for(i=0;i<3;i++)
        {
            *Buffer=PH_FRINFC_NDEFRECORD_BUF_TNF_VALUE;
            Buffer++;
        }
        return (PHNFCSTVAL(CID_NFC_NONE, NFCSTATUS_SUCCESS));
     }

    /* check the TypeNameFlag for PH_FRINFC_NDEFRECORD_TNF_RESERVED */
    /* TNF should not be reserved one*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Tnf,PH_FRINFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == PH_FRINFC_NDEFRECORD_TNF_RESERVED)
    {
        return (PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD, NFCSTATUS_INVALID_PARAMETER));
    }

    /* check for TNF Unknown or Unchanged */
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Tnf,PH_FRINFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == PH_FRINFC_NDEFRECORD_TNF_UNKNOWN || \
        FlagCheck == PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
    {
        *Buffer = PH_FRINFC_NDEFRECORD_BUF_TNF_VALUE;
        Buffer++;
    }
    else
    {
        *Buffer = Record->TypeLength;
        Buffer++;
        TypeCheck=1;
    }

    /* check for the short record bit if it is then payloadlength is only one byte */
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_SR);
    if(FlagCheck!=0)
    {
        *Buffer = (uint8_t)(Record->PayloadLength & 0x000000ff);
        Buffer++;
    }
    else
    {
        /* if it is normal record payloadlength is 4 byte(32 bit)*/
        *Buffer = (uint8_t)((Record->PayloadLength & 0xff000000) >> PHNFCSTSHL24);
        Buffer++;
        *Buffer = (uint8_t)((Record->PayloadLength & 0x00ff0000) >> PHNFCSTSHL16);
        Buffer++;
        *Buffer = (uint8_t)((Record->PayloadLength & 0x0000ff00) >> PHNFCSTSHL8);
        Buffer++;
        *Buffer = (uint8_t)((Record->PayloadLength & 0x000000ff));
        Buffer++;
    }

    /*check for IL bit set(Flag), if so then IDlength is present*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_IL);
    if(FlagCheck!=0)
    {
        *Buffer=Record->IdLength;
        Buffer++;
    }

    /*check for TNF and fill the Type*/
    temp=Record->Type;
    if(TypeCheck!=0)
    {
        for(i=0;i<(Record->TypeLength);i++)
        {
            *Buffer = *temp;
            Buffer++;
            temp++;
        }
    }

    /*check for IL bit set(Flag), if so then IDlength is present and fill the ID*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_IL);
    temp=Record->Id;
    if(FlagCheck!=0)
    {
        for(i=0;i<(Record->IdLength);i++)
        {
            *Buffer = *temp;
            Buffer++;
            temp++;
        }
    }

    temp=Record->PayloadData;
    /*check for SR bit and then correspondingly use the payload length*/
    FlagCheck=phFriNfc_NdefRecord_NdefFlag(Record->Flags,PH_FRINFC_NDEFRECORD_FLAGS_SR);
    for(i_data=0;i_data < (Record->PayloadLength) ;i_data++)
    {
        *Buffer = *temp;
        Buffer++;
        temp++;
    }

    return (PHNFCSTVAL(CID_NFC_NONE, NFCSTATUS_SUCCESS));
}

static uint8_t phFriNfc_NdefRecord_RecordFlag ( uint8_t    *Record)
{
    uint8_t flag = 0;

    if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_MB) == PH_FRINFC_NDEFRECORD_FLAGS_MB )
    {
        flag = flag | PH_FRINFC_NDEFRECORD_FLAGS_MB;
    }
    if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_ME) == PH_FRINFC_NDEFRECORD_FLAGS_ME )
    {
        flag = flag | PH_FRINFC_NDEFRECORD_FLAGS_ME;
    }
    if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_CF) == PH_FRINFC_NDEFRECORD_FLAGS_CF )
    {
        flag = flag | PH_FRINFC_NDEFRECORD_FLAGS_CF;
    }
    if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_SR) == PH_FRINFC_NDEFRECORD_FLAGS_SR )
    {
        flag = flag | PH_FRINFC_NDEFRECORD_FLAGS_SR;
    }
    if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_IL) == PH_FRINFC_NDEFRECORD_FLAGS_IL )
    {
        flag = flag | PH_FRINFC_NDEFRECORD_FLAGS_IL;
    }
    return flag;
}

static uint8_t phFriNfc_NdefRecord_TypeNameFormat ( uint8_t    *Record)
{
    uint8_t     tnf = 0;

    switch (*Record & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK)
    {
    case PH_FRINFC_NDEFRECORD_TNF_EMPTY:
        tnf = PH_FRINFC_NDEFRECORD_TNF_EMPTY;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN:
        tnf = PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_MEDIATYPE:
        tnf = PH_FRINFC_NDEFRECORD_TNF_MEDIATYPE;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_ABSURI:
        tnf = PH_FRINFC_NDEFRECORD_TNF_ABSURI;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_NFCEXT:
        tnf = PH_FRINFC_NDEFRECORD_TNF_NFCEXT;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_UNKNOWN:
        tnf = PH_FRINFC_NDEFRECORD_TNF_UNKNOWN;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_UNCHANGED:
        tnf = PH_FRINFC_NDEFRECORD_TNF_UNCHANGED;
        break;

    case PH_FRINFC_NDEFRECORD_TNF_RESERVED:
        tnf = PH_FRINFC_NDEFRECORD_TNF_RESERVED;
        break;
    default :
        tnf = 0xFF;
        break;
    }

    return tnf;
}


static NFCSTATUS phFriNfc_NdefRecord_RecordIDCheck ( uint8_t       *Record,
                                              uint8_t       *TypeLength,
                                              uint8_t       *TypeLengthByte,
                                              uint8_t       *PayloadLengthByte,
                                              uint32_t      *PayloadLength,
                                              uint8_t       *IDLengthByte,
                                              uint8_t       *IDLength)
{
    NFCSTATUS   Status = NFCSTATUS_SUCCESS;

    /* Check for Tnf bits 0x07 is reserved for future use */
    if ((*Record & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) ==
        PH_FRINFC_NDEFRECORD_TNF_RESERVED)
    {
        /* TNF 07  Error */
        Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);
        return Status;
    }

    /* Check for Type Name Format  depending on the TNF,  Type Length value is set*/
    if ((*Record & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK)==
        PH_FRINFC_NDEFRECORD_TNF_EMPTY)
    {
        *TypeLength = *(Record + PH_FRINFC_NDEFRECORD_BUF_INC1);

        if (*(Record + PH_FRINFC_NDEFRECORD_BUF_INC1) != 0)
        {
            /* Type Length  Error */
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                NFCSTATUS_INVALID_FORMAT);
            return Status;
        }

        *TypeLengthByte = 1;

        /* Check for Short Record */
        if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_SR) == PH_FRINFC_NDEFRECORD_FLAGS_SR)
        {
            /* For Short Record, Payload Length Byte is 1 */
            *PayloadLengthByte = 1;
            /*  1 for Header byte */
            *PayloadLength = *(Record + *TypeLengthByte + 1);
            if (*PayloadLength != 0)
            {
                /* PayloadLength  Error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                    NFCSTATUS_INVALID_FORMAT);
                return Status;
            }
        }
        else
        {
            /* For Normal Record, Payload Length Byte is 4 */
            *PayloadLengthByte = PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;
            *PayloadLength =    ((((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC2))) << PHNFCSTSHL24) +
                                (((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC3))) << PHNFCSTSHL16) +
                                (((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC4))) << PHNFCSTSHL8)  +
                                             *(Record + PH_FRINFC_NDEFRECORD_BUF_INC5));
            if (*PayloadLength != 0)
            {
                /* PayloadLength  Error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                    NFCSTATUS_INVALID_FORMAT);
                return Status;
            }
        }

        /* Check for ID Length existence */
        if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_IL) == PH_FRINFC_NDEFRECORD_FLAGS_IL)
        {
            /* Length Byte exists and it is 1 byte */
            *IDLengthByte = 1;
            /*  1 for Header byte */
            *IDLength = (uint8_t)*(Record + *PayloadLengthByte + *TypeLengthByte + PH_FRINFC_NDEFRECORD_BUF_INC1);
            if (*IDLength != 0)
            {
                /* IDLength  Error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                                    NFCSTATUS_INVALID_FORMAT);
                return Status;
            }
        }
        else
        {
            *IDLengthByte = 0;
            *IDLength = 0;
        }
    }
    else
    {
        if ((*Record & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK)== PH_FRINFC_NDEFRECORD_TNF_UNKNOWN
                || (*Record & PH_FRINFC_NDEFRECORD_TNFBYTE_MASK) ==
                    PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
        {
            if (*(Record + PH_FRINFC_NDEFRECORD_BUF_INC1) != 0)
            {
                /* Type Length  Error */
                Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_RECORD,
                            NFCSTATUS_INVALID_FORMAT);
                return Status;
            }
            *TypeLength = 0;
            *TypeLengthByte = 1;
        }
        else
        {
            /*  1 for Header byte */
            *TypeLength = *(Record + PH_FRINFC_NDEFRECORD_BUF_INC1);
            *TypeLengthByte = 1;
        }

        /* Check for Short Record */
        if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_SR) ==
                PH_FRINFC_NDEFRECORD_FLAGS_SR)
        {
            /* For Short Record, Payload Length Byte is 1 */
            *PayloadLengthByte = 1;
            /*  1 for Header byte */
            *PayloadLength = *(Record + *TypeLengthByte + PH_FRINFC_NDEFRECORD_BUF_INC1);
        }
        else
        {
            /* For Normal Record, Payload Length Byte is 4 */
            *PayloadLengthByte = PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE;
            *PayloadLength =    ((((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC2))) << PHNFCSTSHL24) +
                                (((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC3))) << PHNFCSTSHL16) +
                                (((uint32_t)(*(Record + PH_FRINFC_NDEFRECORD_BUF_INC4))) << PHNFCSTSHL8)  +
                                             *(Record + PH_FRINFC_NDEFRECORD_BUF_INC5));
        }

        /* Check for ID Length existence */
        if ((*Record & PH_FRINFC_NDEFRECORD_FLAGS_IL) ==
                PH_FRINFC_NDEFRECORD_FLAGS_IL)
        {
            *IDLengthByte = 1;
            /*  1 for Header byte */
            *IDLength = (uint8_t)*(Record + *PayloadLengthByte + *TypeLengthByte + PH_FRINFC_NDEFRECORD_BUF_INC1);
        }
        else
        {
            *IDLengthByte = 0;
            *IDLength = 0;
        }
    }
    return Status;
}
