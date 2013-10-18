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
#include "include/proxy/membership_db.hpp"
#include "include/proxy/message_format.hpp"

#include <iostream>
#include <set>
#include <map>
#include <chrono>
#include <memory>


struct gaddr_info {
    gaddr_info(group_mem_protocol compatibility_mode_variable);
    gaddr_info(const gaddr_info&) = default;
    gaddr_info& operator=(const gaddr_info&) = default;
    gaddr_info(gaddr_info&&) = default;
    gaddr_info& operator=(gaddr_info&&) = default;

    mc_filter filter_mode;

    group_mem_protocol compatibility_mode_variable; //RFC3810 - 8.3.2. In the Presence of MLDv1 Multicast Address Listeners

    std::shared_ptr<timer_msg> shared_filter_timer;

    std::shared_ptr<timer_msg> group_retransmission_timer;
    int group_retransmission_count;

    std::shared_ptr<timer_msg> source_retransmission_timer; //runs as long as a source in include_requested_list has an retransmission timer greater than zero

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
    membership_db(group_mem_protocol querier_version_mode);
    membership_db(const membership_db&) = default;
    membership_db& operator=(const membership_db&) = default;
    membership_db(membership_db&&) = default;
    membership_db& operator=(membership_db&&) = default;

    std::shared_ptr<timer_msg> general_query_timer;
    int startup_query_count;

    group_mem_protocol querier_version_mode; //RFC3810 - 8.3.1. In the Presence of MLDv1 Routers
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

