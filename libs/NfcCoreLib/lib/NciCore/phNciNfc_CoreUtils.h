/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include "phNciNfc_Core.h"
#include "phNciNfc_CoreStatus.h"

#define PH_NCINFC_UTILS_PAYLOAD_SIZE_0BYTES         (0)
#define PH_NCINFC_UTILS_PAYLOAD_SIZE_1BYTE          (1)
#define PH_NCINFC_UTILS_PAYLOAD_SIZE_2BYTES         (2)
#define PH_NCINFC_UTILS_PAYLOAD_SIZE_3BYTES         (3)
#define PH_NCINFC_UTILS_PAYLOAD_SIZE_4BYTES         (4)

NFCSTATUS phNciNfc_CoreUtilsUpdatePktInfo(pphNciNfc_CoreContext_t pNciCoreCtx, uint8_t *pBuff,
                                              uint16_t wLength);

extern NFCSTATUS
phNciNfc_CoreUtilsValidateGID(
                              uint8_t bGID
                             );

extern NFCSTATUS
phNciNfc_CoreUtilsValidateCtrlPktOID(
                                     uint8_t bMT,
                                     uint8_t bGID,
                                     uint8_t bOID
                                    );
