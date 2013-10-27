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
#include "include/proxy/proxy_configuration.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/simple_mc_proxy_routing.hpp"
#include "include/proxy/simple_routing_data.hpp"
#include "include/proxy/igmp_sender.hpp"

#include <iostream>
#include <unistd.h>

void tester(int arg_count, char* args[]);
void test_log();
void test_mctables();
void test_MC_TestTool();
void test_mcproxy(int arg_count, char* args[]);
void test_test();

int main(int arg_count, char* args[])
{
    hc_set_default_log_fun(HC_LOG_TRACE_LVL);

    test_mcproxy(arg_count, args);

    //tester(arg_count, args);

    //test_test();
    return 0;
}

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

void test_mcproxy(int arg_count, char* args[])
{
    hc_set_default_log_fun(HC_LOG_ERROR_LVL);

    try {
        proxy p(arg_count, args);
    } catch (const char* e) {
        std::cout << e << std::endl;
    }

}

void tester(int arg_count, char* args[])
{
    if (arg_count > 2) {
        std::string if_name = args[1];
        mc_socket ms;
        ms.create_udp_ipv4_socket();
        if (std::string(args[2]).compare("1") == 0) {
            std::cout << "join group 239.99.99.99" << std::endl;
            if (!ms.join_group(addr_storage("239.99.99.99"), interfaces::get_if_index(if_name))) {
                std::cout << "join group error" << std::endl;
            }
            sleep(10);
        } else if (std::string(args[2]).compare("2") == 0) {
            std::cout << "join group 239.99.99.99" << std::endl;
            if (!ms.join_group(addr_storage("239.99.99.99"), interfaces::get_if_index(if_name))) {
                std::cout << "join group error" << std::endl;
            }

            std::cout << "set source filter INCLUDE_MODE with ip 123 and 124" << std::endl;
            if (!ms.set_source_filter(interfaces::get_if_index(if_name), addr_storage("239.99.99.99"), INCLUDE_MODE, {addr_storage("123.123.123.123"), addr_storage("124.124.124.124")})) {
                std::cout << "set source filter error" << std::endl;
            }

            sleep(10);
        } else if (std::string(args[2]).compare("3") == 0) {
            std::cout << "join group 239.99.99.99" << std::endl;
            if (!ms.join_group(addr_storage("239.99.99.99"), interfaces::get_if_index(if_name))) {
                std::cout << "join group error" << std::endl;
            }

            std::cout << "set source filter EXLCUDE_MODE with ip 125 and 124" << std::endl;
            if (!ms.set_source_filter(interfaces::get_if_index(if_name), addr_storage("239.99.99.99"), EXLCUDE_MODE , {addr_storage("125.125.125.125"), addr_storage("124.124.124.124")})) {
                std::cout << "set source filter error" << std::endl;
            }
            sleep(10);
        } else if (std::string(args[2]).compare("4") == 0) {
            std::cout << "choose multicast interface" << std::endl;
            if (!ms.choose_if(interfaces::get_if_index(if_name))) {
                
            }

            std::cout << "send packet: bob is cool" << std::endl;
            if (!ms.send_packet(addr_storage("239.99.99.99"), "bob is cool")) {
                std::cout << "send packet error" << std::endl;
            }
            return;
        }

    } else {
        std::cout << "arg musst be [ifname] [1], [2], [3] or [4] " << std::endl;
        std::cout << "[1]: join group 239.99.99.99" << std::endl;
        std::cout << "[2]: [1] and set source filter INCLUDE_MODE with ip 123.123.123.123 and 124.124.124.124" << std::endl;
        std::cout << "[3]: [1] and set source filter EXLCUDE_MODE with ip 124.124.124.124 and 125.125.125.125" << std::endl;
        std::cout << "[l]: send test packet to 239.99.99.99" << std::endl;

    }
}

void test_test()
{
    //mc_socket::test_all();
    //addr_storage::test_addr_storage_a();
    //addr_storage::test_addr_sto_b();
    //membership_db::test_arithmetic();
    //timers_values::test_timers_values();
    //timers_values::test_timers_values_copy();
    //timing::test_timing();
    //proxy_configuration::test_proxy_configuration();
    //worker::test_worker();
    proxy_instance::test_querier("dummy0");
    //simple_routing_data::test_simple_routing_data();
    //igmp_sender::test_igmp_sender();
}
