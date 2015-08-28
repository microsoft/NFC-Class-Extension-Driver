/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phFriNfc.h>
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc_NdefMap.h>

#define PH_FRINFC_NDEFMAP_TLVLEN_ZERO           0

/* NFC Device Major and Minor Version numbers*/
/* !!CAUTION!! these needs to be updated periodically.Major and Minor version numbers
   should be compatible to the version number of currently implemented mapping document.
    Example : NFC Device version Number : 1.0 , specifies
              Major VNo is 1,
              Minor VNo is 0 */
#define PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM             0x01
#ifdef DESFIRE_EV1
#define PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM_2           0x02
#endif /* */
#define PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM             0x00

/* Macros to find major and minor TAG : Ex:Type1/Type2/Type3/Type4 version numbers*/
#define PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(a)           (((a) & (0xf0))>>(4))
#define PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(a)           ((a) & (0x0f))

/* NFC Device Major and Minor Version numbers*/
/* !!CAUTION!! these needs to be updated periodically.Major and Minor version numbers
   should be compatible to the version number of currently implemented mapping document.
    Example : NFC Device version Number : 1.0 , specifies
              Major VNo is 1,
              Minor VNo is 0 */
#define PH_NFCFRI_MFSTDMAP_NFCDEV_MAJOR_VER_NUM             0x40
#define PH_NFCFRI_MFSTDMAP_NFCDEV_MINOR_VER_NUM             0x00

/* Macros to find major and minor TAG : Ex:Type1/Type2/Type3/Type4 version numbers*/
#define PH_NFCFRI_MFSTDMAP_GET_MAJOR_TAG_VERNO(a)           ((a) & (0x40)) // must be 0xC0
#define PH_NFCFRI_MFSTDMAP_GET_MINOR_TAG_VERNO(a)           ((a) & (0x30))

NFCSTATUS phFriNfc_MapTool_ChkSpcVer( const phFriNfc_NdefMap_t  *NdefMap,
                                      uint8_t             VersionIndex);

NFCSTATUS phFriNfc_MapTool_SetCardState(phFriNfc_NdefMap_t  *NdefMap,
                                        uint32_t            Length);
