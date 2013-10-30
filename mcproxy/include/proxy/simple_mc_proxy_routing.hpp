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

#include <list>
#include <memory>
#include <chrono>

struct timer_msg;
struct source;
struct new_source_timer;



class simple_mc_proxy_routing : public routing_management
{
private:
    simple_routing_data m_data;

    std::chrono::seconds get_source_life_time();

    bool is_upstream(unsigned int if_index) const;
    unsigned int get_upstream() const;

    std::list<std::pair<source, std::list<unsigned int>>> collect_interested_interfaces(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) const;

    std::pair<mc_filter, source_list<source>> collect_group_membership_infos(const addr_storage& gaddr);

    void merge_membership_infos(std::pair<mc_filter, source_list<source>>& merge_to, const std::pair<mc_filter, source_list<source>>& merge_from ) const;

    void add_route(unsigned int input_if_index, const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const;

    void del_route(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) const;

    void send_record(unsigned int if_index, const addr_storage& gaddr, mc_filter filter_mode, const source_list<source>& slist) const;

    std::shared_ptr<new_source_timer> set_source_timer(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);

public:
    simple_mc_proxy_routing(const proxy_instance* p);

    void event_new_source(const std::shared_ptr<proxy_msg>& msg) override;
    void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) override;

    void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) override;

    std::string to_string() const override;
};



#endif // SIMPLE_MC_PROXY_ROUTING_HPP

