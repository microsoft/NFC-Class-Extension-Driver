/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    TraceCommon.h

Abstract:

    This module defines the WPP tracing macros.

Environment:

    User mode.

--*/

#pragma once


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Class method entry/exit tracing macros.  They trace the "this" pointer on entry and exit.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Define shorter versions of the ETW trace levels
//
#define LEVEL_CRITICAL  TRACE_LEVEL_CRITICAL
#define LEVEL_ERROR     TRACE_LEVEL_ERROR
#define LEVEL_WARNING   TRACE_LEVEL_WARNING
#define LEVEL_INFO      TRACE_LEVEL_INFORMATION
#define LEVEL_VERBOSE   TRACE_LEVEL_VERBOSE
#define LEVEL_FATAL     TRACE_LEVEL_FATAL

//
// This is a special LEVEL that changes the trace macro level from ERROR to VERBOSE
// depending on whether the return value passed to the macro was non-zero or zero,
// respectively.
//
#define LEVEL_COND 0xFFFF

//
// Override the default LOGGER and ENABLED macros to support true ETW level control
// instead of the WPP flag control.
//
#define WPP_LEVEL_LOGGER(LEVEL) (WPP_CONTROL(WPP_BIT_ ## DUMMY).Logger),
#define WPP_LEVEL_ENABLED(LEVEL) (WPP_CONTROL(WPP_BIT_ ## DUMMY).Level >= LEVEL)

//
// This macro is to be used by the WPP custom macros below that want to do conditional
// logging based on return value. If LEVEL_VERBOSE is specified when calling a macro that
// uses this, the level will be set to LEVEL_INFO if return code is 0 or
// LEVEL_ERROR if the return code is not 0. This can be called in any PRE macro.
//
// The "LEVEL == LEVEL_COND" check generates a compiler warning that the "conditional
// expression is constant" so we explicitly disable that.
#define WPP_CONDITIONAL_LEVEL_OVERRIDE(LEVEL, RET)          \
    BOOL bEnabled = WPP_LEVEL_ENABLED(LEVEL);               \
    __pragma(warning(push))                                 \
    __pragma(warning(disable: 4127))                        \
    if( LEVEL == LEVEL_COND )                               \
    {                                                       \
        if( S_OK == RET )                                   \
            bEnabled = WPP_LEVEL_ENABLED(LEVEL_VERBOSE);    \
        else                                                \
            bEnabled = WPP_LEVEL_ENABLED(LEVEL_ERROR);      \
    }                                                       \
    __pragma(warning(pop))


//*********************************************************
// MACRO: TRACE_METHOD_ENTRY
//
// begin_wpp config
// USEPREFIX (TRACE_METHOD_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_ENTRY(TRACE_METHOD_ENTRY_EXP);
// USESUFFIX (TRACE_METHOD_ENTRY, "Enter, this=0x%p", this);
// end_wpp

#define WPP_TRACE_METHOD_ENTRY_EXP_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_METHOD_ENTRY_EXP_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_METHOD_EXIT
//
// begin_wpp config
// USEPREFIX (TRACE_METHOD_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_EXIT(TRACE_METHOD_EXIT_EXP);
// USESUFFIX (TRACE_METHOD_EXIT, "Exit, this=0x%p", this);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_EXP_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_METHOD_EXIT_EXP_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_METHOD_EXIT_HR
//
// begin_wpp config
// USEPREFIX (TRACE_METHOD_EXIT_HR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_EXIT_HR(TRACE_METHOD_EXIT_HR_EXP, HR);
// USESUFFIX (TRACE_METHOD_EXIT_HR, "Exit, this=0x%p, hr=%!HRESULT!", this, HR);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_PRE(LEVEL, HR) { WPP_CONDITIONAL_LEVEL_OVERRIDE(LEVEL, HR)
#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_POST(LEVEL, HR) ;}
#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_ENABLED(LEVEL, HR) bEnabled
#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_LOGGER(LEVEL, HR) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_METHOD_EXIT_DWORD
// 
// begin_wpp config
// USEPREFIX (TRACE_METHOD_EXIT_DWORD, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_EXIT_DWORD(TRACE_METHOD_EXIT_DWORD_EXP, RETVAL);
// USESUFFIX (TRACE_METHOD_EXIT_DWORD, "Exit, this=0x%p, ret=0x%08Ix ", this, RETVAL);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_DWORD_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_METHOD_EXIT_DWORD_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_METHOD_EXIT_NTSTATUS
// 
// begin_wpp config
// USEPREFIX (TRACE_METHOD_EXIT_NTSTATUS, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_EXIT_NTSTATUS(TRACE_METHOD_EXIT_NTSTATUS_EXP, RETVAL);
// USESUFFIX (TRACE_METHOD_EXIT_NTSTATUS, "Exit, this=0x%p, ret=%!STATUS! ", this, RETVAL);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_NTSTATUS_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_METHOD_EXIT_NTSTATUS_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_METHOD_EXIT_PTR
// 
// begin_wpp config
// USEPREFIX (TRACE_METHOD_EXIT_PTR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_METHOD_EXIT_PTR(TRACE_METHOD_EXIT_PTR_EXP, RETVAL);
// USESUFFIX (TRACE_METHOD_EXIT_PTR,"Exit, this=0x%p, retptr=0x%p", this, RETVAL);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_PTR_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_METHOD_EXIT_PTR_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function entry/exit tracing macros.  Similar to the method tracing macros, but do not trace "this" pointer.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: TRACE_FUNCTION_ENTRY
//
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_ENTRY, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_ENTRY(TRACE_FUNCTION_ENTRY_EXP);
// USESUFFIX (TRACE_FUNCTION_ENTRY, "Enter");
// end_wpp

#define WPP_TRACE_FUNCTION_ENTRY_EXP_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_ENTRY_EXP_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_FUNCTION_EXIT
//
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_EXIT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_EXIT(TRACE_FUNCTION_EXIT_EXP);
// USESUFFIX (TRACE_FUNCTION_EXIT, "Exit");
// end_wpp

#define WPP_TRACE_FUNCTION_EXIT_EXP_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_EXIT_EXP_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RECORDER_TRACE_FUNCTION_EXIT_EXP_ARGS(LEVEL) WPP_RECORDER_LEVEL_ARGS(LEVEL)

//*********************************************************
// MACRO: TRACE_FUNCTION_EXIT_HR
// 
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_EXIT_HR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_EXIT_HR(TRACE_FUNCTION_EXIT_HR_EXP, HR);
// USESUFFIX (TRACE_FUNCTION_EXIT_HR, "Exit, hr=%!HRESULT!", HR);
// end_wpp

#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_PRE(LEVEL, HR) { WPP_CONDITIONAL_LEVEL_OVERRIDE(LEVEL, HR)
#define WPP_TRACE_METHOD_EXIT_HR_EXP_HR_POST(LEVEL, HR) ;}
#define WPP_TRACE_FUNCTION_EXIT_HR_EXP_HR_ENABLED(LEVEL, HR) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_EXIT_HR_EXP_HR_LOGGER(LEVEL, HR) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_FUNCTION_EXIT_DWORD
// 
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_EXIT_DWORD, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_EXIT_DWORD(TRACE_FUNCTION_EXIT_DWORD_EXP, RETVAL);
// USESUFFIX (TRACE_FUNCTION_EXIT_DWORD, "Exit, ret=0x%08Ix", RETVAL);
// end_wpp

#define WPP_TRACE_FUNCTION_EXIT_DWORD_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_EXIT_DWORD_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_FUNCTION_EXIT_NTSTATUS
// 
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_EXIT_NTSTATUS, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_EXIT_NTSTATUS(TRACE_FUNCTION_EXIT_NTSTATUS_EXP, RETVAL);
// USESUFFIX (TRACE_FUNCTION_EXIT_NTSTATUS, "Exit, status=%!STATUS!", RETVAL);
// end_wpp

#define WPP_TRACE_FUNCTION_EXIT_NTSTATUS_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_EXIT_NTSTATUS_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)


//*********************************************************
// MACRO: TRACE_FUNCTION_EXIT_PTR
// 
// begin_wpp config
// USEPREFIX (TRACE_FUNCTION_EXIT_PTR, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_FUNCTION_EXIT_PTR(TRACE_FUNCTION_EXIT_PTR_EXP, RETVAL);
// USESUFFIX (TRACE_FUNCTION_EXIT_PTR, "Exit, retptr=0x%p", RETVAL);
// end_wpp

#define WPP_TRACE_FUNCTION_EXIT_PTR_EXP_RETVAL_ENABLED(LEVEL, RETVAL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_FUNCTION_EXIT_PTR_EXP_RETVAL_LOGGER(LEVEL, RETVAL) WPP_LEVEL_LOGGER(LEVEL)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Generic tracing macros.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//*********************************************************
// MACRO: TRACE_LINE
//
// begin_wpp config
// USEPREFIX (TRACE_LINE, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_LINE(TRACE_LINE_EXP, MSG, ...);
// end_wpp

#define WPP_TRACE_LINE_EXP_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_TRACE_LINE_EXP_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)

//*********************************************************
// MACRO: TRACE_HRESULT
//
// begin_wpp config
// USEPREFIX (TRACE_HRESULT, "%!STDPREFIX!%!FUNC!:%s", " ");
// FUNC TRACE_HRESULT(TRACE_HRESULT_EXP, HR, MSG, ...);
// USESUFFIX (TRACE_HRESULT, ", ret=%!HRESULT!", HR);
// end_wpp

#define WPP_TRACE_HRESULT_EXP_HR_PRE(LEVEL, HR) { WPP_CONDITIONAL_LEVEL_OVERRIDE(LEVEL, HR)
#define WPP_TRACE_HRESULT_EXP_HR_POST(LEVEL, HR) ;}
#define WPP_TRACE_HRESULT_EXP_HR_ENABLED(LEVEL, HR) bEnabled
#define WPP_TRACE_HRESULT_EXP_HR_LOGGER(LEVEL, HR) WPP_LEVEL_LOGGER(LEVEL)


#define W32
#define WPP_CHECK_FOR_NULL_STRING //to prevent exceptions due to NULL strings

