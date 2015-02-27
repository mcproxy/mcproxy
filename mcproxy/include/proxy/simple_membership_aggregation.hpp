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

#ifndef MEMBERSHIP_AGGREGATION_HPP
#define MEMBERSHIP_AGGREGATION_HPP

#include "include/parser/interface.hpp"
#include "include/proxy/def.hpp"

struct source;
struct interface_infos;
class simple_routing_data;

class test_status;

struct mem_source_state {
    mem_source_state();
    mem_source_state(mc_filter filter, const source_list<source>& slist);
    mem_source_state(std::pair<mc_filter, source_list<source>> sstate);

    mc_filter m_mc_filter = INCLUDE_MODE;
    source_list<source> m_source_list;

    std::pair<mc_filter, const source_list<source>&> get_mc_source_state() const;

    bool operator==(const mem_source_state& mss) const;
    bool operator!=(const mem_source_state& mss) const;

    std::string to_string() const;
};

struct filter_source_state {
    filter_source_state();
    filter_source_state(rb_filter_type rb_filter, const source_list<source>& slist);

    rb_filter_type m_rb_filter = FT_WHITELIST;
    source_list<source> m_source_list;

    bool operator==(const filter_source_state& fss) const;
    bool operator!=(const filter_source_state& fss) const;

    std::string to_string() const;
};

class simple_membership_aggregation
{
private:
    //using state_pair = std::pair<source_state, const std::shared_ptr<const interface>>;
    //using state_list = std::list<state_pair>;

    group_mem_protocol m_group_mem_protocol;
    const std::shared_ptr<const interface_infos> m_ii;
    const std::shared_ptr<const simple_routing_data> m_routing_data;

    const source_list<source> m_fallback_list;
    std::map<unsigned int, mem_source_state> m_data;
    
    source_list<source> get_source_list(const std::set<addr_storage>& addr_set);

    filter_source_state& convert_wildcard_filter(filter_source_state& rb_filter) const;

    mem_source_state& merge_group_memberships(mem_source_state& merge_to_mc_group, const mem_source_state& merge_from_mc_group) const;
    mem_source_state& merge_memberships_filter(mem_source_state& merge_to_mc_group, const filter_source_state& merge_from_rb_filter) const;
    mem_source_state& merge_memberships_filter_reminder(mem_source_state& merge_to_mc_group, const mem_source_state& result, const filter_source_state& merge_from_rb_filter) const;
    mem_source_state& disjoin_group_memberships(mem_source_state& merge_to_mc_group, const mem_source_state& merge_from_mc_group) const;


    void set_to_block_all(mem_source_state& mc_groups) const;

    std::map<unsigned int, mem_source_state> get_merged_mem(const addr_storage& gaddr);
    void process_upstream_in_first(const addr_storage& gaddr);
    void process_upstream_in_mutex(const addr_storage& gaddr);
    

    simple_membership_aggregation(group_mem_protocol group_mem_protocol);

    friend struct test_membership_aggregation;
public:
    simple_membership_aggregation(rb_rule_matching_type upstream_in_rule_matching_type, const addr_storage& gaddr, const std::shared_ptr<const simple_routing_data>& routing_data, group_mem_protocol group_mem_protocol, const std::shared_ptr<const interface_infos>& interface_infos);

    std::pair<mc_filter, const source_list<source>&> get_group_memberships(unsigned int upstream_if_index) const;

    std::string to_string() const;
};


#endif // MEMBERSHIP_AGGREGATION
