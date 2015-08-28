/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

void
phFriNfc_ISO15693_FmtReset (
    phFriNfc_sNdefSmtCrdFmt_t *psNdefSmtCrdFmt);

NFCSTATUS
phFriNfc_ISO15693_Format (
    phFriNfc_sNdefSmtCrdFmt_t *psNdefSmtCrdFmt);

void
phFriNfc_ISO15693_FmtProcess (
    void        *pContext,
    NFCSTATUS   Status);
