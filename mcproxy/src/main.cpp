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

#ifndef TESTER
    test_mcproxy(arg_count, args);
    //test_test();


#else
    tester(arg_count, args);
#endif

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

    auto help = []() {
        std::cout << "tester [<ifname>] [ipv4|ip6] [<group>] [<action>] [time in sec (default=10)]" << std::endl;
        std::cout << "\te.g. eth0 ipv4 1 1 60" << std::endl;
        std::cout << std::endl;
        std::cout << "ipv4 [group]: " << "[1]= 239.1.1.1; [9]= 239.9.9.9; [255]= ..." << std::endl;
        std::cout << "ipv6 [group]: " << "[1]= FF05::1:1; [9]= FF05::9:9; [FFFF]= ...."  << std::endl;
        std::cout << std::endl;
        std::cout << "[action]: " << "[1]= join group [group]" << std::endl;
        std::cout << "\t[2]: [1] and set source filter INCLUDE_MODE with ip 1, 2, 3, 4" << std::endl;
        std::cout << "\t[3]: [1] and set source filter INCLUDE_MODE with ip 5, 6, 7, 8" << std::endl;
        std::cout << "\t[4]: [1] and set source filter EXLCUDE_MODE with ip 3, 4, 5, 6" << std::endl;
        std::cout << "\t[5]: [1] and set source filter EXLCUDE_MODE with ip 1, 2, 7, 8" << std::endl;
        std::cout << "\t[6]: send test packet to [group]" << std::endl;
        exit(0);
    };

    const int IF_NAME = 1;
    const int IP_VERSION = 2;
    const int GROUP = 3;
    const int ACTION = 4;
    const int TIME = 5;

    std::cout << arg_count << std::endl;
    if (arg_count - 1 == ACTION || arg_count - 1 == TIME) {
        std::string if_name = args[IF_NAME];

        mc_socket ms;
        group_mem_protocol mem_protocol;
        addr_storage gaddr;
        int sleep_time;

        std::vector<addr_storage> g(9);

        if (std::string(args[IP_VERSION]).compare("ipv4") == 0 ) {
            mem_protocol = IGMPv3;
            ms.create_udp_ipv4_socket();

            for (unsigned int i = 1; i < g.size(); ++i) {
                std::ostringstream s;
                s << i << i << "." << i << "." << i << "." << i;
                g[i] = s.str();
            }
        } else if (std::string(args[IP_VERSION]).compare("ipv6") == 0 ) {
            mem_protocol = MLDv2;
            ms.create_udp_ipv6_socket();

            for (unsigned int i = 1; i < g.size(); ++i) {
                std::ostringstream s;
                s << i << "::" << i << ":" << i << ":" << i;
                g[i] = s.str();
            }
        } else {
            std::cout << "unknown protocol version: " << args[IP_VERSION] << std::endl;
            return;
        }

        if (is_IPv4(mem_protocol)) {
            std::ostringstream s;
            s << "239." << args[GROUP] << "." << args[GROUP] << "." << args[GROUP];
            gaddr = s.str();
            if (!gaddr.is_valid()) {
                std::cout << "unknown group: " << s.str()  << std::endl;
                return;
            }
        } else if (is_IPv6(mem_protocol)) {
            std::ostringstream s;
            s << "FF05::" << args[GROUP] << ":" << args[GROUP];
            gaddr = s.str();
            if (!gaddr.is_valid()) {
                std::cout << "unknown group: " << s.str()  << std::endl;
                return;
            }

        } else {
            std::cout << "unknon protocol version: " << args[IP_VERSION] << std::endl;
            return;
        }

        if (arg_count == TIME) {
            try {
                sleep_time = std::stoi(args[TIME]);
            } catch (std::logic_error e) {
                std::cout << "failed to parse time" << std::endl;
                return;
            }
        } else {
            sleep_time = 10;
        }

        std::string ac = args[ACTION];
        if (ac.compare("1") == 0 || ac.compare("2") == 0 || ac.compare("3") == 0 || ac.compare("4") == 0 || ac.compare("5") == 0) {
            std::cout << "join group " << gaddr << std::endl;
            if (!ms.join_group(gaddr, interfaces::get_if_index(if_name))) {
                std::cout << "join group error" << std::endl;
            }

            if (ac.compare("2") == 0) {
                std::cout << "set source filter INCLUDE_MODE with ip 1, 2, 3, 4" << std::endl;
                if (!ms.set_source_filter(interfaces::get_if_index(if_name), gaddr, INCLUDE_MODE, {g[1], g[2], g[3], g[4]})) {
                    std::cout << "set source filter error" << std::endl;
                }
            } else if (ac.compare("3") == 0) {
                std::cout << "set source filter INCLUDE_MODE with ip 5, 6, 7, 8" << std::endl;
                if (!ms.set_source_filter(interfaces::get_if_index(if_name), gaddr, INCLUDE_MODE, {g[5], g[6], g[7], g[8]})) {
                    std::cout << "set source filter error" << std::endl;
                }
            } else if (ac.compare("4") == 0) {
                std::cout << "set source filter EXLCUDE_MODE with ip 3, 4, 5, 6" << std::endl;
                if (!ms.set_source_filter(interfaces::get_if_index(if_name), gaddr, EXLCUDE_MODE , {g[3], g[4], g[5], g[6]})) {
                    std::cout << "set source filter error" << std::endl;
                }
            } else if (ac.compare("5") == 0) {
                std::cout << "set source filter EXLCUDE_MODE with ip 1, 2, 7, 8" << std::endl;
                if (!ms.set_source_filter(interfaces::get_if_index(if_name), gaddr, EXLCUDE_MODE , {g[1], g[2], g[7], g[8]})) {
                    std::cout << "set source filter error" << std::endl;
                }
            }
            sleep(sleep_time);
            return;
        } else if (ac.compare("6") == 0) {
            std::cout << "choose multicast interface" << std::endl;
            if (!ms.choose_if(interfaces::get_if_index(if_name))) {
                std::cout << "choose interface error" << std::endl;
            }

            std::cout << "send packet: \"bob is cool\" to port 1234 " << std::endl;
            if (!ms.send_packet(gaddr.set_port(1234), "bob is cool")) {
                std::cout << "send packet error" << std::endl;
            }
            return;
        }
    }

    help();
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
