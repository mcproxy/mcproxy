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
#include "include/proxy/interfaces.hpp"
#include "include/proxy/message_format.hpp"
#include "include/proxy/def.hpp"

#include <set>
#include <thread>
#include <mutex>
#include <memory>
#include <sstream>

class proxy_instance;

/**
 * @brief Receive timout set to have not a blocking receive funktion.
 */
#define RECEIVER_RECV_TIMEOUT 100 //msec

/**
 * @brief Abstract basic receiver class.
 */
class receiver
{
private:

    bool m_running;
    bool m_in_debug_testing_mode;
    std::unique_ptr<std::thread> m_thread;

    std::set<unsigned int> m_relevant_if_index;

    void worker_thread();

    std::mutex m_data_lock;

    void stop();
    void join();

protected:
    const proxy_instance * const m_proxy_instance;

    int m_addr_family;

    const std::shared_ptr<const mroute_socket> m_mrt_sock;

    const std::shared_ptr<const interfaces> m_interfaces;

    void start();

    bool is_if_index_relevant(unsigned int if_index) const;

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

public:
    /**
      * @brief Create a receiver.
     */
    receiver(proxy_instance* pr_i, int addr_family, const std::shared_ptr<const mroute_socket> mrt_sock, const std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode= false);

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
    void registrate_interface(unsigned int if_index);

    /**
     * @brief Delete an registerd interface
     * @param if_index interface index of the interface
     * @param vif virtual interface index of the interface
     */
    void del_interface(unsigned int if_index);

    /**
     * @brief Check whether the receiver is running.
     */
    bool is_running();
};

#endif // RECEIVER_HPP
/** @} */
