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
#include "include/proxy/simple_routing_data.hpp"
#include "include/proxy/message_format.hpp"
#include "include/utils/mroute_socket.hpp"
#include "include/proxy/interfaces.hpp"

simple_routing_data::simple_routing_data(group_mem_protocol group_mem_protocol, const std::shared_ptr<const mroute_socket>& mrt_sock)
    : m_group_mem_protocol(group_mem_protocol)
    , m_mrt_sock(mrt_sock)
{
    HC_LOG_TRACE("");
}

unsigned long simple_routing_data::get_current_packet_count(const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");

    if (is_IPv4(m_group_mem_protocol)) {
        struct sioc_sg_req tmp_stat;
        if (m_mrt_sock->get_mroute_stats(saddr, gaddr, &tmp_stat, nullptr)) {
            return tmp_stat.pktcnt;
        } else {
            return true;
        }
    } else if (is_IPv6(m_group_mem_protocol)) {
        struct sioc_sg_req6 tmp_stat;
        if (m_mrt_sock->get_mroute_stats(saddr, gaddr, nullptr, &tmp_stat)) {
            return tmp_stat.pktcnt;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("unknown IP version");
        return true;
    }
}

void simple_routing_data::set_source(unsigned int if_index, const addr_storage& gaddr, const source& saddr)
{
    HC_LOG_TRACE("");
    auto gaddr_it = m_data.find(gaddr);
    if (gaddr_it != std::end(m_data)) {
        auto list_result = gaddr_it->second.m_source_list.insert(saddr);
        if (!list_result.second) { //failed to inert
            saddr.retransmission_count = get_current_packet_count(gaddr, saddr.saddr);
            gaddr_it->second.m_source_list.erase(list_result.first);
            gaddr_it->second.m_source_list.insert(saddr);
        }

        auto map_result = gaddr_it->second.m_if_map.insert(std::pair<addr_storage, unsigned int>(saddr.saddr, if_index));
        if (!map_result.second) {
            map_result.first->second = if_index;
            HC_LOG_WARN("data already exists");
        }

    } else {
        m_data.insert(s_routing_data_pair(gaddr, sr_data_value({saddr}, {std::pair<addr_storage, unsigned int>(saddr.saddr, if_index)})));
    }
}

void simple_routing_data::del_source(const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto gaddr_it = m_data.find(gaddr);
    if (gaddr_it != std::end(m_data)) {
        gaddr_it->second.m_source_list.erase(saddr);
        gaddr_it->second.m_if_map.erase(saddr);
        if (gaddr_it->second.m_source_list.empty()) {
            m_data.erase(gaddr_it);
        }
    }

}

std::pair<source_list<source>::iterator, bool> simple_routing_data::refresh_source_or_del_it_if_unused(const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto gaddr_it = m_data.find(gaddr);
    if (gaddr_it != std::end(m_data)) {

        auto saddr_it = gaddr_it->second.m_source_list.find(saddr);
        if (saddr_it != std::end(gaddr_it->second.m_source_list)) {

            auto cnt = get_current_packet_count(gaddr, saddr);
            if (static_cast<unsigned long>(saddr_it->retransmission_count) == cnt) {
                gaddr_it->second.m_source_list.erase(saddr_it);
                gaddr_it->second.m_if_map.erase(saddr);
            } else {
                saddr_it->retransmission_count = cnt;
                return std::pair<source_list<source>::iterator, bool>(saddr_it, true);
            }

        }

        gaddr_it->second.m_source_list.erase(saddr);
        gaddr_it->second.m_if_map.erase(saddr);
        if (gaddr_it->second.m_source_list.empty()) {
            m_data.erase(gaddr_it);
        }
    }

    return std::pair<source_list<source>::iterator, bool>(source_list<source>::iterator(), false);
}

const source_list<source>& simple_routing_data::get_available_sources(const addr_storage& gaddr) const
{
    HC_LOG_TRACE("");
    static source_list<source> rt;

    auto gaddr_it = m_data.find(gaddr);
    if (gaddr_it != std::end(m_data)) {
        rt.clear();
        return gaddr_it->second.m_source_list;       
    }

    return rt;
}

std::string simple_routing_data::to_string() const
{
    using  namespace std;
    HC_LOG_TRACE("");
    ostringstream s;
    s << "##-- simple multicast routing information base --##";

    for (auto &  d : m_data) {
        s << endl << "group: " << d.first;
        s << endl << "\tsources: " << d.second.m_source_list;

        for (auto & m : d.second.m_if_map) {
            s << endl << "\t" << m.first  << " ==> " << interfaces::get_if_name(m.second);
        }
    }

    return s.str();
}

const std::map<addr_storage, unsigned int>& simple_routing_data::get_interface_map(const addr_storage& gaddr) const
{
    HC_LOG_TRACE("");
    auto it = m_data.find(gaddr);
    if(it != std::end(m_data)){
        return it->second.m_if_map; 
    }else{
        static std::map<addr_storage, unsigned int> result;
        result.clear();
        return result; 
    }
}

std::ostream& operator<<(std::ostream& stream, const simple_routing_data& rm)
{
    return stream << rm.to_string();
}

#ifdef DEBUG_MODE
void simple_routing_data::test_simple_routing_data()
{
    using namespace std;
    //simple_routing_data srd;
    //cout << srd << endl;
    //srd.set_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    //srd.set_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    //srd.set_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.3"));
    //srd.set_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    //srd.set_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.2"));
    //srd.set_source(1, addr_storage("10.1.1.3"), addr_storage("1.1.1.1"));
    //srd.set_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    //srd.set_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    //cout << srd << endl;
    //srd.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    //srd.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    //srd.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    //srd.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    //cout << srd << endl;
}
#endif /* DEBUG_MODE */
