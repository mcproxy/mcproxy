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
 * @addtogroup mod_communication Communication
 * @{
 */

#ifndef MESSAGE_FORMAT_HPP
#define MESSAGE_FORMAT_HPP

#include "include/hamcast_logging.h"
#include "include/utils/addr_storage.hpp"
#include <sys/socket.h>
#include <boost/intrusive_ptr.hpp>
#include <iostream>
#include <string>
#include <list>

#include <sstream>
#include <boost/thread.hpp>

/**
 * @brief Message container implements an intrusive pointer to save a
 * pointer of a message with a reference counter.
 */
struct intrusive_message {
     intrusive_message(): refs(0) {}

private:
     int refs;
     boost::mutex m_global_lock;

     /**
      * @brief Release the memory space of the message if no one refer to this message.
      */
     friend inline void intrusive_ptr_release(struct intrusive_message* p){
          HC_LOG_TRACE("pointer: " << p);

          p->m_global_lock.lock();
          if(--p->refs == 0 ) {
               HC_LOG_DEBUG("del element (pointer: " << p << ")");
               p->m_global_lock.unlock();
               delete p;
               return;
          }
          HC_LOG_DEBUG("release ref ==> now: " << p->refs);
          p->m_global_lock.unlock();
     }

     /**
      * @brief Increment the reference counter if
      */
     friend inline void intrusive_ptr_add_ref(struct intrusive_message* p){
          HC_LOG_TRACE("pointer: " << p);

          boost::lock_guard<boost::mutex> lock(p->m_global_lock);
          p->refs++;
          HC_LOG_DEBUG("add ref ==> now: " << p->refs);
     }
};

//##-- generic struct for message_queue --##
/**
 * @brief Generic message for the #message_queue.
 */

typedef struct {

     /**
      * @brief Available message types.
      */
     enum message_type{
          TEST_MSG       /** Test message type to test the message queue and the intrusive pointer. */,
          CLOCK_MSG      /** Message type used from module @ref mod_timer. */,
          RECEIVER_MSG   /** Message type used from module @ref mod_receiver. */,
          CONFIG_MSG     /** Message type used from module @ref mod_proxy to set and delete interfaces of the proxy instances. */,
          EXIT_CMD       /** Message type to stop the proxy instances. */,
          DEBUG_MSG      /** Message type to collect debug information for the module @ref mod_proxy. */
     };

     std::string msg_type_to_string(){
          HC_LOG_TRACE("");
          switch(type){
          case TEST_MSG: return "TEST_MSG";
          case CLOCK_MSG: return "CLOCK_MSG";
          case RECEIVER_MSG: return "RECEIVER_MSG";
          case DEBUG_MSG: return "DEBUG_MSG";
          case EXIT_CMD: return "EXIT_CMD";
          case CONFIG_MSG: return "CONFIG_MSG";
          default: return "ERROR";
          }
     }

     /**
      * @brief Message type of the Message.
      */
     message_type type;

     /**
      * @brief Intrusive pointer to the message contain.
      */
     boost::intrusive_ptr<struct intrusive_message> msg;
} proxy_msg;


//##-- possible messages --##
/**
 * @brief Test message to test the message queue and the intrusive pointer.
 */
struct test_msg: public intrusive_message{
     /**
      * @brief Create a test_msg.
      */
     test_msg(int value): m_value(value) {
          HC_LOG_TRACE("");
     }

     virtual ~test_msg(){
          HC_LOG_TRACE("");
     }

     /**
      * @brief Do some output.
      */
     void test(){
          HC_LOG_TRACE("");
          std::cout << "Test Message value:"  << m_value << std::endl;
     }
private:
     int m_value;
};

//message_type: CLOCK_MSG
/**
 * @brief Message used from module @ref mod_timer. It is the contain of a reminder.
 */
struct clock_msg: public intrusive_message{

     /**
      * @brief A module @ref mod_timer can remind about this actions.
      */
     enum clock_action{
          SEND_GQ_TO_ALL /** Send to all downstreams General Queries. */,
          SEND_GSQ       /** Send a Group Specific Query to an interface and to a group. */,
          DEL_GROUP      /** Delete a group from an interface. */,
          SEND_GQ        /** not implementeted at the moment. */
     };

     /**
      * @brief Constructor used for the actions DEL_GROUP, SEND_GQ and SEND_GSQ.
      * @param type type of the clock action
      * @param if_index actionfor a specific interface index
      * @param g_addr action for a specific multicast group
      */
     clock_msg(clock_action type, int if_index, addr_storage g_addr){
          HC_LOG_TRACE("");
          this->type = type;
          this->if_index = if_index;
          this->g_addr = g_addr;
     }

     /**
      * @brief Constructor used for the action SEND_GQ_TO_ALL.
      * @param type type of the clock action
      */
     clock_msg(clock_action type){
          this->type = type;
     }

     virtual ~clock_msg(){
          HC_LOG_TRACE("");
     }

     /**
      * @brief Type of the clock message.
      */
     clock_action type;

     /**
      * @brief Action on a specific interface index.
      */
     int if_index;

     /**
      * @brief Action for a specific multicast group.
      */
     addr_storage g_addr;
};

//message_type: RECEIVER_MSG
/**
 * @brief Message used from module @ref mod_receiver to inform the
 * module @ref mod_proxy_instance of received a message.
 */
struct receiver_msg: public intrusive_message{

     /**
      * @brief A module @ref mod_receiver can receive the following messages.
      */
     enum receiver_action{
          JOIN           /** a Join message for a specific group on a specific interface */,
          LEAVE          /** a Leave message for a specific group on a specific interface */,
          CACHE_MISS     /** a Cache Miss message from the Linux Kernel */
     };

     //CACHE_MISS
     /**
      * @brief Constructor used for the action CACHE_MISS.
      * @param type type of the receiver action
      * @param if_index action for a specific interface index
      * @param src_addr action for a specific source
      * @param g_addr action for a specific multicast group
      */
     receiver_msg(receiver_action type, int if_index, addr_storage src_addr, addr_storage g_addr):
          type(type), if_index(if_index), src_addr(src_addr), g_addr(g_addr) {
          HC_LOG_TRACE("");
     }

     //JOIN, LEAVE
     /**
      * @brief Constructor used for the actions JOIN and LEAVE.
      * @param type type of the receiver action
      * @param if_index action for a specific interface index
      * @param g_addr action for a specific multicast group
      */
     receiver_msg(receiver_action type, int if_index, addr_storage g_addr):
          type(type), if_index(if_index), g_addr(g_addr) {
          HC_LOG_TRACE("");
     }

     virtual ~receiver_msg(){
          HC_LOG_TRACE("");
     }

     /**
      * @brief Type of the receiver message.
      */
     receiver_action type;

     /**
      * @brief Action on a specific interface index.
      */
     int if_index;

     /**
      * @brief Action for a specific source address.
      */
     addr_storage src_addr;

     /**
      * @brief Action for a specific multicast group.
      */
     addr_storage g_addr;

};

//message_type: CONFIG_MSG
/**
 * @brief Message used from module @ref mod_proxy to
 * set and delete interfaces of the proxy instances.
 */
struct config_msg: public intrusive_message{

     /**
      * @brief configure types for proxy instances
      */
     enum config_action{
          ADD_DOWNSTREAM /** downstreams can be added to a proxy instance*/,
          DEL_DOWNSTREAM /** downstreams can be delete form a proxy instance */,
          SET_UPSTREAM   /** an upstream can be changed */
     };

     //routing_action: ADD_VIF and DEL_VIF
     /**
      * @brief Create a config_msg.
      * @param type configuration type
      * @param if_index index of the to change interface
      * @param vif virtual index of the to change interface
      */
     config_msg(config_action type,int if_index, int vif):
          type(type), if_index(if_index), vif(vif) {
          HC_LOG_TRACE("");
     }

     virtual ~config_msg(){
          HC_LOG_TRACE("");
     }

     /**
      * @brief Type of the config_msg.
      */
     config_action type;

     /**
      * @brief Action on a specific interface index.
      */
     int if_index;

     /**
      * @brief Action on a virtual interface index.
      */
     int vif;
};

//message_type: DEBUG_MSG
/**
 * @brief Message to collect debug information for the module @ref mod_proxy.
 */
struct debug_msg: public intrusive_message{
public:

     /**
      * @brief Level of detail to collect debug informaiton.
      */
     enum lod{ //level of detail
          LESS=0         /** low level of detail */,
          NORMAL=1       /** normal level of detail */,
          MORE=2         /** high level of detail */,
          MORE_MORE=3    /** highest level of detail */
     };

     /**
      * @brief Create a debug message.
      * @param details level of detail to collect debug information
      * @param counter how many proxy instances have to collect debug information
      * @param timeout_msec if a proxy instance dont response it will be ignored after a period of time
      */
     debug_msg(lod details, int counter,  int timeout_msec): level_of_detail(details), m_counter(counter), m_timeout_msec(timeout_msec){
          HC_LOG_TRACE("");
          HC_LOG_DEBUG("counter: " << m_counter);
     }

     virtual ~debug_msg(){
          HC_LOG_TRACE("");
     }

     /**
      * @brief Add debug information as string.
      */
     void add_debug_msg(std::string debug_input){
          HC_LOG_TRACE("");
          {
              boost::lock_guard<boost::mutex> lock(m_global_lock);
              m_str << debug_input << std::endl;
              m_counter--;
          }
          cond_all_done.notify_all();
     }

     /**
      * @brief Get true if all proxy instances response.
      */
     bool all_done(){
          HC_LOG_TRACE("");
          boost::lock_guard<boost::mutex> lock(m_global_lock);
          return m_counter <= 0;
     }

     /**
      * @brief Get all debug information.
      */
     std::string get_debug_msg(){
          HC_LOG_TRACE("");
          boost::lock_guard<boost::mutex> lock(m_global_lock);
          return m_str.str();
     }

     /**
      * @brief Wait until all proxy instances has response or the timeout expires.
      */
     void join_debug_msg(){
          boost::unique_lock<boost::mutex> lock(m_global_lock);
          while(m_counter > 0){
               cond_all_done.timed_wait(lock,boost::posix_time::millisec(m_timeout_msec));
          }
          m_counter = 0;
     }

     /**
      * @brief Get the level of detail.
      */
     lod get_level_of_detail(){
          return level_of_detail;
     }

private:
     lod level_of_detail;
     std::stringstream m_str;
     boost::mutex m_global_lock;
     boost::condition_variable cond_all_done;
     int m_counter;
     int m_timeout_msec;

};

#endif // MESSAGE_FORMAT_HPP
/** @} */
