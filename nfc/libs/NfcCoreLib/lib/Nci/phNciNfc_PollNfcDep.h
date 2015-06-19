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
#include "phNciNfc.h"

extern
NFCSTATUS
phNciNfc_P2P_TransceiveSend(
                              void *psContext
                             );
extern NFCSTATUS
phNciNfc_P2P_TransceiveReceive(void *psContext,
                              NFCSTATUS wStatus);

extern
NFCSTATUS
phNciNfc_NfcDepPollRdrAInit(
                         pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen
                        );

extern
NFCSTATUS
phNciNfc_NfcDepPollRdrFInit(
                         pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                         uint8_t *pBuff,
                         uint16_t wLen
                        );
