/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"

extern
NFCSTATUS
phNciNfc_Send4AData(
                      void   *psContext
                 );

extern
NFCSTATUS
phNciNfc_Recv4AResp(
                    void                *psContext,
                    NFCSTATUS           wStatus
                   );
