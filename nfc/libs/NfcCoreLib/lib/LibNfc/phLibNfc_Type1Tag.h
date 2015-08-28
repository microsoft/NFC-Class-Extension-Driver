/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phLibNfc_Sequence.h"

#define PHLIBNFC_RID_RESP_LEN        (0x06) /** Length of RID Response from NFCC */
#define PHLIBNFC_UID_LEN             (0x04) /** Length of UID of Type 1 Tag from NFCC */

extern
phLibNfc_Sequence_t gphLibNfc_T1tGetUidSequence[];
