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
 */

#ifndef HAMCAST_LOGGING_H
#define HAMCAST_LOGGING_H

#ifdef __cplusplus
#include <sstream>
extern "C" {
#endif

#define HC_LOG_TRACE_LVL 0x0100
#define HC_LOG_DEBUG_LVL 0x0200
#define HC_LOG_INFO_LVL  0x0300
#define HC_LOG_WARN_LVL  0x0400
#define HC_LOG_ERROR_LVL 0x0500
#define HC_LOG_FATAL_LVL 0x0600

typedef void (*hc_log_fun_t)(int, const char*, const char*);

/**
 * @brief Get a pointer to the current active log function.
 */
hc_log_fun_t hc_get_log_fun();

/**
 * @brief Set the log function to @p function_ptr.
 */
void hc_set_log_fun(hc_log_fun_t function_ptr);

/**
 * @brief Invokes the function pointer returned by {@link hc_get_log_fun()}
 *        if not NULL.
 */
void hc_log(int log_lvl, const char* function_name, const char* log_msg);

/**
 * @brief Get a default logging implementation (one logfile per thread).
 * @param log_lvl The desired logging level.
 */
void hc_set_default_log_fun(int log_lvl);

#ifdef __cplusplus
}
#endif

#ifdef __GNUC__
#  define HC_FUN __PRETTY_FUNCTION__
#else
#  define HC_FUN __FUNCTION__
#endif

#ifdef __cplusplus

#define HC_DO_LOG(message, loglvl)                                             \
    {                                                                          \
        std::ostringstream scoped_oss;                                         \
        scoped_oss << message;                                                 \
        std::string scoped_osss = scoped_oss.str();                            \
		hc_log( loglvl , HC_FUN , scoped_osss.c_str());                        \
    } ((void) 0)

namespace {
template<int m_lvl>
struct HC_trace_helper
{
    const char* m_fun;
    HC_trace_helper(const char* fun, const std::string& initmsg) : m_fun(fun)
    {
        std::string msg = "ENTER";
        if (!initmsg.empty())
        {
            msg += ": ";
            msg += initmsg;
        }
        hc_log(m_lvl, m_fun, msg.c_str());
    }
    ~HC_trace_helper() { hc_log(m_lvl, m_fun, "LEAVE"); }
};
}

#define HC_LOG_TRACE(message)                                                  \
    ::std::ostringstream hc_trace_helper_##__LINE__ ;                          \
    hc_trace_helper_##__LINE__ << message ;                                    \
    ::HC_trace_helper< HC_LOG_TRACE_LVL > hc_fun_HC_trace_helper_##__LINE__    \
		( HC_FUN , hc_trace_helper_##__LINE__ .str() )

#else
#  define HC_DO_LOG(message, loglvl) hc_log( loglvl , HC_FUN , message)
#  define HC_LOG_TRACE(message) HC_DO_LOG(message, HC_LOG_TRACE_LVL)
#endif

#define HC_LOG_DEBUG(message) HC_DO_LOG(message, HC_LOG_DEBUG_LVL)
#define HC_LOG_INFO(message)  HC_DO_LOG(message, HC_LOG_INFO_LVL)
#define HC_LOG_WARN(message)  HC_DO_LOG(message, HC_LOG_WARN_LVL)
#define HC_LOG_ERROR(message) HC_DO_LOG(message, HC_LOG_ERROR_LVL)
#define HC_LOG_FATAL(message) HC_DO_LOG(message, HC_LOG_FATAL_LVL)

#endif // HAMCAST_LOGGING_H
