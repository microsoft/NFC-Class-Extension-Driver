/*++

Copyright (c) 2012      Microsoft Corporation

Module Name:

   NciParsersLib.h

Abstract:

   NciParsers Library declarations

Environment:

    User mode

--*/

#pragma once

//
// All NCI Packets are at least 3 bytes
//
#define NCI_PACKET_HEADER_LENGTH  (0x3)

typedef struct _NCI_PACKET_HEADER {
    UCHAR MessageType;
    UCHAR PBF;
    union {
        struct {
            UCHAR Gid;
            UCHAR Oid;
        } ControlPacket;

        struct {
            UCHAR ConnId;
        } DataPacket;
    } Header;
    UCHAR PayloadLength;
} NCI_PACKET_HEADER, *PNCI_PACKET_HEADER;

#ifdef __cplusplus
    extern "C" {
#endif

BOOLEAN
NciPacketHeaderGetFromBuffer(
    _In_bytecount_(BufferLength) PUCHAR Buffer,
    _In_ UINT BufferLength,
    _Out_ PNCI_PACKET_HEADER NciPacketHeader
    );

#ifdef __cplusplus
    }
#endif
