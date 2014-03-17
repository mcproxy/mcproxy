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

#ifndef IF_PROP_NEW_HPP
#define IF_PROP_NEW_HPP

#include <string>
#include <list>
#include <map>
#include <ifaddrs.h>

//typedef pair<struct ifaddrs*, list<struct ifaddrs*> > ipv4_6_pair;

struct ipv4_6_pair {
    const struct ifaddrs* ip4_addr;
    std::list<const struct ifaddrs*> ip6_addr;

    ipv4_6_pair(const struct ifaddrs* ip4_addr, std::list<const struct ifaddrs*> ip6_addr) {
        this->ip4_addr = ip4_addr;
        this->ip6_addr = ip6_addr;
    }
};

//map<if_name, <ipv4 struct, ipv6 struct list> >
typedef std::map<std::string, ipv4_6_pair > if_prop_map;
typedef std::pair<std::string, ipv4_6_pair >if_prop_pair;

/**
 * @brief Prepare and organized the interface properties to a map data structure.
 */
class if_prop
{
private:
    if_prop_map m_if_map;
    struct ifaddrs* m_if_addrs;

public:
    /**
     * @brief Create the class if_prop.
     */
    if_prop();

    /**
     * @brief Return the whole data structure.
     */
    const if_prop_map * get_if_props() const;

    /**
     * @brief Refresh all information of all interfaces.
     * @return Return true on success.
     */
    bool refresh_network_interfaces();

    /**
     * @brief Get the ipv4 interface properties for a specific interface name.
     */
    const struct ifaddrs* get_ip4_if(const std::string& if_name) const;

    /**
     * @brief Get the ipv6 interface properties for a specific interface name.
     */
    const std::list<const struct ifaddrs* >* get_ip6_if(const std::string& if_name) const;

    /**
     * @brief Release all allocated resources.
     */
    virtual ~if_prop();

    /**
     * @brief Check for a valid data structure.
     */
    bool is_getaddrs_valid() const {
        return m_if_addrs != 0;
    }

    /**
     * @brief Print all available network interface information.
     */
    static void print_if_info(if_prop* p);

    /**
     * @brief Print interface information for a specific interface.
     * @param if_p interface properties for a specific inerface.
     */
    static void print_if_addr(const struct ifaddrs* if_p);

    /**
     * @brief Test the class if_prop.
     */
    static void test_if_prop();
};

#endif // IF_PROP_NEW_HPP
