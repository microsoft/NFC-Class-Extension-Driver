/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#pragma once

#include "phLibNfc_Sequence.h"

/* This function shall send Get system information command to ISO15693 tag */
extern NFCSTATUS phLibNfc_GetSysInfoCmd(void *pContext,NFCSTATUS status,void *pInfo);

/* This function shall Process system information received from ISO15693 tag */
extern NFCSTATUS phLibNfc_GetSysInfoResp(void *pcontext,NFCSTATUS status,void *pInfo);

extern
phLibNfc_Sequence_t gphLibNfc_Iso15693GetSysInfoSeq[];
