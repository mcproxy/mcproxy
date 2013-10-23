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

#ifndef SIMPLE_ROUTING_DATA_HPP 
#define SIMPLE_ROUTING_DATA_HPP

#include "include/proxy/def.hpp"
#include <map>
#include <memory>
#include <string>
#include <set>

class addr_storage;
struct source;
struct timer_msg;


using s_group_data = std::map<addr_storage, source_list<source>>;
using s_group_data_pair = std::pair<addr_storage, source_list<source>>;

using s_routing_data = std::map<unsigned int, s_group_data>;
using s_routing_data_pair = std::pair<unsigned int, s_group_data>;

class simple_routing_data 
{
private:
    s_routing_data m_data;

public:
    void add_source(unsigned int if_index, const addr_storage& gaddr, const source& saddr);

    void del_source(unsigned int if_index, const addr_storage& gaddr, const source& saddr);

    source_list<source> get_available_sources(unsigned int if_index, const addr_storage& gaddr, const source_list<source>& slist) const;

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const simple_routing_data& srd); 

    static void test_simple_routing_data();
};



#endif // SIMPLE_ROUTING_DATA_HPP
