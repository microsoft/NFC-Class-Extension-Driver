/*++

Copyright (c) 2013 Microsoft Corporation

Module Name:
    NfcRadioInstanceCollection.cpp
    
Abstract:
    This module implements the Radio Instance Collection object for the NFC Radio Media Manager

Notes:

Environment:
   User mode.

--*/

#include "NfcCxRMPch.h"
#include "NfcRadioInstanceCollection.tmh" 

CNfcRadioInstanceCollection::CNfcRadioInstanceCollection()
{
    m_listHead = NULL;
    m_listLength = 0;
    InitializeCriticalSection(&m_csListLock);
}

HRESULT CNfcRadioInstanceCollection::FinalRelease()
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);
    
    EnterCriticalSection(&m_csListLock);

    // Free all references for objects in the list and clear up node memory
    while (m_listHead != NULL)
    {
        PNFC_RADIO_NODE tempHead = m_listHead;
        m_listHead = m_listHead->next;

        tempHead->nfcRadioInstance->Release();
        delete tempHead;

        m_listLength--;
    }
    LeaveCriticalSection(&m_csListLock);

    DeleteCriticalSection(&m_csListLock);
    
    TRACE_METHOD_EXIT(LEVEL_VERBOSE);
    return S_OK;
}

HRESULT CNfcRadioInstanceCollection::AddRadioInstance(_In_ CNfcRadioInstance* nfcObj)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    PNFC_RADIO_NODE tempNode = NULL;

    if (NULL == nfcObj)
    {
        hr = E_INVALIDARG;
    }

    EnterCriticalSection(&m_csListLock);

    if (SUCCEEDED(hr))
    {
        tempNode = (NFC_RADIO_NODE*)malloc(sizeof(NFC_RADIO_NODE));
        if (NULL != tempNode)
        {
            tempNode->nfcRadioInstance = nfcObj;
            tempNode->next = NULL;
            tempNode->nfcRadioInstance->AddRef();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        // Add the instance to the end of the list to avoid potential conflicts with GetAt and radio instance positions changing
        if (NULL == m_listHead)
        {
            // First entry in the collection
            m_listHead = tempNode;
            tempNode = NULL;
        }
        else
        {
            // We must make sure that we are not writing in a radio that is already in the list
            BSTR tempNodeName;
            PNFC_RADIO_NODE traverseNode = m_listHead;
            BOOL bFound = FALSE;
            
            hr = tempNode->nfcRadioInstance->GetInstanceSignature(&tempNodeName);
            if (SUCCEEDED(hr))
            {
                while(NULL != traverseNode->next)
                {
                    BSTR nodeName;

                    hr = traverseNode->nfcRadioInstance->GetInstanceSignature(&nodeName);
                    if (SUCCEEDED(hr))
                    {
                        if (0 == wcscmp(nodeName, tempNodeName))
                        {
                            TRACE_LINE(LEVEL_VERBOSE, "Attemping to add an instance that is already in the list");
                            bFound = TRUE;
                            hr = S_FALSE;
                            SysFreeString(nodeName);
                            break;
                        }
                        SysFreeString(nodeName);
                        traverseNode = traverseNode->next;
                    }
                }

                if (!bFound)
                {
                    BSTR nodeName;

                    hr = traverseNode->nfcRadioInstance->GetInstanceSignature(&nodeName);
                    if (SUCCEEDED(hr))
                    {
                        if (0 == wcscmp(nodeName, tempNodeName))
                        {
                            hr = S_FALSE;
                        }
                        else
                        {
                            traverseNode->next = tempNode;
                            tempNode = NULL;
                        }
                        SysFreeString(nodeName);
                    }
                }

                SysFreeString(tempNodeName);
            }
        }

        m_listLength++;
    }

    if (NULL != tempNode)
    {
        tempNode->nfcRadioInstance->Release();
        free(tempNode);
    }

    LeaveCriticalSection(&m_csListLock);
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

HRESULT CNfcRadioInstanceCollection::RemoveRadioInstance(_In_ LPCWSTR radioInstanceSignature)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;
    BOOL bFound = FALSE;

    EnterCriticalSection(&m_csListLock);

    if ((NULL == m_listHead) || (NULL == radioInstanceSignature))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // Search through the whole list for a matching radio instance signature
        PNFC_RADIO_NODE traverseNode = NULL;
        BSTR nodeInstanceSignature;
        
        traverseNode = m_listHead;

        hr = traverseNode->nfcRadioInstance->GetInstanceSignature(&nodeInstanceSignature);
        if (SUCCEEDED(hr))
        {
            if (0 == wcscmp(nodeInstanceSignature, radioInstanceSignature))
            {
                bFound = TRUE;
                m_listHead = m_listHead->next;
            
                traverseNode->nfcRadioInstance->Release();
                delete traverseNode;
                traverseNode = NULL;
            }

            SysFreeString(nodeInstanceSignature);
        }
        
        // Wasn't the list head, so look forward in the list for the matching radio instance signature
        while ((!bFound) && ( NULL != traverseNode->next ))
        {
            hr = traverseNode->next->nfcRadioInstance->GetInstanceSignature(&nodeInstanceSignature);
            if (SUCCEEDED(hr))
            {
                if (0 == wcscmp(nodeInstanceSignature, radioInstanceSignature))
                {
                    PNFC_RADIO_NODE targetNode = traverseNode->next;
                    bFound = TRUE;
                
                    traverseNode->next = targetNode->next;

                    targetNode->nfcRadioInstance->Release();
                    delete targetNode;
                }

                SysFreeString(nodeInstanceSignature);
            }
            
            traverseNode = traverseNode->next;
        }
    }

    // If we weren't able to find it, we couldn't remove it so we will return S_FALSE
    if (!bFound)
    {
        hr = S_FALSE;
    }
    else
    {
        m_listLength--;
    }

    LeaveCriticalSection(&m_csListLock);
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}


//
//IRadioInstanceCollection Methods
//
STDMETHODIMP CNfcRadioInstanceCollection::GetCount(
    UINT32 *uInstance)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    if (NULL == uInstance)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        EnterCriticalSection(&m_csListLock);
        *uInstance = m_listLength;
        LeaveCriticalSection(&m_csListLock);
    }
    
    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

STDMETHODIMP CNfcRadioInstanceCollection::GetAt(
    _In_opt_ UINT32 uIndex,
    __deref_out_opt IRadioInstance **ppRadioInstance)
{
    TRACE_METHOD_ENTRY(LEVEL_VERBOSE);

    HRESULT hr = S_OK;

    TRACE_LINE(LEVEL_VERBOSE, "Requesting object at %i. List length is %i", uIndex, m_listLength);

    EnterCriticalSection(&m_csListLock);
    
    // Validate the uIndex is at a valid position since uIndex is 0 based
    if (m_listLength > uIndex)
    {
        UINT32 traverseIndex = 0;
        PNFC_RADIO_NODE traverseNode = m_listHead;

        // Traverse the list till we get to the desired point
        while ( traverseIndex < uIndex )
        {
            traverseNode = traverseNode->next;
            traverseIndex++;
        }

        *ppRadioInstance = traverseNode->nfcRadioInstance;
        (*ppRadioInstance)->AddRef();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    LeaveCriticalSection(&m_csListLock);

    TRACE_METHOD_EXIT_HR(LEVEL_COND, hr);
    return hr;
}

