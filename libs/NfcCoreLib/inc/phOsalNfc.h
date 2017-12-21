/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <stdlib.h>
#include <windows.h>
#include <memory.h>

#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNfcCompId.h>

#define phOsalNfc_SetMemory memset
#define phOsalNfc_MemCopy memcpy

#define PH_OSALNFC_MAX_WAITTIME (INFINITE)

#define PH_OSALNFC_MESSAGE_BASE 0x500

#define PH_OSALNFC_TIMER_MSG                (PH_OSALNFC_MESSAGE_BASE + 0x1)
#define PH_OSALNFC_DEFERRED_CALLBACK        (PH_OSALNFC_MESSAGE_BASE + 0x2)
#define PH_OSALNFC_EXIT_THREAD              (PH_OSALNFC_MESSAGE_BASE + 0x3)

typedef VOID (*phOsalNfc_MsgFunc_t)(_Inout_ void* pContext, _In_ uint32_t Message, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2, _In_ UINT_PTR Param3, _In_ UINT_PTR Param4);

/*!
 * \ingroup grp_osal_nfc
 *
 * OSAL Message structure contains message specific details like,
 * message type, message specific data block details
 */
typedef struct phOsalNfc_Message
{
    uint32_t eMsgType;  /**< Type of the message to be posted */
    void*    pMsgData;  /**< Pointer to message specific data block in case any */
    uint16_t Size;      /**< Size of the datablock */
} phOsalNfc_Message_t,*pphOsalNfc_Message_t;

/*!
 * \ingroup grp_osal_nfc
 * Enum definition contains supported exception types
 */
typedef enum
{
    phOsalNfc_e_NoMemory,                       /**< Memory allocation failed */
    phOsalNfc_e_PrecondFailed,                  /**< precondition wasn't met */
    phOsalNfc_e_InternalErr,                    /**< Unrecoverable error */
    phOsalNfc_e_UnrecovFirmwareErr,             /**< Unrecoverable firmware error */
    phOsalNfc_e_DALerror,                       /**< Unrecoverable DAL error */
    phOsalNfc_e_Noerror                         /**< No errortype */
} phOsalNfc_ExceptionType_t;

/*!
 * \ingroup grp_osal_nfc
 * Configuration for the OSAL
 */
typedef struct phOsalNfc_Config
{
    uint32_t dwCallbackThreadId;            /**< Thread ID of the message thread */
    phOsalNfc_MsgFunc_t pfnCallback;        /**< Callback to the message handler */
    void* pCallbackContext;                 /**< Callback context to the message function */
} phOsalNfc_Config_t, *pphOsalNfc_Config_t; /**< Pointer to #phOsalNfc_Config_t */

/*!
 * \ingroup grp_osal_nfc
 * Deferred call declaration.
 * This type of API is called from ClientApplication (main thread) to notify
 * specific callback.
 */
typedef void (*pphOsalNfc_DeferFuncPointer_t) (void*);

/*!
 * \ingroup grp_osal_nfc
 * Deferred message specific info declaration.
 * This type information packed as UINT_PTR when windows message
 * is posted to message handler thread.
 */
typedef struct phOsalNfc_DeferedCallInfo
{
    pphOsalNfc_DeferFuncPointer_t   pDeferredCall;  /**< pointer to Deferred callback */
    void*                           pParam;         /**< contains timer message specific details */
} phOsalNfc_DeferedCallInfo_t;

/**
 * \ingroup grp_osal_nfc
 * \brief OSAL initialization.
 * This function initializes Timer queue and Critical section variable.
 *
 * \param[in] pOsalConfig                   Osal Configuration file.
 * \retval #NFCSTATUS_SUCCESS               Initialization of OSAL module was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER     Client Thread ID passed is invalid.
 * \retval #NFCSTATUS_ALREADY_INITIALISED   Osal Module is already Initialized.
 * \retval #NFCSTATUS_FAILED                Initialization of OSAL module was not successful.
 */
NFCSTATUS phOsalNfc_Init(pphOsalNfc_Config_t pOsalConfig);

/**
 * \ingroup grp_osal_nfc
 * \brief OSAL deinitialization.
 * This function De-Initializes the Objects and Memory acquired during
 * Initialization. This function also closes the objects if any of it is still open.
 */
void phOsalNfc_DeInit(void);

/**
 * \ingroup grp_osal_nfc
 * This API allows to delay the current thread execution.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] dwDelay  Duration in milliseconds for which thread execution to be halted.
 */
void phOsalNfc_Delay(uint32_t dwDelay);

/*!
 * \ingroup grp_osal_nfc
 * \brief Raises exception
 *
 * The program jumps out of the current execution flow, i.e. this function doesn't return.
 * The given exception contains information on what has happened and how severe the error is.
 * @warning This function should only be used for exceptional error situations where there is no
 * means to recover.
 *
 * \param[in] eExceptiontype  exception Type.
 * \param[in] reason     This is an additional reason value that gives a vendor specific reason code
 *
 * \retval  None
 */
void phOsalNfc_RaiseException(phOsalNfc_ExceptionType_t eExceptiontype, uint16_t reason);

/*!
 * \ingroup grp_osal_nfc
 * \brief Converts a NFCSTATUS error code to NTSTATUS
 */
LONG phOsalNfc_NtStatusFromNfcStatus(NFCSTATUS nfcStatus);

/*!
 * \ingroup grp_osal_nfc
 * \brief Converts a NTSTATUS error code to NFCSTATUS
 */
NFCSTATUS phOsalNfc_NfcStatusFromNtStatus(LONG ntStatus);

/*!
 * \ingroup grp_osal_nfc
 * \brief Function use to post a message to the integration thread.
 *
 * \param[in] Message       The type of message to be posted
 * \param[in] Param1        Additional message-specific information
 * \param[in] Param2        Additional message-specific information
 * \param[in] Param3        Additional message-specific information
 * \param[in] Param4        Additional message-specific information
 */
void phOsalNfc_PostMsg(_In_ uint32_t Message, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2, _In_ UINT_PTR Param3, _In_ UINT_PTR Param4);

/*!
 * \ingroup grp_osal_nfc
 * \brief Queues a deferred callback to be executed
 */
NFCSTATUS phOsalNfc_QueueDeferredCallback(_In_ pphOsalNfc_DeferFuncPointer_t DeferredCallback, _In_ void* Context);

/*!
 * \ingroup grp_osal_nfc
 * \brief Allocates some memory.
 *
 * \param[in] Size   Size, in uint8_t, to be allocated
 *
 * \retval NON-NULL value:  The memory was successfully allocated; the return value points to the allocated memory location
 * \retval NULL:            The operation was not successful, certainly because of insufficient resources.
 *
 */
_Must_inspect_result_
_When_(return!=0, __drv_allocatesMem(Mem))
_Ret_maybenull_ _Post_writable_byte_size_(Size)
PVOID FORCEINLINE
phOsalNfc_GetMemory(
    _In_ uint32_t Size
    )
{
    void* pMem = (void*)malloc(Size);
    return pMem;
}

/*!
 * \ingroup grp_osal_nfc
 * \brief This API allows to free already allocated memory.
 * \param[in] pMem  Pointer to the memory block to deallocated
 * \retval None
 */
VOID FORCEINLINE
phOsalNfc_FreeMemory(
    _In_ __drv_freesMem(Mem) PVOID Memory
    )
{
    if (NULL != Memory) {
        free(Memory);
    }
}

/*!
 * \ingroup grp_osal_nfc
 * \brief Compares the values stored in the source memory with the
 * values stored in the destination memory.
 *
 * \param[in] src   Pointer to the Source Memory
 * \param[in] dest  Pointer to the Destination Memory
 * \param[in] n     Number of bytes to be compared.
 *
 * \retval zero if the comparison is successful, non-zero otherwise
 */
int FORCEINLINE
phOsalNfc_MemCompare(
    _In_reads_bytes_(n) const void* src,
    _In_reads_bytes_(n) const void* dest,
    _In_ unsigned int n
    )
{
    return memcmp(src, dest, n);
}
