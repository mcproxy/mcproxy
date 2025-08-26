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

#ifndef SIMPLE_MC_PROXY_ROUTING_HPP
#define SIMPLE_MC_PROXY_ROUTING_HPP


#include <list>
#include <memory>
#include <chrono>
#include <algorithm>

#include "include/hamcast_logging.h"

#include "include/utils/addr_storage.hpp"

#include "include/parser/interface.hpp"

#include "include/proxy/interfaces.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/routing_management.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/simple_routing_data.hpp"
#include "include/proxy/timing.hpp"


struct timer_msg;
struct source;
struct new_source_timer_msg;

struct source_state {
    source_state();
    source_state(std::pair<mc_filter, source_list<source>> sstate);
    mc_filter m_mc_filter;
    source_list<source> m_source_list;
    std::string to_string() const;
};

class interface_memberships
{
private:
    using state_pair = std::pair<source_state, const std::shared_ptr<const interface>>;
    using state_list = std::list<state_pair>;

    std::list<std::pair<unsigned int, std::list<source_state>>> m_data;

    void merge_membership_infos(source_state& merge_to, const source_state& merge_from) const;

    void process_upstream_in_first(const addr_storage& gaddr, const proxy_instance* pi);
    void process_upstream_in_mutex(const addr_storage& gaddr, const proxy_instance* pi, const simple_routing_data& routing_data);

public:
    interface_memberships(rb_rule_matching_type upstream_in_rule_matching_type, const addr_storage& gaddr, const proxy_instance* pi, const simple_routing_data& routing_data);

    source_state get_group_memberships(unsigned int upstream_if_index);

    std::string to_string() const;

    static void print(const state_list& sl);
};

/**
 * @brief the simplest way of calculate forwarding rules.
 */
class simple_mc_proxy_routing : public routing_management
{
private:
    simple_routing_data m_data;

    std::chrono::seconds get_source_life_time();

    bool is_rule_matching_type(rb_interface_type interface_type, rb_interface_direction interface_direction, rb_rule_matching_type rule_matching_type) const;

    std::list<std::pair<source, std::list<unsigned int>>> collect_interested_interfaces(const addr_storage& gaddr, const source_list<source>& slist) const;

    void set_routes(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const;

    void del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    void send_record(unsigned int upstream_if_index, const addr_storage& gaddr, const source_state& sstate) const;

    std::shared_ptr<new_source_timer_msg> set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);

    bool check_interface(rb_interface_type interface_type, rb_interface_direction interface_direction, unsigned int checking_if_index, unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    void process_membership_aggregation(rb_rule_matching_type rule_matching_type, const addr_storage& gaddr);

public:
    simple_mc_proxy_routing(const proxy_instance* p);

    void event_new_source(const std::shared_ptr<proxy_msg>& msg) override;

    void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr) override;

    void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) override;

    std::string to_string() const override;
};

#endif // SIMPLE_MC_PROXY_ROUTING_HPP
