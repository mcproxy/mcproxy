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


#include "include/hamcast_logging.h"
#include "include/proxy/sender.hpp"
#include "include/proxy/message_format.hpp" //source
#include "include/proxy/timers_values.hpp"

#include <iostream>
sender::sender(const std::shared_ptr<const interfaces>& interfaces, group_mem_protocol gmp)
    : m_group_mem_protocol(gmp)
    , m_interfaces(interfaces)
{
    HC_LOG_TRACE("");

    if (is_IPv4(m_group_mem_protocol)) {
        if (!m_sock.create_raw_ipv4_socket()) {
            throw "failed to create raw ipv4 socket";
        }
    } else if (is_IPv6(m_group_mem_protocol)) {
        if (!m_sock.create_raw_ipv6_socket()) {
            throw "failed to create raw ipv6 socket";
        }
    } else {
        HC_LOG_ERROR("wrong addr_family: " << get_group_mem_protocol_name(m_group_mem_protocol));
        throw "wrong addr_family";
    }

    if (!m_sock.set_loop_back(false)) {
        throw "failed to set loop back";
    }
}

#ifdef DEBUG_MODE
bool sender::send_record(unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist) const
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "!!--ACTION: send report" << endl;
    cout << "interface: " << interfaces::get_if_name(if_index) << endl;
    cout << "group address: " << gaddr << endl;
    cout << "filter mode: " << get_mc_filter_name(filter_mode) << endl;
    cout << "source list: " << slist << endl;
    cout << endl;
    return true;
}

bool sender::send_general_query(unsigned int if_index, const timers_values& tv) const
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "!!--ACTION: send general query" << endl;
    cout << "interface: " << interfaces::get_if_name(if_index) << endl;
    cout << "max response time: " << time_to_string(tv.get_query_response_interval()) << endl;
    cout << "qrv: " << tv.get_robustness_variable() << endl;
    cout << "qqi: " << time_to_string(tv.get_query_interval()) << endl;
    cout << endl;
    return true;
}

bool sender::send_mc_addr_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag) const
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "!!--ACTION: send multicast address specific query" << endl;
    cout << "interface: " << interfaces::get_if_name(if_index) << endl;
    cout << "max response time: " << time_to_string(tv.get_query_response_interval()) << endl;
    cout << "group address: " << gaddr << endl;
    cout << "s-flag: " << (s_flag ? "true" : "false") << endl;
    cout << "qrv: " << tv.get_robustness_variable() << endl;
    cout << "qqi: " << time_to_string(tv.get_query_interval())  << endl;
    cout << endl;
    return true;
}

bool sender::send_mc_addr_and_src_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, source_list<source>& slist) const
{
    using namespace std;
    HC_LOG_TRACE("");
    source_list<source> list_lower;
    source_list<source> list_higher;
    bool rc = false;
    for (auto & e : slist) {
        if (e.retransmission_count > 0) {
            e.retransmission_count--; //side effect!!!!!!!!!

            if (e.retransmission_count > 0 ) {
                rc = true;
            }

            if (e.shared_source_timer.get() != nullptr) {
                if (e.shared_source_timer->is_remaining_time_greater_than(tv.get_last_listener_query_time())) {
                    list_higher.insert(e);
                } else {
                    list_lower.insert(e);
                }
            } else {
                HC_LOG_ERROR("the shared source timer shouldnt be null");
            }
        }
    }

    if (!list_lower.empty() || !list_higher.empty() ) {
        cout << "!!--ACTION: send one or two multicast address and source specific queries" << endl;
        cout << "interface: " << interfaces::get_if_name(if_index) << endl;
        cout << "max response time: " << time_to_string(tv.get_query_response_interval()) << endl;
        cout << "group address: " << gaddr << endl;
        cout << "qrv: " << tv.get_robustness_variable() << endl;
        cout << "qqi: " << time_to_string(tv.get_query_interval()) << endl;
        cout << "source list with without S-flag: " << list_lower << endl;
        cout << "source list with with S-flag: " << list_higher << endl;
        cout << endl;
    }

    return rc;
}
#else

bool sender::send_record(unsigned int, mc_filter, const addr_storage&, const source_list<source>&) const
{
    return false;
}

bool sender::send_general_query(unsigned int, const timers_values&) const
{
    return false;
}

bool sender::send_mc_addr_specific_query(unsigned int, const timers_values&, const addr_storage&, bool) const
{
    return false;
}

bool sender::send_mc_addr_and_src_specific_query(unsigned int, const timers_values&, const addr_storage&, source_list<source>&) const{
    return false;    
}

#endif /* DEBUG_MODE */

sender::~sender()
{
    HC_LOG_TRACE("");

}
