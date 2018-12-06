/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>

extern phNciNfc_SequenceP_t gphNciNfc_InitSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_ReleaseSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_NfccResetSequence[];
extern NFCSTATUS phNciNfc_ResetNtfCb(void*     pContext,
				     void *pInfo,
				     NFCSTATUS status);
