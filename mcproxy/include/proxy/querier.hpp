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

#ifndef QUERIER_HPP
#define QUERIER_HPP

#include "include/proxy/membership_db.hpp"
#include "include/proxy/timers_values.hpp"

#include <functional>
#include <string>
#include <memory>
#include <functional>

class timing;
class sender;
class worker;

/**
 * @brief Callback function to publish querier state change informations.
 * The callback function informs about the involved interface index, group address and the involved multicast sources.
 */
typedef std::function<void(unsigned int, const addr_storage&)> callback_querier_state_change;

/**
 * @brief Defines the behaviour of a multicast querier for a specific interface.
 */
class querier
{
private:
    worker* const m_msg_worker;
    const unsigned int m_if_index;
    membership_db m_db;
    timers_values m_timers_values;
    callback_querier_state_change m_cb_state_change;

    const std::shared_ptr<const sender> m_sender;
    const std::shared_ptr<timing> m_timing;

    //join all router groups or leave them
    bool router_groups_function(bool subscribe) const;
    bool send_general_query();

    //
    void receive_record_in_include_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, gaddr_info& ginfo);
    void receive_record_in_exclude_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, gaddr_info& ginfo);

    //RFC3810 Section 7.2.3 Definition of Souce timers
    //Updates the filter_timer to the Multicast Address Listener Interval
    void mali(const addr_storage& gaddr, gaddr_info& ginfo) const;

    //Updates a list of source_timers to the Multicast Address Listener Interval
    void mali(const addr_storage& gaddr, source_list<source>& slist) const;

    //Updates specific source timers (tmp_slist) of list slist to the Multicast Address Listener Interval
    void mali(const addr_storage& gaddr, source_list<source>& slist, source_list<source>&& tmp_slist) const;

    //Set specific source timers (tmp_slist) of list slist to the corresponding filter time
    void filter_time(gaddr_info& ginfo, source_list<source>& slist, source_list<source>&& tmp_slist);

    //send multicast address specific query
    void send_Q(const addr_storage& gaddr, gaddr_info& ginfo);

    //send multicast address and source specific and include only elements of tmp_list
    void send_Q(const addr_storage& gaddr, gaddr_info& ginfo, source_list<source>& slist, source_list<source>&& tmp_list, bool in_retransmission_state = false);

    void timer_triggerd_filter_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_ret_group_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_ret_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_older_host_present_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg);
    void timer_triggerd_general_query_timer(const std::shared_ptr<timer_msg>& msg);

    //call the callback function querier_state_change
    void state_change_notification(const addr_storage& gaddr);

public:
    virtual ~querier();

    /**
     * @param msg_worker Saves the message worker which receives all querier specific timer events and forward them back to this querier.
     * @param querier_version_mode Defines the highest group membership protocol version for IPv4 or Ipv6 to use.
     * @param if_index Interface index of the querier.
     * @param sender For sending queriers and subscribing router specific groups.
     * @param shared_timing Stores and triggers all time-dependent events for this querier.
     * @param tv contain all nessesary timers and values.
     * @param cb_state_change Callback function to publish querier state change informations.
     */
    querier(worker* msg_worker, group_mem_protocol querier_version_mode, int if_index, const std::shared_ptr<const sender>& sender, const std::shared_ptr<timing>& timing, const timers_values& tv, callback_querier_state_change cb_state_change);

    /**
     * @brief All received group records of the interface maintained by this querier musst be submitted to this function. 
     * @param msg the reveived group record
     */
    void receive_record(const std::shared_ptr<proxy_msg>& msg);

    /**
     * @brief all timer events orderd by this querier musst be submitted to this function. 
     * @param msg the timer event 
     */
    void timer_triggerd(const std::shared_ptr<proxy_msg>& msg);

    //bool suggest_to_forward_traffic(const addr_storage& gaddr, const addr_storage& saddr, mc_filter* filter_mode = nullptr, source_list<source>* slist = nullptr) const; //4.2.  Per-Interface State (merge your own multicast state)
    /**
     * @brief RFC 3810 Section 7.3. MLDv2 Source Specific Forwarding Rules 
     * A querier can make suggestions to forward traffic to its maintained interface. 
     * @param gaddr make suggestion for traffic send to this group address.
     * @param rt_slist contains a list of sources for the suggestions and return list with the suggestions
     * @param interface_filter_fun If the filter function is false the interface will be not added to rt_slist
     * If the querier suggest to forward traffic of the group address gaddr and the source it adds its own interface to the return list.
     */
    void suggest_to_forward_traffic(const addr_storage& gaddr, std::list<std::pair<source, std::list<unsigned int>>>& rt_slist, std::function<bool(const addr_storage&)> interface_filter_fun) const;

    /**
     * @return return all group membership information of group address gaddr
     */
    std::pair<mc_filter, source_list<source>> get_group_membership_infos(const addr_storage& gaddr);

    /**
     * @brief Roadworks
     */
    void receive_query();

    /**
     * @return return the timers and counter values for a modification
     */
    timers_values& get_timers_values();

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const querier& q);
};

#endif // QUERIER_HPP
