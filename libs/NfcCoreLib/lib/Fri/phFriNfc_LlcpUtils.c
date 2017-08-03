/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include <phNfcHalTypes2.h>
#include <phFriNfc_LlcpUtils.h>
#include <phFriNfc_Llcp.h>

#include "phFriNfc_LlcpUtils.tmh"

NFCSTATUS 
phFriNfc_Llcp_DecodeTLV( 
    _In_ phNfc_sData_t  *psRawData,
    _In_ uint32_t       *pOffset,
    _Out_ uint8_t        *pType,
    _Out_ phNfc_sData_t  *psValueBuffer 
    )
{
   uint8_t type;
   uint8_t length;
   uint32_t offset ;

   if ((psRawData == NULL) || (pOffset == NULL) || (pType == NULL) || (psValueBuffer == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }
   *pType = 0;

    offset = *pOffset;

   if (offset > psRawData->length)
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Check if enough room for Type and Length (with overflow check) */
   if ((offset+2 > psRawData->length) && (offset+2 > offset))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Get Type and Length from current TLV, and update offset */
   type = psRawData->buffer[offset];
   length = psRawData->buffer[offset+1];
   offset += 2;

   /* Check if enough room for Value with announced Length (with overflow check) */
   if ((offset+length > psRawData->length) && (offset+length > offset))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Save response, and update offset */
   *pType = type;
   psValueBuffer->buffer = psRawData->buffer + offset;
   psValueBuffer->length = length;
   offset += length;

   /* Save updated offset */
   *pOffset = offset;

   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}


_Post_satisfies_(*pOffset < psValueBuffer->length)
NFCSTATUS
phFriNfc_Llcp_EncodeTLV(
    _In_                phNfc_sData_t   *psValueBuffer,
    _Inout_             uint32_t        *pOffset,
    _In_                uint8_t         type,
    _In_                uint8_t         length,
    _In_reads_(length)  uint8_t         *pValue
    )
{
   uint32_t offset ;
   uint32_t finalOffset ;
   uint8_t i;

   /* Check for NULL pointers */
   if ((psValueBuffer == NULL) || (pOffset == NULL) || (pValue == NULL))
   {
      PH_LOG_LLCP_FUNC_EXIT();
      return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
        offset = *pOffset;
        finalOffset = offset + 2 + length;
       /* Check offset */
       if (offset >= psValueBuffer->length)
       {
          PH_LOG_LLCP_FUNC_EXIT();
          return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
       }

       /* Check if enough room for Type, Length and Value (with overflow check) */
       if ((finalOffset >= psValueBuffer->length) || (finalOffset < offset))
       {
          PH_LOG_LLCP_FUNC_EXIT();
          return PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
       }

       /* Set the TYPE */
       psValueBuffer->buffer[offset] = type;
       offset += 1;

       /* Set the LENGTH */
       psValueBuffer->buffer[offset] = length;
       offset += 1;

       /* Set the VALUE */
       for(i=0;i<length;i++,offset++)
       {
          psValueBuffer->buffer[offset]  = pValue[i];
       }

       /* Save updated offset */
       *pOffset = offset;
   }
   PH_LOG_LLCP_FUNC_EXIT();
   return NFCSTATUS_SUCCESS;
}

NFCSTATUS phFriNfc_Llcp_AppendTLV( phNfc_sData_t  *psValueBuffer,
                                   uint32_t       nTlvOffset,
                                   uint32_t       *pCurrentOffset,
                                   uint8_t        length,
                                   uint8_t        *pValue)
{
   uint32_t offset ;
   uint32_t finalOffset;
   NFCSTATUS status;

   /* Check for NULL pointers */
   if ((psValueBuffer == NULL) || (pCurrentOffset == NULL) || (pValue == NULL))
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
      goto Done;
   }

    offset = *pCurrentOffset;
    finalOffset = offset + length;
    /* Check offset */
    if (offset >= psValueBuffer->length)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
        goto Done;
    }

    /* Check if enough room for Type and Length (with overflow check) */
    if ((finalOffset >= psValueBuffer->length) || (finalOffset < offset))
    {
        status = PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
        goto Done;
    }

    /* Validate nTlvOffset */
    if ((nTlvOffset + 1) >= psValueBuffer->length)
    {
        status = PHNFCSTVAL(CID_FRI_NFC_LLCP, NFCSTATUS_INVALID_PARAMETER);
        goto Done;
    }

    /* Update the LENGTH */
    psValueBuffer->buffer[nTlvOffset+1] += length;

    /* Set the VALUE */
    phOsalNfc_MemCopy(psValueBuffer->buffer + offset, pValue, length);
    offset += length;

    /* Save updated offset */
    *pCurrentOffset = offset;

    status = NFCSTATUS_SUCCESS;

Done:

   PH_LOG_LLCP_FUNC_EXIT();
   return status;
}

void phFriNfc_Llcp_EncodeRW(uint8_t *pRw)
{
   *pRw = *pRw & PHFRINFC_LLCP_TLV_RW_MASK;
}

void phFriNfc_Llcp_CyclicFifoInit(P_UTIL_FIFO_BUFFER   pUtilFifo,
                                  const uint8_t        *pBuffStart,
                                  uint32_t             buffLength)
{
   pUtilFifo->pBuffStart = (uint8_t *)pBuffStart;
   pUtilFifo->pBuffEnd   = (uint8_t *)pBuffStart + buffLength-1;
   pUtilFifo->pIn        = (uint8_t *)pBuffStart;
   pUtilFifo->pOut       = (uint8_t *)pBuffStart;
   pUtilFifo->bFull      = FALSE;
}

void phFriNfc_Llcp_CyclicFifoClear(P_UTIL_FIFO_BUFFER pUtilFifo)
{
   pUtilFifo->pIn = pUtilFifo->pBuffStart;
   pUtilFifo->pOut = pUtilFifo->pBuffStart;
   pUtilFifo->bFull      = FALSE;
}

uint32_t phFriNfc_Llcp_CyclicFifoWrite(P_UTIL_FIFO_BUFFER   pUtilFifo,
                                       uint8_t              *pData,
                                       uint32_t             dataLength)
{
   uint32_t dataLengthWritten = 0;
   uint8_t * pNext;

   while(dataLengthWritten < dataLength)
   {
      pNext = (uint8_t*)pUtilFifo->pIn+1;

      if(pNext > pUtilFifo->pBuffEnd)
      {
         //Wrap around
         pNext = pUtilFifo->pBuffStart;
      }

      if(pUtilFifo->bFull)
      {
         break;
      }

      if(pNext == pUtilFifo->pOut)
      {
         // Trigger Full flag
         pUtilFifo->bFull = TRUE;
      }

      dataLengthWritten++;
      *pNext = *pData++;
      pUtilFifo->pIn = pNext;
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return dataLengthWritten;
}

uint32_t
phFriNfc_Llcp_CyclicFifoFifoRead(
    _Inout_                             P_UTIL_FIFO_BUFFER  pUtilFifo,
    _Out_writes_to_(dataLength, return) uint8_t             *pBuffer,
    _In_                                uint32_t            dataLength
    )
{
   uint32_t  dataLengthRead = 0;
   uint8_t * pNext;

   while(dataLengthRead < dataLength)
   {
      if((pUtilFifo->pOut == pUtilFifo->pIn) && (pUtilFifo->bFull == FALSE))
      {
         //No more bytes in buffer
         break;
      }
      else
      {
         dataLengthRead++;

         if(pUtilFifo->pOut == pUtilFifo->pBuffEnd)
         {
            /* Wrap around */
            pNext = pUtilFifo->pBuffStart;
         }
         else
         {
            pNext = (uint8_t*)pUtilFifo->pOut + 1;
         }

         *pBuffer++ = *pNext;

         pUtilFifo->pOut = pNext;

         pUtilFifo->bFull = FALSE;
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return dataLengthRead;
}

uint32_t phFriNfc_Llcp_CyclicFifoUsage(P_UTIL_FIFO_BUFFER pUtilFifo)
{
   uint32_t dataLength;
   uint8_t * pIn        = (uint8_t *)pUtilFifo->pIn;
   uint8_t * pOut       = (uint8_t *)pUtilFifo->pOut;

   if (pUtilFifo->bFull)
   {
      dataLength = (uint32_t)(pUtilFifo->pBuffEnd - pUtilFifo->pBuffStart + 1);
   }
   else
   {
      if(pIn >= pOut)
      {
         dataLength = (uint32_t)(pIn - pOut);
      }
      else
      {
         dataLength = (uint32_t)(pUtilFifo->pBuffEnd - pOut);
         dataLength += (uint32_t)((pIn+1) - pUtilFifo->pBuffStart);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return dataLength;
}

uint32_t phFriNfc_Llcp_CyclicFifoAvailable(P_UTIL_FIFO_BUFFER pUtilFifo)
{
   uint32_t dataLength;
   uint32_t  size;
   uint8_t * pIn         = (uint8_t *)pUtilFifo->pIn;
   uint8_t * pOut        = (uint8_t *)pUtilFifo->pOut;

   if (pUtilFifo->bFull)
   {
      dataLength = 0;
   }
   else
   {
      if(pIn >= pOut)
      {
         size = (uint32_t)(pUtilFifo->pBuffEnd - pUtilFifo->pBuffStart + 1);
         dataLength = (uint32_t)(size - (pIn - pOut));
      }
      else
      {
         dataLength = (uint32_t)(pOut - pIn);
      }
   }

   PH_LOG_LLCP_FUNC_EXIT();
   return dataLength;
}



uint32_t phFriNfc_Llcp_Header2Buffer( phFriNfc_Llcp_sPacketHeader_t *psHeader, uint8_t *pBuffer, uint32_t nOffset )
{
   uint32_t nOriginalOffset = nOffset;
   pBuffer[nOffset++] = (uint8_t)((psHeader->dsap << 2)  | (psHeader->ptype >> 2));
   pBuffer[nOffset++] = (uint8_t)((psHeader->ptype << 6) | psHeader->ssap);
   PH_LOG_LLCP_FUNC_EXIT();
   return nOffset - nOriginalOffset;
}

uint32_t phFriNfc_Llcp_Sequence2Buffer( phFriNfc_Llcp_sPacketSequence_t *psSequence, uint8_t *pBuffer, uint32_t nOffset )
{
   uint32_t nOriginalOffset = nOffset;
   pBuffer[nOffset++] = (uint8_t)((psSequence->ns << 4) | (psSequence->nr));
   PH_LOG_LLCP_FUNC_EXIT();
   return nOffset - nOriginalOffset;
}

uint32_t phFriNfc_Llcp_Buffer2Header( uint8_t *pBuffer, uint32_t nOffset, phFriNfc_Llcp_sPacketHeader_t *psHeader )
{
    PH_LOG_LLCP_FUNC_ENTRY();
   psHeader->dsap  = (pBuffer[nOffset] & 0xFC) >> 2;
   psHeader->ptype = ((pBuffer[nOffset]  & 0x03) << 2) | ((pBuffer[nOffset+1] & 0xC0) >> 6);
   psHeader->ssap  = pBuffer[nOffset+1] & 0x3F;
   PH_LOG_LLCP_FUNC_EXIT();
   return PHFRINFC_LLCP_PACKET_HEADER_SIZE;
}

uint32_t phFriNfc_Llcp_Buffer2Sequence( uint8_t *pBuffer, uint32_t nOffset, phFriNfc_Llcp_sPacketSequence_t *psSequence )
{
   psSequence->ns = pBuffer[nOffset] >> 4;
   psSequence->nr = pBuffer[nOffset] & 0x0F;
   PH_LOG_LLCP_FUNC_EXIT();
   return PHFRINFC_LLCP_PACKET_SEQUENCE_SIZE;
}
