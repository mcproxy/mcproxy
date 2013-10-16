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

#include <iostream>
sender::sender(int addr_family)
{
    HC_LOG_TRACE("");
    m_addr_family = addr_family;

    if (m_addr_family == AF_INET) {
        if (!m_sock.create_raw_ipv4_socket()) {
            throw "failed to create raw ipv4 socket";
        }
    } else if (m_addr_family == AF_INET6) {
        if (!m_sock.create_raw_ipv6_socket()) {
            throw "failed to create raw ipv6 socket";
        }
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        throw "wrong addr_family";
    }

    if (!m_sock.set_loop_back(false)) {
        throw "failed to set loop back";
    }
}

bool sender::send_general_query(const std::chrono::milliseconds& max_response_time, bool s_flag, unsigned int qrv, const std::chrono::seconds& qqi) const
{
    using namespace std;
    HC_LOG_TRACE("");
    
    cout << "!!--ACTION: send general query" << endl;
    cout << "max response time: " << max_response_time.count() << "msec" << endl; 
    cout << "s-flag: " << s_flag << endl;
    cout << "qrv: " << qrv << endl;
    cout << "qqi: " << qqi.count() << "sec" << endl;
    return true;
}

bool sender::send_mc_addr_specific_query(const std::chrono::milliseconds& max_response_time, const addr_storage& gaddr, bool s_flag, unsigned int qrv, const std::chrono::seconds& qqi) const
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "!!--ACTION: send multicast address specific query" << endl;
    cout << "max response time: " << max_response_time.count() << "msec" << endl; 
    cout << "group address: " << gaddr << endl;
    cout << "s-flag: " << s_flag << endl;
    cout << "qrv: " << qrv << endl;
    cout << "qqi: " << qqi.count() << "sec" << endl;
    return true;
}

bool sender::send_mc_addr_and_src_specific_query(const std::chrono::milliseconds& max_response_time, const addr_storage& gaddr, bool s_flag, unsigned int qrv, const std::chrono::seconds& qqi, const source_list<source>& slist) const
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "!!--ACTION: send multicast address specific query" << endl;
    cout << "max response time: " << max_response_time.count() << "msec" << endl; 
    cout << "group address: " << gaddr << endl;
    cout << "s-flag: " << s_flag << endl;
    cout << "qrv: " << qrv << endl;
    cout << "qqi: " << qqi.count() << "sec" << endl;
    cout << "source list size: " << slist.size() << endl;
    cout << "source list: " << slist << endl; 
    return true;
}

sender::~sender()
{
    HC_LOG_TRACE("");

}
