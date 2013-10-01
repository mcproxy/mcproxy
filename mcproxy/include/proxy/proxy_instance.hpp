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

#include "include/utils/addr_storage.hpp"
#include "include/proxy/message_format.hpp"
#include "include/proxy/worker.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/receiver.hpp"
#include "include/proxy/check_source.hpp"
#include "include/proxy/timing.hpp"

#include <memory>
#include <vector>

/**
 * @brief Represent a multicast Proxy
 */
class proxy_instance: public worker
{
private:
    int m_addr_family; //AF_INET or AF_INET6
    int m_table_number; // 0 means no table number


    //check_source m_check_source;

    const std::shared_ptr<const interfaces> m_interfaces;
    const std::shared_ptr<timing> m_timing;

    std::shared_ptr<mroute_socket> m_mrt_sock;
    std::shared_ptr<sender> m_sender;

    std::unique_ptr<receiver> m_receiver;
    std::unique_ptr<routing> m_routing;

    //init
    bool init_mrt_socket();
    bool init_sender();
    bool init_receiver();
    bool init_routing(); 


    virtual void worker_thread() override;

    //processed joins and leaves
    //void handle_igmp(struct receiver_msg* r);

    ////processed clock events
    //void handle_clock(struct clock_msg* c);

    ////create debug output
    //void handle_debug_msg(struct debug_msg* db);

    ////add and del interfaces
    //void handle_config(struct config_msg* c);


public:
    /**
     * @brief Set default values of the class members.
     */
    proxy_instance(int addr_family, int table_number, const std::shared_ptr<const interfaces> interfaces, const std::shared_ptr<timing> shared_timing);

    /**
     * @brief Release all resources.
     */
    virtual ~proxy_instance();
};

#endif // PROXY_INSTANCE_HPP
/** @}*/
