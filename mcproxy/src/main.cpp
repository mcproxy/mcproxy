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
#include "include/utils/if_prop.hpp"
#include "include/utils/mc_socket.hpp"
#include "include/utils/mroute_socket.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/proxy/proxy.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/check_if.hpp"
#include "include/utils/if_prop.hpp"
#include "include/proxy/membership_db.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/timers_values.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/simple_mc_proxy_routing.hpp"
#include "include/proxy/simple_routing_data.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/parser/configuration.hpp"
#include "include/tester/tester.hpp"

#include <iostream>
#include <unistd.h>

void test_log();
void test_test();

int main(int arg_count, char* args[])
{
#ifdef TESTER
    try {
        tester(arg_count, args);
    } catch (const char* e) {
        std::cout << e << std::endl;
    }
#else
    try {
        proxy p(arg_count, args);
    } catch (const char* e) {
        std::cout << e << std::endl;
    }

    //test_test();
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

void test_test()
{
    hc_set_default_log_fun(HC_LOG_TRACE_LVL);
    //mc_socket::test_all();
    //addr_storage::test_addr_storage_a();
    //addr_storage::test_addr_storage_b();
    //membership_db::test_arithmetic();
    //timers_values::test_timers_values();
    //timers_values::test_timers_values_copy();
    //timing::test_timing();
    //worker::test_worker();
    //proxy_instance::test_querier("lo");
    //simple_routing_data::test_simple_routing_data();
    //igmp_sender::test_igmp_sender();
    //mroute_socket::quick_test();
    //configuration::test_configuration();
    //if_prop::test_if_prop();
}
#endif /* DEBUG_MODE */
