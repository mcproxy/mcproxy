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
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/sender.hpp"

simple_mc_proxy_routing::simple_mc_proxy_routing(const proxy_instance* p)
    : routing_management(p)
{
    HC_LOG_TRACE("");

}

void simple_mc_proxy_routing::event_new_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    add_proxy_route(if_index, gaddr, saddr, collect_interested_interfaces(if_index, gaddr, saddr));
}

void simple_mc_proxy_routing::event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    mc_filter filter_mode = INCLUDE_MODE;
    source_list<source> slist;
    add_proxy_route(if_index, gaddr, saddr, collect_interested_interfaces(if_index, gaddr, saddr, &filter_mode, &slist));
    send_record(if_index, gaddr, saddr, filter_mode, slist);
}

bool simple_mc_proxy_routing::is_upstream(unsigned int if_index)
{
    HC_LOG_TRACE("");

    if (m_p->m_upstream == if_index) {
        return true;
    } else {
        return false;
    }
}

std::list<unsigned int> simple_mc_proxy_routing::collect_interested_interfaces(unsigned int receiver_if, const addr_storage& gaddr, const addr_storage& saddr, mc_filter* filter_mode, source_list<source>* slist )
{
    HC_LOG_TRACE("");
    std::list<unsigned int> rt;

    if (!is_upstream(receiver_if)) {
        rt.push_back(receiver_if);
    }

    for (auto & e : m_p->m_querier) {
        if (e.first != receiver_if) {
            if (e.second->suggest_to_forward_traffic(gaddr, saddr, filter_mode, slist)) {
                rt.push_back(receiver_if);
            }
        }
    }

    return rt;
}

void simple_mc_proxy_routing::add_proxy_route(unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr, const std::list<unsigned int>& output_if_index) const
{
    HC_LOG_TRACE("");

    if (output_if_index.empty()) {
        m_p->m_routing->del_route(m_p->m_interfaces->get_virtual_if_index(input_if_index), gaddr, saddr);
    } else {
        std::list<int> vif_out;
        for (auto e : output_if_index) {
            vif_out.push_back(m_p->m_interfaces->get_virtual_if_index(e));
        }

        m_p->m_routing->add_route(m_p->m_interfaces->get_virtual_if_index(input_if_index), gaddr, saddr, vif_out);
    }
}

void simple_mc_proxy_routing::send_record(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr, mc_filter filter_mode, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    m_p->m_sender->send_report(if_index, filter_mode, gaddr, slist);

}
