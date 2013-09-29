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

#include "include/utils/addr_storage.hpp"
#include "include/utils/if_prop.hpp"
#include <string>
#include <map>

#define INTERFACES_UNKOWN_IF_INDEX 0
#define INTERFACES_UNKOWN_VIF_INDEX -1

class interfaces
{
private:
    int m_addr_family;
    if_prop m_if_prop;

    std::map<int, int> m_vif_if;
    std::map<int, int> m_if_vif;
public:
    interfaces(int addr_family);
    ~interfaces();

    bool refresh_network_interfaces();

    bool add_interface(const std::string& if_name);
    bool add_interface(unsigned int if_index);

    bool del_interface(const std::string& if_name);
    bool del_interface(unsigned int if_index);


    int get_virtual_if_index(unsigned int if_index) const;

    std::string get_if_name(unsigned int if_index) const;

    unsigned int get_if_index(const std::string& if_name) const;
    unsigned int get_if_index(const char* if_name) const;
    unsigned int get_if_index(int virtual_if_index) const;
    unsigned int get_if_index(const addr_storage& addr) const;

    int get_free_vif_number() const;
};


#endif // INTERFACES_HPP
/** @} */
















































































