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
#include<sys/time.h>

bool tester::m_running = true;
volatile int busy_waiting;

unsigned long long int get_clock()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long int)tv.tv_usec + 1000000 * tv.tv_sec;
}

int high_pres_usleep(unsigned long long int delay) //minimum delay about 7usec
{
    //The basic idea is to divide the delay into sleep and busy wait parts.
    unsigned long long int start;
    unsigned long long int end;

    start = get_clock();
    end = start + delay;

    //if it is a very big delay wait until the delay is about 2 seconds
    if (delay > 2000000) {
        int sleep_delay =  static_cast<int>((delay - 2000000) / 1000000);
        sleep(sleep_delay);
        delay = end - get_clock();
    }

    //wait until the delay is about 20 milliseconds
    if (delay > 20000) {
        int sleep_delay =  static_cast<int>(delay - 20000);
        usleep(sleep_delay);
    }

    while (get_clock() < end) {}

    return 0;
}

packet_manager::packet_manager(unsigned int max_count)
    : m_max_count(max_count)
{
    HC_LOG_TRACE("");
}

bool packet_manager::is_packet_new(long packet_number)
{
    HC_LOG_TRACE("");
    auto result = m_data.insert(packet_number);
    if (result.second) {
        if (m_data.size() > m_max_count) {
            m_data.erase(m_data.begin());
        }
        return true;
    } else {
        return false;
    }
}

tester::tester(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    signal(SIGINT, tester::signal_handler);
    signal(SIGTERM, tester::signal_handler);

    hc_set_default_log_fun(HC_LOG_TRACE_LVL);

    std::string config_file;
    std::string output_file;
    std::string to_do;
    std::string msg;

    int c;
    optind = 2;
    while ( (c = getopt(arg_count, args, "i:o:m:")) != -1) {
        switch (c) {
        case 'i':
            config_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'm':
            msg = optarg;
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
    std::cout << "msg: msg" << std::endl;

    if (config_file.empty()) {
        m_config_map.read_ini(TESTER_DEFAULT_CONIG_PATH);
    } else {
        m_config_map.read_ini(config_file);
    }

    packet_manager pmanager(1000);
    run(to_do, output_file, 0, pmanager, msg);
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

    cout << "\t-m" << endl;
    cout << "\t\tSet the message to be send (this doesn't include the" << endl;
    cout << "\t\tpacket number (9 chars) plus one escape char add the end of the string)" << endl;

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
        std::cout << "no group found, in unicast mode: need to select IP version" << std::endl;
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
            mf = mc_filter::EXCLUDE_MODE;
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

std::string tester::get_msg(const std::string& to_do, const std::string& proposal)
{
    HC_LOG_TRACE("");
    if (!proposal.empty()) {
        return proposal;
    } else {
        std::string msg = m_config_map.get(to_do, "msg");
        if (msg.empty()) {
            msg = "this is a test message";
        }
        return msg;
    }
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

int tester::get_int(const std::string& to_do, std::string&& compare, int default_return)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, compare);
    if (result.empty()) {
        return default_return;
    } else {
        try {
            return std::stoi(result);
        } catch (std::logic_error e) {
            std::cout << "failed to parse " << compare << std::endl;
            exit(0);
        }
    }
}

bool tester::get_boolean(const std::string& to_do, std::string&& compare, bool default_return)
{
    HC_LOG_TRACE("");

    std::string result = m_config_map.get(to_do, compare);
    if (result.empty()) {
        return default_return;
    }

    if (result.compare("true") == 0) {
        return true;
    } else if (result.compare("false") == 0) {
        return false;
    } else {
        std::cout << "failed to parse " << compare << std::endl;
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

void tester::receive_data(const std::unique_ptr<const mc_socket>& ms, int port, unsigned long max_count, bool parse_time_stamp, bool print_status_msg, bool save_to_file, const std::string& file_name, bool include_file_header, bool include_data, bool include_summary, bool ignore_duplicated_packets, packet_manager& pmanager, const std::string& file_operation_mode)
{
    HC_LOG_TRACE("");

    const unsigned int size = 1472;
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
            std::cout << "failed to open file: " << file_name << std::endl;
            exit(0);
        } else {
            if (include_file_header) {
                if (parse_time_stamp) {
                    file << "packet_number(#) time_stamp_sender(ms) time_stamp_receiver(ms) delay(ms) message(str)" << std::endl;
                } else {
                    file << "packet_number(#)" << std::endl;
                }
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
        if (parse_time_stamp) {
            std::cout << "packet number: 0; packet count: 0; total data size: 0byte; last delay: 0ms; last msg: ";
            std::flush(std::cout);
        } else {
            std::cout << "packet number: 0; packet count: 0; total data size: 0byte; last msg: ";
            std::flush(std::cout);
        }
    }


    unsigned long packet_count = 0;
    long long data_total_size = 0; /*in byte*/
    long long receive_start_time_stamp = 0;

    long long send_time_stamp = 0;
    long packet_number = 0;
    long long delay = 0;
    long long receive_time_stamp = 0;
    std::string msg;

    while (m_running && (max_count == 0 || packet_count < max_count)) {
        if (!ms->receive_packet(reinterpret_cast<unsigned char*>(buf.data()), size - 1, info_size)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            //this could be an error but it could be also an interrupt (SIGINIT)
        }

        if (info_size != 0 && m_running) { //no timeout
            data_total_size += info_size;

            if (packet_count == 0) {
                receive_start_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            }


            if (parse_time_stamp && (include_data || print_status_msg)) {
                std::ostringstream oss;
                std::istringstream iss(std::string(buf.data()));
                receive_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                iss >> packet_number >> send_time_stamp;
                oss << iss.rdbuf();
                msg = oss.str();
                delay = receive_time_stamp - send_time_stamp;
            }

            if (ignore_duplicated_packets && !pmanager.is_packet_new(packet_number)) {
                continue;
            }

            if (print_status_msg) {
                if (parse_time_stamp) {
                    std::cout << "\rpacket number: " << packet_number << "; packet count: " << packet_count << "; total data size: " << data_total_size << "byte; last delay: " << delay <<  "ms; last msg:" << msg;
                } else {
                    std::cout << "\rpacket count: " << packet_count << "; total data size: " << data_total_size << "byte; last msg:" << msg;
                }
                std::flush(std::cout);
            }

            if (save_to_file && include_data && parse_time_stamp) {
                file <<  packet_number << " " << send_time_stamp << " " << receive_time_stamp << " " << delay << msg << std::endl;
            }

            ++packet_count;
        }
    }

    //calculate summary
    long long receive_duration = 0;
    float goodput = 0;
    int packets_per_sec = 0;
    if (receive_start_time_stamp != 0) {
        receive_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - receive_start_time_stamp;
        goodput = (data_total_size / 1024 / 1024) / (receive_duration / 1000.0);
        packets_per_sec = (packet_count / receive_duration) * 1000.0;
    }

    std::ostringstream oss_summary;
    oss_summary << "--- summary==> packet_count(#): " << packet_count << "; total data size(byte): " << data_total_size << "; receive duration(ms): " << receive_duration << "; packets per sec: " << packets_per_sec << "; goodput(MByte/s): " << goodput << std::endl;

    if (print_status_msg) {
        std::cout << "\r";
    }

    std::cout << oss_summary.str();

    if (save_to_file) {
        if (include_summary) {
            file << oss_summary.str();
        }
        file.close();
    }
}

void tester::send_data(const std::unique_ptr<const mc_socket>& ms, addr_storage& gaddr, int port, int ttl, unsigned long max_count, unsigned int& current_packet_number, bool include_time_stamp, const std::chrono::milliseconds& interval, int busy_waiting_counter,  const std::string& msg, bool print_status_msg)
{
    HC_LOG_TRACE("");

    unsigned int start_packet_number = current_packet_number;
    long long data_total_size = 0; /*in byte*/

    std::cout << "set ttl to " << ttl << std::endl;
    if (!ms->set_ttl(ttl)) {
        std::cout << "failed to set ttl" << std::endl;
        exit(0);
    }

    std::cout << "send message: " << msg << " to port " << port << std::endl;
    std::string send_msg = msg;

    long long receive_start_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    for (unsigned long i = 0; (i < max_count || max_count == 0 ) && (m_running) ; ++i) {
        current_packet_number = start_packet_number + i + 1;

        if (include_time_stamp) {
            std::ostringstream oss;
            oss.width(9);
            long long send_time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            oss << current_packet_number << " " << send_time_stamp << " " << msg;
            send_msg = oss.str();
        }

        if (print_status_msg) {
            std::cout << "\rsend: " << i + 1 << "/" << max_count;
            std::flush(std::cout);
        }

        if (!ms->send_packet(gaddr.set_port(port), send_msg)) {
            std::cout << "failed to send packet" << std::endl;
            exit(0);
        }

        data_total_size += send_msg.size();

        if (interval.count() != 0) {
            std::this_thread::sleep_for(interval);
            //high_pres_usleep(6);
        }

        busy_waiting = 0;
        while (busy_waiting < busy_waiting_counter) {
            ++busy_waiting;
        }

    }

    long long receive_duration = 0;
    float goodput = 0;
    int packets_per_sec = 0;
    if (receive_start_time_stamp != 0) {
        receive_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - receive_start_time_stamp;
        goodput = (data_total_size / 1024 / 1024) / (receive_duration / 1000.0);
        packets_per_sec = ((current_packet_number - start_packet_number) / receive_duration) * 1000.0;
    }

    if (print_status_msg) {
        std::cout << std::endl;
    }

    std::cout << "summary==> packet_count(#): " << current_packet_number - start_packet_number << "; total data size(byte): " << data_total_size << "; receive duration(ms): " << receive_duration << "; packets per sec: " << packets_per_sec << "; goodput(MByte/s): " <<  goodput << std::endl;
}

void tester::run(const std::string& to_do, const std::string& output_file, unsigned int current_packet_number, packet_manager& pmanager, const std::string& send_msg)
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

    int ttl = get_int(to_do, "ttl", 10);
    HC_LOG_DEBUG("ttl: " << ttl);

    int port = get_int(to_do, "port", 1234);
    HC_LOG_DEBUG("port: " << port);

    int busy_waiting_counter = get_int(to_do, "busy_waiting_counter", 0);
    HC_LOG_DEBUG("busy_waiting_counter: " << busy_waiting_counter);

    std::string msg = get_msg(to_do, send_msg);
    HC_LOG_DEBUG("msg: " << msg);

    std::chrono::milliseconds interval = get_send_interval(to_do);
    HC_LOG_DEBUG("interval: " << interval.count() << "milliseconds");

    bool print_status_msg = get_boolean(to_do, "print_status_msg", false);
    HC_LOG_DEBUG("print_status_msg: " << print_status_msg);

    bool save_to_file = get_boolean(to_do, "save_to_file", false);
    HC_LOG_DEBUG("save_to_file: " << save_to_file);

    std::string file_name = get_file_name(to_do, output_file);
    HC_LOG_DEBUG("file_name: " << file_name);

    bool include_file_header = get_boolean(to_do, "include_file_header", true);
    HC_LOG_DEBUG("include_file_header: " << include_file_header);

    bool include_data = get_boolean(to_do, "include_data", true);
    HC_LOG_DEBUG("include_data: " << include_file_header);

    bool include_summary = get_boolean(to_do, "include_summary", true);
    HC_LOG_DEBUG("include_summary: " << include_file_header);

    bool ignore_duplicated_packets = get_boolean(to_do, "ignore_duplicated_packets", false);
    HC_LOG_DEBUG("ignore_duplicated_packets: " << ignore_duplicated_packets);

    bool parse_time_stamp = get_boolean(to_do, "parse_time_stamp", true);
    HC_LOG_DEBUG("parse_time_stamp: " << parse_time_stamp);

    bool include_time_stamp = get_boolean(to_do, "include_time_stamp", true);
    HC_LOG_DEBUG("include_time_stamp: " << include_time_stamp);

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
        if (gaddr.is_multicast_addr()) {
            std::cout << "join group " << gaddr << " on interface " << if_name << std::endl;
            if (!ms->join_group(gaddr, interfaces::get_if_index(if_name))) {
                std::cout << "failed to join group " << gaddr << std::endl;
                exit(0);
            }
        }

        if (gaddr.is_multicast_addr() && !slist.empty()) {
            std::cout << "set source filter " << get_mc_filter_name(mfilter) << " with source address: " << source_list_to_string(slist) << std::endl;
            if (!ms->set_source_filter(interfaces::get_if_index(if_name), gaddr, mfilter, slist)) {
                std::cout << "failed to set source filter" << std::endl;
                exit(0);
            }
        }

        receive_data(ms, port, max_count, parse_time_stamp, print_status_msg, save_to_file, file_name, include_file_header, include_data, include_summary, ignore_duplicated_packets, pmanager, file_operation_mode);
        ms->close_socket();
        if (to_do_next.compare("null") != 0) {
            run(to_do_next, output_file, current_packet_number, pmanager, send_msg);
        }

        return;
    } else if (action.compare("send") == 0) {
        std::cout << "choose multicast interface: " << if_name << std::endl;
        if (!ms->choose_if(interfaces::get_if_index(if_name))) {
            std::cout << "failed to choose interface " << if_name << std::endl;
            exit(0);
        }

        send_data(ms, gaddr, port, ttl, max_count, current_packet_number, include_time_stamp, interval, busy_waiting_counter, msg, print_status_msg);
        ms->close_socket();
        if (to_do_next.compare("null") != 0) {
            run(to_do_next, output_file, current_packet_number, pmanager, send_msg);
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
