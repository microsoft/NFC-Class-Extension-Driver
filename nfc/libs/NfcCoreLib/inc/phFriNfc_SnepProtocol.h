/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
* =============================================================================
*/

#pragma once

#include <Basetsd.h>
#include <Windows.h>
#include <phLibNfc.h>
#include <phNfcStatus.h>
#include <phOsalNfc.h>

/* SNEP Version details (length in octets)*/
#define SNEP_VERSION_LENGTH 1
#define SNEP_VERSION_MAJOR  1
#define SNEP_VERSION_MINOR  0
#define SNEP_VERSION_FIELD   4

/* SNEP Request details*/
#define SNEP_REQUEST_LENGTH 1
#define SNEP_REQUEST_CONTINUE 0x00
#define SNEP_REQUEST_GET      0x01
#define SNEP_REQUEST_PUT      0x02
#define SNEP_REQUEST_REJECT   0x7F

/* Response field values */
#define SNEP_RESPONSE_CONTINUE              0x80
#define SNEP_RESPONSE_SUCCESS               0x81
#define SNEP_RESPONSE_NOT_FOUND             0xC0
#define SNEP_RESPONSE_EXCESS_DATA           0xC1
#define SNEP_RESPONSE_BAD_REQUEST           0xC2
#define SNEP_RESPONSE_NOT_IMPLEMENTED       0xE0
#define SNEP_RESPONSE_UNSUPPORTED_VERSION   0xE1
#define SNEP_RESPONSE_REJECT                0xFF

#define SNEP_REQUEST_LENGTH_SIZE   4
#define SNEP_REQUEST_ACCEPTABLE_LENGTH_SIZE 4

#define SNEP_HEADER_SIZE (SNEP_VERSION_LENGTH+SNEP_REQUEST_LENGTH+SNEP_REQUEST_LENGTH_SIZE)
#define SNEP_VERSION (SNEP_VERSION_MAJOR*16)+SNEP_VERSION_MINOR

/* Response field values */
#define NFCSTATUS_SNEP_RESPONSE_CONTINUE              0x50
#define NFCSTATUS_SNEP_RESPONSE_SUCCESS               0x51
#define NFCSTATUS_SNEP_RESPONSE_NOT_FOUND             0x52
#define NFCSTATUS_SNEP_RESPONSE_EXCESS_DATA           0x53
#define NFCSTATUS_SNEP_RESPONSE_BAD_REQUEST           0x54
#define NFCSTATUS_SNEP_RESPONSE_NOT_IMPLEMENTED       0x55
#define NFCSTATUS_SNEP_RESPONSE_UNSUPPORTED_VERSION   0x56
#define NFCSTATUS_SNEP_RESPONSE_REJECT                0x57
#define NFCSTATUS_SNEP_REQUEST_REJECT                 0x58
#define NFCSTATUS_SNEP_INVALID_PROTOCOL_DATA          0x59
#define NFCSTATUS_SNEP_REQUEST_CONTINUE_FAILED        0x60
#define NFCSTATUS_SNEP_REQUEST_REJECT_FAILED          0x61

/* Packet types */
typedef enum
{
    phLibNfc_SnepPut = 0x00,   /* Put request by client */
    phLibNfc_SnepGet,          /* Get request */
    phLibNfc_SnepContinue,
    phLibNfc_SnepSuccess,
    phLibNfc_SnepNotFound,
    phLibNfc_SnepExcessData,
    phLibNfc_SnepBadRequest,
    phLibNfc_SnepNotImplemented,
    phLibNfc_SnepUnsupportedVersion,
    phLibNfc_SnepReject,
    phLibNfc_SnepInvalid
} phLibNfc_SnepPacket_t;

typedef void(*pphLibNfc_SnepProtocol_SendRspComplete_t) (void* pContext, NFCSTATUS  Status, phLibNfc_Handle ConnHandle);

/** SNEP Get Data Context structure */
typedef struct putGetDataContext
{
    uint32_t        iDataSent;         /* count of data sent so far */
    uint32_t        iDataReceived;     /* count of data received so far */
    phNfc_sData_t   *pSnepPacket;      /* prepared snep packet */
    BOOL            bWaitForContinue;  /* Do we need to wait for continue before send? */
    BOOL            bContinueReceived; /* Have we received continue to send */
    phNfc_sData_t   *pReqResponse;     /* response data to be sent back to upper layer */
    phNfc_sData_t   *pProcessingBuffer;  /* Processing and temp buffer for receive */
}putGetDataContext_t, *pputGetDataContext_t;

/** SNEP Response Data Context structure */
typedef struct sendResponseDataContext
{
    uint32_t        iAcceptableLength; /* acceptable length */
    BOOL            bIsExcessData;     /* Excess data flag */
    uint32_t        iDataSent;         /* count of data sent so far */
    phNfc_sData_t   *pSnepPacket;      /* prepared snep packet */
    BOOL            bWaitForContinue;  /* Do we need to wait for continue before send? */
    BOOL            bContinueReceived; /* Have we received continue to send */
    phNfc_sData_t   *pProcessingBuffer;  /* Processing buffer for send*/
    pphLibNfc_SnepProtocol_SendRspComplete_t fSendCompleteCb;
    void            *cbContext;        /* pointer to the call back context */
}sendResponseDataContext_t, *psendResponseDataContext_t;
