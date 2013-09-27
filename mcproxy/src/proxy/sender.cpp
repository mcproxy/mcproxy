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

    if (!m_if_prop.refresh_network_interfaces()) {
        throw "failed to refresh network interfaces"; 
    }

}

sender::~sender(){
    HC_LOG_TRACE("");

}
