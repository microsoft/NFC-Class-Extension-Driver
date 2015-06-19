/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*    
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNfcCompId.h>

/**
 *  This component implements Ndef Message composing and processing routines
 *  The Ndef Record is a data structure used by NFC Forum compliant devices for data transfer.
 *  The capabilities of this module are:
 *  \li Interpret a given buffer by listing the embedded NDEF records
 *  \li Extract a Ndef record from a given buffer
 *  \li Compose a NDEF record and optionally append it to an existing buffer/message
 */

/**
 * \brief The TNF specifies the structure of the NDEF Record TYPE field
 * \name NDEF Record Type Name Format
 */
#define PH_FRINFC_NDEFRECORD_TNF_EMPTY        ((uint8_t)0x00)  /**< Empty Record, no type, ID or payload present. */
#define PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN ((uint8_t)0x01)  /**< NFC well-known type (RTD). */
#define PH_FRINFC_NDEFRECORD_TNF_MEDIATYPE    ((uint8_t)0x02)  /**< Media Type. */
#define PH_FRINFC_NDEFRECORD_TNF_ABSURI       ((uint8_t)0x03)  /**< Absolute URI. */
#define PH_FRINFC_NDEFRECORD_TNF_NFCEXT       ((uint8_t)0x04)  /**< Nfc Extenal Type (following the RTD format). */
#define PH_FRINFC_NDEFRECORD_TNF_UNKNOWN      ((uint8_t)0x05)  /**< Unknown type; Contains no Type information. */
#define PH_FRINFC_NDEFRECORD_TNF_UNCHANGED    ((uint8_t)0x06)  /**< Unchanged: Used for Chunked Records. */
#define PH_FRINFC_NDEFRECORD_TNF_RESERVED     ((uint8_t)0x07)  /**< RFU, must not be used. */

/**
 * \brief These are the flags specifying the content, structure or purpose of a NDEF Record.
 * \name NDEF Record Header Flags
 *
 * Flags of the first record byte, as defined by the NDEF specification.
 *
 */
#define PH_FRINFC_NDEFRECORD_FLAGS_MB       ((uint8_t)0x80)  /**< This marks the begin of a NDEF Message. */
#define PH_FRINFC_NDEFRECORD_FLAGS_ME       ((uint8_t)0x40)  /**< Set if the record is at the Message End. */
#define PH_FRINFC_NDEFRECORD_FLAGS_CF       ((uint8_t)0x20)  /**< Chunk Flag: The record is a record chunk only. */
#define PH_FRINFC_NDEFRECORD_FLAGS_SR       ((uint8_t)0x10)  /**< Short Record: Payload Length is encoded in ONE byte only. */
#define PH_FRINFC_NDEFRECORD_FLAGS_IL       ((uint8_t)0x08)  /**< The ID Length Field is present. */

/* NDEF Record #defines for constant value */
#define PHFRINFCNDEFRECORD_CHUNKBIT_SET         1               /** \internal Chunk Bit is set. */
#define PHFRINFCNDEFRECORD_CHUNKBIT_SET_ZERO    0               /** \internal Chunk Bit is not set. */
#define PHNFCSTSHL16                            16              /** \internal Shift 16 bits(left or right). */
#define PHNFCSTSHL24                            24              /** \internal Shift 24 bits(left or right). */
#define PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE   4               /** \internal Normal record. */
#define PH_FRINFC_NDEFRECORD_TNFBYTE_MASK       ((uint8_t)0x07) /** \internal For masking */
#define PH_FRINFC_NDEFRECORD_BUF_INC1           1               /** \internal Increment Buffer Address by 1 */
#define PH_FRINFC_NDEFRECORD_BUF_INC2           2               /** \internal Increment Buffer Address by 2 */
#define PH_FRINFC_NDEFRECORD_BUF_INC3           3               /** \internal Increment Buffer Address by 3 */
#define PH_FRINFC_NDEFRECORD_BUF_INC4           4               /** \internal Increment Buffer Address by 4 */
#define PH_FRINFC_NDEFRECORD_BUF_INC5           5               /** \internal Increment Buffer Address by 5 */
#define PH_FRINFC_NDEFRECORD_BUF_TNF_VALUE      ((uint8_t)0x00) /** \internal If TNF = Empty, Unknown and Unchanged, the id, type and payload length is ZERO  */
#define PH_FRINFC_NDEFRECORD_FLAG_MASK          ((uint8_t)0xF8) /** \internal To Mask the Flag Byte */

/**
 *\ingroup grp_lib_nfc
 *\brief NFC NDEF Record structure definition.
 *
 *The NDEF Record Compound used for:
 *\li \b Extraction: The calling function(-ality) receives the NDEF data of an extracted
 *                     record in this structure.
 *\li \b Composition The caller is required to specify the data to write within this structure
 *                     in order to have it serialized into a new NDEF record.
 *                     The structure offers a user-friendly way to accomplish this.
 */
typedef struct phFriNfc_NdefRecord
{
    /**
     *  The flags control (or inform about) the structure of a record.
     *
     *  \li \b Extraction:  The component fills in the flags, extracted from the NDEF record.
     *  \li \b Composition: The caller has to specify the proper flags (OR'ing of the individual
     *                      flag definitions (such as PH_FRINFC_NDEFRECORD_FLAGS_MB). This
     *                      information goes into the generated (composed) record. The caller
     *                      is responsible for applying the correct flags in order to mark the
     *                      begin or end of a message or other conditions.
     */
    uint8_t                 Flags;

    /**
     *  The Type Name Format, according to the NDEF specification, e.g. PH_FRINFC_NDEFRECORD_TNF_NFCWELLKNOWN .
     *
     *  \li \b Extraction:  The component fills the TNF value, extracted from the NDEF record.
     *  \li \b Composition: The caller needs to specify the TNF according to the definitions in the
     *                      NDEF specification.
     */
    uint8_t                 Tnf;

    /**
     *  The length of the Type field. See \ref phFriNfc_NdefRecord_t::Tnf and
     *  \ref phFriNfc_NdefRecord_t::Type .
     *
     *  \li \b Extraction:  If the TNF indicates that a type field is present this
     *                      member contains its length. Otherwise the length is set
     *                      to zero by the library.
     *  \li \b Composition: If the caller specifies via TNF that a Type field is present
     *                      the length of the Type has to be specified in this member.
     *                      Otherwise, this member is ignored by the library.
     */
    uint8_t                 TypeLength;

    /**
     *  Contained record type: This is a buffer holding the Type
     *  as defined in the NDEF specification of the NFC Forum.
     *  No zero-termination is present, the length is determined by
     *  \ref phFriNfc_NdefRecord_t::TypeLength .
     *
     *  \li \b Extraction:  If the record holds a type (see TNF) the function sets the pointer to the
     *                      beginning of the Type field of the record. Otherwise, if no type is present
     *                      this member is set to NULL by the library.
     *  \li \b Composition: The caller has to specify a pointer to a buffer holding the record type. If the
     *                      caller specifies that the record is without type (TNF) this member is ignored
     *                      by the library.
     */
    uint8_t                *Type;

    /**
     *  The length of the ID field. See \ref phFriNfc_NdefRecord_t::Flags .
     *
     *  \li \b Extraction:  If the IL flag indicates that an ID field is present this
     *                      member contains its length. Otherwise the length is set
     *                      to zero by the library.
     *  \li \b Composition: If the caller specifies via IL that an ID field is present
     *                      the length of the ID has to be specified in this member.
     *                      Otherwise, this member is ignored by the library.
     */
    uint8_t                 IdLength;

    /**
     *  Record ID: This is a buffer holding the ID
     *  as written in the NDEF specification of the NFC Forum.
     *  No zero-termination is present, the length is determined by
     *  \ref phFriNfc_NdefRecord_t::IdLength .
     *
     *  \li \b Extraction:  If the record holds an ID (IL Flag) the function sets the pointer to the
     *                      beginning of the ID field of the record. Otherwise, if no ID is present
     *                      this member is set to NULL by the library.
     *  \li \b Composition: The caller has to specify a pointer to a buffer holding the record ID. If the
     *                      caller specifies that the record is without ID (IL Flag) this member is ignored
     *                      by the library.
     */
    uint8_t                *Id;

    /**
     * The length of the Payload, in bytes. The maximum length is 2^32 - 1.
     *
     * \li \b Extraction:  The value is set by the function. If the extraction encounters
     *                     an error, this member is set to zero.
     * \li \b Composition: The value must be different from ZERO and and less than 2^32 and has to be
     *                     provided by the caller.
     */
    uint32_t                PayloadLength;

    /**
     * Payload Data, pointer to a buffer containing the payload data.
     *
     * \li \b Extraction:  The pointer is set to the beginning of the payload.
     *                     No pre-set is required before the extraction function call.
     * \li \b Composition: The referenced data is copied to the buffer where the library composes the record.
     *                     The pointer is provided by the caller and must not be NULL and valid.
     */
    uint8_t                *PayloadData;

} phFriNfc_NdefRecord_t;

 /**
 * \ingroup grp_lib_nfc
 * \brief NDEF Record \b Get \b Records function.
 *
 *
 *  Get a list of NDEF records from the raw data as provided by the caller. This data is a
 *  buffer holding one or more NDEF records within a NDEF message (received from a NFC peer
 *  device, for example). The function returns the NumberOfRawRecords present in the message
 *  with pointers to the records stored in the array RawRecords. The function also tells
 *  whether a record is chunked.
 *
 * \param[in]     pBuffer                The data buffer holding the NDEF Message, as provided
 *                                      by the caller.
 * \param[in]     BufferLength          The data length, as provided by the caller.
 * \param[in,out] pRawRecords            Array of pointers, receiving the references to the found
 *                                      Ndef Records in the given Message. The caller has to provide the
 *                                      array of pointers. The array is filled with valid pointers
 *                                      up to the number of records found or the array size if the
 *                                      number of found records exceeds the size.\n
 *                                      If this parameter is set to NULL by the caller it is ignored.
 * \param[in,out] IsChunked             This array of booleans indicates whether a record has the
 *                                      CHUNKED flag set (is a partial record).
 *                                      The number of caller-provided array positions has to be the same
 *                                      as "NumberOfRawRecords".
 *                                      If the caller sets this parameter to NULL it is ignored.
 * \param[in,out] pNumberOfRawRecords    Length of the RawRecords array and IsChunked list.
 *                                      The value is set by the extracting function to the actual number
 *                                      of records found in the data. If the user specifies 0 (zero)
 *                                      the function only yields the number of records without filling
 *                                      in pointers.\n The value NULL is invalid.
 *
 * \retval NFCSTATUS_SUCCESS            Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER  At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_FORMAT     This error is returned in the following scenarios
 *
 * -# There is no Message Begin (MB) Bit in the Message.
 * -# Type Name Format (TNF) Bits are set to 0x07 (Reserved) which is reserved and has not
 *    to be used.
 * -# MB Bit is set to 1 and TNF bits are set to 0x06 (unchanged).
 * -# MB Bit is set to 1 and TNF bits are not set to 0x05 or 0x00 (unknown or
 *     Empty). However,Type Length equals 0.
 * -# Message End Bit is set to 1 and Chunked Flag(CF)is set to 1 in the same record.
 * -# Not a First Record, either the MB Bit OR the CF bit is set to 1. IDLength (IL) Bit
 *    is set to 1 and TNF bits are set to 0x06 (Unchanged).
 * -# Not a First Record, the CF Bit of previous record is set to 1 and TNF bits are not
 *     set to 0x06 (unchanged) OR\n The CF Bit of the previous record is not set to 1 and
 *     TNF bits are not set to 0x06 (unchanged).
 * -# Check for Last Chunk, CF Bit of previous record is set to 1 and CF Bit of present
 *    record is set to 0, but Type Length or ID Length is not equal to zero.
 * -# Inconsistency between the calculated length of the message and the length provided
 *    by the caller.or if TNF bits are set to 0x00 or 0x05 or 0x06 (Empty,Unknown and
 *    Unchanged), but Type Length is not equal to zero
 * -# TNF bits are set to 0x00 (Empty), but Payload Length is not equal to zero.
 * -# TNF bits are set to 0x00 (Empty),but ID Length is not equal to zero.
 *
 * \note The correct number of found records is returned by the function also in case that:
 *       - The "RawRecords" array is too short to hold all values: It is filled up to the allowed maximum.
 *       - The "RawRecords" array is NULL: Only the number is returned.
 *       - The "NumberOfRawRecords" parameter is zero: The array is not filled, just the number is returned.
 *       .
 *       This can be used for targeted memory allocation: Specify NULL for "RawRecords" and/or
 *       zero for "NumberOfRawRecords" and the function just yields the correct array size to allocate
 *       for a second call.
 * \note \b Security: This function verifies the given NDEF message buffer integrity. Its purpose is to
 *       initially check incoming data and taking this effort away from \ref phFriNfc_NdefRecord_Parse.
 *       It is a strong requirement for \ref phFriNfc_NdefRecord_GetRecords to be called to check incoming data
 *       before it is handed over to further processing stages.
 */
 NFCSTATUS phFriNfc_NdefRecord_GetRecords(uint8_t* pBuffer,
                                          uint32_t BufferLength,
                                          uint8_t* pRawRecords[],
                                          uint8_t  IsChunked[],
                                          uint32_t* pNumberOfRawRecords
                                          );


 /**
 *\ingroup grp_lib_nfc
 *
 *\brief <b>NDEF Record \b Parse function</b>
 *
 *
 *  Extract a specific NDEF record from the data, provided by the caller. The data is a buffer holding
 *  at least the entire NDEF record (received via the NFC link, for example).
 *
 * \param[out] pRecord        The NDEF record structure. The storage for the structure has to
 *                            be provided by the caller matching the requirements for \b Extraction,
 *                            as described in the compound documentation. It is important to note
 *                            that all the pointers inside the extracted record structure direct
 *                            to the \b original buffer and not to a copy (see notes) after extraction.
 * \param[in]  pRawRecord     The Pointer to the record location, selected out of the records array,
 *                            returned by the \ref phFriNfc_NdefRecord_GetRecords function.
 *
 * \retval NFCSTATUS_SUCCESS            Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER  At least one parameter of the function is invalid.
 * \retval NFCSTATUS_INVALID_FORMAT     This error is returned in the following scenarios
 *
 *<br>1. Type Name Format (TNF) Bits are set to 0x07 (Reserved) which is reserved and has not to be used.
 *<br>2. TNF bits are set to 0x00 or 0x05 or 0x06 (Empty,Unknown and Unchanged), but Type Length is not
 *                                         equal to 0.
 *<br>3. TNF bits are set to 0x00 (Empty),but Payload Length is not equal to 0.
 *<br>4. TNF bits are set to 0x00 (Empty),but ID Length is not equal to 0.
 *
 * \note There are some caveats:
 *     \li The "RawRecord" Data buffer must exist at least as long as the function execution time
 *         plus the time needed by the caller to evaluate the extracted information. No copying of
 *         the contained data is done internally.
 *     \li \b Security: It is not allowed to feed an unchecked NDEF message into the RawRecord
 *         parameter of the function in an attempt to extract just the first record. The
 *         rule is always to use \ref phFriNfc_NdefRecord_GetRecords in order to retrieve the
 *         verified pointers to the individual records within a raw message. A violation of this
 *         rule leads to the loss of buffer boundary overflow protection.
 */
 NFCSTATUS phFriNfc_NdefRecord_Parse(phFriNfc_NdefRecord_t* pRecord,
                                     uint8_t*               pRawRecord);


/**
 *\ingroup grp_lib_nfc
 *\brief <b>NDEF Record \b Generate function</b>
 *
 *  The function writes one NDEF record to a specified memory location. Called within a loop, it is
 *  possible to write more records into a contiguous buffer, in each cycle advancing by the number
 *  of bytes written for each record.
 *
 * \param[in]     pRecord             The NDEF record structure to append. The structure
 *                                    has to be initialized by the caller matching the requirements for
 *                                    \b Composition, as described in the documentation of
 *                                    the \ref phFriNfc_NdefRecord_t "NDEF Record" structure.
 * \param[in]     pBuffer             The pointer to the buffer to which the record shall be written.
 * \param[in]     MaxBufferSize       The data buffer's (remaining) maximum size, provided by the caller.
 *                                    This must be the \b actual number of bytes that can be written into
 *                                    the buffer. The user must update this value for each call.
 * \param[out]    pBytesWritten       The actual number of bytes written to the buffer during the recent call.
 *                                    This can be used by the caller to serialize more than one record
 *                                    into the same buffer (composing a NDEF message).
 *
 * \retval NFCSTATUS_SUCCESS                  Operation successful.
 * \retval NFCSTATUS_INVALID_PARAMETER        At least one parameter of the function is invalid.
 * \retval NFCSTATUS_BUFFER_TOO_SMALL         The data buffer, provided by the caller is to small to
 *                                            hold the composed NDEF record. The existing content is
 *                                            not changed, no data have been written.
 *
 *  \note The Caller of this function must take care that, with each call, the parameters are
 *        updated correctly:
 *        \li MaxBufferSize must decreased by the previous call number of \b BytesWritten
 *        \li Pointer to Buffer is advanced by the previous call number of \b BytesWritten.
 */
NFCSTATUS phFriNfc_NdefRecord_Generate(phFriNfc_NdefRecord_t *pRecord,
                                       uint8_t               *pBuffer,
                                       uint32_t               MaxBufferSize,
                                       uint32_t              *pBytesWritten
);


/*  Helper function Prototypes - Used only internally   */

/** \internal
 *
 *  This is a sub function to the Generate Function. This function gives the length of a record given as
 *  input and this will not check for any errors in the Record.
 *
 */
uint32_t phFriNfc_NdefRecord_GetLength(phFriNfc_NdefRecord_t* pRecord);
