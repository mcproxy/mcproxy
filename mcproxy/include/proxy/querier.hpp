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

#ifndef QUERIER_HPP
#define QUERIER_HPP

#include "include/proxy/membership_db.hpp"
#include "include/proxy/sender.hpp"

#include <functional>
#include <string>
#include <memory>
/**
 * @brief define the behaviour of a multicast querier for a specific interface
 */
class querier 
{
private:
    int m_addr_family;
    int m_if_index;
    membership_db m_db;

    std::shared_ptr<sender> m_sender;

    bool init_db();

    //join all router groups or leave them
    bool router_groups_function(function<bool(std::shared_ptr<sender>, int,addr_storage)> f);
    void receive_record_in_include_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& saddr_list, int report_version, gaddr_info& db_info);
    void receive_record_in_exclude_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& saddr_list, int report_version, gaddr_info& db_info);
public:

    virtual ~querier();

    querier(int addr_family, int if_index, std::shared_ptr<sender> sender);

    void receive_record(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& saddr_list, int report_version);
    void receive_query(); 
    void timer_triggerd();
   
    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const querier& q);

    /**
     * @brief Test the functionality of the querier.
     */
    static void test_querier(int addr_family, string if_name);
    static void send_test_record(querier& q, mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>&& saddr_list, int report_version);
};

#endif // QUERIER_HPP
/** @} */
