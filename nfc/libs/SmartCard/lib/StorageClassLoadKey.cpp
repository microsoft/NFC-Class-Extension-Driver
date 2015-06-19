/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageCardLoadKey.h

Abstract:

    Storage class load key implementation
    
Environment:

    User mode

--*/
#include "Pch.h"

#include "StorageClassLoadKey.tmh"

LoadKey::LoadKey()
    : m_MaxNumberOfKeys(0)
    , m_NumberOfKeys(0)
    , m_pBeginningOfKeyNumberStore(NULL)
    , m_pEndOfKeyNumberStore(NULL)
    , m_pWritePos(NULL)
    , m_pReadPos(NULL)
    , m_LoadKeyArray(NULL)
{
}

LoadKey::~LoadKey()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    Uninitialize();
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

NTSTATUS 
LoadKey::Initialize(DWORD n)
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    Uninitialize();

    m_NumberOfKeys = 0;
    m_pBeginningOfKeyNumberStore = new DWORD[n];
    if (NULL == m_pBeginningOfKeyNumberStore) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    m_pEndOfKeyNumberStore = m_pBeginningOfKeyNumberStore + n;
    m_pWritePos = m_pBeginningOfKeyNumberStore;
    m_pReadPos = m_pBeginningOfKeyNumberStore;

    m_LoadKeyArray = new BYTE *[n] ;
    if (NULL == m_LoadKeyArray) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    for (DWORD i = 0 ; i < n ; i++ ) {
        m_LoadKeyArray[i] = new BYTE[KEYSIZE];
        if (NULL == m_LoadKeyArray[i]) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }
        m_MaxNumberOfKeys = i + 1;
    }

Done:
    if (!NT_SUCCESS(status)) {
        Uninitialize();
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

VOID
LoadKey::Uninitialize()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    if (m_pBeginningOfKeyNumberStore != NULL) {
        delete [] m_pBeginningOfKeyNumberStore;
        m_pBeginningOfKeyNumberStore = NULL;
    }

    if (m_LoadKeyArray != NULL) {
        for (DWORD i = 0 ; i < m_MaxNumberOfKeys ; i++ ) {
            if (m_LoadKeyArray[i] != NULL) {
                delete [] m_LoadKeyArray[i];
                m_LoadKeyArray[i] = NULL;
            }
        }
    
        delete [] m_LoadKeyArray;
        m_LoadKeyArray = NULL;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

BOOL LoadKey::KeyTableFull()
{
    return m_MaxNumberOfKeys == m_NumberOfKeys;
}

BOOL LoadKey::KeyTableEmpty()
{
    return m_NumberOfKeys == 0;
}

BOOL LoadKey::InsertKey(
                        _In_ DWORD KeyNumber, 
                        _In_reads_bytes_(cbSize) const BYTE *pbKeyData,
                        _In_ DWORD cbSize
                        )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, "nkey = %0x, maxkey = %0x", m_NumberOfKeys, m_MaxNumberOfKeys);

    if (cbSize < KEYSIZE) {
        return FALSE;
    }

    if (m_NumberOfKeys < m_MaxNumberOfKeys) {
        *(m_pWritePos + m_NumberOfKeys) = KeyNumber;
        RtlCopyMemory(m_LoadKeyArray[m_NumberOfKeys], pbKeyData, KEYSIZE);
        m_NumberOfKeys++;
        return TRUE;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return FALSE;
}

DWORD LoadKey::ExtractKeyIndex(DWORD KeyNumber)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    for (DWORD i = 0; i < m_NumberOfKeys; i++) {
        if(KeyNumber == *(m_pReadPos + i)) {
            TRACE_LINE(LEVEL_INFO, "KeyNumber Matched = %0x", i);
            return i;
        }
    }
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return (DWORD)-1;
}

BOOL LoadKey::ExtractKeys(
                        _In_ DWORD KeyIndex, 
                        _Out_writes_bytes_all_(cbSize) BYTE* pbKeyData,
                        _In_ DWORD cbSize
                        )
{
   TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

   if (cbSize < KEYSIZE) {
       return FALSE;
   }
   
   RtlCopyMemory(pbKeyData, m_LoadKeyArray[KeyIndex], KEYSIZE);
   
   TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
   return TRUE;
}

BOOL LoadKey::OverWriteKeys(
                            _In_ DWORD KeyIndex, 
                            _In_ DWORD KeyNumber, 
                            _In_reads_bytes_(cbSize) const BYTE *pbKeyData,
                            _In_ DWORD cbSize
                            )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (cbSize < KEYSIZE) {
        return FALSE;
    }

    if (KeyIndex < m_MaxNumberOfKeys) {
        TRACE_LINE(LEVEL_INFO, "KeyIndex = %0x, KeyNumber = %0x", KeyIndex, KeyNumber);
        *(m_pWritePos + KeyIndex) = KeyNumber;
        RtlCopyMemory(m_LoadKeyArray[KeyIndex], pbKeyData, KEYSIZE);
        return TRUE;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return FALSE;
}