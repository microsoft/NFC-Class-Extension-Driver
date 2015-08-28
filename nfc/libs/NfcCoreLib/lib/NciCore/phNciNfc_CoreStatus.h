/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status OK
  */
#define PH_NCINFC_STATUS_OK                                         (0x0000)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status rejected
  */
#define PH_NCINFC_STATUS_REJECTED                                   (0x0001)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC Rf frame corrupted
  */
#define PH_NCINFC_STATUS_RF_FRAME_CORRUPTED                         (0x0002)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status failed
  */
#define PH_NCINFC_STATUS_FAILED                                     (0x0003)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status not initialized
  */
#define PH_NCINFC_STATUS_NOT_INITIALIZED                            (0x0004)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status syntax error
  */
#define PH_NCINFC_STATUS_SYNTAX_ERROR                               (0x0005)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status semantic error
  */
#define PH_NCINFC_STATUS_SEMANTIC_ERROR                             (0x0006)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status invalid parameter
  */
#define PH_NCINFC_STATUS_INVALID_PARAM                              (0x0009)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status message size exceeded
  */
#define PH_NCINFC_STATUS_MESSAGE_SIZE_EXCEEDED                      (0x000A)

/* RF Discovery Specific Status Codes */
/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status discovery already started
  */
#define PH_NCINFC_STATUS_DISCOVERY_ALREADY_STARTED                  (0x00A0)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status discovery target activation failed
  */
#define PH_NCINFC_STATUS_DISCOVERY_TARGET_ACTIVATION_FAILED         (0x00A1)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status discovery tear down
  */
#define PH_NCINFC_STATUS_DISCOVERY_TEAR_DOWN                        (0x00A2)

/* RF Interface Specific Status Codes */
/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status RF transmission error
  */
#define PH_NCINFC_STATUS_RF_TRANSMISSION_ERROR                      (0x00B0)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status RF protocol error
  */
#define PH_NCINFC_STATUS_RF_PROTOCOL_ERROR                          (0x00B1)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status RF timeout error
  */
#define PH_NCINFC_STATUS_RF_TIMEOUT_ERROR                           (0x00B2)

/* NFCEE Interface Specific Status Codes */
/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status Nfcee interface activaion failed
  */
#define PH_NCINFC_STATUS_NFCEE_INTERFACE_ACTIVATION_FAILED          (0x00C0)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status NFCEE transmission error
  */
#define PH_NCINFC_STATUS_NFCEE_TRANSMISSION_ERROR                   (0x00C1)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status NFCEE protocol error
  */
#define PH_NCINFC_STATUS_NFCEE_PROTOCOL_ERROR                       (0x00C2)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status NFCEE timeout error
  */
#define PH_NCINFC_STATUS_NFCEE_TIMEOUT_ERROR                        (0x00C3)

/*NCI SW Stack status Code for NCI CORE*/

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status invalid message type
  */
#define PH_NCINFC_STATUS_INVALID_MT                                 (0x00F0)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NFC status invalid packet length
  */
#define PH_NCINFC_STATUS_INVALID_PKT_LEN                            (0x00F1)


/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NCI NFC status Connection not found
  */

#define PH_NCINFC_STATUS_CONNID_NOTFOUND                                (0x00F2)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NCI NFC status invalid Connection Id
  */

#define PH_NCINFC_INVALID_CONNID                                        (0x00F3)

/**
  * \ingroup grp_nci_nfc_core
  *
  * \brief NCI NFC status invalid Connection Id
  */

#define PH_NCINFC_INVALID_CNTRL_PAYLOAD_LENGTH                          (0x00F4)
