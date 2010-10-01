// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_
#pragma once

#include <string>
#include <cstring>
#include <sstream>

#include "base/basictypes.h"

//
// Optional message capabilities
// -----------------------------
// Assertion failed messages and fatal errors are displayed in a dialog box
// before the application exits. However, running this UI creates a message
// loop, which causes application messages to be processed and potentially
// dispatched to existing application windows. Since the application is in a
// bad state when this assertion dialog is displayed, these messages may not
// get processed and hang the dialog, or the application might go crazy.
//
// Therefore, it can be beneficial to display the error dialog in a separate
// process from the main application. When the logging system needs to display
// a fatal error dialog box, it will look for a program called
// "DebugMessage.exe" in the same directory as the application executable. It
// will run this application with the message as the command line, and will
// not include the name of the application as is traditional for easier
// parsing.
//
// The code for DebugMessage.exe is only one line. In WinMain, do:
//   MessageBox(NULL, GetCommandLineW(), L"Fatal Error", 0);
//
// If DebugMessage.exe is not found, the logging code will use a normal
// MessageBox, potentially causing the problems discussed above.


// Instructions
// ------------
//
// Make a bunch of macros for logging.  The way to log things is to stream
// things to LOG(<a particular severity level>).  E.g.,
//
//   LOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// The above will cause log messages to be output on the 1st, 11th, 21st, ...
// times it is executed.  Note that the special COUNTER value is used to
// identify which repetition is happening.
//
// The CHECK(condition) macro is active in both debug and release builds and
// effectively performs a LOG(FATAL) which terminates the process and
// generates a crashdump unless a debugger is attached.
//
// There are also "debug mode" logging macros like the ones above:
//
//   DLOG(INFO) << "Found cookies";
//
//   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.  LOG_IF and development flags also work well together
// because the code can be compiled away sometimes.
//
// We also have
//
//   LOG_ASSERT(assertion);
//   DLOG_ASSERT(assertion);
//
// which is syntactic sugar for {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// There are "verbose level" logging macros.  They look like
//
//   VLOG(1) << "I'm printed when you run the program with --v=1 or more";
//   VLOG(2) << "I'm printed when you run the program with --v=2 or more";
//
// These always log at the INFO log level (when they log at all).
// The verbose logging can also be turned on module-by-module.  For instance,
//    --vmodule=profile=2,icon_loader=1,browser_*=3 --v=0
// will cause:
//   a. VLOG(2) and lower messages to be printed from profile.{h,cc}
//   b. VLOG(1) and lower messages to be printed from icon_loader.{h,cc}
//   c. VLOG(3) and lower messages to be printed from files prefixed with
//      "browser"
//   d. VLOG(0) and lower messages to be printed from elsewhere
//
// The wildcarding functionality shown by (c) supports both '*' (match
// 0 or more characters) and '?' (match any single character) wildcards.
//
// There's also VLOG_IS_ON(n) "verbose level" condition macro. To be used as
//
//   if (VLOG_IS_ON(2)) {
//     // do some logging preparation and logging
//     // that can't be accomplished with just VLOG(2) << ...;
//   }
//
// There is also a VLOG_IF "verbose level" condition macro for sample
// cases, when some extra computation and preparation for logs is not
// needed.
//
//   VLOG_IF(1, (size > 1024))
//      << "I'm printed when size is more than 1024 and when you run the "
//         "program with --v=1 or more";
//
// We also override the standard 'assert' to use 'DLOG_ASSERT'.
//
// Lastly, there is:
//
//   PLOG(ERROR) << "Couldn't do foo";
//   DPLOG(ERROR) << "Couldn't do foo";
//   PLOG_IF(ERROR, cond) << "Couldn't do foo";
//   DPLOG_IF(ERROR, cond) << "Couldn't do foo";
//   PCHECK(condition) << "Couldn't do foo";
//   DPCHECK(condition) << "Couldn't do foo";
//
// which append the last system error to the message in string form (taken from
// GetLastError() on Windows and errno on POSIX).
//
// The supported severity levels for macros that allow you to specify one
// are (in increasing order of severity) INFO, WARNING, ERROR, ERROR_REPORT,
// and FATAL.
//
// Very important: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).
//
// Note the special severity of ERROR_REPORT only available/relevant in normal
// mode, which displays error dialog without terminating the program. There is
// no error dialog for severity ERROR or below in normal mode.
//
// There is also the special severity of DFATAL, which logs FATAL in
// debug mode, ERROR in normal mode.

namespace logging {

// Where to record logging output? A flat file and/or system debug log via
// OutputDebugString. Defaults on Windows to LOG_ONLY_TO_FILE, and on
// POSIX to LOG_ONLY_TO_SYSTEM_DEBUG_LOG (aka stderr).
enum LoggingDestination { LOG_NONE,
                          LOG_ONLY_TO_FILE,
                          LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
                          LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG };

// Indicates that the log file should be locked when being written to.
// Often, there is no locking, which is fine for a single threaded program.
// If logging is being done from multiple threads or there can be more than
// one process doing the logging, the file should be locked during writes to
// make each log outut atomic. Other writers will block.
//
// All processes writing to the log file must have their locking set for it to
// work properly. Defaults to DONT_LOCK_LOG_FILE.
enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };

// On startup, should we delete or append to an existing log file (if any)?
// Defaults to APPEND_TO_OLD_LOG_FILE.
enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

// TODO(avi): do we want to do a unification of character types here?
#if defined(OS_WIN)
typedef wchar_t PathChar;
#else
typedef char PathChar;
#endif

// Define different names for the BaseInitLoggingImpl() function depending on
// whether NDEBUG is defined or not so that we'll fail to link if someone tries
// to compile logging.cc with NDEBUG but includes logging.h without defining it,
// or vice versa.
#if NDEBUG
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_with_NDEBUG
#else
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_without_NDEBUG
#endif

// Implementation of the InitLogging() method declared below.  We use a
// more-specific name so we can #define it above without affecting other code
// that has named stuff "InitLogging".
void BaseInitLoggingImpl(const PathChar* log_file,
                         LoggingDestination logging_dest,
                         LogLockingState lock_log,
                         OldFileDeletionState delete_old);

// Sets the log file name and other global logging state. Calling this function
// is recommended, and is normally done at the beginning of application init.
// If you don't call it, all the flags will be initialized to their default
// values, and there is a race condition that may leak a critical section
// object if two threads try to do the first log at the same time.
// See the definition of the enums above for descriptions and default values.
//
// The default log file is initialized to "debug.log" in the application
// directory. You probably don't want this, especially since the program
// directory may not be writable on an enduser's system.
inline void InitLogging(const PathChar* log_file,
                        LoggingDestination logging_dest,
                        LogLockingState lock_log,
                        OldFileDeletionState delete_old) {
  BaseInitLoggingImpl(log_file, logging_dest, lock_log, delete_old);
}

// Sets the log level. Anything at or above this level will be written to the
// log file/displayed to the user (if applicable). Anything below this level
// will be silently ignored. The log level defaults to 0 (everything is logged)
// if this function is not called.
void SetMinLogLevel(int level);

// Gets the current log level.
int GetMinLogLevel();

// Gets the current vlog level for the given file (usually taken from
// __FILE__).

// Note that |N| is the size *with* the null terminator.
int GetVlogLevelHelper(const char* file_start, size_t N);

template <size_t N>
int GetVlogLevel(const char (&file)[N]) {
  return GetVlogLevelHelper(file, N);
}

// Sets the common items you want to be prepended to each log message.
// process and thread IDs default to off, the timestamp defaults to on.
// If this function is not called, logging defaults to writing the timestamp
// only.
void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount);

// Sets whether or not you'd like to see fatal debug messages popped up in
// a dialog box or not.
// Dialogs are not shown by default.
void SetShowErrorDialogs(bool enable_dialogs);

// Sets the Log Assert Handler that will be used to notify of check failures.
// The default handler shows a dialog box and then terminate the process,
// however clients can use this function to override with their own handling
// (e.g. a silent one for Unit Tests)
typedef void (*LogAssertHandlerFunction)(const std::string& str);
void SetLogAssertHandler(LogAssertHandlerFunction handler);
// Sets the Log Report Handler that will be used to notify of check failures
// in non-debug mode. The default handler shows a dialog box and continues
// the execution, however clients can use this function to override with their
// own handling.
typedef void (*LogReportHandlerFunction)(const std::string& str);
void SetLogReportHandler(LogReportHandlerFunction handler);

// Sets the Log Message Handler that gets passed every log message before
// it's sent to other log destinations (if any).
// Returns true to signal that it handled the message and the message
// should not be sent to other log destinations.
typedef bool (*LogMessageHandlerFunction)(int severity, const std::string& str);
void SetLogMessageHandler(LogMessageHandlerFunction handler);

typedef int LogSeverity;
const LogSeverity LOG_INFO = 0;
const LogSeverity LOG_WARNING = 1;
const LogSeverity LOG_ERROR = 2;
const LogSeverity LOG_ERROR_REPORT = 3;
const LogSeverity LOG_FATAL = 4;
const LogSeverity LOG_NUM_SEVERITIES = 5;

// LOG_DFATAL_LEVEL is LOG_FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
const LogSeverity LOG_DFATAL_LEVEL = LOG_ERROR;
#else
const LogSeverity LOG_DFATAL_LEVEL = LOG_FATAL;
#endif

// A few definitions of macros that don't generate much code. These are used
// by LOG() and LOG_IF, etc. Since these are used all over our code, it's
// better to have compact code for these operations.
#define COMPACT_GOOGLE_LOG_EX_INFO(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_INFO , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_WARNING(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_WARNING , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_ERROR(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_ERROR , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_ERROR_REPORT(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, \
                     logging::LOG_ERROR_REPORT , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_FATAL(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_FATAL , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_DFATAL(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, \
                     logging::LOG_DFATAL_LEVEL , ##__VA_ARGS__)

#define COMPACT_GOOGLE_LOG_INFO \
  COMPACT_GOOGLE_LOG_EX_INFO(LogMessage)
#define COMPACT_GOOGLE_LOG_WARNING \
  COMPACT_GOOGLE_LOG_EX_WARNING(LogMessage)
#define COMPACT_GOOGLE_LOG_ERROR \
  COMPACT_GOOGLE_LOG_EX_ERROR(LogMessage)
#define COMPACT_GOOGLE_LOG_ERROR_REPORT \
  COMPACT_GOOGLE_LOG_EX_ERROR_REPORT(LogMessage)
#define COMPACT_GOOGLE_LOG_FATAL \
  COMPACT_GOOGLE_LOG_EX_FATAL(LogMessage)
#define COMPACT_GOOGLE_LOG_DFATAL \
  COMPACT_GOOGLE_LOG_EX_DFATAL(LogMessage)

// wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
// substituted with 0, and it expands to COMPACT_GOOGLE_LOG_0. To allow us
// to keep using this syntax, we define this macro to do the same thing
// as COMPACT_GOOGLE_LOG_ERROR, and also define ERROR the same way that
// the Windows SDK does for consistency.
#define ERROR 0
#define COMPACT_GOOGLE_LOG_EX_0(ClassName, ...) \
  COMPACT_GOOGLE_LOG_EX_ERROR(ClassName , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_0 COMPACT_GOOGLE_LOG_ERROR

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token COMPACT_GOOGLE_LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.
//
// We can't do any caching tricks with VLOG_IS_ON() like the
// google-glog version since it requires GCC extensions.  This means
// that using the v-logging functions in conjunction with --vmodule
// may be slow.
#define VLOG_IS_ON(verboselevel) \
  (logging::GetVlogLevel(__FILE__) >= (verboselevel))

#define LOG(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()
#define SYSLOG(severity) LOG(severity)
#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))

// TODO(akalin): Add more VLOG variants, e.g. VPLOG.

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) LOG_IF(severity, condition)
#define VLOG_IF(verboselevel, condition) \
  LOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

#if defined(OS_WIN)
#define LOG_GETLASTERROR(severity) \
  COMPACT_GOOGLE_LOG_EX_ ## severity(Win32ErrorLogMessage, \
      ::logging::GetLastSystemErrorCode()).stream()
#define LOG_GETLASTERROR_MODULE(severity, module) \
  COMPACT_GOOGLE_LOG_EX_ ## severity(Win32ErrorLogMessage, \
      ::logging::GetLastSystemErrorCode(), module).stream()
// PLOG is the usual error logging macro for each platform.
#define PLOG(severity) LOG_GETLASTERROR(severity)
#define DPLOG(severity) DLOG_GETLASTERROR(severity)
#elif defined(OS_POSIX)
#define LOG_ERRNO(severity) \
  COMPACT_GOOGLE_LOG_EX_ ## severity(ErrnoLogMessage, \
      ::logging::GetLastSystemErrorCode()).stream()
// PLOG is the usual error logging macro for each platform.
#define PLOG(severity) LOG_ERRNO(severity)
#define DPLOG(severity) DLOG_ERRNO(severity)
// TODO(tschmelcher): Should we add OSStatus logging for Mac?
#endif

#define PLOG_IF(severity, condition) \
  !(condition) ? (void) 0 : logging::LogMessageVoidify() & PLOG(severity)

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.
#define CHECK(condition) \
  LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

#define PCHECK(condition) \
  PLOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

// A container for a string pointer which can be evaluated to a bool -
// true iff the pointer is NULL.
struct CheckOpString {
  CheckOpString(std::string* str) : str_(str) { }
  // No destructor: if str_ is non-NULL, we're about to LOG(FATAL),
  // so there's no point in cleaning up str_.
  operator bool() const { return str_ != NULL; }
  std::string* str_;
};

// Build the error message string.  This is separate from the "Impl"
// function template because it is not performance critical and so can
// be out of line, while the "Impl" code should be inline.
template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  std::ostringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  std::string* msg = new std::string(ss.str());
  return msg;
}

// MSVC doesn't like complex extern templates and DLLs.
#if !defined(COMPILER_MSVC)
// Commonly used instantiations of MakeCheckOpString<>. Explicitly instantiated
// in logging.cc.
extern template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
extern template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
extern template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
extern template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
extern template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ et al below.
#define CHECK_OP(name, op, val1, val2)  \
  if (logging::CheckOpString _result = \
      logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
    logging::LogMessage(__FILE__, __LINE__, _result).stream()

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected) \
  std::string* Check##func##expected##Impl(const char* s1, \
                                           const char* s2, \
                                           const char* names);
DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(_stricmp, true)
DECLARE_CHECK_STROP_IMPL(_stricmp, false)
#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define CHECK_STROP(func, op, expected, s1, s2) \
  while (CheckOpString _result = \
      logging::Check##func##expected##Impl((s1), (s2), \
                                           #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_

// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(_stricmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(_stricmp, !=, false, s1, s2)

#define CHECK_INDEX(I,A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B,A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define CHECK_EQ(val1, val2) CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(GT, > , val1, val2)

// http://crbug.com/16512 is open for a real fix for this.  For now, Windows
// uses OFFICIAL_BUILD and other platforms use the branding flag when NDEBUG is
// defined.
#if ( defined(OS_WIN) && defined(OFFICIAL_BUILD)) || \
    (!defined(OS_WIN) && defined(NDEBUG) && defined(GOOGLE_CHROME_BUILD))
// In order to have optimized code for official builds, remove DLOGs and
// DCHECKs.
#define ENABLE_DLOG 0
#define ENABLE_DCHECK 0

#elif defined(NDEBUG)
// Otherwise, if we're a release build, remove DLOGs but not DCHECKs
// (since those can still be turned on via a command-line flag).
#define ENABLE_DLOG 0
#define ENABLE_DCHECK 1

#else
// Otherwise, we're a debug build so enable DLOGs and DCHECKs.
#define ENABLE_DLOG 1
#define ENABLE_DCHECK 1
#endif

// Definitions for DLOG et al.

#if ENABLE_DLOG

#define DLOG(severity) LOG(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

#if defined(OS_WIN)
#define DLOG_GETLASTERROR(severity) LOG_GETLASTERROR(severity)
#define DLOG_GETLASTERROR_MODULE(severity, module) \
  LOG_GETLASTERROR_MODULE(severity, module)
#elif defined(OS_POSIX)
#define DLOG_ERRNO(severity) LOG_ERRNO(severity)
#endif

#define DPLOG_IF(severity, condition) PLOG_IF(severity, condition)

#else  // ENABLE_DLOG

#define DLOG(severity) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_IF(severity, condition) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
  true ? (void) 0 : LOG_ASSERT(condition)

#if defined(OS_WIN)
#define DLOG_GETLASTERROR(severity) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG_GETLASTERROR(severity)
#define DLOG_GETLASTERROR_MODULE(severity, module) \
  true ? (void) 0 : logging::LogMessageVoidify() & \
      LOG_GETLASTERROR_MODULE(severity, module)
#elif defined(OS_POSIX)
#define DLOG_ERRNO(severity) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG_ERRNO(severity)
#endif

#define DPLOG_IF(severity, condition) \
  true ? (void) 0 : logging::LogMessageVoidify() & PLOG(severity)

#endif  // ENABLE_DLOG

// DEBUG_MODE is for uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// We tie its state to ENABLE_DLOG.
enum { DEBUG_MODE = ENABLE_DLOG };

#undef ENABLE_DLOG

// Definitions for DCHECK et al.

// This macro can be followed by a sequence of stream parameters in
// non-debug mode. The DCHECK and friends macros use this so that
// the expanded expression DCHECK(foo) << "asdf" is still syntactically
// valid, even though the expression will get optimized away.
#define DCHECK_EAT_STREAM_PARAMETERS \
  logging::LogMessage(__FILE__, __LINE__).stream()

#if ENABLE_DCHECK

#ifndef NDEBUG
// On a regular debug build, we want to have DCHECKS enabled.

#define DCHECK(condition) CHECK(condition)
#define DPCHECK(condition) PCHECK(condition)

// Helper macro for binary operators.
// Don't use this macro directly in your code, use DCHECK_EQ et al below.
#define DCHECK_OP(name, op, val1, val2)  \
  if (logging::CheckOpString _result = \
      logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
    logging::LogMessage(__FILE__, __LINE__, _result).stream()

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define DCHECK_STROP(func, op, expected, s1, s2) \
  while (CheckOpString _result = \
      logging::Check##func##expected##Impl((s1), (s2), \
                                           #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_

// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. DCHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define DCHECK_STREQ(s1, s2) DCHECK_STROP(strcmp, ==, true, s1, s2)
#define DCHECK_STRNE(s1, s2) DCHECK_STROP(strcmp, !=, false, s1, s2)
#define DCHECK_STRCASEEQ(s1, s2) DCHECK_STROP(_stricmp, ==, true, s1, s2)
#define DCHECK_STRCASENE(s1, s2) DCHECK_STROP(_stricmp, !=, false, s1, s2)

#define DCHECK_INDEX(I,A) DCHECK(I < (sizeof(A)/sizeof(A[0])))
#define DCHECK_BOUND(B,A) DCHECK(B <= (sizeof(A)/sizeof(A[0])))

#else  // NDEBUG
// On a regular release build we want to be able to enable DCHECKS through the
// command line.

// Set to true in InitLogging when we want to enable the dchecks in release.
extern bool g_enable_dcheck;
#define DCHECK(condition) \
    !logging::g_enable_dcheck ? void (0) : \
        LOG_IF(ERROR_REPORT, !(condition)) << "Check failed: " #condition ". "

#define DPCHECK(condition) \
    !logging::g_enable_dcheck ? void (0) : \
        PLOG_IF(ERROR_REPORT, !(condition)) << "Check failed: " #condition ". "

// Helper macro for binary operators.
// Don't use this macro directly in your code, use DCHECK_EQ et al below.
#define DCHECK_OP(name, op, val1, val2)  \
  if (logging::g_enable_dcheck) \
    if (logging::CheckOpString _result = \
        logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
      logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR_REPORT, \
                          _result).stream()

#define DCHECK_STREQ(str1, str2) \
  while (false) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRNE(str1, str2) \
  while (false) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASENE(str1, str2) \
  while (false) DCHECK_EAT_STREAM_PARAMETERS

#endif  // NDEBUG

// Equality/Inequality checks - compare two values, and log a LOG_FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   DCHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   DCHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These may not compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define DCHECK_EQ(val1, val2) DCHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2) DCHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2) DCHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2) DCHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2) DCHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2) DCHECK_OP(GT, > , val1, val2)

#else  // ENABLE_DCHECK

// In order to avoid variable unused warnings for code that only uses a
// variable in a CHECK, we make sure to use the macro arguments.

#define DCHECK(condition) \
  while (false && (condition)) DCHECK_EAT_STREAM_PARAMETERS

#define DPCHECK(condition) \
  while (false && (condition)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_EQ(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_NE(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_LE(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_LT(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_GE(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_GT(val1, val2) \
  while (false && (val1) == (val2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STREQ(str1, str2) \
  while (false && (str1) == (str2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false && (str1) == (str2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRNE(str1, str2) \
  while (false && (str1) == (str2)) DCHECK_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASENE(str1, str2) \
  while (false && (str1) == (str2)) DCHECK_EAT_STREAM_PARAMETERS

#endif  // ENABLE_DCHECK
#undef ENABLE_DCHECK

#undef DCHECK_EAT_STREAM_PARAMETERS

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <class t1, class t2> \
  inline std::string* Check##name##Impl(const t1& v1, const t2& v2, \
                                        const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  } \
  inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  }
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define NOTREACHED() DCHECK(false)

// Redefine the standard assert to use our nice log files
#undef assert
#define assert(x) DLOG_ASSERT(x)

// This class more or less represents a particular log message.  You
// create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to the appropriate destination.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though.  You should use the LOG() macro (and variants thereof)
// above.
class LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity, int ctr);

  // Two special constructors that generate reduced amounts of code at
  // LOG call sites for common cases.
  //
  // Used for LOG(INFO): Implied are:
  // severity = LOG_INFO, ctr = 0
  //
  // Using this constructor instead of the more complex constructor above
  // saves a couple of bytes per call site.
  LogMessage(const char* file, int line);

  // Used for LOG(severity) where severity != INFO.  Implied
  // are: ctr = 0
  //
  // Using this constructor instead of the more complex constructor above
  // saves a couple of bytes per call site.
  LogMessage(const char* file, int line, LogSeverity severity);

  // A special constructor used for check failures.
  // Implied severity = LOG_FATAL
  LogMessage(const char* file, int line, const CheckOpString& result);

  // A special constructor used for check failures, with the option to
  // specify severity.
  LogMessage(const char* file, int line, LogSeverity severity,
             const CheckOpString& result);

  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  void Init(const char* file, int line);

  LogSeverity severity_;
  std::ostringstream stream_;
  size_t message_start_;  // Offset of the start of the message (past prefix
                          // info).
#if defined(OS_WIN)
  // Stores the current value of GetLastError in the constructor and restores
  // it in the destructor by calling SetLastError.
  // This is useful since the LogMessage class uses a lot of Win32 calls
  // that will lose the value of GLE and the code that called the log function
  // will have lost the thread error value when the log call returns.
  class SaveLastError {
   public:
    SaveLastError();
    ~SaveLastError();

    unsigned long get_error() const { return last_error_; }

   protected:
    unsigned long last_error_;
  };

  SaveLastError last_error_;
#endif

  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// A non-macro interface to the log facility; (useful
// when the logging level is not a compile-time constant).
inline void LogAtLevel(int const log_level, std::string const &msg) {
  LogMessage(__FILE__, __LINE__, log_level).stream() << msg;
}

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

#if defined(OS_WIN)
typedef unsigned long SystemErrorCode;
#elif defined(OS_POSIX)
typedef int SystemErrorCode;
#endif

// Alias for ::GetLastError() on Windows and errno on POSIX. Avoids having to
// pull in windows.h just for GetLastError() and DWORD.
SystemErrorCode GetLastSystemErrorCode();

#if defined(OS_WIN)
// Appends a formatted system message of the GetLastError() type.
class Win32ErrorLogMessage {
 public:
  Win32ErrorLogMessage(const char* file,
                       int line,
                       LogSeverity severity,
                       SystemErrorCode err,
                       const char* module);

  Win32ErrorLogMessage(const char* file,
                       int line,
                       LogSeverity severity,
                       SystemErrorCode err);

  std::ostream& stream() { return log_message_.stream(); }

  // Appends the error message before destructing the encapsulated class.
  ~Win32ErrorLogMessage();

 private:
  SystemErrorCode err_;
  // Optional name of the module defining the error.
  const char* module_;
  LogMessage log_message_;

  DISALLOW_COPY_AND_ASSIGN(Win32ErrorLogMessage);
};
#elif defined(OS_POSIX)
// Appends a formatted system message of the errno type
class ErrnoLogMessage {
 public:
  ErrnoLogMessage(const char* file,
                  int line,
                  LogSeverity severity,
                  SystemErrorCode err);

  std::ostream& stream() { return log_message_.stream(); }

  // Appends the error message before destructing the encapsulated class.
  ~ErrnoLogMessage();

 private:
  SystemErrorCode err_;
  LogMessage log_message_;

  DISALLOW_COPY_AND_ASSIGN(ErrnoLogMessage);
};
#endif  // OS_WIN

// Closes the log file explicitly if open.
// NOTE: Since the log file is opened as necessary by the action of logging
//       statements, there's no guarantee that it will stay closed
//       after this call.
void CloseLogFile();

// Async signal safe logging mechanism.
void RawLog(int level, const char* message);

#define RAW_LOG(level, message) logging::RawLog(logging::LOG_ ## level, message)

#define RAW_CHECK(condition)                                                   \
  do {                                                                         \
    if (!(condition))                                                          \
      logging::RawLog(logging::LOG_FATAL, "Check failed: " #condition "\n");   \
  } while (0)

}  // namespace logging

// These functions are provided as a convenience for logging, which is where we
// use streams (it is against Google style to use streams in other places). It
// is designed to allow you to emit non-ASCII Unicode strings to the log file,
// which is normally ASCII. It is relatively slow, so try not to use it for
// common cases. Non-ASCII characters will be converted to UTF-8 by these
// operators.
std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr) {
  return out << wstr.c_str();
}

// The NOTIMPLEMENTED() macro annotates codepaths which have
// not been implemented yet.
//
// The implementation of this macro is controlled by NOTIMPLEMENTED_POLICY:
//   0 -- Do nothing (stripped by compiler)
//   1 -- Warn at compile time
//   2 -- Fail at compile time
//   3 -- Fail at runtime (DCHECK)
//   4 -- [default] LOG(ERROR) at runtime
//   5 -- LOG(ERROR) at runtime, only once per call-site

#ifndef NOTIMPLEMENTED_POLICY
// Select default policy: LOG(ERROR)
#define NOTIMPLEMENTED_POLICY 4
#endif

#if defined(COMPILER_GCC)
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the NOTIMPLEMENTED message.
#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED() ;
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED() do {\
  static int count = 0;\
  LOG_IF(ERROR, 0 == count++) << NOTIMPLEMENTED_MSG;\
} while(0)
#endif

#endif  // BASE_LOGGING_H_
