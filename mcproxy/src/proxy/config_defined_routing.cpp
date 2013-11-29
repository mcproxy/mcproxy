/*
 *
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
#include "include/proxy/config_defined_routing.hpp"

#include "include/proxy/interfaces.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/timing.hpp"

config_defined_routing::config_defined_routing(const proxy_instance* p)
    : simple_mc_proxy_routing(p)
{
    HC_LOG_TRACE("");
}

void config_defined_routing::send_records(std::list<unsigned int> upstream_if_indexes, const addr_storage& gaddr, mc_filter filter_mode, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");
    simple_mc_proxy_routing::send_records(upstream_if_indexes, gaddr, filter_mode, slist);
}

void config_defined_routing::add_route(unsigned int input_if_index, const addr_storage& gaddr, const std::list<std::pair<source, std::list<unsigned int>>>& output_if_index) const
{
    HC_LOG_TRACE("");
    simple_mc_proxy_routing::add_route(input_if_index, gaddr, output_if_index);
}


