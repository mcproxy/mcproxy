/*
 *
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
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */


#include "include/hamcast_logging.h"
#include "include/tester/tester.hpp"
#include "include/utils/mc_socket.hpp"
#include "include/proxy/def.hpp"
#include "include/proxy/interfaces.hpp"

#include <unistd.h>

#include <vector>


tester::tester(int arg_count, char* args[])
{
    const int FILE_NAME = 1;

    std::cout << "start tester" << std::endl;

    if (arg_count == FILE_NAME - 1) {
        m_config_map.read_ini(args[FILE_NAME]);
    } else {
        m_config_map.read_ini(TESTER_DEFAULT_CONIG_PATH);
    }

    //std::cout << "m_config_map.has_group(join)" << m_config_map.has_group("join") << std::endl;
    //std::cout << "m_config_map.has_group(send)" << m_config_map.has_group("send") << std::endl;
    //std::cout << "m_config_map.has_group(xxx)" << m_config_map.has_group("xxx") << std::endl;

    //std::cout << "tester finished" << std::endl;

    //for (auto it_group = m_config_map.begin(); it_group != m_config_map.end(); ++it_group) {
    //std::cout << it_group->first << std::endl;
    //for (auto it_key = it_group->second.begin(); it_key != it_group->second.end(); ++it_key) {
    //std::cout << it_key->first <<  it_key->second << std::endl;
    //}
    //}

    run(arg_count, args);
}

void tester::run(int arg_count, char* args[])
{
    if (arg_count == 2) {
        std::string to_do = args[1];
        if (!m_config_map.has_group(to_do)) {
            std::cout << "group " << to_do << " not found" << std::endl;
            return;
        }

        std::string if_name = m_config_map.get(to_do, "interface");
        if (if_name.empty()) {
            std::cout << "no interface found" << std::endl;
            return;
        }

        mc_socket ms;
        addr_storage gaddr;
        {
            std::string tmp_gaddr = m_config_map.get(to_do, "group");
            if(tmp_gaddr.empty()){
                std::cout << "no group found" << std::endl; 
                return;
            }
            gaddr = tmp_gaddr;
            if(!gaddr.is_valid()){
                std::cout << "group " << tmp_gaddr << " is not an ipaddress" << std::endl; 
                return;
            }
        }
        
        int sleep_time;
        int& count = sleep_time;

        std::vector<addr_storage> s(9);
        {
        
        
        
        }

        if (arg_count - 1 == TIME) {
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

            std::cout << "set ttl to 10" << std::endl;
            if (!ms.set_ttl(10)) {
                std::cout << "ttl set error" << std::endl;
            }

            std::cout << "send packet: \"bob is cool\" to port 1234 " << std::endl;
            for (int i = 0; i < count; i++) {
                if (!ms.send_packet(gaddr.set_port(1234), "bob is cool")) {
                    std::cout << "send packet error" << std::endl;
                }
            }
            return;
        }
    } else {
        if (m_config_map.size() > 0) {
            auto is_last = [this](config_map::const_iterator it) {
                if (++it == m_config_map.end()) {
                    return true;
                } else {
                    return false;
                }
            };

            auto is_first = [this](const config_map::const_iterator & it) {
                if (it == m_config_map.begin()) {
                    return true;
                } else {
                    return false;
                }
            };

            std::cout << "wrong argument!! expect: " << args[0] << " [";
            for (auto it = m_config_map.begin(); it != m_config_map.end(); ++it) {

                if (is_first(it) && is_last(it)) {
                    std::cout << it->first;
                } else if (is_first(it)) {
                    std::cout << it->first << " | ";
                } else if (is_last(it)) {
                    std::cout << it->first;
                } else {
                    std::cout << it->first << " ";
                }
            }

            std::cout << "]" << std::endl;
        }
        //std::cout << "wrong argument: " << args[0] << " ";
        //for(m_)
    }
}


