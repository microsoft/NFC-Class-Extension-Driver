/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfcTypes.h"

extern
NFCSTATUS
phNciNfc_RdrAInit(
                    pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                );
