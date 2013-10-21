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
#include "include/proxy/def.hpp"

#include <map>
#include <set>
#include <list>


struct source;

using group_data = std::map<addr_storage, std::set<addr_storage>>;
using group_data_pair = std::pair<addr_storage, std::set<addr_storage>>;
using routing_data = std::map<unsigned int, group_data>;
using routing_data_pair = std::pair<unsigned int, group_data>;

class simple_mc_proxy_routing : public routing_management
{
private:
    bool is_upstream(unsigned int if_index);
    std::list<unsigned int> collect_interested_interfaces(unsigned int receiver_if, const addr_storage& gaddr, const addr_storage& saddr, mc_filter* filter_mode= nullptr, source_list<source>* slist= nullptr);
    void add_proxy_route(unsigned int input_if_index, const addr_storage &gaddr, const addr_storage &saddr, const std::list<unsigned int> &output_if_index) const;
    void send_record(unsigned int receiver_if, const addr_storage& gaddr, const addr_storage& saddr, mc_filter filter_mode, const source_list<source>& slist) const;
public:
    simple_mc_proxy_routing(const proxy_instance* p);

    void event_new_source(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) override;
    void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr) override;
};



#endif // SIMPLE_MC_PROXY_ROUTING_HPP

