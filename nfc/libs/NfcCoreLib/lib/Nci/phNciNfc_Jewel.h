/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"

extern
NFCSTATUS
phNciNfc_JewelInit(uint8_t *pSensRes);

extern
NFCSTATUS
phNciNfc_SendJewelReq(
                              void   *psContext
                         );

extern
NFCSTATUS
phNciNfc_RecvJewelResp(
                        void                *psContext,
                        NFCSTATUS           wStatus
                       );
