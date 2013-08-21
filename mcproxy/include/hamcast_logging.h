/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Dominik Charousset <dominik.charousset@haw-hamburg.de>
 *
 */

#ifndef HAMCAST_LOGGING_H
#define HAMCAST_LOGGING_H

/**
 * @defgroup Logging Logging.
 * @brief libhamcast provides its own logging mechanism. It's inteded
 *        for debugging of libhamcast, the middleware and technology
 *        modules only and must be enabled at compile by defining
 *        <code>HC_DEBUG_MODE</code>.
 *
 * Usage example (C++):
 * @include logging_example.txt
 * @{
 */

#ifdef __cplusplus
#include <sstream>
#include <iostream>
extern "C" {
#else
#include <stdio.h>
#endif

#ifdef HC_DOCUMENTATION

/**
 * @brief Constant for logging of trace informations.
 */
#define HC_LOG_TRACE_LVL

/**
 * @brief Constant for logging of debug informations.
 */
#define HC_LOG_DEBUG_LVL

/**
 * @brief Constant for logging of runtime informations.
 */
#define HC_LOG_INFO_LVL

/**
 * @brief Constant for logging of warnings.
 */
#define HC_LOG_WARN_LVL

/**
 * @brief Constant for logging of errors.
 */
#define HC_LOG_ERROR_LVL

/**
 * @brief Constant for logging of fatal errors.
 */
#define HC_LOG_FATAL_LVL

#else

#define HC_LOG_TRACE_LVL 0x0100
#define HC_LOG_DEBUG_LVL 0x0200
#define HC_LOG_INFO_LVL  0x0300
#define HC_LOG_WARN_LVL  0x0400
#define HC_LOG_ERROR_LVL 0x0500
#define HC_LOG_FATAL_LVL 0x0600

#endif

/**
 * @brief A function that could be used for logging.
 *
 * The first argument is the log level, the second argument is the
 * (human readable) name of the function where this log event occured
 * and the last argument is the message.
 */
typedef void (*hc_log_fun_t)(int, const char*, const char*);

/**
 * @brief Get a pointer to the current active log function.
 * @returns The current active log function (might be <code>NULL</code>).
 */
hc_log_fun_t hc_get_log_fun();

/**
 * @brief Set the log function to @p function_ptr.
 * @param function_ptr A custom logging implementation.
 */
void hc_set_log_fun(hc_log_fun_t function_ptr);

/**
 * @brief Invokes the function pointer returned by {@link hc_get_log_fun()}
 *        if not NULL.
 * @param log_lvl One of <code>{ HC_LOG_TRACE_LVL, HC_LOG_DEBUG_LVL,
 *                HC_LOG_INFO_LVL, HC_LOG_WARN_LVL,
 *                HC_LOG_ERROR_LVL, HC_LOG_FATAL_LVL }</code>.
 * @param function_name Usually <code>__FUNCTION__</code> or
 *                      <code>__PRETTY_FUNCTION__</code>.
 * @param log_msg The user defined log message.
 */
void hc_log(int log_lvl, const char* function_name, const char* log_msg);

/**
 * @brief Get a default logging implementation (one logfile per thread).
 * @param log_lvl The desired logging level.
 * @returns Set a log function that discards all log messages with
 *         <code>level < @p log_lvl</code>.
 */
void hc_set_default_log_fun(int log_lvl);

/**
 * @brief Check if libHAMcast was compiled with logging enabled.
 * @returns @c 1 if logging is enabled; otherwise @c 0
 */
int hc_logging_enabled();

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __GNUC__
#  define HC_FUN __PRETTY_FUNCTION__
#else
#  define HC_FUN __FUNCTION__
#endif

#if defined(HC_DOCUMENTATION)

/**
 * @brief Log tracing informations for the current function.
 *
 * If you're compiling C++ then this logs an ENTER event with @p message
 * and an EXIT event if the function is leaved.
 * @param message The user defined log message.
 * @note If you're compiling C++, @p message will be streamed. This allows you
 *       to use the stream operator:
 *       <code>HC_LOG_TRACE("arg1: " << arg1)</code>.
 */
#define HC_LOG_TRACE(message)

/**
 * @brief Log debug informations.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_DEBUG(message)

/**
 * @brief Log runtime informations.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_INFO(message)

/**
 * @brief Log warnings.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_WARN(message)

/**
 * @brief Log errors.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_ERROR(message)

/**
 * @brief Log fatal errors.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_FATAL(message)

/**
 * @brief Log tracing informations for the current scope.
 * @param scope_name The user defined name of the scope. This name is written
 *                   to the log instead of the enclosing function name.
 * @param message The user defined log message.
 * @see HC_LOG_TRACE(message)
 */
#define HC_LOG_SCOPE(scope_name, message)

#elif defined(__cplusplus)

#define HC_DO_LOG(message, loglvl)                                             \
    {                                                                          \
        std::ostringstream scoped_oss;                                         \
        scoped_oss << message;                                                 \
        std::string scoped_osss = scoped_oss.str();                            \
        hc_log( loglvl , HC_FUN , scoped_osss.c_str());                        \
    } ((void) 0)

namespace
{
template<int m_lvl>
struct HC_trace_helper {
    const char* m_fun;
    HC_trace_helper(const char* fun, const std::string& initmsg) : m_fun(fun) {
        std::string msg = "ENTER";
        if (!initmsg.empty()) {
            msg += ": ";
            msg += initmsg;
        }
        hc_log(m_lvl, m_fun, msg.c_str());
    }
    ~HC_trace_helper() {
        hc_log(m_lvl, m_fun, "LEAVE");
    }
};
}

#define HC_LOG_TRACE(message)                                                  \
    ::std::ostringstream hc_trace_helper_##__LINE__ ;                          \
    hc_trace_helper_##__LINE__ << message ;                                    \
    ::HC_trace_helper< HC_LOG_TRACE_LVL > hc_fun_HC_trace_helper_##__LINE__    \
        ( HC_FUN , hc_trace_helper_##__LINE__ .str() )

#define HC_LOG_SCOPE(scope_name, message)                                      \
    ::std::ostringstream hc_trace_helper_##__LINE__ ;                          \
    hc_trace_helper_##__LINE__ << message ;                                    \
    ::HC_trace_helper< HC_LOG_TRACE_LVL > hc_fun_HC_trace_helper_##__LINE__    \
        ( scope_name , hc_trace_helper_##__LINE__ .str() )

#define HC_PRINT(message) std::cerr << message << std::endl;

#else
#  define HC_DO_LOG(message, loglvl) hc_log( loglvl , HC_FUN , message)
#  define HC_LOG_TRACE(message) HC_DO_LOG(message, HC_LOG_TRACE_LVL)
#  define HC_LOG_SCOPE(scope_name, message) hc_log( loglvl , scope_name , message)
#  define HC_PRINT(message) printf("%s", message);
#endif

#ifndef HC_DOCUMENTATION
#  ifndef DEBUG_MODE
#    undef HC_DO_LOG
#    define HC_DO_LOG(unused1, unused2)
#    undef HC_LOG_TRACE
#    define HC_LOG_TRACE(unused)
#    undef HC_LOG_SCOPE
#    define HC_LOG_SCOPE(unused1, unused2)
#  endif
#  define HC_LOG_DEBUG(message) HC_DO_LOG(message, HC_LOG_DEBUG_LVL)
#  define HC_LOG_INFO(message)  HC_DO_LOG(message, HC_LOG_INFO_LVL)
#  define HC_LOG_WARN(message)  HC_DO_LOG(message, HC_LOG_WARN_LVL)
#  ifndef DEBUG_MODE
#    define HC_LOG_ERROR(message) HC_PRINT("ERROR: " << message)
#    define HC_LOG_FATAL(message) HC_PRINT("FATAL: " << message)
#  else
#    define HC_LOG_ERROR(message) HC_DO_LOG(message, HC_LOG_ERROR_LVL)
#    define HC_LOG_FATAL(message) HC_DO_LOG(message, HC_LOG_FATAL_LVL)
#  endif
#endif

// end of group Logging
/**
 * @}
 */

#endif // HAMCAST_LOGGING_H
