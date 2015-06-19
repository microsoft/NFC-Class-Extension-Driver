#ifndef _NFCCX_Vstubs_NfcCx_version_data_xml_TYPES_H_
#define _NFCCX_Vstubs_NfcCx_version_data_xml_TYPES_H_


typedef enum _NFCCXFUNCENUM_Vstubs_NfcCx_version_data_xml {
    NfcCxFunctionTableNumEntries_Vstubs_NfcCx_version_data_xml = 9,
} NFCCXFUNCENUM_Vstubs_NfcCx_version_data_xml;

typedef struct _NFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml *PNFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml;
typedef const struct _NFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml *PCNFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml;
typedef struct _NFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml *PNFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml;
typedef const struct _NFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml *PCNFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml;
typedef struct _NFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml *PNFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml;
typedef const struct _NFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml *PCNFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml;
typedef struct _NFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml *PNFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml;
typedef const struct _NFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml *PCNFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml;
typedef struct _NFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml *PNFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml;
typedef const struct _NFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml *PCNFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml;

//
// Versioning of structures for nfccx.h
//
typedef struct _NFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml {
    ULONG Reserved;

} NFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml, *PNFCCX_DRIVER_GLOBALS_Vstubs_NfcCx_version_data_xml;

// 
// Client Configuration
// 
typedef struct _NFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml {
    // 
    // Size of this structure in bytes
    // 
    ULONG Size;

    WDF_TRI_STATE IsPowerPolicyOwner;

    // 
    // IdleTimeout value is in milliseconds
    // 
    ULONG PowerIdleTimeout;

    WDF_POWER_POLICY_IDLE_TIMEOUT_TYPE PowerIdleType;

    NFC_CX_TRANSPORT_TYPE BusType;

    // 
    // Combination of NFC_CX_DRIVER_FLAGS values
    // 
    ULONG DriverFlags;

    // 
    // Device mode
    // 
    NFC_CX_DEVICE_MODE DeviceMode;

    // 
    // Event callbacks
    // 
    PFN_NFC_CX_WRITE_NCI_PACKET EvtNfcCxWriteNciPacket;

    PFN_NFC_CX_DEVICE_IO_CONTROL EvtNfcCxDeviceIoControl;

} NFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml, *PNFC_CX_CLIENT_CONFIG_Vstubs_NfcCx_version_data_xml;

typedef struct _NFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml {
    NTSTATUS HardwareStatus;

    NFC_CX_HOST_ACTION HostAction;

} NFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml, *PNFC_CX_HARDWARE_EVENT_Vstubs_NfcCx_version_data_xml;

// 
// RF Discovery Configuration
// 
typedef struct _NFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml {
    // 
    // Size of this structure in bytes
    // 
    ULONG Size;

    // 
    // Total Duration of the single discovery period in milliseconds
    // 
    USHORT TotalDuration;

    // 
    // Combination of NFC_CX_POLL_MODE_CONFIG values
    // 
    ULONG PollConfig;

    // 
    // Combination of NFC_CX_NFCIP_MODE_CONFIG values
    // 
    UCHAR NfcIPMode;

    // 
    // Combination of NFC_CX_NFCIP_TGT_MODE_CONFIG values
    // 
    UCHAR NfcIPTgtMode;

    // 
    // Combination of NFC_CX_CE_MODE_CONFIG values
    // 
    UCHAR NfcCEMode;

    // 
    // Combination of NFC_CX_POLL_BAILOUT_CONFIG values
    // 
    UCHAR BailoutConfig;

} NFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml, *PNFC_CX_RF_DISCOVERY_CONFIG_Vstubs_NfcCx_version_data_xml;

// 
// LLCP Configuration
// 
typedef struct _NFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml {
    // 
    // Size of this structure in bytes
    // 
    ULONG Size;

    // 
    // Max information unit in bytes
    // 
    USHORT Miu;

    // 
    // LTO timeout in multiples of 10 milliseconds
    // 
    UCHAR LinkTimeout;

    // 
    // The receive window size as per LLCP spec
    // 
    UCHAR RecvWindowSize;

} NFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml, *PNFC_CX_LLCP_CONFIG_Vstubs_NfcCx_version_data_xml;

// End of versioning of structures for nfccx.h


#endif // _NFCCX_Vstubs_NfcCx_version_data_xml_TYPES_H_
