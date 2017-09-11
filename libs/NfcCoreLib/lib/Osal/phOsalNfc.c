/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phOsalNfc_Pch.h"

#include "phOsalNfc.tmh"

typedef struct phOsalNfc_ErrorMapping {
    NFCSTATUS   NfcStatus;
    NTSTATUS    NtStatus;
} phOsalNfc_ErrorMapping_t;

static const phOsalNfc_ErrorMapping_t g_NfcToNTStatusMap [] = {
    {NFCSTATUS_SUCCESS,                     STATUS_SUCCESS},
    {NFCSTATUS_INVALID_PARAMETER,           STATUS_INVALID_PARAMETER},
    {NFCSTATUS_BUFFER_TOO_SMALL,            STATUS_BUFFER_TOO_SMALL},
    {NFCSTATUS_INVALID_DEVICE,              STATUS_INVALID_DEVICE_REQUEST},
    {NFCSTATUS_MORE_INFORMATION,            STATUS_BUFFER_TOO_SMALL},
    {NFCSTATUS_RF_TIMEOUT,                  STATUS_IO_TIMEOUT},
    {NFCSTATUS_RF_ERROR,                    STATUS_LINK_FAILED},
    {NFCSTATUS_INSUFFICIENT_RESOURCES,      STATUS_INSUFFICIENT_RESOURCES},
    {NFCSTATUS_PENDING,                     STATUS_PENDING},
    {NFCSTATUS_BOARD_COMMUNICATION_ERROR,   STATUS_IO_DEVICE_ERROR},
    {NFCSTATUS_INVALID_STATE,               STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_NOT_INITIALISED,             STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_ALREADY_INITIALISED,         STATUS_SUCCESS},
    {NFCSTATUS_FEATURE_NOT_SUPPORTED,       STATUS_NOT_SUPPORTED},
    {NFCSTATUS_NOT_REGISTERED,              STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_ALREADY_REGISTERED,          STATUS_ALREADY_REGISTERED},
    {NFCSTATUS_MULTIPLE_PROTOCOLS,          STATUS_SUCCESS},
    {NFCSTATUS_MULTIPLE_TAGS,               STATUS_SUCCESS},
    {NFCSTATUS_DESELECTED,                  STATUS_LINK_FAILED},
    {NFCSTATUS_RELEASED,                    STATUS_LINK_FAILED},
    {NFCSTATUS_NOT_ALLOWED,                 STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_BUSY,                        STATUS_DEVICE_BUSY},
    {NFCSTATUS_INVALID_REMOTE_DEVICE,       STATUS_INVALID_DEVICE_REQUEST},
    {NFCSTATUS_SMART_TAG_FUNC_NOT_SUPPORTED, STATUS_NOT_SUPPORTED},
    {NFCSTATUS_READ_FAILED,                 STATUS_INVALID_NETWORK_RESPONSE},
    {NFCSTATUS_WRITE_FAILED,                STATUS_INVALID_NETWORK_RESPONSE},
    {NFCSTATUS_NO_NDEF_SUPPORT,             STATUS_NOT_SUPPORTED},
    {NFCSTATUS_EOF_NDEF_CONTAINER_REACHED,  STATUS_UNSUCCESSFUL},
    {NFCSTATUS_INVALID_RECEIVE_LENGTH,      STATUS_UNSUCCESSFUL},
    {NFCSTATUS_INVALID_FORMAT,              STATUS_UNSUCCESSFUL},
    {NFCSTATUS_INSUFFICIENT_STORAGE,        STATUS_UNSUCCESSFUL},
    {NFCSTATUS_FORMAT_ERROR,                STATUS_INVALID_NETWORK_RESPONSE},
    {NFCSTATUS_CREDIT_TIMEOUT,              STATUS_IO_TIMEOUT},
    {NFCSTATUS_RESPONSE_TIMEOUT,            STATUS_IO_TIMEOUT},
    {NFCSTATUS_ALREADY_CONNECTED,           STATUS_SUCCESS},
    {NFCSTATUS_ANOTHER_DEVICE_CONNECTED,    STATUS_DEVICE_ALREADY_ATTACHED},
    {NFCSTATUS_SINGLE_TAG_ACTIVATED,        STATUS_SUCCESS},
    {NFCSTATUS_SINGLE_TAG_DISCOVERED,       STATUS_SUCCESS},
    {NFCSTATUS_SECURE_ELEMENT_ACTIVATED,    STATUS_SUCCESS},
    {NFCSTATUS_UNKNOWN_ERROR,               STATUS_UNSUCCESSFUL},
    {NFCSTATUS_FAILED,                      STATUS_UNSUCCESSFUL},
    {NFCSTATUS_CMD_ABORTED,                 STATUS_CANCELLED},
    {NFCSTATUS_MULTI_POLL_NOT_SUPPORTED,    STATUS_NOT_SUPPORTED},
    {NFCSTATUS_NO_DEVICE_FOUND,             STATUS_NOT_FOUND},
    {NFCSTATUS_NO_TARGET_FOUND,             STATUS_NOT_FOUND},
    {NFCSTATUS_NO_DEVICE_CONNECTED,         STATUS_DEVICE_NOT_CONNECTED},
    {NFCSTATUS_EXTERNAL_RF_DETECTED,        STATUS_SUCCESS},
    {NFCSTATUS_MSG_NOT_ALLOWED_BY_FSM,      STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_ACCESS_DENIED,               STATUS_ACCESS_DENIED},
    {NFCSTATUS_NODE_NOT_FOUND,              STATUS_NOT_FOUND},
    {NFCSTATUS_SMX_BAD_STATE,               STATUS_UNSUCCESSFUL},
    {NFCSTATUS_ABORT_FAILED,                STATUS_UNSUCCESSFUL},
    {NFCSTATUS_REG_OPMODE_NOT_SUPPORTED,    STATUS_NOT_SUPPORTED},
    {NFCSTATUS_SHUTDOWN,                    STATUS_INVALID_DEVICE_STATE},
    {NFCSTATUS_TARGET_LOST,                 STATUS_LINK_FAILED},
    {NFCSTATUS_REJECTED,                    STATUS_DEVICE_BUSY},
    {NFCSTATUS_TARGET_NOT_CONNECTED,        STATUS_LINK_FAILED},
    {NFCSTATUS_INVALID_HANDLE,              STATUS_INVALID_HANDLE},
    {NFCSTATUS_ABORTED,                     STATUS_CANCELLED},
    {NFCSTATUS_COMMAND_NOT_SUPPORTED,       STATUS_NOT_SUPPORTED},
    {NFCSTATUS_NON_NDEF_COMPLIANT,          STATUS_UNSUCCESSFUL},
    {NFCSTATUS_NOT_ENOUGH_MEMORY,           STATUS_INSUFFICIENT_RESOURCES},
    {NFCSTATUS_INCOMING_CONNECTION,         STATUS_SUCCESS},
    {NFCSTATUS_CONNECTION_SUCCESS,          STATUS_SUCCESS},
    {NFCSTATUS_CONNECTION_FAILED,           STATUS_UNSUCCESSFUL},
};

static phOsalNfc_Context_t* gpphOsalNfc_Context = NULL;

phOsalNfc_Context_t* phOsalNfc_GetContext()
{
    return gpphOsalNfc_Context;
}

void phOsalNfc_SetContext(_In_opt_ phOsalNfc_Context_t* pOsalContext)
{
    gpphOsalNfc_Context = pOsalContext;
}

void phOsalNfc_RaiseException(phOsalNfc_ExceptionType_t eExceptiontype, uint16_t reason)
{
    PHNFC_UNUSED_VARIABLE(reason);
    
    if ((eExceptiontype> phOsalNfc_e_NoMemory) && (eExceptiontype <= phOsalNfc_e_DALerror))
    {
        RaiseException(0, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
}

NFCSTATUS phOsalNfc_CreateMutex(void **hMutex)
{
    NFCSTATUS wStatus = NFCSTATUS_SUCCESS;
    LPCRITICAL_SECTION pLock = NULL;
    PH_LOG_OSAL_FUNC_ENTRY();

    if (NULL != hMutex)
    {
        pLock = (LPCRITICAL_SECTION) malloc(sizeof(CRITICAL_SECTION));

        if (NULL != pLock)
        {
            InitializeCriticalSection(pLock);
            *hMutex = pLock;
        }
        else
        {
            PH_LOG_OSAL_INFO_STR("OSAL out of memory");
            wStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
    }
    else
    {
        PH_LOG_OSAL_INFO_STR("OSAL Invalid parameter");
        wStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    PH_LOG_OSAL_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS phOsalNfc_Init(pphOsalNfc_Config_t pOsalConfig)
{
    NFCSTATUS wInitStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    PH_LOG_OSAL_FUNC_ENTRY();

    /* Check whether OSAL is already Initialized */
    if(NULL == pOsalContext)
    {
        if( (NULL != pOsalConfig) &&
            (NULL != pOsalConfig->pfnCallback) &&
            (NULL != pOsalConfig->pCallbackContext) )
        {
            pOsalContext = phOsalNfc_GetMemory(sizeof(phOsalNfc_Context_t));
            if(NULL != pOsalContext)
            {
                phOsalNfc_SetMemory((void *)pOsalContext,0x00,sizeof(phOsalNfc_Context_t));
                phOsalNfc_SetContext(pOsalContext);

                wInitStatus = phOsalNfc_MsgQueue_Init();
                if(NFCSTATUS_SUCCESS == wInitStatus)
                {
                    pOsalContext->pfnCallback = pOsalConfig->pfnCallback;
                    pOsalContext->pCallbackContext = pOsalConfig->pCallbackContext;
                    pOsalConfig->dwCallbackThreadId = pOsalContext->dwCallbackThreadID;
                    phOsalNfc_Timer_Init();
                }
                else
                {
                    wInitStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_FAILED);
                }
            }
            else
            {
                PH_LOG_OSAL_INFO_STR("OSAL out of memory");
                wInitStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_FAILED);
            }
        }
        else
        {
            PH_LOG_OSAL_INFO_STR("OSAL invalid parameters");
            wInitStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        PH_LOG_OSAL_INFO_STR("OSAL already initialized");
        wInitStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_ALREADY_INITIALISED);
    }

    PH_LOG_OSAL_FUNC_EXIT();
    return wInitStatus;
}

void phOsalNfc_DeInit(void )
{
    PH_LOG_OSAL_FUNC_ENTRY();
    phOsalNfc_Context_t* pOsalContext = phOsalNfc_GetContext();

    if(NULL != pOsalContext)
    {
        phOsalNfc_Timer_DeInit();
        phOsalNfc_MsgQueue_DeInit();

        /* Reset all the values of the Context structure,release memory and set to NULL */
        phOsalNfc_SetMemory((void *)pOsalContext, 0x00,sizeof(phOsalNfc_Context_t));

        phOsalNfc_FreeMemory((void *)pOsalContext);
        phOsalNfc_SetContext(NULL);
    }
    PH_LOG_OSAL_FUNC_EXIT();
}

void phOsalNfc_Delay(uint32_t dwDelay)
{
    PH_LOG_OSAL_FUNC_ENTRY();
    Sleep(dwDelay);
    PH_LOG_OSAL_FUNC_EXIT();
}

LONG phOsalNfc_NtStatusFromNfcStatus(NFCSTATUS nfcStatus)
{
    for(uint16_t i = 0; i < ARRAYSIZE(g_NfcToNTStatusMap); i++)
    {
        if(g_NfcToNTStatusMap[i].NfcStatus == PHNFCSTATUS(nfcStatus)) {
            return g_NfcToNTStatusMap[i].NtStatus;
        }
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

NFCSTATUS phOsalNfc_NfcStatusFromNtStatus(LONG ntStatus)
{
    for(uint16_t i = 0; i < ARRAYSIZE(g_NfcToNTStatusMap); i++)
    {
        if(g_NfcToNTStatusMap[i].NtStatus == ntStatus) {
            return g_NfcToNTStatusMap[i].NfcStatus;
        }
    }

    return NFCSTATUS_INVALID_PARAMETER;
}

NFCSTATUS
phOsalNfc_QueueDeferredCallback(
    _In_ pphOsalNfc_DeferFuncPointer_t  DeferredCallback,
    _In_ void*                          Context
    )
{
    phOsalNfc_PostMsg(PH_OSALNFC_DEFERRED_CALLBACK, (UINT_PTR) DeferredCallback, (UINT_PTR) Context, (UINT_PTR) NULL, (UINT_PTR) NULL);
    return NFCSTATUS_SUCCESS;
}
