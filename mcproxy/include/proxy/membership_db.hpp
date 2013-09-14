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
 * @addtogroup mod_proxy Proxy
 * @{
 */

#ifndef MEMBERSHIP_DB_HPP
#define MEMBERSHIP_DB_HPP

#include "include/utils/addr_storage.hpp"
#include "include/proxy/def.hpp"
#include <iostream>
#include <set>
#include <map>

//------------------------------------------------------------------------
template<typename T> using source_list = std::set<T>;

//A+B means the union of set A and B
template<typename T>
inline source_list<T>& operator+=(source_list<T>& l, const source_list<T>& r)
{
    using namespace std;
    l.insert(r.cbegin(), r.cend());
    return l;
}

template<typename T>
inline source_list<T> operator+(const source_list<T>& l, const source_list<T>& r)
{
    using namespace std;
    source_list<T> new_sl(l);
    new_sl += r;
    return new_sl;
}

//A*B means the intersection of set A and B
template<typename T>
inline source_list<T>& operator*=(source_list<T>& l, const source_list<T>& r)
{
    using namespace std;
    auto cur_l = begin(l);
    auto cur_r = begin(r);

    while (cur_l != end(l) && cur_r != end(r)) {
        if (*cur_l == *cur_r) {
            cur_l++;
            cur_r++;
        } else if (*cur_l < *cur_r) {
            cur_l = l.erase(cur_l);
        } else { //cur_l > cur_r
            cur_r++;
        }
    }

    if (cur_l != end(l)) {
        l.erase(cur_l, end(l));
    }

    return l;
}

template<typename T>
inline source_list<T> operator*(const source_list<T>& l, const source_list<T>& r)
{
    using namespace std;
    source_list<T> new_sl(l);
    new_sl *= r;
    return new_sl;
}

//A-B means the removal of all elements of set B from set A
template<typename T>
inline source_list<T>& operator-=(source_list<T>& l, const source_list<T>& r)
{
    using namespace std;

    for (auto e : r) {
        l.erase(e);
    }

    return l;
}

template<typename T>
inline source_list<T> operator-(const source_list<T>& l, const source_list<T>& r)
{
    using namespace std;
    source_list<T> new_sl(l);
    new_sl -= r;
    return new_sl;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, const source_list<T> sl)
{
    using namespace std;
    for (auto e : sl) {
        stream << e << " ";
    }    
    return stream;
}

//------------------------------------------------------------------------
struct source {
    addr_storage saddr;
    void* source_timer;
    void* current_state;
};

struct group_info{
    mc_filter filter_mode; 
    void* filter_timer;
    void* current_state;
    source_list<source> include_list;
    source_list<source>& requested_list = include_list;
    source_list<source> exclude_list;
};


/**
 * @brief The Membership Database maintaines the membership records for one specific interface (RFC 4605)
 */
struct membership_db
{
    group_mem_protocol compatibility_mode_variable; //RFC3810 - Section 6
    bool is_querier; 
    std::map<addr_storage,group_info> gaddr_map; //subscribed multicast group with there source lists  

    static void test_arithmetic();

};


#endif // MEMBERSHIP_DB_HPP
/** @} */

