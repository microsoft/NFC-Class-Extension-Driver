/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcProximityBuffer.cpp

Abstract:

    Buffer parsing utilities

Environment:

    User mode

--*/

#include "Pch.h"

#include "NfcProximityBuffer.tmh"

typedef struct _URI_PREFIX_IDENT_ENTRY {
    UCHAR PrefixKey;
    PWSTR Prefix;
} URI_PREFIX_IDENT_ENTRY, *PURI_PREFIX_IDENT_ENTRY;

//
// The URI identifier specified in the  NFCForum-TS-RTD_URI_1.0 spec, It is changed the
// order in the below format, because to validate L"urn:", and L"urn:epc:id:" cases.
//
static const URI_PREFIX_IDENT_ENTRY PrefixTable[] = {
    {0x00, L"N/A",}, //Invalid
    {0x01, L"http://www."},
    {0x02, L"https://www."},
    {0x03, L"http://"},
    {0x04, L"https://"},
    {0x05, L"tel:"},
    {0x06, L"mailto:"},
    {0x07, L"ftp://anonymous:anonymous@"},
    {0x08, L"ftp://ftp."},
    {0x09, L"ftps://"},
    {0x0A, L"sftp://"},
    {0x0B, L"smb://"},
    {0x0C, L"nfs://"},
    {0x0D, L"ftp://"},
    {0x0E, L"dav://"},
    {0x0F, L"news:"},
    {0x10, L"telnet://"},
    {0x11, L"imap:"},
    {0x12, L"rtsp://"},
    {0x1E, L"urn:epc:id:"},
    {0x1F, L"urn:epc:tag:"},
    {0x20, L"urn:epc:pat:"},
    {0x21, L"urn:epc:raw:"},
    {0x22, L"urn:epc:"},
    {0x23, L"urn:nfc:"},
    {0x13, L"urn:"},
    {0x14, L"pop:"},
    {0x15, L"sip:"},
    {0x16, L"sips:"},
    {0x17, L"tftp:"},
    {0x18, L"btspp://"},
    {0x19, L"btl2cap://"},
    {0x1A, L"btgoep://"},
    {0x1B, L"tcpobex://"},
    {0x1C, L"irdaobex://"},
    {0x1D, L"file://"}
};

static NTSTATUS FORCEINLINE
NTSTATUS_FROM_STRSAFE_HRESULT(
    _In_ HRESULT strSafeHr
    )
{
    return NTSTATUS_FROM_WIN32(HRESULT_CODE(strSafeHr));
}

NTSTATUS
CNFCProximityBuffer::ValidateNdefMessage(
    _In_ USHORT cbRawNdef,
    _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT32 cRawRecords = MAX_RECORDS_PER_MESSAGE;
    UCHAR IsChunked[MAX_RECORDS_PER_MESSAGE];
    UCHAR *RawRecords[MAX_RECORDS_PER_MESSAGE];

    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NFCSTATUS nfcStatus = phFriNfc_NdefRecord_GetRecords(pbRawNdef,
                                                         cbRawNdef,
                                                         RawRecords,
                                                         IsChunked,
                                                         &cRawRecords);

    if ((NFCSTATUS_SUCCESS != nfcStatus) || (cRawRecords == 0)) {
        TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_GetRecords failed with status: 0x%02X", nfcStatus);
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

Done:
    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
CNFCProximityBuffer::InitializeBarcode(
    _In_ USHORT cbPayload,
    _In_bytecount_(cbPayload) PBYTE pbPayload
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    NTSTATUS status = STATUS_SUCCESS;

    if (cbPayload < BARCODE_TYPE_MIN_LENGTH || cbPayload > BARCODE_TYPE_MAX_LENGTH) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    status = SetRawBuffer(cbPayload, pbPayload);

    if (!NT_SUCCESS(status)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    m_Barcode = TRUE;

    // Check barcode payload type
    switch (pbPayload[1])
        {
        case BARCODE_HTTP_WWW_TYPE:
        case BARCODE_HTTPS_WWW_TYPE:
        case BARCODE_HTTP_TYPE:
        case BARCODE_HTTPS_TYPE:
        {
            m_cbPayload = cbPayload - BARCODE_NON_PAYLOAD_SIZE;
            UCHAR i;
            for (i = 2; i < m_cbPayload; i++)
            {
                if (pbPayload[i] == BARCODE_TYPE_URI_TERMINATOR)
                {
                    m_cbPayload = i - 1;
                    break;
                }
            }

            // Include format identifier
            m_cbPayload += 1;
            m_pbPayload = new BYTE[m_cbPayload];

            // Take on extra to get prefix identifier
            CopyMemory(m_pbPayload, pbPayload + 1, m_cbPayload);
            m_tnf = NDEFRECORD_TNF_NFCWELLKNOWN;
            break;
        }
        case BARCODE_EPC_URN_TYPE:
        {
            m_cbPayload = cbPayload - BARCODE_NON_PAYLOAD_SIZE;

            // Include format identifier
            m_cbPayload += 1;
            m_pbPayload = new BYTE[m_cbPayload];

            CopyMemory(m_pbPayload, pbPayload + 1, m_cbPayload);
            m_tnf = NDEFRECORD_TNF_NFCWELLKNOWN;
            break;
        }
        default:
        {
            m_cbPayload = cbPayload;
            m_pbPayload = new BYTE[m_cbPayload];
            CopyMemory(m_pbPayload, pbPayload, m_cbPayload);
            m_tnf = 0xFF;
            break;
        }
    }

Done:
    return status;
}

NTSTATUS
CNFCProximityBuffer::InitializeRaw(
    _In_ USHORT cbRawNdef,
    _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    NFCSTATUS nfcStatus;
    UINT32 cRawRecords = MAX_RECORDS_PER_MESSAGE;
    UINT32 cbPayloadLength = 0;
    UCHAR IsChunked[MAX_RECORDS_PER_MESSAGE];
    UCHAR *RawRecords[MAX_RECORDS_PER_MESSAGE];
    phFriNfc_NdefRecord_t sNdefRecord[MAX_RECORDS_PER_MESSAGE];

    status = SetRawBuffer(cbRawNdef, pbRawNdef);

    if (!NT_SUCCESS(status)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    nfcStatus = phFriNfc_NdefRecord_GetRecords(m_pbBuffer,
                                               m_cbBuffer,
                                               RawRecords,
                                               IsChunked,
                                               &cRawRecords);

    // Now parse NDEF message, if it contains at least one valid record
    if ((nfcStatus != NFCSTATUS_SUCCESS) || (cRawRecords == 0)) {
        TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_GetRecords failed with status: %!NFCSTATUS!", nfcStatus);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    // Calling phFriNfc_NdefRecord_GetRecords to retrieve the cRawRecords is sufficient validation to
    // safely parse the first record. It will fail if the NDEF message isn't per NFC Forum standards.
    nfcStatus = phFriNfc_NdefRecord_Parse(&sNdefRecord[0], RawRecords[0]);

    if (nfcStatus != NFCSTATUS_SUCCESS) {
        TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_Parse failed with status: %!NFCSTATUS!", nfcStatus);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    m_tnf = sNdefRecord[0].Tnf & 0x07;
    m_cbSubTypeExt = sNdefRecord[0].TypeLength;
    m_pbSubTypeExt = sNdefRecord[0].Type;
    cbPayloadLength = sNdefRecord[0].PayloadLength;

    // The spec doesn't allow for more than one full payload in an NDEF message.
    // If there are multiple then we will use the first and discard the rest.
    if (IsChunked[0] == PHFRINFCNDEFRECORD_CHUNKBIT_SET)
    {
        for (UINT32 nNdefRecord = 1; nNdefRecord < cRawRecords; nNdefRecord++)
        {
            nfcStatus = phFriNfc_NdefRecord_Parse(&sNdefRecord[nNdefRecord], RawRecords[nNdefRecord]);
            if (nfcStatus != NFCSTATUS_SUCCESS) {
                TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_Parse failed with status: %!NFCSTATUS!", nfcStatus);
                status = STATUS_INVALID_DEVICE_REQUEST;
                goto Done;
            }

            status = RtlUInt32Add(cbPayloadLength, sNdefRecord[nNdefRecord].PayloadLength, &cbPayloadLength);
            if (!NT_SUCCESS(status)) {
                TRACE_LINE(LEVEL_ERROR, "Integer overflow collecting ndef records");
                goto Done;
            }

            if (IsChunked[nNdefRecord] == PHFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO) {
                break;
            }
        }
    }

    if (USHORT_MAX < cbPayloadLength) {
        TRACE_LINE(LEVEL_ERROR, "Parsed Ndef record too large");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    m_cbPayload = (USHORT)cbPayloadLength;
    m_pbPayload = new BYTE[cbPayloadLength];

    if (m_pbPayload == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    for (UINT32 nNdefRecord = 0, cbBytesCopied = 0; (nNdefRecord < cRawRecords) && (cbBytesCopied < m_cbPayload); nNdefRecord++) {
        memcpy_s(&m_pbPayload[cbBytesCopied], m_cbPayload - cbBytesCopied, sNdefRecord[nNdefRecord].PayloadData, sNdefRecord[nNdefRecord].PayloadLength);
        cbBytesCopied += sNdefRecord[nNdefRecord].PayloadLength;
    }

    ExtractPairingPayload(RawRecords, cRawRecords);

Done:
    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

NTSTATUS
CNFCProximityBuffer::SetRawBuffer(
    _In_ USHORT cbRawNdef,
    _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    m_pbBuffer = new BYTE[cbRawNdef];

    if (m_pbBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    m_cbBuffer = cbRawNdef;
    CopyMemory(m_pbBuffer, pbRawNdef, cbRawNdef);

Done:
    return status;
}

NTSTATUS
CNFCProximityBuffer::InitializeNdef(
    _In_ UCHAR  Tnf,
    _In_ UCHAR  cbType,
    _In_bytecount_(cbType) PCHAR  pbType,
    _In_ USHORT cbPayload,
    _In_bytecount_(cbPayload) PBYTE pbPayload
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    NFCSTATUS nfcStatus;
    phFriNfc_NdefRecord_t record = {};
    UINT32 cbWritten;

    record.Flags |= PH_FRINFC_NDEFRECORD_FLAGS_MB;
    record.Flags |= PH_FRINFC_NDEFRECORD_FLAGS_ME;
    record.Flags |= (cbPayload <= 255) ? PH_FRINFC_NDEFRECORD_FLAGS_SR : 0;

    record.Tnf           = Tnf;
    record.TypeLength    = cbType;
    record.Type          = (PBYTE)pbType;
    record.PayloadLength = cbPayload;
    record.PayloadData   = pbPayload;

    UINT32 cbBuffer = phFriNfc_NdefRecord_GetLength(&record);

    if (cbBuffer > USHORT_MAX) {
        status = STATUS_BUFFER_OVERFLOW;
        goto Done;
    }

    m_pbBuffer = new BYTE[cbBuffer];

    if (m_pbBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_LINE(LEVEL_ERROR, "Failed to allocate NDEF buffer");
        goto Done;
    }

    m_cbBuffer = (USHORT)cbBuffer;
    nfcStatus = phFriNfc_NdefRecord_Generate(&record, m_pbBuffer, m_cbBuffer, &cbWritten);

    if (nfcStatus != NFCSTATUS_SUCCESS) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_Generate failed status = %!NFCSTATUS!", nfcStatus);
        goto Done;
    }

Done:
    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}

#define IsStringPrefixed(_STRING_, _STRING_LENGTH_, _PREFIX_)                  \
    (CompareStringOrdinal(_STRING_,                                            \
                         (int)min((_PREFIX_ ## _CHARS), _STRING_LENGTH_),      \
                         _PREFIX_,                                             \
                         (_PREFIX_ ## _CHARS),                                 \
                         FALSE) == CSTR_EQUAL)

#define IsStringEqual(_STRING1_, _STRING2_) \
    (CompareStringOrdinal(_STRING1_, -1, _STRING2_, -1, FALSE) == CSTR_EQUAL)

NTSTATUS
CNFCProximityBuffer::AnalyzeMessageType(
    _In_z_ LPWSTR  pszMessageType,
    _Outptr_result_maybenull_z_ LPWSTR *ppszSubTypeExt,
    _In_ BOOL fPublication,
    _Out_ UCHAR *pchTNF,
    _Out_ TRANSLATION_TYPE_PROTOCOL *pTranslationType
    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    size_t cchMessageTypeLength;

    *ppszSubTypeExt = NULL;
    *pchTNF = 0;
    cchMessageTypeLength = wcslen(pszMessageType);
    if (IsStringPrefixed(pszMessageType, cchMessageTypeLength, NFC_BARCODE)) {
        TRACE_LINE(LEVEL_INFO, "NFC_BARCODE: %S", pszMessageType);
        *pTranslationType = TRANSLATION_TYPE_NFC_BARCODE;
        *pchTNF = 0xFF;
    }
    else if (IsStringPrefixed(pszMessageType, cchMessageTypeLength, WINDOWS_PROTOCOL)) {
        TRACE_LINE(LEVEL_INFO, "WINDOWS_PROTOCOL: %S", pszMessageType);
        LPWSTR pszNext = pszMessageType + WINDOWS_PROTOCOL_CHARS;
        cchMessageTypeLength -= WINDOWS_PROTOCOL_CHARS;

        if (*pszNext == L'.') {
            pszNext++;
            cchMessageTypeLength--;
            *ppszSubTypeExt = pszNext;
            *pTranslationType = TRANSLATION_TYPE_PAYLOAD_ONLY;
            *pchTNF = NDEFRECORD_TNF_ABSURI;
        }
        else if (*pszNext == L':') {
            pszNext++;
            cchMessageTypeLength--;
            if (fPublication && IsStringPrefixed(pszNext, cchMessageTypeLength, WRITETAG)) {
                pszNext = pszNext + WRITETAG_CHARS;
                if (*pszNext == L'.') {
                    pszNext++;
                    *ppszSubTypeExt = pszNext;
                    *pTranslationType = TRANSLATION_TYPE_PAYLOAD_ONLY_WRITETAG;
                    *pchTNF = NDEFRECORD_TNF_ABSURI;
                }
            }
        }
        else if (IsStringPrefixed(pszNext, cchMessageTypeLength, WINDOWS_URI)) {
            TRACE_LINE(LEVEL_INFO, "WINDOWS_URI");
            pszNext = pszNext + WINDOWS_URI_CHARS;
            cchMessageTypeLength -= WINDOWS_URI_CHARS;

            if (*pszNext == L'\0') {
                *pTranslationType = TRANSLATION_TYPE_WINDOWSURI;
                *pchTNF = NDEFRECORD_TNF_NFCWELLKNOWN;
            }
            else if (*pszNext == L':') {
                pszNext++;
                cchMessageTypeLength--;
                if (fPublication && IsStringEqual(pszNext, WRITETAG)) {
                    *pTranslationType = TRANSLATION_TYPE_WINDOWSURI_WRITETAG;
                    *pchTNF = NDEFRECORD_TNF_NFCWELLKNOWN;
                }
            }
        }
        else if (IsStringPrefixed(pszNext, cchMessageTypeLength, WINDOWS_MIME)) {
            TRACE_LINE(LEVEL_INFO, "WINDOWS_MIME");
            pszNext = pszNext + WINDOWS_MIME_CHARS;
            cchMessageTypeLength -= WINDOWS_MIME_CHARS;
            if (!fPublication && (*pszNext == L'\0')) {
                *pTranslationType = TRANSLATION_TYPE_WINDOWSMIME_MATCH_ALL;
                *pchTNF = NDEFRECORD_TNF_MEDIATYPE;
            }
            else if (fPublication && (*pszNext == L':')) {
                pszNext++;
                cchMessageTypeLength--;
                if (IsStringPrefixed(pszNext, cchMessageTypeLength, WRITETAG)) {
                    pszNext = pszNext + WRITETAG_CHARS;
                    if (*pszNext == L'.') {
                        pszNext++;
                        cchMessageTypeLength--;
                        *ppszSubTypeExt = pszNext;
                        *pTranslationType = TRANSLATION_TYPE_PAYLOAD_ONLY_WRITETAG;
                        *pchTNF = NDEFRECORD_TNF_MEDIATYPE;
                    }
                }
            }
            else if (*pszNext == L'.') {
                pszNext++;
                cchMessageTypeLength--;
                *ppszSubTypeExt = pszNext;
                *pTranslationType = TRANSLATION_TYPE_PAYLOAD_ONLY;
                *pchTNF = NDEFRECORD_TNF_MEDIATYPE;
            }
        }
    }
    else if (IsStringPrefixed(pszMessageType, cchMessageTypeLength, NDEF_PROTOCOL)) {
        TRACE_LINE(LEVEL_INFO, "NDEF_PROTOCOL %S", pszMessageType);
        // Protocol component of message type begins with NDEF
        LPWSTR pszNext = pszMessageType + NDEF_PROTOCOL_CHARS;
        cchMessageTypeLength -= NDEF_PROTOCOL_CHARS;
        if (*pszNext == L'\0') {
            // Protocol component of message type is NDEF
            // Sub Protocol component is absent i.e. no colon (:) character
            *pTranslationType = TRANSLATION_TYPE_NDEF;
            *pchTNF = 0xFF;
        }
        else if (*pszNext == L':') {
            pszNext++;
            cchMessageTypeLength--;
            // As per spec (Sec 3.3.2)Subscriptions or Publications with this type
            // MUST be rejected by the proximity provider driver with STATUS_INVALID_PARAMETER
            if (IsStringEqual(pszNext, NDEF_EMPTY)) {
                status = STATUS_INVALID_PARAMETER;
            }

            // Sub Protocol component is also present after colon (:) character
            if (fPublication) {
                if (IsStringEqual(pszNext, WRITETAG)) {
                    *pTranslationType = TRANSLATION_TYPE_RAW_NDEF_WRITETAG;
                    *pchTNF = 0xFF;
                }
            }
            else {
                LPWSTR pszTnfType = pszNext;
                *pTranslationType = TRANSLATION_TYPE_RAW_NDEF;

                if (IsStringPrefixed(pszTnfType, cchMessageTypeLength, NDEF_EXT)) {
                    *pchTNF = NDEFRECORD_TNF_NFCEXT;
                    *ppszSubTypeExt = pszTnfType + NDEF_EXT_CHARS;
                }
                else if(IsStringPrefixed(pszTnfType, cchMessageTypeLength, NDEF_MIME)) {
                    *pchTNF = NDEFRECORD_TNF_MEDIATYPE;
                    *ppszSubTypeExt = pszTnfType + NDEF_MIME_CHARS;
                }
                else if(IsStringPrefixed(pszTnfType, cchMessageTypeLength, NDEF_URI)) {
                    *pchTNF = NDEFRECORD_TNF_ABSURI;
                    *ppszSubTypeExt = pszTnfType + NDEF_URI_CHARS;
                }
                else if(IsStringPrefixed(pszTnfType, cchMessageTypeLength, NDEF_WKT)) {
                    *pchTNF = NDEFRECORD_TNF_NFCWELLKNOWN;
                    *ppszSubTypeExt = pszTnfType + NDEF_WKT_CHARS;
                }
                else if (IsStringEqual(pszTnfType, NDEF_UNKNOWN)) {
                    *pchTNF = NDEFRECORD_TNF_UNKNOWN;
                }
            }
        }
    }
    else if (fPublication)
    {
        if (IsStringEqual(pszMessageType, LAUNCH_APP_WRITETAG)) {
            TRACE_LINE(LEVEL_INFO, "LAUNCH_APP_WRITETAG");
            *pTranslationType = TRANSLATION_TYPE_LAUNCH_APP_WRITETAG;
            *pchTNF = 0xFF;
        }
        else if (IsStringEqual(pszMessageType, SETTAG_READONLY)) {
            TRACE_LINE(LEVEL_INFO, "SETTAG_READONLY");
            *pTranslationType = TRANSLATION_TYPE_SETTAG_READONLY;
            *pchTNF = 0xFF;
        }
    }
    else if (!fPublication)
    {
        if (IsStringEqual(pszMessageType, DEVICE_ARRIVED)) {
            *pTranslationType = TRANSLATION_TYPE_ARRIVAL;
            *pchTNF = 0xFF;
        }
        else if (IsStringEqual(pszMessageType, DEVICE_DEPARTED)) {
            *pTranslationType = TRANSLATION_TYPE_REMOVAL;
            *pchTNF = 0xFF;
        }
        else if (IsStringEqual(pszMessageType, PAIRING_BLUETOOTH_PROTOCOL)) {
            *pTranslationType = TRANSLATION_TYPE_PAIRING_BLUETOOTH;
            *pchTNF = 0xFF;
        }
        else if (IsStringEqual(pszMessageType, PAIRING_UPNP_PROTOCOL)) {
            *pTranslationType = TRANSLATION_TYPE_PAYLOAD_ONLY;
            *pchTNF = NDEFRECORD_TNF_MEDIATYPE;
            *ppszSubTypeExt = UPNP_PAIRING_MIME_TYPE;
        }
        else if (IsStringEqual(pszMessageType, WRITEABLE_TAG)) {
            *pTranslationType = TRANSLATION_TYPE_WRITABLETAG_SIZE;
            *pchTNF = 0xFF;
        }
    }

    if ((NT_SUCCESS(status)) && (*pchTNF == 0)) {
        // Unrecognized protocol MUST complete with STATUS_OBJECT_PATH_NOT_FOUND
        status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    TRACE_FUNCTION_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

BOOL
CNFCProximityBuffer::MatchesSubscription(
    _In_ TRANSLATION_TYPE_PROTOCOL translationType,
    _In_ UCHAR tnf,
    _In_ UCHAR cbType,
    _In_bytecount_(cbType) PCHAR pbType
    )
{
    if (tnf == m_tnf) {
        //
        // If the proximity technology is advertised as NFC,
        // then the driver MUST treat subscriptions for the WindowsUri Type as
        // equivalent to a subscription for the URI payload within an NDEF.wkt:U message
        // The driver MUST NOT match a WindowsUri subscription with the URI payload within
        // an NDEF:wkt.Sp message
        //
        if (translationType == TRANSLATION_TYPE_WINDOWSURI) {
            if ( m_cbSubTypeExt >= 1 &&
                 m_pbSubTypeExt != NULL &&
                 ((memcmp(m_pbSubTypeExt, "U", 1) == 0)
                    || (memcmp(m_pbSubTypeExt, "Sp", 2) == 0))) {
                return TRUE;
            }
            else if (m_Barcode) {
                return TRUE;
            }
        }
        else if (translationType == TRANSLATION_TYPE_WINDOWSMIME_MATCH_ALL) {
            //
            // If the proximity technology is advertised as NFC, then the driver MUST match
            // subscriptions for WindowsMime with all NDEF Messages that have a TNF field value of 0x02
            //
            return TRUE;
        }
        else if (m_tnf == PH_FRINFC_NDEFRECORD_TNF_UNKNOWN) {
            //
            // For Unknown type TNF value should be 0x05 TYPE_LENGTH field MUST be zero
            // and thus the TYPE field is omitted from the NDEF record.
            //
            return TRUE;
        }
        else if (cbType == m_cbSubTypeExt && memcmp(pbType, m_pbSubTypeExt, m_cbSubTypeExt) == 0) {
            return TRUE;
        }
    }
    else if (TRANSLATION_TYPE_NFC_BARCODE == translationType)
    {
        return m_Barcode;
    }
    else if (TRANSLATION_TYPE_NDEF == translationType) {
        if (!m_Barcode) {
            // Translation type for subscription of Subs\\NDEF
            return TRUE;
        }
    }
    else if ((translationType == TRANSLATION_TYPE_PAIRING_BLUETOOTH) &&
             (m_cbPairingBtPayload >= BLUETOOTH_PAIRING_MIME_MIN_OOB_SIZE) &&
             (m_pbPairingBtPayload != NULL)) {
        // Translation type for subscription of Pairing:Bluetooth
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
CNFCProximityBuffer::InitializeWithMessagePayload(
    _In_ UCHAR  Tnf,
    _In_ TRANSLATION_TYPE_PROTOCOL translationType,
    _In_ UCHAR  cbType,
    _In_bytecount_(cbType) PCHAR pbType,
    _In_ USHORT cbPayload,
    _In_bytecount_(cbPayload) PBYTE pbPayload
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    m_tnf = Tnf;

    if (cbPayload > 0)
    {
        switch (translationType)
        {
        case TRANSLATION_TYPE_LAUNCH_APP_WRITETAG:
            status = InitializeLaunchAppMessage(cbPayload/sizeof(WCHAR), (LPWSTR)pbPayload);
            break;

        case TRANSLATION_TYPE_NDEF:
        case TRANSLATION_TYPE_RAW_NDEF:
        case TRANSLATION_TYPE_RAW_NDEF_WRITETAG:
            // The provided buffer should already be encoded as RAW NDEF
            status = ValidateNdefMessage((USHORT)cbPayload, pbPayload);
            if (NT_SUCCESS(status)) {
                status = SetRawBuffer(cbPayload, pbPayload);
            }
            break;

        case TRANSLATION_TYPE_PAIRING_BLUETOOTH:
            // This case isn't required per spec. "Pairing:Bluetooth" is currently only
            // supported for subscriptions
            status = STATUS_INVALID_PARAMETER;
            break;

        case TRANSLATION_TYPE_PAYLOAD_ONLY:
        case TRANSLATION_TYPE_PAYLOAD_ONLY_WRITETAG:
            // The buffer is only the payload, encode it as NDEF now
            status = InitializeNdef(Tnf, cbType, pbType, cbPayload, pbPayload);
            break;

        case TRANSLATION_TYPE_WINDOWSMIME_MATCH_ALL:
             // The below shouldn't be allowed per spec. "WindowsMime" is only valid for subscriptions
             // This should be an ASSERT, because this case should never happen.
             status = STATUS_INVALID_PARAMETER;
             break;

        case TRANSLATION_TYPE_WINDOWSURI:
        case TRANSLATION_TYPE_WINDOWSURI_WRITETAG:
            {
                // The client buffer is UTF-16LE WITHOUT the NULL-terminator
                // It needs to be encoded as per NFCForum-TS-RTD_URI_1.0.pdf
                // WideCharToMultiByte(CP_UTF8, ...) with a specific size ensures that the result is NOT NULL-terminated
                if ((cbPayload == 0) || (cbPayload > INTERNET_MAX_URL_LENGTH * sizeof(WCHAR)) || (cbPayload % sizeof(WCHAR) != 0)) {
                    TRACE_LINE(LEVEL_ERROR, "Unexpected size for the payload (Type = %d)", translationType);
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

                // A UTF8 string can (in the worst case) be 150% the size of a UTF16 string
                UINT32 cbEncodedUri = (cbPayload * 2) + 50;
                BYTE *pbEncodedUri = new BYTE[cbEncodedUri];

                if (pbEncodedUri == NULL) {
                   status = STATUS_INSUFFICIENT_RESOURCES;
                   TRACE_LINE(LEVEL_ERROR, "Failed to allocate memory, %!STATUS!, (Type = %d)", status, translationType);
                   break;
                }

                UINT32 cchPayload = (UINT32)(cbPayload / sizeof(WCHAR));
                LPWSTR pszPayload = (LPWSTR)pbPayload;

                pbEncodedUri[0] = FindUriPrefixKey(pszPayload, cchPayload);
                UINT32 cbEncoded = WideCharToMultiByte(CP_UTF8, 0, pszPayload, cchPayload,
                                                     (LPSTR)pbEncodedUri + 1, cbEncodedUri - 1, NULL, NULL);
                if (cbEncoded == 0) {
                   delete [] pbEncodedUri;
                   status = NTSTATUS_FROM_WIN32(GetLastError());
                   break;
                }

                cbEncoded++; // Add one byte for the URI Identifier Code Prefix
                status = InitializeNdef(PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN,
                                        ARRAYSIZE("U")-1, // subtract 1 to remove NULL
                                        "U",
                                        (USHORT)cbEncoded,
                                        pbEncodedUri);

                delete [] pbEncodedUri;
            }
            break;

       case TRANSLATION_TYPE_SETTAG_READONLY:
           status = STATUS_INVALID_PARAMETER;
           break;

       default:
           status = STATUS_INVALID_DEVICE_STATE;
           TRACE_LINE(LEVEL_ERROR, "Unknown translation type (Type = %d)", translationType);
           break;
       }
    }
    else if (translationType != TRANSLATION_TYPE_SETTAG_READONLY) {
       status = STATUS_INVALID_PARAMETER;
       TRACE_LINE(LEVEL_ERROR, "Empty payload");
    }

    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

NTSTATUS
CNFCProximityBuffer::GetMessagePayload(
    _In_ TRANSLATION_TYPE_PROTOCOL translationType,
    _Out_ USHORT *pcbPayload,
    _Outptr_result_buffer_maybenull_(*pcbPayload) PBYTE *ppbPayload
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;

    *ppbPayload = NULL;
    *pcbPayload = 0;

    switch (translationType)
    {
    case TRANSLATION_TYPE_PAIRING_BLUETOOTH:
        *pcbPayload = m_cbPairingBtPayload;
        *ppbPayload = m_pbPairingBtPayload;
        break;

    case TRANSLATION_TYPE_PAYLOAD_ONLY:
        // "Windows" and "WindowsMime": only return the NdefPayload to client
        *pcbPayload = m_cbPayload;
        *ppbPayload = m_pbPayload;
        break;

    case TRANSLATION_TYPE_WINDOWSMIME_MATCH_ALL:
        if (m_pbWindowsMimeBuffer == NULL)
        {
            if (m_cbSubTypeExt >= WINDOWSMIME_MIME_TYPE_LENGTH) {
                TRACE_LINE(LEVEL_ERROR, "Invalid NDEF type (Type = %d)", translationType);
                return STATUS_INVALID_PARAMETER;
            }

            m_cbWindowsMimeBuffer = WINDOWSMIME_MIME_TYPE_LENGTH + m_cbPayload;
            m_pbWindowsMimeBuffer = new BYTE[m_cbWindowsMimeBuffer];

            // Copy the MIME type (will be NULL terminated)
            status = NTSTATUS_FROM_STRSAFE_HRESULT(StringCchCopyNA((LPSTR)m_pbWindowsMimeBuffer,
                                                                   WINDOWSMIME_MIME_TYPE_LENGTH,
                                                                   (LPSTR)m_pbSubTypeExt,
                                                                   m_cbSubTypeExt));

            if (NT_SUCCESS(status)) {
                memset(m_pbWindowsMimeBuffer + m_cbSubTypeExt, 0, WINDOWSMIME_MIME_TYPE_LENGTH - m_cbSubTypeExt);
                memcpy(m_pbWindowsMimeBuffer + WINDOWSMIME_MIME_TYPE_LENGTH, m_pbPayload, m_cbPayload);
            }
        }

        *ppbPayload = m_pbWindowsMimeBuffer;
        *pcbPayload = m_cbWindowsMimeBuffer;
        break;

    case TRANSLATION_TYPE_WINDOWSURI:
        if (m_cbPayload > 0)
        {
            PBYTE pbPayload = m_pbPayload;
            UINT32 cbPayload = m_cbPayload;

            // NdefPayload is encoded as per NFCForum-TS-RTD_URI_1.0.pdf.
            // It MUST be decoded to a client buffer as NULL-terminated UTF-16LE

            if (!m_Barcode && 0 == memcmp(m_pbSubTypeExt, "Sp", 2))
            {
                BOOL fMatchUriRecord = FALSE;
                UINT32 cRawRecords = MAX_RECORDS_PER_MESSAGE;
                UCHAR IsChunked[MAX_RECORDS_PER_MESSAGE];
                UCHAR *RawRecords[MAX_RECORDS_PER_MESSAGE];
                phFriNfc_NdefRecord_t sNdefRecord;

                // Record is of type NDEF:wkt.Sp
                // For this record, the payload will contain an encapsulated record of type NDEF:wkt.U
                // Hence, we must again parse the payload we have thus far obtained  to get the subsequent records

                NFCSTATUS nfcStatus = phFriNfc_NdefRecord_GetRecords(pbPayload,
                                                                     cbPayload,
                                                                     RawRecords,
                                                                     IsChunked,
                                                                     &cRawRecords);

                if ((nfcStatus != NFCSTATUS_SUCCESS) || (cRawRecords == 0)) {
                    TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_GetRecords failed with status: %!NFCSTATUS!", nfcStatus);
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

                for (UCHAR nNdefRecord = 0; (nNdefRecord < MAX_RECORDS_PER_MESSAGE) && (nNdefRecord < cRawRecords); nNdefRecord++)
                {
                    nfcStatus = phFriNfc_NdefRecord_Parse(&sNdefRecord, RawRecords[nNdefRecord]);

                    if (NFCSTATUS_SUCCESS != nfcStatus) {
                        TRACE_LINE(LEVEL_ERROR, "phFriNfc_NdefRecord_Parse failed with status: %!NFCSTATUS!", nfcStatus);
                        return STATUS_INVALID_PARAMETER;
                    }

                    fMatchUriRecord = (0 == memcmp(sNdefRecord.Type, "U", 1));

                    if (fMatchUriRecord) {
                        pbPayload = sNdefRecord.PayloadData;
                        cbPayload = sNdefRecord.PayloadLength;
                        break;
                    }
                }

                if (!fMatchUriRecord) {
                    TRACE_LINE(LEVEL_ERROR, "No URI record found inside the smart poster. Invalid message");
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }
            if (pbPayload[0] == BARCODE_TYPE_EPC_URI && m_Barcode) {
                status = CreateEPCRawUri(cbPayload, pbPayload);
                if (status != STATUS_SUCCESS) {
                    TRACE_LINE(LEVEL_ERROR, "Insufficient buffer (Type = %d)", translationType);
                    break;
                }
            }
            else {
                // First byte of URI payload is a prefix ID
                UCHAR prefixId = FindUriPrefixIndex(pbPayload[0]);
                UINT32 cchPrefix = (UINT32)(prefixId == 0 ? 0 : wcslen(PrefixTable[prefixId].Prefix));
                UINT32 cchSuffix = 0;

                // If the contents of this field is zero (0x00), then NO prepending SHALL be done.
                // All fields marked RFU SHALL be treated as if they were value zero
                // (no perpending). A compliant system MUST NOT produce values that are marked RFU.
                if (cchPrefix > 0) {
                    status = NTSTATUS_FROM_STRSAFE_HRESULT(StringCchCopyW(m_szDecodedUri,
                                                                          ARRAYSIZE(m_szDecodedUri),
                                                                          PrefixTable[prefixId].Prefix));
                    NT_ASSERT(status == STATUS_SUCCESS);
                }

                // Convert to UTF-16LE using MultiByteToWideChar(CP_UTF8, ...) and make sure the result IS NULL-terminated
                // Also decode correct URI Identifier Code prefixes
                cchSuffix = MultiByteToWideChar(CP_UTF8,
                                                0,
                                                (PCSTR)(pbPayload + 1),
                                                cbPayload - 1,
                                                m_szDecodedUri + cchPrefix,
                                                ARRAYSIZE(m_szDecodedUri) - cchPrefix);
                if (cchSuffix == 0) {
                    TRACE_LINE(LEVEL_ERROR, "Insufficient buffer (Type = %d)", translationType);
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                // Append NULL terminating char
                UINT32 cchDecodedUri = cchPrefix + cchSuffix;

                if (cchDecodedUri >= ARRAYSIZE(m_szDecodedUri)) {
                    TRACE_LINE(LEVEL_ERROR, "Insufficient buffer (Type = %d)", translationType);
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                m_szDecodedUri[cchDecodedUri] = L'\0';
            }

            *pcbPayload = (USHORT)((wcslen(m_szDecodedUri) + 1) * sizeof(WCHAR));
            *ppbPayload = (PBYTE)m_szDecodedUri;
        }
        break;
    case TRANSLATION_TYPE_NFC_BARCODE:
        *pcbPayload = m_cbBuffer;
        *ppbPayload = m_pbBuffer;
        break;
    case TRANSLATION_TYPE_NDEF:
    case TRANSLATION_TYPE_RAW_NDEF:
        // NDEF protocol returns RAW NDEF to client
        *pcbPayload = m_cbBuffer;
        *ppbPayload = m_pbBuffer;
        break;
    default:
        TRACE_LINE(LEVEL_ERROR, "Invalid translation type (Type = %d)", translationType);
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);

    return status;
}

UCHAR
CNFCProximityBuffer::FindUriPrefixIndex(
    _In_ UCHAR Prefix
    )
{
    UCHAR i;

    for (i = 0; i < ARRAYSIZE(PrefixTable); i++)
    {
        if (PrefixTable[i].PrefixKey == Prefix) {
            return i;
        }
    }

    return 0;
}

UCHAR
CNFCProximityBuffer::FindUriPrefixKey(
    _Inout_count_(cchPayload) LPWSTR & pszPayload,
    _Inout_ UINT32 & cchPayload
    )
{
    UCHAR i;

    for (i = 0; i < ARRAYSIZE(PrefixTable); i++) {
       if (CompareStringOrdinal(pszPayload,
                                min(((UINT32)wcslen(PrefixTable[i].Prefix)), cchPayload),
                                PrefixTable[i].Prefix,
                                ((UINT32)wcslen(PrefixTable[i].Prefix)),
                                FALSE) == CSTR_EQUAL) {
           USHORT prefixLen = ((USHORT)wcslen(PrefixTable[i].Prefix));
           pszPayload = pszPayload + prefixLen;
           cchPayload = cchPayload - prefixLen;
           return PrefixTable[i].PrefixKey;
       }
    }

   return 0;
}

NTSTATUS
CNFCProximityBuffer::CreateEPCRawUri(
    _In_ UINT32 cchPayload,
    _In_count_(cchPayload) PBYTE pbPayload
    )
{
    NTSTATUS status = NTSTATUS_FROM_STRSAFE_HRESULT(StringCchCopyW(m_szDecodedUri,
                                                                   ARRAYSIZE(m_szDecodedUri),
                                                                   NFC_BARCODE_URI_PREFIX));
    NT_ASSERT(status == STATUS_SUCCESS);
    if (status == STATUS_SUCCESS)
    {
        size_t remaining = 0;
        LPWSTR buf = (LPWSTR)(m_szDecodedUri + NFC_BARCODE_URI_PREFIX_CHARS);
        status = NTSTATUS_FROM_STRSAFE_HRESULT(StringCchPrintfExW(buf,
                                                                  ARRAYSIZE(m_szDecodedUri) - NFC_BARCODE_URI_PREFIX_CHARS,
                                                                  &buf,
                                                                  &remaining,
                                                                  STRSAFE_NO_TRUNCATION,
                                                                  L"%d.x",
                                                                  (cchPayload - 1) * 8));
        NT_ASSERT(status == STATUS_SUCCESS);
        for (UCHAR i = 1; i < cchPayload; i++) {
            status = NTSTATUS_FROM_STRSAFE_HRESULT(StringCchPrintfExW(buf,
                                                                      remaining,
                                                                      &buf,
                                                                      &remaining,
                                                                      STRSAFE_NO_TRUNCATION,
                                                                      L"%02X",
                                                                      pbPayload[i]));
            NT_ASSERT(status == STATUS_SUCCESS);
            if (status != STATUS_SUCCESS) {
                break;
            }
        }

        if (status == STATUS_SUCCESS) {
            *buf = L'\0';
        }
    }

    return status;
}


#define IsConnectionHandoverSelectRecord()                          \
    ((m_tnf == NDEFRECORD_TNF_NFCWELLKNOWN) &&                      \
     (m_cbSubTypeExt == 2) &&                                       \
     (m_pbSubTypeExt[0] == 'H') && (m_pbSubTypeExt[1] == 's') &&    \
     (m_cbPayload > 2) && ((m_pbPayload[0] & 0xF0) == 0x10))

#define IsAlternateCarrierRecord(_Record_)                          \
    ((_Record_.Tnf == NDEFRECORD_TNF_NFCWELLKNOWN) &&               \
     (_Record_.TypeLength == 2) &&                                  \
     (_Record_.Type[0] == 'a') && (_Record_.Type[1] == 'c') &&      \
     (_Record_.PayloadLength >= 3))

#define IsBTAlternateCarrierRecord(_Record_, _Id_, _IdLength_)      \
    ((_Record_.Tnf == NDEFRECORD_TNF_MEDIATYPE) &&                  \
     (_Record_.TypeLength == BLUETOOTH_PAIRING_MIME_TYPE_CHARS) &&  \
     (_Record_.IdLength == _IdLength_) &&                           \
     (memcmp(_Record_.Id, _Id_, _IdLength_) == 0) &&                \
     (memcmp(_Record_.Type, BLUETOOTH_PAIRING_MIME_TYPE, BLUETOOTH_PAIRING_MIME_TYPE_CHARS) == 0 || \
      memcmp(_Record_.Type, BLUETOOTH_LE_PAIRING_MIME_TYPE, BLUETOOTH_PAIRING_MIME_TYPE_CHARS) == 0) && \
     (_Record_.PayloadLength >= BLUETOOTH_PAIRING_MIME_MIN_OOB_SIZE) && \
     (_Record_.PayloadLength < USHORT_MAX))

#define IsBTSimplifiedTagFormat()                                   \
    ((m_tnf == NDEFRECORD_TNF_MEDIATYPE) &&                         \
     (m_cbSubTypeExt == BLUETOOTH_PAIRING_MIME_TYPE_CHARS) &&       \
     (memcmp(m_pbSubTypeExt, BLUETOOTH_PAIRING_MIME_TYPE, BLUETOOTH_PAIRING_MIME_TYPE_CHARS) == 0 || \
      memcmp(m_pbSubTypeExt, BLUETOOTH_LE_PAIRING_MIME_TYPE, BLUETOOTH_PAIRING_MIME_TYPE_CHARS) == 0) && \
     (m_cbPayload >= BLUETOOTH_PAIRING_MIME_MIN_OOB_SIZE))

#define IsNokiaBTPairingFormat()                                    \
    ((m_tnf == NDEFRECORD_TNF_NFCEXT) &&                            \
     (m_cbSubTypeExt == NOKIA_BLUETOOTH_PAIRING_TYPE_CHARS) &&      \
     (memcmp(m_pbSubTypeExt, NOKIA_BLUETOOTH_PAIRING_TYPE, NOKIA_BLUETOOTH_PAIRING_TYPE_CHARS) == 0) && \
     (m_cbPayload >= 27))

void
CNFCProximityBuffer::ExtractPairingPayload(
    _In_count_(cRawRecords) UCHAR *rgpRawRecords[],
    _In_ UINT32 cRawRecords
    )
{
    if ((cRawRecords > 1) &&
        (cRawRecords < MAX_RECORDS_PER_MESSAGE) &&
        IsConnectionHandoverSelectRecord())
    {
        UINT32 cAlternativeCarrierRecords = MAX_RECORDS_PER_MESSAGE;
        UCHAR *rgpAlternativeCarrierRecords[MAX_RECORDS_PER_MESSAGE];
        UCHAR  rgIsChunked[MAX_RECORDS_PER_MESSAGE] = {0};

        NFCSTATUS status = phFriNfc_NdefRecord_GetRecords(m_pbPayload + 1,
                                                          m_cbPayload - 1,
                                                          rgpAlternativeCarrierRecords,
                                                          rgIsChunked,
                                                          &cAlternativeCarrierRecords);

        for (UCHAR i = 0; (m_pbPairingBtPayload == NULL) && (i < cAlternativeCarrierRecords) && (status == NFCSTATUS_SUCCESS); i++)
        {
            phFriNfc_NdefRecord_t sAlternativeCarrierRecord = {};
            status = phFriNfc_NdefRecord_Parse(&sAlternativeCarrierRecord, rgpAlternativeCarrierRecords[i]);

            if (status != NFCSTATUS_SUCCESS || !IsAlternateCarrierRecord(sAlternativeCarrierRecord)) {
                continue;
            }

            for (UCHAR j = 1; (j < cRawRecords) && (status == NFCSTATUS_SUCCESS); j++)
            {
                phFriNfc_NdefRecord_t sCarrierDataRecord = {};
                status = phFriNfc_NdefRecord_Parse(&sCarrierDataRecord, rgpRawRecords[j]);

                if (status != NFCSTATUS_SUCCESS ||
                    !IsBTAlternateCarrierRecord(sCarrierDataRecord,
                                                sAlternativeCarrierRecord.PayloadData + 2,
                                                sAlternativeCarrierRecord.PayloadData[1])) {
                    continue;
                }

                m_cbPairingBtPayload = (USHORT)sCarrierDataRecord.PayloadLength;
                m_pbPairingBtPayload = sCarrierDataRecord.PayloadData;
                break;
            }
        }
    } else if (IsBTSimplifiedTagFormat()) {

        m_cbPairingBtPayload = m_cbPayload;
        m_pbPairingBtPayload = m_pbPayload;

    } else if (IsNokiaBTPairingFormat()) {

        m_cbPairingBtPayload = 8;
        m_pbPairingBtPayload = m_rgBuffer;

        // Initialize size to zero (we'll fill in size at the end)
        m_pbPairingBtPayload[0] = 0;
        m_pbPairingBtPayload[1] = 0;

        // Copy BTH Address
        m_pbPairingBtPayload[2] = m_pbPayload[6];
        m_pbPairingBtPayload[3] = m_pbPayload[5];
        m_pbPairingBtPayload[4] = m_pbPayload[4];
        m_pbPairingBtPayload[5] = m_pbPayload[3];
        m_pbPairingBtPayload[6] = m_pbPayload[2];
        m_pbPairingBtPayload[7] = m_pbPayload[1];

        UCHAR *pbNext = m_pbPairingBtPayload + 8;

        // Copy Class of Device
        m_cbPairingBtPayload += 5;
        pbNext[0] = 0x04;
        pbNext[1] = 0x0D;
        pbNext[2] = m_pbPayload[9];
        pbNext[3] = m_pbPayload[8];
        pbNext[4] = m_pbPayload[7];

        pbNext += 5;

        // Copy Name
        m_cbPairingBtPayload += (2 + m_pbPayload[26]);
        pbNext[0] = 1 + m_pbPayload[26];
        pbNext[1] = 0x08;
        pbNext += 2;

        // Truncate the Short Name Length if it goes past the Payload length
        if (m_cbPayload < (27 + m_pbPayload[26])) {
            m_pbPayload[26] = (BYTE)(m_cbPayload - 27);
        }

        CopyMemory(pbNext, m_pbPayload + 27, m_pbPayload[26]);

        // Total size
        m_pbPairingBtPayload[0] = (m_cbPairingBtPayload & 0xFF);
        m_pbPairingBtPayload[1] = (m_cbPairingBtPayload >> 8);
    }
}

NTSTATUS
CNFCProximityBuffer::InitializeLaunchAppMessage(
    _In_ USHORT cchPayload,
    _In_count_(cchPayload) LPWSTR pwszPayload
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    UINT32 cbPayload = cchPayload * 2;
    PBYTE pbPayload = NULL;
    UINT32 cbEncoded = 0;
    PBYTE prgMessage = NULL;
    USHORT cbMessage = 0;
    CHAR *pszArgs = NULL;
    USHORT cbArgs;
    CHAR *pszContext = NULL;
    USHORT cPlatforms = 0;
    PCHAR ppszPlatform[LAUNCH_APP_MAX_PLATFORMS];
    PCHAR ppszAppId[LAUNCH_APP_MAX_PLATFORMS];

    if ((pwszPayload == NULL) || (cchPayload == 0)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    pbPayload = new BYTE[cbPayload];

    if (NULL == pbPayload) {
        TRACE_LINE(LEVEL_ERROR, "Memory Allocated Failed");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    cbEncoded = WideCharToMultiByte(CP_UTF8,
                                    0,
                                    pwszPayload,
                                    cchPayload,
                                    (LPSTR)pbPayload,
                                    cbPayload-1,
                                    NULL,
                                    NULL);

    if ((cbEncoded < LAUNCH_APP_MINIMUM_BYTES) ||
        (cbEncoded > LAUNCH_APP_MAXIMUM_BYTES)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    cbPayload = (USHORT)cbEncoded;
    pbPayload[cbPayload++] = '\0';

    pszArgs = strtok_s((CHAR*)pbPayload, "\t", &pszContext);

    if (pszArgs == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    for (cPlatforms = 0; cPlatforms < LAUNCH_APP_MAX_PLATFORMS; cPlatforms++)
    {
        ppszPlatform[cPlatforms] = strtok_s(NULL, "\t", &pszContext);

        if (ppszPlatform[cPlatforms] == NULL) {
            break;
        }

        if (strlen(ppszPlatform[cPlatforms]) == 0) {
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        ppszAppId[cPlatforms] = strtok_s(NULL, "\t", &pszContext);

        if (ppszAppId[cPlatforms] == NULL || strlen(ppszAppId[cPlatforms]) == 0) {
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }
    }

    if (cPlatforms == 0) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    prgMessage = new BYTE[LAUNCH_APP_MESSAGE_SIZE];

    if (prgMessage == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    prgMessage[cbMessage++] = 0x00;
    prgMessage[cbMessage++] = (BYTE)cPlatforms;

    for (USHORT i = 0; i < cPlatforms; i++) {
        UCHAR cbPlatform = (UCHAR)strlen(ppszPlatform[i]);
        prgMessage[cbMessage++] = cbPlatform;
        memcpy(prgMessage + cbMessage, ppszPlatform[i], cbPlatform);
        cbMessage += cbPlatform;

        UCHAR cbAppId = (UCHAR)strlen(ppszAppId[i]);
        prgMessage[cbMessage++] = cbAppId;
        memcpy(prgMessage + cbMessage, ppszAppId[i], cbAppId);
        cbMessage += cbAppId;
    }

    cbArgs = (USHORT)(strlen(pszArgs) & 0xFFFF);
    prgMessage[cbMessage++] = (UCHAR)(cbArgs >> 8);
    prgMessage[cbMessage++] = cbArgs & 0xFF;
    memcpy(prgMessage + cbMessage, pszArgs, cbArgs);
    cbMessage += cbArgs;

    status = InitializeNdef(PH_FRINFC_NDEFRECORD_TNF_ABSURI,
                            LAUNCH_APP_TYPE_CHARS,
                            LAUNCH_APP_TYPE,
                            (USHORT)cbMessage,
                            prgMessage);

Done:

    delete [] prgMessage;
    delete [] pbPayload;

    TRACE_METHOD_EXIT_NTSTATUS(LEVEL_VERBOSE, status);
    return status;
}
