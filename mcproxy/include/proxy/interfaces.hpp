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

#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include "include/utils/if_prop.hpp"
#include "include/utils/reverse_path_filter.hpp"

#include <string>
#include <map>
#include <vector>
#include <sstream>

class addr_storage;

#define INTERFACES_UNKOWN_IF_INDEX 0
#define INTERFACES_UNKOWN_VIF_INDEX -1

/**
 * @brief summary of the most use interface properties
 */
class interfaces
{
private:
    int m_addr_family;

    //ipv4 only
    bool m_reset_reverse_path_filter;
    if_prop m_if_prop;
    reverse_path_filter m_reverse_path_filter;

    std::map<int, unsigned int> m_vif_if;
    std::map<unsigned int, int> m_if_vif;

    int get_free_vif_number() const;

    //flags example: IFF_UP IFF_LOOPBACK IFF_POINTOPOINT IFF_RUNNING IFF_ALLMULTI
    bool is_interface(unsigned if_index, unsigned int interface_flags) const;

public:
    interfaces(int addr_family, bool reset_reverse_path_filter);
    ~interfaces();

    bool refresh_network_interfaces();

    bool add_interface(const std::string& if_name);
    bool add_interface(unsigned int if_index);

    bool del_interface(const std::string& if_name);
    bool del_interface(unsigned int if_index);


    int get_virtual_if_index(unsigned int if_index) const;
    addr_storage get_saddr(const std::string& if_name) const;

    static std::string get_if_name(unsigned int if_index);

    static unsigned int get_if_index(const std::string& if_name);
    static unsigned int get_if_index(const char* if_name);
    unsigned int get_if_index(int virtual_if_index) const;

    //ipv4 only
    unsigned int get_if_index(const addr_storage& saddr) const;

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const interfaces& i);
};

#endif // INTERFACES_HPP
