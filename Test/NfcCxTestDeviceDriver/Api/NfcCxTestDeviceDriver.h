//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <minwindef.h>

typedef LONG NTSTATUS;

#define FILE_NAMESPACE_NCI_SIMULATOR L"NciSimulator"

// {C64AFEB6-48E4-4B57-8B2A-4F05070AF89F}
static constexpr GUID GUID_DEVINTERFACE_NCI_SIMULATOR = { 0xc64afeb6, 0x48e4, 0x4b57, { 0x8b, 0x2a, 0x4f, 0x5, 0x7, 0xa, 0xf8, 0x9f } };

//
// NCI Simulator DDI
//
#define FILE_DEVICE_NCISIM  (ULONG)0xDB2F

// Waits for and retrieves the next callback message.
// Input: <none>
// Output: NciSimCallbackHeader
#define IOCTL_NCISIM_GET_NEXT_CALLBACK              CTL_CODE(FILE_DEVICE_NCISIM, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Provides an NCI response packet or NCI notification packet, to be forwarded to the NfcCx.
// Input: BYTE[]
// Output: <none>
#define IOCTL_NCISIM_NCI_READ                       CTL_CODE(FILE_DEVICE_NCISIM, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Requests for 'WdfDeviceStopIdle' to be called.
// Input: <none>
// Output: <none>
#define IOCTL_NCISIM_ADD_D0_POWER_REFERENCE         CTL_CODE(FILE_DEVICE_NCISIM, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Requests for 'WdfDeviceResumeIdle' to be called.
// Input: <none>
// Output: <none>
#define IOCTL_NCISIM_REMOVE_D0_POWER_REFERENCE      CTL_CODE(FILE_DEVICE_NCISIM, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Provides the result for a sequence handler.
// Input: NciSimSequenceHandlerComplete
// Output: <none>
#define IOCTL_NCISIM_SEQUENCE_HANDLER_COMPLETE      CTL_CODE(FILE_DEVICE_NCISIM, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Signals that the NCI write packet has been received successfully.
// Input: <none>
// Output: <none>
#define IOCTL_NCISIM_NCI_WRITE_COMPLETE             CTL_CODE(FILE_DEVICE_NCISIM, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

enum class NciSimCallbackType : DWORD
{
    NciWrite = 0,
    SequenceHandler = 1,
};

// NfcCx.h has a hard dependency on Wdf.h, which isn't accessible to normal apps.
// So duplicate any required definitions.
#ifndef _NFCCX_H_

enum NFC_CX_SEQUENCE
{
    SequencePreInit = 0,
    SequenceInitComplete,
    SequencePreRfDiscStart,
    SequenceRfDiscStartComplete,
    SequencePreRfDiscStop,
    SequenceRfDiscStopComplete,
    SequencePreNfceeDisc,
    SequenceNfceeDiscComplete,
    SequencePreShutdown,
    SequenceShutdownComplete,
    SequencePreRecovery,
    SequenceRecoveryComplete,
    SequenceMaximum,
};

#define NFC_CX_SEQUENCE_PRE_INIT_FLAG_SKIP_CONFIG     0x00000001 // Skip configuration during init sequence
#define NFC_CX_SEQUENCE_PRE_INIT_FLAG_FORCE_CONFIG    0x00000002 // Force update configuration during init sequence
#define NFC_CX_SEQUENCE_INIT_COMPLETE_FLAG_REDO       0x00000001 // Request to redo the init sequence
#define NFC_CX_SEQUENCE_PRE_NFCEE_DISC_FLAG_SKIP      0x00000001 // Skip the NFCEE discovery sequence
#define NFC_CX_SEQUENCE_PRE_SHUTDOWN_FLAG_SKIP_RESET  0x00000001 // Skip sending NCI reset during shutdown sequence

#endif // _NFCCX_H_

struct NciSimCallbackHeader
{
    NciSimCallbackType Type;
};

struct NciSimCallbackNciWrite :
    public NciSimCallbackHeader
{
    BYTE NciMessage[ANYSIZE_ARRAY];
};

struct NciSimCallbackSequenceHandler :
    public NciSimCallbackHeader
{
    NFC_CX_SEQUENCE Sequence;
};

struct NciSimSequenceHandlerComplete
{
    NTSTATUS Status;
    ULONG Flags;
};

static constexpr size_t NciSimCallbackNciWriteMinSize = sizeof(NciSimCallbackNciWrite) - 1; // remove dummy byte in array

// These data types are used directly in the I/O requests.
// So they need to be plain-old-data types to avoid undefined behavior.
static_assert(__is_trivial(NciSimCallbackHeader), "NCI Simulator API types must be trivial.");
static_assert(__is_trivial(NciSimCallbackNciWrite), "NCI Simulator API types must be trivial.");
static_assert(__is_trivial(NciSimCallbackSequenceHandler), "NCI Simulator API types must be trivial.");
static_assert(__is_trivial(NciSimSequenceHandlerComplete), "NCI Simulator API types must be trivial.");
