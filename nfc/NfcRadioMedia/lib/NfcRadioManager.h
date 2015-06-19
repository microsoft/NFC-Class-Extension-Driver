/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioInstance.h
    
Abstract:
    This module defines the radio manager instance for Radio Management

Notes:

Environment:
   User mode.

--*/

#pragma once

#define MAX_CCH_LOCAL_FRIENDLYNAME MAX_PATH

typedef struct _SYSTEM_STATE_SWITCH_CONTEXT {
    CNfcRadioInstance* pRadioInstance;
    SYSTEM_RADIO_STATE sysRadioState;
} SYSTEM_STATE_SWITCH_CONTEXT, *PSYSTEM_STATE_SWITCH_CONTEXT;

class ATL_NO_VTABLE CNfcRadioManager:
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CNfcRadioManager, &CLSID_NfcRadioManager>,
    public IConnectionPointContainerImpl<CNfcRadioManager>,
    public IConnectionPointImpl<CNfcRadioManager, &IID_IMediaRadioManagerNotifySink>,
    public IMediaRadioManager
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CNfcRadioManager)
    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CNfcRadioManager)
        COM_INTERFACE_ENTRY(IMediaRadioManager)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CNfcRadioManager)
        CONNECTION_POINT_ENTRY(IID_IMediaRadioManagerNotifySink)
    END_CONNECTION_POINT_MAP()

    CNfcRadioManager();

    HRESULT FinalConstruct();
    HRESULT FinalRelease();

    //IMediaRadioManager
    STDMETHODIMP GetRadioInstances(
        __deref_out_opt IRadioInstanceCollection** ppCollection);
    
    STDMETHODIMP OnSystemRadioStateChange(
        _In_opt_ SYSTEM_RADIO_STATE sysRadioState,
        _In_ UINT32 uTimeoutSec);

    HRESULT FireOnInstanceAdd(
        _In_ IRadioInstance *pRadioInstance
        );

    HRESULT FireOnInstanceRemove(
        _In_ BSTR bstrRadioInstanceID
        );

    HRESULT FireOnInstanceChange(
        _In_ IRadioInstance *pnfcRadioInstanceObj
        );

    // Devquery callback
    static VOID WINAPI NFCInterfaceEnumCallback(
        _In_ HDEVQUERY hDevQuery,
        _In_opt_ PVOID pContext,
        _In_ const DEV_QUERY_RESULT_ACTION_DATA *pActionData
        );

    // Threadpool callback
    static VOID CALLBACK AsyncRadioChange(
        PTP_CALLBACK_INSTANCE ptpInstance, 
        PVOID pContext, 
        PTP_WORK ptpWork
        );

    // Helper function called when device removal is detected by the instance
    HRESULT RemoveRadio(
        _In_ LPCWSTR radioInstanceSignature
        );
    
private:
    HRESULT AddRadio(
        _In_ LPCWSTR DeviceInstanceID,
        _In_ LPWSTR FriendlyName
        );

    HRESULT CreateDevQuery();
        
private:
    CComObject<CNfcRadioInstanceCollection> * m_nfcRadioCollection;
    HDEVQUERY m_nfcDevQuery;
    CRITICAL_SECTION m_csAddRemoveLock;
};
