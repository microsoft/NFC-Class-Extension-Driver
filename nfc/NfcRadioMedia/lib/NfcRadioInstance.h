/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioInstance.h
    
Abstract:
    This module defines the radio manager instances that each represent a single NFC radio
    Facilitates IOCTL calls down to the class driver.

Notes:

Environment:
   User mode.

--*/


#pragma once


// Forward declaration for BthRadioManager
class CNfcRadioManager;

class ATL_NO_VTABLE CNfcRadioInstance: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CNfcRadioInstance>,
    public IRadioInstance
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CNfcRadioInstance)
    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CNfcRadioInstance)
        COM_INTERFACE_ENTRY(IRadioInstance)
    END_COM_MAP()

    //IRadioInstance
    STDMETHODIMP GetRadioManagerSignature(
        __out GUID *pGuid);
    
    STDMETHODIMP GetInstanceSignature(
        __deref_out BSTR *pbstrID);

    STDMETHODIMP GetFriendlyName(
        _In_opt_ LCID lcid,
        __deref_out BSTR *pbstrName);

    STDMETHODIMP GetRadioState(
        __out DEVICE_RADIO_STATE * pRadioState);

    STDMETHODIMP SetRadioState(
        _In_opt_ DEVICE_RADIO_STATE radioState,
        _In_ UINT32 uTimeoutSec);

    BOOL STDMETHODCALLTYPE IsMultiComm() { return FALSE; }

    BOOL STDMETHODCALLTYPE IsAssociatingDevice() { return FALSE; }

    // Constructor / Destuctor
    CNfcRadioInstance();

    HRESULT FinalRelease();

    //Helper methods
    HRESULT Init(_In_ LPCWSTR DeviceInstanceID, _In_ LPWSTR FriendlyName, _In_ CNfcRadioManager* pRadioManager); //will initialize the instance for correct values

    // Turns off the radio as part of system radio state changes
    HRESULT SetSystemState(_In_opt_ SYSTEM_RADIO_STATE sysRadioState);

    // Callback from device notification registration
    static DWORD NfcProcessDeviceChange(
        _In_ HCMNOTIFICATION       hNotification,          
        _In_ PVOID                 Context,  
        _In_ CM_NOTIFY_ACTION      Action,  
        _In_ PCM_NOTIFY_EVENT_DATA EventData,  
        _In_ DWORD                 EventDataSize  
        );

    // Worker threads for unregistering and closing handles
    static VOID CALLBACK BeginRadioHandleRemoval(PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork);
    static VOID CALLBACK BeginRadioQueryRemoveCancel(PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork);

private:
    HRESULT RegisterDevice();
    VOID RadioHandleRemoval();
    VOID RadioQueryRemoveCancel();
    VOID RadioQueryRemoval();

private:
    BSTR m_deviceInstanceID;
    LPWSTR m_deviceFriendlyName;
    CNfcRadioManager* m_radioManager;
    CRITICAL_SECTION m_csHandleLock;
    HANDLE m_hDevice;
    HCMNOTIFICATION m_hDevNotification;
    PTP_WORK m_ptpRadioHandleRemoval;
    PTP_WORK m_ptpRadioQueryRemoveCancel;
    bool m_fReregisterEnabled;
};

