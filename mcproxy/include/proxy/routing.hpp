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

/**
 * @defgroup mod_routing Routing
 * @brief The module Routing set/delete virtual interfaces and multicast forwarding rules.
 * @{
 */

#ifndef ROUTING_HPP
#define ROUTING_HPP

//#include "include/utils/mroute_socket.hpp"
#include "include/utils/if_prop.hpp"

#include <set>
#include <list>
#include <memory>

class interfaces;
class mroute_socket;
class addr_storage;

/**
 * @brief Set and delete virtual interfaces and forwarding rules in the Linux kernel.
 */
class routing
{
private:
    int m_table_number;
    int m_addr_family; //AF_INET or AF_INET6

    const std::shared_ptr<const interfaces> m_interfaces;
    const std::shared_ptr<const mroute_socket> m_mrt_sock;
    if_prop m_if_prop; //return interface properties

    mutable std::set<unsigned int> m_added_ifs; 

public:
    routing(int addr_family, std::shared_ptr<const mroute_socket> mrt_sock, std::shared_ptr<const interfaces> interfaces, int table_number);

    virtual ~routing();
    /**
      * @brief Add a virtual interface to the linux kernel table.
      * @return Return true on success.
      */
    bool add_vif(int if_index, int vif) const;

    /**
      * @brief Delete a virtual interface from the linux kernel table.
      * @return Return true on success.
      */
    bool del_vif(int if_index, int vif) const;

    /**
      * @brief Add a multicast route to the linux kernel table.
      * @return Return true on success.
      */
    bool add_route(int input_vif, const addr_storage& g_addr, const addr_storage& src_addr, const std::list<int>& output_vif) const;

    /**
      * @brief Delete a multicast route from the linux kernel table.
      * @return Return true on success.
      */
    bool del_route(int vif, const addr_storage& g_addr, const addr_storage& src_addr) const;
};

#endif // ROUTING_HPP
/** @} */
