/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioInstanceCollection.h
    
Abstract:
    This module defines the Radio Instance Collection object for the NFC Radio Media Manager

Notes:

Environment:
    User mode.

--*/

#pragma once

typedef struct _NFC_RADIO_NODE {
    CNfcRadioInstance* nfcRadioInstance;
    _NFC_RADIO_NODE* next;
} NFC_RADIO_NODE, *PNFC_RADIO_NODE;

class ATL_NO_VTABLE CNfcRadioInstanceCollection: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CNfcRadioInstanceCollection>,
    public IRadioInstanceCollection
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CNfcRadioInstanceCollection)

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CNfcRadioInstanceCollection)
        COM_INTERFACE_ENTRY(IRadioInstanceCollection)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalRelease();

    //IRadioInstanceCollection
    STDMETHODIMP GetCount(
        UINT32 *uInstance);

    STDMETHODIMP GetAt(
        _In_opt_ UINT32 uIndex,
        __deref_out_opt IRadioInstance **ppRadioInstance);

    //Internal methods
    HRESULT AddRadioInstance(_In_ CNfcRadioInstance* nfcObj);

    HRESULT RemoveRadioInstance(_In_ LPCWSTR radioInstanceSignature);
    
    // Constructor / Destuctor
    CNfcRadioInstanceCollection();


protected:
    PNFC_RADIO_NODE m_listHead;
    UINT32 m_listLength;
    CRITICAL_SECTION m_csListLock; // All functions require locking for accurate list state
};


