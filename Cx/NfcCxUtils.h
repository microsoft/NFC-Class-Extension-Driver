/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    NfcCxUtils.h

Abstract:

    This module declares utility functions that are used throughout
    the class extension implementation

--*/

#pragma once

#define STR_GUID_FMTW       L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
#define STR_GUID_LENGTH     ARRAYSIZE(L"{00000000-0000-0000-0000-000000000000}")

NFCSTATUS
NfcCxNfcStatusFromNtStatus(
    _In_ NTSTATUS Status
    );

NTSTATUS
NfcCxNtStatusFromNfcStatus(
    _In_ NFCSTATUS NfcStatus
    );

NTSTATUS
NfcCxGuidFromUnicodeString(
    _In_ PCUNICODE_STRING GuidString,
    _Out_ GUID* Guid
    );

NTSTATUS
NfcCxUnicodeStringFromGuid(
    _In_ REFGUID Guid,
    _Out_ PUNICODE_STRING GuidString
    );

NTSTATUS
NfcCxWideStringToNarrowString(
    _In_ size_t cchWideStr,
    _In_reads_z_(cchWideStr) PCWSTR WideStr,
    _Out_ size_t* cchNarrowStr,
    _Out_writes_z_(*cchNarrowStr) PSTR* NarrowStr
    );

NTSTATUS
NfcCxCopyToBuffer(
    _In_reads_bytes_(cbInput) const void* pbInput,
    _In_ size_t cbInput,
    _Out_writes_bytes_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ size_t* pcbOutputBuffer
    );

NTSTATUS
NfcCxRegistryQueryULong(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ ULONG* Value
    );

NTSTATUS
NfcCxRegistryQueryBoolean(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ BOOLEAN* Value
    );

NTSTATUS
NfcCxRegistryAssignULong(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ ULONG Value
    );

NTSTATUS
NfcCxRegistryAssignBoolean(
    _In_ WDFKEY Key,
    _In_ PCWSTR ValueName,
    _Out_ BOOLEAN Value
    );

class CNFCPayload
{
public:
    CNFCPayload() : m_pbPayload(NULL), m_cbPayload(0)
    {
        InitializeListHead(&m_ListEntry);
    }  
    ~CNFCPayload()
    {  
        if ((m_pbPayload != NULL) &&
            (m_pbPayload != (PBYTE) &m_dwFlags)) {
            delete [] m_pbPayload;
        }

        m_pbPayload = NULL;
        m_cbPayload = 0;
    }
    NTSTATUS Initialize(
        _In_reads_bytes_(cbPayload) PBYTE pbPayload,
        _In_ DWORD cbPayload
        )  
    {  
        NTSTATUS Status = STATUS_SUCCESS;
  
        if (cbPayload > sizeof(DWORD)) {
            m_pbPayload = new BYTE[cbPayload];
        }
        else {
            m_pbPayload = (PBYTE) &m_dwFlags;
        }

        if (m_pbPayload != NULL) {
            m_cbPayload = cbPayload;

            if (pbPayload != NULL) {
                RtlCopyMemory(m_pbPayload, pbPayload, cbPayload);
            }
            else {
                RtlZeroMemory(m_pbPayload, m_cbPayload);
            }
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        return Status;
    }
    PBYTE GetPayload()
    {
        return m_pbPayload;
    }
    DWORD GetSize()
    {
        return m_cbPayload;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CNFCPayload* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CNFCPayload*) CONTAINING_RECORD(pEntry, CNFCPayload, m_ListEntry);
    }

private:
    PBYTE           m_pbPayload;
    DWORD           m_cbPayload;
    DWORD           m_dwFlags;
    LIST_ENTRY      m_ListEntry;
};

class CNFCPendedRequest 
{    
public:
    CNFCPendedRequest () : m_bInitialized(FALSE) {};
    ~CNFCPendedRequest () {};

    _No_competing_thread_
    NTSTATUS
    Initialize(
        _In_ PNFCCX_FDO_CONTEXT FdoContext,
        _In_ BOOL _HintSize,
        _In_ BYTE _MaxQueueLength
        );
    
    VOID
    Deinitialize();
    
    NTSTATUS
    ProcessRequest(
        _In_ PNFCCX_FILE_CONTEXT FileContext,
        _In_ WDFREQUEST Request,
        _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
        _In_ size_t OutputBufferLength
        );
    
    NTSTATUS
    ProcessPayload(
        _In_ CNFCPayload* Payload
        );
    
    NTSTATUS
    CopyPayloadData(
        _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
        _In_ ULONG OutputBufferLength,
        _In_bytecount_(DataLength) PVOID Data,
        _In_ ULONG DataLength,
        _Out_ PULONG BufferUsed
        );

    NTSTATUS
    CompleteRequest(
        _In_bytecount_(DataLength) PVOID Data,
        _In_ ULONG DataLength
        );

private:
    WDFWAITLOCK m_QueueLock;
    _Guarded_by_(m_QueueLock)
    LIST_ENTRY m_PayloadQueue;
    UCHAR m_ucPayloadQueueLength;
    WDFQUEUE m_RequestQueue;
    BOOL m_bHintSize;
    BOOL m_bInitialized;
    BYTE m_bMaxQueueLength;
};

