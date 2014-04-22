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
 * @defgroup mod_proxy_instance Proxy Instance
 * @brief A Proxy Instance represents a full multicast proxy. Each
 *        instance has access to the modules Receiver, Routing, Sender and Timer.
 * @{
 */

#ifndef PROXY_INSTANCE_HPP
#define PROXY_INSTANCE_HPP

#include "include/proxy/worker.hpp"
#include "include/proxy/def.hpp"
#include "include/proxy/querier.hpp"
#include "include/parser/interface.hpp"

#include <memory>
#include <set>
#include <functional>

class timing;
class receiver;
class sender;
class routing;
class mroute_socket;
class interface;
class simple_mc_proxy_routing;
class routing_management;
class interface_memberships;

/**
 * @brief Represent a multicast proxy (RFC 4605)
 */
class proxy_instance: public worker
{
private:
    struct downstream_infos {
        downstream_infos(std::unique_ptr<querier> querier, const std::shared_ptr<interface>& interf)
            : m_querier(std::move(querier))
            , m_interface(interf) {}

        std::unique_ptr<querier> m_querier;
        std::shared_ptr<interface> m_interface;
    };

    struct upstream_infos {
        upstream_infos(unsigned int if_index, const std::shared_ptr<interface>& interf, unsigned int priority)
            : m_if_index(if_index)
            , m_interface(interf) 
            , m_priority(priority) {}

        unsigned int m_if_index;
        std::shared_ptr<interface> m_interface;
        unsigned int m_priority;
            
        friend bool operator < (const upstream_infos& l, const upstream_infos& r) {
            HC_LOG_TRACE("");
            return l.m_priority < r.m_priority;
        }
    };

    //IGMPv1, IGMPv2, IGMPv3, MLDv1, MLDv2
    const group_mem_protocol m_group_mem_protocol;

    //defines the mulitcast routing talbe, if set to 0 (default routing table) no other instances running on the system to simplifie the kernel calls.
    const std::string m_instance_name;
    const int m_table_number;
    const bool m_in_debug_testing_mode;

    const std::shared_ptr<const interfaces> m_interfaces;
    const std::shared_ptr<timing> m_timing;

    std::shared_ptr<mroute_socket> m_mrt_sock;
    std::shared_ptr<sender> m_sender;

    std::unique_ptr<receiver> m_receiver;
    std::unique_ptr<routing> m_routing;
    std::unique_ptr<routing_management> m_routing_management;

    //to match the proxy debug output with the wireshark time stamp
    const std::chrono::time_point<std::chrono::monotonic_clock> m_proxy_start_time;

    std::set<upstream_infos> m_upstreams;

    //if_indexes of the downstreams, querier
    //std::map<unsigned int, std::unique_ptr<querier>> m_querier;
    std::map<unsigned int, downstream_infos> m_downstreams;

    std::shared_ptr<rule_binding> m_upstream_input_rule;
    std::shared_ptr<rule_binding> m_upstream_output_rule;

    //init
    bool init_mrt_socket();
    bool init_sender();
    bool init_receiver();
    bool init_routing();
    bool init_routing_management();

    //receives and process all events
    void worker_thread();

    //add and del interfaces
    void handle_config(const std::shared_ptr<config_msg>& msg);

    bool is_upstream(unsigned int if_index) const;
    bool is_downstream(unsigned int if_index) const;

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const proxy_instance& pr_i);

public:
    /**
     * @param group_mem_protocol Defines the highest group membership protocol version for IPv4 or Ipv6 to use.
     * @param table_number Set the multicast routing table. If set to 0 (default routing table) no other instances running on the system (this simplifie the kernel calls).
     * @param interfaces Holds all possible needed information of all upstream and downstream interfaces.
     * @param shared_timing Stores and triggers all time-dependent events for this proxy instance.
     * @param in_debug_testing_mode If true this proxy instance stops receiving group membership messages and prints a lot of status messages to the command line.
     */
    proxy_instance(group_mem_protocol group_mem_protocol, const std::string& intance_name, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing, bool in_debug_testing_mode = false);

    /**
     * @brief Release all resources.
     */
    virtual ~proxy_instance();

    static void test_querier(std::string if_name);

    static void test_a(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);
    static void test_b(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);
    static void test_c(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);
    static void test_d(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);
    static void test_backward_compatibility(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);

    static void quick_test(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);
    static void rand_test(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance);

    friend class routing_management;
    friend class simple_mc_proxy_routing;
    friend class interface_memberships;
};

#endif // PROXY_INSTANCE_HPP
/** @}*/
