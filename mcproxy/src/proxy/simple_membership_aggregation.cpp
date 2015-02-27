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
#include "include/proxy/simple_membership_aggregation.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/simple_routing_data.hpp"

#include <algorithm>
#include <utility>

mem_source_state::mem_source_state()
{
    HC_LOG_TRACE("");
}

mem_source_state::mem_source_state(mc_filter filter, const source_list<source>& slist)
    : m_mc_filter(filter)
    , m_source_list(slist)
{
    HC_LOG_TRACE("");
}

mem_source_state::mem_source_state(std::pair<mc_filter, source_list<source>> sstate)
    : mem_source_state(sstate.first, sstate.second)
{
    HC_LOG_TRACE("");
}

std::pair<mc_filter, const source_list<source>&> mem_source_state::get_mc_source_state() const
{
    HC_LOG_TRACE("");
    return std::pair<mc_filter, const source_list<source>&>(m_mc_filter, m_source_list);
}

bool mem_source_state::operator==(const mem_source_state& mss) const
{
    HC_LOG_TRACE("");
    return m_source_list == mss.m_source_list && m_mc_filter == mss.m_mc_filter;
}

bool mem_source_state::operator!=(const mem_source_state& mss) const
{
    HC_LOG_TRACE("");
    return !(*this == mss);
}

std::string mem_source_state::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << get_mc_filter_name(m_mc_filter) << "{" << m_source_list << "}";
    return s.str();
}
//-----------------------------------------------------------------
filter_source_state::filter_source_state()
{
    HC_LOG_TRACE("");
}

filter_source_state::filter_source_state(rb_filter_type rb_filter, const source_list<source>& slist)
    : m_rb_filter(rb_filter)
    , m_source_list(slist)
{
    HC_LOG_TRACE("");
}

bool filter_source_state::operator==(const filter_source_state& mss) const
{
    HC_LOG_TRACE("");
    return m_source_list == mss.m_source_list && m_rb_filter == mss.m_rb_filter;
}

bool filter_source_state::operator!=(const filter_source_state& mss) const
{
    HC_LOG_TRACE("");
    return !(*this == mss);
}

std::string filter_source_state::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << get_rb_filter_type_name(m_rb_filter) << "{" << m_source_list << "}";
    return s.str();
}

//-----------------------------------------------------------------

simple_membership_aggregation::simple_membership_aggregation(group_mem_protocol group_mem_protocol)
    : m_group_mem_protocol(group_mem_protocol)
    , m_routing_data(nullptr)
{
    HC_LOG_TRACE("");

}

simple_membership_aggregation::simple_membership_aggregation(rb_rule_matching_type upstream_in_rule_matching_type, const addr_storage& gaddr, const std::shared_ptr<const simple_routing_data>& routing_data, group_mem_protocol group_mem_protocol, const std::shared_ptr<const interface_infos>& interface_infos)
    : m_group_mem_protocol(group_mem_protocol)
    , m_ii(interface_infos)
    , m_routing_data(routing_data)

{
    HC_LOG_TRACE("");

    if (upstream_in_rule_matching_type == RMT_FIRST) {
        process_upstream_in_first(gaddr);
    } else if (upstream_in_rule_matching_type == RMT_MUTEX) {
        process_upstream_in_mutex(gaddr);
    } else {
        HC_LOG_ERROR("unkown upstream input rule matching type");
    }
}

void simple_membership_aggregation::set_to_block_all(mem_source_state& mc_groups) const
{
    mc_groups.m_source_list.clear();
    mc_groups.m_mc_filter = INCLUDE_MODE;
}

mem_source_state& simple_membership_aggregation::merge_group_memberships(mem_source_state& merge_to_mc_group, const mem_source_state& merge_from_mc_group) const
{
    HC_LOG_TRACE("");

    if (merge_to_mc_group.m_mc_filter == INCLUDE_MODE) {
        if (merge_from_mc_group.m_mc_filter == INCLUDE_MODE) {
            merge_to_mc_group.m_source_list += merge_from_mc_group.m_source_list;
        } else if (merge_from_mc_group.m_mc_filter == EXCLUDE_MODE) {
            merge_to_mc_group.m_mc_filter = EXCLUDE_MODE;
            merge_to_mc_group.m_source_list = merge_from_mc_group.m_source_list - merge_to_mc_group.m_source_list;
        } else {
            HC_LOG_ERROR("unknown mc filter mode in parameter merge_from_mc_group");
            set_to_block_all(merge_to_mc_group);
        }
    } else if (merge_to_mc_group.m_mc_filter == EXCLUDE_MODE) {
        if (merge_from_mc_group.m_mc_filter == INCLUDE_MODE) {
            merge_to_mc_group.m_source_list -= merge_from_mc_group.m_source_list;
        } else if (merge_from_mc_group.m_mc_filter == EXCLUDE_MODE) {
            merge_to_mc_group.m_source_list *= merge_from_mc_group.m_source_list;
        } else {
            HC_LOG_ERROR("unknown mc filter mode in parameter merge_from_mc_group");
            set_to_block_all(merge_to_mc_group);
        }
    } else {
        HC_LOG_ERROR("unknown mc filter mode in parameter merge_to_mc_group");
        set_to_block_all(merge_to_mc_group);
    }

    return merge_to_mc_group;
}

filter_source_state& simple_membership_aggregation::convert_wildcard_filter(filter_source_state& rb_filter) const
{
    HC_LOG_TRACE("");

    //if contains wildcard address
    if (rb_filter.m_source_list.find(addr_storage(get_addr_family(m_group_mem_protocol))) != std::end(rb_filter.m_source_list)) {
        if (rb_filter.m_rb_filter == FT_WHITELIST) { //WL{*} ==> BL{}
            rb_filter.m_rb_filter = FT_BLACKLIST;
            rb_filter.m_source_list.clear();
        } else if (rb_filter.m_rb_filter == FT_BLACKLIST) { //BL{*} ==> WL{}
            rb_filter.m_rb_filter = FT_WHITELIST;
            rb_filter.m_source_list.clear();
        } else {
            HC_LOG_ERROR("unknown rb filter mode in parameter merge_from_rb_filter");
        }
    }

    return rb_filter;
}


mem_source_state& simple_membership_aggregation::merge_memberships_filter(mem_source_state& merge_to_mc_group, const filter_source_state& merge_from_rb_filter) const
{
    HC_LOG_TRACE("");
    filter_source_state from = merge_from_rb_filter;
    //not very efficient ?????
    convert_wildcard_filter(from);

    if (merge_to_mc_group.m_mc_filter == INCLUDE_MODE) {
        if (from.m_rb_filter == FT_WHITELIST) {
            merge_to_mc_group.m_source_list *= from.m_source_list;
        } else if (from.m_rb_filter == FT_BLACKLIST) {
            merge_to_mc_group.m_source_list -= from.m_source_list;
        } else {
            HC_LOG_ERROR("unknown rb filter mode in parameter merge_from_rb_filter");
            set_to_block_all(merge_to_mc_group);
        }
    } else if (merge_to_mc_group.m_mc_filter == EXCLUDE_MODE) {
        if (from.m_rb_filter == FT_WHITELIST) {
            merge_to_mc_group.m_mc_filter = INCLUDE_MODE;
            merge_to_mc_group.m_source_list = from.m_source_list - merge_to_mc_group.m_source_list;
        } else if (from.m_rb_filter == FT_BLACKLIST) {
            merge_to_mc_group.m_source_list += from.m_source_list;
        } else {
            HC_LOG_ERROR("unknown rb filter mode in parameter merge_from_rb_filter");
            set_to_block_all(merge_to_mc_group);
        }
    } else {
        HC_LOG_ERROR("unknown mc filter mode in parameter merge_to_mc_group");
        set_to_block_all(merge_to_mc_group);
    }

    return merge_to_mc_group;
}

mem_source_state& simple_membership_aggregation::merge_memberships_filter_reminder(mem_source_state& merge_to_mc_group, const mem_source_state& result, const filter_source_state& merge_from_rb_filter) const
{
    HC_LOG_TRACE("");
    filter_source_state from = merge_from_rb_filter;
    //not very efficient ?????
    convert_wildcard_filter(from);

    if (merge_to_mc_group.m_mc_filter == INCLUDE_MODE) {
        if (from.m_rb_filter == FT_WHITELIST || from.m_rb_filter == FT_BLACKLIST) {
            merge_to_mc_group.m_source_list -= result.m_source_list;
        } else {
            HC_LOG_ERROR("unknown rb filter mode in parameter merge_from_rb_filter");
        }
    } else if (merge_to_mc_group.m_mc_filter == EXCLUDE_MODE) {
        if (from.m_rb_filter == FT_WHITELIST) {
            merge_to_mc_group.m_source_list += result.m_source_list;
        } else if (from.m_rb_filter == FT_BLACKLIST) {
            merge_to_mc_group.m_source_list = result.m_source_list - merge_to_mc_group.m_source_list;
            merge_to_mc_group.m_mc_filter = INCLUDE_MODE;
        } else {
            HC_LOG_ERROR("unknown rb filter mode in parameter merge_from_rb_filter");
        }
    } else {
        HC_LOG_ERROR("unknown mc filter mode in parameter merge_to_mc_group");
    }

    return merge_to_mc_group;
}

mem_source_state& simple_membership_aggregation::disjoin_group_memberships(mem_source_state& merge_to_mc_group, const mem_source_state& merge_from_mc_group) const
{
    HC_LOG_TRACE("");

    if (merge_to_mc_group.m_mc_filter == INCLUDE_MODE) {
        if (merge_from_mc_group.m_mc_filter == INCLUDE_MODE) {
            merge_to_mc_group.m_source_list -= merge_from_mc_group.m_source_list;
        } else if (merge_from_mc_group.m_mc_filter == EXCLUDE_MODE) {
            merge_to_mc_group.m_source_list *= merge_from_mc_group.m_source_list;
        } else {
            HC_LOG_ERROR("unknown mc filter mode in parameter merge_from_mc_group");
            set_to_block_all(merge_to_mc_group);
        }
    } else if (merge_to_mc_group.m_mc_filter == EXCLUDE_MODE) {
        if (merge_from_mc_group.m_mc_filter == INCLUDE_MODE) {
            merge_to_mc_group.m_source_list += merge_from_mc_group.m_source_list;
        } else if (merge_from_mc_group.m_mc_filter == EXCLUDE_MODE) {
            merge_to_mc_group.m_mc_filter = INCLUDE_MODE;
            merge_to_mc_group.m_source_list -= merge_from_mc_group.m_source_list;
        } else {
            HC_LOG_ERROR("unknown mc filter mode in parameter merge_from_mc_group");
            set_to_block_all(merge_to_mc_group);
        }
    } else {
        HC_LOG_ERROR("unknown mc filter mode in parameter merge_to_mc_group");
        set_to_block_all(merge_to_mc_group);
    }

    return merge_to_mc_group;
}


source_list<source> simple_membership_aggregation::get_source_list(const std::set<addr_storage>& addr_set)
{
    HC_LOG_TRACE("");
    source_list<source> result;
    //a reverse iteration is maybe faster????????????
    std::for_each(addr_set.begin(), addr_set.end(), [&result](const addr_storage & e) {
        result.insert(e);
    });
    return result;
}

std::map<unsigned int, mem_source_state> simple_membership_aggregation::get_merged_mem(const addr_storage& gaddr)
{
    HC_LOG_TRACE("");

    //filters group memberships of the queries with the interface filter
    auto get_filtered_mem = [&gaddr, this](const std::pair<const unsigned int, downstream_infos>& dstream, const std::string & input_if_name, bool explicite_if_name) {
        rb_filter_type tmp_ft = dstream.second.m_interface->get_filter_type(ID_OUT);
        const source_list<source>& tmp_srcl = get_source_list(dstream.second.m_interface->get_saddr_set(ID_OUT, input_if_name, gaddr, explicite_if_name));
        auto filter = filter_source_state(tmp_ft, tmp_srcl);

        std::pair<mc_filter, source_list<source>> tmp_gmem_src = dstream.second.m_querier->get_group_membership_infos(gaddr);
        auto gmem_src = mem_source_state(tmp_gmem_src);

        merge_memberships_filter(gmem_src, filter); //(p1)
        return gmem_src;
    };

    //upstream interface index, membership infos
    std::map<unsigned int, mem_source_state> downstreams_mem_merge; //(p3)
    mem_source_state downstreams_merge_empty_if; //(p2

    for (auto & dstream : m_ii->m_downstreams) {
        merge_group_memberships(downstreams_merge_empty_if, get_filtered_mem(dstream, "", false)); //(p2)
    }

    for (auto & ustream : m_ii->m_upstreams) {
        mem_source_state if_specific_merge;
        std::string uif_name = interfaces::get_if_name(ustream.m_if_index);
        for (auto & dstream : m_ii->m_downstreams) {
            merge_group_memberships(if_specific_merge, get_filtered_mem(dstream, uif_name, true)); //(p2)
        }
        merge_group_memberships(if_specific_merge, downstreams_merge_empty_if);
        downstreams_mem_merge.insert(std::make_pair(ustream.m_if_index, std::move(if_specific_merge)));
    }

    return downstreams_mem_merge;
}

void simple_membership_aggregation::process_upstream_in_first(const addr_storage& gaddr)
{
    HC_LOG_TRACE("");
    std::map<unsigned int, mem_source_state> downstreams_mem_merge = get_merged_mem(gaddr); //(p3) and result

    mem_source_state reminder;
    for (auto & ustream : m_ii->m_upstreams) {
        auto d_mem_merge_it = downstreams_mem_merge.find(ustream.m_if_index);
        if (d_mem_merge_it == downstreams_mem_merge.end()) {
            HC_LOG_ERROR("upstream interface index (" << ustream.m_if_index << ") not found");
            downstreams_mem_merge.erase(d_mem_merge_it);
            continue;
        }

        mem_source_state& d_mem_merge = d_mem_merge_it->second; //(p3)

        rb_filter_type tmp_ft = ustream.m_interface->get_filter_type(ID_IN);
        const source_list<source>& tmp_srcl = get_source_list(ustream.m_interface->get_saddr_set(ID_IN, interfaces::get_if_name(ustream.m_if_index), gaddr));
        auto filter = filter_source_state(tmp_ft, tmp_srcl);
        merge_memberships_filter(d_mem_merge, filter);
        merge_memberships_filter_reminder(reminder, d_mem_merge, filter);
    }

    m_data = std::move(downstreams_mem_merge);
}


void simple_membership_aggregation::process_upstream_in_mutex(const addr_storage& gaddr)
{


}

std::pair<mc_filter, const source_list<source>&> simple_membership_aggregation::get_group_memberships(unsigned int upstream_if_index) const
{
    HC_LOG_TRACE("");
    auto tmp_it = m_data.find(upstream_if_index);
    if (tmp_it != m_data.end()) {
        return std::make_pair(tmp_it->second.m_mc_filter, tmp_it->second.m_source_list);
    } else {
        HC_LOG_ERROR("upstream if index " << upstream_if_index << " not found");
        return std::make_pair(INCLUDE_MODE, m_fallback_list);
    }
}

//void simple_membership_aggregation::process_upstream_in_first(const addr_storage& gaddr)
//{
//HC_LOG_TRACE("");

//state_list init_sstate_list;
//for (auto & downs_e : m_ii->m_downstreams) {
//init_sstate_list.push_back(state_pair(source_state(downs_e.second.m_querier->get_group_membership_infos(gaddr)), downs_e.second.m_interface));
//}

////init and fill database
//for (auto & upstr_e : m_ii->m_upstreams) {

//state_list tmp_sstate_list;

//for (auto & cs : init_sstate_list) {

//source_state tmp_sstate;
//tmp_sstate.m_mc_filter = cs.first.m_mc_filter;

////sort out all unwanted sources
//for (auto source_it = cs.first.m_source_list.begin(); source_it != cs.first.m_source_list.end();) {

////downstream out TODO: old code match
////if (!cs.second->match_output_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
////source_it = cs.first.m_source_list.erase(source_it);
////continue;
////}

////upstream in TODO: old code match
////if (!upstr_e.m_interface->match_input_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
////tmp_sstate.m_source_list.insert(*source_it);
////source_it = cs.first.m_source_list.erase(source_it);
////continue;
////}

//++source_it;
//}

//if (!tmp_sstate.m_source_list.empty()) {
//tmp_sstate_list.push_back(state_pair(tmp_sstate, cs.second));
//}

//}

//std::list<source_state> ret_source_list;
//for (auto & e : init_sstate_list) {
//ret_source_list.push_back(std::move(e.first));
//}
//m_data.push_back(std::pair<unsigned int, std::list<source_state>>(upstr_e.m_if_index, std::move(ret_source_list)));
//init_sstate_list = std::move(tmp_sstate_list);
//}

//}

//void simple_membership_aggregation::process_upstream_in_mutex(const addr_storage& gaddr)
//{
//HC_LOG_TRACE("");

//state_list ref_sstate_list;

//for (auto & downs_e : m_ii->m_downstreams) {
//ref_sstate_list.push_back(state_pair(source_state(downs_e.second.m_querier->get_group_membership_infos(gaddr)), downs_e.second.m_interface));
//}
////print(ref_sstate_list);

////init and fill database
//for (auto & upstr_e : m_ii->m_upstreams) {

//std::list<source_state> tmp_sstate_list;

////for every downstream interface
//for (auto cs_it = ref_sstate_list.begin(); cs_it != ref_sstate_list.end();) {

//source_state tmp_sstate;
//tmp_sstate.m_mc_filter = cs_it->first.m_mc_filter;

////sort out all unwanted sources
//for (auto source_it = cs_it->first.m_source_list.begin(); source_it != cs_it->first.m_source_list.end();) {

////downstream out TODO: old code match
////if (!cs_it->second->match_output_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
////++source_it;
////continue;
////}

////upstream in TODO: old code match
////if (!upstr_e.m_interface->match_input_filter(interfaces::get_if_name(upstr_e.m_if_index), gaddr, source_it->saddr)) {
////++source_it;
////continue;
////}

//const std::map<addr_storage, unsigned int>& available_sources = m_routing_data->get_interface_map(gaddr);
//auto av_src_it = available_sources.find(source_it->saddr);
//if (av_src_it != available_sources.end()) {

//if (m_ii->is_upstream(av_src_it->second)) {
//tmp_sstate.m_source_list.insert(*source_it);
//}

////clean this->m_data
//for (auto & data_e : m_data) {
//for (auto sstate_it = data_e.second.begin(); sstate_it != data_e.second.end();) {

//auto s_it = sstate_it->m_source_list.find(*source_it);
//if (s_it != sstate_it->m_source_list.end()) {
//sstate_it->m_source_list.erase(s_it);
//}

//if (sstate_it->m_source_list.empty()) {
//sstate_it = data_e.second.erase(sstate_it);
//continue;
//}
//++sstate_it;
//}
//}

//source_it = cs_it->first.m_source_list.erase(source_it);
//continue;

//} else {
//tmp_sstate.m_source_list.insert(*source_it);
//}

//++source_it;
//}

//if (!tmp_sstate.m_source_list.empty()) {
//tmp_sstate_list.push_back(tmp_sstate);
//}

//if (cs_it->first.m_source_list.empty()) {
//cs_it = ref_sstate_list.erase(cs_it);
//continue;
//}

//++cs_it;
//}

//m_data.push_back(std::pair<unsigned int, std::list<source_state>>(upstr_e.m_if_index, std::move(tmp_sstate_list)));
//}

//}

//std::pair<mc_filter, const source_list<source>&> simple_membership_aggregation::get_group_memberships(unsigned int upstream_if_index)
//{
//HC_LOG_TRACE("");

//source_state result;
//auto data_it = m_data.begin();
//if (data_it != m_data.end() && data_it->first != upstream_if_index) {
//HC_LOG_ERROR("unexpected upstream interface " << interfaces::get_if_name(upstream_if_index));
//return result.get_mc_source_list();
//}

//for (auto & e : data_it->second) {
////TODO
////merge_membership_infos(result, e);
//}

//m_data.pop_front();
//return result.get_mc_source_list();
//}

std::string simple_membership_aggregation::to_string() const
{
    std::ostringstream s;
    //for (auto & e : m_data) {
    //s << interfaces::get_if_name(e.first) <<  ":";
    //for (auto & f : e.second) {
    //s << std::endl << indention(f.to_string());
    //}
    //}
    return s.str();
}

