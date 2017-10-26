/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#define PH_NCINFC_NFCEE_DISC_ENABLE                 (0x01)
#define PH_NCINFC_NFCEE_DISC_DISABLE                (0x00)

#define PH_NCINFC_NFCEE_T3T_CMDSET_LEN_MIN          (9u)
#define PH_NCINFC_NFCEE_T3T_CMDSET_LEN_MAX          (169u)

#define PH_NCINFC_NFCEE_START_RFCOMM_TLV            (0x00)
#define PH_NCINFC_NFCEE_STOP_RFCOMM_TLV             (0x01)
#define PH_NCINFC_NFCEE_START_STOP_RFCOMM_TLV_LEN   (0x03)

#define PH_NCINFC_NFCEE_MIN_TLV_LEN                 (0x02)
#define PH_NCINFC_NFCEE_T3T_ENTRY_LEN               (10U)

/** Value indicates length of Nfcee Mode set command payload */
#define PHNCINFC_NFCEEMODESET_PAYLOADLEN            (0x02)
#define PHNCINFC_NFCEEMODESET_RESP_LEN              (0x01)

#define PH_NCINFC_NFCEEACTION_MIN_LEN               (0x03)
#define PH_NCINFC_NFCEEDISC_MIN_LEN                 (0x06)

#define PH_NCINFC_NFCEE_PROTOS_PROP_MIN             (0x80)
#define PH_NCINFC_NFCEE_PROTOS_PROP_MAX             (0xFE)

#define PH_NCINFC_NFCEEPOWERLINKCTRL_PAYLOADLEN     (0x02)

typedef struct phNciNfc_NfceeContext
{
    /** Total number of Nfcee discovered from response*/
   uint8_t bNumberOfNfcee;
    /** Total number of Nfcee discovered from Discover Response*/
   uint8_t bNfceeCount;
    /** Index of Intended Nfcee from discovered group of Nfcee*/
    phNciNfc_NfceeModes_t eNfceeMode;/**< Used to store the NFCEE Mode to be set*/
    uint8_t bNfceeDiscState;/**< Indicates whether NFCEE Discovery is enabled/Disabled*/
    /** Nfcee Device information This can be changed to array of pointers if memory increases
    and then allocate memory for each discoverd device*/
    phNciNfc_NfceeDeviceHandle_t pNfceeDevInfo[PH_NCINFC_NFCEE_DEVICE_MAX];
}phNciNfc_NfceeContext_t, *pphNciNfc_NfceeContext_t;    /**< Pointer to #phNciNfc_NfceeContext_t structure */

typedef void (*pphNciNfc_Notification_CB_t) (
                                                void *pContext,
                                                uint8_t type,
                                                void *pInfo
                                             );

extern phNciNfc_SequenceP_t gphNciNfc_NfceeDiscSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_ModeSetSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_SePowerAndLinkCtrlSequence[];

NFCSTATUS phNciNfc_DeActivateNfcee(void *pContext);
NFCSTATUS phNciNfc_RfFieldInfoNtfHandler(void *pContext, void *pInfo,NFCSTATUS Status);
NFCSTATUS phNciNfc_NfceeActionNtfHandler(void *pContext, void *pInfo, NFCSTATUS Status);
NFCSTATUS phNciNfc_NfceeProcessRfActvtdNtf(void *pContext,uint8_t *pBuff, uint16_t Len);
NFCSTATUS phNciNfc_NfceeDiscReqNtfHandler(void *pContext, void *pInfo, NFCSTATUS Status);
