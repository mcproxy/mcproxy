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

#ifndef CONFIG_DEFINED_ROUTING_HPP
#define CONFIG_DEFINED_ROUTING_HPP

#include "include/proxy/simple_mc_proxy_routing.hpp"

class config_defined_routing : public simple_mc_proxy_routing
{
private:
    virtual void send_records(std::list<unsigned int> upstream_if_indexes, const addr_storage& gaddr, mc_filter filter_mode, const source_list<source>& slist) const override;

    virtual void add_route(const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const override;

public:
    config_defined_routing(const proxy_instance* p);
};

#endif // CONFIG_DEFINED_ROUTING_HPP
