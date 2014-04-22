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
#include <set>
#include <string>
#include <iostream>
#include <chrono>

//------------------------------------------------------------------------
enum mc_filter {INCLUDE_MODE = MCAST_INCLUDE, EXCLUDE_MODE = MCAST_EXCLUDE};
std::string get_mc_filter_name(mc_filter mf);

enum group_mem_protocol {IGMPv1 = 0x1, IGMPv2 = 0x2, IGMPv3 = 0x4, MLDv1 = 0x8, MLDv2 = 0x10};
bool is_IPv4(group_mem_protocol gmp);
bool is_IPv6(group_mem_protocol gmp);
bool is_older_or_equal_version(group_mem_protocol older, group_mem_protocol comp_to);
bool is_newest_version(group_mem_protocol gmp);
group_mem_protocol get_next_newer_version(group_mem_protocol gmp);
int get_addr_family(group_mem_protocol gmp);
std::string get_group_mem_protocol_name(group_mem_protocol gmp);

enum mcast_addr_record_type {MODE_IS_INCLUDE = 1, MODE_IS_EXCLUDE = 2, CHANGE_TO_INCLUDE_MODE = 3, CHANGE_TO_EXCLUDE_MODE = 4, ALLOW_NEW_SOURCES = 5, BLOCK_OLD_SOURCES = 6};
std::string get_mcast_addr_record_type_name(mcast_addr_record_type art);

//------------------------------------------------------------------------
std::string time_to_string(const std::chrono::seconds& sec);
std::string time_to_string(const std::chrono::milliseconds& msec);

//------------------------------------------------------------------------
std::string indention(std::string str);
//------------------------------------------------------------------------
#define source_list std::set

//A+B means the union of set A and B
template<typename T>
inline source_list<T>& operator+=(source_list<T>& l, const source_list<T>& r)
{
    l.insert(r.cbegin(), r.cend());
    return l;
}

template<typename T>
inline source_list<T> operator+(const source_list<T>& l, const source_list<T>& r)
{
    source_list<T> new_sl(l);
    new_sl += r;
    return new_sl;
}

//A*B means the intersection of set A and B
template<typename T>
inline source_list<T>& operator*=(source_list<T>& l, const source_list<T>& r)
{
    auto cur_l = std::begin(l);
    auto cur_r = std::begin(r);

    while (cur_l != std::end(l) && cur_r != std::end(r)) {
        if (*cur_l == *cur_r) {
            cur_l++;
            cur_r++;
        } else if (*cur_l < *cur_r) {
            cur_l = l.erase(cur_l);
        } else { //cur_l > cur_r
            cur_r++;
        }
    }

    if (cur_l != std::end(l)) {
        l.erase(cur_l, std::end(l));
    }

    return l;
}

template<typename T>
inline source_list<T> operator*(const source_list<T>& l, const source_list<T>& r)
{
    source_list<T> new_sl(l);
    new_sl *= r;
    return new_sl;
}

//A-B means the removal of all elements of set B from set A
template<typename T>
inline source_list<T>& operator-=(source_list<T>& l, const source_list<T>& r)
{
    for (auto e : r) {
        l.erase(e);
    }

    return l;
}

template<typename T>
inline source_list<T> operator-(const source_list<T>& l, const source_list<T>& r)
{
    source_list<T> new_sl(l);
    new_sl -= r;
    return new_sl;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, const source_list<T> sl)
{
    int i = 1;
    for (auto e : sl) {
        if (i % 3 == 0 ) {
            stream << std::endl << "\t";
        }
        stream << e << "; ";
        i++;
    }
    return stream;
}

#endif //DEF_HPP
