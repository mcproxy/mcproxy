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

source_state::source_state()
    : m_mc_filter(INCLUDE_MODE)
{
    HC_LOG_TRACE("");
}

source_state::source_state(rb_filter_type rb_filter, source_list<source> slist)
    : m_rb_filter(rb_filter)
    , m_source_list(slist)
{
    HC_LOG_TRACE("");
}

source_state::source_state(mc_filter mc_filter, source_list<source> slist)
    : m_mc_filter(mc_filter)
    , m_source_list(slist)
{
    HC_LOG_TRACE("");
}

source_state::source_state(std::pair<mc_filter, source_list<source>> sstate)
    : source_state(sstate.first, sstate.second)
{
    HC_LOG_TRACE("");
}

std::pair<mc_filter, const source_list<source>&> source_state::get_mc_source_list() const
{
    HC_LOG_TRACE("");
    return std::pair<mc_filter, const source_list<source>&>(m_mc_filter, m_source_list);
}

bool source_state::operator==(const source_state& ss) const{
    HC_LOG_TRACE("");
    return m_source_list == ss.m_source_list && m_mc_filter == ss.m_mc_filter;
}

bool source_state::operator!=(const source_state& ss) const{
    HC_LOG_TRACE("");
    return !(*this == ss);
}

std::string source_state::to_string_mc() const
{
    std::ostringstream s;
    s << get_mc_filter_name(m_mc_filter) << "{" << m_source_list << "}";
    return s.str();
}

std::string source_state::to_string_rb() const
{
    std::ostringstream s;
    s << get_rb_filter_type_name(m_rb_filter) << "{" << m_source_list << "}";
    return s.str();
}

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

void simple_membership_aggregation::set_to_block_all(source_state& mc_groups) const
{
    mc_groups.m_source_list.clear();
    mc_groups.m_mc_filter = INCLUDE_MODE;
}

source_state& simple_membership_aggregation::merge_group_memberships(source_state& merge_to_mc_group, const source_state& merge_from_mc_group) const
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

source_state& simple_membership_aggregation::convert_wildcard_filter(source_state& rb_filter) const
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


source_state& simple_membership_aggregation::merge_memberships_filter(source_state& merge_to_mc_group, const source_state& merge_from_rb_filter) const
{
    HC_LOG_TRACE("");
    source_state from = merge_from_rb_filter;
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

source_state& simple_membership_aggregation::merge_memberships_filter_reminder(source_state& merge_to_mc_group, const source_state& result, const source_state& merge_from_rb_filter) const
{
    HC_LOG_TRACE("");
    source_state from = merge_from_rb_filter;
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

void simple_membership_aggregation::process_upstream_in_first(const addr_storage& gaddr){
        
}


void simple_membership_aggregation::process_upstream_in_mutex(const addr_storage& gaddr){


}

std::pair<mc_filter, const source_list<source>&> simple_membership_aggregation::get_group_memberships(unsigned int upstream_if_index){

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

