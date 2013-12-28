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
#include "include/proxy/interfaces.hpp"

#include <unistd.h>

#include <vector>
#include <limits>
#include <thread>
#include <fstream>
#include <iostream>


tester::tester(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    if (arg_count == 2) {
        if (std::string(args[1]).compare("-h") == 0 || std::string(args[1]).compare("--help") == 0) {
            std::cout << "tester <to_do> [<config file>]" << std::endl;
        } else {
            m_config_map.read_ini(TESTER_DEFAULT_CONIG_PATH);
            run(std::string(args[1]));
        }
    } else if (arg_count == 3) {
        m_config_map.read_ini(args[2]);
        run(std::string(args[1]));
    } else {
        run(std::string(std::string()));
    }
}

addr_storage tester::get_gaddr(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string tmp_gaddr = m_config_map.get(to_do, "group");
    if (tmp_gaddr.empty()) {
        std::cout << "no group found" << std::endl;
        exit(0);
    }
    addr_storage gaddr(tmp_gaddr);
    if (!gaddr.is_valid()) {
        std::cout << "group " << tmp_gaddr << " is not an ipaddress" << std::endl;
        exit(0);
    }

    return gaddr;
}

std::unique_ptr<const mc_socket> tester::get_mc_socket(int addr_family)
{
    std::unique_ptr<mc_socket> ms(new mc_socket);
    if (addr_family == AF_INET) {
        ms->create_udp_ipv4_socket();
    } else if (addr_family == AF_INET6) {
        ms->create_udp_ipv6_socket();
    } else {
        std::cout << "unknown address family" << std::endl;
        exit(0);
    }
    return std::move(ms);
}

std::list<addr_storage> tester::get_src_list(const std::string& to_do, int addr_family)
{
    HC_LOG_TRACE("");

    std::list<addr_storage> slist;
    int i = 0;

    while (true) {
        std::ostringstream oss;
        oss << "src_" << i;
        std::string str_saddr = m_config_map.get(to_do, oss.str());
        if (str_saddr.empty()) {
            break; //all sources found
        }
        addr_storage saddr(str_saddr);
        if (addr_family != saddr.get_addr_family()) {
            std::cout << oss.str() << " is not an ip address or has the wrong ip version" << std::endl;
            exit(0);
        }
        slist.push_back(saddr);
        ++i;
    }

    return slist;
}

mc_filter tester::get_mc_filter(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_mf = m_config_map.get(to_do, "filter_mode");
    mc_filter mf;
    if (str_mf.empty()) {
        mf = mc_filter::INCLUDE_MODE;
    } else {
        if (str_mf.compare("include") == 0) {
            mf = mc_filter::INCLUDE_MODE;
        } else if (str_mf.compare("exclude") == 0) {
            mf = mc_filter::EXLCUDE_MODE;
        } else {
            std::cout << str_mf << " is not filter_mode" << std::endl;
            exit(0);
        }
    }
    return mf;
}

int tester::get_count(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_count = m_config_map.get(to_do, "count");
    int count;
    if (str_count.empty()) {
        count = 1;
    } else {
        try {
            count = std::stoi(str_count);
        } catch (std::logic_error e) {
            std::cout << "failed to parse count" << std::endl;
            exit(0);
        }
    }
    return count;
}

std::string tester::get_if_name(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string if_name = m_config_map.get(to_do, "interface");
    if (if_name.empty()) {
        std::cout << "no interface found" << std::endl;
        exit(0);
    }
    if (interfaces::get_if_index(if_name) == 0) {
        std::cout << "interface " << if_name << " not found" << std::endl;
        exit(0);
    }
    return if_name;
}

std::string tester::get_action(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string action = m_config_map.get(to_do, "action");
    if (action.empty()) {
        std::cout << "no action found" << std::endl;
        exit(0);
    }
    return action;
}

std::string tester::source_list_to_string(const std::list<addr_storage>& slist)
{
    HC_LOG_TRACE("");

    std::ostringstream oss;
    for (auto & e : slist) {
        oss << e << " ";
    }
    return oss.str();
}

int tester::get_ttl(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_ttl = m_config_map.get(to_do, "ttl");
    int ttl;
    if (str_ttl.empty()) {
        ttl = 10;
    } else {
        try {
            ttl = std::stoi(str_ttl);
        } catch (std::logic_error e) {
            std::cout << "failed to parse ttl" << std::endl;
            exit(0);
        }
    }
    return ttl;
}

int tester::get_port(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_port = m_config_map.get(to_do, "port");
    int port;
    if (str_port.empty()) {
        port = 1234;
    } else {
        try {
            port = std::stoi(str_port);
        } catch (std::logic_error e) {
            std::cout << "failed to parse port" << std::endl;
            exit(0);
        }
    }
    return port;
}

std::string tester::get_msg(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string msg = m_config_map.get(to_do, "msg");
    if (msg.empty()) {
        msg = "this is a test message";
    }
    return msg;
}

std::chrono::milliseconds tester::get_send_interval(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_interval = m_config_map.get(to_do, "send_interval");
    int interval;
    if (str_interval.empty()) {
        interval = 1000; //milliseconds
    } else {
        try {
            interval = std::stoi(str_interval);
        } catch (std::logic_error e) {
            std::cout << "failed to parse interval" << std::endl;
            exit(0);
        }
    }
    return std::chrono::milliseconds(interval);
}

bool tester::get_print_status_msg(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, "print_status_msg");
    if (result.empty()) {
        return false;
    }

    if (result.compare("true") == 0) {
        return true;
    } else if (result.compare("false") == 0) {
        return false;
    } else {
        std::cout << "failed to parse print_status_msg" << std::endl;
        exit(0);
    }
}

bool tester::get_save_to_file(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, "save_to_file");
    if (result.empty()) {
        return false;
    }

    if (result.compare("true") == 0) {
        return true;
    } else if (result.compare("false") == 0) {
        return false;
    } else {
        std::cout << "failed to parse save_to_file" << std::endl;
        exit(0);
    }
}

std::string tester::get_file_name(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, "save_to_file");
    if (result.empty()) {
        return std::string("delay_measurment_file");
    }

    return result;
}

void tester::receive_data(const std::unique_ptr<const mc_socket>& ms, int port, bool print_status_msg, bool save_to_file, const std::string& file_name)
{
    HC_LOG_TRACE("");

    const unsigned int size = 201;
    std::vector<unsigned char> buf(size, 0);
    int info_size = 0;

    std::ofstream file;
    if (save_to_file) {
        file.open(file_name);
        if (!file.is_open()) {
            std::cout << "failed to open file" << std::endl;
            exit(0);
        } else {
            file << "count\tdelay\tmessage" << std::endl;
        }
    }

    if (!ms->bind_udp_socket(port)) {
        std::cout << "failed to bind port " << port << " to socket" << std::endl;
        exit(0);
    }

    while (true) {
        std::istringstream iss(std::string(reinterpret_cast<char*>(buf.data())));

        long send_time_stamp;
        long delay = 0;
        int count = 0;

        iss >> count >> send_time_stamp;
        delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - send_time_stamp;;

        if (print_status_msg) {
            std::cout << "\rcount: " << count << "; last delay: " << delay << "ms ; last msg: " << iss.rdbuf();
            std::flush(std::cout);
        }

        if (save_to_file) {
            file <<  count << "\t" << delay << "\t" << iss.rdbuf() << std::endl;
        }

        ms->receive_packet(buf.data(), size - 1, info_size);
    }

    if (print_status_msg) {
        std::cout << std::endl;
    }

    if (save_to_file) {
        file.close();
    }
}

void tester::send_data(const std::unique_ptr<const mc_socket>& ms, addr_storage& gaddr, int port, int ttl, int count, const std::chrono::milliseconds& interval, const std::string& msg, bool print_status_msg)
{
    HC_LOG_TRACE("");

    std::cout << "set ttl to " << ttl << std::endl;
    if (!ms->set_ttl(ttl)) {
        std::cout << "failed to set ttl" << std::endl;
        exit(0);
    }

    std::cout << "send message:" << msg << " to port " << port << std::endl;

    for (int i = 0; i < count; ++i) {
        std::ostringstream oss;
        oss << count << " " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() << " " << msg;

        if (print_status_msg) {
            std::cout << "\rsend: " << i + 1 << "/" << count;
            std::flush(std::cout);
        }

        if (!ms->send_packet(gaddr.set_port(port), oss.str() )) {
            std::cout << "failed to send packet" << std::endl;
            exit(0);
        }

        usleep(std::chrono::duration_cast<std::chrono::microseconds>(interval).count());
    }

    if (print_status_msg) {
        std::cout << std::endl;
    }
}

void tester::run(const std::string& to_do)
{
    HC_LOG_TRACE("");
    if (!to_do.empty()) {
        if (!m_config_map.has_group(to_do)) {
            std::cout << "to_do " << to_do << " not found" << std::endl;
            exit(0);
        }

        std::string if_name = get_if_name(to_do);
        HC_LOG_DEBUG("if_name: " << if_name);
        addr_storage gaddr = get_gaddr(to_do);
        HC_LOG_DEBUG("gaddr: " << gaddr);
        const std::unique_ptr<const mc_socket> ms(get_mc_socket(gaddr.get_addr_family()));
        int count = get_count(to_do);
        HC_LOG_DEBUG("count: " << count);

        std::list<addr_storage> slist = get_src_list(to_do, gaddr.get_addr_family());
        HC_LOG_DEBUG("source list size: " << slist.size());

        std::string action = get_action(to_do);
        HC_LOG_DEBUG("action: " << action);

        mc_filter mfilter = get_mc_filter(to_do);
        HC_LOG_DEBUG("filter mode: " << get_mc_filter_name(mfilter));

        int ttl = get_ttl(to_do);
        HC_LOG_DEBUG("ttl: " << ttl);

        int port = get_port(to_do);
        HC_LOG_DEBUG("port: " << port);

        std::string msg = get_msg(to_do);
        HC_LOG_DEBUG("msg: " << msg);

        std::chrono::milliseconds interval = get_send_interval(to_do);
        HC_LOG_DEBUG("interval: " << interval.count() << "milliseconds");

        bool print_status_msg = get_print_status_msg(to_do);
        HC_LOG_DEBUG("print_status_msg: " << print_status_msg);

        bool save_to_file = get_save_to_file(to_do);
        HC_LOG_DEBUG("save_to_file: " << save_to_file);

        std::string file_name = get_file_name(to_do);
        HC_LOG_DEBUG("file_name: " << file_name);

        if (action.compare("receive") == 0) {
            std::cout << "join group " << gaddr << " on interface " << if_name << std::endl;
            if (!ms->join_group(gaddr, interfaces::get_if_index(if_name))) {
                std::cout << "failed to join group " << gaddr << std::endl;
                exit(0);
            }

            if (!slist.empty()) {
                std::cout << "set source filter " << get_mc_filter_name(mfilter) << " with source address: " << source_list_to_string(slist) << std::endl;
                if (!ms->set_source_filter(interfaces::get_if_index(if_name), gaddr, mfilter, slist)) {
                    std::cout << "failed to set source filter" << std::endl;
                    exit(0);
                }
            }

            receive_data(ms, port, print_status_msg, save_to_file, file_name);

            return;
        } else if (action.compare("send") == 0) {
            std::cout << "choose multicast interface: " << if_name << std::endl;
            if (!ms->choose_if(interfaces::get_if_index(if_name))) {
                std::cout << "failed to choose interface " << if_name << std::endl;
                exit(0);
            }

            send_data(ms, gaddr, port, ttl, count, interval, msg, print_status_msg);

            return;
        } else {
            std::cout << "action " << action << " not available" << std::endl;
            exit(0);
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

            std::cout << "wrong argument!! expect: tester (";
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

            std::cout << ") [<path>]" << std::endl;
        } else {
            std::cout << "wrong argument!! expect: tester <to_do> [<config file>]" << std::endl;
        }
    }
}


