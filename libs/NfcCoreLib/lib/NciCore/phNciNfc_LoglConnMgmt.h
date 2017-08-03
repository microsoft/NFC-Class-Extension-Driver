/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfc_Core.h"

#define CONNRFTYPE_STATIC               (0x00U)  /**< Static RF connection id/index */
#define CONNHCITYPE_STATIC              (0x01U)  /**< Static HCI connection id/index */
#define MAX_LOGICAL_CONNS               (0x03U)  /**< Maximum number of Logical Connections Supported */
#define INVALID_CONN_ID                 (0xFFU)  /**< Range beyond 15 is invalid connid */
#define UNASSIGNED_DESTID               (0xFFU)  /**< Unassigned destination id */
#define FLOW_CONTROL_DISABLED           (0xFFU)  /**< Flow control disabled during data exchange */
#define MAX_CREDITS_LIMIT               (0xFEU)  /**< Max possible Credits for a connection */

#define PHNCINFC_MIN_WAITCREDIT_TO      (250)

typedef struct phNciNfc_LogConn_Rsp
{
    uint8_t     bConnId;           /**< NFCC assigned Connection Identifier */
    uint8_t     bNumCredits;       /**< Initial Number of Credits for this Connection */
    uint8_t     bMaxDpldSize;      /**< Max Data Packet Payload Size */
}phNciNfc_LogConn_Rsp_t;

typedef struct phNciNfc_LogConn_Info
{
    uint8_t bDestId;                    /**< NFCC/RF id */
    phNciNfc_DestType_t bDestType;      /**< Destination Type */
    phNciNfc_LogConn_Rsp_t tConn;       /**< Connection Response info */
    void *pActvDevHandle;               /**< Handle of the device currently active */
    bool_t  bIfActive;                  /**< Flag to check if interface is active or not */
}phNciNfc_LogConn_Info_t;

typedef struct phNciNfc_LogConn_Int
{
    uint8_t bOpenConns;                 /**< Currently Open/Created Connections */
    phNciNfc_LogConn_Info_t tConnList[MAX_LOGICAL_CONNS+1];  /**< List of Connections */
}phNciNfc_LogConn_Int_t;

typedef struct phNciNfc_CreditWaitTimer
{
    uint32_t   dwTimerId;                  /**< Timer for LogConn to handle credit waiting */
    uint8_t    TimerStatus;                /**< 0 = Timer not running 1 = timer running*/
}phNciNfc_CreditWaitTimer_t;

typedef struct phNciNfc_LogConnMgmt_Int
{
    phNciNfc_LogConn_Int_t tConnInfo;          /**<Internal Information of all the connections created and opened*/
    uint8_t bConnId;                           /**< ConnId for waiting for credits */
    pphNciNfc_ConnCreditsNtf_t pCrdtsNotify;   /**< Credits awaiting caller CB */
    void     *pContext;                        /**< Upper layer context */
    bool_t bCreditsAwaited;                    /**< Flag to indicate Core is waiting for data credits */
    phNciNfc_CreditWaitTimer_t tCWTimerIf;     /**< Timer context used for credit wait */
}phNciNfc_LogConnMgmt_Int_t;

extern
NFCSTATUS
phNciNfc_LogConnMgmtInit();

extern
void
phNciNfc_LogConnMgmtDeInit();

/* Connection Management Functions */

extern
uint8_t
phNciNfc_GetConnCount();

extern
NFCSTATUS
phNciNfc_CreateConn(
                    uint8_t bDestId,
                    phNciNfc_DestType_t bDestType
                    );

extern
NFCSTATUS
phNciNfc_CloseConn(
                   uint8_t bConnId
                   );

extern
NFCSTATUS
phNciNfc_GetConnId(
                   void     *pDevHandle,
                   uint8_t  *pConnId
                   );

extern
NFCSTATUS
phNciNfc_GetConnInfo(
                     uint8_t    bDestId,
                     phNciNfc_DestType_t tDestType,
                     uint8_t    *pConnId
                     );

extern
NFCSTATUS
phNciNfc_UpdateConnInfo(
                        uint8_t bDestId,
                        phNciNfc_DestType_t tDestType,
                        uint8_t bConnId,
                        uint8_t bInitialCredits,
                        uint8_t bMaxDpldSize
                        );

extern
NFCSTATUS
phNciNfc_UpdateConnDestInfo(
                        uint8_t bDestId,
                        phNciNfc_DestType_t tDestType,
                        void *pHandle
                    );

/* Credit Based Flow Control Functions */

extern
NFCSTATUS
phNciNfc_GetConnCredits(
                        uint8_t   bConnId,
                        uint8_t  *pCredits
                        );

extern
NFCSTATUS
phNciNfc_GetConnMaxPldSz(
                           uint8_t   bConnId,
                           uint8_t *pMaxPldSz
                           );

extern
NFCSTATUS
phNciNfc_IncrConnCredits(
                          uint8_t bConnId,
                          uint8_t bVal
                          );

extern
NFCSTATUS
phNciNfc_DecrConnCredit(
                      uint8_t bConnId
                      );

extern
NFCSTATUS
phNciNfc_RegForConnCredits(
                            uint8_t bConnId,
                            pphNciNfc_ConnCreditsNtf_t pNotify,
                            void *pContext,
                            uint32_t CreditTo
                          );

extern
NFCSTATUS
phNciNfc_ProcessConnCreditNtf(
                        void *psContext,
                        void *pInfo,
                        NFCSTATUS wStatus
                        );
