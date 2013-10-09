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
#include "include/proxy/interfaces.hpp"
#include "include/proxy/querier.hpp"

#include <memory>
#include <vector>


struct config_msg : public proxy_msg {

    enum config_instruction {
        ADD_DOWNSTREAM,
        DEL_DOWNSTREAM,
        ADD_UPSTREAM,
        DEL_UPSTREAM
    };

    config_msg(config_instruction instruction, unsigned int if_index)
        : proxy_msg(CONFIG_MSG, SYSTEMIC)
        , m_instruction(instruction)
        , m_if_index(if_index) {
        HC_LOG_TRACE("");
    }

    config_instruction get_instruction() {
        return m_instruction;
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

private:
    config_instruction m_instruction;
    unsigned int m_if_index;
};

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

    //if_index, querier
    std::map<int, std::unique_ptr<querier>> m_querier;

    //init
    bool init_mrt_socket();
    bool init_sender();
    bool init_receiver();
    bool init_routing();

    void worker_thread();

    //processed joins and leaves
    //void handle_igmp(struct receiver_msg* r);

    ////processed clock events
    //void handle_clock(struct clock_msg* c);

    ////create debug output
    //void handle_debug_msg(struct debug_msg* db);

    //add and del interfaces
    void handle_config(const std::shared_ptr<config_msg>& msg);

public:
    /**
     * @brief Set default values of the class members.
     */
    proxy_instance(int addr_family, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing);

    /**
     * @brief Release all resources.
     */
    virtual ~proxy_instance();

    static void test_querier(int addr_family, std::string if_name);
    static void send_test_record(querier& q, mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>&& saddr_list, int report_version);
};

#endif // PROXY_INSTANCE_HPP
/** @}*/
