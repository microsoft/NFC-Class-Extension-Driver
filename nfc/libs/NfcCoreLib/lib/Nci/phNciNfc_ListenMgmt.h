/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#pragma once

#include "phNciNfcTypes.h"

extern
NFCSTATUS
phNciNfc_ListenMgmt(
                  void    *psContext,
                  pphNciNfc_RemoteDevInformation_t pRemDevInf,
                  uint8_t *pBuff,
                  uint16_t wLen
                 );

extern
void
phNciNfc_ListenMgmt_DeActivate(void* pContext,
                                    pphNciNfc_RemoteDevInformation_t pRemDevInfo);

extern NFCSTATUS
phNciNfc_ReceiveCb(void* pContext, void *pInfo, NFCSTATUS wStatus);
