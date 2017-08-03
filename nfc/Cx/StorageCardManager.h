/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageCardManager.h

Abstract:

    Storage card manager declaration
    
Environment:

    User mode

--*/

#pragma once

#include "StorageClass.h"
#include "StorageClassJewel.h"
#include "StorageClassMiFare.h"
#include "StorageClassMifareStd.h"
#include "StorageClassFelica.h"
#include "StorageClassISO15693.h"

#define TAG_MULTI_BYTE_MASK         0x1F
#define TAG_COMPREHENSION_MASK      0x80
#define TAG_LENGTH_MULTI_BYTE_MASK  0x80

class StorageCardManager : public IStorageCard
{
public:
    static StorageCardManager*
    Create(
        _In_ phNfc_eRemDevType_t DeviceType,
        _In_ DWORD Sak,
        _In_ PNFCCX_SC_INTERFACE PSCInterface
        );

    ULONG AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG Release()
    {
        ULONG cRef = 0;

        if ((cRef = InterlockedDecrement(&m_cRef)) == 0) {
            delete this;
        }

        return cRef;
    }

public:
    BOOLEAN
    AtrCached();

    NTSTATUS
    GetCachedAtr(
                 _Out_writes_bytes_to_(cbBufferSize, *pcbReturnBufferSize) BYTE* pbBuffer,
                 _In_ DWORD cbBufferSize,
                 _Out_ DWORD* pcbReturnBufferSize
                 );

    void
    CacheAtr(
             _In_reads_bytes_(cbAtr) const BYTE* pbAtr,
             _In_ DWORD cbAtr
             );

    ApduResult
    ValidateParameters(
                        _In_opt_ BYTE *dataBuffer,
                        _In_opt_ BYTE *outBuffer
                        );

    ApduResult
    GetCommandFromAPDU(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_ PcscInsByte *pCommand
                        );

    void
    PrepareResponseCode(
                        _In_ ApduResult retError,
                        _Out_writes_bytes_all_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[]
                        );

    NTSTATUS
    Transmit(
             _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
             _In_ DWORD cbSize,
             _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
             _In_ DWORD cbOutBufferSize,
             _Out_ DWORD *cbReturnBufferSize,
             _In_ USHORT usTimeout
             );

public:
    void 
    UpdateUniqueID(
                    _In_reads_bytes_(uidLength) BYTE *Uid,
                    _In_ BYTE uidLength
                    )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->UpdateUniqueID(Uid, uidLength);
    }

    void 
    GetUniqueID(
                _Outptr_result_bytebuffer_(*uidLength) BYTE **uid,
                _Out_ BYTE *uidLength
                )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->GetUniqueID(uid, uidLength);
    }

    void 
    UpdateHistoricalBytes(
                        _In_reads_bytes_(HistoBytesLength) BYTE *HistoBytes,
                        _In_ DWORD HistoBytesLength
                        )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->UpdateHistoricalBytes(HistoBytes, HistoBytesLength);
    }
    
    ApduResult
    GetDataCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize 
                   )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->GetDataCommand(pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult 
    UpdateBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->UpdateBinary(pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult
    ReadBinary(
                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                _In_ DWORD cbSize,
                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                _In_ DWORD cbOutBufferSize,
                _Out_ DWORD *cbReturnBufferSize
                )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->ReadBinary(pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult 
    GetGeneralAuthenticateCommand(
                                _In_ void* getLoadKey,
                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                _In_ DWORD cbSize,
                                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                _In_ DWORD cbOutBufferSize,
                                _Out_ DWORD *cbReturnBufferSize  
                                )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->GetGeneralAuthenticateCommand(getLoadKey, pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult 
    HandleIncDecCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *cbReturnBufferSize  
                        )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->HandleIncDecCommand(pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult
    HandleManageSessionCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *cbReturnBufferSize  
                        );

    ApduResult
    HandleTransSessionCommand(
                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                        _In_ DWORD cbSize,
                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                        _In_ DWORD cbOutBufferSize,
                        _Out_ DWORD *cbReturnBufferSize  
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
                                        )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->UpdateResponseCodeToResponseBuffer(RetError, Command, cbSize, Sw1Sw2Return, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize);
    }

    ApduResult
    StorageCardTransceive(
                         _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                         _In_ DWORD cbSize,
                         _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                         _In_ DWORD cbOutBufferSize,
                         _Out_ DWORD *cbReturnBufferSize,
                         _In_ USHORT usTimeout = 0
                         )
    {
        NT_ASSERT(m_pStorageClass);
        return m_pStorageClass->StorageCardTransceive(pbDataBuffer, cbSize, pbOutBuffer, cbOutBufferSize, cbReturnBufferSize, usTimeout);
    }

protected:
    StorageCardManager(
                       _In_ phNfc_eRemDevType_t DeviceType,
                       _In_ DWORD Sak,
                       _In_ PNFCCX_SC_INTERFACE PSCInterface
                       );

    virtual ~StorageCardManager();

private:
    ApduResult
    ReadTlvDataObject(
                     _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                     _In_ DWORD cbSize,
                     _In_ DWORD BufferOffset,
                     _Outptr_result_buffer_(*pcbTagSize) const BYTE **ppbTag,
                     _Out_ DWORD *pcbTagSize,
                     _Out_ DWORD *pcbLengthSize,
                     _Outptr_result_buffer_(*pcbValueSize) const BYTE **ppbValue,
                     _Out_ DWORD *pcbValueSize
                     );

    ApduResult
    AppendTlvDataObjectToResponseBuffer(
                                        _In_ USHORT DataObjectTag,
                                        _In_ BYTE cbDataObjectSize,
                                        _In_reads_bytes_(cbDataObjectSize) const BYTE *pbDataObjectValue,
                                        _Out_writes_bytes_to_(cbOutBufferSize, *pBufferOffset) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Inout_ DWORD *pBufferOffset
                                        );

protected:
    ULONG                   m_cRef;
    PNFCCX_SC_INTERFACE     m_pScInterface;
    IStorageClass *         m_pStorageClass;
    BOOLEAN                 m_fTransparentSession;

private:
    BYTE                    m_rgbAtr[PHNFC_MAX_ATR_LENGTH];
    DWORD                   m_cbAtr;
};
