//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "TagPayloads.h"

// NFC Forum, NFC Data Exchange Format (NDEF), Version 1.0, Section 3
// NFC Forum, URI Record Type Definition, Version 1.0, Section 3
const uint8_t TagPayloads::NdefBingUri[13] = { 0xD1, 0x01, 0x09, 0x55, 0x01, 0x62, 0x69, 0x6E, 0x67, 0x2E, 0x63, 0x6F, 0x6D };
