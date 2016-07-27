/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcProximityBuffer.h

Abstract:

    Buffer parsing utilities declarations

Environment:

    User mode

--*/

#pragma once

// As per the spec Max length of file name is:
// "Pubs\\"(5) + Prtocol length(250)+"."(1) + SubType(250)+ '\0'(1)
#define MAX_FILE_NAME_LENGTH    507
#define CHAR_PROTOCOL_TERMINATOR L'.'

#define WINDOWS_PROTOCOL        L"Windows"
#define WINDOWS_PROTOCOL_CHARS  (ARRAYSIZE(WINDOWS_PROTOCOL) - 1)

#define WINDOWS_URI             L"Uri"
#define WINDOWS_URI_CHARS       (ARRAYSIZE(WINDOWS_URI) - 1)

#define WINDOWS_MIME            L"Mime"
#define WINDOWS_MIME_CHARS      (ARRAYSIZE(WINDOWS_MIME) - 1)

#define WRITETAG                L"WriteTag"
#define WRITETAG_CHARS          (ARRAYSIZE(WRITETAG) - 1)

#define SETTAG_READONLY         L"SetTagReadOnly"
#define SETTAG_READONLY_CHARS   (ARRAYSIZE(SETTAG_READONLY) - 1)

#define NDEF_PROTOCOL           L"NDEF"
#define NDEF_PROTOCOL_CHARS     (ARRAYSIZE(NDEF_PROTOCOL) - 1)

#define NDEF_UNKNOWN            L"Unknown"
#define NDEF_UNKNOWN_LENGTH     (ARRAYSIZE(NDEF_UNKNOWN) - 1)

#define NDEF_EMPTY              L"Empty"
#define NDEF_EMPTY_LENGTH       (ARRAYSIZE(NDEF_EMPTY) - 1)

#define NDEF_EXT                L"ext."
#define NDEF_EXT_CHARS          (ARRAYSIZE(NDEF_EXT) - 1)

#define NDEF_MIME               L"MIME."
#define NDEF_MIME_CHARS         (ARRAYSIZE(NDEF_MIME) - 1)

#define NDEF_URI                L"URI."
#define NDEF_URI_CHARS          (ARRAYSIZE(NDEF_URI) - 1)

#define NDEF_WKT                L"wkt."
#define NDEF_WKT_CHARS          (ARRAYSIZE(NDEF_WKT) - 1)

#define NFC_BARCODE             L"NfcBarcode"
#define NFC_BARCODE_CHARS       (ARRAYSIZE(NFC_BARCODE) - 1)

#define WRITEABLE_TAG           L"WriteableTag"

#define PAIRING_BLUETOOTH_PROTOCOL      L"Pairing:Bluetooth"
#define PAIRING_UPNP_PROTOCOL           L"Pairing:UPnP"

#define BLUETOOTH_PAIRING_MIME_TYPE         "application/vnd.bluetooth.ep.oob"
#define BLUETOOTH_LE_PAIRING_MIME_TYPE      "application/vnd.bluetooth.le.oob"

#define BLUETOOTH_PAIRING_MIME_TYPE_CHARS   (ARRAYSIZE(BLUETOOTH_PAIRING_MIME_TYPE) - 1)
#define BLUETOOTH_PAIRING_MIME_MIN_OOB_SIZE 8

C_ASSERT(BLUETOOTH_PAIRING_MIME_TYPE_CHARS == (ARRAYSIZE(BLUETOOTH_LE_PAIRING_MIME_TYPE)-1));

#define NOKIA_BLUETOOTH_PAIRING_TYPE        "nokia.com:bt"
#define NOKIA_BLUETOOTH_PAIRING_TYPE_CHARS  (ARRAYSIZE(NOKIA_BLUETOOTH_PAIRING_TYPE) - 1)

#define UPNP_PAIRING_MIME_TYPE              L"application/vnd.upnp."
#define UPNP_PAIRING_MIME_TYPE_CHARS        (ARRAYSIZE(UPNP_PAIRING_MIME_TYPE) - 1)

#define NDEFRECORD_TNF_EMPTY        ((uint8_t)0x00)  // Empty Record, no type, ID or payload present.
#define NDEFRECORD_TNF_NFCWELLKNOWN ((uint8_t)0x01)  // NFC well-known type (RTD).
#define NDEFRECORD_TNF_MEDIATYPE    ((uint8_t)0x02)  // Media Type.
#define NDEFRECORD_TNF_ABSURI       ((uint8_t)0x03)  // Absolute URI.
#define NDEFRECORD_TNF_NFCEXT       ((uint8_t)0x04)  // Nfc Extenal Type (following the RTD format).
#define NDEFRECORD_TNF_UNKNOWN      ((uint8_t)0x05)  // Unknown type; Contains no Type information.
#define NDEFRECORD_TNF_UNCHANGED    ((uint8_t)0x06)  // Unchanged: Used for Chunked Records.
#define NDEFRECORD_TNF_RESERVED     ((uint8_t)0x07)  // RFU, must not be used.

#define DEVICE_ARRIVED              L"DeviceArrived"
#define DEVICE_DEPARTED             L"DeviceDeparted"

#define LAUNCH_APP_WRITETAG         L"LaunchApp:WriteTag"

#define LAUNCH_APP_TYPE             "windows.com/LaunchApp"
#define LAUNCH_APP_TYPE_CHARS       (ARRAYSIZE(LAUNCH_APP_TYPE) - 1)

#define LAUNCH_APP_MESSAGE_SIZE         3050
#define LAUNCH_APP_MAXIMUM_BYTES        3000
#define LAUNCH_APP_MINIMUM_BYTES        5
#define LAUNCH_APP_MAX_PLATFORMS        20

#define MAX_RECORDS_PER_MESSAGE         100

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define INTERNET_MAX_URL_LENGTH         (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define WINDOWSMIME_MIME_TYPE_LENGTH    256

#define BARCODE_TYPE_MIN_LENGTH        16
#define BARCODE_TYPE_MAX_LENGTH         32
#define BARCODE_TYPE_URI_TERMINATOR    0xFE
#define BARCODE_NON_PAYLOAD_SIZE       0x4
#define BARCODE_TYPE_EPC_URI           0x5
#define NFC_BARCODE_URI_PREFIX         L"urn:epc:raw:"
#define NFC_BARCODE_URI_PREFIX_CHARS   (ARRAYSIZE(NFC_BARCODE_URI_PREFIX) - 1)

#define BARCODE_HTTP_WWW_TYPE          0x1
#define BARCODE_HTTPS_WWW_TYPE         0x2
#define BARCODE_HTTP_TYPE              0x3
#define BARCODE_HTTPS_TYPE             0x4
#define BARCODE_EPC_URN_TYPE           0x5

typedef enum _TRANSLATION_TYPE_PROTOCOL {
    TRANSLATION_TYPE_UNDEFINED,
    TRANSLATION_TYPE_ARRIVAL,
    TRANSLATION_TYPE_REMOVAL,
    TRANSLATION_TYPE_WRITABLETAG_SIZE,
    TRANSLATION_TYPE_PAYLOAD_ONLY,
    TRANSLATION_TYPE_PAYLOAD_ONLY_WRITETAG,
    TRANSLATION_TYPE_WINDOWSURI,
    TRANSLATION_TYPE_WINDOWSURI_WRITETAG,
    TRANSLATION_TYPE_NDEF,
    TRANSLATION_TYPE_RAW_NDEF,
    TRANSLATION_TYPE_RAW_NDEF_WRITETAG,
    TRANSLATION_TYPE_WINDOWSMIME_MATCH_ALL,
    TRANSLATION_TYPE_LAUNCH_APP_WRITETAG,
    TRANSLATION_TYPE_PAIRING_BLUETOOTH,
    TRANSLATION_TYPE_SETTAG_READONLY,
    TRANSLATION_TYPE_NFC_BARCODE
} TRANSLATION_TYPE_PROTOCOL;

class CNFCProximityBuffer
{
public:
    CNFCProximityBuffer()
        : m_pbBuffer(NULL),
          m_cbBuffer(0),
          m_pbSubTypeExt(NULL),
          m_cbSubTypeExt(0),
          m_pbPayload(NULL),
          m_cbPayload(0),
          m_pbPairingBtPayload(NULL),
          m_cbPairingBtPayload(0),
          m_pbWindowsMimeBuffer(NULL),
          m_cbWindowsMimeBuffer(0),
          m_Barcode(FALSE)
    {
        InitializeListHead(&m_ListEntry);
    }

    ~CNFCProximityBuffer()
    {
        if (m_pbBuffer != NULL) {
            delete [] m_pbBuffer;
            m_pbBuffer = NULL;
        }

        if (m_pbPayload != NULL) {
            delete [] m_pbPayload;
            m_pbPayload = NULL;
        }

        if (m_pbWindowsMimeBuffer != NULL) {
            delete [] m_pbWindowsMimeBuffer;
            m_pbWindowsMimeBuffer = NULL;
            m_cbWindowsMimeBuffer = 0;
        }
    }

    NTSTATUS
    InitializeBarcode(
            _In_ USHORT cbPayload,
            _In_bytecount_(cbPayload) PBYTE pbPayload
            );

    NTSTATUS
    InitializeRaw(
        _In_ USHORT cbPayload,
        _In_bytecount_(cbPayload) PBYTE pbPayload
        );

    NTSTATUS
    InitializeNdef(
        _In_ UCHAR Tnf,
        _In_ UCHAR cbType,
        _In_bytecount_(cbType) PCHAR pbType,
        _In_ USHORT cbPayload,
        _In_bytecount_(cbPayload) PBYTE pbPayload
        );

    NTSTATUS
    ValidateNdefMessage(
        _In_ USHORT cbRawNdef,
        _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
        );

    //
    // Converts the message type
    // to the corresponding NDEF TNF character and translation type protocol.
    //
    static NTSTATUS
    AnalyzeMessageType(
        _In_z_ LPWSTR  pszMessageType,
        _Outptr_result_maybenull_z_ LPWSTR *ppszSubTypeExt,
        _In_ BOOL fPublication,
        _Out_ UCHAR *pchTNF,
        _Out_ TRANSLATION_TYPE_PROTOCOL *pTranslationType
        );

    //
    // Initializes the object given a message payload and translation type protocol
    //
    NTSTATUS
    InitializeWithMessagePayload(
        _In_ UCHAR Tnf,
        _In_ TRANSLATION_TYPE_PROTOCOL translationType,
        _In_ UCHAR cbType,
        _In_bytecount_(cbType) PCHAR pbType,
        _In_ USHORT cbPayload,
        _In_bytecount_(cbPayload) PBYTE pbPayload
        );

    //
    // Gets the message payload from the object given the translation type protocol
    //
    NTSTATUS
    GetMessagePayload(
        _In_ TRANSLATION_TYPE_PROTOCOL translationType,
        _Out_ uint16_t *pcbPayload,
        _Outptr_result_buffer_maybenull_(*pcbPayload) PBYTE *ppbPayload
        );

    //
    // Returns whether the supplied subscription type information matches the message
    //
    BOOL
    MatchesSubscription(
        _In_ TRANSLATION_TYPE_PROTOCOL translationType,
        _In_ uint8_t tnf,
        _In_ uint8_t cbType,
        _In_bytecount_(cbType) PCHAR pbType
        );

    PBYTE Get()
    {
        return m_pbBuffer;
    }

    USHORT GetSize()
    {
        return m_cbBuffer;
    }

    PBYTE GetPayload()
    {
        return m_pbPayload;
    }

    USHORT GetPayloadSize()
    {
        return m_cbPayload;
    }

    UCHAR GetTnf()
    {
        return m_tnf;
    }

    PBYTE GetSubTypeExt()
    {
        return m_pbSubTypeExt;
    }

    USHORT GetSubTypeExtSize()
    {
        return m_cbSubTypeExt;
    }

    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }

    static CNFCProximityBuffer* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CNFCProximityBuffer*) CONTAINING_RECORD(pEntry, CNFCProximityBuffer, m_ListEntry);
    }

private:
    NTSTATUS
    InitializeLaunchAppMessage(
        _In_ USHORT cchPayload,
        _In_count_(cchPayload) LPWSTR pwszPayload
        );

    void
    ExtractPairingPayload(
        _In_count_(cRawRecords) UCHAR *rgpRawRecords[],
        _In_ UINT32 cRawRecords
        );

    NTSTATUS
    SetRawBuffer(
        _In_ USHORT cbRawNdef,
        _In_bytecount_(cbRawNdef) PBYTE pbRawNdef
        );

    UCHAR
    FindUriPrefixIndex(
        _In_ UCHAR Prefix
        );

    UCHAR
    FindUriPrefixKey(
        _Inout_count_(cchPayload) LPWSTR & pszPayload,
        _Inout_ UINT32 & cchPayload
        );

    NTSTATUS
    CreateEPCRawUri(
        _In_ UINT32 cchPayload,
        _In_count_(cchPayload) PBYTE pbPayload
        );

private:
    PBYTE    m_pbBuffer;
    USHORT   m_cbBuffer;
    PBYTE    m_pbPayload;
    USHORT   m_cbPayload;
    PBYTE    m_pbPairingBtPayload;
    USHORT   m_cbPairingBtPayload;
    PBYTE    m_pbWindowsMimeBuffer;
    USHORT   m_cbWindowsMimeBuffer;
    UCHAR    m_tnf;
    PBYTE    m_pbSubTypeExt;
    USHORT   m_cbSubTypeExt;
    BOOLEAN  m_Barcode;
    union
    {
        WCHAR    m_szDecodedUri[INTERNET_MAX_URL_LENGTH];
        uint8_t  m_rgBuffer[INTERNET_MAX_URL_LENGTH * 2];
    };

    LIST_ENTRY m_ListEntry;
};
