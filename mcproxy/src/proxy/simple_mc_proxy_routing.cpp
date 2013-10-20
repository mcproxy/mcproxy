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
#include "include/proxy/simple_mc_proxy_routing.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/utils/addr_storage.hpp"

simple_mc_proxy_routing::simple_mc_proxy_routing(const proxy_instance* p)
    : routing_management(p)
{
    HC_LOG_TRACE("");

}

void simple_mc_proxy_routing::add_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto data_it = m_data.find(if_index);
    if (data_it != std::end(m_data)) {
        add_source(data_it, gaddr, saddr);
    } else {
        m_data.insert(routing_data_pair(if_index, { group_data_pair(gaddr, {saddr}) } ));
    }

}

void simple_mc_proxy_routing::add_source(routing_data::iterator data_it, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto gaddr_it = data_it->second.find(gaddr);
    if (gaddr_it != std::end(data_it->second)) {
        add_source(gaddr_it, saddr);
    } else {
        data_it->second.insert(group_data_pair(gaddr, {saddr}));
    }
}

void simple_mc_proxy_routing::add_source(group_data::iterator gaddr_it, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    gaddr_it->second.insert(saddr);
}


void simple_mc_proxy_routing::del_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto data_it = m_data.find(if_index);
    if (data_it != std::end(m_data)) {
        del_source(data_it, gaddr, saddr);
        if (data_it->second.empty()) {
            m_data.erase(data_it);
        }
    }

}

void simple_mc_proxy_routing::del_source(routing_data::iterator data_it, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto gaddr_it = data_it->second.find(gaddr);
    if (gaddr_it != std::end(data_it->second)) {
        del_source(gaddr_it, saddr);
        if (gaddr_it->second.empty()) {
            data_it->second.erase(gaddr_it);
        }
    }
}

void simple_mc_proxy_routing::del_source(group_data::iterator gaddr_it, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    gaddr_it->second.erase(saddr);
}

void simple_mc_proxy_routing::event_new_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");

}

void simple_mc_proxy_routing::event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");


}

void simple_mc_proxy_routing::timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");



}

std::string simple_mc_proxy_routing::to_string() const
{
    using  namespace std;
    HC_LOG_TRACE("");
    ostringstream s;
    s << "##-- simple multicast routing information base --##" << endl;;
    for (auto a : m_data) {
        s << "-- " << interfaces::get_if_name(a.first) << " --" << endl;

        for (auto b : a.second) {
            s << "\tgroup: " << b.first << endl;
            s << "\t\tsources (#" << a.second.size() << "): ";

            int i = 1;
            for (auto & c : b.second) {
                ++i;
                if (i % 4 == 0) {
                    s << endl<< "\t";
                }
                s << c.to_string() << " ";
            }
            s << endl;
        }
    }

    return s.str();
}

void simple_mc_proxy_routing::test_simple_mc_proxy_routing()
{
    using namespace std;
    simple_mc_proxy_routing sr(nullptr);
    cout << sr << endl; 
    sr.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.1")); 
    sr.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.2")); 
    sr.add_source(1, addr_storage("10.1.1.1"), addr_storage("1.1.1.3")); 
    sr.add_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1")); 
    sr.add_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.2")); 
    sr.add_source(1, addr_storage("10.1.1.3"), addr_storage("1.1.1.1")); 
    sr.add_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1")); 
    sr.add_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2")); 
    cout << sr << endl; 
    sr.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1")); 
    sr.del_source(1, addr_storage("10.1.1.2"), addr_storage("1.1.1.1")); 
    sr.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.1")); 
    sr.del_source(0, addr_storage("10.1.1.1"), addr_storage("1.1.1.2")); 
    cout << sr << endl; 
}
