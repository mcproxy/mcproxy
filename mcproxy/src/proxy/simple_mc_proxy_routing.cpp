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

#include <algorithm>
#include <memory>

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

source_state::source_state()
    : m_mc_filter(INCLUDE_MODE)
{
    HC_LOG_TRACE("");
}

source_state::source_state(std::pair<mc_filter, source_list<source>> sstate)
    : m_mc_filter(sstate.first)
    , m_source_list(sstate.second)
{
    HC_LOG_TRACE("");
}

std::string source_state::to_string() const
{
    std::ostringstream s;
    s << get_mc_filter_name(m_mc_filter) << "{" << m_source_list << "}";
    return s.str();
}
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
interface_memberships::interface_memberships(rb_rule_matching_type upstream_in_rule_matching_type, const addr_storage& gaddr, const proxy_instance* pi, const simple_routing_data& routing_data)
{
    HC_LOG_TRACE("");

    if (upstream_in_rule_matching_type == RMT_FIRST) {
        process_upstream_in_first(gaddr, pi);
    } else {
        process_upstream_in_mutex(gaddr, pi, routing_data);
    }
}

void interface_memberships::merge_membership_infos(source_state& merge_to, const source_state& merge_from) const
{
    HC_LOG_TRACE("");

    if (merge_to.m_mc_filter == INCLUDE_MODE) {
        if (merge_from.m_mc_filter == INCLUDE_MODE) {
            merge_to.m_source_list += merge_from.m_source_list;
        } else if (merge_from.m_mc_filter == EXCLUDE_MODE) {
            merge_to.m_mc_filter = EXCLUDE_MODE;
            merge_to.m_source_list = merge_from.m_source_list - merge_to.m_source_list;
        } else {
            HC_LOG_ERROR("unknown filter mode in parameter merge_from");
        }
    } else if (merge_to.m_mc_filter == EXCLUDE_MODE) {
        if (merge_from.m_mc_filter == INCLUDE_MODE) {
            merge_to.m_source_list -= merge_from.m_source_list;
        } else if (merge_from.m_mc_filter == EXCLUDE_MODE) {
            merge_to.m_source_list *= merge_from.m_source_list;
        } else {
            HC_LOG_ERROR("unknown filter mode in parameter merge_from");
        }
    } else {
        HC_LOG_ERROR("unknown filter mode in parameter merge_to");
    }
}

void interface_memberships::process_upstream_in_first(const addr_storage& gaddr, const proxy_instance* pi)
{
    HC_LOG_TRACE("");

    state_list init_sstate_list;
    for (auto & downs_e : pi->m_downstreams) {
        init_sstate_list.push_back(state_pair(source_state(downs_e.second.m_querier->get_group_membership_infos(gaddr)), downs_e.second.m_interface));
    }

    //init and fill database
    for (auto & upstr_e : pi->m_upstreams) {

        state_list tmp_sstate_list;

        for (auto & cs : init_sstate_list) {

            source_state tmp_sstate;
            tmp_sstate.m_mc_filter = cs.first.m_mc_filter;

            //sort out all unwanted sources
            for (auto source_it = cs.first.m_source_list.begin(); source_it != cs.first.m_source_list.end();) {

                //downstream out
                if (!cs.second->match_output_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
                    source_it = cs.first.m_source_list.erase(source_it);
                    continue;
                }

                //upstream in
                if (!upstr_e.m_interface->match_input_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
                    tmp_sstate.m_source_list.insert(*source_it);
                    source_it = cs.first.m_source_list.erase(source_it);
                    continue;
                }

                ++source_it;
            }

            if (!tmp_sstate.m_source_list.empty()) {
                tmp_sstate_list.push_back(state_pair(tmp_sstate, cs.second));
            }

        }

        std::list<source_state> ret_source_list;
        for (auto & e : init_sstate_list) {
            ret_source_list.push_back(std::move(e.first));
        }
        m_data.push_back(std::pair<unsigned int, std::list<source_state>>(upstr_e.m_if_index, std::move(ret_source_list)));
        init_sstate_list = std::move(tmp_sstate_list);
    }

}

void interface_memberships::process_upstream_in_mutex(const addr_storage& gaddr, const proxy_instance* pi, const simple_routing_data& routing_data)
{
    HC_LOG_TRACE("");

    state_list ref_sstate_list;

    for (auto & downs_e : pi->m_downstreams) {
        ref_sstate_list.push_back(state_pair(source_state(downs_e.second.m_querier->get_group_membership_infos(gaddr)), downs_e.second.m_interface));
    }
    //print(ref_sstate_list);

    //init and fill database
    for (auto & upstr_e : pi->m_upstreams) {

        std::list<source_state> tmp_sstate_list;

        //for every downstream interface
        for (auto cs_it = ref_sstate_list.begin(); cs_it != ref_sstate_list.end();) {

            source_state tmp_sstate;
            tmp_sstate.m_mc_filter = cs_it->first.m_mc_filter;

            //sort out all unwanted sources
            for (auto source_it = cs_it->first.m_source_list.begin(); source_it != cs_it->first.m_source_list.end();) {

                //downstream out
                if (!cs_it->second->match_output_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
                    ++source_it;
                    continue;
                }

                //upstream in
                if (!upstr_e.m_interface->match_input_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
                    ++source_it;
                    continue;
                }

                const std::map<addr_storage, unsigned int>& available_sources = routing_data.get_interface_map(gaddr);
                auto av_src_it = available_sources.find(source_it->saddr);
                if (av_src_it != available_sources.end()) {

                    if (pi->is_upstream(av_src_it->second)) {
                        tmp_sstate.m_source_list.insert(*source_it);
                    }

                    //clean this->m_data
                    for (auto & data_e : m_data) {
                        for (auto sstate_it = data_e.second.begin(); sstate_it != data_e.second.end();) {

                            auto s_it = sstate_it->m_source_list.find(*source_it);
                            if (s_it != sstate_it->m_source_list.end()) {
                                sstate_it->m_source_list.erase(s_it);
                            }

                            if (sstate_it->m_source_list.empty()) {
                                sstate_it = data_e.second.erase(sstate_it);
                                continue;
                            }
                            ++sstate_it;
                        }
                    }

                    source_it = cs_it->first.m_source_list.erase(source_it);
                    continue;

                } else {
                    tmp_sstate.m_source_list.insert(*source_it);
                }

                ++source_it;
            }

            if (!tmp_sstate.m_source_list.empty()) {
                tmp_sstate_list.push_back(tmp_sstate);
            }

            if (cs_it->first.m_source_list.empty()) {
                cs_it = ref_sstate_list.erase(cs_it);
                continue;
            }

            ++cs_it;
        }

        m_data.push_back(std::pair<unsigned int, std::list<source_state>>(upstr_e.m_if_index, std::move(tmp_sstate_list)));
    }

}

source_state interface_memberships::get_group_memberships(unsigned int upstream_if_index)
{
    HC_LOG_TRACE("");

    source_state result;
    auto data_it = m_data.begin();
    if (data_it != m_data.end() && data_it->first != upstream_if_index) {
        HC_LOG_ERROR("unexpected upstream interface " << interfaces::get_if_name(upstream_if_index));
        return result;
    }

    for (auto & e : data_it->second) {
        merge_membership_infos(result, e);
    }

    m_data.pop_front();
    return result;
}

std::string interface_memberships::to_string() const
{
    std::ostringstream s;
    for (auto & e : m_data) {
        s << interfaces::get_if_name(e.first) <<  ":";
        for (auto & f : e.second) {
            s << std::endl << indention(f.to_string());
        }
    }
    return s.str();
}

#ifdef DEBUG_MODE
void interface_memberships::print(const state_list& sl)
{
    std::cout << "-- print state_list --" << std::endl;
    for (auto & e : sl) {
        std::cout << "source state(first): " << e.first.to_string() << std::endl;
        std::cout << "interface name(second): " << e.second->to_string_interface();
        std::cout << "interface rule binding(second): " << e.second->to_string_rule_binding();
    }

}
#endif /* DEBUG_MODE */

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
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

        //route calculation
        m_data.set_source(sm->get_if_index(), sm->get_gaddr(), s);

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

void simple_mc_proxy_routing::event_querier_state_change(unsigned int /*if_index*/, const addr_storage& gaddr)
{
    HC_LOG_TRACE("");

    //route calculation
    set_routes(gaddr, collect_interested_interfaces(gaddr, m_data.get_available_sources(gaddr)));

    //membership agregation
    if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_FIRST)) {
        process_membership_aggregation(RMT_FIRST, gaddr);
    } else if (is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
        process_membership_aggregation(RMT_MUTEX, gaddr);
    } else {
        HC_LOG_ERROR("unkown rule matching type in this context");
    }
}

void simple_mc_proxy_routing::timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    std::shared_ptr<new_source_timer_msg> tm;

    if (!msg.unique()) {
        switch (msg->get_type()) {
        case proxy_msg::NEW_SOURCE_TIMER_MSG: {
            tm = std::static_pointer_cast<new_source_timer_msg>(msg);

            auto cmp_source_lst = m_data.get_available_sources(tm->get_gaddr());
            auto cmp_source_it = cmp_source_lst.find(tm->get_saddr());
            if (cmp_source_it != cmp_source_lst.end()) {
                if (tm.get() == cmp_source_it->shared_source_timer.get()) {
                    auto saddr_it = m_data.refresh_source_or_del_it_if_unused(tm->get_gaddr(), tm->get_saddr());
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

bool simple_mc_proxy_routing::is_rule_matching_type(rb_interface_type interface_type, rb_interface_direction interface_direction, rb_rule_matching_type rule_matching_type) const
{
    HC_LOG_TRACE("");

    if (interface_type == IT_UPSTREAM) {
        if (interface_direction == ID_IN) {
            if (m_p->m_upstream_input_rule != nullptr) {
                return m_p->m_upstream_input_rule->get_rule_matching_type() == rule_matching_type;
            } else {
                HC_LOG_ERROR("upstream input rule is null");
                return false;
            }
        } else if (interface_direction == ID_OUT) {
            if (m_p->m_upstream_output_rule != nullptr) {
                return m_p->m_upstream_output_rule->get_rule_matching_type() == rule_matching_type;
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

std::list<std::pair<source, std::list<unsigned int>>> simple_mc_proxy_routing::collect_interested_interfaces(const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    const std::map<addr_storage, unsigned int>& input_if_index_map = m_data.get_interface_map(gaddr);

    //add upstream interfaces
    std::list<std::pair<source, std::list<unsigned int>>> rt_list;
    for (auto & s : slist) {

        auto input_if_it = input_if_index_map.find(s.saddr);
        if (input_if_it == input_if_index_map.end()) {
            HC_LOG_ERROR("input interface of multicast source " << s.saddr << " not found");
            return rt_list;
        }

        if (m_p->is_downstream(input_if_it->second)) {
            auto input_if_it = input_if_index_map.find(s.saddr);
            if (input_if_it == input_if_index_map.end()) {
                HC_LOG_ERROR("input interface of multicast source " << s.saddr << " not found");
                continue;
            }

            std::list<unsigned int> up_if_list;
            for (auto ui : m_p->m_upstreams) {
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

    for (auto & dif : m_p->m_downstreams) {
        dif.second.m_querier->suggest_to_forward_traffic(gaddr, rt_list, std::bind(filter_fun, dif.first, std::placeholders::_1));
    }

    return rt_list;
}

void simple_mc_proxy_routing::process_membership_aggregation(rb_rule_matching_type rule_matching_type, const addr_storage& gaddr)
{
    HC_LOG_TRACE("");

    if (rule_matching_type == RMT_FIRST || rule_matching_type == RMT_MUTEX) {
        interface_memberships im(rule_matching_type , gaddr, m_p, m_data);
        for (auto & e : m_p->m_upstreams) {
            send_record(e.m_if_index, gaddr, im.get_group_memberships(e.m_if_index));
        }
    } else {
        HC_LOG_ERROR("unkown rule matching type in this context");
    }
}

void simple_mc_proxy_routing::set_routes(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const
{
    HC_LOG_TRACE("");

    const std::map<addr_storage, unsigned int>& input_if_index_map = m_data.get_interface_map(gaddr);
    unsigned int input_if_index;

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
                vif_out.push_back(m_p->m_interfaces->get_virtual_if_index(outif));
            }

            auto input_if_it = input_if_index_map.find(e.first.saddr);
            if (input_if_it != std::end(input_if_index_map)) {
                input_if_index = input_if_it->second;

                bool use_this_interface = false;
                if (m_p->is_upstream(input_if_index)) {
                    if (check_interface(IT_UPSTREAM, ID_IN, input_if_index, input_if_index, gaddr, e.first.saddr)) {
                        use_this_interface = true;
                    }
                }

                if (!use_this_interface && m_p->is_downstream(input_if_index)) {
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

            m_p->m_routing->add_route(m_p->m_interfaces->get_virtual_if_index(input_if_index), gaddr, e.first.saddr, vif_out);
        }

    }
}

void simple_mc_proxy_routing::send_record(unsigned int upstream_if_index, const addr_storage& gaddr, const source_state& sstate) const
{
    HC_LOG_TRACE("");
    m_p->m_sender->send_record(upstream_if_index, sstate.m_mc_filter, gaddr, sstate.m_source_list);
}

void simple_mc_proxy_routing::del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const
{
    HC_LOG_TRACE("");
    m_p->m_routing->del_route(m_p->m_interfaces->get_virtual_if_index(if_index), gaddr, saddr);
}

std::shared_ptr<new_source_timer_msg> simple_mc_proxy_routing::set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
{
    HC_LOG_TRACE("");
    std::chrono::milliseconds source_life_time;
    if (m_p->is_upstream(if_index) && is_rule_matching_type(IT_UPSTREAM, ID_IN, RMT_MUTEX)) {
        source_life_time = m_p->m_upstream_input_rule->get_timeout();
    } else {
        source_life_time = get_source_life_time();
    }

    auto nst = std::make_shared<new_source_timer_msg>(if_index, gaddr, saddr, source_life_time);
    m_p->m_timing->add_time(get_source_life_time(), m_p, nst);

    return nst;
}

bool simple_mc_proxy_routing::check_interface(rb_interface_type interface_type, rb_interface_direction interface_direction, unsigned int checking_if_index, unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr) const
{
    HC_LOG_TRACE("");

    std::shared_ptr<interface> interf;
    if (interface_type == IT_UPSTREAM) {
        auto uinfo_it = std::find_if(m_p->m_upstreams.begin(), m_p->m_upstreams.end(), [&](const proxy_instance::upstream_infos & ui) {
            return ui.m_if_index == checking_if_index;
        });

        if (uinfo_it != m_p->m_upstreams.end()) {
            interf = uinfo_it->m_interface;
        } else {
            HC_LOG_ERROR("upstream interface " << interfaces::get_if_name(checking_if_index) << " not found");
            return false;
        }
    } else if (interface_type == IT_DOWNSTREAM) {
        auto dinfo_it = m_p->m_downstreams.find(checking_if_index);
        if (dinfo_it != m_p->m_downstreams.end()) {
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

    std::string input_if_index_name = interfaces::get_if_name(input_if_index);
    if (!input_if_index_name.empty()) {
        if (interface_direction == ID_IN) {
            return interf->match_input_filter(input_if_index_name, gaddr, saddr);
        } else if (interface_direction == ID_OUT) {
            return interf->match_output_filter(input_if_index_name, gaddr, saddr);
        } else {
            HC_LOG_ERROR("unkown interface direction");
            return false;
        }
    } else {
        HC_LOG_ERROR("failed to map interface index " << input_if_index << " to interface name");
        return false;
    }
}

std::string simple_mc_proxy_routing::to_string() const
{
    HC_LOG_TRACE("");
    return m_data.to_string();
}

