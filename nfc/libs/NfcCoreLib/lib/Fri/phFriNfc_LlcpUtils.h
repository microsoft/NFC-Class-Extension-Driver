/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

#include <phNfcHalTypes2.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc.h>
#include <phFriNfc_Llcp.h>

typedef struct UTIL_FIFO_BUFFER
{
   uint8_t          *pBuffStart;    /* Points to first valid location in buffer */
   uint8_t          *pBuffEnd;      /* Points to last valid location in buffer */
   volatile uint8_t *pIn;           /* Points to 1 before where the next TU1 will enter buffer */
   volatile uint8_t *pOut;          /* Points to 1 before where the next TU1 will leave buffer */
   volatile bool_t  bFull;         /* TRUE if buffer is full */
}UTIL_FIFO_BUFFER, *P_UTIL_FIFO_BUFFER;

NFCSTATUS 
phFriNfc_Llcp_DecodeTLV( 
    _In_ phNfc_sData_t  *psRawData,
    _In_ uint32_t       *pOffset,
    _Out_ uint8_t        *pType,
    _Out_ phNfc_sData_t  *psValueBuffer
    );

_Post_satisfies_(*pOffset < psValueBuffer->length)
NFCSTATUS
phFriNfc_Llcp_EncodeTLV(
    _In_                phNfc_sData_t   *psValueBuffer,
    _Inout_             uint32_t        *pOffset,
    _In_                uint8_t         type,
    _In_                uint8_t         length,
    _In_reads_(length)  uint8_t         *pValue
    );

NFCSTATUS phFriNfc_Llcp_AppendTLV( phNfc_sData_t  *psValueBuffer,
                                   uint32_t       nTlvOffset,
                                   uint32_t       *pCurrentOffset,
                                   uint8_t        length,
                                   uint8_t        *pValue);

void phFriNfc_Llcp_EncodeRW(uint8_t *pRw);


void phFriNfc_Llcp_CyclicFifoInit(P_UTIL_FIFO_BUFFER     sUtilFifo,
                                  const uint8_t        *pBuffStart,
                                  uint32_t             buffLength);

void phFriNfc_Llcp_CyclicFifoClear(P_UTIL_FIFO_BUFFER sUtilFifo);

uint32_t phFriNfc_Llcp_CyclicFifoWrite(P_UTIL_FIFO_BUFFER     sUtilFifo,
                                       uint8_t              *pData,
                                       uint32_t             dataLength);

uint32_t
phFriNfc_Llcp_CyclicFifoFifoRead(
    _Inout_                             P_UTIL_FIFO_BUFFER  sUtilFifo,
    _Out_writes_to_(dataLength, return) uint8_t             *pBuffer,
    _In_                                uint32_t            dataLength
    );


uint32_t phFriNfc_Llcp_CyclicFifoUsage(P_UTIL_FIFO_BUFFER sUtilFifo);
uint32_t phFriNfc_Llcp_CyclicFifoAvailable(P_UTIL_FIFO_BUFFER sUtilFifo);

uint32_t phFriNfc_Llcp_Header2Buffer( phFriNfc_Llcp_sPacketHeader_t *psHeader,
                                      uint8_t *pBuffer,
                                      uint32_t nOffset );

uint32_t phFriNfc_Llcp_Sequence2Buffer( phFriNfc_Llcp_sPacketSequence_t *psSequence,
                                        uint8_t *pBuffer,
                                        uint32_t nOffset );

uint32_t phFriNfc_Llcp_Buffer2Header( uint8_t *pBuffer,
                                      uint32_t nOffset,
                                      phFriNfc_Llcp_sPacketHeader_t *psHeader );

uint32_t phFriNfc_Llcp_Buffer2Sequence( uint8_t *pBuffer,
                                        uint32_t nOffset,
                                        phFriNfc_Llcp_sPacketSequence_t *psSequence );
