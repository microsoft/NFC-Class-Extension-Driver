/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phFriNfc_Pch.h"

#include <phFriNfc_NdefMap.h>
#include <phFriNfc_MapTools.h>
#include <phFriNfc_MifareULMap.h>

#include <phFriNfc_OvrHal.h>

#include "phFriNfc_MapTools.tmh"

NFCSTATUS phFriNfc_MapTool_SetCardState(phFriNfc_NdefMap_t  *NdefMap,
                                        uint32_t            Length)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    PH_LOG_NDEF_FUNC_ENTRY();
    if(Length == PH_FRINFC_NDEFMAP_MFUL_VAL0)
    {
        /* As the NDEF LEN / TLV Len is Zero, irrespective of any state the card
           shall be set to INITIALIZED STATE*/
        NdefMap->CardState =(uint8_t) (((NdefMap->CardState ==
                                PH_NDEFMAP_CARD_STATE_READ_ONLY) ||
                                (NdefMap->CardState ==
                                PH_NDEFMAP_CARD_STATE_INVALID))?
                                PH_NDEFMAP_CARD_STATE_INVALID:
                                PH_NDEFMAP_CARD_STATE_INITIALIZED);
    }
    else
    {
        switch(NdefMap->CardState)
        {
            case PH_NDEFMAP_CARD_STATE_INITIALIZED:
                NdefMap->CardState =(uint8_t) ((NdefMap->CardState ==
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_WRITE);
            break;

            case PH_NDEFMAP_CARD_STATE_READ_ONLY:
                NdefMap->CardState = (uint8_t)((NdefMap->CardState ==
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_ONLY);
            break;

            case PH_NDEFMAP_CARD_STATE_READ_WRITE:
                NdefMap->CardState = (uint8_t)((NdefMap->CardState ==
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_WRITE);
            break;

            default:
                NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_NO_NDEF_SUPPORT);
            break;
        }
    }
    Result = ((NdefMap->CardState ==
                PH_NDEFMAP_CARD_STATE_INVALID)?
                PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                NFCSTATUS_NO_NDEF_SUPPORT):
                Result);
    PH_LOG_NDEF_FUNC_EXIT();
    return Result;
}

NFCSTATUS   phFriNfc_MapTool_ChkSpcVer( const phFriNfc_NdefMap_t  *NdefMap,
                                        uint8_t             VersionIndex)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    uint8_t TagVerNo = NdefMap->SendRecvBuf[VersionIndex];
    PH_LOG_NDEF_FUNC_ENTRY();
    PH_LOG_NDEF_INFO_STR("Checking the Ndef version suported by the card...");
    if ( TagVerNo == 0 )
    {
        /*Return Status Error “ Invalid Format”*/
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
    }
    else
    {
        switch (NdefMap->CardType)
        {
            case PH_FRINFC_NDEFMAP_MIFARE_STD_1K_CARD:
            case PH_FRINFC_NDEFMAP_MIFARE_STD_4K_CARD:
            {
                /* calculate the major and minor version number of Mifare std version number */
                status = (( (( PH_NFCFRI_MFSTDMAP_NFCDEV_MAJOR_VER_NUM ==
                                PH_NFCFRI_MFSTDMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                            ( PH_NFCFRI_MFSTDMAP_NFCDEV_MINOR_VER_NUM ==
                                PH_NFCFRI_MFSTDMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
                            (( PH_NFCFRI_MFSTDMAP_NFCDEV_MAJOR_VER_NUM ==
                                PH_NFCFRI_MFSTDMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                            ( PH_NFCFRI_MFSTDMAP_NFCDEV_MINOR_VER_NUM <
                                PH_NFCFRI_MFSTDMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))?
                        NFCSTATUS_SUCCESS:
                        PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                                    NFCSTATUS_INVALID_FORMAT));
                break;
            }

#ifdef DESFIRE_EV1
            case PH_FRINFC_NDEFMAP_ISO14443_4A_CARD_EV1:
            {
                /* calculate the major and minor version number of T3VerNo */
                if( (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM_2 ==
                        PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM ==
                        PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
                    (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM_2 ==
                        PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM <
                        PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))
                {
                    status = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
                }
                else
                {
                    if (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM_2 <
                            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo) ) ||
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM_2 >
                            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo)))
                    {
                        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
                    }
                }
                break;
            }
#endif /* #ifdef DESFIRE_EV1 */

            default:
            {
                /* calculate the major and minor version number of T3VerNo */
                if( (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM ==
                        PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM ==
                        PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
                    (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM ==
                        PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM <
                        PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))
                {
                    status = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
                }
                else
                {
                    if (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM <
                            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo) ) ||
                    ( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM >
                            PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo)))
                    {
                        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
                    }
                }
                break;
            }
        }
    }
    PH_LOG_NDEF_FUNC_EXIT();
    return (status);
}
