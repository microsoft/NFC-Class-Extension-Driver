/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassMifareStd.h

Abstract:

    Storage card Mifare Classic declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"
#include "StorageClassMiFare.h"
#include "StorageClassLoadKey.h"

#define MIFARE_CLASSIC_AUTH_UIDSIZE     0x04
#define MIFARE_CLASSIC_BLOCK_SIZE       0x10

class StorageClassMifareStd: public StorageClassMiFareUL
{
public:
    StorageClassMifareStd(IStorageCard *pStorageCard);
    virtual ~StorageClassMifareStd(){}

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
    ParseIncDecDataAndDoTransceive(
                            _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                            _In_ DWORD cbSize,
                            _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                            _In_ DWORD cbOutBufferSize,
                            _Out_ DWORD *cbReturnBufferSize 
                            );

    ApduResult ValidateGeneralAuthenticateParameters(
                                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                _In_ DWORD cbSize
                                                );

    ApduResult 
    PrepareTransceiveForGeneralAuthenticate(
                                            _In_ void* getLoadKey,
                                            _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                            _In_ DWORD cbSize,
                                            _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                            _In_ DWORD cbOutBufferSize,
                                            _Out_ DWORD *cbReturnBufferSize 
                                            );

    ApduResult
    ValidateReadBinaryParamters(
                                BYTE p1,
                                BYTE p2,
                                BYTE le
                                );

    ApduResult
    PrepareTransceiveForRead(
                             BYTE p1,
                             BYTE p2,
                             BYTE le
                             );

    ApduResult
    ValidateUpdateBinaryParameters(
                             BYTE p1,
                             BYTE p2,
                             BYTE lc
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
};
