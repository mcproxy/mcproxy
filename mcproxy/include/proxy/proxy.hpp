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
 * @defgroup mod_proxy Proxy
 * @brief The module Proxy reads the configuration file on startup.
 *        It then configures and starts all other modules, acquire the mrt-flag
 *        on a socket for manipulating the multicast routing tables of the Linux kernel.
 * @{
 */

#ifndef PROXY_HPP
#define PROXY_HPP

#include "include/utils/addr_storage.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/receiver.hpp"

#include <map>
#include <vector>
#include <string>

//--------------------------------------------------

/**
 * @brief Downstream vector for #up_down_map.
 * @param vector of the downstream interfaces indexes
 */
using down_vector = std::vector<int>;
/**
 * @brief data structure to mangage the upstream/downstream instances.
 * @param first index of the upstream interface
 * @param second index vector of the downstream interfaces
 */
using up_down_map = std::map<int, down_vector>; //map<upstream, downstreams>

/**
 * @brief Pair for #up_down_map.
 * @param first index of the upstream interface
 * @param second vector of the downstream interfaces
 */
using up_down_pair = std::pair<int, down_vector>;

//--------------------------------------------------

/**
 * @brief If s proxy instance in the timeout time dont react to an debug message it will be ignored.
 */
#define PROXY_DEBUG_MSG_TIMEOUT 3000 //msec



/**
  * @brief Instanced the multicast proxy
  */
class proxy
{
private:
    //control data
    static bool m_running;
    int m_verbose_lvl;
    bool m_print_status;


    string m_config_path;
    int m_addr_family; //AF_INET or AF_INET6
    int m_version; //for AF_INET (1,2,3) to use IGMPv1/2/3, for AF_INET6 (1,2) to use MLDv1/2

    //--
    vector<proxy_instance*> m_proxy_instances;
    interface_map m_interface_map;
    up_down_map m_up_down_map;


    bool m_rest_rp_filter;

    vector<int> all_if_to_list();

    //##############
    //##-- Init --##
    //##############

    bool prozess_commandline_args(int arg_count, char* args[]);
    void help_output();



    //check the state_table for valid input, interfaces can only used on time ==> true = check ok, false = double interfaces
    bool check_and_set_flags(vector<int>& interface_list); //check up and running flag, set multicast and allMulti flag
    bool start_proxy_instances();


    //bool init_routing_table(); //add all interfaces from state_table to ip_mr_vif (phyint or tunnel) , allocate memory for m_vif
    static void signal_handler(int sig);
    void close();


public:
    /**
     * @brief Set default values of the class members and add signal handlers for the signal SIGINT and SIGTERM.
     */
    proxy();

    /**
     * @brief Release all resources and restore reverse path flags if changed.
     */
    virtual ~proxy();


    bool init(int arg_count, char* args[]);

    /**
     * @brief Start the proxy.
     * @return Return true on success.
     */
    bool start();

    /**
     * @brief Stop the proxy.
     */
    void stop();
};

#endif // PROXY_HPP
/** @}*/

