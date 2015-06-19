/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Abstract:

    Implements a driver simulator broadcast

Environment:

    User-mode only.
    
--*/
#include "Internal.h"
#include "DriverSimBroadcast.tmh"
#include <IPHlpApi.h>

#define INADDR_ALL                      0xFFFFFFFF
#define XDE_MAC_ADDRESS_BYTE_SIZE       6
#define BROADCAST_INTERVAL_MS           10000 // 10 second broadcast interval
#define NFC_DRIVER_SIM_BROADCAST_PORT   9299

CDriverSimBroadcast::CDriverSimBroadcast()
    : _ulListenAddress(INADDR_ANY),
      _ulBroadcastSrcAddress(INADDR_ANY),
      _BroadcastDestAddress(INADDR_ALL),
      _TpBroadcast(nullptr),
      _hXdeModule(nullptr),
      _pfnXdeInitialize(nullptr),
      _pfnXdeGetInternalMacAddress(nullptr)
{
    _hXdeModule = LoadLibraryEx(L"XdeUtils.dll", NULL, 0);

    if (_hXdeModule != nullptr) {
        _pfnXdeInitialize = (XDE_INITIALIZE)GetProcAddress(_hXdeModule, "XdeUtilsInitialize");
        _pfnXdeGetInternalMacAddress = (XDE_GET_INTERNAL_MAC_ADDRESS)GetProcAddress(_hXdeModule, "XdeUtilsGetInternalMacAddress");
    }
}

CDriverSimBroadcast::~CDriverSimBroadcast()
{
    Uninitialize();

    SAFE_FREELIBRARY(_hXdeModule);

    _pfnXdeInitialize = nullptr;
    _pfnXdeGetInternalMacAddress = nullptr;
}

HRESULT CDriverSimBroadcast::Initialize(_In_ CSocketListener* pSocketListener)
{
    MethodEntry("...");

    _pSocketListener = pSocketListener;

    HRESULT hr = InitBroadcastMessage();
    
    if (SUCCEEDED(hr)) {
        _TpBroadcast = CreateThreadpoolTimer(&s_BroadcastTimerFired, (PVOID)this, nullptr);
        if (_TpBroadcast == nullptr) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr)) {
        // Convert milliseconds to relative filetime units (100ns)
        LONGLONG dueTime = Int32x32To64(BROADCAST_INTERVAL_MS, -10000);
        SetThreadpoolTimer(_TpBroadcast, (FILETIME*)&dueTime, BROADCAST_INTERVAL_MS, 3000);
    }

    MethodReturnHR(hr);
}

HRESULT CDriverSimBroadcast::Uninitialize()
{
    MethodEntry("...");

    HRESULT hr = S_OK;

    if (_TpBroadcast != nullptr) {
        SetThreadpoolTimer(_TpBroadcast, nullptr, 0, 0);
        WaitForThreadpoolTimerCallbacks(_TpBroadcast, TRUE);
        CloseThreadpoolTimer(_TpBroadcast);
        _TpBroadcast = nullptr;
    }

    _pSocketListener = nullptr;

    MethodReturnHR(hr);
}

HRESULT CDriverSimBroadcast::FindListenAddress()
{
    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = nullptr;
    ULONG cbAdapterInfo = 0;
    ULONG ulFirstFoundAddress = INADDR_ANY;
    ULONG ulFirstFoundDestination = INADDR_ALL;
    bool fAddressChanged = false;
    bool fBroadcastAddressFound = false;
    bool fIsXdeAdapter = false;
    BYTE XdeMacAddress[XDE_MAC_ADDRESS_BYTE_SIZE] = {};

    //
    // If we find an XDE adapter then we'll listen only on that adapter, and broadcast there too.
    // If we don't find an XDE adapter, then we listen on any adapter (the default) and broadcast
    // on the first adapter we find.
    //
    if (_pfnXdeInitialize != nullptr && _pfnXdeInitialize()) {
        if (_pfnXdeGetInternalMacAddress != nullptr) {
            fIsXdeAdapter = (_pfnXdeGetInternalMacAddress(XdeMacAddress, sizeof(XdeMacAddress)) == TRUE);
        }
    }

    if (GetAdaptersInfo(nullptr, &cbAdapterInfo) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(cbAdapterInfo);
        
        if (pAdapterInfo != nullptr) {
            hr = HRESULT_FROM_WIN32(GetAdaptersInfo(pAdapterInfo, &cbAdapterInfo));
        }
        else {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr)) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

        while (pAdapter != nullptr) {
            if (pAdapter->AddressLength == 6 && // Ethernet MAC address length
                pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
                ULONG ulAddress = inet_addr(pAdapter->IpAddressList.IpAddress.String);
                ULONG ulSubnetMask = inet_addr(pAdapter->IpAddressList.IpMask.String);
                ULONG ulBroadcastDest = (ulAddress & ulSubnetMask) | ~ulSubnetMask;

                if (ulAddress != INADDR_NONE) {
                    if (fIsXdeAdapter && memcmp(pAdapter->Address, XdeMacAddress, sizeof(XdeMacAddress)) == 0) {
                        if (_ulListenAddress != ulAddress) {
                            _ulListenAddress = ulAddress;
                            fAddressChanged = true;
                        }

                        _ulBroadcastSrcAddress = ulAddress;
                        fBroadcastAddressFound = true;
                        break;
                    }

                    if (ulFirstFoundAddress == INADDR_ANY) {
                        ulFirstFoundAddress = ulAddress;
                        ulFirstFoundDestination = ulBroadcastDest;
                        
                        if (_ulBroadcastSrcAddress == INADDR_ANY) {
                            // Broadcast on the first address we find, unless we later find an XDE internal address above
                            _ulBroadcastSrcAddress = ulAddress;
                        }
                    }

                    if (ulAddress == _ulBroadcastSrcAddress) {
                        fBroadcastAddressFound = true;
                    }
                }
            }

            pAdapter = pAdapter->Next;
        }
    }

    if (!fBroadcastAddressFound) {
        // If we never found the old broadcast address, then the address may have changed.
        // Fallback to using the first found address.
        _ulBroadcastSrcAddress = ulFirstFoundAddress;
    }

    if (fIsXdeAdapter && _ulListenAddress == INADDR_ANY) {
        // We're running on XDE but we didn't find the XDE internal adapter, we'll not listen on any adapter
        _ulListenAddress = INADDR_NONE;
    }

    if (pAdapterInfo != nullptr) {
        free(pAdapterInfo);
    }

    if (SUCCEEDED(hr) && !fAddressChanged) {
        // No change was made to the listen address
        hr = S_FALSE;
    }
    
    return hr;
}

HRESULT CDriverSimBroadcast::InitBroadcastMessage()
{
    MethodEntry("...");

    HRESULT hr = S_OK;
    WCHAR wszAddress[MAX_PATH] = {};
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    DWORD cbAddresses = 0;
    WCHAR szBuildString[160] = {};
    OSVERSIONINFO versionInfo = {};
    WCHAR szDevicePlatform[MAX_PATH] = {};
    DWORD cbDevicePlatform = sizeof(szDevicePlatform);
    WCHAR szDeviceClass[MAX_PATH] = {};
    DWORD cbDeviceClass = sizeof(szDeviceClass);

    // Now get the platform and device class name for the device
    if (!NT_SUCCESS(RtlConvertDeviceFamilyInfoToString(
            &cbDevicePlatform,
            &cbDeviceClass,
            szDevicePlatform,
            szDeviceClass))) {
        // Use a default value for the platform name
        StringCchCopy(szDevicePlatform, _countof(szDevicePlatform), L"Windows");
    }

    // Now get the build string for the device
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

    if (GetVersionEx(&versionInfo)) {
        StringCchPrintf(
            szBuildString,
            _countof(szBuildString),
            L"%d.%d.%d.%d",
            versionInfo.dwMajorVersion,
            versionInfo.dwMinorVersion,
            LOWORD(versionInfo.dwBuildNumber),
            HIWORD(versionInfo.dwBuildNumber));
    }
    else {
        // Use a default value for build number
        StringCchCopy(szBuildString, _countof(szBuildString), L"0.0.0.0");
    }

    // Now get the device's MAC address to include in the broadcast
    if (GetAdaptersAddresses(
            AF_UNSPEC,
            0,
            nullptr,
            nullptr,
            &cbAddresses) == ERROR_BUFFER_OVERFLOW) {

        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(cbAddresses);
        if (pAddresses == nullptr) {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr)) {
        if (GetAdaptersAddresses(
                AF_UNSPEC,
                0,
                nullptr,
                pAddresses,
                &cbAddresses) == ERROR_SUCCESS) {
            PIP_ADAPTER_ADDRESSES pCurrentAddress = pAddresses;

            while (pCurrentAddress != nullptr) {
                if (pCurrentAddress->PhysicalAddressLength != 0 &&
                    pCurrentAddress->IfType == IF_TYPE_ETHERNET_CSMACD) {
                    WCHAR szNibble[3] = {};

                    for (DWORD i = 0; i < pCurrentAddress->PhysicalAddressLength; i++) {
                        StringCchPrintf(
                            szNibble,
                            _countof(szNibble),
                            L"%.2X",
                            pCurrentAddress->PhysicalAddress[i]);

                        if (FAILED(StringCchCat(wszAddress, _countof(wszAddress), szNibble))) {
                            wszAddress[0] = L'\0';
                            break;
                        }
                    }
                    break;
                }

                pCurrentAddress = pCurrentAddress->Next;
            }
        }
    }

    if (SUCCEEDED(hr)) {
        hr = StringCchPrintf(
                _wszBroadcastMessage,
                _countof(_wszBroadcastMessage),
                L"%s\x01%s\x01%s",
                szDevicePlatform,
                szBuildString, wszAddress);
    }

    if (pAddresses != nullptr) {
        free(pAddresses);
    }

    MethodReturnHR(hr);
}

void CALLBACK
CDriverSimBroadcast::s_BroadcastTimerFired(
    _Inout_ PTP_CALLBACK_INSTANCE /*pInstance*/,
    _In_    PVOID                 pContext,
    _Inout_ PTP_TIMER             /*pTimer*/
    )
{
    CDriverSimBroadcast* pDevice = (CDriverSimBroadcast*)pContext;
    WCHAR *pwszBroadcastMessage = pDevice->_wszBroadcastMessage;
    
    sockaddr_in dest;
    sockaddr_in local;

    pDevice->FindListenAddress();

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = pDevice->_ulBroadcastSrcAddress;
    local.sin_port = 0;

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = pDevice->_BroadcastDestAddress;
    dest.sin_port = htons( NFC_DRIVER_SIM_BROADCAST_PORT );

    SOCKET s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if (s != INVALID_SOCKET) {
        int so_broadcast = TRUE;
        if (setsockopt(s,
                SOL_SOCKET,
                SO_BROADCAST,
                (LPSTR)&so_broadcast,
                sizeof(so_broadcast)) == 0) {
            if (bind( s, (sockaddr *)&local, sizeof(local) ) == 0) {
                sendto( s, (LPCSTR)pwszBroadcastMessage, (int)(wcslen(pwszBroadcastMessage) * sizeof(WCHAR)), 0, (sockaddr *)&dest, sizeof(dest) );
            }
        }
        closesocket(s);
    }
}
