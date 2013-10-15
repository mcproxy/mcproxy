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
#include "include/proxy/def.hpp"
#include "include/proxy/interfaces.hpp"

#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <chrono>

struct proxy_msg {

    enum message_type {
        INIT_MSG,
        TEST_MSG,
        EXIT_MSG,
        FILTER_TIMER_MSG,
        SOURCE_TIMER_MSG,
        CONFIG_MSG,
        GROUP_RECORD_MSG,
        DEBUG_MSG
    };

    enum message_priority {
        USER_INPUT = 1, //high
        SYSTEMIC = 10,
        LOSEABLE = 100 //low
    };

    static std::string get_message_type_name(message_type mt) {
        std::map<proxy_msg::message_type, std::string> name_map = {
            {proxy_msg::INIT_MSG,             "INIT_MSG"         },
            {proxy_msg::TEST_MSG,             "TEST_MSG"         },
            {proxy_msg::EXIT_MSG,             "EXIT_MSG"         },
            {proxy_msg::FILTER_TIMER_MSG,     "FILTER_TIMER_MSG" },
            {proxy_msg::SOURCE_TIMER_MSG,     "SOURCE_TIMER_MSG" },
            {proxy_msg::CONFIG_MSG,           "CONFIG_MSG"       },
            {proxy_msg::GROUP_RECORD_MSG,     "GROUP_RECORD_MSG" },
            {proxy_msg::DEBUG_MSG,            "DEBUG_MSG"        }
        };
        return name_map[mt];
    }

    static std::string get_message_priority_name(message_priority mp) {
        std::map<proxy_msg::message_priority, std::string> name_map = {
            {proxy_msg::SYSTEMIC,     "SYSTEMIC"  },
            {proxy_msg::USER_INPUT,   "USER_INPUT"},
            {proxy_msg::LOSEABLE,     "LOSEABLE"  },
        };
        return name_map[mp];
    }

    proxy_msg(): m_type(INIT_MSG), m_prio(SYSTEMIC) {
        HC_LOG_TRACE("");
    }

    ~proxy_msg() {
        HC_LOG_TRACE("");
    }

    friend bool operator< (const proxy_msg& l, const proxy_msg& r) {
        HC_LOG_TRACE("");
        return l.m_prio > r.m_prio;
    }

    message_type get_type() {
        HC_LOG_TRACE("");
        return m_type;
    }

    message_priority get_priority() {
        return m_prio;
    }

    virtual void operator() () {
        HC_LOG_TRACE("Empty operator");
    }

protected:
    proxy_msg(message_type type, message_priority prio): m_type(type), m_prio(prio) {
        HC_LOG_TRACE("");
    }

    message_type m_type;
    message_priority m_prio;
};




//------------------------------------------------------------------------
struct test_msg : public proxy_msg {
    test_msg(int value, message_priority prio): proxy_msg(TEST_MSG, prio), m_value(value) {
        HC_LOG_TRACE("");
    }

    virtual ~test_msg() {
        HC_LOG_TRACE("");
    }

    virtual void operator()() override {
        HC_LOG_TRACE("");
        std::cout << "Test Message value: "  << m_value << " prio: " << proxy_msg::get_message_priority_name(get_priority()) << std::endl;
    }

private:
    int m_value;
};

//------------------------------------------------------------------------
struct timer_msg : public proxy_msg {
    timer_msg(message_type type, unsigned int if_index, const addr_storage& gaddr, const std::chrono::milliseconds& duration): proxy_msg(type, SYSTEMIC), m_if_index(if_index), m_gaddr(gaddr), m_end_time(std::chrono::steady_clock::now() + duration) {
        HC_LOG_TRACE("");
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    const addr_storage& get_gaddr() {
        return m_gaddr;
    }

    std::string get_remaining_time() {
        using namespace std::chrono;
        std::ostringstream s;
        auto current_time = steady_clock::now();
        auto time_span = m_end_time - current_time;
        double seconds = time_span.count()  * steady_clock::period::num / steady_clock::period::den;
        s << seconds << "sec";
        return s.str();
    }

private:
    unsigned int m_if_index;
    addr_storage m_gaddr;
    std::chrono::time_point<std::chrono::steady_clock> m_end_time;
};

struct filter_timer : public timer_msg {
    filter_timer(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(FILTER_TIMER_MSG, if_index, gaddr, duration), m_is_used_as_source_timer(false) {
        HC_LOG_TRACE("");
    }

    bool is_used_as_source_timer() {
        return m_is_used_as_source_timer;
    }

    void set_as_source_timer() {
        m_is_used_as_source_timer = true;
    }

private:
    bool m_is_used_as_source_timer;
};

struct source_timer : public timer_msg {
    source_timer(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(SOURCE_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};


//------------------------------------------------------------------------
struct debug_msg : public proxy_msg {
    debug_msg(): proxy_msg(DEBUG_MSG, SYSTEMIC) {
        HC_LOG_TRACE("");
    }
};

//------------------------------------------------------------------------

struct source {
    source() = default;
    source(source&&) = default;
    source& operator=(source && ) = default;

    source(const source&) = default;
    source& operator=(const source& s) = default;

    source(const addr_storage& saddr)
        : saddr(saddr)
        , shared_source_timer(nullptr)
        , current_retransmission_count(NOT_IN_RETRANSMISSION_STATE)  
    {}

    addr_storage saddr;

    mutable std::shared_ptr<timer_msg> shared_source_timer;
    int current_retransmission_count;

    std::string to_string() const {
        std::ostringstream s;
        s << saddr;
        if (shared_source_timer.get() != nullptr) {
            s << "(c:" << shared_source_timer->get_remaining_time() << ")";
        }
        if (current_retransmission_count != NOT_IN_RETRANSMISSION_STATE) {
            s << "(rt:" << shared_source_timer->get_remaining_time() << ")";
        }
        return s.str();
    }

    friend std::ostream& operator<<(std::ostream& stream, const source& s) {
        return stream << s.to_string();
    }

    friend bool operator< (const source& l, const source& r) {
        return l.saddr < r.saddr;
    }

    friend bool operator==(const source& l, const source& r) {
        return l.saddr == r.saddr;
    }
};

struct group_record_msg : public proxy_msg {

    group_record_msg()
        : group_record_msg(0, MODE_IS_INCLUDE, addr_storage(), source_list<source>(), -1 ) {}

    group_record_msg(unsigned int if_index, mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>&& slist, int report_version)
        : proxy_msg(GROUP_RECORD_MSG, LOSEABLE)
        , m_if_index(if_index)
        , m_record_type(record_type)
        , m_gaddr(gaddr)
        , m_slist(slist)
        , m_report_version(report_version) {}

    friend std::ostream& operator<<(std::ostream& stream, const group_record_msg& r) {
        return stream << r.to_string();
    }

    std::string to_string() const {
        HC_LOG_TRACE("");
        std::ostringstream s;
        s << "interface: " << interfaces::get_if_name(m_if_index) << std::endl;
        s << "record_type: " << get_mcast_addr_record_type_name(m_record_type) << std::endl;
        s << "group address: " << m_gaddr << std::endl;
        s << "source list: " << m_slist << std::endl;
        s << "report version: " << m_report_version;
        return s.str();
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    mcast_addr_record_type get_record_type() {
        return m_record_type;
    }

    const addr_storage& get_gaddr() {
        return m_gaddr;
    }

    source_list<source>& get_slist() {
        return m_slist;
    }

    int get_report_version() {
        return m_record_type;
    }

private:
    unsigned int m_if_index;
    mcast_addr_record_type m_record_type;
    addr_storage m_gaddr;
    source_list<source> m_slist;
    int m_report_version;
};

//------------------------------------------------------------------------
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

struct exit_cmd : public proxy_msg {
    exit_cmd(): proxy_msg(EXIT_MSG, USER_INPUT) {
        HC_LOG_TRACE("");
    }
};

//message_type: DEBUG_MSG
/**
* @brief Message to collect debug information for the module @ref mod_proxy.
*/
//struct debug_msg: public intrusive_message {
//public:

/**
* @brief Level of detail to collect debug informaiton.
*/
//enum lod { //level of detail
//LESS = 0         [>* low level of detail <],
//NORMAL = 1       [>* normal level of detail <],
//MORE = 2         [>* high level of detail <],
//MORE_MORE = 3  [>* highest level of detail <]
//};

/**
* @brief Create a debug message.
* @param details level of detail to collect debug information
* @param counter how many proxy instances have to collect debug information
* @param timeout_msec if a proxy instance dont response it will be ignored after a period of time
*/
//debug_msg(lod details, int counter,  int timeout_msec): level_of_detail(details), m_counter(counter),
//m_timeout_msec(timeout_msec) {
//HC_LOG_TRACE("");
//HC_LOG_DEBUG("
//counter: " << m_counter);
//}

//virtual ~debug_msg() {
//HC_LOG_TRACE("");
//}

/**
* @brief Add debug information as string.
*/
//void add_debug_msg(std::string debug_input) {
//HC_LOG_TRACE("");
//{
//boost::lock_guard<boost::mutex> lock(m_global_lock);
//m_str << debug_input << std::endl;
//m_counter--;
//}
//cond_all_done.notify_all();
//}

/**
* @brief Get true if all proxy instances response.
*/
//bool all_done() {
//HC_LOG_TRACE("");
//boost::lock_guard<boost::mutex> lock(m_global_lock);
//return m_counter <= 0;
//}

/**
* @brief Get all debug information.
*/
//std::string get_debug_msg() {
//HC_LOG_TRACE("");
//boost::lock_guard<boost::mutex> lock(m_global_lock);
//return m_str.str();
//}

/**
* @brief Wait until all proxy instances has response or the timeout expires.
*/
//void join_debug_msg() {
//boost::unique_lock<boost::mutex> lock(m_global_lock);
//while (m_counter > 0) {
//cond_all_done.timed_wait(lock, boost::posix_time::millisec(m_timeout_msec));
//}
//m_counter = 0;
//}

/**
* @brief Get the level of detail.
*/
//lod get_level_of_detail() {
//return level_of_detail;
//}

//private:
//lod level_of_detail;
//std::stringstream m_str;
//boost::mutex m_global_lock;
//boost::condition_variable cond_all_done;
//int m_counter;
//int m_timeout_msec;

//};


////message_type: CLOCK_MSG
/**
* @brief Message used from module @ref mod_timer. It is the contain of a reminder.
*/
//struct clock_msg: public intrusive_message {

/**
* @brief A module @ref mod_timer can remind about this actions.
*/
//enum clock_action {
//SEND_GQ_TO_ALL [>* Send to all downstreams General Queries. <],
//SEND_GSQ       [>* Send a Group Specific Query to an interface and to a group. <],
//DEL_GROUP      [>* Delete a group from an interface. <],
//SEND_GQ        [>* not implementeted at the moment. <]
//};

/**
* @brief Constructor used for the actions DEL_GROUP, SEND_GQ and SEND_GSQ.
* @param type type of the clock action
* @param if_index actionfor a specific interface index
* @param g_addr action for a specific multicast group
*/
//clock_msg(clock_action type, int if_index, addr_storage g_addr) {
//HC_LOG_TRACE("");
//this->type = type;
//this->if_index = if_index;
//this->g_addr = g_addr;
//}

/**
* @brief Constructor used for the action SEND_GQ_TO_ALL.
* @param type type of the clock action
*/
//clock_msg(clock_action type) {
//this->type = type;
//}

//virtual ~clock_msg() {
//HC_LOG_TRACE("");
//}

/**
* @brief Type of the clock message.
*/
//clock_action type;

/**
* @brief Action on a specific interface index.
*/
//int if_index;

/**
* @brief Action for a specific multicast group.
*/
//addr_storage g_addr;
//};

////message_type: RECEIVER_MSG
/**
* @brief Message used from module @ref mod_receiver to inform the
* module @ref mod_proxy_instance of received a message.
*/
//struct receiver_msg: public intrusive_message {

/**
* @brief A module @ref mod_receiver can receive the following messages.
*/
//enum receiver_action {
//JOIN           [>* a Join message for a specific group on a specific interface <],
//LEAVE          [>* a Leave message for a specific group on a specific interface <],
//CACHE_MISS     [>* a Cache Miss message from the Linux Kernel <]
//};

////CACHE_MISS
/**
* @brief Constructor used for the action CACHE_MISS.
* @param type type of the receiver action
* @param if_index action for a specific interface index
* @param src_addr action for a specific source
* @param g_addr action for a specific multicast group
*/
//receiver_msg(receiver_action type, int if_index, addr_storage src_addr, addr_storage g_addr):
//type(type), if_index(if_index), src_addr(src_addr), g_addr(g_addr) {
//HC_LOG_TRACE("");
//}
/* @brief Constructor used for the actions JOIN and LEAVE.
* @param type type of the receiver action
* @param if_index action for a specific interface index
* @param g_addr action for a specific multicast group
*/
//receiver_msg(receiver_action type, int if_index, addr_storage g_addr):
//type(type), if_index(if_index), g_addr(g_addr) {
//HC_LOG_TRACE("");
//}

//virtual ~receiver_msg() {
//HC_LOG_TRACE("");
//}

/**
* @brief Type of the receiver message.
*/
//receiver_action type;

/**
* @brief Action on a specific interface index.
*/
//int if_index;

/**
* @brief Action for a specific source address.
*/
//addr_storage src_addr;

/**
* @brief Action for a specific multicast group.
*/
//addr_storage g_addr;

//};

////message_type: CONFIG_MSG
/**
* @brief Message used from module @ref mod_proxy to
* set and delete interfaces of the proxy instances.
*/
//struct config_msg: public intrusive_message {

/**
* @brief configure types for proxy instances
*/
//enum config_action {
//ADD_DOWNSTREAM [>* downstreams can be added to a proxy instance<],
//DEL_DOWNSTREAM [>* downstreams can be delete form a proxy instance <],
//SET_UPSTREAM   [>* an upstream can be changed <]
//};

////routing_action: ADD_VIF and DEL_VIF
/**
* @brief Create a config_msg.
* @param type configuration type
* @param if_index index of the to change interface
* @param vif virtual index of the to change interface
*/
//config_msg(config_action type, int if_index, int vif):
//type(type), if_index(if_index), vif(vif) {
//HC_LOG_TRACE("");
//}

//virtual ~config_msg() {
//HC_LOG_TRACE("");
//}

/**
* @brief Type of the config_msg.
*/
//config_action type;

/**
* @brief Action on a specific interface index.
*/
//int if_index;

/**
* @brief Action on a virtual interface index.
*/
//int vif;
//};


#endif // MESSAGE_FORMAT_HPP
/** @} */
