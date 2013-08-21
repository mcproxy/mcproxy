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
#include "include/utils/mroute_socket.hpp"
#include "include/utils/if_prop.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/receiver.hpp"

#include <map>
#include <vector>
#include <string>
using namespace std;

//--------------------------------------------------
/**
 * @brief data structure to the interface index with the current virtual interface index.
 * @param first if_index
 * @param second vif
 */
typedef map<int, int> vif_map;

/**
 * @brief Pair for #vif_map.
 * @param first if_index
 * @param second vif
 */
typedef pair<int, int> vif_pair;

//--------------------------------------------------

/**
 * @brief Downstream vector for #up_down_map.
 * @param vector of the downstream interfaces
 */
typedef vector<int> down_vector;
/**
 * @brief Downstream vector for #up_down_map.
 * @param vector of the downstream interfaces
 */
typedef vector<int> down_vector;
/**
 * @brief data structure to mangage the upstream/downstream instances.
 * @param first index of the upstream interface
 * @param second index vector of the downstream interfaces
 */
typedef map<int, down_vector > up_down_map; //map<upstream, downstreams>

/**
 * @brief Pair for #up_down_map.
 * @param first index of the upstream interface
 * @param second vector of the downstream interfaces
 */
typedef pair<int, down_vector > up_down_pair;

//--------------------------------------------------

/**
 * @brief Lookup map to find a proxyinstance for a spezific interface index
 * @param first interface index
 * @param second proxyinstance index based on arbitrary vector
 */
typedef map<int, int > interface_map; //map< interface, proxyinstance_index>

/**
 * @brief Pair for #interface_map
 * @param first interface index
 * @param second proxyinstance index based on arbitrary vector
 */
typedef pair<int, int> interface_pair;

//--------------------------------------------------
/**
 * @brief Maximum length of a line in the config file.
 */
#define PROXY_CONFIG_LINE_LENGTH 200

/**
 * @brief If s proxy instance in the timeout time dont react to an debug message it will be ignored.
 */
#define PROXY_DEBUG_MSG_TIMEOUT 3000 //msec

/**
 * @brief Path to change the rp filter flag.
 */
#define PROXY_RP_FILTER_PATH "/proc/sys/net/ipv4/conf/"

/**
 * @brief Default path to find the config file.
 */
#define PROXY_DEFAULT_CONIG_PATH "mcproxy.conf"

/**
  * @brief Instanced the multicast proxy
  */
class proxy
{
private:
    //control data
    static bool m_running;
    bool m_is_single_instance;
    int m_verbose_lvl;
    bool m_print_status;

    bool m_rest_rp_filter;
    vector<string> m_restore_rp_filter_vector; //save interfaces wiche musst set to true after before terminating

    string m_config_path;
    int m_addr_family; //AF_INET or AF_INET6
    int m_version; //for AF_INET (1,2,3) to use IGMPv1/2/3, for AF_INET6 (1,2) to use MLDv1/2

    //--
    vector<proxy_instance*> m_proxy_instances;
    interface_map m_interface_map;
    up_down_map m_up_down_map;
    vif_map m_vif_map;

    if_prop m_if_prop;

    int get_free_vif_number();

    vector<int> all_if_to_list();

    //##############
    //##-- Init --##
    //##############

    bool prozess_commandline_args(int arg_count, char* args[]);
    void help_output();
    bool get_rp_filter(string interface);
    bool set_rp_filter(string interface, bool to);
    bool restore_rp_filter();

    bool load_config(string path); //load the config file and add the interfaces to state_table


    //check the state_table for valid input, interfaces can only used on time ==> true = check ok, false = double interfaces
    bool check_double_used_if(const vector<int>* new_interfaces);
    bool init_vif_map();
    bool init_if_prop();
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

    /**
     * @brief Return readable state table information.
     */
    string get_state_table();

    /**
     * @brief initialize the proxy
     * @param arg_count Number of passed parameter.
     * @param args Passed parameter
     *
     *     Usage: mcproxy [-h] [-f] [-d] [-s] [-v [-v]] [-c <configfile>]
     *
     *         -h
     *              Display this help screen.
     *         -f
     *              Reset the reverse path filter flag, to accept data from
     *              foreign Subnets.
     *         -d
     *              Run in debug mode. Output all log messages on thread[X]
     *              file.
     *         -s
     *              Print proxy status information.
     *         -v
     *              Be verbose. Give twice to see even more messages
     *         -c
     *              To specify the configuration file.
     *
     * @return Return true on success.
     */
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

