/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

/**
 *  \ingroup grp_comp_id
 *
 *  \name Component IDs
 *
 *  IDs for all NFC components. Combined with the Status Code they build the value (status)
 *  returned by each function.
 *
 *  ID Number Spaces:
 *  - 01..1F: HAL
 *  - 20..3F: NFC-MW (Local Device)
 *  - 40..5F: NFC-MW (Remote Device)
 *  .
 *
 *  \note The value \ref CID_NFC_NONE does not exist for Component IDs. Do not use this value except
 *         for \ref NFCSTATUS_SUCCESS. The enumeration function uses \ref CID_NFC_NONE
 *         to mark unassigned "References".
 *
 *  \if hal
 *   \sa \ref phHalNfc_Enumerate
 *  \endif
 */

#define CID_NFC_NONE                    0x00    /**< Unassigned or doesn't apply (see #NFCSTATUS_SUCCESS) */
#define CID_NFC_TML                     0x01    /**< Transport Mapping Layer */
#define CID_NFC_LLC                     0x07    /**< Logical Link Control Layer */
#define CID_NFC_NCI                     0x08    /**< NFC Controller(NFCC) Interface Layer */
#define CID_NFC_DNLD                    0x09    /**< Firmware Download Management Layer */
#define CID_NFC_HAL                     0x10    /**< Hardware Abstraction Layer */
#define CID_NFC_OSAL                    CID_NFC_NONE  /**< Operating System Abstraction Layer*/
#define CID_FRI_NFC_OVR_HAL             0x20    /**< NFC-Device, HAL-based */
#define CID_FRI_NFC_NDEF_RECORD         0x22    /**< NDEF Record Tools Library. */
#define CID_FRI_NFC_NDEF_MAP            0x23    /**< NDEF Mapping. */
#define CID_FRI_NFC_NDEF_REGISTRY       0x24    /**< NDEF_REGISTRY. */
#define CID_FRI_NFC_AUTO_DEV_DIS        0x25    /**< Automatic Device Discovery. */
#define CID_FRI_NFC_NDEF_SMTCRDFMT      0x26    /**< Smart Card Formatting */
#define CID_NFC_LIB                     0x30    /**< NFC Library Layer*/
#define CID_MAX_VALUE                   0xF0    /**< The maximum CID value that is defined. */
#define CID_FRI_NFC_LLCP                0x40    /**< Logical Link Control Protocol */
#define CID_FRI_NFC_LLCP_TRANSPORT      0x50
#define CID_FRI_NFC_LLCP_MAC            0x60
#define CID_NFC_HCI                     CID_NFC_NCI
#define CID_NFC_DAL                     CID_NFC_TML
