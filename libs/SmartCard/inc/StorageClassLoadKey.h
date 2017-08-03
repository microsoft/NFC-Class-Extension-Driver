/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassLoadKey.h

Abstract:

    Storage card load key declaration
    
Environment:

    User mode

--*/

#pragma once

#define DEFAULT_KEY_SPACE 10
#define KEYSIZE 0x06

class LoadKey
{
public:
    LoadKey();
    virtual ~LoadKey();

    NTSTATUS Initialize(DWORD n);
    VOID Uninitialize();
    
    BOOL InsertKey(
                    _In_ DWORD KeyNumber, 
                    _In_reads_bytes_(cbSize) const BYTE *pbKeyData,
                    _In_ DWORD cbSize
                    );
    DWORD ExtractKeyIndex(DWORD KeyNumber);
    BOOL ExtractKeys(
                    _In_ DWORD KeyIndex, 
                    _Out_writes_bytes_all_(cbSize) BYTE* pbKeyData,
                    _In_ DWORD cbSize
                    );
    BOOL OverWriteKeys(
                        _In_ DWORD KeyIndex, 
                        _In_ DWORD KeyNumber, 
                        _In_reads_bytes_(cbSize) const BYTE *pbKeyData,
                        _In_ DWORD cbSize
                        );
    BOOL KeyTableFull();
    BOOL KeyTableEmpty();

private:
    DWORD m_MaxNumberOfKeys;
    DWORD m_NumberOfKeys;
    DWORD *m_pBeginningOfKeyNumberStore;
    DWORD *m_pEndOfKeyNumberStore;
    DWORD *m_pWritePos;
    DWORD *m_pReadPos;
    BYTE **m_LoadKeyArray;
};
