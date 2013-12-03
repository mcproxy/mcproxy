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

#include "include/proxy/routing_management.hpp"
#include "include/proxy/simple_routing_data.hpp"
#include "include/parser/interface.hpp"

#include <list>
#include <memory>
#include <chrono>

struct timer_msg;
struct source;
struct new_source_timer;

/**
 * @brief the simplest way of calculate forwarding rules.
 */
class simple_mc_proxy_routing : public routing_management
{
protected:
    simple_routing_data m_data;

    std::chrono::seconds get_source_life_time();

    virtual bool is_upstream(unsigned int if_index) const;
    virtual bool is_downstream(unsigned int if_index) const;

    virtual bool is_rule_matching_type(rb_interface_type interface_type, rb_interface_direction interface_direction, rb_rule_matching_type rule_matching_type) const;

    virtual std::list<std::pair<source, std::list<unsigned int>>> collect_interested_interfaces(unsigned int event_if_index, const addr_storage& gaddr, const source_list<source>& slist) const;

    virtual std::pair<mc_filter, source_list<source>> collect_group_membership_infos(const addr_storage& gaddr);

    virtual void merge_membership_infos(std::pair<mc_filter, source_list<source>>& merge_to, const std::pair<mc_filter, source_list<source>>& merge_from) const;

    virtual void set_routes(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const;

    virtual void del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    virtual void send_records(std::list<unsigned int> upstream_if_indexes, const addr_storage& gaddr, mc_filter filter_mode, const source_list<source>& slist) const;

    virtual std::shared_ptr<new_source_timer> set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);

    virtual bool check_interface(rb_interface_type interface_type, rb_interface_direction interface_direction, unsigned int checking_if_index, unsigned int input_if_index, const addr_storage& gaddr, const addr_storage& saddr) const;
public:
    simple_mc_proxy_routing(const proxy_instance* p);

    virtual void event_new_source(const std::shared_ptr<proxy_msg>& msg) override;

    virtual void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) override;

    virtual void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) override;

    virtual std::string to_string() const override;
};



#endif // SIMPLE_MC_PROXY_ROUTING_HPP

