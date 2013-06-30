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
 * @defgroup mod_sender Sender
 * @brief The module Sender generates and sends Membership Queries to the downstreams and Membership Reports to the upstreams.
 * @{
 */

#ifndef SENDER_HPP
#define SENDER_HPP

#include "include/utils/mroute_socket.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/utils/if_prop.hpp"

/**
 * @brief Abstract basic sender class.
 */
class sender{
private:
protected:
     /**
      * @brief Used IP version (AF_INET or AF_INET6).
      */
     int m_addr_family; //AF_INET or AF_INET6

     /**
      *  @brief Used group membership version.
      */
     int m_version; //for AF_INET (1,2,3) to use IGMPv1/2/3, for AF_INET6 (1,2) to use MLDv1/2

     /**
      * @brief Abstracted multicast socket, that use raw-socket to send the messages.
      */
     mroute_socket m_sock;

     /**
      * @brief Collect interface properties. Used to generate multicast messages.
      */
     if_prop m_if_prop; //return interface properties

     /**
      * @brief initialize the interface properties class.
      */
     bool init_if_prop();
public:

     /**
      * @brief Create a sender.
      */
     sender();

     /**
      * @brief initialise the sender
      * @param addr_family used IP version (AF_INET or AF_INET6)
      * @param version used group membership version
      * @return Return true on success.
      */
     virtual bool init(int addr_family, int version);

     /**
      * @brief Send a General Query to a specific interface.
      * @param if_index used interface
      * @return Return true on success.
      */
     virtual bool send_general_query(int if_index)=0;

     /**
      * @brief Send a Group Specific Query to a multicast group and a specific interface.
      * @param if_index used interface
      * @param g_addr used multicast group
      * @return Return true on success.
      */
     virtual bool send_group_specific_query(int if_index, const addr_storage& g_addr)=0;

     /**
      * @brief Send a Membership Report to a multicast group and a specific interface.
      * @param if_index used interface
      * @param g_addr used multicast group
      * @return Return true on success.
      */
     virtual bool send_report(int if_index, const addr_storage& g_addr)=0;

     /**
      * @brief Send a leave Message to a multicast group and a specific interface.
      * @param if_index used interface
      * @param g_addr used multicast group
      * @return Return true on success.
      */
     virtual bool send_leave(int if_index, const addr_storage& g_addr)=0;

};


#endif // SENDER_HPP
/** @} */
