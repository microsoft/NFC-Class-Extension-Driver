/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    NciPacketParser.cpp

Abstract:

    This module implements the Nci Packet Parser

--*/
#include "NciParsersLibPch.h"

#include "NciPacketParser.tmh"

#ifdef __cplusplus
    extern "C" {
#endif

BOOLEAN
NciPacketHeaderGetFromBuffer(
    _In_bytecount_(BufferLength) PUCHAR Buffer,
    _In_ UINT BufferLength,
    _Out_ PNCI_PACKET_HEADER NciPacketHeader
    )
/**++

Routine Description:

    Given a 3 byte buffer, reads and validates the NciPacketHeader

Arguments:

    Buffer - A byte buffer that contains the packet header
    BufferLength - The buffer length
    NciPacketHeader - A pointer to a caller allocated structure to receive the content
                      of the Nci Packet Header

Return Value:

    BOOLEAN

--*/
{
    BOOLEAN retValue = TRUE;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);


    if (NCI_PACKET_HEADER_LENGTH > BufferLength) {
        TRACE_LINE(LEVEL_ERROR, "Buffer is too small");
        retValue = FALSE;
        goto Done;
    }
    else if (BufferLength > NCI_PACKET_HEADER_LENGTH + NCI_PACKET_MAX_SIZE) {
        TRACE_LINE(LEVEL_ERROR, "Buffer is too large");
        retValue = FALSE;
        goto Done;
    }
    else if (NULL == Buffer) {
        TRACE_LINE(LEVEL_ERROR, "Buffer is NULL");
        retValue = FALSE;
        goto Done;
    }

    if (BufferLength > NCI_PACKET_MAX_SIZE) {
        //
        // For diagnostics.
        //
        TRACE_LINE(LEVEL_INFO, "NCI packet length is over 255: BufferLength=%u", BufferLength);
    }

    //
    // The first byte contains the packet's MT and PBF
    //
    NciPacketHeader->MessageType = NCI_PACKET_HEADER_GET_MT(Buffer);
    NciPacketHeader->PBF = NCI_PACKET_HEADER_GET_PBF(Buffer);

    //
    // Validate the MT
    //
    if (NCI_PACKET_MT_DATA != NciPacketHeader->MessageType &&
        NCI_PACKET_MT_CONTROL_COMMAND != NciPacketHeader->MessageType &&
        NCI_PACKET_MT_CONTROL_RESPONSE != NciPacketHeader->MessageType &&
        NCI_PACKET_MT_CONTROL_NOTIFICATION != NciPacketHeader->MessageType) {

        TRACE_LINE(LEVEL_ERROR, "Unsupported MessageType, %d", NciPacketHeader->MessageType);
        retValue = FALSE;
        goto Done;
    }

    if (NCI_PACKET_MT_DATA == NciPacketHeader->MessageType) {
        NciPacketHeader->Header.DataPacket.ConnId = NCI_PACKET_HEADER_GET_DATA_CONNID(Buffer);
    }
    else {
        NciPacketHeader->Header.ControlPacket.Gid = NCI_PACKET_HEADER_GET_CONTROL_GID(Buffer);
        NciPacketHeader->Header.ControlPacket.Oid = NCI_PACKET_HEADER_GET_CONTROL_OID(Buffer);
    }

    //
    // In all the above (validated) MTs, the Payload
    // length is in the 3rd byte.  Since all values
    // from 0 to 255, no further validation is required
    // on the payload length
    //
    NciPacketHeader->PayloadLength = NCI_PACKET_HEADER_GET_PAYLOADLENGTH(Buffer);

Done:

    if (FALSE == retValue) {
        ZeroMemory(NciPacketHeader, sizeof(NCI_PACKET_HEADER));
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);

    return retValue;
}

#ifdef __cplusplus
    }
#endif
