/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioManager.cpp
    
Abstract:
    This module implements the Radio Manager object for the NFC Radio Media Manager

Notes:

Environment:
   User mode.

--*/

#include "NfcCxRMPch.h"
#include "NfcRadioManager.tmh"

DECLARE_OBJECT_ENTRY_AUTO(__uuidof(NfcRadioManager), CNfcRadioManager);

VOID WINAPI CNfcRadioManager::NFCInterfaceEnumCallback(
    _In_ HDEVQUERY hDevQuery,
    _In_opt_ PVOID pContext,
    _In_ const DEV_QUERY_RESULT_ACTION_DATA *pActionData
    )
{
    UNREFERENCED_PARAMETER(hDevQuery);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    CNfcRadioManager* pNfcRM = (CNfcRadioManager*) pContext;
    LPWSTR friendlyName = NULL;
    DEVPROPCOMPKEY propKeyFriendlyName[] = {{DEVPKEY_Device_FriendlyName, DEVPROP_STORE_SYSTEM, NULL}};
    ULONG devPropCount = 0;
    DEVPROPERTY* pDevProp = NULL;
    size_t cchFriendlyName = 0;
    
    switch (pActionData->Action)
    {
    case DevQueryResultStateChange:
        TRACE_LINE(LEVEL_INFO, "State change. New state=%d", pActionData->Data.State);
        if (DevQueryStateAborted == pActionData->Data.State)
        {
            //
            // If we hit an error, then we cannot tell if a new device comes or an old device goes. We will destroy the existing list of radios and then restart the query
            //
            hr = pNfcRM->CreateDevQuery();
        }
        else if (DevQueryStateEnumCompleted == pActionData->Data.State)
        {
            TRACE_LINE(LEVEL_INFO, "Initial enumeration complete.");
        }
        break;

    case DevQueryResultUpdate:
    case DevQueryResultAdd:

        // Though we do not expect any Updates to the fields that we are looking into, they will be funneled through Add where they will fail instance creation due to existing instance
        TRACE_LINE(LEVEL_INFO, "Action=%d received for %S", pActionData->Action, pActionData->Data.DeviceObject.pszObjectId);

        //
        // Retrieve the DEVPKEY_Device_FriendlyName property and use it to add a new device
        //
        hr = DevGetObjectProperties(DevObjectTypeDevice,
                                    pActionData->Data.DeviceObject.pszObjectId,
                                    DevQueryFlagNone,
                                    1, propKeyFriendlyName,
                                    &devPropCount, (const DEVPROPERTY**) &pDevProp);
        if (SUCCEEDED(hr))
        {
            cchFriendlyName = pDevProp->BufferSize + 1;
            friendlyName = (LPWSTR) malloc(sizeof(WCHAR) * cchFriendlyName);
            if (NULL != friendlyName)
            {
                CopyMemory(friendlyName, pDevProp->Buffer, pDevProp->BufferSize);
                friendlyName[cchFriendlyName - 1] = L'\0';

                TRACE_LINE(LEVEL_INFO, "DEVPKEY_Device_FriendlyName=%S for %S", friendlyName, pActionData->Data.DeviceObject.pszObjectId);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else // The driver doesn't report a friendly name so load the localized friendlyname
        {
            WCHAR szLocalFriendlyName[MAX_CCH_LOCAL_FRIENDLYNAME];
            ZeroMemory(szLocalFriendlyName, sizeof(szLocalFriendlyName));

            hr = S_OK;
            
            TRACE_LINE(LEVEL_INFO, "AEP query for %S did not return DEVPKEY_Device_FriendlyName property", pActionData->Data.DeviceObject.pszObjectId);
            
            DWORD cchCopied = LoadString(g_hInstance, IDS_NFC_RADIO_FRIENDLYNAME, szLocalFriendlyName, ARRAYSIZE(szLocalFriendlyName));

            if (cchCopied > 0)
            {
                TRACE_LINE(LEVEL_INFO, "Localized Name is %S", szLocalFriendlyName);
                cchFriendlyName = cchCopied;
            }
            else
            {
                TRACE_LINE(LEVEL_INFO, "Nothing found in Resource module");
                StringCchCopy(szLocalFriendlyName, ARRAYSIZE(szLocalFriendlyName), L"NFC");
                hr = StringCchLength(szLocalFriendlyName, STRSAFE_MAX_CCH, &cchFriendlyName);
            }

            if (SUCCEEDED(hr))
            {
                cchFriendlyName = cchFriendlyName + 1; // Additional space for the NULL character
                friendlyName = (LPWSTR) malloc(sizeof(WCHAR) * cchFriendlyName);
                
                if (NULL != friendlyName)
                {
                    CopyMemory(friendlyName, szLocalFriendlyName, cchCopied * sizeof(WCHAR));
                    friendlyName[cchFriendlyName - 1] = L'\0';
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (SUCCEEDED(hr) && (NULL != friendlyName))
        {
            //
            // Adding new radio with properties. NameLength should not matter here because the string should be a null terminated PWSTR.
            // RadioInstance takes ownership of friendlyName
            hr = pNfcRM->AddRadio(pActionData->Data.DeviceObject.pszObjectId, friendlyName);

            // AddRadio has successfully taken ownership of the friendly name
            friendlyName = NULL;
        }
        break;

    case DevQueryResultRemove :
        TRACE_LINE(LEVEL_INFO, "Action=%d received for %S", pActionData->Action, pActionData->Data.DeviceObject.pszObjectId);
        hr = pNfcRM->RemoveRadio(pActionData->Data.DeviceObject.pszObjectId);
        break;

    default:
        TRACE_LINE(LEVEL_INFO, "Action=%d received for %S", pActionData->Action, pActionData->Data.DeviceObject.pszObjectId);
        break;
    }

    if (friendlyName)
    {
        free(friendlyName);
    }

    if (pDevProp)
    {
        DevFreeObjectProperties(devPropCount, pDevProp);
        pDevProp = NULL;
        devPropCount = 0;
    }

    TRACE_FUNCTION_EXIT_HR(LEVEL_VERBOSE, hr);
    return;
}


//---------------------------------------------------------------------------
// Begin implemetation of NfcRadioManager
//---------------------------------------------------------------------------


CNfcRadioManager::CNfcRadioManager():
    m_nfcRadioCollection(NULL),
    m_nfcDevQuery(NULL)
{
    InitializeCriticalSection(&m_csAddRemoveLock);
}

HRESULT CNfcRadioManager::FinalConstruct()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    hr = CComObject<CNfcRadioInstanceCollection>::CreateInstance(&m_nfcRadioCollection);

    if (SUCCEEDED(hr))
    {
        m_nfcRadioCollection->AddRef();
    }

    hr = CreateDevQuery();

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioManager::FinalRelease()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    if (NULL != m_nfcDevQuery)
    {
        DevCloseObjectQuery(m_nfcDevQuery);
        m_nfcDevQuery = NULL;
    }

    if (m_nfcRadioCollection)
    {
        m_nfcRadioCollection->Release();
        m_nfcRadioCollection = NULL;
    }

    DeleteCriticalSection(&m_csAddRemoveLock);

    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
    return S_OK;
}

HRESULT CNfcRadioManager::CreateDevQuery()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    DEVPROP_FILTER_EXPRESSION Filter[2];

    const DEVPROP_BOOLEAN devprop_true = DEVPROP_TRUE;

    // Close any preexisting queries
    if (NULL != m_nfcDevQuery)
    {
        DevCloseObjectQuery(m_nfcDevQuery);
        m_nfcDevQuery = NULL;
    }
    
    Filter[0].Operator = DEVPROP_OPERATOR_EQUALS;
    Filter[0].Property.CompKey.Key = DEVPKEY_DeviceInterface_ClassGuid;
    Filter[0].Property.CompKey.Store = DEVPROP_STORE_SYSTEM;
    Filter[0].Property.CompKey.LocaleName = NULL;
    Filter[0].Property.Type = DEVPROP_TYPE_GUID;
    Filter[0].Property.BufferSize = sizeof(GUID);
    Filter[0].Property.Buffer = (void*)&GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE;

    Filter[1].Operator = DEVPROP_OPERATOR_EQUALS;
    Filter[1].Property.CompKey.Key = DEVPKEY_DeviceInterface_Enabled;
    Filter[1].Property.CompKey.Store = DEVPROP_STORE_SYSTEM;
    Filter[1].Property.CompKey.LocaleName = NULL;
    Filter[1].Property.Type = DEVPROP_TYPE_BOOLEAN;
    Filter[1].Property.BufferSize = sizeof(DEVPROP_BOOLEAN);
    Filter[1].Property.Buffer = (void*)&devprop_true;
        
    hr = DevCreateObjectQuery(DevObjectTypeDeviceInterface,
                            DevQueryFlagUpdateResults,
                            0, NULL,
                            2, &Filter[0],
                            NFCInterfaceEnumCallback,
                            this,
                            &m_nfcDevQuery);
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;

}


//
//IMediaRadioManager
//
STDMETHODIMP CNfcRadioManager::GetRadioInstances(
    __deref_out_opt IRadioInstanceCollection** ppCollection)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if (NULL == ppCollection)
    {
        hr =  E_INVALIDARG;
    }
    else
    {
        *ppCollection = m_nfcRadioCollection;
        (*ppCollection)->AddRef();
    }
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

//
// This function is invoked only with Airplane mode change, but not with NFC radio state change.
//
STDMETHODIMP CNfcRadioManager::OnSystemRadioStateChange(
        _In_opt_ SYSTEM_RADIO_STATE sysRadioState,
        _In_ UINT32 uTimeoutSec)
{
    UNREFERENCED_PARAMETER(sysRadioState);
    UNREFERENCED_PARAMETER(uTimeoutSec);

    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    PTP_POOL threadPool = NULL;
    TP_CALLBACK_ENVIRON callbackEnviron;
    PTP_CLEANUP_GROUP ptpCleanupGroup = NULL;
    
    // Create threadpool to handle all of the radios
    threadPool = CreateThreadpool(NULL);
    if (NULL == threadPool)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        SetThreadpoolThreadMaximum(threadPool, 50);
        InitializeThreadpoolEnvironment(&callbackEnviron);
        SetThreadpoolCallbackPool(&callbackEnviron, threadPool);
        ptpCleanupGroup = CreateThreadpoolCleanupGroup();

        if (NULL == ptpCleanupGroup)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            // Associate the cleanup group with our thread pool
            SetThreadpoolCallbackCleanupGroup(&callbackEnviron,
                                                ptpCleanupGroup,
                                                NULL );
        }
    }

    // Lock so that any adds or removes will not cause list changes during system airplane mode
    EnterCriticalSection(&m_csAddRemoveLock);

    if (SUCCEEDED(hr))
    {
        UINT32 i, count = 0;
        SYSTEM_STATE_SWITCH_CONTEXT* pContext = NULL;

        hr = m_nfcRadioCollection->GetCount(&count);

        if (count > 0)
        {
            if (SUCCEEDED(hr))
            {
                pContext = (SYSTEM_STATE_SWITCH_CONTEXT*)malloc(count * sizeof(SYSTEM_STATE_SWITCH_CONTEXT));
                if (NULL == pContext)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            
            for (i = 0; (SUCCEEDED(hr) && (i < count)); i++)
            {
                PTP_WORK ptpThreadWork = NULL;
                
                pContext[i].sysRadioState = sysRadioState;
                m_nfcRadioCollection->GetAt(i, (IRadioInstance**)&(pContext[i].pRadioInstance));
                
                ptpThreadWork = CreateThreadpoolWork( 
                                     (PTP_WORK_CALLBACK)&CNfcRadioManager::AsyncRadioChange, 
                                     &(pContext[i]), 
                                     &(callbackEnviron) );
                
                if (ptpThreadWork == NULL)
                {   
                    // pRadioInstance context was not successfully added to threadpool.
                    hr = pContext[i].pRadioInstance->SetSystemState(sysRadioState);
                    pContext[i].pRadioInstance->Release();
                }
                else
                {
                    ::SubmitThreadpoolWork(ptpThreadWork);
                }
            }
        }

        // Wait for all threadpools to drain and clean up threads
        CloseThreadpoolCleanupGroupMembers(ptpCleanupGroup, FALSE, NULL);

        if (NULL != pContext)
        {
            free(pContext);
        }
    }
    else
    {
        // Failed to set up threadpool for parallel system radio change. Only choice is to do it serially now
        UINT32 i, count = 0;
        CNfcRadioInstance* pRadioInstance = NULL;

        hr = m_nfcRadioCollection->GetCount(&count);
        for (i = 0; (SUCCEEDED(hr) && (i < count)); i++)
        {
            hr = m_nfcRadioCollection->GetAt(i, (IRadioInstance**)&pRadioInstance);
            if (SUCCEEDED(hr))
            {
                hr = pRadioInstance->SetSystemState(sysRadioState);
            }
            
            pRadioInstance->Release();
        }
    }

    LeaveCriticalSection(&m_csAddRemoveLock);

    DestroyThreadpoolEnvironment(&callbackEnviron);

    if(ptpCleanupGroup)
    {
        CloseThreadpoolCleanupGroup(ptpCleanupGroup);
        ptpCleanupGroup = NULL;
    }
  
    if(threadPool)
    {
        CloseThreadpool(threadPool);
        threadPool = NULL;
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


HRESULT CNfcRadioManager::FireOnInstanceAdd(_In_ IRadioInstance *pRadioInstance)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    
    //
    // Use ATL standard lock to be sync with Advise()/UnAdvise()
    // This can make sure sink is still alive when event is raised
    //
    Lock();
    for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
    {
        if ((nullptr != ppUnkSrc) && (nullptr != *ppUnkSrc))
        {
            CComPtr<IMediaRadioManagerNotifySink> spSink;

            hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));
            if (SUCCEEDED(hr))
            {
                spSink->OnInstanceAdd(pRadioInstance);
                TRACE_LINE(LEVEL_VERBOSE, L"FireOnInstanceAdd completed successfully");
            }
        }
    }
    Unlock();

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioManager::FireOnInstanceRemove(_In_ BSTR bstrRadioInstanceID)
{
    UNREFERENCED_PARAMETER(bstrRadioInstanceID);

    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    
    HRESULT hr = S_OK;
    
    //
    // Use ATL standard lock to be sync with Advise()/UnAdvise()
    // This can make sure sink is still alive when event is raised
    //
    Lock();
    for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
    {
        if ((nullptr != ppUnkSrc) && (nullptr != *ppUnkSrc))
        {
            CComPtr<IMediaRadioManagerNotifySink> spSink;

            hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));
            if (SUCCEEDED(hr))
            {
                spSink->OnInstanceRemove(bstrRadioInstanceID);
                TRACE_LINE(LEVEL_VERBOSE, L"FireOnInstanceRemove completed successfully");
            }
        }
    }
    Unlock();

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioManager::FireOnInstanceChange(
    _In_ IRadioInstance *pnfcRadioInstanceObj
    )
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    DEVICE_RADIO_STATE radioStateCurrent = DRS_HW_RADIO_OFF_UNCONTROLLABLE;
    BSTR bstrInstanceID = NULL;
        
    hr = pnfcRadioInstanceObj->GetInstanceSignature(&bstrInstanceID);

    if (SUCCEEDED(hr))
    {
        hr = pnfcRadioInstanceObj->GetRadioState(&radioStateCurrent);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Use ATL standard lock to be sync with Advise()/UnAdvise()
        // This can make sure sink is still alive when event is raised
        //
        Lock();

        for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
        {
            if ((NULL != ppUnkSrc) && (NULL != *ppUnkSrc))
            {
                CComPtr<IMediaRadioManagerNotifySink> spSink;

                hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));

                if (SUCCEEDED(hr))
                {
                    hr = spSink->OnInstanceRadioChange(bstrInstanceID, radioStateCurrent);

                    if (FAILED(hr))
                    {
                        TRACE_LINE(LEVEL_ERROR, L"NotifySink OnInstanceRadioChange Failed");
                    }
                    else
                    {
                        TRACE_LINE(LEVEL_VERBOSE, L"FireOnInstanceChange completed successfully");
                    }
                }
            }
        }
        Unlock();
    }

    if (bstrInstanceID)
    {
        SysFreeString(bstrInstanceID);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


HRESULT CNfcRadioManager::AddRadio( _In_ LPCWSTR DeviceInstanceID, _In_ LPWSTR FriendlyName)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    
    HRESULT hr = S_OK;
    CComObject<CNfcRadioInstance> *pInstance = NULL;

    hr = CComObject<CNfcRadioInstance>::CreateInstance(&pInstance);

    if (SUCCEEDED(hr))
    {
        pInstance->AddRef();
        hr = pInstance->Init(DeviceInstanceID, FriendlyName, this);
    }

    // AddRadio passes ownership of FriendlyName to the Instance on successful Init
    // Otherwise, clean it up here to differentiate from AddRadioInstance and FireOnInstanceAdd failures

    if ((S_OK != hr) && (FriendlyName))
    {
        delete [] FriendlyName;
    }

    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_csAddRemoveLock);
        
        hr = m_nfcRadioCollection->AddRadioInstance(pInstance);

        if (S_OK == hr) // Explicitly check for S_OK
        {
            hr = FireOnInstanceAdd(pInstance);
        }
        
        LeaveCriticalSection(&m_csAddRemoveLock);
    }

    if (pInstance)
    {
        pInstance->Release();
        pInstance = NULL;
    }
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioManager::RemoveRadio(_In_ LPCWSTR radioInstanceSignature)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    
    HRESULT hr = S_OK;
    BSTR bstrRadioInstance = NULL;

    if (NULL == radioInstanceSignature)
    {
        hr = E_INVALIDARG;
    }

    bstrRadioInstance = SysAllocString(radioInstanceSignature);
    if (NULL == bstrRadioInstance)
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_csAddRemoveLock);
        
        hr = m_nfcRadioCollection->RemoveRadioInstance(radioInstanceSignature);
        
        if (S_OK == hr) // Explicitly check for S_OK
        {
            hr = FireOnInstanceRemove(bstrRadioInstance);
        }
        
        LeaveCriticalSection(&m_csAddRemoveLock);
    }

    if (NULL != bstrRadioInstance)
    {
        SysFreeString(bstrRadioInstance);
    }

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


VOID CALLBACK CNfcRadioManager::AsyncRadioChange(PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork)
{
    UNREFERENCED_PARAMETER(ptpInstance);
    UNREFERENCED_PARAMETER(ptpWork);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    SYSTEM_STATE_SWITCH_CONTEXT* pSystemContext = (SYSTEM_STATE_SWITCH_CONTEXT*)pContext;

    if (NULL != pSystemContext && NULL != pSystemContext->pRadioInstance)
    {
        hr = (pSystemContext->pRadioInstance)->SetSystemState(pSystemContext->sysRadioState);
        (pSystemContext->pRadioInstance)->Release();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TRACE_FUNCTION_EXIT_HR(LEVEL_VERBOSE, hr);
}
