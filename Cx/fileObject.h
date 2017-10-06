/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    fileObject.h

Abstract:

    Cx File Object declarations.
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#include "power.h"

#define NFCCX_FILE_CONTEXT_SIGNATURE ('FcfN') //'NfcF'

typedef enum _FILE_OBJECT_ROLE {
    ROLE_UNDEFINED,
    ROLE_CONFIGURATION,
    ROLE_SUBSCRIPTION,
    ROLE_PUBLICATION,
    ROLE_SMARTCARD,
    ROLE_SECUREELEMENTEVENT,
    ROLE_SECUREELEMENTMANAGER,
    ROLE_EMBEDDED_SE,
} FILE_OBJECT_ROLE;

typedef struct _NFCCX_FILE_CONTEXT {

    ULONG Signature;

    //
    // List entry into the client lists
    //
    LIST_ENTRY ListEntry;

    //
    // When a publication is sending data,
    // it gets added to the send client list.
    // this list entry is the link.
    //
    LIST_ENTRY SendListEntry;

    //
    // Back pointer into the FdoContext
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Back pointer to the file object
    //
    WDFFILEOBJECT FileObject;

    //
    // If this member is TRUE, the process holding
    // the file handle is an App Container Process
    //
    BOOLEAN IsAppContainerProcess;

    //
    // Protected by fdoContext.Power->WaitLock
    //
    LONG PowerReferences[NfcCxPowerReferenceType_MaxCount];

    //
    // File object state lock
    //
    WDFWAITLOCK StateLock;

    //
    // Unresponsive client detection timer
    //
    WDFTIMER UnresponsiveClientDetectionTimer;
    _Guarded_by_(StateLock)
    BOOLEAN IsUnresponsiveClientDetected;

    //
    // Opened Interface role
    //
    _Guarded_by_(StateLock)
    BOOLEAN Enabled;
    FILE_OBJECT_ROLE Role;

    TRANSLATION_TYPE_PROTOCOL TranslationType;
    UCHAR Tnf;

    // NULL terminated string
    PSTR pszTypes;
    // Character count
    UCHAR cchTypes;

    //
    // Role specific parameters
    //
    union {
        struct  {
            _Guarded_by_(StateLock)
            CNFCProximityBuffer * PublicationBuffer;
            _Guarded_by_(StateLock)
            WDFQUEUE SendMsgRequestQueue;
            _Guarded_by_(StateLock)
            UCHAR SentMsgCounter;
        } Pub;

        struct {
            _Guarded_by_(StateLock)
            LIST_ENTRY SubscribedMessageQueue;
            UCHAR SubscribedMessageQueueLength;
            _Guarded_by_(StateLock)
            WDFQUEUE SubsMessageRequestQueue;
        } Sub;

        struct {
            _Guarded_by_(StateLock)
            LIST_ENTRY EventQueue;
            UCHAR EventQueueLength;
            _Guarded_by_(StateLock)
            WDFQUEUE EventRequestQueue;
            GUID SEIdentifier;
            SECURE_ELEMENT_EVENT_TYPE eSEEventType;
        } SEEvent;

        struct {
            _Guarded_by_(StateLock)
            LIST_ENTRY PacketQueue;
            UCHAR PacketQueueLength;
            _Guarded_by_(StateLock)
            WDFQUEUE PacketRequestQueue;
        } SEManager;
    } RoleParameters;
}NFCCX_FILE_CONTEXT, *PNFCCX_FILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NFCCX_FILE_CONTEXT, NfcCxFileGetContext);


typedef struct _NFCCX_IMPERSONATION_CONTEXT {
    ULONG ProcessId;
    PNFCCX_FILE_CONTEXT FileContext;
} NFCCX_IMPERSONATION_CONTEXT, *PNFCCX_IMPERSONATION_CONTEXT;


//
// Wdf callbacks
//
EVT_WDFCX_DEVICE_FILE_CREATE NfcCxEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE NfcCxEvtFileClose;

BOOLEAN FORCEINLINE
NfcCxFileObjectIsPubSub(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return (FileContext->Role == ROLE_SUBSCRIPTION) || 
           (FileContext->Role == ROLE_PUBLICATION);
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsNormalSubscription(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_SUBSCRIPTION) && 
            (FileContext->TranslationType != TRANSLATION_TYPE_ARRIVAL) && 
            (FileContext->TranslationType != TRANSLATION_TYPE_REMOVAL) &&
            (FileContext->TranslationType != TRANSLATION_TYPE_WRITABLETAG_SIZE));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsArrivalSubscription(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_SUBSCRIPTION) && 
            (FileContext->TranslationType == TRANSLATION_TYPE_ARRIVAL));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsRemovalSubscription(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_SUBSCRIPTION) && 
            (FileContext->TranslationType == TRANSLATION_TYPE_REMOVAL));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsWritableTagSubscription(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_SUBSCRIPTION) && 
            (FileContext->TranslationType == TRANSLATION_TYPE_WRITABLETAG_SIZE));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsTagWriteClient(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_PUBLICATION) &&
            ((FileContext->TranslationType == TRANSLATION_TYPE_PAYLOAD_ONLY_WRITETAG) ||
                (FileContext->TranslationType == TRANSLATION_TYPE_RAW_NDEF_WRITETAG) ||
                (FileContext->TranslationType == TRANSLATION_TYPE_WINDOWSURI_WRITETAG) ||
                (FileContext->TranslationType == TRANSLATION_TYPE_LAUNCH_APP_WRITETAG) ||
                (FileContext->TranslationType == TRANSLATION_TYPE_SETTAG_READONLY)));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsTagReadOnlyClient(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return ((FileContext->Role == ROLE_PUBLICATION) &&
            ((FileContext->TranslationType == TRANSLATION_TYPE_SETTAG_READONLY)));
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsSEEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return (FileContext->Role == ROLE_SECUREELEMENTEVENT);
}

BOOLEAN FORCEINLINE
NfcCxFileObjectIsSEManager(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return (FileContext->Role == ROLE_SECUREELEMENTMANAGER);
}

PNFP_INTERFACE FORCEINLINE
NfcCxFileObjectGetNfpInterface(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return FileContext->FdoContext->NfpInterface;
}

PNFCCX_SC_INTERFACE FORCEINLINE
NfcCxFileObjectGetScInterface(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return FileContext->FdoContext->SCInterface;
}

PNFCCX_SE_INTERFACE FORCEINLINE
NfcCxFileObjectGetSEInterface(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return FileContext->FdoContext->SEInterface;
}

PNFCCX_FDO_CONTEXT FORCEINLINE
NfcCxFileObjectGetFdoContext(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    return FileContext->FdoContext;
}

NTSTATUS 
NfcCxFileObjectSetType(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ LPWSTR Type
    );

EVT_WDF_REQUEST_IMPERSONATE
NfcCxFileObjectImpersonate;

NTSTATUS
NfcCxFileObjectCheckInitiatorProcess(
    _In_ PNFCCX_IMPERSONATION_CONTEXT ImpersonationContext
    );

NTSTATUS
NfcCxFileObjectDetectRole(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_opt_ PUNICODE_STRING FileName
    );

NTSTATUS
NfcCxFileObjectNfpDisable(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxFileObjectNfpEnable(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

NTSTATUS
NfcCxFileObjectValidateAndSetPayload(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_bytecount_(PayloadLength) PVOID Payload,
    _In_  size_t PayloadLength);

NTSTATUS
NfcCxFileObjectValidateAndSubscribeForEvent(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ GUID& SEIdentifier,
    _In_  SECURE_ELEMENT_EVENT_TYPE eSEEventType);

_Requires_lock_held_(FileContext->StateLock)
NTSTATUS
NfcCxFileObjectCompleteSentMessageRequestLocked(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    );

VOID FORCEINLINE
NfcCxFileObjectStartUnresponsiveClientDetectionTimer(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    WdfTimerStart(FileContext->UnresponsiveClientDetectionTimer, 
                  WDF_REL_TIMEOUT_IN_MS(NFP_UNRESPONSIVE_CLIENT_TIMER_MS));
}

_Requires_lock_held_(FileContext->StateLock)
VOID FORCEINLINE
NfcCxFileObjectResetUnresponsiveClientDetection(
    _In_ PNFCCX_FILE_CONTEXT FileContext
    )
{
    FileContext->IsUnresponsiveClientDetected = FALSE;
}

VOID FORCEINLINE
NfcCxFileObjectStopUnresponsiveClientDetectionTimer(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ BOOLEAN Wait
    )
{
    WdfTimerStop(FileContext->UnresponsiveClientDetectionTimer, Wait);
}

EVT_WDF_TIMER
NfcCxFileObjectUnresponsiveClientDetectionTimer;
