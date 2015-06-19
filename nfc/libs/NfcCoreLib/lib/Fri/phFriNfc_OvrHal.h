/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

#include <phLibNfc.h>
#include <phFriNfc.h>
#include <phNfcCompId.h>
#include <phNfcStatus.h>
#include <phNciNfc.h>

/** \defgroup grp_fri_nfc_ovr_hal Overlapped HAL
 *
 *  This component encapsulates the HAL functions, suited for the NFC-FRI overlapped way of operation. The HAL itself
 *  is used as it is, wrapped by this component. The purpose of the wrapper is to de-couple a blocking I/O, as used by
 *  the HAL, from the overlapped I/O operation mode the FRI is using.
 *
 *  \par Device Based Functions
 *  NFC Device Based Functions are used to address the NFC device (local device) directly.
 *  These are all functions that use no Remote Device Information.
 *
 *  \par Connection Based Functions
 *  Connection Based Functions use the Remote Device Information to describe a connection
 *  to a certain Remote Device.
 *
 *  \par Component Instance Sharing
 *  FRI components accessing one NFC device share one instance of the Overlapped HAL. Therefore
 *  each calling FRI component must specify - together with the call - where to deliver the
 *  response of the overlapped operation.
 *
 *  \par Lowest Layer
 *  The Overlapped HAL represents the NFC Device, the lowest layer of the FRI components.
 *
 *  \par Completion Forced
 *  The \b HAL \b functions (and underlying functions) of this library must complete before a new call can
 *  be issued. No HAL operation must be pending.
 *
 */

#define PH_FRINFC_OVRHAL_MAX_NUM_MOCKUP_PARAM           255    /**< Number of mockup indices that are are prepared. */
#define PH_FRINFC_OVRHAL_MAX_NUM_MOCKUP_RDI             4     /**< Max. number of mockup RDIs. */
#define PH_FRINFC_OVRHAL_MAX_TEST_DELAY                 1000  /**< Max. test delay in OVR HAL. */
#define PH_FRINFC_OVRHAL_POLL_PAYLOAD_LEN               5     /**< Length of the POLL payload. */ /* @GK/5.6.06 */

#define PH_FRINFC_OVRHAL_NUL             (0)     /**< \brief We're in NO command */

#define PH_FRINFC_OVRHAL_ENU             (1)     /**< \brief Enumerate */
#define PH_FRINFC_OVRHAL_OPE             (2)     /**< \brief Open */
#define PH_FRINFC_OVRHAL_CLO             (3)     /**< \brief Close */
#define PH_FRINFC_OVRHAL_GDC             (4)     /**< \brief Get Dev Caps */
#define PH_FRINFC_OVRHAL_POL             (5)     /**< \brief Poll */
#define PH_FRINFC_OVRHAL_CON             (6)     /**< \brief Connect */
#define PH_FRINFC_OVRHAL_DIS             (7)     /**< \brief Disconnect */
#define PH_FRINFC_OVRHAL_TRX             (8)     /**< \brief Transceive */
#define PH_FRINFC_OVRHAL_STM             (9)     /**< \brief Start Target Mode */
#define PH_FRINFC_OVRHAL_SND             (10)     /**< \brief Send */
#define PH_FRINFC_OVRHAL_RCV             (11)    /**< \brief Receive */
#define PH_FRINFC_OVRHAL_IOC             (12)    /**< \brief IOCTL */

#define PH_FRINFC_OVRHAL_TST             (255)   /**< \brief OVR HAL test-related command */


typedef void (*pphFriNfc_OvrHalPostMsg_t)(void*);

typedef NFCSTATUS (*pphFriNfc_OvrHalAbort_t)(void*);

typedef void (*pphOvrHal_CB_t) (phHal_sRemoteDevInformation_t *RemoteDevHandle,
                                NFCSTATUS status,
                                phNfc_sData_t  *pRecvdata,
                                void *context);

typedef uint8_t (*pphFriNfc_OvrHalPresetParm)(void*, uint16_t, void*, NFCSTATUS*);


typedef struct phHal4Nfc_TransactInfo
{
    phHal_eRFDevType_t               remotePCDType;
}phHal4Nfc_TransactInfo_t;

typedef struct phFriNfc_OvrHal
{
    /** Currently active operation of the component. If no operation is pending, the content of this member is
     *  \ref PH_FRINFC_OVRHAL_NUL .  The component refuses a new call if the contenet is different, namely one
     *  of the other values defined in \ref grp_ovr_hal_cmd .
     */
    uint8_t                         Operation;

    /** The \b temporary pointer to the completion routine information. The HAL needs - for each call - to be told about the
     *  completion routine of the upper (calling) component. This major difference to other components is because
     *  some functions of the HAL are connection-based and some are not. Moreover it is because the HAL is shared
     *  among the FRI components. So, with a variety of potential callers it is required for each caller to instruct
     *  the HAL about the "delivery" address of the response for each individual call.
     */
    phFriNfc_CplRt_t                TemporaryCompletionInfo;
    phFriNfc_CplRt_t                TemporaryRcvCompletionInfo;
    phFriNfc_CplRt_t                TemporarySndCompletionInfo;

    /** Points to a function within the Integration that presets the parameters for the actual
     *  HAL call.
     */
    pphFriNfc_OvrHalPresetParm      Presetparameters;

    /** Posts a message to the actual HAL integration, starting a  NFC HAL I/O with the pre-set
     *  parameters.
     */
    pphFriNfc_OvrHalPostMsg_t       PostMsg;

    /** The context of the Integration (the SW around this component). This is needed to let
     *  the Overlapped HAL access the Integration's functionality to post a message to another
     *  thread.
     */
    void                           *IntegrationContext;

    /** Device reference returned during enumeration: This has to be filled in by the integrating software after
        a call to the HAL Enumerate function (not contained in the overlapped HAl API). */
    phHal_sHwReference_t           *psHwReference;

    /** This flag is set by the ABORT function. The OVR HAL then does no I/O to the real HAL
     *  or to the mockup any more but just completed with the ABORTED status.
     */
    uint8_t OperationAborted;

    /** Abort function to be implemented by the integration. This parameter can be (optionally) initialized
     *  via the call of \ref phFriNfc_OvrHal_Reset_Abort function.
     *  If it is not NULL, the function pointed by \ref will be internally called by the \ref phFriNfc_OvrHal_Abort
     *  function.
     */
    pphFriNfc_OvrHalAbort_t      AbortIntegrationFunction;

    /** Integration-defined Context passed as a parameter of the \ref AbortIntegrationFunction.
     */
    void*                        AbortIntegrationContext;

    void*                        OvrCompletion;

    phHal_sTransceiveInfo_t      TranceiveInfo;

    phNfc_sData_t                sReceiveData;

    phNfc_sData_t                sSendData;

    phHal4Nfc_TransactInfo_t     TransactInfo;

    uint16_t                     *pndef_recv_length;

    uint8_t                      bRecvReq;
    void                        *pRemoteDevInfo;
} phFriNfc_OvrHal_t;

NFCSTATUS
phFriNfc_OvrHal_Transceive(
    _Out_                           phFriNfc_OvrHal_t                   *OvrHal,
    _In_                            phFriNfc_CplRt_t                    *CompletionInfo,
    _In_                            phLibNfc_sRemoteDevInformation_t    *RemoteDevInfo,
    _In_                            phNfc_uCmdList_t                    Cmd,
    _In_                            phNfc_sDepAdditionalInfo_t          *DepAdditionalInfo,
    _In_reads_bytes_(SendLength)    uint8_t                             *SendBuf,
    _In_                            uint16_t                            SendLength,
    _In_reads_bytes_(*RecvLength)   uint8_t                             *RecvBuf,
    _In_                            uint16_t                            *RecvLength
    );

NFCSTATUS
    phFriNfc_OvrHal_Receive(
    _In_                    phFriNfc_OvrHal_t               *OvrHal,
    _In_                    phFriNfc_CplRt_t                *CompletionInfo,
    _In_                    phNfc_sRemoteDevInformation_t   *RemoteDevInfo,
    _In_reads_(*RecvLength) uint8_t                         *RecvBuf,
    _In_                    uint16_t                        *RecvLength
    );

NFCSTATUS phFriNfc_OvrHal_Send(phFriNfc_OvrHal_t              *OvrHal,
                               phFriNfc_CplRt_t               *CompletionInfo,
                               phNfc_sRemoteDevInformation_t  *RemoteDevInfo,
                               uint8_t                        *SendBuf,
                               uint16_t                       SendLength);


NFCSTATUS phFriNfc_OvrHal_Reconnect(phFriNfc_OvrHal_t              *OvrHal,
                                     phFriNfc_CplRt_t               *CompletionInfo,
                                     phNfc_sRemoteDevInformation_t  *RemoteDevInfo);


NFCSTATUS phFriNfc_OvrHal_Connect(phFriNfc_OvrHal_t              *OvrHal,
                                        phFriNfc_CplRt_t               *CompletionInfo,
                                        phHal_sRemoteDevInformation_t  *RemoteDevInfo,
                                        phHal_sDevInputParam_t         *DevInputParam);

