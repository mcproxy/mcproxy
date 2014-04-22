/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, * but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include "include/proxy/timers_values.hpp"
#include "include/parser/interface.hpp"

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
        NEW_SOURCE_MSG,
        NEW_SOURCE_TIMER_MSG,
        RET_GROUP_TIMER_MSG, //retransmission group timer message
        RET_SOURCE_TIMER_MSG,
        OLDER_HOST_PRESENT_TIMER_MSG,
        GENERAL_QUERY_TIMER_MSG,
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
            {INIT_MSG,             "INIT_MSG"            },
            {TEST_MSG,             "TEST_MSG"            },
            {EXIT_MSG,             "EXIT_MSG"            },
            {FILTER_TIMER_MSG,     "FILTER_TIMER_MSG"    },
            {SOURCE_TIMER_MSG,     "SOURCE_TIMER_MSG"    },
            {NEW_SOURCE_MSG,       "NEW_SOURCE_MSG"      },
            {NEW_SOURCE_TIMER_MSG, "NEW_SOURCE_TIMER_MSG"},
            {RET_GROUP_TIMER_MSG,  "RET_GROUP_TIMER_MSG" },
            {RET_SOURCE_TIMER_MSG, "RET_SOURCE_TIMER_MSG"},
            {OLDER_HOST_PRESENT_TIMER_MSG, "OLDER_HOST_PRESENT_TIMER_MSG"},
            {GENERAL_QUERY_TIMER_MSG,      "GENERAL_QUERY_TIMER_MSG"     },
            {CONFIG_MSG,           "CONFIG_MSG"          },
            {GROUP_RECORD_MSG,     "GROUP_RECORD_MSG"    },
            {DEBUG_MSG,            "DEBUG_MSG"           }
        };
        return name_map[mt];
    }

    static std::string get_message_priority_name(message_priority mp) {
        std::map<proxy_msg::message_priority, std::string> name_map = {
            {SYSTEMIC,   "SYSTEMIC"  },
            {USER_INPUT, "USER_INPUT"},
            {LOSEABLE,   "LOSEABLE"  },
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
        return l.m_prio < r.m_prio;
    }

    friend bool operator> (const proxy_msg& l, const proxy_msg& r) {
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

struct comp_proxy_msg {
    bool operator()(const std::shared_ptr<proxy_msg>& l, const std::shared_ptr<proxy_msg>& r) const {
        return *l > *r;
    }
};

//------------------------------------------------------------------------
struct test_msg : public proxy_msg {
    test_msg(int value, message_priority prio): proxy_msg(TEST_MSG, prio), m_value(value) {
        HC_LOG_TRACE("");
    }

    virtual ~test_msg() {
        HC_LOG_TRACE("");
    }

    virtual void operator()() {
        HC_LOG_TRACE("");
        HC_LOG_DEBUG("Test Message value: " << m_value);
        HC_LOG_DEBUG("Test Message prio: " << get_priority());
        std::cout << "Test Message value: "  << m_value << " prio: " << proxy_msg::get_message_priority_name(get_priority()) << std::endl;
    }

private:
    int m_value;
};

//------------------------------------------------------------------------
struct timer_msg : public proxy_msg {
    timer_msg(message_type type, unsigned int if_index, const addr_storage& gaddr, const std::chrono::milliseconds& duration)
        : proxy_msg(type, SYSTEMIC)
        , m_if_index(if_index)
        , m_gaddr(gaddr)
        , m_end_time(std::chrono::monotonic_clock::now() + duration) {
        HC_LOG_TRACE("");
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    const addr_storage& get_gaddr() {
        return m_gaddr;
    }

    bool is_remaining_time_greater_than(std::chrono::milliseconds comp_time) {
        return (std::chrono::monotonic_clock::now() + comp_time) <= m_end_time;
    }

    std::string get_remaining_time() {
        using namespace std::chrono;
        std::ostringstream s;
        auto current_time = monotonic_clock::now();
        auto time_span = m_end_time - current_time;
        double seconds = time_span.count()  * monotonic_clock::period::num / monotonic_clock::period::den;
        if (seconds >= 0) {
            s << seconds << "sec";
        } else {
            s << "0sec";
        }
        return s.str();
    }

private:
    unsigned int m_if_index;
    addr_storage m_gaddr;
    std::chrono::time_point<std::chrono::monotonic_clock> m_end_time;
};

struct filter_timer_msg : public timer_msg {
    filter_timer_msg(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(FILTER_TIMER_MSG, if_index, gaddr, duration), m_is_used_as_source_timer(false) {
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

struct source_timer_msg : public timer_msg {
    source_timer_msg(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(SOURCE_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};

struct retransmit_group_timer_msg : public timer_msg {
    retransmit_group_timer_msg(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(RET_GROUP_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};

struct retransmit_source_timer_msg : public timer_msg {
    retransmit_source_timer_msg(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(RET_SOURCE_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};

struct older_host_present_timer_msg : public timer_msg {
    older_host_present_timer_msg(unsigned int if_index, const addr_storage& gaddr, std::chrono::milliseconds duration): timer_msg(OLDER_HOST_PRESENT_TIMER_MSG, if_index, gaddr, duration) {
        HC_LOG_TRACE("");
    }
};

struct general_query_timer_msg : public timer_msg {
    general_query_timer_msg(unsigned int if_index, std::chrono::milliseconds duration): timer_msg(GENERAL_QUERY_TIMER_MSG, if_index, addr_storage(), duration) {
        HC_LOG_TRACE("");
    }
};

struct new_source_timer_msg : public timer_msg {
    new_source_timer_msg(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr, std::chrono::milliseconds duration)
        : timer_msg(NEW_SOURCE_TIMER_MSG, if_index, gaddr, duration)
        , m_saddr(saddr)  {
        HC_LOG_TRACE("");
    }

    const addr_storage& get_saddr() {
        HC_LOG_TRACE("");
        return m_saddr;
    }

private:
    addr_storage m_saddr;
};

//------------------------------------------------------------------------

struct debug_msg : public proxy_msg {
    debug_msg(): proxy_msg(DEBUG_MSG, USER_INPUT) {
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
        , retransmission_count(-1) { /*not in a retransmission state*/
    }

    std::string to_string() const {
        std::ostringstream s;
        s << saddr;
        if ((shared_source_timer.get() != nullptr) && (retransmission_count >= 0)) {
            s << "(" << shared_source_timer->get_remaining_time() << "," << retransmission_count << "x)";
        } else if (shared_source_timer.get() != nullptr) {
            s << "(" << shared_source_timer->get_remaining_time() << ")";
        } else if (retransmission_count >= 0) {
            s << "(" << retransmission_count << "x)";
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

    addr_storage saddr;
    mutable std::shared_ptr<timer_msg> shared_source_timer;
    mutable long retransmission_count;
};

struct group_record_msg : public proxy_msg {
    //group_record_msg()
    //: group_record_msg(0, MODE_IS_INCLUDE, addr_storage(), source_list<source>(), IGMPv3) {}

    group_record_msg(unsigned int if_index, mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>&& slist, group_mem_protocol grp_mem_proto)
        : proxy_msg(GROUP_RECORD_MSG, LOSEABLE)
        , m_if_index(if_index)
        , m_record_type(record_type)
        , m_gaddr(gaddr)
        , m_slist(slist)
        , m_grp_mem_proto(grp_mem_proto){}

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
        s << "report version: " << get_group_mem_protocol_name(m_grp_mem_proto);
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

    group_mem_protocol get_grp_mem_proto() {
        return m_grp_mem_proto;
    }

private:
    unsigned int m_if_index;
    mcast_addr_record_type m_record_type;
    addr_storage m_gaddr;
    source_list<source> m_slist;
    group_mem_protocol m_grp_mem_proto;
};

struct new_source_msg : public proxy_msg {
    new_source_msg(unsigned int if_index, const addr_storage& gaddr, const addr_storage& saddr)
        : proxy_msg(NEW_SOURCE_MSG, LOSEABLE)
        , m_if_index(if_index)
        , m_gaddr(gaddr)
        , m_saddr(saddr) {
        HC_LOG_TRACE("");
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    const addr_storage& get_gaddr() {
        return m_gaddr;
    }

    const addr_storage& get_saddr() {
        return m_saddr;
    }

private:
    unsigned int m_if_index;
    addr_storage m_gaddr;
    addr_storage m_saddr;
};

//------------------------------------------------------------------------
struct config_msg : public proxy_msg {
    enum config_instruction {
        ADD_DOWNSTREAM,
        DEL_DOWNSTREAM,
        ADD_UPSTREAM,
        DEL_UPSTREAM,
        SET_GLOBAL_RULE_BINDING
    };

    config_msg(config_instruction instruction, unsigned int if_index, unsigned int upstream_priority, const std::shared_ptr<interface>& interf)
        : proxy_msg(CONFIG_MSG, SYSTEMIC)
        , m_instruction(instruction)
        , m_if_index(if_index)
        , m_upstream_priority(upstream_priority)
        , m_interface(interf)
        , m_tv(timers_values()) {
        if (instruction != DEL_DOWNSTREAM && instruction != ADD_UPSTREAM && instruction != DEL_UPSTREAM) {
            HC_LOG_ERROR("config_msg is incomplet, missing parameter timer_values");
            throw "config_msg is incomplet, missing parameter timer_values";
        }
    }

    config_msg(config_instruction instruction, unsigned int if_index, const std::shared_ptr<interface>& interf, const timers_values& tv)
        : proxy_msg(CONFIG_MSG, SYSTEMIC)
        , m_instruction(instruction)
        , m_if_index(if_index)
        , m_interface(interf)
        , m_tv(tv) {
        HC_LOG_TRACE("");
    }

    config_msg(config_instruction instruction, const std::shared_ptr<rule_binding>& rule_binding)
        : proxy_msg(CONFIG_MSG, SYSTEMIC)
        , m_instruction(instruction)
        , m_rule_binding(rule_binding) {
        HC_LOG_TRACE("");
    }

    config_instruction get_instruction() {
        return m_instruction;
    }

    const std::shared_ptr<interface>& get_interface() {
        return m_interface;
    }

    unsigned int get_if_index() {
        return m_if_index;
    }

    unsigned int get_upstream_priority() {
        return m_upstream_priority;
    }

    const timers_values& get_timers_values() {
        return m_tv;
    }

    const std::shared_ptr<rule_binding>& get_rule_binding() {
        return m_rule_binding;
    }

private:
    config_instruction m_instruction;
    unsigned int m_if_index;
    unsigned int m_upstream_priority;
    std::shared_ptr<interface> m_interface;
    timers_values m_tv;
    std::shared_ptr<rule_binding> m_rule_binding;
};

struct exit_cmd : public proxy_msg {
    exit_cmd(): proxy_msg(EXIT_MSG, USER_INPUT) {
        HC_LOG_TRACE("");
    }
};

#endif // MESSAGE_FORMAT_HPP
/** @} */
