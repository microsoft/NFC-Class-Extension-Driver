/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:
    NfcCoreLibTracing.h

Abstract:
    This module defines the WPP tracing macros for the NFC core library

Environment:
    User mode.

--*/

#pragma once

//
// Define the Control GUID and the component flags.
// 696D4914-12A4-422C-A09E-E7E0EB25806A
//
#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        NfcCoreLibTraceControl, (696D4914, 12A4, 422C, A09E, E7E0EB25806A), \
        WPP_DEFINE_BIT(TF_NCI)                                              \
        WPP_DEFINE_BIT(TF_HCI)                                              \
        WPP_DEFINE_BIT(TF_OSAL)                                             \
        WPP_DEFINE_BIT(TF_LIBNFC)                                           \
        WPP_DEFINE_BIT(TF_FRI)                                              \
        WPP_DEFINE_BIT(TF_DTA)                                              \
        WPP_DEFINE_BIT(TF_LLCP)                                             \
        WPP_DEFINE_BIT(TF_SNEP)                                             \
        WPP_DEFINE_BIT(TF_NDEF)                                             \
        )

//
// Define shorter versions of the ETW trace levels
//
#define LEVEL_CRITICAL  TRACE_LEVEL_CRITICAL
#define LEVEL_ERROR     TRACE_LEVEL_ERROR
#define LEVEL_WARNING   TRACE_LEVEL_WARNING
#define LEVEL_INFO      TRACE_LEVEL_INFORMATION
#define LEVEL_VERBOSE   TRACE_LEVEL_VERBOSE

/** Catastrophic/Critical Failure
 *
 * Mostly, this level would be enabled in production code.
 * Use this to alert for failure cases that <b>can not</b> be handled gracefully. */
#define PHFL_LOG_LEVEL_CRIT TRACE_LEVEL_CRITICAL

/** Unexpected failures that can be safely handled
 *
 * Mostly, this level would be enabled during testing. */
#define PHFL_LOG_LEVEL_WARN TRACE_LEVEL_WARNING

/** General Information
 *
 * Extra debugging information to help understand the program flow. */
#define PHFL_LOG_LEVEL_INFO TRACE_LEVEL_INFORMATION

/** Function Logging of Entry and Exit
 *
 * Very verbose logging to understand the flow of called functions. */
#define PHFL_LOG_LEVEL_FUNC TRACE_LEVEL_VERBOSE

/** \def PHFL_LOG_LEVEL_MIN
 * The minimum possible value of logging level.
 *
 * Anything less than this value would mean undefined value.
 */
/** \def PHFL_LOG_LEVEL_MAX
 *
 * The maximum level that can be set
 *
 * Anything above this value would mean undefined value.
 */
#define PHFL_LOG_LEVEL_MIN TRACE_LEVEL_CRITICAL
#define PHFL_LOG_LEVEL_MAX PHFL_LOG_LEVEL_FUNC

#define WPP_FLAG_LEVEL_LOGGER(_flags_, _lvl_) WPP_LEVEL_LOGGER(_flags_)
#define WPP_FLAG_LEVEL_ENABLED(_flags_, _lvl_) (WPP_LEVEL_ENABLED(_flags_) && WPP_CONTROL(WPP_BIT_ ## _flags_).Level >= _lvl_)

#define WPP_FLAG_LEVEL_VALUE_LOGGER(_flags_, _lvl_, _value_) WPP_LEVEL_LOGGER(_flags_)
#define WPP_FLAG_LEVEL_VALUE_ENABLED(_flags_, _lvl_, _value_) (WPP_LEVEL_ENABLED(_flags_) && WPP_CONTROL(WPP_BIT_ ## _flags_).Level >= _lvl_)

#define WPP_FLAG_LEVEL_DESCRIPTION_VALUE_LOGGER(_flags_, _lvl_, _description_, _value_) WPP_LEVEL_LOGGER(_flags_)
#define WPP_FLAG_LEVEL_DESCRIPTION_VALUE_ENABLED(_flags_, _lvl_, _description_, _value_) (WPP_LEVEL_ENABLED(_flags_) && WPP_CONTROL(WPP_BIT_ ## _flags_).Level >= _lvl_)

#define WPP_FLAG_LEVEL_EXPECTED_LOGGER(_flags_, _lvl_, _expected_) WPP_LEVEL_LOGGER(_flags_)
#define WPP_FLAG_LEVEL_EXPECTED_ENABLED(_flags_, _lvl_, _expected_) (!(_expected_) && WPP_LEVEL_ENABLED(_flags_) && WPP_CONTROL(WPP_BIT_ ## _flags_).Level >= _lvl_)


#define W32
#define WPP_CHECK_FOR_NULL_STRING // to prevent exceptions due to NULL strings


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Module by Module Trace function definitions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _WPP_HEX {
    BYTE*  _buf;
    USHORT _len;
} WPP_HEX;

FORCEINLINE
WPP_HEX WppLogHex(
    BYTE*  _buf,
    USHORT _len
    )
{
    WPP_HEX hex;
    hex._buf = _buf;
    hex._len = _len;
    return hex;
}

// begin_wpp config
// DEFINE_CPLX_TYPE(HEXDUMP, WPP_LOGHEXDUMP, WPP_HEX, ItemHEXDump,"s", _HEX_, 0,2);
// end_wpp

#define WPP_LOGHEXDUMP(x) WPP_LOGPAIR(2, &((x)._len)) WPP_LOGPAIR((x)._len, (x)._buf)


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NCI
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_HEXDUMP
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_HEXDUMP, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_HEXDUMP{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_STR{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_STR{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_STR{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_U32{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_NCI_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_U32{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_NCI_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_U32{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_NCI_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_U32MSG{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_U32MSG{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_U32MSG{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_X32{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_NCI_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_X32{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_NCI_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_X32{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_NCI_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_X32MSG{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_X32MSG{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_X32MSG{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NCI_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_BOOL{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_NCI_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_BOOL{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_NCI_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_BOOL{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_NCI_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_NCI_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_CRIT_EXPECT{FLAG=TF_NCI,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_NCI_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_WARN_EXPECT{FLAG=TF_NCI,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_NCI_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_INFO_EXPECT{FLAG=TF_NCI,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_NCI_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_NCI_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phOsalNfc_LogHexData
 */
#define PH_LOG_NCI_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NCI_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NCI_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NCI_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:



//*********************************************************
// MACRO: PH_LOG_NCI_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_FUNC_ENTRY{FLAG=TF_NCI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_NCI_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NCI_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NCI_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NCI_FUNC_EXIT{FLAG=TF_NCI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_NCI_FUNC_EXIT, "Exit");
// end_wpp

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HCI
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_HCI_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_HCI_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_HCI_CRIT_STR{FLAG=TF_HCI,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_HCI_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_HCI_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_HCI_INFO_X32MSG{FLAG=TF_HCI,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_HCI_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_HCI_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_HCI_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_HCI_INFO_EXPECT{FLAG=TF_HCI,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_HCI_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_HCI_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_HCI_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_HCI_FUNC_ENTRY{FLAG=TF_HCI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_HCI_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_HCI_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_HCI_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_HCI_FUNC_EXIT{FLAG=TF_HCI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_HCI_FUNC_EXIT, "Exit");
// end_wpp

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// OSAL
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_STR{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_STR{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_STR{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_U32{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_OSAL_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_U32{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_OSAL_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_U32{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_OSAL_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_U32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_U32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_U32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_X32{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_OSAL_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_X32{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_OSAL_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_X32{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_OSAL_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_X32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_X32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_X32MSG{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_OSAL_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_BOOL{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_OSAL_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_BOOL{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_OSAL_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_BOOL{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_OSAL_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_OSAL_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_CRIT_EXPECT{FLAG=TF_OSAL,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_OSAL_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_WARN_EXPECT{FLAG=TF_OSAL,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_OSAL_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_INFO_EXPECT{FLAG=TF_OSAL,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_OSAL_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_OSAL_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phOsalNfc_LogHexData
 */
#define PH_LOG_OSAL_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_OSAL_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_OSAL_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_OSAL_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_OSAL_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_FUNC_ENTRY{FLAG=TF_OSAL,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_OSAL_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_OSAL_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_OSAL_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_OSAL_FUNC_EXIT{FLAG=TF_OSAL,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_OSAL_FUNC_EXIT, "Exit");
// end_wpp



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LIBNFC
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_STR{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_STR{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_STR{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_U32{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_U32{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_LIBNFC_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_U32{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_LIBNFC_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_U32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_U32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_U32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_X32{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_X32{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_LIBNFC_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_X32{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_LIBNFC_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_X32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_X32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_X32MSG{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_BOOL{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_BOOL{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_BOOL{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_LIBNFC_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_LIBNFC_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_CRIT_EXPECT{FLAG=TF_LIBNFC,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_LIBNFC_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_WARN_EXPECT{FLAG=TF_LIBNFC,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_LIBNFC_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_INFO_EXPECT{FLAG=TF_LIBNFC,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_LIBNFC_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_LIBNFC_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phLIBNFCNfc_LogHexData
 */
#define PH_LOG_LIBNFC_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LIBNFC_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LIBNFC_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LIBNFC_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_LIBNFC_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_FUNC_ENTRY{FLAG=TF_LIBNFC,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_LIBNFC_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LIBNFC_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LIBNFC_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LIBNFC_FUNC_EXIT{FLAG=TF_LIBNFC,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_LIBNFC_FUNC_EXIT, "Exit");
// end_wpp



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FRI
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_STR{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_STR{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_STR{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_U32{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_FRI_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_U32{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_FRI_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_U32{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_FRI_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_U32MSG{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_U32MSG{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_U32MSG{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_X32{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_FRI_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_X32{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_FRI_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_X32{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_FRI_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_X32MSG{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_X32MSG{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_X32MSG{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_FRI_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_BOOL{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_FRI_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_BOOL{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_FRI_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_BOOL{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_FRI_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_FRI_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_CRIT_EXPECT{FLAG=TF_FRI,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_FRI_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_WARN_EXPECT{FLAG=TF_FRI,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_FRI_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_INFO_EXPECT{FLAG=TF_FRI,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_FRI_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_FRI_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phFRINfc_LogHexData
 */
#define PH_LOG_FRI_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_FRI_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_FRI_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_FRI_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_FRI_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_FUNC_ENTRY{FLAG=TF_FRI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_FRI_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_FRI_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_FRI_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_FRI_FUNC_EXIT{FLAG=TF_FRI,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_FRI_FUNC_EXIT, "Exit");
// end_wpp



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DTA
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_STR{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_STR{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_STR{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_U32{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_DTA_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_U32{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_DTA_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_U32{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_DTA_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_U32MSG{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_U32MSG{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_U32MSG{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_X32{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_DTA_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_X32{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_DTA_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_X32{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_DTA_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_X32MSG{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_X32MSG{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_X32MSG{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_DTA_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_BOOL{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_DTA_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_BOOL{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_DTA_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_BOOL{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_DTA_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_DTA_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_CRIT_EXPECT{FLAG=TF_DTA,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_DTA_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_WARN_EXPECT{FLAG=TF_DTA,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_DTA_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_INFO_EXPECT{FLAG=TF_DTA,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_DTA_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_DTA_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phDTANfc_LogHexData
 */
#define PH_LOG_DTA_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_DTA_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_DTA_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_DTA_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_DTA_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_FUNC_ENTRY{FLAG=TF_DTA,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_DTA_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_DTA_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_DTA_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_DTA_FUNC_EXIT{FLAG=TF_DTA,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_DTA_FUNC_EXIT, "Exit");
// end_wpp





/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LLCP
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_STR{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_STR{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_STR{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_U32{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_LLCP_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_U32{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_LLCP_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_U32{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_LLCP_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_U32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_U32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_U32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_X32{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_LLCP_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_X32{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_LLCP_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_X32{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_LLCP_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_X32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_X32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_X32MSG{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_LLCP_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_BOOL{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_LLCP_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_BOOL{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_LLCP_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_BOOL{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_LLCP_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_LLCP_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_CRIT_EXPECT{FLAG=TF_LLCP,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_LLCP_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_WARN_EXPECT{FLAG=TF_LLCP,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_LLCP_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_INFO_EXPECT{FLAG=TF_LLCP,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_LLCP_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_LLCP_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phLLCPNfc_LogHexData
 */
#define PH_LOG_LLCP_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LLCP_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LLCP_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_LLCP_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_LLCP_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_FUNC_ENTRY{FLAG=TF_LLCP,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_LLCP_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_LLCP_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_LLCP_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_LLCP_FUNC_EXIT{FLAG=TF_LLCP,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_LLCP_FUNC_EXIT, "Exit");
// end_wpp




/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SNEP
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_STR{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_STR{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_STR{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_U32{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_SNEP_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_U32{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_SNEP_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_U32{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_SNEP_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_U32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_U32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_U32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_X32{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_SNEP_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_X32{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_SNEP_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_X32{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_SNEP_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_X32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_X32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_X32MSG{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_SNEP_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_BOOL{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_SNEP_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_BOOL{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_SNEP_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_BOOL{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_SNEP_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_SNEP_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_CRIT_EXPECT{FLAG=TF_SNEP,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_SNEP_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_WARN_EXPECT{FLAG=TF_SNEP,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_SNEP_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_INFO_EXPECT{FLAG=TF_SNEP,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_SNEP_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_SNEP_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phSNEPNfc_LogHexData
 */
#define PH_LOG_SNEP_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_SNEP_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_SNEP_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_SNEP_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_SNEP_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_FUNC_ENTRY{FLAG=TF_SNEP,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_SNEP_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_SNEP_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_SNEP_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_SNEP_FUNC_EXIT{FLAG=TF_SNEP,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_SNEP_FUNC_EXIT, "Exit");
// end_wpp


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NDEF
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_STR{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_STR{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(MSG, ...);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_STR
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_STR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_STR{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(MSG, ...);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_U32{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(VALUE);
// USESUFFIX (PH_LOG_NDEF_CRIT_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_U32{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_NDEF_WARN_U32, " = %d", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_U32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_U32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_U32{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_NDEF_INFO_U32, " = %d", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_U32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_CRIT_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_U32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_WARN_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_U32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_U32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_U32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_INFO_U32MSG, "%s = %d", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_X32{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}( VALUE);
// USESUFFIX (PH_LOG_NDEF_CRIT_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_X32{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(VALUE);
// USESUFFIX (PH_LOG_NDEF_WARN_X32, " = 0x%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_X32
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_X32, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_X32{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(VALUE);
// USESUFFIX (PH_LOG_NDEF_INFO_X32, " = 0x%x", VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_X32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_CRIT_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_X32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_WARN_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_X32MSG
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_X32MSG, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_X32MSG{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(DESCRIPTION, VALUE);
// USESUFFIX (PH_LOG_NDEF_INFO_X32MSG, "%s = 0x%x", DESCRIPTION, VALUE);
// end_wpp


//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_BOOL{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(MSG, VALUE);
// USESUFFIX (PH_LOG_NDEF_CRIT_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_BOOL{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(MSG, VALUE);
// USESUFFIX (PH_LOG_NDEF_WARN_BOOL, "BOOL=%x", VALUE);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_BOOL
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_BOOL, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_BOOL{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(MSG, VALUE);
// USESUFFIX (PH_LOG_NDEF_INFO_BOOL, "BOOL=%x", VALUE);
// end_wpp



//*********************************************************
// MACRO: PH_LOG_NDEF_CRIT_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_CRIT_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_CRIT_EXPECT{FLAG=TF_NDEF,LEVEL=LEVEL_CRITICAL}(EXPECTED);
// USESUFFIX (PH_LOG_NDEF_CRIT_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_WARN_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_WARN_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_WARN_EXPECT{FLAG=TF_NDEF,LEVEL=LEVEL_WARNING}(EXPECTED);
// USESUFFIX (PH_LOG_NDEF_WARN_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_INFO_EXPECT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_INFO_EXPECT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_INFO_EXPECT{FLAG=TF_NDEF,LEVEL=LEVEL_INFO}(EXPECTED);
// USESUFFIX (PH_LOG_NDEF_INFO_EXPECT, "UNEXPECTED VALUE=%x", EXPECTED);
// end_wpp

/** \def PH_LOG_NDEF_HEXDATA(LEVEL,MSG,HEXDATA,DATALEN)
 *
 * Logging a stream of hexadecimal bytes
 *
 * \sa phNDEFNfc_LogHexData
 */
#define PH_LOG_NDEF_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NDEF_CRIT_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NDEF_WARN_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:
#define PH_LOG_NDEF_INFO_HEXDATA(MSG,HEXDATA,DATALEN) //TODO:


//*********************************************************
// MACRO: PH_LOG_NDEF_FUNC_ENTRY
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_FUNC_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_FUNC_ENTRY{FLAG=TF_NDEF,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_NDEF_FUNC_ENTRY, "Enter");
// end_wpp

//*********************************************************
// MACRO: PH_LOG_NDEF_FUNC_EXIT
//
// begin_wpp config
// USEPREFIX (PH_LOG_NDEF_FUNC_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC PH_LOG_NDEF_FUNC_EXIT{FLAG=TF_NDEF,LEVEL=LEVEL_VERBOSE}(...);
// USESUFFIX (PH_LOG_NDEF_FUNC_EXIT, "Exit");
// end_wpp
