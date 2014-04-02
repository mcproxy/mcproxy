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
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */

#ifndef TESTER_HPP
#define TESTER_HPP

#include "include/tester/config_map.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/proxy/def.hpp"
#include "include/utils/mc_socket.hpp"

#include <memory>
#include <list>
#include <chrono>

#define TESTER_DEFAULT_CONIG_PATH "tester.ini"

class packet_manager
{
private:
    unsigned int m_max_count;
    std::set<long> m_data;
public:
    packet_manager(unsigned int max_count);     
    bool is_packet_new(long packet_number);
};

class tester
{
private:
    config_map m_config_map;
    static bool m_running;
    void help();
    void run(const std::string& to_do, const std::string& output_file, unsigned int current_packet_number, packet_manager& pmanager, const std::string& send_msg);

    addr_storage get_gaddr(const std::string& to_do);
    std::unique_ptr<const mc_socket> get_mc_socket(int addr_family);
    std::string get_if_name(const std::string& to_do);
    std::list<addr_storage> get_src_list(const std::string& to_do, int addr_family);
    mc_filter get_mc_filter(const std::string& to_do);
    unsigned long get_max_count(const std::string& to_do);
    std::string get_action(const std::string& to_do);
    std::string source_list_to_string(const std::list<addr_storage>& slist);
    std::string get_msg(const std::string& to_do, const std::string& proposal);
    std::chrono::milliseconds get_send_interval(const std::string& to_do);
    std::chrono::milliseconds get_lifetime(const std::string& to_do);

    int get_int(const std::string& to_do, std::string&& compare, int default_return);
    bool get_boolean(const std::string& to_do, std::string&& compare, bool default_return);    
    std::string get_file_name(const std::string& to_do, const std::string& proposal);
    std::string get_file_operation_mode(const std::string& to_do);
    std::string get_to_do_next(const std::string& to_do);

    void send_data(const std::unique_ptr<const mc_socket>& ms, addr_storage& gaddr, int port, int ttl, unsigned long max_count, unsigned int& current_packet_number, bool include_time_stamp, const std::chrono::milliseconds& interval, int busy_waiting_counter,  const std::string& msg, bool print_status_msg);
    void receive_data(const std::unique_ptr<const mc_socket>& ms, int port, unsigned long max_count, bool parse_time_stamp, bool print_status_msg, bool save_to_file, const std::string& file_name, bool include_file_header, bool include_data, bool include_summary, bool ignore_duplicated_packets, packet_manager& pmanager, const std::string& file_operation_mode);
    static void signal_handler(int sig);

public:
    tester(int arg_count, char* args[]);
};

#endif // TESTER_HPP
