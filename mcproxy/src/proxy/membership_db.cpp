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

#include "include/proxy/membership_db.hpp"
#include "include/hamcast_logging.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

#ifdef DEBUG_MODE
void membership_db::test_arithmetic()
{
    using namespace std;
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
#endif /* DEBUG_MODE */

gaddr_info::gaddr_info(group_mem_protocol compatibility_mode_variable)
    : filter_mode(INCLUDE_MODE)
    , shared_filter_timer(nullptr)
    , compatibility_mode_variable(compatibility_mode_variable)
    , older_host_present_timer(nullptr)  
    , group_retransmission_timer(nullptr)
    , group_retransmission_count(-1) //not in a retransmission state
    , source_retransmission_timer(nullptr)
{
    HC_LOG_TRACE("");
}


bool gaddr_info::is_in_backward_compatibility_mode() const{
    return !is_newest_version(compatibility_mode_variable);
}

bool gaddr_info::is_under_bakcward_compatibility_effects() const{
    return older_host_present_timer.get() != nullptr;        
}

std::ostream& operator<<(std::ostream& stream, const gaddr_info& g)
{
    return stream << g.to_string();
}

std::string gaddr_info::to_string() const
{
    using namespace std;
    ostringstream s;
    s << get_group_mem_protocol_name(compatibility_mode_variable);
    if (older_host_present_timer != nullptr){
        s << "(" << older_host_present_timer->get_remaining_time() << ")";    
    }

    s << ", " << get_mc_filter_name(filter_mode);
    if ((filter_mode == EXCLUDE_MODE) && (group_retransmission_timer.get() != nullptr)) {
        s << "(" << shared_filter_timer->get_remaining_time() << "," << group_retransmission_timer->get_remaining_time() << "," << group_retransmission_count << "x)" << endl;
    } else if (filter_mode == EXCLUDE_MODE) {
        s << "(" << shared_filter_timer->get_remaining_time() << ")" << endl;
    }else{
        s << endl; 
    }

    if (source_retransmission_timer.get() != nullptr) {
        if (filter_mode == INCLUDE_MODE) {
            s << "included list(" << source_retransmission_timer->get_remaining_time() << ", #" << include_requested_list.size() << "): " << include_requested_list << endl;
        } else if (filter_mode == EXCLUDE_MODE) {
            s << "requested list(" << source_retransmission_timer->get_remaining_time() << ", #" << include_requested_list.size() << "): " << include_requested_list << endl;
            s << "exclude_list(" << source_retransmission_timer->get_remaining_time() << ", #" << exclude_list.size() << "): " << exclude_list;
        } else {
            HC_LOG_ERROR("unknown filter mode");
        }
    } else {
        if (filter_mode == INCLUDE_MODE) {
            s << "included list(#" << include_requested_list.size() << "): " << include_requested_list << endl;
        } else if (filter_mode == EXCLUDE_MODE) {
            s << "requested list(#" << include_requested_list.size() << "): " << include_requested_list << endl;
            s << "exclude_list(#" << exclude_list.size() << "): " << exclude_list;
        } else {
            HC_LOG_ERROR("unknown filter mode");
        }
    }
    return s.str();
}

membership_db::membership_db(group_mem_protocol querier_version_mode)
    : general_query_timer(nullptr)
    , startup_query_count(0)
    , querier_version_mode(querier_version_mode)
    , is_querier(true)

{
    HC_LOG_TRACE("");
}

std::string membership_db::to_string() const
{
    using namespace std;
    ostringstream s;
    s << "querier version: " << get_group_mem_protocol_name(querier_version_mode) << endl;
    s << "is querier: " << (is_querier ? "true" : "false") << endl;
    if (general_query_timer.get() != nullptr) {
        s << "general query timer: " << general_query_timer->get_remaining_time() << endl;
    }
    s << "startup query count: " << startup_query_count << endl;

    s << "subscribed groups: " << group_info.size();
    for (auto & e : group_info) {
        s << endl << "-- group address: " << e.first << endl;
        s << indention(e.second.to_string());
    }

    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const membership_db& mdb)
{
    return stream << mdb.to_string();
}


