/*
*          Modifications Copyright ? Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
*/

#include "phOsalNfc_Pch.h"

#include "phOsalNfc_MsgQueue.tmh"

static DWORD WINAPI phOsalNfc_MsgQueue_Thread(void* pContext);

NFCSTATUS phOsalNfc_MsgQueue_Init(void)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    PH_LOG_OSAL_FUNC_ENTRY();

    InitializeCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);

    gpphOsalNfc_Context->hMsgQueueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(gpphOsalNfc_Context->hMsgQueueEvent != NULL)
    {
        gpphOsalNfc_Context->hCallbackThread = CreateThread(NULL, 0, phOsalNfc_MsgQueue_Thread, NULL, CREATE_SUSPENDED, &gpphOsalNfc_Context->dwCallbackThreadID);

        if(gpphOsalNfc_Context->hCallbackThread != NULL)
        {
            SetThreadPriority(gpphOsalNfc_Context->hCallbackThread, THREAD_PRIORITY_TIME_CRITICAL);
            ResumeThread(gpphOsalNfc_Context->hCallbackThread);
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

    if(gpphOsalNfc_Context != NULL)
    {
        if(gpphOsalNfc_Context->hCallbackThread != NULL)
        {
            phOsalNfc_PostMsg(PH_OSALNFC_EXIT_THREAD, (UINT_PTR)NULL, (UINT_PTR)NULL, (UINT_PTR)NULL, (UINT_PTR)NULL);
            WaitForSingleObject(gpphOsalNfc_Context->hCallbackThread, INFINITE);

            CloseHandle(gpphOsalNfc_Context->hCallbackThread);
            gpphOsalNfc_Context->hCallbackThread = NULL;
        }

        if(gpphOsalNfc_Context->hMsgQueueEvent != NULL)
        {
            CloseHandle(gpphOsalNfc_Context->hMsgQueueEvent);
            gpphOsalNfc_Context->hMsgQueueEvent = NULL;
        }

        DeleteCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);

        gpphOsalNfc_Context->bMsgQueueHead = 0;
        gpphOsalNfc_Context->bMsgQueueTail = 0;
        gpphOsalNfc_Context->bMsgQueueUsed = 0;
    }

    PH_LOG_OSAL_FUNC_EXIT();
}

void phOsalNc_GetMsg(_Out_ uint32_t *pMessage, _Out_ UINT_PTR * pParam1, _Out_ UINT_PTR * pParam2, _Out_ UINT_PTR * pParam3, _Out_ UINT_PTR * pParam4)
{
    bool_t bMessageAvailable = FALSE;

    NT_ASSERT(gpphOsalNfc_Context->dwCallbackThreadID == GetCurrentThreadId());

    while(!bMessageAvailable)
    {
        WaitForSingleObject(gpphOsalNfc_Context->hMsgQueueEvent, INFINITE);

        EnterCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);
        bMessageAvailable = gpphOsalNfc_Context->bMsgQueueUsed > 0;

        if(bMessageAvailable)
        {
            *pMessage = gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueHead].Message;
            *pParam1 = gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueHead].Param1;
            *pParam2 = gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueHead].Param2;
            *pParam3 = gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueHead].Param3;
            *pParam4 = gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueHead].Param4;

            gpphOsalNfc_Context->bMsgQueueHead = ((gpphOsalNfc_Context->bMsgQueueHead+1) < PH_OSAL_MSG_QUEUE_MAX_SIZE) ? gpphOsalNfc_Context->bMsgQueueHead+1 : 0; 
            gpphOsalNfc_Context->bMsgQueueUsed--;

            if(gpphOsalNfc_Context->bMsgQueueUsed == 0) {
                ResetEvent(gpphOsalNfc_Context->hMsgQueueEvent);
            }
        }

        LeaveCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);
    }
}

void phOsalNfc_PostMsg(_In_ uint32_t Message, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2, _In_ UINT_PTR Param3, _In_ UINT_PTR Param4)
{
    phOsalNfc_MsgQueue_t msgEntry = {0};

    msgEntry.Message = Message;
    msgEntry.Param1 = Param1;
    msgEntry.Param2 = Param2;
    msgEntry.Param3 = Param3;
    msgEntry.Param4 = Param4;

    EnterCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);

    if(gpphOsalNfc_Context->bMsgQueueUsed < PH_OSAL_MSG_QUEUE_MAX_SIZE)
    {
        phOsalNfc_MemCopy(&gpphOsalNfc_Context->MsgQueue[gpphOsalNfc_Context->bMsgQueueTail], &msgEntry, sizeof(phOsalNfc_MsgQueue_t));
        gpphOsalNfc_Context->bMsgQueueTail = ((gpphOsalNfc_Context->bMsgQueueTail+1) < PH_OSAL_MSG_QUEUE_MAX_SIZE) ? gpphOsalNfc_Context->bMsgQueueTail+1 : 0;
        gpphOsalNfc_Context->bMsgQueueUsed++;
        SetEvent(gpphOsalNfc_Context->hMsgQueueEvent);
    }
    else
    {
        //
        // This condition is only hit when the messaging thread is unresponsive and is not draining the message
        // queue and hence this is a fatal condition
        //
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }

    LeaveCriticalSection(&gpphOsalNfc_Context->MsgQueueLock);
}

static DWORD WINAPI phOsalNfc_MsgQueue_Thread(void* pContext)
{
    bool_t bContinue = TRUE;
    uint32_t message;
    UINT_PTR param1, param2, param3, param4;

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
            gpphOsalNfc_Context->pfnCallback(gpphOsalNfc_Context->pCallbackContext, message, param1, param2, param3, param4);
        }
    }

    PH_LOG_OSAL_FUNC_EXIT();
    return ERROR_SUCCESS;
}
