/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNciNfc.h>

/* Watch dog time for callback */ 
#define PH_LIBNFC_WD_TIMEOUT 5000

/* Listen Nfc-A technology supported by NFEE */
#define PH_LIBNFC_SE_LSTN_NFC_A_SUPP    0x01
/* Listen Nfc-B technology supported by NFEE */
#define PH_LIBNFC_SE_LSTN_NFC_B_SUPP    0x02
/* Listen Nfc-F technology supported by NFEE */
#define PH_LIBNFC_SE_LSTN_NFC_F_SUPP    0x04

extern phLibNfc_Sequence_t gphLibNfc_SetSeModeSeq[];
extern phLibNfc_Sequence_t gphLibNfc_SePowerAndLinkCtrlSeq[];

#include <phNfcStatus.h>
#include "phNciNfcTypes.h"

#define PH_LIBNFC_GETRFCONFIG_LENGTH            0x49
#define PH_LIBNFC_RF_DATA_OFFSET                6

/* RF Technology A */
#define PH_LIBNFC_UIDREGSIZE_A_TECH             10
#define PH_LIBNFC_ATQASIZE_A_TECH                2
#define PH_LIBNFC_APPLICATIONDATASIZE_A_TECH    15
#define PH_LIBNFC_DATAMAXRATESIZE_A_TECH         3

/* RF Technology B */
#define PH_LIBNFC_PUPIREG_SIZE_B_TECH             4
#define PH_LIBNFC_ATQASIZE_B_TECH                 4
#define PH_LIBNFC_HIGHLLAYERRSP_B_TECH           15
#define PH_LIBNFC_DATAMAXRATESIZE_B_TECH          3

extern
void phLibNfc_SENtfHandler(void* pContext,
                           phNciNfc_NotificationType_t eNtfType,
                           pphNciNfc_NotificationInfo_t pSEInfo,
                           NFCSTATUS status);

extern
void phLibNfc_InvokeSeNtfCallback(void* pContext,
                                  void* pInfo,
                                  NFCSTATUS status,
                                  uint8_t bPipeId,
                                  phLibNfc_eSE_EvtType_t eEventType);

extern NFCSTATUS
phLibNfc_LaunchNfceeDiscCompleteSequence(void *pContext,
                                         NFCSTATUS wStatus,
                                         void *pInfo);

extern
void phLibNfc_ConfigRoutingTableCb(void *pContext,
                                   NFCSTATUS wStatus,
                                   void *pInfo);

extern
void phLibNfc_SeEventHotPlugCb(void* pContext,
                               NFCSTATUS wStatus,
                               void *pInfo);

extern NFCSTATUS
phLibNfc_SE_GetIndex(void* pContext,
                     phLibNfc_SE_Status_t bSeState,
                     uint8_t *pbIndex);

extern
phLibNfc_SE_Type_t phLibNfc_SE_GetType(void* pContext,
                                       pphNciNfc_NfceeInfo_t pNfceeInfo);

NFCSTATUS phLibNfc_DelayForSeNtfProc(void* pContext,NFCSTATUS status,void* pInfo);
NFCSTATUS phLibNfc_DelayForSeNtf(void* pContext, NFCSTATUS status, void* pInfo);
