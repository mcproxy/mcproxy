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
 * @defgroup mod_receiver Receiver
 * @brief The module Receiver uses the socket with the mrt-flag to receive Membership Reports
 * and Cache Miss messages. Cache Miss messages are sent from the Linux kernel in order to
 * inform proxies about new multicast sources.
 * @{
 */

#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "include/utils/mroute_socket.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/utils/if_prop.hpp"
#include "include/proxy/interfaces.hpp"

#include <map>
#include <thread>
#include <mutex>
#include <memory>

class proxy_instance;

/**
 * @brief Receive timout set to have not a blocking receive funktion.
 */
#define RECEIVER_RECV_TIMEOUT 100 //msec

//--------------------------------------------------
//             if_index, proxy_instance
/**
 * @brief Data structure to save the interface index with the incidental Proxy Instance.
 * @param first interface index
 * @param second pointer to the incidental Proxy Instance
 */
using if_poxy_instance_map = std::map<int, proxy_instance*>;

/**
 * @brief Pair for #if_poxy_instance_map.
 * @param first interface index
 * @param second pointer to the incidental Proxy Instance
 */
using if_proxy_instance_pair = std::pair<int, proxy_instance*>;

/**
 * @brief Abstract basic receiver class.
 */
class receiver
{
private:

    bool m_running;
    std::unique_ptr<std::thread> m_thread;
    void worker_thread();

    std::mutex m_data_lock;

    void start();
    void stop();
    void join();
protected:
    int m_addr_family;

    if_poxy_instance_map m_if_proxy_map;

    if_prop m_if_property;

    std::shared_ptr<mroute_socket> m_mrt_sock;
    
    std::shared_ptr<const interfaces> m_interfaces; 


    /**
     * @brief Get the size for the control buffer for recvmsg().
     */
    virtual int get_ctrl_min_size() = 0;

    /**
     * @brief Get the size for the iov vector for recvmsg().
     */
    virtual int get_iov_min_size() = 0;

    /**
     * @brief Analyze the received packet and send a message to the relevant proxy instance.
     * @param msg received message
     * @param info_size received information size
     */
    virtual void analyse_packet(struct msghdr* msg, int info_size) = 0;

    //return prody instance pointer and on error NULL
    /**
     * @brief Get the proxy instance pointer to the interface index. Search in #m_if_proxy_map.
     * @param if_index interface index
     * @return pointer of the proxy instance or NULL if not found
     */
    proxy_instance* get_proxy_instance(int if_index);

public:
    /**
      * @brief Create a receiver.
     */
    receiver(int addr_family, std::shared_ptr<mroute_socket> mrt_sock, std::shared_ptr<const interfaces> interfaces);

    /**
     * @brief Release all resources.
     */
    virtual ~receiver();


    /**
     * @brief Register an interface at the receiver.
     * @param if_index interface index of the registered interface
     * @param vif virtual interface indxe of the inteface
     * @param proxy_instance* who register the interface
     */
    void registrate_interface(int if_index, proxy_instance* p);

    /**
     * @brief Delete an registerd interface
     * @param if_index interface index of the interface
     * @param vif virtual interface index of the interface
     */
    void del_interface(int if_index);

    /**
     * @brief Check whether the receiver is running.
     */
    bool is_running();

};


#endif // RECEIVER_HPP
/** @} */
