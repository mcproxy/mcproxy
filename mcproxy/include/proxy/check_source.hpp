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
 * @addtogroup mod_proxy_instance Proxy Instance
 * @{
 */

#ifndef CHECK_SOURCE_H
#define CHECK_SOURCE_H

//#include "include/utils/mc_tables.hpp"
#include <map>
#include "include/utils/mroute_socket.hpp"

//--------------------------------------------------
/**
 * @brief Data structure to save a specific multicast route
 * @param first source address
 * @param second group address
 **/
typedef std::pair<addr_storage, addr_storage> src_grp_pair;

//--------------------------------------------------
/**
 * @brief Data structure to save a packet counter for a specific multicast route
 * @param first (source address, group address) #src_grp_pair
 * @param second counter for received packets
 */
typedef std::map<src_grp_pair, int> pkt_cnt_map;

/**
 * @brief Pair for #pkt_cnt_map.
 * @param first (source address, group address) #src_grp_pair
 * @param second counter for received packets
 */
typedef std::pair<src_grp_pair, int> pkt_cnt_pair;

//--------------------------------------------------

/**
 * @brief Monitored the forwarding rules in the Linux kernel table. If a source is unused for
 * a long time when it can be removed.
 */
class check_source
{
private:
    int m_addr_family;

    mroute_socket* m_sock;

    pkt_cnt_map m_check_src_a;
    pkt_cnt_map m_check_src_b;
    pkt_cnt_map* m_current_check;

public:

    /**
      * @brief Initialize check_source.
      * @param addr_family used IP version (AF_INET or AF_INET6)
      * @return Return true on success.
      */
    bool init(int addr_family, mroute_socket* sock );

    /**
      * @brief Trigger the monitoring.
      * @return Return true on success.
      */
    bool check();

    /**
      * @brief Check wether an unique forwarding rule is unused since the last monitoring trigger.
      * @param vif virutal interface of the forwarding rule
      * @param src_addr source address of the forwarding rule
      * @param g_addr multicast group address of the forwarding rule
      */
    bool is_src_unused(int vif, addr_storage src_addr, addr_storage g_addr);

};

#endif // CHECK_IF_H
/** @} */
