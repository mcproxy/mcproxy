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
#include "include/proxy/timers_values.hpp"

#include <functional>
#include <string>
#include <memory>

class timing;
class sender;
class worker;
/**
 * @brief define the behaviour of a multicast querier for a specific interface
 */
class querier
{
private:
    worker* const m_msg_worker;
    int m_if_index;
    membership_db m_db;
    timers_values m_timers_values;

    const std::shared_ptr<const sender> m_sender;
    const std::shared_ptr<timing> m_timing;

    //join all router groups or leave them
    bool router_groups_function(std::function<bool(const sender&, int, addr_storage)> f) const;
    void receive_record_in_include_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, int report_version, gaddr_info& ginfo);
    void receive_record_in_exclude_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, int report_version, gaddr_info& ginfo);

    //updates the filter_timer
    void mali(const addr_storage& gaddr, gaddr_info& ginfo) const; //Multicast Address Listener Interval

    //updates a list of source_timers
    void mali(const addr_storage& gaddr, source_list<source>& slist) const; //Multicast Address Listener Interval

    //updates only specific source timers of a list
    void mali(const addr_storage& gaddr, source_list<source>& slist, source_list<source>&& tmp_slist) const; //Multicast Address Listener Interval

    //set only specific source timers to the corresponding filter time
    void filter_time(gaddr_info& ginfo, source_list<source>& slist, source_list<source>&& tmp_slist);

    //send multicast address specific query
    void send_Q(const addr_storage& gaddr, gaddr_info& ginfo);

    //send multicast address and source specific query
    void send_Q(const addr_storage& gaddr, gaddr_info& ginfo, source_list<source>& slist);

    //send multicast address and source specific and include only elements of tmp_list
    void send_Q(const addr_storage& gaddr, gaddr_info& ginfo, source_list<source>& slist, source_list<source>&& tmp_list);

    void timer_triggerd_filter_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_ret_filter_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_ret_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
public:

    virtual ~querier();

    querier(worker* msg_worker, group_mem_protocol querier_version_mode, int if_index, const std::shared_ptr<const sender>& sender, const std::shared_ptr<timing>& timing);

    void receive_record(const std::shared_ptr<proxy_msg>& msg);
    void timer_triggerd(const std::shared_ptr<proxy_msg>& msg);

    void receive_query();

    timers_values& get_timers_values();
    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const querier& q);

    /**
     * @brief Test the functionality of the querier.
     */
};

#endif // QUERIER_HPP
/** @} */
