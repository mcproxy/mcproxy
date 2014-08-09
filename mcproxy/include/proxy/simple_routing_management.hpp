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

#ifndef SIMPLE_ROUTING_MANAGEMENT_HPP
#define SIMPLE_ROUTING_MANAGEMENT_HPP

#include "include/proxy/routing_management.hpp"
#include "include/proxy/simple_routing_data.hpp"
#include "include/parser/interface.hpp"

#include <list>
#include <map>
#include <set>
#include <memory>
#include <chrono>

struct timer_msg;
struct new_source_timer_msg;
struct upstream_infos;
struct interface_infos;
struct downstream_infos;
class routing;
class sender;
class timing;
class interfaces;
class worker;

/**
 * @brief the simplest way of calculate forwarding rules.
 */
class simple_routing_management: public routing_management
{
private:
    const worker* const m_msg_worker;
    group_mem_protocol m_group_mem_protocol;
    const std::shared_ptr<simple_routing_data> m_data;
    const std::shared_ptr<const mroute_socket> m_mrt_sock;
    const std::shared_ptr<const sender> m_sender;
    const std::shared_ptr<const routing> m_routing;
    const std::shared_ptr<timing> m_timing;
    const std::shared_ptr<const interface_infos> m_ii;
    const std::shared_ptr<const interfaces> m_interfaces;

    std::chrono::seconds get_source_life_time();

    bool is_rule_matching_type(rb_interface_type interface_type, rb_interface_direction interface_direction, rb_rule_matching_type rule_matching_type) const;

    std::list<std::pair<source, std::list<unsigned int>>> collect_interested_interfaces(const addr_storage& gaddr, const source_list<source>& slist) const;

    void set_routes(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const;

    void del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    void send_record(unsigned int upstream_if_index, const addr_storage& gaddr, std::pair<mc_filter, const source_list<source>&> sstate) const;

    std::shared_ptr<new_source_timer_msg> set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);

    bool check_interface(rb_interface_type interface_type, rb_interface_direction interface_direction, unsigned int checking_if_index, unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    void process_membership_aggregation(rb_rule_matching_type rule_matching_type, const addr_storage& gaddr);

public:
    simple_routing_management(const worker* msg_worker, group_mem_protocol group_mem_protocol, const std::shared_ptr<const mroute_socket>& mrt_sock, const std::shared_ptr<const sender>& sender, const std::shared_ptr<const routing>& routing, const std::shared_ptr<timing>& timing, const std::shared_ptr<const interface_infos>& interface_infos, const std::shared_ptr<const interfaces> interfaces);

    void event_new_source(const std::shared_ptr<proxy_msg>& msg) override;

    void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr) override;

    void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) override;

    std::string to_string() const override;
};

#endif // SIMPLE_ROUTING_MANAGEMENT_HPP
