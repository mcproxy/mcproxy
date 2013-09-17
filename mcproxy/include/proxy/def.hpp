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

#ifndef DEF_HPP
#define DEF_HPP

#include <netinet/in.h>

#include <map>
#include <string>

enum mc_filter {INCLUDE_MODE = MCAST_INCLUDE, EXLCUDE_MODE = MCAST_EXCLUDE};
static std::map<mc_filter, std::string> mc_filter_name = {
    {INCLUDE_MODE, "INCLUDE_MODE"},
    {EXLCUDE_MODE, "EXLCUDE_MODE"}
};

enum group_mem_protocol {IGMPv1, IGMPv2, IGMPv3, MLDv1, MLDv2};
static std::map<group_mem_protocol, std::string> group_mem_protocol_name = {
    {IGMPv1, "IGMPv1"},
    {IGMPv2, "IGMPv2"},
    {IGMPv3, "IGMPv3"},
    {MLDv1, "MLDv1"},
    {MLDv2, "MLDv2"}
};

enum mcast_addr_record_type {MODE_IS_INCLUDE = 1, MODE_IS_EXCLUDE = 2, CHANGE_TO_INCLUDE_MODE = 3, CHANGE_TO_EXCLUDE_MODE = 4, ALLOW_NEW_SOURCES = 5, BLOCK_OLD_SOURCES = 6};
static std::map<mcast_addr_record_type, std::string> mcast_addr_record_type_name = {
    {MODE_IS_INCLUDE, "MODE_IS_INCLUDE"},
    {MODE_IS_EXCLUDE, "MODE_IS_EXCLUDE"},
    {CHANGE_TO_INCLUDE_MODE, "CHANGE_TO_INCLUDE_MODE"},
    {CHANGE_TO_EXCLUDE_MODE, "CHANGE_TO_EXCLUDE_MODE"},
    {ALLOW_NEW_SOURCES, "ALLOW_NEW_SOURCES"},
    {BLOCK_OLD_SOURCES, "BLOCK_OLD_SOURCES"}
};

#endif //DEF_HPP

