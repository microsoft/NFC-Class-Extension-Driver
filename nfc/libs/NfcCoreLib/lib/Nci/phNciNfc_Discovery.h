/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNciNfc.h>

/** Length of Discover Response from NFCC */
#define PHNCINFC_DISC_RESP_LEN              (0x01)
/** Length of DeActivate Response from NFCC */
#define PHNCINFC_DEACTIVATE_RESP_LEN        (0x01)
/** Length of Nfcee Discover Response from NFCC */
#define PHNCINFC_NFCEEDISC_RESP_LEN         (0x02)

/** NFCID1 4 bytes length*/
#define PHNCINFC_NFCID1_4BYTES              (4U)
/** NFCID1 7 bytes length*/
#define PHNCINFC_NFCID1_7BYTES              (7U)
/** NFCID1 10 bytes length*/
#define PHNCINFC_NFCID1_10BYTES             (10U)

/** Value indicates Discovery is not in progress */
#define PHNCINFC_RFDISCSTATE_RESET          (0x00)
/** Value indicates Discovery is in progress */
#define PHNCINFC_RFDISCSTATE_SET            (0x01)

/** Value indicates to retrieve the Target whose Remote device handle is passed */
#define PHNCINFC_RETRIEVE_TARGET            (0x00)
/** Value indicates to retrieve the Target which is Activated */
#define PHNCINFC_RETRIEVEACTIVE_TARGET      (0x01)

typedef struct phNciNfc_DiscoveryParams
{
    phNciNfc_RfTechMode_t eRfTechMode;/**< Value of RF Technology mode*/
    /** 0x01-0x0A indicates the frequency of Poll period **/
    uint8_t bDiscoverFreq;
}phNciNfc_DiscoveryParams_t,*pphNciNfc_DiscoveryParams_t;/**< pointer to #phNciNfc_DiscoveryParams_t */

typedef struct phNciNfc_DiscoveryConfig
{
    uint8_t bNoOfConfig;/**< Number of Config parameters */
    /** Maximum of 6 discovery types can be configured */
    phNciNfc_DiscoveryParams_t aParams[PH_NCINFC_MAXCONFIGPARAMS];
}phNciNfc_DiscoveryConfig_t,*pphNciNfc_DiscoveryConfig_t;/**< pointer to #phNciNfc_DiscoveryConfig_t */

typedef struct phNciNfc_DiscContext
{
    phNciNfc_ADD_Cfg_t tConfig;/**<Structure object to #phNciNfc_ADD_Cfg_t*/
    phNciNfc_DeviceInfo_t tDevInfo;/**< Structure object to #phNciNfc_DeviceInfo_t*/
    uint8_t *pDiscPayload;/**< Pointer to Payload buffer of Discover command */
    uint8_t bDiscPayloadLen;/**< Length of Discover command payload*/
    /** Type of Deactivation initiated. This is used to detect whether
    Notification is expected or not */
    phNciNfc_DeActivateType_t eDeActvType;
    /** Flag indicates whether to Wait for De-Activate Notification or not
            0 - Deactivate Complete
            1 - Waiting for De-Activation Notification for Completion*/
    /** State of Discovery, 0 - Discovery process not started
                             1 - Discovery process is started */
    uint8_t bDiscState;
}phNciNfc_DiscContext_t,*pphNciNfc_DiscContext_t;/**< pointer to #phNciNfc_DiscContext_t */

extern phNciNfc_SequenceP_t gphNciNfc_DiscoverSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_DiscSelSequence[];
extern phNciNfc_SequenceP_t gphNciNfc_DeActivateSequence[];

extern NFCSTATUS phNciNfc_ProcessActvNtf(void *pContext, void *pInfo, NFCSTATUS wStatus);
extern NFCSTATUS phNciNfc_ProcessDeActvNtf(void *pContext, void *pInfo, NFCSTATUS wStatus);
extern NFCSTATUS phNciNfc_ProcessDiscNtf(void* pContext, void *pInfo, NFCSTATUS status);

extern void phNciNfc_ClearDiscContext(void *pContext);
extern NFCSTATUS phNciNfc_UpdateDiscConfigParams(void *pNciHandle, phNciNfc_ADD_Cfg_t *pPollConfig);

extern NFCSTATUS phNciNfc_ProcessDeActvState(void *pContext);
extern NFCSTATUS phNciNfc_ValidateDeActvType(PVOID pNciCtx,
                                            pphNciNfc_DiscContext_t pDiscCtx,
                                            phNciNfc_DeActivateType_t eSrcDeActvType,
                                            phNciNfc_DeActivateType_t *pDestDeActvType);

extern void phNciNfc_HandlePriorityDeactv(pphNciNfc_DiscContext_t pDiscCtx);
