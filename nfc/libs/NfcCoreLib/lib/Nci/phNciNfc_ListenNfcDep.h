/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"
#include "phNciNfc.h"

extern
NFCSTATUS
phNciNfc_NfcDepLstnRdrAInit(
                         pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen
                        );

extern
NFCSTATUS
phNciNfc_NfcDepLstnRdrFInit(
                         pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen
                        );
