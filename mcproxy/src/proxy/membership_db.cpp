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

#include "include/hamcast_logging.h"
#include "include/proxy/membership_db.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
using namespace std;

void membership_db::test_arithmetic()
{
    source_list<int> a;
    source_list<int> b;
    cout << "##-- membership_db test --##" << endl;
    const source_list<int> sl_a {0, 1, 2, 55};
    cout << "a: " << sl_a << endl;

    const source_list<int> sl_b {1, 2, 3, 55};
    cout << "b: " << sl_b << endl;

    cout << "doing: a += b" << endl;
    a = sl_a;
    b = sl_b;
    a += b;
    cout << "a: " << a << endl;
    cout << "b: " << b << endl;

    cout << "doing:  a + b" << endl;
    cout << "a + b: " << sl_a + sl_b << endl;
    cout << "b + a: " << sl_b + sl_a << endl;

    cout << "doing: a *= b" << endl;
    a = sl_a;
    b = sl_b;
    a *= b;
    cout << "a: " << a << endl;
    cout << "b: " << b << endl;

    cout << "doing:  a * b" << endl;
    cout << "a * b: " << sl_a * sl_b << endl;
    cout << "b * a: " << sl_b * sl_a << endl;

    cout << "doing: a -= b" << endl;
    a = sl_a;
    b = sl_b;
    a -= b;
    cout << "a: " << a << endl;
    cout << "b: " << b << endl;

    cout << "doing:  a - b" << endl;
    cout << "a - b: " << sl_a - sl_b << endl;
    cout << "b - a: " << sl_b - sl_a << endl;

    cout << "random suff" << endl;
    cout << "source_list<int>{1, 5, 2} - source_list<int>{2} - source_list<int>{5, 2}" << endl;
    cout << source_list<int> {1, 5, 2} - source_list<int> {2} - source_list<int> {5, 2}  << endl;

}

string source::to_string() const
{
    HC_LOG_TRACE("");
    ostringstream s;
    s << saddr << "(?timer?, ?current state?)";
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const source& s)
{
    HC_LOG_TRACE("");
    return stream << s.to_string();
}

std::ostream& operator<<(std::ostream& stream, const gaddr_info& g)
{
    HC_LOG_TRACE("");
    return stream << g.to_string();
}

bool operator< (const source& s1, const source& s2)
{
    HC_LOG_TRACE("");
    return s1.saddr < s2.saddr;
}

source::source(addr_storage a): saddr(a)
{
    HC_LOG_TRACE("");
}

source::source()
{
    HC_LOG_TRACE("");
}

bool source::operator==(const source& s) const
{
    HC_LOG_TRACE("");
    return this->saddr == s.saddr;
}

std::string gaddr_info::to_string() const
{
    HC_LOG_TRACE("");
    ostringstream s;
    s << "filter mode: " << mc_filter_name[filter_mode] << endl;
    s << "filter timer: " << "?filter_timer?" << endl;
    s << "current state: " << "?current_state?" << endl;
    s << "included list: " << include_list << endl;
    s << "requested list: " << requested_list << endl;
    s << "exclude_list: " << exclude_list;

    s << "&included list: " << &include_list << endl;
    s << "&requested list: " << &requested_list << endl;
    return s.str();
}

std::string membership_db::to_string() const
{
    HC_LOG_TRACE("");
    ostringstream s;
    s << "compatibility mode variable: " << group_mem_protocol_name[compatibility_mode_variable] << endl;
    s << "is querier: " << (is_querier ? "true" : "false") << endl;
    s << "subscribed groups: " << group_info.size();

    for (auto & e : group_info) {
        s << endl << "-- group address: " << e.first << endl;
        s << indention(e.second.to_string());
    }

    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const membership_db& mdb)
{
    HC_LOG_TRACE("");
    return stream << mdb.to_string();
}

std::string membership_db::indention(std::string str) const
{
    HC_LOG_TRACE("");
    unsigned long cpos = 0;

    cpos = str.find("\n", cpos);
    while ((cpos < (str.size() - 2)) && (cpos != std::string::npos)) {
        str.insert(cpos + 1, "\t");
        cpos = str.find("\n", cpos + 1);
    }

    str.insert(0,"\t");

    return str;
}

