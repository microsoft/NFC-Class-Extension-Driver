/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#include "phFriNfc_Pch.h"

#include "phFriNfc_NdefReg.tmh"

_Check_return_
int16_t FORCEINLINE
phFriNfc_NdefReg_Strncmp (
    _In_reads_or_z_(count) const int8_t *s1, 
    _In_reads_or_z_(count) const int8_t *s2, 
    _In_ uint32_t count)
{
    return (int16_t)strncmp((const char*)s1,(const char*)s2, count);
}

_Check_return_
int16_t  FORCEINLINE
phFriNfc_NdefReg_Strnicmp(
    _In_z_ const int8_t *s1, 
    _In_z_ const int8_t *s2, 
    _In_ uint32_t count)
{
    return (int16_t)_strnicmp((const char*)s1, (const char*)s2, count);
}

NFCSTATUS
phFriNfc_NdefReg_Reset(
    _Out_   phFriNfc_NdefReg_t          *NdefReg,
    _In_    uint8_t                     **NdefTypesarray,
    _In_    phFriNfc_NdefRecord_t       *RecordsExtracted,
    _In_    phFriNfc_NdefReg_CbParam_t  *CbParam,
    _In_    uint8_t                     *ChunkedRecordsarray,
    _In_    uint32_t                    NumberOfRecords
    )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    /* Reset/Initialise the values of the Context structure */
    /* Store the required pointers in the context structure */
    if(NdefReg == NULL || NdefTypesarray == NULL || RecordsExtracted == NULL || CbParam == NULL)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        NdefReg->State=PH_FRINFC_NDEFREG_STATE_INIT;
        NdefReg->NdefData=NULL;
        NdefReg->NdefDataLength=0;
        NdefReg->NdefTypeList=NULL;
        NdefReg->NdefTypes = NdefTypesarray ;
        NdefReg->RecordsExtracted = RecordsExtracted;
        NdefReg->CbParam = CbParam;
        NdefReg->validPreviousTnf = 0xff;
        NdefReg->IsChunked = ChunkedRecordsarray;
        NdefReg->NumberOfRecords = NumberOfRecords;
        NdefReg->newRecordextracted = 0;
        NdefReg->MainTnfFound = 0;
    }
    PH_LOG_NDEF_FUNC_EXIT();

#pragma prefast(suppress: __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION, "ESP:1220 PreFast Bug")
    return(status);
}

NFCSTATUS phFriNfc_NdefReg_AddCb(phFriNfc_NdefReg_t     *NdefReg,
                                 phFriNfc_NdefReg_Cb_t  *NdefCb)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(  NdefReg == NULL || NdefCb ==NULL)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /*  Check whether we can accomodate all these records in a single node */
        if(NdefCb->NumberOfRTDs <= PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED)
        {
            /*  Yes, all these Records can be accomodated in a single node */
            /* No existing node. This will be the first Node    */
            if(NdefReg->NdefTypeList==NULL)
            {
                NdefCb->Previous =NULL;
                NdefCb->Next =NULL;
                NdefReg->NdefTypeList = NdefCb;
            }
            else
            {
                /*  Some nodes are existing. Add the new CB node at the front
                    of the List and update pointers */
                NdefCb->Next = NdefReg->NdefTypeList;
                NdefCb->Previous = NULL;
                NdefReg->NdefTypeList->Previous = NdefCb;
                NdefReg->NdefTypeList = NdefReg->NdefTypeList->Previous;
            }
        }
        else
        {
            /*  We cannot accomodate more records than PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED
                in this node.
                So return warning NFCSTATUS_INVALID_PARAMETER. */
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY,
                                NFCSTATUS_INVALID_PARAMETER);
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
#pragma prefast(suppress: __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION, "ESP:1220 PreFast Bug")
    return(status);
}

NFCSTATUS phFriNfc_NdefReg_RmCb(phFriNfc_NdefReg_t    *NdefReg,
                                phFriNfc_NdefReg_Cb_t *NdefCb)
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phFriNfc_NdefReg_Cb_t   *tempNode;
    uint8_t                 found=0;/* to check for the node present or not */
    PH_LOG_NDEF_FUNC_ENTRY();
    if(  NdefReg==NULL || NdefCb ==NULL)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        tempNode = NdefReg->NdefTypeList;
        /* if the list is empty */
        if(NdefReg->NdefTypeList == NULL)
        {
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_NOT_REGISTERED);
        }
        else
        {
            while(tempNode!=NULL)
            {
                if(NdefCb == tempNode)
                {
                    found=1;/* if the node is present */

                    /* node is present in the front of the list so
                    update the NdefReg->NdefTypeList pointer */
                    if(tempNode->Previous==NULL && tempNode->Next!=NULL)
                    {
                        NdefReg->NdefTypeList =NdefReg->NdefTypeList->Next;
                        NdefReg->NdefTypeList->Previous = NULL;
                        break;
                    }
                    /* only one node in the list so update the head node */
                    if(tempNode->Next==NULL && tempNode->Previous==NULL)
                    {
                          NdefReg->NdefTypeList=NULL;
                          break;
                    }
                    if (tempNode->Previous != NULL)
                    {
                        tempNode->Previous->Next = tempNode->Next;
                    }
                    if (tempNode->Next != NULL)
                    {
                        tempNode->Next->Previous = tempNode->Previous;
                    }
                    break;
                }
                /* move to the next node */
                tempNode = tempNode->Next;
            }
        }
        if(!found )
        {
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_NOT_REGISTERED);
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
#pragma prefast(suppress: __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION, "ESP:1220 PreFast Bug")
    return(status);
}

NFCSTATUS phFriNfc_NdefReg_DispatchPacket(phFriNfc_NdefReg_t   *NdefReg,
                                          uint8_t              *PacketData,
                                          uint16_t              PacketDataLength)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(NdefReg==NULL ||PacketData==NULL || PacketDataLength==0)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        NdefReg->NdefData = PacketData;
        NdefReg->NdefDataLength = PacketDataLength;
        NdefReg->State = PH_FRINFC_NDEFREG_STATE_DIS_PKT;
        NdefReg->NumberOfNdefTypes = 0;
        NdefReg->RecordIndex = 0;
        NdefReg->RtdIndex = 0;
        status = PHNFCSTVAL(CID_NFC_NONE, NFCSTATUS_SUCCESS);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return(status);
}

NFCSTATUS phFriNfc_NdefReg_DispatchRecord(phFriNfc_NdefReg_t     *NdefReg,
                                          phFriNfc_NdefRecord_t  *RecordsExtracted)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(NdefReg==NULL || RecordsExtracted==NULL)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        NdefReg->RecordsExtracted = RecordsExtracted;
        NdefReg->State = PH_FRINFC_NDEFREG_STATE_DIS_RCD;

        status = PHNFCSTVAL(CID_NFC_NONE, NFCSTATUS_SUCCESS);
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return(status);
}

uint8_t phFriNfc_NdefReg_Process(phFriNfc_NdefReg_t  *NdefReg,
                                 NFCSTATUS           *Status)
{
    uint8_t     count = 0,
                ret_val=0,
                TnfMatchFound = 0,
                index = 0,
                matchFound = 0;
    PH_LOG_NDEF_FUNC_ENTRY();


    if(NdefReg->NdefTypeList == NULL)
    {
        /*  No Nodes added. Return error  */
        *Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_NOT_REGISTERED);
        ret_val = 1;
        goto Done;
    }

    *Status = NFCSTATUS_SUCCESS;
    switch(NdefReg->State)
    {
        case PH_FRINFC_NDEFREG_STATE_DIS_RCD:

        /* for START ( each node in the list NdefReg->NdefTypeList), do the following */
        for(index=0;index<NdefReg->NdefTypeList->NumberOfRTDs;index++)
        {
            /* First, Match should be done with The TNF*/
            if( (NdefReg->NdefTypeList->Tnf[index] & PH_FRINFC_NDEFRECORD_TNF_MASK ) ==
                (NdefReg->RecordsExtracted->Tnf & PH_FRINFC_NDEFRECORD_TNF_MASK ) )
            {
                /* Second, Match should be done with The Typelength*/
                if( NdefReg->NdefTypeList->NdeftypeLength[index] == \
                    NdefReg->RecordsExtracted->TypeLength )
                {
                    /* Third, Match should be done with The Type*/
                    matchFound = 0;
                    switch(NdefReg->NdefTypeList->Tnf[index])
                    {
                        case 1:
                            /*  TNF = 0X01 NFC Forum well-known type : Case sensitive  */
                            /*  comparison is to be made. */
                            if( !phFriNfc_NdefReg_Strncmp( (const int8_t *)NdefReg->NdefTypeList->NdefType[index] ,
                                (const int8_t *)(NdefReg->RecordsExtracted->Type),
                                NdefReg->RecordsExtracted->TypeLength))
                            {
                                matchFound = 1;
                            }
                        break;

                        case 2:
                        case 4:
                            /*  For TNF = 0X02 Media-type as defined in RFC 2046
                                From iana.org, it has never been found any registered media type
                                with non-ascii character ==> Comparison to be used for *types* in
                                *registry*: case INSENSITIVE comparison.

                                For TNF = 0X04 NFC Forum external type : ==> Comparison to be used
                                for *types* in *registry* : case INSENSITIVE comparison */
                            if( !phFriNfc_NdefReg_Strnicmp( (int8_t *)NdefReg->NdefTypeList->NdefType[index] ,
                                (int8_t *)(NdefReg->RecordsExtracted->Type),
                                NdefReg->RecordsExtracted->TypeLength))
                            {
                                matchFound = 1;
                            }
                        break;

                        case 3:
                            /*  TNF = 0X03 Absolute URI as defined in RFC 3986 [RFC 3986]
                                Big issue concerning the Encoding, which is actually determined
                                by the application.Conclusion is that: Comparison to be used for
                                *types* in *registry*: NONE. only TNF ; type should be ignored,
                                and its comparison shall be done by the application. */
                            matchFound = 1;
                        break;
                        default:
                            break;
                    }
                    if(matchFound)
                    {
                        if(count >= PH_FRINFC_NDEFREG_MAX_RTD)
                        {
                            PH_LOG_NDEF_CRIT_STR("Not enough space to store RTD in callback parameter!");
                            *Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_BUFFER_TOO_SMALL);
                            ret_val = 1;
                            goto Done;
                        }

                        /*  Copy the chunked flag info to the callback parameter */
                        NdefReg->CbParam->Chunked[count] = (NdefReg->RecordsExtracted->Flags & \
                                                            PH_FRINFC_NDEFRECORD_FLAGS_CF );

                        /*  NOTE: Raw record and the raw record size cannot be
                            copied as this itself is an extracted record ! */

                        /*  Copy the record in the format phFriNfc_NdefRecord_t
                            to the callback parameter */
                            phOsalNfc_MemCopy( &NdefReg->CbParam->Records[count],
                                                    NdefReg->RecordsExtracted,
                                                    sizeof(phFriNfc_NdefRecord_t));
                            count++;
                    }
                }
            }
        }

        /*  now, we have all the matching entries for the RTDs present
            in this particular node NdefReg->NdefTypeList.
            call the callback */
        if(count>0)
        {
            NdefReg->CbParam->Count = count;
            NdefReg->CbParam->CbContext = NdefReg->NdefTypeList->CbContext;
            NdefReg->NdefTypeList->NdefCallback (NdefReg->CbParam);
        }
        else
        {
            NdefReg->CbParam->Count = 0;
        }

        /* for END ( each node in the list NdefReg->NdefTypeList), do the following */
        if(NdefReg->NdefTypeList->Next==NULL)
        {
            NdefReg->State = PH_FRINFC_NDEFREG_STATE_INIT;
            while(NdefReg->NdefTypeList->Previous!=NULL)
            {
                NdefReg->NdefTypeList = NdefReg->NdefTypeList->Previous;
            }
            /* No further node present in the list. End of process function call */
            ret_val=1;
        }
        else
        {
            /* Move the linked list by one node.*/
            NdefReg->NdefTypeList = NdefReg->NdefTypeList->Next;
            /* list is not empty so come again */
            ret_val=0;
        }
        break;

        case PH_FRINFC_NDEFREG_STATE_DIS_PKT:

        if(NdefReg->NumberOfNdefTypes == 0)
        {
            /*  First, Get all the records present in the data packet NdefReg->NdefData
                using NDEF tool library */
            /*  and, get the NumberOfNdefTypes from NdefData */
            *Status = phFriNfc_NdefRecord_GetRecords(   NdefReg->NdefData,
                                                        NdefReg->NdefDataLength,
                                                        NULL,
                                                        NdefReg->IsChunked,
                                                        &NdefReg->NumberOfNdefTypes);
            if((*Status)!= NFCSTATUS_SUCCESS)
            {
                /* Error in the Packet. Exit from the process */
                ret_val = 1;
                NdefReg->NumberOfNdefTypes = 0;
                break;
            }

            if(NdefReg->NumberOfNdefTypes > NdefReg->NumberOfRecords)
            {
                /*  There is not enough space in the arrays NdefReg->NdefTypes
                    and NdefReg->IsChunked, to write NdefReg->NumberOfRecords
                    number of records. Return error. */
                *Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_BUFFER_TOO_SMALL);
                ret_val = 1;
                NdefReg->NumberOfNdefTypes = 0;
                break;
            }

            /* Now, get the NdefTypes */
            *Status = phFriNfc_NdefRecord_GetRecords(   NdefReg->NdefData,
                                                        NdefReg->NdefDataLength,
                                                        NdefReg->NdefTypes,
                                                        NdefReg->IsChunked,
                                                        &NdefReg->NumberOfNdefTypes);
            if((*Status)!= NFCSTATUS_SUCCESS)
            {
                /* Error in the Packet. Exit from the process */
                ret_val = 1;
                NdefReg->NumberOfNdefTypes = 0;
                break;
            }
        }

        /*  For each record, in the arry NdefReg->NdefTypes, check
            whether it matches with any of the registered RTD in the
            present node. If it matches, copy this to the Callback
            parameter array and increment count.
            If count is 8, call the callback function.
            Remember the record index that we last processed, before
            calling the callback.   */

        /*  for each record in the Message  */
        while(NdefReg->RecordIndex < NdefReg->NumberOfNdefTypes)
        {
            /*  Extract a record from the Message */
            *Status = phFriNfc_NdefRecord_Parse(NdefReg->RecordsExtracted,
                                                NdefReg->NdefTypes[NdefReg->RecordIndex]);
            if((*Status)!= NFCSTATUS_SUCCESS)
            {
                /* Error in the Record. Exit from the process */
                NdefReg->NumberOfNdefTypes = 0;
                ret_val = 1;
                goto Done;
            }

            NdefReg->newRecordextracted = 1;

            /*  for each RTD in the present node */
            while(NdefReg->RtdIndex < NdefReg->NdefTypeList->NumberOfRTDs)
            {
                /*  Harsha: Fix for 0000243: [JF] phFriNfc_NdefReg_Process
                    won't parse correctly chunked records   */

                /*  Check whether the TNF = 0x06
                    PH_FRINFC_NDEFRECORD_TNF_UNCHANGED */

                if((NdefReg->RecordsExtracted->Tnf & PH_FRINFC_NDEFRECORD_TNF_MASK ) ==
                    PH_FRINFC_NDEFRECORD_TNF_UNCHANGED)
                {
                    if(NdefReg->MainTnfFound == 1)
                    {
                        /*  Matching RTD is there  */
                        TnfMatchFound = 1;
                    }
                }
                else
                {
                    if(NdefReg->newRecordextracted)
                    {
                        NdefReg->MainTnfFound  = 0;
                    }
                    /*  This is a tnf other than 0x06.
                        that means, this is a valid tnf */
                    NdefReg->validPreviousTnf = NdefReg->RecordsExtracted->Tnf;

                    /* First, Match should be done with The TNF*/
                    /* Second, Match should be done with The Typelength*/
                        /* Third, Match should be done with The Type*/
                    if((NdefReg->NdefTypeList->Tnf[NdefReg->RtdIndex] &
                        PH_FRINFC_NDEFRECORD_TNF_MASK ) ==
                        (NdefReg->RecordsExtracted->Tnf & PH_FRINFC_NDEFRECORD_TNF_MASK ) &&
                        (NdefReg->NdefTypeList->NdeftypeLength[NdefReg->RtdIndex] ==
                        NdefReg->RecordsExtracted->TypeLength))
                    {
                        matchFound = 0;
                        switch(NdefReg->NdefTypeList->Tnf[NdefReg->RtdIndex])
                        {
                            case 1:
                                /*  TNF = 0X01 NFC Forum well-known type : Case sensitive
                                    comparison is to be made. */
                                if(!phFriNfc_NdefReg_Strncmp((const int8_t *)NdefReg->NdefTypeList->NdefType[NdefReg->RtdIndex],
                                            (const int8_t *)(NdefReg->RecordsExtracted->Type),

                                            NdefReg->RecordsExtracted->TypeLength))
                                {
                                    matchFound = 1;
                                }
                            break;

                            case 2:
                            case 4:
                                /*  For TNF = 0X02 Media-type as defined in RFC 2046
                                    From iana.org, it has never been found any registered media type
                                    with non-ascii character ==> Comparison to be used for *types* in
                                    *registry*: case INSENSITIVE comparison. */

                                /*  For TNF = 0X04 NFC Forum external type : ==> Comparison to be used
                                    for *types* in *registry* : case INSENSITIVE comparison */
                                if( !phFriNfc_NdefReg_Strnicmp(  (int8_t *)NdefReg->NdefTypeList->NdefType[NdefReg->RtdIndex],
                                                (int8_t *)(NdefReg->RecordsExtracted->Type),
                                                NdefReg->RecordsExtracted->TypeLength))
                                {
                                    matchFound = 1;
                                }
                            break;

                            case 3:
                                /*  TNF = 0X03 Absolute URI as defined in RFC 3986 [RFC 3986]
                                    Big issue concerning the Encoding, which is actually determined
                                    by the application.Conclusion is that: Comparison to be used for
                                    *types* in *registry*: NONE. only TNF ; type should be ignored,
                                    and its comparison shall be done by the application. */
                                matchFound = 1;
                            break;
                            default:
                            break;
                        }
                        if(matchFound == 1)
                        {
                            TnfMatchFound = 1;
                            NdefReg->MainTnfFound  = 1;
                        }
                        else
                        {
                            TnfMatchFound = 0;
                        }
                    }
                    else
                    {
                        TnfMatchFound = 0;
                    }
                }

                /*  we have found a matching TNF  */
                if(TnfMatchFound == 1)
                {
                    /* Code below will break out of the loop if count >= PH_FRINFC_NDEFREG_MAX_RTD
                       after incrementing it.
                       TODO: Find out why PreFast cannot automatically deduce this. */

                    _Analysis_assume_(count < PH_FRINFC_NDEFREG_MAX_RTD);
                    /*  Copy the chunked flag info to the callback parameter */
                    NdefReg->CbParam->Chunked[count] = NdefReg->IsChunked[NdefReg->RecordIndex];

                    /*  Copy the raw record to the callback parameter */
                    NdefReg->CbParam->RawRecord[count] = NdefReg->NdefTypes[NdefReg->RecordIndex];

                    /*  Copy the raw record size to the callback parameter */
                    NdefReg->CbParam->RawRecordSize[count] = phFriNfc_NdefRecord_GetLength(NdefReg->RecordsExtracted);

                    /*  Copy the record in the format phFriNfc_NdefRecord_t
                        to the callback parameter */
                    phOsalNfc_MemCopy( &NdefReg->CbParam->Records[count],
                                            NdefReg->RecordsExtracted,
                                            sizeof(phFriNfc_NdefRecord_t));


                    /*if count is greater than PH_FRINFC_NDEFREG_MAX_RTD (presently 8)
                    then call the callback since there can be more than
                    PH_FRINFC_NDEFREG_MAX_RTD of a single type with in the packet,as a result
                    call the callback and initialize count to zero.This may also happen
                    when there are chunked records */

                    count++;
                    if(count >= PH_FRINFC_NDEFREG_MAX_RTD )
                    {
                        NdefReg->CbParam->Count = count;
                        NdefReg->CbParam->CbContext = NdefReg->NdefTypeList->CbContext;
                        NdefReg->NdefTypeList->NdefCallback (NdefReg->CbParam);
                        count=0;
                        /*  Only one callback per node. */
                        NdefReg->RecordIndex++;
                        ret_val = 0;
                        goto Done;
                    }
                }

                /*  If the match is already found for the UNCHANGED_TNF,
                    don't comapre it again */
                if((NdefReg->RecordsExtracted->Tnf & PH_FRINFC_NDEFRECORD_TNF_MASK )
                    == PH_FRINFC_NDEFRECORD_TNF_UNCHANGED &&
                    TnfMatchFound == 1)
                {
                    TnfMatchFound = 0;
                    break;
                }

                NdefReg->RtdIndex++;

                NdefReg->newRecordextracted = 0;
            }   /*  for each RTD in the present node
                    while(NdefReg->RtdIndex < NdefReg->NdefTypeList->NumberOfRTDs) */

            /*   One Record processing done for this node */

            NdefReg->RtdIndex = 0;
            NdefReg->RecordIndex++;
        }   /*  while(NdefReg->RecordIndex<NdefReg->NumberOfNdefTypes)  */

        /*  One node has been checked. Move to the next node  */

        if(count>0)
        {
            NdefReg->CbParam->Count = count;
            NdefReg->CbParam->CbContext = NdefReg->NdefTypeList->CbContext;
            NdefReg->NdefTypeList->NdefCallback (NdefReg->CbParam);
        }
        else
        {
            NdefReg->CbParam->Count = 0;
        }

        if(NdefReg->NdefTypeList->Next==NULL)
        {
            /*  All the nodes have been checked. Return */
            NdefReg->State = PH_FRINFC_NDEFREG_STATE_INIT;
            while(NdefReg->NdefTypeList->Previous!=NULL)
            {
                NdefReg->NdefTypeList = NdefReg->NdefTypeList->Previous;
            }
            /*  No further node present in the list.
                End of Dispatch packet function. Return TRUE*/
            ret_val = 1;
        }
        else
        {
            /* Move the linked list by one node.*/
            NdefReg->NdefTypeList = NdefReg->NdefTypeList->Next;
            /* list is not empty so come again */
            ret_val = 0;
        }

        /*  Start from the first record again. */
        NdefReg->RecordIndex = 0;
        NdefReg->RtdIndex = 0;

        break;

        default:
            /* return error */
            *Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_REGISTRY, NFCSTATUS_INVALID_DEVICE_REQUEST);
            /* Unknown state Error exit from the process function */
            ret_val= 1;
        break;

    }

Done:

    PH_LOG_NDEF_FUNC_EXIT();
#pragma prefast(suppress: __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION, "ESP:1220 PreFast Bug")
    return(ret_val);
}
