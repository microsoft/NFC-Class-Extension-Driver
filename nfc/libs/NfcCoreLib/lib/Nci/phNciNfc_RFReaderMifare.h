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

#include "phNfcTypes.h"

extern
NFCSTATUS
phNciNfc_SendMfReq(
                    void   *psContext
                    );


extern
NFCSTATUS
phNciNfc_RecvMfResp(
                        void                *psContext,
                        NFCSTATUS           wStatus
                       );
