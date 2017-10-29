/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>

#define PHNCINFC_DISCFREQCONFIG_BITMASK         (0x01)
#define PHNCINFC_DISCCONFIGMODE_BITMASK         (0x06)

#define PHNCINFC_TECHNBASEDRTNG_BITMASK         (0x02)
#define PHNCINFC_PROTOBASEDRTNG_BITMASK         (0x04)
#define PHNCINFC_AIDBASEDRTNG_BITMASK           (0x08)

#define PHNCINFC_BATTOFFSTATE_BITMASK           (0x01)
#define PHNCINFC_SWITCHOFFSTATE_BITMASK         (0x02)

extern phNciNfc_SequenceP_t gphNciNfc_InitSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_ReleaseSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_NfccResetSequence[];

extern NFCSTATUS phNciNfc_ResetNtfCb(void*     pContext,
                                     void *pInfo,
                                     NFCSTATUS status);
