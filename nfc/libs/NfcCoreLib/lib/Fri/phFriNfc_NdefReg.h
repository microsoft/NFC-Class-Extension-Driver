/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

#include <phFriNfc_NdefRecord.h>

#define PH_FRINFC_NDEFREG_STATE_INIT        0       /**< \internal Init state. The start-up state */
#define PH_FRINFC_NDEFREG_STATE_DIS_PKT     1       /**< \internal Dispatch Packet is in progress */
#define PH_FRINFC_NDEFREG_STATE_DIS_RCD     2       /**< \internal Dispatch Record is in progress */

#define PH_FRINFC_NDEFRECORD_TNF_MASK       0x07    /**< \internal */
#define PH_FRINFC_NDEFREG_CH_FLG_ARR_INDEX  50      /**< \internal */

#define PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED    64    /**< Maximum number of RTDs per node */
#define PH_FRINFC_NDEFREG_MAX_RTD               8u    /**< Maximum number of RTDs per callback function. */

typedef void(*pphFriNfc_NdefReg_Cb_t)(void*);

typedef struct phFriNfc_NdefReg_CbParam
{
    /**
     * Number of array Positions. Each array position carries data from a NDEF Record. The maximum
     * number is \ref PH_FRINFC_NDEFREG_MAX_RTD .
     */
    _Field_range_(<=, PH_FRINFC_NDEFREG_MAX_RTD)
    uint8_t                 Count;

    /**
     * The records that matched with the registred RTDs for this callback.
     * The number of records here will be equal to the first parameter Count.
     */
    phFriNfc_NdefRecord_t   Records[PH_FRINFC_NDEFREG_MAX_RTD];

    /** Indicates whether a record is chunked or not. */
    uint8_t                 Chunked[PH_FRINFC_NDEFREG_MAX_RTD];

    /** Pointer to the raw record. */
    uint8_t                 *RawRecord[PH_FRINFC_NDEFREG_MAX_RTD];

    /** Size of the raw record */
    uint32_t                RawRecordSize[PH_FRINFC_NDEFREG_MAX_RTD];

    /** Pointer for usage by the registering entity. The software component that registers for
        a specific RTD can specify this \b context pointer. With the help of the pointer
        the component is able to resolve its own address, context or object, respectively.\n
        \b Example: \ref grp_fri_nfc_ndef_reg "This SW component" is embedded into a C++ system
        that has one object registered for a certain RTD. \ref grp_fri_nfc_ndef_reg "This library"
        itself is written in C and therefore it requires a pure "C" callback that can be provided by
        C++ through a \b static member function. The registering C++ object will consequently set the
        \ref phFriNfc_NdefReg_CbParam_t::CbContext pointer to its \c this pointer. When the static
        member function of the C++ class is called it immediately knows the instance and can
        call into one of the C++ instance members again (\ref phFriNfc_NdefReg_CbParam_t::CbContext
        needs to be casted back to the original C++ class type).
    */
    void                    *CbContext;
} phFriNfc_NdefReg_CbParam_t;

typedef struct phFriNfc_NdefReg_Cb
{
    /**
     * Number of array Positions. Each array position carries data specifying a RTD. The maximum number
     * is \ref PH_FRINFC_NDEFREG_MAX_RTD .
     *
     * \li Needs to be set by the registering entity.
     */
    _Field_range_(<=, PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED)
    uint8_t                     NumberOfRTDs;
    /**
     *  The Type Name Format, according to the NDEF specification, see the NDEF Record (Tools) component.
     *
     * \li Needs to be set by the registering entity.
     */
    uint8_t                     Tnf[PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED];

    /**
     * Array of pointers to the individual RTD buffers.
     *
     * \li Needs to be set by the registering entity.
     */
    uint8_t                     *NdefType[PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED];

    /**
     * Array of length indicators of the RTD buffers.
     *
     * \li Needs to be set by the registering entity.
     */
    uint8_t                      NdeftypeLength[PH_FRINFC_NDEFREG_MAX_RTD_REGISTERED];

    /**
     * Function pointer to the C-style function within the registering entity.
     *
     * \li Needs to be set by the registering entity.
     */
    pphFriNfc_NdefReg_Cb_t       NdefCallback;

    /**
     * Context pointer of the registering entity (see \ref phFriNfc_NdefReg_CbParam_t).
     *
     * \li Needs to be set by the registering entity.
     */
    void                        *CbContext;

    /** \internal
     * This member is required by the library to link to the previous registered item. In case of the
     * first item this member is NULL.
     */
    struct phFriNfc_NdefReg_Cb  *Previous;
    /** \internal
     * This member is required by the library to link to the next registered item. In case of the
     * last item this member is NULL.
     */
    struct phFriNfc_NdefReg_Cb  *Next;
} phFriNfc_NdefReg_Cb_t;

typedef struct phFriNfc_NdefReg
{
    phFriNfc_NdefReg_Cb_t       *NdefTypeList;    /**< \internal List of Callback Structures (Listeners). */
    uint8_t                     *NdefData;        /**< \internal Data to process. */
    uint32_t                    NdefDataLength;   /**< \internal Length of the NDEF data. */
    uint8_t                     State;            /**< \internal The state of the library. */
    uint8_t                   **NdefTypes;        /**< \internal */

    phFriNfc_NdefRecord_t      *RecordsExtracted; /**< \internal */

    phFriNfc_NdefReg_CbParam_t  *CbParam;         /**< \internal */

    /*  Harsha: Fix for 0000252: [JF] Buffer overshoot in phFriNfc_NdefRecord_GetRecords */
    uint8_t                     *IsChunked;       /**< \internal Array of chunked flags */

    /*  Harsha: Fix for 0000252: [JF] Buffer overshoot
        in phFriNfc_NdefRecord_GetRecords   */
    uint32_t                    NumberOfRecords;  /**< \internal Space available in NdefTypes
                                                                and IsChunked arrays */

    /*  Harsha: Fix for 0000243: [JF] phFriNfc_NdefReg_Process
        won't parse correctly chunked records   */
    /*  Used to remember the last valid TNF */
    uint8_t                     validPreviousTnf; /**< \internal The last valid TNF that we had. */

    uint32_t                    NumberOfNdefTypes;/**< \internal */

    uint32_t                    RecordIndex;      /**< \internal */

    uint32_t                    RtdIndex;         /**< \internal */

    /*  This flag is used to remember whether we have found a
        TNF which matches with the Registered RTD */
    uint8_t                     MainTnfFound;     /**< \internal */

    /*  This flag is used to tell whether the present record
        being processed is newly extracted */
    uint8_t                     newRecordextracted;/**< \internal */

}phFriNfc_NdefReg_t;

NFCSTATUS
phFriNfc_NdefReg_Reset(
    _Out_   phFriNfc_NdefReg_t          *NdefReg,
    _In_    uint8_t                     **NdefTypesarray,
    _In_    phFriNfc_NdefRecord_t       *RecordsExtracted,
    _In_    phFriNfc_NdefReg_CbParam_t  *CbParam,
    _In_    uint8_t                     *ChunkedRecordsarray,
    _In_    uint32_t                    NumberOfRecords
    );

NFCSTATUS phFriNfc_NdefReg_AddCb(phFriNfc_NdefReg_t     *NdefReg,
                                 phFriNfc_NdefReg_Cb_t  *NdefCb);

NFCSTATUS phFriNfc_NdefReg_RmCb(phFriNfc_NdefReg_t    *NdefReg,
                                phFriNfc_NdefReg_Cb_t *NdefCb);

NFCSTATUS phFriNfc_NdefReg_DispatchPacket(phFriNfc_NdefReg_t    *NdefReg,
                                          uint8_t               *PacketData,
                                          uint16_t               PacketDataLength);

NFCSTATUS phFriNfc_NdefReg_DispatchRecord(phFriNfc_NdefReg_t     *NdefReg,
                                          phFriNfc_NdefRecord_t  *RecordsExtracted);

uint8_t phFriNfc_NdefReg_Process(phFriNfc_NdefReg_t  *NdefReg,
                                 NFCSTATUS           *Status);
