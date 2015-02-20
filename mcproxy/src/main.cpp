/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */

#include "include/hamcast_logging.h"
#include "include/proxy/proxy.hpp"

#include "testing/ut_bootstrap.hpp"

#include <iostream>
#include <unistd.h>

void test_log();

int main(int arg_count, char* args[])
{
#ifdef TESTER
    try {
        tester(arg_count, args);
    } catch (const char* e) {
        std::cout << e << std::endl;
    }
#elif UNIT_TESTS
    //unit tests bootstrap
    ut_bootstrap(arg_count, args);
#else
    try {
        proxy p(arg_count, args);
    } catch (const char* e) {
        std::cout << e << std::endl;
    }
#endif

    return 0;
}

#ifdef DEBUG_MODE
void test_log()
{
    hc_set_default_log_fun(HC_LOG_TRACE_LVL);
    HC_LOG_TRACE("");

    HC_LOG_DEBUG("HC_LOG_DEBUG");
    HC_LOG_INFO("HC_LOG_INFO");
    HC_LOG_WARN("HC_LOG_WARN");
    HC_LOG_ERROR("HC_LOG_ERROR");
    HC_LOG_FATAL("HC_LOG_FATAL");
}

#endif /* DEBUG_MODE */
