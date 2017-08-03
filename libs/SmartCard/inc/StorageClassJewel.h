/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassJewel.h

Abstract:

    Storage card Jewel declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"

/* Topaz static commands */
#define TOPAZ_READID_COMMAND        0x78U
#define TOPAZ_READALL_COMMAND       0x00U
#define TOPAZ_READ_COMMAND          0x01U
#define TOPAZ_WRITEE_COMMAND        0x53U
#define TOPAZ_WRITENE_COMMAND       0x1AU
  
/* Topaz dynamic commands */
#define TOPAZ_RSEG_COMMAND          0x10U
#define TOPAZ_READ8_COMMAND         0x02U
#define TOPAZ_WRITEE8_COMMAND       0x54U
#define TOPAZ_WRITENE8_COMMAND      0x1BU

class StorageClassJewel : public IStorageClass
{
public:
    StorageClassJewel(IStorageCard *pStorageCard);
    virtual ~StorageClassJewel() {}

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
                             BYTE p1,
                             BYTE p2,
                             BYTE le
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
    BYTE                m_HistoricalBytes[MAX_STORAGECARD_HISTOBYTES];
    DWORD               m_HistoricalByteLength;
    BYTE                m_CommandBuffer[COMMAND_BUFFER_SIZE];
    DWORD               m_CmdBufferSize;
    BYTE                m_Uid[PHHAL_MAX_UID_LENGTH];
    BYTE                m_UidLength;
    BOOL                m_IsLesserLeLc;
};
