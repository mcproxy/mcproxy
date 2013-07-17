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
#include "include/proxy/message_queue.hpp"
#include "include/proxy/message_format.hpp"
#include "include/proxy/worker.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/mld_sender.hpp"
#include "include/proxy/receiver.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/check_source.hpp"

#include <vector>
using namespace std;

/**
 * @brief Maximum size of the job queue.
 */
#define PROXY_INSTANCE_MSG_QUEUE_SIZE 1000

/**
 * @brief Saved groupmemberships will be deleted if there robustness counter is equal this define.
 */
#define PROXY_INSTANCE_DEL_IMMEDIATELY 0

/**
 * @brief Data structure to save multicast sources/groups and there states.
 */
struct src_state {
     /**
      * @brief Represents the possible states of a multicast source/group
      */
    enum state {
        INIT            /** unused state to create this structure */,
        RUNNING         /** group is joined */,
        RESPONSE_STATE  /** leave message received, send Group Specific Query and wait for a new join message */,
        WAIT_FOR_DEL    /** last interval before delete this group */,
        UNUSED_SRC      /** multicast data packet received by the kernel, but not used by kernel routing table */,
        CACHED_SRC       /** multicast source forwarded by the Linux kernel */
    };

    /**
     * @brief Convert the current state to  a string.
     */
    std::string state_type_to_string(){
        HC_LOG_TRACE("");
        switch(flag){
        case INIT: return "INIT";
        case RUNNING: return "RUNNING";
        case RESPONSE_STATE: return "RESPONSE_STATE";
        case WAIT_FOR_DEL: return "WAIT_FOR_DEL";
        case UNUSED_SRC: return "UNUSED_SRC";
        case CACHED_SRC: return "CACHED_SRC";
        default: return "ERROR";
        }
    }

    /**
     * @brief Create a new default #src_state.
     */
    src_state(): robustness_counter(0), flag(INIT) {}

    /**
     * @brief Create a new initialized #src_state.
     */
    src_state(int counter, state flag): robustness_counter(counter), flag(flag) {}

    /**
     * @brief Save a counter that is linked to the current state.
     */
    int robustness_counter;

    /**
     * @brief Save the current state.
     */
    state flag;
};

//--------------------------------------------------
/**
 * @brief Data structure to save the interface index with the current virtual interface index.
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
 * @brief Data structure to save sources and there states.
 * @param first source address
 * @param second states of the source
 */
typedef map<addr_storage, struct src_state> src_state_map;

/**
 * @brief Pair for #src_state_map.
 * @param first source address
 * @param second states of the source
 */
typedef pair<addr_storage, struct src_state> src_state_pair;

//--------------------------------------------------
/**
 * @brief Data structure for the upstream interface to save group memberships and there states
 * @param first group address
 * @param second map of sources and there states
 */
typedef map<addr_storage, src_state_map> upstream_src_state_map;

/**
 * @brief Pair for #upstream_src_state_map
 * @param first group address
 * @param second map of sources and there states
 */
typedef pair<addr_storage, src_state_map> upstream_src_state_pair;

//--------------------------------------------------
/**
 * @brief Data structure to save a number of sources with there states and group membership states.
 * @param first data structure to save sources and there states
 * @param second group membership state
 */
typedef pair<src_state_map, struct src_state> src_group_state_pair;

//--------------------------------------------------
/**
 * @brief Data structure to save group memberships with there states and exsists sources with there states
 * @param first group address
 * @param second Data structure to save a number of sources with there states and group membership states.
 */
typedef map<addr_storage, src_group_state_pair > g_state_map;

/**
 * @brief Pair for #g_state_map
 * @param first group address
 * @param second data structure to save a number of sources with there states and group membership states.
 */
typedef pair<addr_storage, src_group_state_pair > g_state_pair;

//--------------------------------------------------

/**
 * @brief Data structure to save downstream interfaces and there multicast group information
 * @param first interface index of the downstream interface
 * @param second multicast group information
 */
typedef map< int, g_state_map > state_table_map;

/**
 * @brief Pair for #state_table_map
 * @param first interface index of the downstream interface
 * @param second multicast group information
 */
typedef pair< int, g_state_map > state_tabel_pair;

/**
 * @brief Represent a multicast Proxy
 */
class proxy_instance: public worker{
private:
    bool m_is_single_instance;
    int m_table_number;
    mroute_socket m_mrt_sock;

    //upstream inforamtion
    int m_upstream; //if_index
    upstream_src_state_map m_upstream_state;

    //downstream inforamtion
    state_table_map m_state_table;


    vif_map m_vif_map; //if_index to vif

    int m_addr_family; //AF_INET or AF_INET6
    int m_version; //for AF_INET (1,2,3) to use IGMPv1/2/3, for AF_INET6 (1,2) to use MLDv1/2

    check_source m_check_source;

    routing m_routing;
    sender* m_sender;
    receiver* m_receiver;
    timing* m_timing;

    //init
    bool init_mrt_socket();
    bool init_receiver();
    bool init_sender();


    void worker_thread();

    // registrate/unregistrate to reciever, router, and to the network
    void registrate_if(int if_index);
    void unregistrate_if(int if_index);

    //##-- igmp automat --##
    //send general query to all downstream
    bool send_gq_to_all();

    //processed joins and leaves
    void handle_igmp(struct receiver_msg* r);

    //processed clock events
    void handle_clock(struct clock_msg* c);

    //create debug output
    void handle_debug_msg(struct debug_msg* db);

    //add and del interfaces
    void handle_config(struct config_msg* c);

    //need for aggregate states
    bool is_group_joined(int without_if_index, const addr_storage& g_addr);

    //handel multicast routes
    //need for CACHE_MISS, General Query
    bool split_traffic(int if_index, const addr_storage& g_addr, const addr_storage& src_addr);
    bool del_route(int if_index, const addr_storage& g_addr, const addr_storage& src_addr);

    //need for join, del group
    void refresh_all_traffic(int if_index, const addr_storage& g_addr);

    //fill vif_list with downstream vifs who has the same g_addr
    //without_if_index will be ignored
    void add_all_group_vifs_to_list(std::list<int>* vif_list, int without_if_index, addr_storage g_addr);


    void close();
public:
    /**
     * @brief Set default values of the class members.
     */
    proxy_instance();

    /**
     * @brief Release all resources.
     */
    virtual ~proxy_instance();


    /**
     * @brief initialise the proxy
     * @param addr_family AF_INET or AF_INET6
     * @param version used group membership version
     * @param upstream_index interface index of the upstream
     * @param upstream_vif virtual interface index of the upstream
     * @param downstream_index  interface index of the downstream
     * @param downstram_vif virtual interface index of the downstream
     * @param receiver* pointer to the modul @ref mod_receiver 
     */
    bool init(int addr_family, int version, int upstream_index, int upstream_vif, int downstream_index, int downstram_vif, bool single_instance);
};

#endif // PROXY_INSTANCE_HPP
/** @}*/
