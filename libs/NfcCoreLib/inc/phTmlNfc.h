/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

/**
 * Transaction (Tx/Rx) completion information structure of TML
 *
 * This structure holds the completion callback information of the
 * transaction passed from the TML layer to the Upper layer
 * along with the completion callback.
 *
 * The value of field <code>wStatus</code> can be interpreted as:
 *
 *     - #NFCSTATUS_SUCCESS                    Transaction performed successfully.
 *     - #NFCSTATUS_FAILED                     Failed to wait on Read/Write operation.
 *     - #NFCSTATUS_INSUFFICIENT_STORAGE       Not enough memory to store data in case of read.
 *     - #NFCSTATUS_BOARD_COMMUNICATION_ERROR  Failure to Read/Write from the file or timeout.
 */

typedef struct phTmlNfc_TransactInfo
{
    NFCSTATUS           wStatus;    /**< Status of the Transaction Completion*/

    _Field_size_bytes_(wLength) 
    uint8_t             *pBuff;     /**< Response Data of the Transaction*/
    uint16_t            wLength;    /**< Data size of the Transaction*/
}phTmlNfc_TransactInfo_t;           /**< Instance of Transaction structure */

/**
 * TML transreceive completion callback to Upper Layer
 *
 * \param[in] pContext Context provided by upper layer
 * \param[in] pInfo Transaction info. See \ref phTmlNfc_TransactInfo
 */
typedef void (*pphTmlNfc_TransactCompletionCb_t) (void *pContext,
                                                    phTmlNfc_TransactInfo_t *pInfo);

/**
 * Enum definition contains  supported ioctl control codes.
 *
 * \sa phTmlNfc_IoCtl
 */
typedef enum
{
    phTmlNfc_e_Invalid = 0,         /** Internal */
    phTmlNfc_e_ResetDevice,         /**< Reset the device */
    phTmlNfc_e_EnableDownloadMode,  /**< Do the hardware setting to enter into download mode */
    phTmlNfc_e_EnableNormalMode     /**< Hardware setting for normal mode of operation */
} phTmlNfc_ControlCode_t ;          /**< Control code for IOCTL call */

/**
 *
 * This API allows to Initialize the TML layer and the Hardware interface.
 * This function configures the given Hardware Interface and
 * sends the HANDLE to the caller.
 *
 * \param[in,out]   pHwRef     Information of the hardware
 *
 * \retval #NFCSTATUS_SUCCESS                    Initialization happened successfully.
 * \retval #NFCSTATUS_INVALID_PARAMETER          At least one parameter of the function
 *                                              is invalid.
 * \retval #NFCSTATUS_FAILED                     Initialization failed(example.unable to
 *                                              open HW Interface).
 * \retval #NFCSTATUS_INVALID_DEVICE             The device has not been opened or
 *                                              has been disconnected.
 */
NFCSTATUS phTmlNfc_Init( void *pHwRef );

/**
 *
 * This API allows to DeInitialize the TML layer and the HardWare interface.
 *
 * \param[in] pHwRef            Information of the hardware
 *
 * \retval #NFCSTATUS_SUCCESS            TML Configuration Released successfully.
 * \retval #NFCSTATUS_INVALID_PARAMETER  At least one parameter of the function is invalid.
 * \retval #NFCSTATUS_FAILED             Deinitialization failed(example: Unable to close interface).
 *
 */
NFCSTATUS phTmlNfc_Shutdown( void *pHwRef );

/**
 *
 * Allows to write a data block to HardWare Interface.
 *
 * This asynchronous function writes the given block of data to the driver.
 * This interface enables writer thread in case there is no write requests pending and returns
 * successfully once writer thread completes write operation. It notifies upper layer using callback
 * mechanism.
 *
 * \param[in]  pContext             Context provided by the upper layer.
 * \param[in]  pHwRef               (optional) Information of the hardware
 * \param[in]  pBuffer              data to be sent
 * \param[in]  wLength              The length of data buffer
 * \param[in]  pTmlWriteComplete    Pointer to the function to be invoked on completion
 *                                  of Write Operation.
 *
 * \retval #NFCSTATUS_PENDING If the command is yet to be process
 * \retval #NFCSTATUS_INVALID_PARAMETER At least one parameter of the function is invalid.
 * \retval #NFCSTATUS_BUSY A Write request is already in progress.
 *
 * \note
 *   -# It is important to post a message with id #PH_TMLNFC_WRITE_MESSAGE to IntegrationThread after a data has been written to the NFCC.
 *   -# If CRC needs to be computed, then input buffer should be capable to store two more bytes apart from length of packet.
 *
 *
 */
NFCSTATUS phTmlNfc_Write(void *pHwRef,
                         uint8_t *pBuffer, uint16_t wLength ,
                         pphTmlNfc_TransactCompletionCb_t pTmlWriteComplete,  void *pContext);

/**
 *
 * Allows to Read data block from HW Interface.
 *
 * This asynchronous function reads the data from the driver in which length
 * to be read and the buffer are sent by upper layer. This interface enables
 * reader thread in case there is no read requests pending and returns sucessfully.
 * Once read operation is complete, it notifies  to upper layer through callback
 * mechanism
 *
 * \param[in]  pContext             Context provided by the upper layer.
 * \param[in]  pHwRef               Information of the hardware
 * \param[in,out]  pBuffer          Location to send read data to the upper layer via the callback
 * \param[in]  wLength              The length of pBuffer given by the upper layer
 * \param[in]  pTmlReadComplete     Pointer to the function to be invoked on completion
 *                                  of Read operation.
 *
 * \retval #NFCSTATUS_PENDING               If the command is yet to be processed.
 * \retval #NFCSTATUS_INVALID_PARAMETER     At least one parameter of the function
 *                                          is invalid.
 * \retval #NFCSTATUS_BUSY                  A Read request is already in progress.
 *
 * \note
 *    -# It is important to post a message with id #PH_TMLNFC_READ_MESSAGE to IntegrationThread after a data has been read from the NFCC
 *    -# Reader thread notifies upper layer callback by posting message on the caller's message queue
 *    -# pBuffer would be over-written as part of the call back.  When this function returns pBuffer still has no data
 *
 */
NFCSTATUS phTmlNfc_Read(void *pHwRef,
                        uint8_t *pBuffer, uint16_t wLength,
                        pphTmlNfc_TransactCompletionCb_t pTmlReadComplete,  void *pContext);


/**
 * Aborts the pending write request (if any)
 *
 * \param[in]  pHwRef               Information of the hardware
 *
 * \retval #NFCSTATUS_SUCCESS                Ongoing Write operation Aborted.
 * \retval #NFCSTATUS_INVALID_PARAMETER      At least one parameter of the function
 *                                          is invalid.
 * \retval #NFCSTATUS_NOT_INITIALISED        TML layer is not initialized.
 * \retval #NFCSTATUS_BOARD_COMMUNICATION_ERROR  Unable to Cancel Write operation.
 *
 */
NFCSTATUS phTmlNfc_WriteAbort(void *pHwRef);

/**
 * Aborts the pending read request (if any)
 *
 * \param[in]  pHwRef               Information of the hardware
 *
 * \retval #NFCSTATUS_SUCCESS               Ongoing Read operation Aborted.
 * \retval #NFCSTATUS_INVALID_PARAMETER     At least one parameter of the function
 *                                          is invalid.
 * \retval #NFCSTATUS_NOT_INITIALISED TML layer is not initialized.
 * \retval #NFCSTATUS_BOARD_COMMUNICATION_ERROR Unable to Cancel Read operation.
 *
 */
NFCSTATUS phTmlNfc_ReadAbort(void *pHwRef);


/**
 * This API allows to reset the device.
 *
 * This function resets the device when insisted by the upper layer.
 * \note This function shall also be used for other operations which is under discussion.
 *
 * \param[in]       eControlCode   Control code for a specific operation. (See \ref phTmlNfc_ControlCode_t )
 *
 * \retval #NFCSTATUS_SUCCESS       Ioctl Cmd completed successfully.
 * \retval #NFCSTATUS_FAILED        Ioctl Cmd request failed
 *
 */
NFCSTATUS phTmlNfc_IoCtl(void *pHwRef, phTmlNfc_ControlCode_t eControlCode);
