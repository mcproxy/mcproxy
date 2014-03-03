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

#include <cstring>
#include <thread>
#include <csignal>
#include <vector>
#include <limits>
#include <thread>
#include <fstream>
#include <iostream>

#include <unistd.h> //for getopt

bool tester::m_running = true;

tester::tester(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    signal(SIGINT, tester::signal_handler);
    signal(SIGTERM, tester::signal_handler);

    hc_set_default_log_fun(HC_LOG_TRACE_LVL);

    std::string config_file;
    std::string output_file;
    std::string to_do;

    int c;
    optind = 2;
    while ( (c = getopt(arg_count, args, "i:o:")) != -1) {
        switch (c) {
        case 'i':
            config_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        default:
            std::cout << "Unknown argument" << std::endl;
            exit(0);
        }
    }

    if (optind < arg_count) {
        std::cout << "Unknown option argument: " << args[optind] << std::endl;
        exit(0);
    }

    if (arg_count >= 1) {
        to_do =  args[1];
        if (to_do.compare("-h") == 0 || to_do.compare("--help") == 0 || to_do.compare("help") == 0) {
            help();
            exit(0) ;
        }
    } else {
        std::cout << "to do not defined" << std::endl;
        exit(0);
    }

    std::cout << "to do: " << to_do << std::endl;
    std::cout << "config file: " << config_file << std::endl;
    std::cout << "output file: " << output_file << std::endl;

    if (config_file.empty()) {
        m_config_map.read_ini(TESTER_DEFAULT_CONIG_PATH);
    } else {
        m_config_map.read_ini(config_file);
    }

    run(to_do, output_file);
}

void tester::help()
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "Multicast tester" << endl;

    cout << "Project page: http://mcproxy.realmv6.org/" << endl;
    cout << endl;
    cout << "Usage:" << endl;
    cout << "  tester [-h]" << endl;
    cout << "  tester [to do] [-i <config file>] [-o <output file]" << endl;
    cout << endl;
    cout << "\t-h" << endl;
    cout << "\t\tDisplay this help screen." << endl;

    cout << "\tto do" << endl;
    cout << "\t\tThe name of *to do* is defined in the INI-file" << endl;

    cout << "\t-i" << endl;
    cout << "\t\tDefine an configuration file (default " << TESTER_DEFAULT_CONIG_PATH << ")" << endl;

    cout << "\t-o" << endl;
    cout << "\t\tSet the log file name of the receiver (higher priority)" << endl;

    cout << endl;
    cout << "\tfor example:" << endl;
    cout << "\t\t./tester send" << endl;
    cout << "\t\t./tester recv -i tester.ini" << endl;
    cout << "\t\t./tester send_a_hallo -i tester.ini -o logfile" << endl;
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

unsigned long tester::get_max_count(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_max_count = m_config_map.get(to_do, "max_count");
    unsigned long max_count;
    if (str_max_count.empty()) {
        max_count = 0; //infinity
    } else {
        try {
            max_count = std::stol(str_max_count);
        } catch (std::logic_error e) {
            std::cout << "failed to parse max_count" << std::endl;
            exit(0);
        }
    }
    return max_count;
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

std::chrono::milliseconds tester::get_lifetime(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string str_lifetime = m_config_map.get(to_do, "lifetime");
    int lifetime;
    if (str_lifetime.empty()) {
        lifetime = 0; //endless
    } else {
        try {
            lifetime = std::stoi(str_lifetime);
        } catch (std::logic_error e) {
            std::cout << "failed to parse lifetime" << std::endl;
            exit(0);
        }
    }
    return std::chrono::milliseconds(lifetime);
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

bool tester::get_include_file_header(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, "include_file_header");
    if (result.empty()) {
        return true;
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

std::string tester::get_file_name(const std::string& to_do, const std::string& proposal)
{
    HC_LOG_TRACE("");

    if (!proposal.empty()) {
        return proposal;
    } else {
        std::string result = m_config_map.get(to_do, "file_name");
        if (result.empty()) {
            return std::string("delay_measurment_file");
        }

        return result;
    }
}

std::string tester::get_file_operation_mode(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, "file_operation_mode");
    if (result.empty()) {
        return std::string("override");
    }

    if (result.compare("override") != 0 && result.compare("append") != 0) {
        std::cout << "failed to file_operation_mode" << std::endl;
        exit(0);
    }

    return result;
}

std::string tester::get_to_do_next(const std::string& to_do)
{
    HC_LOG_TRACE("");

    std::string to_do_next = m_config_map.get(to_do, "to_do_next");
    if (to_do_next.empty()) {
        to_do_next = "null";
    }

    if (to_do_next.compare("null") != 0 && !m_config_map.has_group(to_do_next)) {
        std::cout << "to_do_next " << to_do_next << " not found" << std::endl;
        exit(0);
    }

    return to_do_next;
}

void tester::receive_data(const std::unique_ptr<const mc_socket>& ms, int port, unsigned long max_count, bool print_status_msg, bool save_to_file, const std::string& file_name, bool include_file_header, const std::string& file_operation_mode)
{
    HC_LOG_TRACE("");

    const unsigned int size = 201;
    std::vector<char> buf(size, 0);

    int info_size = 0;
    std::ofstream file;
    if (save_to_file) {
        if (file_operation_mode.compare("override") == 0) {
            file.open(file_name, std::ios::trunc);
        } else if (file_operation_mode.compare("append") == 0) {
            file.open(file_name, std::ios::app);
        }

        if (!file.is_open()) {
            std::cout << "failed to open file" << std::endl;
            exit(0);
        } else {
            if (include_file_header) {
                file << "packet_number(#) time_stamp_sender(ms) time_stamp_receiver(ms) delay(ms) message(str)" << std::endl;
            }
        }
    }

    if (!ms->set_reuse_port()) {
        std::cout << "failed to set socket option reuse port" << std::endl;
        exit(0);
    }

    if (!ms->bind_udp_socket(port)) {
        std::cout << "failed to bind port " << port << " to socket" << std::endl;
        exit(0);
    }

    if (!ms->set_receive_timeout(100)) {
        std::cout << "failed to set receive timeout" << std::endl;
        exit(0);
    }

    if (print_status_msg) {
        std::cout << "packet number: 0; last delay: 0ms; lost packets: 0; last msg: ";
        std::flush(std::cout);
    }

    long lost_packets = 0;
    long old_packet_number = 0;
    unsigned long count = 0;

    while (m_running && (max_count == 0 || count < max_count)) {
        if (!ms->receive_packet(reinterpret_cast<unsigned char*>(buf.data()), size - 1, info_size)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            //this could be an error but it could be also an interrupt (SIGINIT)
        }

        if (info_size != 0 && m_running) { //no timeout
            std::istringstream iss(std::string(buf.data()));
            std::ostringstream oss;

            long long send_time_stamp = 0;
            long long receive_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            long long delay = 0;
            long packet_number = 0;

            iss >> packet_number >> send_time_stamp;
            oss << iss.rdbuf();
            delay = receive_time_stamp - send_time_stamp;

            if (print_status_msg) {

                // unsorted packets counts as packet lost
                // and duplicated packets are ignored
                if (packet_number > old_packet_number + 1) {
                    lost_packets += packet_number - old_packet_number - 1;
                }
                old_packet_number = packet_number;

                std::cout << "\rpacket number: " << packet_number << "; last delay: " << delay <<  "ms; lost packets: " << lost_packets << "; last msg:" << oss.str();
                std::flush(std::cout);
            }

            if (save_to_file) {
                file <<  packet_number << " " << send_time_stamp << " " << receive_time_stamp << " " << delay << oss.str() << std::endl;
            }

            ++count;
        }
    }

    if (print_status_msg) {
        std::cout << std::endl;
    }

    if (save_to_file) {
        file.close();
    }
}

void tester::send_data(const std::unique_ptr<const mc_socket>& ms, addr_storage& gaddr, int port, int ttl, unsigned long max_count, const std::chrono::milliseconds& interval, const std::string& msg, bool print_status_msg)
{
    HC_LOG_TRACE("");

    std::cout << "set ttl to " << ttl << std::endl;
    if (!ms->set_ttl(ttl)) {
        std::cout << "failed to set ttl" << std::endl;
        exit(0);
    }

    std::cout << "send message: " << msg << " to port " << port << std::endl;

    for (unsigned long i = 0; (i < max_count || max_count == 0 ) && (m_running) ; ++i) {
        std::ostringstream oss;
        long long send_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        oss << i + 1 << " " << send_time_stamp << " " << msg;

        if (print_status_msg) {
            std::cout << "\rsend: " << i + 1 << "/" << max_count;
            std::flush(std::cout);
        }

        if (!ms->send_packet(gaddr.set_port(port), oss.str() )) {
            std::cout << "failed to send packet" << std::endl;
            exit(0);
        }

        std::this_thread::sleep_for(interval);
    }

    if (print_status_msg) {
        std::cout << std::endl;
    }
}

void tester::run(const std::string& to_do, const std::string& output_file)
{
    HC_LOG_TRACE("to_do: " << to_do);
    if (!m_config_map.has_group(to_do)) {
        std::cout << "to_do " << to_do << " not found" << std::endl;
        exit(0);
    }

    m_running = true;

    std::string if_name = get_if_name(to_do);
    HC_LOG_DEBUG("if_name: " << if_name);
    addr_storage gaddr = get_gaddr(to_do);
    HC_LOG_DEBUG("gaddr: " << gaddr);
    const std::unique_ptr<const mc_socket> ms(get_mc_socket(gaddr.get_addr_family()));
    unsigned long max_count = get_max_count(to_do);
    HC_LOG_DEBUG("max_count: " << max_count);

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

    std::string file_name = get_file_name(to_do, output_file);
    HC_LOG_DEBUG("file_name: " << file_name);

    bool include_file_header = get_include_file_header(to_do);
    HC_LOG_DEBUG("include_file_header: " << include_file_header);

    std::string file_operation_mode = get_file_operation_mode(to_do);
    HC_LOG_DEBUG("file_operation_mode: " << file_operation_mode);

    std::chrono::milliseconds lifetime = get_lifetime(to_do);
    HC_LOG_DEBUG("lifetime: " << lifetime.count() << "milliseconds");

    std::string to_do_next = get_to_do_next(to_do);
    HC_LOG_DEBUG("to_do_next: " << to_do_next);

    if (lifetime.count() > 0) {
        std::thread t([&]() {
            HC_LOG_TRACE("");
            std::this_thread::sleep_for(lifetime);
            raise(SIGINT);
        });
        t.detach();
    }

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

        receive_data(ms, port, max_count, print_status_msg, save_to_file, file_name, include_file_header, file_operation_mode);
        ms->close_socket();
        if (to_do_next.compare("null") != 0) {
            run(to_do_next, output_file);
        }

        return;
    } else if (action.compare("send") == 0) {
        std::cout << "choose multicast interface: " << if_name << std::endl;
        if (!ms->choose_if(interfaces::get_if_index(if_name))) {
            std::cout << "failed to choose interface " << if_name << std::endl;
            exit(0);
        }

        send_data(ms, gaddr, port, ttl, max_count, interval, msg, print_status_msg);
        ms->close_socket();
        if (to_do_next.compare("null") != 0) {
            run(to_do_next, output_file);
        }

        return;
    } else {
        std::cout << "action " << action << " not available" << std::endl;
        exit(0);
    }
}

void tester::signal_handler(int)
{
    tester::m_running = false;
}
