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

struct source_state {
    source_state();
    source_state(rb_filter_type rb_filter, source_list<source> slist);
    source_state(mc_filter, source_list<source>);
    source_state(std::pair<mc_filter, source_list<source>> sstate);

    union {
        mc_filter m_mc_filter; // = INCLUDE_MODE;
        rb_filter_type m_rb_filter;
    };
    source_list<source> m_source_list;

    std::pair<mc_filter, const source_list<source>&> get_mc_source_list() const;

    bool operator==(const source_state& ss) const;

    std::string to_string_mc() const;
    std::string to_string_rb() const;
};

class simple_membership_aggregation
{
private:
    //using state_pair = std::pair<source_state, const std::shared_ptr<const interface>>;
    //using state_list = std::list<state_pair>;

    group_mem_protocol m_group_mem_protocol;
    const std::shared_ptr<const interface_infos> m_ii;
    const std::shared_ptr<const simple_routing_data> m_routing_data;

    std::list<std::pair<unsigned int, std::list<source_state>>> m_data;

    const source_state& convert_wildcard_filter(const source_state& rb_filter) const;

    source_state& merge_group_memberships(source_state& merge_to_mc_group, const source_state& merge_from_mc_group) const;
    source_state& merge_memberships_filter(source_state& merge_to_mc_group, const source_state& merge_from_rb_filter) const;
    source_state& merge_memberships_filter_reminder(source_state& merge_to_mc_group, const source_state& result, const source_state& merge_from_rb_filter) const;

    void set_to_block_all(source_state& mc_groups) const;

    void process_upstream_in_first(const addr_storage& gaddr);
    void process_upstream_in_mutex(const addr_storage& gaddr);


    simple_membership_aggregation(group_mem_protocol group_mem_protocol);
public:
    simple_membership_aggregation(rb_rule_matching_type upstream_in_rule_matching_type, const addr_storage& gaddr, const std::shared_ptr<const simple_routing_data>& routing_data, group_mem_protocol group_mem_protocol, const std::shared_ptr<const interface_infos>& interface_infos);

    std::pair<mc_filter, const source_list<source>&> get_group_memberships(unsigned int upstream_if_index);

    std::string to_string() const;

    //static void print(const state_list& sl);
    static void test_merge_functions();
};


#endif // MEMBERSHIP_AGGREGATION
