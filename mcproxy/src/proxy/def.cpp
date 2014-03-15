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
#include "include/proxy/def.hpp"

#include <chrono>
#include <sstream>

bool is_IPv4(group_mem_protocol gmp)
{
    return (IGMPv1 | IGMPv2 | IGMPv3) & gmp;
}

bool is_IPv6(group_mem_protocol gmp)
{
    return (MLDv1 | MLDv2) & gmp;
}

bool is_older_or_equal_version(group_mem_protocol older, group_mem_protocol comp_to)
{
    return older <= comp_to;
}

bool is_newest_version(group_mem_protocol gmp)
{
    if (gmp == IGMPv3 || gmp == MLDv2) {
        return true;
    } else {
        return false;
    }
}

int get_addr_family(group_mem_protocol gmp)
{
    if (is_IPv4(gmp)) {
        return AF_INET;
    } else if (is_IPv6(gmp)) {
        return  AF_INET6;
    } else {
        return AF_UNSPEC;
    }
}

group_mem_protocol get_next_newer_version(group_mem_protocol gmp)
{
    if (is_newest_version(gmp)) {
        return gmp;
    } else {
        return static_cast<group_mem_protocol>(gmp * 2);
    }
}

std::string get_mc_filter_name(mc_filter mf)
{
    std::map<mc_filter, std::string> name_map = {
        {INCLUDE_MODE, "INCLUDE_MODE"},
        {EXCLUDE_MODE, "EXCLUDE_MODE"}
    };
    return name_map[mf];
}

std::string get_group_mem_protocol_name(group_mem_protocol gmp)
{
    std::map<group_mem_protocol, std::string> name_map = {
        {IGMPv1,          "IGMPv1"},
        {IGMPv2,          "IGMPv2"},
        {IGMPv3,          "IGMPv3"},
        {MLDv1,           "MLDv1" },
        {MLDv2,           "MLDv2" }
    };
    return name_map[gmp];
}

std::string get_mcast_addr_record_type_name(mcast_addr_record_type art)
{
    std::map<mcast_addr_record_type, std::string> name_map = {
        {MODE_IS_INCLUDE,        "MODE_IS_INCLUDE"       },
        {MODE_IS_EXCLUDE,        "MODE_IS_EXCLUDE"       },
        {CHANGE_TO_INCLUDE_MODE, "CHANGE_TO_INCLUDE_MODE"},
        {CHANGE_TO_EXCLUDE_MODE, "CHANGE_TO_EXCLUDE_MODE"},
        {ALLOW_NEW_SOURCES,      "ALLOW_NEW_SOURCES"     },
        {BLOCK_OLD_SOURCES,      "BLOCK_OLD_SOURCES"     }
    };
    return name_map[art];
}

std::string time_to_string(const std::chrono::seconds& sec)
{
    std::ostringstream s;
    s << sec.count() << " sec";
    return s.str();
}

std::string time_to_string(const std::chrono::milliseconds& msec)
{
    std::ostringstream s;
    s << msec.count() << " msec";
    return s.str();
}

std::string indention(std::string str)
{
    unsigned long cpos = 0;

    cpos = str.find("\n", cpos);
    while ((cpos < (str.size() - 1)) && (cpos != std::string::npos)) {
        str.insert(cpos + 1, "\t");
        cpos = str.find("\n", cpos + 1);
    }

    str.insert(0, "\t");

    return str;
}
