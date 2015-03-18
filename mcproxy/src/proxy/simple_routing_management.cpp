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
#include "include/proxy/simple_routing_management.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/simple_membership_aggregation.hpp"

#include <algorithm>
#include <memory>

simple_routing_management::simple_routing_management(const worker* msg_worker, group_mem_protocol group_mem_protocol, const std::shared_ptr<const mroute_socket>& mrt_sock, const std::shared_ptr<const sender>& sender, const std::shared_ptr<const routing>& routing, const std::shared_ptr<timing>& timing, const std::shared_ptr<const interface_infos>& interface_infos, const std::shared_ptr<const interfaces> interfaces)
    : m_msg_worker(msg_worker)
    , m_group_mem_protocol(group_mem_protocol)
    , m_data(std::make_shared<simple_routing_data>(group_mem_protocol, mrt_sock))
    , m_mrt_sock(mrt_sock)
    , m_sender(sender)
    , m_routing(routing)
    , m_timing(timing)
    , m_ii(interface_infos)
    , m_interfaces(interfaces)

{
    HC_LOG_TRACE("");
}

simple_routing_management::~simple_routing_management()
{
    HC_LOG_TRACE("");
}

std::chrono::seconds simple_routing_management::get_source_life_time()
{
    HC_LOG_TRACE("");
    return std::chrono::seconds(20);
}

void simple_routing_management::event_new_source(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    switch (msg->get_type()) {
    case proxy_msg::NEW_SOURCE_MSG: {
        auto sm = std::static_pointer_cast<new_source_msg>(msg);
        source s(sm->get_saddr());
        s.shared_source_timer = set_source_timer(sm->get_if_index(), sm->get_gaddr(), sm->get_saddr());

        //route calculation
        m_data->set_source(sm->get_if_index(), sm->get_gaddr(), s);

        set_routes(sm->get_gaddr(), collect_interested_interfaces(sm->get_gaddr(), {sm->get_saddr()}));


        if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
            process_membership_aggregation(RMT_MUTEX, sm->get_gaddr());
        }

    }
    break;
    default:
        HC_LOG_ERROR("unknown message format");
        return;
    }
}

void simple_routing_management::event_querier_state_change(unsigned int /*if_index*/, const addr_storage& gaddr)
{
    HC_LOG_TRACE("");

    //route calculation
    set_routes(gaddr, collect_interested_interfaces(gaddr, m_data->get_available_sources(gaddr)));

    //membership agregation
    if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_FIRST)) {
        process_membership_aggregation(RMT_FIRST, gaddr);
    } else if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
        process_membership_aggregation(RMT_MUTEX, gaddr);
    } else {
        HC_LOG_ERROR("unkown rule matching type in this context");
    }
}

void simple_routing_management::timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    std::shared_ptr<new_source_timer_msg> tm;

    if (!msg.unique()) {
        switch (msg->get_type()) {
        case proxy_msg::NEW_SOURCE_TIMER_MSG: {
            tm = std::static_pointer_cast<new_source_timer_msg>(msg);

            auto& cmp_source_lst = m_data->get_available_sources(tm->get_gaddr());
            auto cmp_source_it = cmp_source_lst.find(tm->get_saddr());
            if (cmp_source_it != cmp_source_lst.end()) {
                if (tm.get() == cmp_source_it->shared_source_timer.get()) {
                    auto saddr_it = m_data->refresh_source_or_del_it_if_unused(tm->get_gaddr(), tm->get_saddr());
                    if (!saddr_it.second) {

                        del_route(tm->get_if_index(), tm->get_gaddr(), tm->get_saddr());

                        if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
                            process_membership_aggregation(RMT_MUTEX, tm->get_gaddr());
                        }
                    } else {
                        saddr_it.first->shared_source_timer = set_source_timer(tm->get_if_index(), tm->get_gaddr(), tm->get_saddr());
                    }

                } else {
                    HC_LOG_DEBUG("filter_timer is outdate");
                }
            } else {
                //debug message ???????????
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

bool simple_routing_management::is_rule_matching_type(rb_interface_type interface_type, rb_interface_direction interface_direction, rb_rule_matching_type rule_matching_type) const
{
    HC_LOG_TRACE("");

    if (interface_type == IT_UPSTREAM) {
        if (interface_direction == ID_IN) {
            if (m_ii->m_upstream_input_rule != nullptr) {
                return m_ii->m_upstream_input_rule->get_rule_matching_type() == rule_matching_type;
            } else {
                HC_LOG_ERROR("upstream input rule is null");
                return false;
            }
        } else if (interface_direction == ID_OUT) {
            if (m_ii->m_upstream_output_rule != nullptr) {
                return m_ii->m_upstream_output_rule->get_rule_matching_type() == rule_matching_type;
            } else {
                HC_LOG_ERROR("upstream output rule is null");
                return false;
            }
        } else {
            HC_LOG_ERROR("interface direction not supported");
            return false;
        }
    } else {
        HC_LOG_ERROR("interface type not supported");
        return false;
    }
}

std::list<std::pair<source, std::list<unsigned int>>> simple_routing_management::collect_interested_interfaces(const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    //            <source_addr, if_index>
    const std::map<addr_storage, unsigned int>& input_if_index_map = m_data->get_interface_map(gaddr);

    //add upstream interfaces
    std::list<std::pair<source, std::list<unsigned int>>> rt_list;
    for (auto & s : slist) {

        auto input_if_it = input_if_index_map.find(s.saddr);
        if (input_if_it == input_if_index_map.end()) {
            HC_LOG_ERROR("input interface of multicast source " << s.saddr << " not found");
            continue;
        }

        if (m_ii->is_downstream(input_if_it->second)) {
            std::list<unsigned int> up_if_list;
            for (auto & ui : m_ii->m_upstreams) {
                if (check_interface(IT_UPSTREAM, ID_OUT, ui.m_if_index, input_if_it->second, gaddr, s.saddr)) {

                    if (is_rule_matching_type(IT_UPSTREAM, ID_OUT, RMT_ALL)) {
                        up_if_list.push_back(ui.m_if_index);
                    } else if (is_rule_matching_type(IT_UPSTREAM, ID_OUT, RMT_FIRST)) {
                        up_if_list.push_back(ui.m_if_index);
                        break;
                    } else {
                        HC_LOG_ERROR("unknown rule matching type");
                    }

                }
            }
            rt_list.push_back(std::pair<source, std::list<unsigned int>>(s, up_if_list));

        } else { //data from an upstream are not forwarded to an other upstream interface
            rt_list.push_back(std::pair<source, std::list<unsigned int>>(s, {}));
        }
    }

    //add downstream interfaces
    std::function<bool(unsigned int, const addr_storage&)> filter_fun = [&](unsigned int output_if_index, const addr_storage & saddr) {
        auto input_if_it = input_if_index_map.find(saddr);
        if (input_if_it == input_if_index_map.end()) {
            HC_LOG_ERROR("input interface of multicast source " << saddr << " not found");
            return false;
        }

        if (output_if_index != input_if_it->second) {
            return check_interface(IT_DOWNSTREAM, ID_OUT, output_if_index, input_if_it->second, gaddr, saddr);
        } else {
            return false;
        }
    };

    for (auto & dif : m_ii->m_downstreams) {
        dif.second.m_querier->suggest_to_forward_traffic(gaddr, rt_list, std::bind(filter_fun, dif.first, std::placeholders::_1));
    }

    return rt_list;
}

void simple_routing_management::process_membership_aggregation(rb_rule_matching_type rule_matching_type, const addr_storage& gaddr)
{
    HC_LOG_TRACE("");

    if (rule_matching_type == RMT_FIRST || rule_matching_type == RMT_MUTEX) {
        simple_membership_aggregation im(rule_matching_type , gaddr, m_data, m_group_mem_protocol, m_ii);
        for (auto & e : m_ii->m_upstreams) {
            send_record(e.m_if_index, gaddr, im.get_group_memberships(e.m_if_index));
        }
    } else {
        HC_LOG_ERROR("unkown rule matching type in this context");
    }
}

void simple_routing_management::set_routes(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const
{
    HC_LOG_TRACE("");

    //            <source_addr, if_index>
    const std::map<addr_storage, unsigned int>& input_if_index_map = m_data->get_interface_map(gaddr);
    unsigned int input_if_index;

    //output_if_index is a bad name ????????
    for (auto & e : output_if_index) {
        if (e.second.empty()) {

            auto input_if_it = input_if_index_map.find(e.first.saddr);
            if (input_if_it != std::end(input_if_index_map)) {
                input_if_index = input_if_it->second;
            } else {
                HC_LOG_ERROR("failed to find input interface of  (" << gaddr << ", " << e.first.saddr << ")");
                continue;
            }

            del_route(input_if_index, gaddr, e.first.saddr);
        } else {
            std::list<int> vif_out;

            for (auto outif : e.second) {
                vif_out.push_back(m_interfaces->get_virtual_if_index(outif));
                //missing error handling if outif is not existing ??????????
            }

            auto input_if_it = input_if_index_map.find(e.first.saddr);
            if (input_if_it != std::end(input_if_index_map)) {
                input_if_index = input_if_it->second;

                bool use_this_interface = false;
                if (m_ii->is_upstream(input_if_index)) {
                    if (check_interface(IT_UPSTREAM, ID_IN, input_if_index, input_if_index, gaddr, e.first.saddr)) {
                        use_this_interface = true;
                    }
                }

                if (!use_this_interface && m_ii->is_downstream(input_if_index)) {
                    if (check_interface(IT_DOWNSTREAM, ID_IN, input_if_index, input_if_index, gaddr, e.first.saddr)) {
                        use_this_interface = true;
                    }
                }

                if (!use_this_interface) {
                    continue;
                }

            } else {
                HC_LOG_ERROR("failed to find input interface of  (" << gaddr << ", " << e.first.saddr << ")");
                continue;
            }

            m_routing->add_route(m_interfaces->get_virtual_if_index(input_if_index), gaddr, e.first.saddr, vif_out);
        }

    }
}

void simple_routing_management::send_record(unsigned int upstream_if_index, const addr_storage& gaddr, std::pair<mc_filter, const source_list<source>&> sstate) const
{
    HC_LOG_TRACE("");
    m_sender->send_record(upstream_if_index, sstate.first, gaddr, sstate.second);
}

void simple_routing_management::del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const
{
    HC_LOG_TRACE("");
    m_routing->del_route(m_interfaces->get_virtual_if_index(if_index), gaddr, saddr);
}

std::shared_ptr<new_source_timer_msg> simple_routing_management::set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    std::chrono::milliseconds source_life_time;
    if (m_ii->is_upstream(if_index) && is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
        source_life_time = m_ii->m_upstream_input_rule->get_timeout();
    } else {
        source_life_time = get_source_life_time();
    }

    auto nst = std::make_shared<new_source_timer_msg>(if_index, gaddr, saddr, source_life_time);
    m_timing->add_time(get_source_life_time(), m_msg_worker, nst);

    return nst;
}

bool simple_routing_management::check_interface(rb_interface_type interface_type, rb_interface_direction interface_direction, unsigned int checking_if_index, unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr) const
{
    HC_LOG_TRACE("");

    std::shared_ptr<interface> interf;
    if (interface_type == IT_UPSTREAM) {
        auto uinfo_it = std::find_if(m_ii->m_upstreams.begin(), m_ii->m_upstreams.end(), [&](const upstream_infos & ui) {
            return ui.m_if_index == checking_if_index;
        });

        if (uinfo_it != m_ii->m_upstreams.end()) {
            interf = uinfo_it->m_interface;
        } else {
            HC_LOG_ERROR("upstream interface " << interfaces::get_if_name(checking_if_index) << " not found");
            return false;
        }
    } else if (interface_type == IT_DOWNSTREAM) {
        auto dinfo_it = m_ii->m_downstreams.find(checking_if_index);
        if (dinfo_it != m_ii->m_downstreams.end()) {
            interf = dinfo_it->second.m_interface;
        } else {
            HC_LOG_ERROR("downstream interface " << interfaces::get_if_name(checking_if_index) << " not found");
            return false;
        }
    } else {
        HC_LOG_ERROR("unkown interface type");
        return false;
    }

    if (interf == nullptr) {
        HC_LOG_ERROR("interface rule_binding of interface " << interfaces::get_if_name(checking_if_index) << " not found");
        return false;
    }

    std::string input_if_name = interfaces::get_if_name(input_if_index);
    if (!input_if_name.empty()) {
        return interf->is_source_allowed(interface_direction, input_if_name, gaddr, saddr);
    } else {
        HC_LOG_ERROR("failed to map interface index " << input_if_index << " to interface name");
        return false;
    }
}

std::string simple_routing_management::to_string() const
{
    HC_LOG_TRACE("");
    return m_data->to_string();
}

