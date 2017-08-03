/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"

extern
NFCSTATUS
phNciNfc_Send4BData(
                              void   *psContext
                         );

extern
NFCSTATUS
phNciNfc_Recv4BResp(
                        void                *psContext,
                        NFCSTATUS           wStatus
                       );
