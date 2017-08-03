/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassMifare.h

Abstract:

    Storage card Mifare UL declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"

#define MIFARE_UL_READ_BUFFER_SIZE      0x10
#define MIFARE_UL_PAGE_SIZE             0x04
#define MIFARE_UL_NUMBER_OF_PAGES       0x10

#define MIFARE_NAK_ACCESS_DENIED        0x00
#define MIFARE_NAK_CRC_ERROR1           0x01
#define MIFARE_NAK_RFU1                 0x02
#define MIFARE_NAK_RFU2                 0x03
#define MIFARE_NAK_UNDER_OR_OVERFLOW    0x04
#define MIFARE_NAK_CRC_ERROR2           0x05
#define MIFARE_NAK_ACK                  0x0A

class StorageClassMiFareUL: public IStorageClass
{
public:
    StorageClassMiFareUL(IStorageCard *pStorageCard);
    virtual ~StorageClassMiFareUL(){}

    virtual void 
    UpdateUniqueID(
                    _In_reads_bytes_(uidLength) BYTE *Uid,
                    _In_ BYTE uidLength
                    );

    virtual void 
    GetUniqueID(
                _Outptr_result_bytebuffer_(*uidLength) BYTE **uid,
                _Out_ BYTE *uidLength
                );

    virtual void 
    UpdateHistoricalBytes(
                            _In_reads_bytes_(HistoBytesLength) BYTE *HistoBytes,
                            _In_ DWORD HistoBytesLength
                            );

    virtual ApduResult
    GetDataCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize 
                   );

    virtual ApduResult 
    UpdateBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                );

    virtual ApduResult
    ReadBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                );

    virtual ApduResult 
    GetGeneralAuthenticateCommand(
                                _In_ void* getLoadKey,
                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                _In_ DWORD cbSize,
                                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                _In_ DWORD cbOutBufferSize,
                                _Out_ DWORD *cbReturnBufferSize  
                                );

    virtual ApduResult 
    HandleIncDecCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *cbReturnBufferSize  
                        );

    ApduResult
    ValidateGetDataCommandParamters(
                                     BYTE p1,
                                     BYTE p2
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
    ValidateGeneralAuthenticateParameters(
                                          BYTE p1,
                                          BYTE p2,
                                          BYTE lc
                                          );

    ApduResult
    PrepareTransceiveForRead(
                             BYTE p1,
                             BYTE p2,
                             BYTE le
                             );

    ApduResult
    PrepareTransceiveForUpdate(
                               _In_ PPcscCommandApduInfo pPcscCmdApdu
                               );

    void
    UpdateResponseCodeToResponseBuffer(
                                        _In_ ApduResult RetError,
                                        _In_ PcscInsByte Command,
                                        _In_ DWORD cbSize,
                                        _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                        _Inout_updates_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Out_ DWORD *cbReturnBufferSize  
                                        );

    virtual ApduResult
    StorageCardTransceive(
                          _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                          _In_ DWORD cbSize,
                          _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                          _In_ DWORD cbOutBufferSize,
                          _Out_ DWORD *cbReturnBufferSize,
                          _In_ USHORT usTimeout
                          );

protected:
    ApduResult 
    MifareTransceive(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize,
                    _In_ USHORT usTimeout = 0
                    );

protected:
    IStorageCard*           m_pStorageCard;
    BYTE                    m_CommandBuffer[COMMAND_BUFFER_SIZE];
    DWORD                   m_CmdBufferSize;
    BOOL                    m_IsAuthSupported;
    BYTE                    m_HistoricalBytes[MAX_STORAGECARD_HISTOBYTES];
    DWORD                   m_HistoricalByteLength;
    BYTE                    m_Uid[PHNFC_MAX_UID_LENGTH];
    BYTE                    m_UidLength;
    BOOL                    m_IsLesserLeLc;
};
