/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassFelica.h

Abstract:

    Storage card Felica declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"

#define FELICA_READ_BLOCK       0x06
#define FELICA_WRITE_BLOCK      0x08

typedef struct __SERVICE_INFO__
{
    uint8_t NumOfServices;
    uint8_t ServiceCodeList[2];
    uint8_t NumOfBlocks;
    uint8_t FirstBlocksElement;
    uint8_t FirstBlocksElementNum;
} ServiceInfo;

typedef struct __WRITE_VERSION_INFO__
{
    uint8_t Version;    // 01 
    uint8_t NbR;        // 10 
    uint8_t NbW;        // 04 
    uint8_t NMbax[2];   // 01 00 
    uint8_t RFU[4];     // 00 00 00 00 
    uint8_t WriteFlag;  // 00 
    uint8_t RWFlag;     // 01 
    uint8_t DataLen[3]; // 00 00 10 
    uint8_t CheckSum[2];// 00 27 
} WriteVersionInfo;

class StorageClassFelica: public IStorageClass
{
public:
    StorageClassFelica(IStorageCard *pStorageCard);
    virtual ~StorageClassFelica() {}

    virtual void 
    GetUniqueID(
                _Outptr_result_bytebuffer_(*pUidLength) BYTE **pUid,
                _Out_ BYTE *pUidLength
                );

    virtual void 
    UpdateUniqueID(
                    _In_reads_bytes_(uidLength) BYTE *pUid,
                    _In_ BYTE uidLength
                    );

    virtual void 
    UpdateHistoricalBytes(
                        _In_reads_bytes_(HistoBytesLength) BYTE *pHistoBytes,
                        _In_ DWORD HistoBytesLength
                        );

    virtual ApduResult 
    GetDataCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *pcbReturnBufferSize 
                    );

    virtual ApduResult 
    UpdateBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *pcbReturnBufferSize
                );

    virtual ApduResult
    ReadBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *pcbReturnBufferSize
                );

    virtual ApduResult 
    GetGeneralAuthenticateCommand(
                                  _In_ void* getLoadKey,
                                  _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                  _In_ DWORD cbSize,
                                  _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                  _In_ DWORD cbOutBufferSize,
                                  _Out_ DWORD *pcbReturnBufferSize  
                                 );

    virtual ApduResult 
    HandleIncDecCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *pcbReturnBufferSize  
                        );

    virtual void
    UpdateResponseCodeToResponseBuffer(
                                        _In_ ApduResult RetError,
                                        _In_ PcscInsByte Command,
                                        _In_ DWORD cbSize,
                                        _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                        _Inout_updates_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Out_ DWORD *pcbReturnBufferSize  
                                        );

    virtual ApduResult
    StorageCardTransceive(
                          _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                          _In_ DWORD cbSize,
                          _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                          _In_ DWORD cbOutBufferSize,
                          _Out_ DWORD *pcbReturnBufferSize,
                          _In_ USHORT usTimeout
                          );

private:
    ApduResult
    ValidateGeneralAuthenticateParameters(
                                            _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                            _In_ DWORD cbSize
                                            );

    ApduResult
    ValidateReadBinaryParamters(
                                BYTE p1,
                                BYTE p2,
                                BYTE le
                                );

    ApduResult
    ValidateUpdateBinaryParameters(
                                     BYTE  p1,
                                     BYTE  p2,
                                     BYTE  lc
                                     );

    ApduResult
    PrepareTransceiveForRead(
                             _In_ PPcscCommandApduInfo pPcscCmdApdu
                             );

    ApduResult
    PrepareTransceiveForUpdate(
                               _In_ PPcscCommandApduInfo pPcscCmdApdu
                               );

    ApduResult 
    PrepareTransceiveForGeneralAuthenticate(
                                            _In_ void* getLoadKey,
                                            _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                            _In_ DWORD cbSize,
                                            _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                            _In_ DWORD cbOutBufferSize,
                                            _Out_ DWORD *pcbReturnBufferSize 
                                            );

protected:
    IStorageCard*       m_pStorageCard;
    BOOL                m_IsAuthSupported;
    BYTE                m_HistoricalBytes[MAX_STORAGECARD_HISTOBYTES];
    DWORD               m_HistoricalByteLength;
    BYTE                m_CommandBuffer[COMMAND_BUFFER_SIZE];
    DWORD               m_CmdBufferSize;
    BYTE                m_Uid[PHHAL_MAX_UID_LENGTH];
    BYTE                m_UidLength;
};
