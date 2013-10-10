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
#include "include/proxy/message_format.hpp"

#include <iostream>
#include <set>
#include <map>
#include <chrono>

//------------------------------------------------------------------------
template<typename T> using source_list = std::set<T>;

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

//------------------------------------------------------------------------
struct timer_msg : public proxy_msg {
    timer_msg(message_type type, unsigned int if_index, std::chrono::milliseconds duration): proxy_msg(type, SYSTEMIC), m_if_index(if_index) {
        HC_LOG_TRACE("");
        set_duration(duration);
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    std::string get_remaining_time() {
        using namespace std::chrono;
        std::ostringstream s;
        auto current_time = steady_clock::now();
        auto time_span = m_end_time - current_time;
        double seconds = time_span.count()  * steady_clock::period::num / steady_clock::period::den;
        s << seconds << "sec";
        return s.str();
    }

private:
    void set_duration(std::chrono::milliseconds duration) {
        m_end_time = std::chrono::steady_clock::now() + duration;
    }

    std::chrono::time_point<std::chrono::steady_clock> m_end_time;
    unsigned int m_if_index;
};


struct filter_timer : public timer_msg {
    filter_timer(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): filter_timer(FILTER_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }

    const addr_storage& get_gaddr() {
        return m_gaddr;
    }
protected:
    filter_timer(message_type type, unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration)
        : timer_msg(type
                    , if_index, duration)
        , m_gaddr(gaddr) {
        HC_LOG_TRACE("");
    }
private:
    addr_storage m_gaddr;
};

struct source_timer : public filter_timer {
    source_timer(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): filter_timer(SOURCE_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};

//------------------------------------------------------------------------
struct source {
    addr_storage saddr;

    mutable std::shared_ptr<source_timer> shared_source_timer;
    void* current_state;

    source();
    source(addr_storage a);

    std::string to_string() const;

    bool operator==(const source& s) const;

    friend std::ostream& operator<<(std::ostream& stream, const source& s);
    friend bool operator< (const source& s1, const source& s2);
};


struct gaddr_info {
    mc_filter filter_mode = INCLUDE_MODE;

    std::shared_ptr<filter_timer> shared_filter_timer;
    void* current_state;

    source_list<source> include_requested_list;
    source_list<source> exclude_list;

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const gaddr_info& g);
};

using gaddr_map = std::map<addr_storage, gaddr_info>;
using gaddr_pair = std::pair<addr_storage, gaddr_info>;

/**
 * @brief The Membership Database maintaines the membership records for one specific interface (RFC 4605)
 */
struct membership_db {
    group_mem_protocol compatibility_mode_variable; //RFC3810 - Section 6
    bool is_querier;
    gaddr_map group_info; //subscribed multicast group with there source lists


    static void test_arithmetic();

    std::string to_string() const;

    friend std::ostream& operator<<(std::ostream& stream, const membership_db& mdb);

private:
    std::string indention(std::string str) const;
};


#endif // MEMBERSHIP_DB_HPP
/** @} */

