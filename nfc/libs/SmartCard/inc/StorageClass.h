/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClass.h

Abstract:

    Storage class interface declaration
    
Environment:

    User mode

--*/

#pragma once

#define IFD_MAJOR_VER                   1
#define IFD_MINOR_VER                   0
#define IFD_BUILD_NUM                   1

#define MAX_STORAGECARD_HISTOBYTES      15
#define MAX_BUFFERSIZE                  255
#define MIN_APDU_HEADER                 5
#define COMMAND_BUFFER_SIZE             512
#define DEFAULT_APDU_STATUS_SIZE        2

enum ApduResult
{
    RESULT_SUCCESS,
    RESULT_INVALID_PARAM,
    RESULT_WRONG_CLA,
    RESULT_WRONG_P1P2,
    RESULT_NOT_SUPPORTED,
    RESULT_DONOT_SEND,
    RESULT_COMMAND_INCOMPATIBLE,
    RESULT_LE_LESSER,
    RESULT_LE_GREATER,
    RESULT_WRONG_LC,
    RESULT_WRONG_LENGTH,    // 0x67, 0x00
    RESULT_AUTH_PARAM_ERR,  // 0x65, 0x81
    RESULT_WRONG_KEY_TYPE,  // 0x69, 0x86
    RESULT_WRONG_KEY_NR,
    RESULT_WRONG_AUTH_VERSION,  // 0x69, 0x84.
    RESULT_WRONG_APDU_LENGTH,
    RESULT_SECURITY_STATUS_NOT_SATISFIED, // 0x69, 0x82
    RESULT_MEMORY_FAILURE, // 0x65, 0x81
    RESULT_INVALID_BLOCK_ADDRESS, // 0x6A, 0x82
    RESULT_COMMAND_NOT_ALLOWED, // 0x69, 0x86
    RESULT_INVALID_VALUE, // 0x6A, 0x80
    RESULT_INVALID_SERVICE_COUNT, // 0xFF 0xA1h
    RESULT_WRONG_SERVICE_CODE_ACCESS, // 0x01 0xA6h
    RESULT_INVALID_BLOCK_COUNT,  // 0xFF 0xA2h
    RESULT_INVALID_BLOCK_LIST, // 0x01 0xA1h
    RESULT_INVALID_DATA_SIZE, // 0x01 0xA9h
    RESULT_UNKNOWN_FAILURE, // 0x6F 0x00
    RESULT_ERROR
};

typedef enum PcscInsByte
{
    PcscMifareLoadAuthKeyCmd = 0x82,
    PcscMifareGenAuthCmd = 0x86,
    PcscReadCmd = 0xB0,
    PcscGetDataCmd = 0xCA,
    PcscWriteCmd = 0xD6,
    PcscSelectFileCmd = 0xA4,
    PcscEnvelopeCmd = 0xC2,
    PcscManageSessionCmd = 0xFEFA,
    PcscTransExchangeCmd = 0xFEFB,
    PcscSwitchProtocolCmd = 0xFEFC,
    PcscIncrementDecrementCmd = 0xFEFD,
    PcscInvalidCmd = 0xFF
};

typedef struct _PcscAuthentication
{
    unsigned char AuthVersion;
    unsigned char AuthMSB;
    unsigned char AuthLSB;
    unsigned char AuthKeyType;
    unsigned char AuthKeyNR;
} PcscAuthentication, *PPcscAuthentication;

typedef struct _PcscCommandApduInfo 
{
    unsigned char Cla; 
    unsigned char Ins; 
    unsigned char P1; 
    unsigned char P2; 
    unsigned char Lc;
    unsigned char DataIn[MAX_BUFFERSIZE];
    unsigned char Le;
    unsigned char APDU[MAX_BUFFERSIZE]; /*Used when APDU format is not as per PCSC part 3 spec*/
    unsigned char APDULen;
} PcscCommandApduInfo, *PPcscCommandApduInfo;

typedef enum PcscManageSession
{
    VersionCmd = 0x80,
    StartTransSessionCmd = 0x81,
    EndTransSessionCmd = 0x82,
    RfFieldOnCmd = 0x83,
    RfFieldOffCmd = 0x84,
    GetParametersCmd = 0xFF6D,
    SetParametersCmd = 0xFF6E,
    GenericErrorRsp = 0xC0,
    VersionRsp = 0x80,
    GetParametersRsp = 0xFF6D,
    SetParametersRsp = 0xFF6E,
};

typedef enum PcscTransparentExchange
{
    TransmitBitFramingCmd = 0x91,
    ReceiptionBitFramingCmd = 0x92,
    TransmitCmd = 0x93,
    ReceiveCmd = 0x94,
    TransceiveCmd = 0x95,
    TimerCmd = 0x5F46,
    ResponseStatus = 0x96,
    IccResponse = 0x97,
};

typedef enum PcscIncrementDecrement
{
    IncrementCmd = 0xA0,
    DecrementCmd = 0xA1,
    BlkAddressCmd = 0x80,
    BlkValueCmd = 0x81,
};

class IStorageCard
{
public:
    virtual NTSTATUS
    Transmit(
              _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
              _In_ DWORD cbSize,
              _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
              _In_ DWORD cbOutBufferSize,
              _Out_ DWORD *pcbReturnBufferSize,
              _In_ USHORT usTimeout
              ) = 0;
};

class IStorageClass
{
public:
    virtual void 
    UpdateUniqueID(
                    _In_reads_bytes_(uidLength) BYTE *Uid,
                    _In_ BYTE uidLength
                    ) = 0;

    virtual void 
    GetUniqueID(
                _Outptr_result_bytebuffer_(*uidLength) BYTE **uid,
                _Out_ BYTE *uidLength
                ) = 0;

    virtual void 
    UpdateHistoricalBytes(
                        _In_reads_bytes_(HistoBytesLength) BYTE *HistoBytes,
                        _In_ DWORD HistoBytesLength
                        ) = 0;
    
    virtual ApduResult
    GetDataCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize 
                   ) = 0;

    virtual ApduResult 
    UpdateBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                ) = 0;

    virtual ApduResult
    ReadBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                ) = 0;

    virtual ApduResult 
    GetGeneralAuthenticateCommand(
                                _In_ void* getLoadKey,
                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                _In_ DWORD cbSize,
                                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                _In_ DWORD cbOutBufferSize,
                                _Out_ DWORD *cbReturnBufferSize  
                                ) = 0;

    virtual ApduResult 
    HandleIncDecCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *cbReturnBufferSize  
                        ) = 0;

    virtual void
    UpdateResponseCodeToResponseBuffer(
                                        _In_ ApduResult RetError,
                                        _In_ PcscInsByte Command,
                                        _In_ DWORD cbSize,
                                        _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                        _Inout_updates_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Out_ DWORD *cbReturnBufferSize  
                                        ) = 0;

    virtual ApduResult
    StorageCardTransceive(
                          _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                          _In_ DWORD cbSize,
                          _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                          _In_ DWORD cbOutBufferSize,
                          _Out_ DWORD *cbReturnBufferSize,
                          _In_ USHORT usTimeout
                          ) = 0;
};
