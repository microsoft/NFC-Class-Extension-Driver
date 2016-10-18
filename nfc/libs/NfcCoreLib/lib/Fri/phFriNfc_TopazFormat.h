/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>

#include "phFriNfc_SmtCrdFmt.h"

NFCSTATUS
phFriNfc_TopazFormat_Format(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt);

void
phFriNfc_TopazFormat_Process(void* Context,
                             NFCSTATUS Status);

NFCSTATUS
phFriNfc_TopazDynamicFormat_Format(phFriNfc_sNdefSmtCrdFmt_t* NdefSmtCrdFmt);

void
phFriNfc_TopazDynamicFormat_Process(void* Context,
                                    NFCSTATUS Status);
