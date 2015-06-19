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
phNciNfc_RdrBInit(
                    pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                );
