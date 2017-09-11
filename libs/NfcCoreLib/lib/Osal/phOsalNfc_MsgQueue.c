/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phOsalNfc_Pch.h"

#include "phOsalNfc_MsgQueue.tmh"

static DWORD WINAPI phOsalNfc_MsgQueue_Thread(void* pContext);

NFCSTATUS phOsalNfc_MsgQueue_Init(void)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    PH_LOG_OSAL_FUNC_ENTRY();

    InitializeCriticalSection(&pOsalContext->MsgQueueLock);

    pOsalContext->hMsgQueueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(pOsalContext->hMsgQueueEvent != NULL)
    {
        pOsalContext->hCallbackThread = CreateThread(NULL, 0, phOsalNfc_MsgQueue_Thread, NULL, CREATE_SUSPENDED, (LPDWORD)&pOsalContext->dwCallbackThreadID);

        if(pOsalContext->hCallbackThread != NULL)
        {
            SetThreadPriority(pOsalContext->hCallbackThread, THREAD_PRIORITY_TIME_CRITICAL);
            ResumeThread(pOsalContext->hCallbackThread);
        }
        else
        {
            wStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INSUFFICIENT_RESOURCES);
            PH_LOG_OSAL_INFO_STR("OSAL out of memory");
        }
    }
    else
    {
        wStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INSUFFICIENT_RESOURCES);
        PH_LOG_OSAL_INFO_STR("OSAL out of memory");
    }

    PH_LOG_OSAL_FUNC_EXIT();
    return wStatus;
}

void phOsalNfc_MsgQueue_DeInit(void)
{
    PH_LOG_OSAL_FUNC_ENTRY();
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    if(pOsalContext != NULL)
    {
        if(pOsalContext->hCallbackThread != NULL)
        {
            phOsalNfc_PostMsg(PH_OSALNFC_EXIT_THREAD, (UINT_PTR)NULL, (UINT_PTR)NULL, (UINT_PTR)NULL, (UINT_PTR)NULL);
            WaitForSingleObject(pOsalContext->hCallbackThread, INFINITE);

            CloseHandle(pOsalContext->hCallbackThread);
            pOsalContext->hCallbackThread = NULL;
        }

        if(pOsalContext->hMsgQueueEvent != NULL)
        {
            CloseHandle(pOsalContext->hMsgQueueEvent);
            pOsalContext->hMsgQueueEvent = NULL;
        }

        DeleteCriticalSection(&pOsalContext->MsgQueueLock);

        pOsalContext->bMsgQueueHead = 0;
        pOsalContext->bMsgQueueTail = 0;
        pOsalContext->bMsgQueueUsed = 0;
    }

    PH_LOG_OSAL_FUNC_EXIT();
}

void phOsalNc_GetMsg(_Out_ uint32_t *pMessage, _Out_ UINT_PTR * pParam1, _Out_ UINT_PTR * pParam2, _Out_ UINT_PTR * pParam3, _Out_ UINT_PTR * pParam4)
{
    bool_t bMessageAvailable = FALSE;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    NT_ASSERT(pOsalContext->dwCallbackThreadID == GetCurrentThreadId());

    while(!bMessageAvailable)
    {
        WaitForSingleObject(pOsalContext->hMsgQueueEvent, INFINITE);

        EnterCriticalSection(&pOsalContext->MsgQueueLock);
        bMessageAvailable = pOsalContext->bMsgQueueUsed > 0;

        if(bMessageAvailable)
        {
            *pMessage = pOsalContext->MsgQueue[pOsalContext->bMsgQueueHead].Message;
            *pParam1 = pOsalContext->MsgQueue[pOsalContext->bMsgQueueHead].Param1;
            *pParam2 = pOsalContext->MsgQueue[pOsalContext->bMsgQueueHead].Param2;
            *pParam3 = pOsalContext->MsgQueue[pOsalContext->bMsgQueueHead].Param3;
            *pParam4 = pOsalContext->MsgQueue[pOsalContext->bMsgQueueHead].Param4;

            pOsalContext->bMsgQueueHead = ((pOsalContext->bMsgQueueHead+1) < PH_OSAL_MSG_QUEUE_MAX_SIZE) ? pOsalContext->bMsgQueueHead+1 : 0; 
            pOsalContext->bMsgQueueUsed--;

            if(pOsalContext->bMsgQueueUsed == 0) {
                ResetEvent(pOsalContext->hMsgQueueEvent);
            }
        }

        LeaveCriticalSection(&pOsalContext->MsgQueueLock);
    }
}

void phOsalNfc_PostMsg(_In_ uint32_t Message, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2, _In_ UINT_PTR Param3, _In_ UINT_PTR Param4)
{
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    phOsalNfc_MsgQueue_t msgEntry = {0};
    msgEntry.Message = Message;
    msgEntry.Param1 = Param1;
    msgEntry.Param2 = Param2;
    msgEntry.Param3 = Param3;
    msgEntry.Param4 = Param4;

    EnterCriticalSection(&pOsalContext->MsgQueueLock);

    if(pOsalContext->bMsgQueueUsed < PH_OSAL_MSG_QUEUE_MAX_SIZE)
    {
        phOsalNfc_MemCopy(&pOsalContext->MsgQueue[pOsalContext->bMsgQueueTail], &msgEntry, sizeof(phOsalNfc_MsgQueue_t));
        pOsalContext->bMsgQueueTail = ((pOsalContext->bMsgQueueTail+1) < PH_OSAL_MSG_QUEUE_MAX_SIZE) ? pOsalContext->bMsgQueueTail+1 : 0;
        pOsalContext->bMsgQueueUsed++;
        SetEvent(pOsalContext->hMsgQueueEvent);
    }
    else
    {
        //
        // This condition is only hit when the messaging thread is unresponsive and is not draining the message
        // queue and hence this is a fatal condition
        //
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }

    LeaveCriticalSection(&pOsalContext->MsgQueueLock);
}

static DWORD WINAPI phOsalNfc_MsgQueue_Thread(void* pContext)
{
    bool_t bContinue = TRUE;
    uint32_t message;
    UINT_PTR param1, param2, param3, param4;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    PH_LOG_OSAL_FUNC_ENTRY();

    UNUSED(pContext);

    while(bContinue)
    {
        phOsalNc_GetMsg(&message, &param1, &param2, &param3, &param4);

        if(message >= PH_OSALNFC_MESSAGE_BASE)
        {
            PH_LOG_OSAL_INFO_STR("%x: %p, %p", message, (void*)param1, (void*)param2);

            switch(message)
            {
                case PH_OSALNFC_DEFERRED_CALLBACK:
                case PH_OSALNFC_TIMER_MSG:
                {
                    pphOsalNfc_DeferFuncPointer_t pfnDeferredCallback = (pphOsalNfc_DeferFuncPointer_t)param1;
                    
                    if(pfnDeferredCallback != NULL) {
                        (*pfnDeferredCallback)((VOID*)param2);
                    }
                }
                break;

                case PH_OSALNFC_EXIT_THREAD:
                {
                    bContinue = FALSE;
                }
                break;
            }
        }
        else
        {
            pOsalContext->pfnCallback(pOsalContext->pCallbackContext, message, param1, param2, param3, param4);
        }
    }

    PH_LOG_OSAL_FUNC_EXIT();
    return ERROR_SUCCESS;
}
