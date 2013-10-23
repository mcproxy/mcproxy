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


void simple_routing_data::add_source(unsigned int if_index, const addr_storage& gaddr, const source& saddr)
{
    HC_LOG_TRACE("");
    auto data_it = m_data.find(if_index);
    if (data_it != std::end(m_data)) {
        auto gaddr_it = data_it->second.find(gaddr);
        if (gaddr_it != std::end(data_it->second)) {
            gaddr_it->second.insert(saddr);
        } else {
            data_it->second.insert(s_group_data_pair(gaddr, {saddr} ));
        }
    } else {
        m_data.insert(s_routing_data_pair(if_index, {s_group_data_pair(gaddr, {saddr} )} ));
    }

}

void simple_routing_data::del_source(unsigned int if_index, const addr_storage& gaddr, const source& saddr)
{
    HC_LOG_TRACE("");
    auto data_it = m_data.find(if_index);
    if (data_it != std::end(m_data)) {
        auto gaddr_it = data_it->second.find(gaddr);
        if (gaddr_it != std::end(data_it->second)) {
            gaddr_it->second.erase(saddr);
            if (gaddr_it->second.empty()) {
                data_it->second.erase(gaddr_it);
            }
        }
        if (data_it->second.empty()) {
            m_data.erase(data_it);
        }
    }

}

source_list<source> simple_routing_data::get_available_sources(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");
    source_list<source> rt;

    auto data_it = m_data.find(if_index);
    if (data_it != std::end(m_data)) {
        auto gaddr_it = data_it->second.find(gaddr);
        if (gaddr_it != std::end(data_it->second)) {
            rt = slist + gaddr_it->second;
        }
    }

    return rt;
}

std::string simple_routing_data::to_string() const
{
    using  namespace std;
    HC_LOG_TRACE("");
    ostringstream s;
    s << "##-- simple multicast routing information base --##";
    for (auto &  a : m_data) {
        s << endl << "-- " << interfaces::get_if_name(a.first) << " --";

        for (auto &  b : a.second) {
            s << endl << "\tgroup: " << b.first; 
            s << endl << "\t\tsources: " << b.second;
        }
    }

    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const simple_routing_data& rm)
{
    return stream << rm.to_string();
}

void simple_routing_data::test_simple_routing_data()
{
    using namespace std;
    simple_routing_data srd;
    cout << srd << endl;
    srd.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    srd.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    srd.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.3"));
    srd.add_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    srd.add_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.2"));
    srd.add_source(1, addr_storage("10.1.1.3"), addr_storage("1.1.1.1"));
    srd.add_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    srd.add_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    cout << srd << endl;
    srd.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    srd.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1"));
    srd.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1"));
    srd.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2"));
    cout << srd << endl;
}
