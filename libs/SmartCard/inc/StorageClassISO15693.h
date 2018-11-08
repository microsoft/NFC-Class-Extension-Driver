/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassISO15693.h

Abstract:

    Storage card ISO15693 declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"

#define ISO15693_BYTES_PER_BLOCK            0x4U

/* Standard command identifiers */
#define ISO15693_READ_COMMAND               0x20U
#define ISO15693_WRITE_COMMAND              0x21U
#define ISO15693_READ_MULTIPLE_COMMAND      0x23U

/* Extended command identifiers */
#define ISO15693_EXT_READ_COMMAND           0x30U
#define ISO15693_EXT_WRITE_COMMAND          0x31U

/* Response flags */
#define ISO15693_RESPONSE_ERROR_FLAG        0x01
#define ISO15693_RESPONSE_EXTENSION_FLAG    0x08

/* Response error codes */
#define ISO15693_COMMAND_NOT_SUPPORTED      0x01
#define ISO15693_COMMAND_NOT_RECOGNIZED     0x02
#define ISO15693_OPTION_NOT_SUPPORTED       0x03
#define ISO15693_UNKNOWN_ERROR              0x04
#define ISO15693_INVALID_BLOCK              0x10
#define ISO15693_BLOCK_ALREADY_LOCKED       0x11
#define ISO15693_BLOCK_LOCKED               0x12
#define ISO15693_BLOCK_NOT_PROGRAMMED       0x13
#define ISO15693_BLOCK_LOCK_FAILED          0x14

/* UID 6th byte manufacturer identifier */
#define ISO15693_MANUFACTURER_STM           0x02U
#define ISO15693_MANUFACTURER_NXP           0x04U
#define ISO15693_MANUFACTURER_TI            0x07U

#define ISO15693_UID_BYTE_5                 0x05U
#define ISO15693_UID_BYTE_6                 0x06U

/* UID value for SL2 ICS20, SL2S2002 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X      0x01U

/* UID value for SL2 ICS53, SL2 ICS54, SL2S5302 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X_S    0x02U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_S    0x00U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_SHC  0x80U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_SY   0x40U

/* UID value for SL2 ICS50, SL2 ICS51, SL2S5002 */
#define ISO15693_UIDBYTE_5_VALUE_SLI_X_L    0x03U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_L    0x00U
#define ISO15693_UIDBYTE_4_VALUE_SLI_X_LHC  0x80U

/* UID value for LRIS64K, M24LR04E-R, M24LR16E-R, M24LR64E-R */
#define ISO15693_UIDBYTE_5_STM_LRIS64K      0x44
#define ISO15693_UIDBYTE_5_STM_M24LR64R     0x2C
#define ISO15693_UIDBYTE_5_STM_M24LR64ER    0x5C
#define ISO15693_UIDBYTE_5_STM_M24LR16ER    0x4C

#define ISO15693_UIDBYTE_5_STM_MASK         0xFC

#define ISO15693_FLAG_PROTOEXT              0x08U
/* Check if protocol extension bit is needed in the request flag */
#define ISO15693_PROTOEXT(pUid) \
    ((ISO15693_MANUFACTURER_STM == pUid[ISO15693_UID_BYTE_6]) && \
        ((pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_LRIS64K || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64R || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR64ER || \
         (pUid[ISO15693_UID_BYTE_5] & ISO15693_UIDBYTE_5_STM_MASK) == ISO15693_UIDBYTE_5_STM_M24LR16ER))

class StorageClassISO15693 : public IStorageClass
{
public:
    StorageClassISO15693(IStorageCard *pStorageCard);
    virtual ~StorageClassISO15693() {}

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

    ApduResult
    MapErrorCodeToApduStatus(
                            _In_ BYTE ErrorCode
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
