/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfcTypes.h"
#include "phNciNfc.h"

extern
NFCSTATUS
phNciNfc_PollMgmt(
                  void    *psContext,
                  pphNciNfc_RemoteDevInformation_t pRemDevInf,
                  uint8_t *pBuff,
                  uint16_t wLen
                 );
