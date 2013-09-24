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

#include "include/proxy/worker.hpp"
#include "include/utils/mroute_socket.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/utils/if_prop.hpp"

#include <map>
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"

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
typedef std::map<int, proxy_instance*> if_poxy_instance_map;

/**
 * @brief Pair for #if_poxy_instance_map.
 * @param first interface index
 * @param second pointer to the incidental Proxy Instance
 */
typedef std::pair<int, proxy_instance*> if_proxy_instance_pair;

//--------------------------------------------------
//     vif number, if_index        !!! reversed !!!
/**
 * @brief Data structure to save the virtual interface index with the interface index with the current.
 * @param first vif
 * @param second if_index
 * @attention This data structure is reversed to the over vif_maps!!
 */
typedef map<int, int> vif_map;

/**
 * @brief Pair for #vif_map.
 * @param first vif
 * @param second if_index
 * @attention This data structure is reversed to the over vif_maps!!
 */
typedef pair<int, int> vif_pair;

/**
 * @brief Abstract basic receiver class.
 */
class receiver
{
private:
    bool m_running;
    boost::thread* m_worker_thread;
    static void worker_thread(void* arg);

    bool init_if_prop();

    boost::mutex m_data_lock;
    vif_map m_vif_map;

    void close();
protected:
    /**
     * @brief Save the interface index with the incidental Proxy Instance.
     */
    if_poxy_instance_map m_if_proxy_map;

    /**
     * @brief Collect interface properties. Used to generate multicast messages.
     */
    if_prop m_if_property;

    /**
     * @brief Abstracted multicast socket to receive multicast messages.
     */
    mroute_socket* m_mrt_sock;

    /**
     * @brief Used IP version (AF_INET or AF_INET6).
     */
    int m_addr_family;

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

    //return on error 0
    /**
     * @brief Get the interface index to a virtual interface index. Search in a private map #vif_map
     * @param vif virutal interface index
     * @return interface index or 0 if not found
     */
    int get_if_index(int vif);
public:
    /**
      * @brief Create a receiver.
     */
    receiver();

    /**
     * @brief Release all resources.
     */
    virtual ~receiver();

    /**
     * @brief Initialize the receiver.
     * @param addr_family used IP version (AF_INET or AF_INET6)
     * @param version used group membership version
     * @param mrt_sock need the multicast routing socket with set mrt-flag
     * @return Return true on success.
     */
    virtual bool init(int addr_family, mroute_socket* mrt_sock);

    /**
     * @brief Register an interface at the receiver.
     * @param if_index interface index of the registered interface
     * @param vif virtual interface indxe of the inteface
     * @param proxy_instance* who register the interface
     */
    void registrate_interface(int if_index, int vif, proxy_instance* p);

    /**
     * @brief Delete an registerd interface
     * @param if_index interface index of the interface
     * @param vif virtual interface index of the interface
     */
    void del_interface(int if_index, int vif);

    /**
     * @brief Check whether the receiver is running.
     */
    bool is_running();

    /**
     * @brief Start the receiver.
     */
    void start();

    /**
     * @brief Stop the receiver, but dont wait for stopped.
     */
    void stop();

    /**
     * @brief Blocked until receiver stopped.
     */
    void join();
};


#endif // RECEIVER_HPP
/** @} */
