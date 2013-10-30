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
#include "include/proxy/timing.hpp"

simple_mc_proxy_routing::simple_mc_proxy_routing(const proxy_instance* p)
    : routing_management(p)
    , m_data(p->m_group_mem_protocol, p->m_mrt_sock)
{
    HC_LOG_TRACE("");
}

std::chrono::seconds simple_mc_proxy_routing::get_source_life_time()
{
    HC_LOG_TRACE("");
    return std::chrono::seconds(20);
}

void simple_mc_proxy_routing::event_new_source(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    switch (msg->get_type()) {
    case proxy_msg::NEW_SOURCE_MSG: {
        auto sm = std::static_pointer_cast<new_source_msg>(msg);
        source s(sm->get_saddr());
        s.shared_source_timer = set_source_timer(sm->get_if_index(), sm->get_gaddr(), sm->get_saddr());

        m_data.set_source(sm->get_gaddr(), s);
        add_route(sm->get_if_index(), sm->get_gaddr(), collect_interested_interfaces(sm->get_if_index(), sm->get_gaddr(), {sm->get_saddr()}));
    }
    break;
    default:
        HC_LOG_ERROR("unknown message format");
        return;
    }

}

void simple_mc_proxy_routing::event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist)
{
    HC_LOG_TRACE("");

    //route calculation
    auto available_sources = m_data.get_available_sources(gaddr, slist);
    add_route(if_index, gaddr, collect_interested_interfaces(if_index, gaddr, available_sources));

    //membership agregation
    auto mem_info = collect_group_membership_infos(gaddr);
    send_record(get_upstream(), gaddr, mem_info.first, mem_info.second);
}

void simple_mc_proxy_routing::timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    std::shared_ptr<new_source_timer> tm;

    if (!msg.unique()) {
        switch (msg->get_type()) {
        case proxy_msg::NEW_SOURCE_TIMER_MSG: {
            tm = std::static_pointer_cast<new_source_timer>(msg);
            auto cmp_source_list = m_data.get_available_sources(tm->get_gaddr(), {tm->get_saddr()});
            if (!cmp_source_list.empty()) {
                if (tm.get() == cmp_source_list.begin()->shared_source_timer.get()) {
                    auto saddr_it = m_data.refresh_source_or_del_it_if_unused(tm->get_gaddr(), tm->get_saddr());
                    if (!saddr_it.second) {
                        std::cout << "#########del route (if:" << interfaces::get_if_name(tm->get_if_index()) << " gaddr:" << tm->get_gaddr() << " saddr:" << tm->get_saddr() << std::endl;
                        del_route(tm->get_if_index(), tm->get_gaddr(), tm->get_saddr());
                    } else {
                        saddr_it.first->shared_source_timer = set_source_timer(tm->get_if_index(), tm->get_gaddr(), tm->get_saddr());
                    }

                } else {
                    HC_LOG_DEBUG("filter_timer is outdate");
                }
            }
        }
        break;
        default:
            HC_LOG_ERROR("unknown timer message format");
            return;
        }
    } else {
        HC_LOG_DEBUG("filter_timer is outdate");
        return;
    }

}

bool simple_mc_proxy_routing::is_upstream(unsigned int if_index) const
{
    HC_LOG_TRACE("");

    if (m_p->m_upstream == if_index) {
        return true;
    } else {
        return false;
    }
}

unsigned int simple_mc_proxy_routing::get_upstream() const
{
    HC_LOG_TRACE("");
    return m_p->m_upstream;
}

std::list<std::pair<source, std::list<unsigned int>>> simple_mc_proxy_routing::collect_interested_interfaces(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    std::list<std::pair<source, std::list<unsigned int>>> rt_list;
    for (auto & e : slist) {
        if (is_upstream(if_index)) {
            rt_list.push_back(std::pair<source, std::list<unsigned int>>(e, {}));
        } else {
            rt_list.push_back(std::pair<source, std::list<unsigned int>>(e, {get_upstream()}));
        }
    }

    for (auto & e : m_p->m_querier) {
        if (e.first != if_index) {
            e.second->suggest_to_forward_traffic(gaddr, rt_list);
        }
    }

    return rt_list;
}

std::pair<mc_filter, source_list<source>> simple_mc_proxy_routing::collect_group_membership_infos(const addr_storage& gaddr)
{
    HC_LOG_TRACE("");
    std::pair<mc_filter, source_list<source>> rt_pair;
    rt_pair.first = INCLUDE_MODE;
    rt_pair.second = {};

    for (auto & e : m_p->m_querier) {
        merge_membership_infos(rt_pair, e.second->get_group_mebership_infos(gaddr));
    }

    return rt_pair;
}

void simple_mc_proxy_routing::merge_membership_infos(std::pair<mc_filter, source_list<source>>& merge_to, const std::pair<mc_filter, source_list<source>>& merge_from) const
{
    HC_LOG_TRACE("");

    if (merge_to.first == INCLUDE_MODE) {
        if (merge_from.first == INCLUDE_MODE) {
            merge_to.second += merge_from.second;
        } else if (merge_from.first == EXLCUDE_MODE) {
            merge_to.first = EXLCUDE_MODE;
            merge_to.second = merge_from.second - merge_to.second;
        } else {
            HC_LOG_ERROR("unknown filter mode in parameter merge_from");
        }
    } else if (merge_to.first == EXLCUDE_MODE) {
        if (merge_from.first == INCLUDE_MODE) {
            merge_to.second -= merge_from.second;
        } else if (merge_from.first == EXLCUDE_MODE) {
            merge_to.second *= merge_from.second;
        } else {
            HC_LOG_ERROR("unknown filter mode in parameter merge_from");
        }
    } else {
        HC_LOG_ERROR("unknown filter mode in parameter merge_to");
    }
}

void simple_mc_proxy_routing::add_route(unsigned int input_if_index, const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const
{
    HC_LOG_TRACE("");
    for (auto & e : output_if_index) {
        if (e.second.empty()) {
            std::cout << "#########2del route (if:" << interfaces::get_if_name(input_if_index) << " gaddr:" << gaddr << " saddr:" << e.first.saddr << std::endl;
            del_route(input_if_index, gaddr, e.first.saddr);
        } else {
            std::list<int> vif_out;

            std::cout << "############3add route (iif:" << interfaces::get_if_name(input_if_index) << " gaddr:" << gaddr <<  " saddr:" << e.first.saddr << " oif:";

            for (auto outif : e.second) {
                std::cout << interfaces::get_if_name(outif) << " ";

                vif_out.push_back(m_p->m_interfaces->get_virtual_if_index(outif));
            }

            m_p->m_routing->add_route(m_p->m_interfaces->get_virtual_if_index(input_if_index), gaddr, e.first.saddr, vif_out);
        }
    }
}

void simple_mc_proxy_routing::send_record(unsigned int if_index, const addr_storage& gaddr, mc_filter filter_mode, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");
    m_p->m_sender->send_report(if_index, filter_mode, gaddr, slist);
}

void simple_mc_proxy_routing::del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const
{
    HC_LOG_TRACE("");
    m_p->m_routing->del_route(m_p->m_interfaces->get_virtual_if_index(if_index), gaddr, saddr);
}

std::shared_ptr<new_source_timer>  simple_mc_proxy_routing::set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    auto nst = std::make_shared<new_source_timer>(if_index, gaddr, saddr, get_source_life_time());
    m_p->m_timing->add_time(get_source_life_time(), m_p, nst);

    return nst;
}

std::string simple_mc_proxy_routing::to_string() const
{
    HC_LOG_TRACE("");
    return m_data.to_string();
}

