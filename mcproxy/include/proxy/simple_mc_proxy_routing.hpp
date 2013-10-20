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
#include <map>
#include <set>

using group_data = std::map<addr_storage, std::set<addr_storage>>;
using group_data_pair = std::pair<addr_storage, std::set<addr_storage>>;
using routing_data = std::map<unsigned int, group_data>;
using routing_data_pair = std::pair<unsigned int, group_data>;

class simple_mc_proxy_routing : public routing_management
{
private:
    routing_data m_data;
    void add_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);
    void add_source(routing_data::iterator data_it, const addr_storage& gaddr, const addr_storage& saddr);
    void add_source(group_data::iterator gaddr_it, const addr_storage& saddr);

    void del_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr);
    void del_source(routing_data::iterator data_it, const addr_storage& gaddr, const addr_storage& saddr);
    void del_source(group_data::iterator _gaddr_it, const addr_storage& saddr);
public:
    simple_mc_proxy_routing(const proxy_instance* p);

    void event_new_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) override;
    void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) override;
    void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) override;

    std::string to_string() const override;
    static void test_simple_mc_proxy_routing();
};



#endif // SIMPLE_MC_PROXY_ROUTING_HPP

