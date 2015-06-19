/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Abstract:

    Implements a driver simulator broadcast

Environment:

    User-mode only.
    
--*/
#pragma once

typedef BOOL (__stdcall *XDE_INITIALIZE)(VOID);
typedef BOOL (__stdcall *XDE_GET_INTERNAL_MAC_ADDRESS)(BYTE *, ULONG);

class CDriverSimBroadcast
{
public:
    CDriverSimBroadcast();
    ~CDriverSimBroadcast();

public:
    HRESULT Initialize(_In_ CSocketListener* pSocketListener);
    HRESULT Uninitialize();

    HRESULT InitBroadcastMessage();
    HRESULT FindListenAddress();

    static void CALLBACK s_BroadcastTimerFired(_Inout_ PTP_CALLBACK_INSTANCE /*pInstance*/, _In_ PVOID pContext, _Inout_ PTP_TIMER /*pTimer*/);

private:
    ULONG               _ulListenAddress;
    ULONG               _ulBroadcastSrcAddress;
    ULONG               _BroadcastDestAddress;
    USHORT              _usListenPort;
    CSocketListener*    _pSocketListener;
    PTP_TIMER           _TpBroadcast;
    WCHAR               _wszBroadcastMessage[MAX_PATH + 160 + MAX_PATH + 1 + 1];
    HMODULE             _hXdeModule;
    XDE_INITIALIZE      _pfnXdeInitialize;
    XDE_GET_INTERNAL_MAC_ADDRESS _pfnXdeGetInternalMacAddress;
};
