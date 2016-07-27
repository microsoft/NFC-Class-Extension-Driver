/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    WdmLinkedList.h

Abstract:

    Doubly-linked list manipulation routines from wdm.h. Since NfcCx and client
    drivers are UMDF drivers, they need to include windows.h, which will not
    allow wdm.h to be included. Thus, the linked list routines from wdm.h are
    copied here.

--*/

#pragma once

#include <minwindef.h>
#include <winnt.h>

#pragma warning(push)
#pragma warning(disable:4714) // function marked as FORCEINLINE not inlined
#pragma warning(disable:4793) // function compiled as native

//++
//VOID
//RtlFailFast (
//    _In_ ULONG Code
//    );
//
// Routine Description:
//
//    This routine brings down the caller immediately in the event that
//    critical corruption has been detected.  No exception handlers are
//    invoked.
//
//    The routine may be used in libraries shared with user mode and
//    kernel mode.  In user mode, the process is terminated, whereas in
//    kernel mode, a KERNEL_SECURITY_CHECK_FAILURE bug check is raised.
//
// Arguments
//
//    Code - Supplies the reason code describing what type of corruption
//           was detected.
//
// Return Value:
//
//    None.  There is no return from this routine.
//
//--

DECLSPEC_NORETURN
FORCEINLINE
VOID
RtlFailFast(
    _In_ ULONG Code
    )

{

    __fastfail(Code);
}


//
//  Doubly-linked list manipulation routines.
//

//
//  VOID
//  InitializeListHead32(
//      PLIST_ENTRY32 ListHead
//      );
//

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#define RTL_STATIC_LIST_HEAD(x) LIST_ENTRY x = { &x, &x }

FORCEINLINE
VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
    )

{

    ListHead->Flink = ListHead->Blink = ListHead;
    return;
}

_Must_inspect_result_
BOOLEAN
CFORCEINLINE
IsListEmpty(
    _In_ const LIST_ENTRY * ListHead
    )

{

    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
BOOLEAN
RemoveEntryListUnsafe(
    _In_ PLIST_ENTRY Entry
    )

{

    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

//++
//VOID
//FatalListEntryError (
//    _In_ PVOID p1,
//    _In_ PVOID p2,
//    _In_ PVOID p3
//    );
//
// Routine Description:
//
//    This routine reports a fatal list entry error.  It is implemented here as a
//    wrapper around RtlFailFast so that alternative reporting mechanisms (such
//    as simply logging and trying to continue) can be easily switched in.
//
// Arguments:
//
//    p1 - Supplies the first failure parameter.
//
//    p2 - Supplies the second failure parameter.
//
//    p3 - Supplies the third failure parameter.
//
//Return Value:
//
//    None.
//--

FORCEINLINE
VOID
FatalListEntryError(
    _In_ PVOID p1,
    _In_ PVOID p2,
    _In_ PVOID p3
    )

{

    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);
    UNREFERENCED_PARAMETER(p3);

    RtlFailFast(FAST_FAIL_CORRUPT_LIST_ENTRY);
}

FORCEINLINE
VOID
RtlpCheckListEntry(
    _In_ PLIST_ENTRY Entry
    )

{

    if ((((Entry->Flink)->Blink) != Entry) || (((Entry->Blink)->Flink) != Entry)) {
        FatalListEntryError((PVOID)(Entry),
                            (PVOID)((Entry->Flink)->Blink),
                            (PVOID)((Entry->Blink)->Flink));
    }
}


FORCEINLINE
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry
    )

{

    PLIST_ENTRY PrevEntry;
    PLIST_ENTRY NextEntry;

    NextEntry = Entry->Flink;
    PrevEntry = Entry->Blink;
    if ((NextEntry->Blink != Entry) || (PrevEntry->Flink != Entry)) {
        FatalListEntryError((PVOID)PrevEntry,
                            (PVOID)Entry,
                            (PVOID)NextEntry);
    }

    PrevEntry->Flink = NextEntry;
    NextEntry->Blink = PrevEntry;
    return (BOOLEAN)(PrevEntry == NextEntry);
}

FORCEINLINE
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
    )

{

    PLIST_ENTRY Entry;
    PLIST_ENTRY NextEntry;

    Entry = ListHead->Flink;

#if DBG

    RtlpCheckListEntry(ListHead);

#endif

    NextEntry = Entry->Flink;
    if ((Entry->Blink != ListHead) || (NextEntry->Blink != Entry)) {
        FatalListEntryError((PVOID)ListHead,
                            (PVOID)Entry,
                            (PVOID)NextEntry);
    }

    ListHead->Flink = NextEntry;
    NextEntry->Blink = ListHead;

    return Entry;
}

FORCEINLINE
PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead
    )
{

    PLIST_ENTRY Entry;
    PLIST_ENTRY PrevEntry;

    Entry = ListHead->Blink;

#if DBG

    RtlpCheckListEntry(ListHead);

#endif

    PrevEntry = Entry->Blink;
    if ((Entry->Flink != ListHead) || (PrevEntry->Flink != Entry)) {
        FatalListEntryError((PVOID)PrevEntry,
                            (PVOID)Entry,
                            (PVOID)ListHead);
    }

    ListHead->Blink = PrevEntry;
    PrevEntry->Flink = ListHead;
    return Entry;
}


FORCEINLINE
VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Out_ __drv_aliasesMem PLIST_ENTRY Entry
    )
{

    PLIST_ENTRY PrevEntry;

#if DBG

    RtlpCheckListEntry(ListHead);

#endif

    PrevEntry = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = PrevEntry;
    if (PrevEntry->Flink != ListHead) {
        FatalListEntryError((PVOID)PrevEntry,
                            (PVOID)ListHead,
                            (PVOID)PrevEntry->Flink);
    }

    PrevEntry->Flink = Entry;
    ListHead->Blink = Entry;
    return;
}


FORCEINLINE
VOID
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Out_ __drv_aliasesMem PLIST_ENTRY Entry
    )

{

    PLIST_ENTRY NextEntry;

#if DBG

    RtlpCheckListEntry(ListHead);

#endif

    NextEntry = ListHead->Flink;
    Entry->Flink = NextEntry;
    Entry->Blink = ListHead;
    if (NextEntry->Blink != ListHead) {
        FatalListEntryError((PVOID)ListHead,
                            (PVOID)NextEntry,
                            (PVOID)NextEntry->Blink);
    }

    NextEntry->Blink = Entry;
    ListHead->Flink = Entry;
    return;
}

FORCEINLINE
VOID
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    RtlpCheckListEntry(ListHead);
    RtlpCheckListEntry(ListToAppend);
    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
    return;
}


FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead
    )
{

    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}


FORCEINLINE
VOID
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry
    )

{

    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
    return;
}

#pragma warning(pop)
