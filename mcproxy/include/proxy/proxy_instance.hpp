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

#include <memory>
#include <vector>
#include <functional>

class querier;
class interfaces;
class timing;
class receiver;
class sender;
class routing;
class mroute_socket;
class routing_management;

/**
 * @brief Represent a multicast Proxy
 */
class proxy_instance: public worker
{
private:
    const group_mem_protocol m_group_mem_protocol; //AF_INET or AF_INET6
    const int m_table_number; // 0 means no table number
    const bool m_in_debug_testing_mode;


    //check_source m_check_source;

    const std::shared_ptr<const interfaces> m_interfaces;
    const std::shared_ptr<timing> m_timing;

    std::shared_ptr<mroute_socket> m_mrt_sock;
    std::shared_ptr<sender> m_sender;

    std::unique_ptr<receiver> m_receiver;
    std::unique_ptr<routing> m_routing;
    std::unique_ptr<routing_management> m_routing_management;

    unsigned int m_upstream;

    //if_index, querier
    std::map<unsigned int, std::unique_ptr<querier>> m_querier;

    //init
    bool init_mrt_socket();
    bool init_sender();
    bool init_receiver();
    bool init_routing();

    void worker_thread();

    //add and del interfaces
    void handle_config(const std::shared_ptr<config_msg>& msg);

    std::string to_string() const; 
    friend std::ostream& operator<<(std::ostream& stream, const proxy_instance& pr_i);
public:
    /**
     * @brief Set default values of the class members.
     */
    proxy_instance(group_mem_protocol group_mem_protocol, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing, bool in_debug_testing_mode = false);

    /**
     * @brief Release all resources.
     */
    virtual ~proxy_instance();

    static void test_querier(std::string if_name);

    static void test_a(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   
    static void test_b(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   
    static void test_c(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   
    static void test_d(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   

    static void quick_test(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   
    static void rand_test(std::function<void(mcast_addr_record_type,source_list<source>&&)> send_record, std::function<void()> print_proxy_instance);   

    friend routing_management;
};

#endif // PROXY_INSTANCE_HPP
/** @}*/
