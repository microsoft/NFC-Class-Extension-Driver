/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:

    Dllmain.cpp

Abstract:

    NFC Radio Manager DLL Entry Points

Environment:

    User mode only.

Revision History:

--*/

#include "pch.h"
#include "dllmain.tmh"

class CNfcRadioModule : public CAtlDllModuleT<CNfcRadioModule>
{
public:
};

IMPLEMENT_OBJECT_ENTRY_AUTO(CNfcRadioManager);

CNfcRadioModule g_AtlModule;
HINSTANCE g_hInstance;

//
// Function: DllMain
// 
// Description: DLL Entry Point
//
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        WPP_INIT_TRACING(L"Microsoft\\Device\\NFC Radio Media Manager");
        break;

    case DLL_PROCESS_DETACH:
        WPP_CLEANUP();
        break;
    }

    return g_AtlModule.DllMain(dwReason, pvReserved);
}

//
// Function: SetNfcRadioState
// 
// Description: Set the NFC radio state
//
extern "C"
HRESULT SetNfcRadioState(__in HANDLE DeviceHandle, __in BOOLEAN SysRadioState, __in BOOLEAN MediaRadioOn)
{
    HRESULT hr = S_OK;
    DWORD dwError = ERROR_SUCCESS;
    OVERLAPPED Overlapped = {0};
    DWORD bytesReturned = 0;
    NFCRM_SET_RADIO_STATE setRadioState = {0};

    setRadioState.SystemStateUpdate = SysRadioState;
    setRadioState.MediaRadioOn = MediaRadioOn;

    Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (Overlapped.hEvent != NULL)
    {
        if (!DeviceIoControl(DeviceHandle,
                            IOCTL_NFCRM_SET_RADIO_STATE, 
                            &setRadioState, 
                            sizeof(setRadioState), 
                            NULL, 
                            0, 
                            &bytesReturned, 
                            &Overlapped))
        {
            dwError = GetLastError();
            
            if (dwError == ERROR_IO_PENDING)
            {
                dwError = ERROR_SUCCESS;

                if (!GetOverlappedResult(DeviceHandle, &Overlapped, &bytesReturned, TRUE))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CancelIo(DeviceHandle);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }

        CloseHandle(Overlapped.hEvent);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//
// Function: GetNfcRadioState
// 
// Description: Get the NFC radio state
//
extern "C"
HRESULT GetNfcRadioState(__in HANDLE DeviceHandle, __out BOOLEAN *MediaRadioOn)
{
    HRESULT hr = S_OK;
    DWORD dwError = ERROR_SUCCESS;
    OVERLAPPED Overlapped = {0};
    DWORD bytesReturned = 0;
    NFCRM_RADIO_STATE queryRadioState = {0};

    Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (Overlapped.hEvent != NULL)
    {
        if (!DeviceIoControl(DeviceHandle,
                            IOCTL_NFCRM_QUERY_RADIO_STATE,
                            NULL,
                            0,
                            &queryRadioState,
                            sizeof(queryRadioState),
                            &bytesReturned,
                            &Overlapped))
        {
            dwError = GetLastError();

            if (dwError == ERROR_IO_PENDING)
            {
                dwError = ERROR_SUCCESS;

                if (!GetOverlappedResult(DeviceHandle, &Overlapped, &bytesReturned, TRUE))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CancelIo(DeviceHandle);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }

        CloseHandle(Overlapped.hEvent);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        *MediaRadioOn = queryRadioState.MediaRadioOn;
    }

    return hr;
}

//
// Function: DllCanUnloadNow
// 
// Description: Used to determine whether the DLL can be unloaded by OLE
//
STDAPI DllCanUnloadNow(void)
{
    return g_AtlModule.DllCanUnloadNow();
}


//
// Function: DllGetClassObject
// 
// Description: Returns a class factory to create an object of the requested type
//
STDAPI DllGetClassObject(
    __in REFCLSID rclsid, 
    __in REFIID riid, 
    __deref_out LPVOID* ppv
    )
{
    return g_AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


//
// Function: DllRegisterServer
// 
// Description: Adds entries to the system registry
//
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    g_AtlModule.DllRegisterServer(FALSE);
    return S_OK;
}


//
// Function: DllUnregisterServer
// 
// Description: Removes entries from the system registry
//
STDAPI DllUnregisterServer(void)
{
    return g_AtlModule.DllUnregisterServer();
}


