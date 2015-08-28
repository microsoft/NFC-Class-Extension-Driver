/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioInstance.cpp

Abstract:
    This module implements the radio manager instances that each represent a single NFC radio

Notes:

Environment:
   User mode.

--*/

#include "NfcCxRMPch.h"
#include "NfcRadioInstance.tmh"

EXTERN_C HRESULT SetNfcRadioState(__in HANDLE DeviceHandle, __in BOOLEAN SysRadioState, __in BOOLEAN MediaRadioOn);
EXTERN_C HRESULT GetNfcRadioState(__in HANDLE DeviceHandle, __out BOOLEAN *MediaRadioOn);

// Worker threads
VOID CALLBACK
CNfcRadioInstance::BeginRadioQueryRemoveCancel(
    PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork)
{
    UNREFERENCED_PARAMETER(ptpInstance);
    UNREFERENCED_PARAMETER(ptpWork);

    CNfcRadioInstance* radioInstance = (CNfcRadioInstance*)pContext;
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    radioInstance->RadioQueryRemoveCancel();
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

VOID CALLBACK
CNfcRadioInstance::BeginRadioHandleRemoval(
    PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork)
{
    UNREFERENCED_PARAMETER(ptpInstance);
    UNREFERENCED_PARAMETER(ptpWork);

    CNfcRadioInstance* radioInstance = (CNfcRadioInstance*)pContext;
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    radioInstance->RadioHandleRemoval();
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}


HRESULT CNfcRadioInstance::RegisterDevice()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    HANDLE retHandle = INVALID_HANDLE_VALUE;

    EnterCriticalSection(&m_csHandleLock);

    if (m_fReregisterEnabled)
    {
        // Open a handle to the device
        retHandle = CreateFile(
            m_deviceInstanceID,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);

        if (INVALID_HANDLE_VALUE != retHandle)
        {
            m_hDevice = retHandle;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (SUCCEEDED(hr))
        {
            CM_NOTIFY_FILTER notifyFilter = { 0 };
            HCMNOTIFICATION hDev = NULL;

            ZeroMemory(&notifyFilter, sizeof(notifyFilter));
            notifyFilter.cbSize = sizeof(notifyFilter);
            notifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
            notifyFilter.u.DeviceHandle.hTarget = m_hDevice;

            hr = CM_Register_Notification(
                &notifyFilter,
                this,
                NfcProcessDeviceChange,
                &hDev);

            if (hDev != NULL)
            {
                m_hDevNotification = hDev;
            }
        }
    }

    LeaveCriticalSection(&m_csHandleLock);
    
    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
    return hr;
}

VOID CNfcRadioInstance::RadioQueryRemoval()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    EnterCriticalSection(&m_csHandleLock);
    
    // Close the handle, but keep handle notification
    if (NULL != m_hDevice)
    {
        TRACE_LINE(LEVEL_VERBOSE, L"Closing handle");
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
    }

    LeaveCriticalSection(&m_csHandleLock);
    
    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
}

VOID CNfcRadioInstance::RadioQueryRemoveCancel()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    
    EnterCriticalSection(&m_csHandleLock);

    // Close the old handle, and kill the device registration and start a new one
    RadioHandleRemoval();

    (void) RegisterDevice();
    
    LeaveCriticalSection(&m_csHandleLock);
    
    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
}

VOID CNfcRadioInstance::RadioHandleRemoval()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    EnterCriticalSection(&m_csHandleLock);

    // Unregister for device notification and close the handle
    if (NULL != m_hDevNotification)
    {
        CM_Unregister_Notification(m_hDevNotification);
        m_hDevNotification = NULL;
    }

    if (NULL != m_hDevice)
    {
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
    }

    LeaveCriticalSection(&m_csHandleLock);

    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
}

DWORD
CNfcRadioInstance::NfcProcessDeviceChange(
    _In_ HCMNOTIFICATION       hNotification,
    _In_ PVOID                 Context,
    _In_ CM_NOTIFY_ACTION      Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD                 EventDataSize
    )
{
    UNREFERENCED_PARAMETER(hNotification);
    UNREFERENCED_PARAMETER(EventDataSize);
    UNREFERENCED_PARAMETER(EventData);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    DWORD dwResult = 0;
    CNfcRadioInstance* radioInstance = (CNfcRadioInstance*)Context;

    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVE:
        TRACE_LINE(LEVEL_VERBOSE, L"QueryRemove notification for Radio Instance 0x%p", radioInstance);
        radioInstance->RadioQueryRemoval();
        break;
        
    case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING:
    case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
        if (Action == CM_NOTIFY_ACTION_DEVICEREMOVEPENDING)
        {
            TRACE_LINE(LEVEL_VERBOSE, L"RemovePending notification for Radio Instance 0x%p", radioInstance);
        }
        else if (Action == CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE)
        {
            TRACE_LINE(LEVEL_VERBOSE, L"RemoveComplete notification for Radio Instance 0x%p", radioInstance);
        }

        // We want to UnregisterDevice notifications and drop the handle
        // Unregistering notifications from the callback handle causes deadlocks, so we will spin up a new thread in the default threadpool
        if (NULL == radioInstance->m_ptpRadioHandleRemoval)
        {
            // Failed to spin up the thread, but can't do anything about it, since doing the handle removal synchronously leads to deadlocks
            TRACE_LINE(LEVEL_VERBOSE, L"Failed to spin up thread for radio handle removal");
        }
        else
        {
            ::SubmitThreadpoolWork(radioInstance->m_ptpRadioHandleRemoval);
        }

        break;

    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED:
        TRACE_LINE(LEVEL_VERBOSE, L"QueryRemoveFailed notification for Radio Instance 0x%p", radioInstance);

        // Unregistering notifications from the callback handle causes deadlocks, so we will spin up a new thread in the default threadpool
        if (NULL == radioInstance->m_ptpRadioQueryRemoveCancel)
        {
            // Failed to spin up the thread, but can't do anything about it, since doing the handle removal synchronously leads to deadlocks
            TRACE_LINE(LEVEL_VERBOSE, L"Failed to spin up thread for radio query cancel");
        }
        else
        {
            ::SubmitThreadpoolWork(radioInstance->m_ptpRadioQueryRemoveCancel);
        }

        break;

    default:
        TRACE_LINE(LEVEL_VERBOSE, L"Unhandled notification - %x for Radio Instance 0x%p", Action, radioInstance);
        break;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, dwResult);
    return dwResult;
}

CNfcRadioInstance::CNfcRadioInstance():
    m_deviceInstanceID(NULL),
    m_deviceFriendlyName(NULL),
    m_hDevice(INVALID_HANDLE_VALUE),
    m_hDevNotification(NULL),
    m_ptpRadioHandleRemoval(NULL),
    m_ptpRadioQueryRemoveCancel(NULL),
    m_fReregisterEnabled(true)
{
    InitializeCriticalSection(&m_csHandleLock);
}

HRESULT CNfcRadioInstance::FinalRelease()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    EnterCriticalSection(&m_csHandleLock);

    m_fReregisterEnabled = false;

    LeaveCriticalSection(&m_csHandleLock);

    RadioHandleRemoval();

    if (NULL != m_ptpRadioQueryRemoveCancel)
    {
        WaitForThreadpoolWorkCallbacks(m_ptpRadioQueryRemoveCancel, TRUE);
        CloseThreadpoolWork(m_ptpRadioQueryRemoveCancel);
        m_ptpRadioQueryRemoveCancel = NULL;
    }

    if (NULL != m_ptpRadioHandleRemoval)
    {
        WaitForThreadpoolWorkCallbacks(m_ptpRadioHandleRemoval, TRUE);
        CloseThreadpoolWork(m_ptpRadioHandleRemoval);
        m_ptpRadioHandleRemoval = NULL;
    }

    m_radioManager = NULL;

    if (NULL != m_deviceInstanceID)
    {
        SysFreeString(m_deviceInstanceID);
        m_deviceInstanceID = NULL;
    }

    if (NULL != m_deviceFriendlyName)
    {
        free(m_deviceFriendlyName);
        m_deviceFriendlyName = NULL;
    }

    DeleteCriticalSection(&m_csHandleLock);

    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
    return S_OK;
}

HRESULT CNfcRadioInstance::Init(_In_ const LPCWSTR DeviceInstanceID, _In_ const LPWSTR FriendlyName, _In_ CNfcRadioManager* pRadioManager)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if ((NULL == DeviceInstanceID) || (NULL == FriendlyName) || (NULL == pRadioManager))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        // Take a shallow pointer to the Radio Manager
        // Shallow pointer is needed here since only the Radio Manager knows when the whole component is going away when it receives FinalRelease
        // Instance should not exist without RadioManager
        m_radioManager = pRadioManager;

        m_deviceInstanceID = SysAllocString(DeviceInstanceID);
        if (NULL == m_deviceInstanceID)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_ptpRadioHandleRemoval = CreateThreadpoolWork(
            CNfcRadioInstance::BeginRadioHandleRemoval,
            this,
            NULL);
        if (NULL == m_ptpRadioHandleRemoval)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        m_ptpRadioQueryRemoveCancel = CreateThreadpoolWork(
            CNfcRadioInstance::BeginRadioQueryRemoveCancel,
            this,
            NULL);
        if (NULL == m_ptpRadioQueryRemoveCancel)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // Open device handle and register for device notifications
    if (SUCCEEDED(hr))
    {
        hr = RegisterDevice();
    }

    if (SUCCEEDED(hr))
    {
        // Take ownership of the friendlyname at the end because any failure before this will need to un-own the friendly name
        TRACE_LINE(LEVEL_VERBOSE, "FriendlyName on Init: %S", FriendlyName);
        m_deviceFriendlyName = FriendlyName;
    }
    else
    {
        // Clean up

        if (NULL != m_ptpRadioQueryRemoveCancel)
        {
            CloseThreadpoolWork(m_ptpRadioQueryRemoveCancel);
            m_ptpRadioQueryRemoveCancel = NULL;
        }

        if (NULL != m_ptpRadioHandleRemoval)
        {
            CloseThreadpoolWork(m_ptpRadioHandleRemoval);
            m_ptpRadioHandleRemoval = NULL;
        }

        if (NULL != m_deviceInstanceID)
        {
            SysFreeString(m_deviceInstanceID);
            m_deviceInstanceID = NULL;
        }
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);

    return hr;
}

//
//IRadioInstance Methods
//
STDMETHODIMP CNfcRadioInstance::GetRadioManagerSignature(
    __out GUID *pGuid)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if (NULL == pGuid)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pGuid = __uuidof(NfcRadioManager);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

STDMETHODIMP CNfcRadioInstance::GetInstanceSignature(
    __deref_out BSTR *pbstrID)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if (NULL == pbstrID) 
    {
        hr =  E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pbstrID = SysAllocString(m_deviceInstanceID);

        if (NULL == *pbstrID)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;

}

STDMETHODIMP CNfcRadioInstance::GetFriendlyName(
    _In_opt_ LCID /*lcid*/,
    __deref_out BSTR *pbstrName)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if (NULL == pbstrName) 
    {
        hr =  E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pbstrName = SysAllocString(m_deviceFriendlyName);
    
        if (NULL == *pbstrName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


STDMETHODIMP CNfcRadioInstance::GetRadioState(
    __out DEVICE_RADIO_STATE * pRadioState)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    BOOLEAN bMediaRadioOn = FALSE;

    hr = GetNfcRadioState(m_hDevice, &bMediaRadioOn);

    if (SUCCEEDED(hr))
    {
        *pRadioState = bMediaRadioOn ? DRS_RADIO_ON : DRS_SW_RADIO_OFF;
        TRACE_LINE(LEVEL_VERBOSE, L"CNfcRadioInstance::GetRadioState - 0x%x", *pRadioState);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


STDMETHODIMP CNfcRadioInstance::SetRadioState(
    _In_opt_ DEVICE_RADIO_STATE radioState,
    _In_ UINT32 uTimeoutSec)
{    
    UNREFERENCED_PARAMETER(uTimeoutSec);

    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    BOOLEAN bMediaRadioOn = FALSE;

    if (DRS_RADIO_ON == radioState)
    {
        bMediaRadioOn = TRUE;
    } 
    else if (DRS_SW_RADIO_OFF != radioState)
    {
        // We only support turning the radio to software on or off. If a command comes in to set any other state, it is an invalid argument
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        TRACE_LINE(LEVEL_VERBOSE, L"Setting radio to %i", bMediaRadioOn);
        hr = SetNfcRadioState(m_hDevice, FALSE, bMediaRadioOn);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioInstance::SetSystemState(_In_opt_ SYSTEM_RADIO_STATE sysRadioState)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    BOOLEAN bMediaRadioOn = FALSE;
    DEVICE_RADIO_STATE drsCurrent = DRS_HW_RADIO_OFF_UNCONTROLLABLE;

    if (SRS_RADIO_ENABLED == sysRadioState)
    {
        bMediaRadioOn = TRUE;
    } 
    else if (SRS_RADIO_DISABLED != sysRadioState)
    {
        // We only support turning the radio to software on or off. If a command comes in to set any other state, it is an invalid argument
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        TRACE_LINE(LEVEL_VERBOSE, L"SystemRadioState %i - Setting radio to %i", sysRadioState, bMediaRadioOn);
        hr = SetNfcRadioState(m_hDevice, TRUE, bMediaRadioOn);
    }

    if (SUCCEEDED(hr))
    {
        // Check to see if our system radio change turned the radio on or not
        hr = GetRadioState(&drsCurrent);
    }

    if ((S_OK != hr) ||
        (DRS_RADIO_ON == drsCurrent))
    {
        // Radio Manager always assumes radio off after any system state command. We will need to alert the RM if the radio comes on
        hr = m_radioManager->FireOnInstanceChange(this);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}
