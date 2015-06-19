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

extern NFCSTATUS
phNciNfc_RdrIso15693Init(pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen);

extern NFCSTATUS
phNciNfc_Iso15693Send(void *psContext);

extern NFCSTATUS
phNciNfc_Iso15693Receive(void *psContext,
                        NFCSTATUS wStatus);

