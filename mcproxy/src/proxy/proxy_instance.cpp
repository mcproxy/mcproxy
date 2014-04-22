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
 *messg written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */


#include "include/hamcast_logging.h"
#include "include/proxy/proxy_instance.hpp"

#include "include/proxy/receiver.hpp"
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/mld_receiver.hpp"
#include "include/proxy/sender.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/mld_sender.hpp"
#include "include/proxy/routing.hpp"
#include "include/proxy/querier.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/routing_management.hpp"
#include "include/proxy/simple_mc_proxy_routing.hpp"

#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>

#include <unistd.h>
#include <net/if.h>

proxy_instance::proxy_instance(group_mem_protocol group_mem_protocol, const std::string& instance_name, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing, bool in_debug_testing_mode)
: m_group_mem_protocol(group_mem_protocol)
, m_instance_name(instance_name)
, m_table_number(table_number)
, m_in_debug_testing_mode(in_debug_testing_mode)
, m_interfaces(interfaces)
, m_timing(shared_timing)
, m_mrt_sock(nullptr)
, m_sender(nullptr)
, m_receiver(nullptr)
, m_routing(nullptr)
, m_proxy_start_time(std::chrono::monotonic_clock::now())
, m_upstream_input_rule(std::make_shared<rule_binding>(instance_name, IT_UPSTREAM, "*", ID_IN, RMT_FIRST, std::chrono::milliseconds(0)))
, m_upstream_output_rule(std::make_shared<rule_binding>(instance_name, IT_UPSTREAM, "*", ID_OUT, RMT_ALL, std::chrono::milliseconds(0)))
{

    //rule_binding(const std::string& instance_name, rb_interface_type interface_type, const std::string& if_name, rb_interface_direction filter_direction, rb_rule_matching_type rule_matching_type, const std::chrono::milliseconds& timeout);
    HC_LOG_TRACE("");

    if (!init_mrt_socket()) {
        throw "failed to initialize mroute socket";
    }

    if (!init_sender()) {
        throw "failed to initialize sender";
    }

    if (!init_receiver()) {
        throw "failed to initialise receiver";
    }

    if (!init_routing()) {
        throw "failed to initialise routing";
    }

    if (!init_routing_management()) {
        throw "failed to initialise routing";
    }

    start();
}

bool proxy_instance::init_mrt_socket()
{
    HC_LOG_TRACE("");
    m_mrt_sock = std::make_shared<mroute_socket>();
    if (is_IPv4(m_group_mem_protocol)) {
        m_mrt_sock->create_raw_ipv4_socket();
    } else if (is_IPv6(m_group_mem_protocol)) {
        m_mrt_sock->create_raw_ipv6_socket();
    } else {
        HC_LOG_ERROR("unknown ip version");
        return false;
    }

    if (m_table_number > 0) {
        if (!m_mrt_sock->set_kernel_table(m_table_number)) {
            return false;
        } else {
            HC_LOG_DEBUG("single proxy instance");
        }
    }

    if (!m_mrt_sock->set_mrt_flag(true)) {
        return false;
    }

    return true;
}

bool proxy_instance::init_sender()
{
    HC_LOG_TRACE("");
    if (is_IPv4(m_group_mem_protocol)) {
        m_sender = std::make_shared<igmp_sender>(m_interfaces);
    } else if (is_IPv6(m_group_mem_protocol)) {
        m_sender = std::make_shared<mld_sender>(m_interfaces);
    } else {
        HC_LOG_ERROR("unknown ip version");
        return false;
    }

    return true;
}

bool proxy_instance::init_receiver()
{
    HC_LOG_TRACE("");

    if (is_IPv4(m_group_mem_protocol)) {
        m_receiver.reset(new igmp_receiver(this, m_mrt_sock, m_interfaces, m_in_debug_testing_mode));
    } else if (is_IPv6(m_group_mem_protocol)) {
        m_receiver.reset(new mld_receiver(this, m_mrt_sock, m_interfaces, m_in_debug_testing_mode));
    } else {
        HC_LOG_ERROR("unknown ip version");
        return false;
    }

    return true;
}

bool proxy_instance::init_routing()
{
    HC_LOG_TRACE("");
    m_routing.reset(new routing(get_addr_family(m_group_mem_protocol), m_mrt_sock, m_interfaces, m_table_number));
    return true;
}

bool proxy_instance::init_routing_management()
{
    HC_LOG_TRACE("");
    m_routing_management.reset(new simple_mc_proxy_routing(this));
    return true;
}

proxy_instance::~proxy_instance()
{
    HC_LOG_TRACE("");
    add_msg(std::make_shared<exit_cmd>());
}

void proxy_instance::worker_thread()
{
    HC_LOG_TRACE("");
    while (m_running) {
        auto msg = m_job_queue.dequeue();
        switch (msg->get_type()) {
        case proxy_msg::TEST_MSG:
            (*msg)();
            break;
        case proxy_msg::CONFIG_MSG:
            handle_config(std::static_pointer_cast<config_msg>(msg));
            break;
        case proxy_msg::FILTER_TIMER_MSG:
        case proxy_msg::SOURCE_TIMER_MSG:
        case proxy_msg::RET_GROUP_TIMER_MSG:
        case proxy_msg::RET_SOURCE_TIMER_MSG:
        case proxy_msg::OLDER_HOST_PRESENT_TIMER_MSG:
        case proxy_msg::GENERAL_QUERY_TIMER_MSG: {
            auto it = m_downstreams.find(std::static_pointer_cast<timer_msg>(msg)->get_if_index());
            if (it != std::end(m_downstreams)) {
                it->second.m_querier->timer_triggerd(msg);
            } else {
                HC_LOG_DEBUG("failed to find querier of interface: " << interfaces::get_if_name(std::static_pointer_cast<timer_msg>(msg)->get_if_index()));
            }
        }
        break;
        case proxy_msg::GROUP_RECORD_MSG: {
            auto r =  std::static_pointer_cast<group_record_msg>(msg);

            if (m_in_debug_testing_mode) {
                std::cout << "!!--ACTION: receive record" << std::endl;
                std::cout << *r << std::endl;
                std::cout << std::endl;
            }

            auto it = m_downstreams.find(r->get_if_index());
            if (it != std::end(m_downstreams)) {
                it->second.m_querier->receive_record(msg);
            } else {
                HC_LOG_DEBUG("failed to find querier of interface: " << interfaces::get_if_name(std::static_pointer_cast<timer_msg>(msg)->get_if_index()));
            }
        }
        break;
        case proxy_msg::NEW_SOURCE_MSG:
            m_routing_management->event_new_source(msg);
            break;
        case proxy_msg::NEW_SOURCE_TIMER_MSG:
            m_routing_management->timer_triggerd_maintain_routing_table(msg);
            break;
        case proxy_msg::DEBUG_MSG:
            std::cout << *this << std::endl;
            std::cout << std::endl;
            break;
        case proxy_msg::EXIT_MSG:
            HC_LOG_DEBUG("received exit command");
            stop();
            break;
        default:
            HC_LOG_ERROR("Received unknown message");
            break;
        }
    }

    HC_LOG_DEBUG("worker thread proxy_instance end");
}

std::string proxy_instance::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;

    auto current_time = std::chrono::monotonic_clock::now();
    auto time_span = current_time - m_proxy_start_time;
    double seconds = time_span.count()  * std::chrono::monotonic_clock::period::num / std::chrono::monotonic_clock::period::den;

    s << "@@##-- proxy instance " << m_instance_name << " (table:" << m_table_number << ",lifetime:" << seconds << "sec)" << " --##@@" << std::endl;;
    s << m_upstream_input_rule->to_string() << std::endl;
    s << m_upstream_output_rule->to_string() << std::endl;

    s << *m_routing_management << std::endl;

    s << "##-- upstream interfaces --##" << std::endl;
    for (auto & e : m_upstreams) {
        s << interfaces::get_if_name(e.m_if_index) << "(index:" << e.m_if_index << ") ";
    }
    s << std::endl;

    for (auto it = std::begin(m_downstreams); it != std::end(m_downstreams); ++it) {
        s << std::endl << *it->second.m_querier;
    }
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const proxy_instance& pr_i)
{
    return stream << pr_i.to_string();
}

void proxy_instance::handle_config(const std::shared_ptr<config_msg>& msg)
{
    HC_LOG_TRACE("");

    switch (msg->get_instruction()) {
    case config_msg::ADD_DOWNSTREAM: {

        if (m_downstreams.find(msg->get_if_index()) == std::end(m_downstreams) ) {
            HC_LOG_DEBUG("register downstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            //register interface
            if (!is_upstream(msg->get_if_index())) {
                m_routing->add_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
                m_receiver->registrate_interface(msg->get_if_index());
            } else {
                HC_LOG_DEBUG("interface also used as upstream");
            }

            //create a querier
            std::function<void(unsigned int, const addr_storage&)> cb_state_change = std::bind(&routing_management::event_querier_state_change, m_routing_management.get(), std::placeholders::_1, std::placeholders::_2);
            std::unique_ptr<querier> q(new querier(this, m_group_mem_protocol, msg->get_if_index(), m_sender, m_timing, msg->get_timers_values(), cb_state_change));
            m_downstreams.insert(std::pair<unsigned int, downstream_infos>(msg->get_if_index(), downstream_infos(move(q), msg->get_interface())));
        } else {
            HC_LOG_WARN("downstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " already exists");
        }
    }
    break;
    case config_msg::DEL_DOWNSTREAM: {
        auto it = m_downstreams.find(msg->get_if_index());
        if (it != std::end(m_downstreams)) {
            HC_LOG_DEBUG("del downstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            //unregister interface
            if (!is_upstream(msg->get_if_index())) {
                m_routing->del_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
                m_receiver->del_interface(msg->get_if_index());
            } else {
                HC_LOG_DEBUG("interface still used as upstream");
            }

            //delete querier
            m_downstreams.erase(it);
        } else {
            HC_LOG_WARN("failed to delete downstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " interface not found");
        }
    }
    break;
    case config_msg::ADD_UPSTREAM: {
        //register interface
        
        if (std::find_if(m_upstreams.begin(), m_upstreams.end(), [&](const upstream_infos & ui) {
        return ui.m_if_index == msg->get_if_index();
        } ) == m_upstreams.end()) {
            HC_LOG_DEBUG("register upstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            if (!is_downstream(msg->get_if_index())) {
                m_routing->add_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
                m_receiver->registrate_interface(msg->get_if_index());
            } else {
                HC_LOG_DEBUG("interface also used as downstream");
            }

            HC_LOG_DEBUG("registerd upstreams: " << m_upstreams.size());
            HC_LOG_DEBUG("upstream priority: " << msg->get_upstream_priority());
            m_upstreams.insert(upstream_infos(msg->get_if_index(), msg->get_interface(), msg->get_upstream_priority()));
        }
        else {
            HC_LOG_WARN("upstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " already exists");
        }
    }
    break;
    case config_msg::DEL_UPSTREAM: {
        //unregister interface
        auto it = std::find_if(m_upstreams.begin(), m_upstreams.end(), [&](const upstream_infos & ui) {
            return ui.m_if_index == msg->get_if_index();
        } );

        if (it != m_upstreams.end()) {
            HC_LOG_DEBUG("del upstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            if (!is_downstream(msg->get_if_index())) {
                m_routing->del_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
                m_receiver->del_interface(msg->get_if_index());
            } else {
                HC_LOG_DEBUG("interface still used as downstream");
            }

            m_upstreams.erase(it);
        } else {
            HC_LOG_WARN("failed to delete upstream interface: " << interfaces::get_if_name(msg->get_if_index()) << " interface not found");
        }
    }
    break;
    case config_msg::SET_GLOBAL_RULE_BINDING: {
        auto rb = msg->get_rule_binding();
        if (rb != nullptr) {
            if (rb->get_rule_binding_type() == RBT_RULE_MATCHING) {
                if (rb->get_interface_type() == IT_UPSTREAM) {
                    if (rb->get_interface_direction() == ID_IN) {
                        m_upstream_input_rule = rb;
                    } else if (rb->get_interface_direction() == ID_OUT) {
                        m_upstream_output_rule = rb;
                    } else {
                        HC_LOG_ERROR("failed to set global rule binding, interface direction not defined");
                    }
                } else {
                    HC_LOG_ERROR("failed to set global rule binding, wrong interface type");
                }
            } else {
                HC_LOG_ERROR("failed to set global rule binding, unknown rule binding type");
            }
        } else {
            HC_LOG_ERROR("failed to set global rule binding, rule not defined");
        }
    }
    break;
    default:
        HC_LOG_ERROR("unknown config message format");
    }
}

bool proxy_instance::is_upstream(unsigned int if_index) const
{
    HC_LOG_TRACE("");

    for (auto & e : m_upstreams) {
        if (e.m_if_index == if_index) {
            return true;
        }
    }

    return false;
}

bool proxy_instance::is_downstream(unsigned int if_index) const
{
    HC_LOG_TRACE("");

    return m_downstreams.find(if_index) != m_downstreams.end();
}

#ifdef DEBUG_MODE
void proxy_instance::test_querier(std::string if_name)
{
    using namespace std;
    addr_storage gaddr("239.99.99.99");

    group_mem_protocol memproto = IGMPv3;
    //create a proxy_instance
    proxy_instance pr_i(memproto, "test", 0,  make_shared<interfaces>(get_addr_family(memproto), false), make_shared<timing>(), true);

    //add a downstream
    timers_values tv;

    //set mali to 10 seconds
    tv.set_query_interval(chrono::seconds(10));
    tv.set_startup_query_interval(chrono::seconds(10));
    tv.set_startup_query_count(4);
    tv.set_query_response_interval(chrono::seconds(4));

    cout << tv << endl;
    cout << endl;

    pr_i.add_msg(make_shared<config_msg>(config_msg::ADD_DOWNSTREAM, interfaces::get_if_index(if_name), std::make_shared<interface>(if_name), tv));

    auto print_proxy_instance = bind(&proxy_instance::add_msg, &pr_i, make_shared<debug_msg>());

    auto __tmp = [&, if_name](mcast_addr_record_type t, source_list<source> && slist, group_mem_protocol gmp) {
        return make_shared<group_record_msg>(interfaces::get_if_index(if_name), t, gaddr, move(slist), gmp);
    };

    auto send_record = bind(&proxy_instance::add_msg, &pr_i, bind(__tmp, placeholders::_1, placeholders::_2, placeholders::_3));

    //-----------------------------------------------------------------
    //quick_test(send_record, print_proxy_instance);
    rand_test(send_record, print_proxy_instance);
    //test_a(send_record, print_proxy_instance);
    //test_b(send_record, print_proxy_instance);
    //test_c(send_record, print_proxy_instance);
    //test_d(send_record, print_proxy_instance);
    //test_backward_compatibility(send_record, print_proxy_instance);
    //test_backward_compatibility(send_record, print_proxy_instance);
}

void proxy_instance::quick_test(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    cout << "##-- querier quick test --##" << endl;
    print_proxy_instance();

    send_record(MODE_IS_INCLUDE, source_list<source> {}, IGMPv3);
    send_record(MODE_IS_INCLUDE, source_list<source> {}, IGMPv3);

    //send_record(MODE_IS_INCLUDE, source_list<source> {});

    for (int i = 0; i < 120; ++i) {
        sleep(1);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_a(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    source s6(addr_storage("6.6.6.6"));
    source s7(addr_storage("7.7.7.7"));

    cout << "##-- querier test A --##" << endl;
    print_proxy_instance();

    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s2, s3, s4, s5}, IGMPv3);
    print_proxy_instance();

    sleep(4);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s3, s5}, IGMPv3);
    print_proxy_instance();

    sleep(5);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s3, s5}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s1, s2, s3}, IGMPv3);
    print_proxy_instance();

    sleep(5);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s2}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s6, s7}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s6, s7}, IGMPv3);
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_b(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{

    using namespace std;
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    source s6(addr_storage("6.6.6.6"));
    source s7(addr_storage("7.7.7.7"));

    cout << "##-- querier test B --##" << endl;
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2, s3}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(BLOCK_OLD_SOURCES, source_list<source> {s2}, IGMPv3);
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_c(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    source s6(addr_storage("6.6.6.6"));
    source s7(addr_storage("7.7.7.7"));

    cout << "##-- querier test C --##" << endl;
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2, s3, s4}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(CHANGE_TO_EXCLUDE_MODE, source_list<source> {s3, s4, s5, s6}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s7}, IGMPv3);
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s6}, IGMPv3);
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_d(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    source s6(addr_storage("6.6.6.6"));
    source s7(addr_storage("7.7.7.7"));

    cout << "##-- querier test D --##" << endl;
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s2, s3}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(BLOCK_OLD_SOURCES, source_list<source> {s2, s7}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(CHANGE_TO_INCLUDE_MODE, source_list<source> {s1, s2}, IGMPv3);
    print_proxy_instance();

    sleep(4);
    send_record(CHANGE_TO_EXCLUDE_MODE, source_list<source> {s1, s7}, IGMPv3);
    print_proxy_instance();
    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_backward_compatibility(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    source s6(addr_storage("6.6.6.6"));
    source s7(addr_storage("7.7.7.7"));

    cout << "##-- querier test BACKWARD COMPATIBILITY --##" << endl;
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s3, s2}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s4, s2}, IGMPv2);
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s5, s2}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s6, s2}, IGMPv3);
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s7, s2}, IGMPv2);
    print_proxy_instance();

    sleep(4);
    for (int i = 0; i < 23; ++i) {
        sleep(2);
        send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2}, IGMPv3);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::rand_test(std::function < void(mcast_addr_record_type, source_list<source>&&, group_mem_protocol) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    default_random_engine re;
    int time;

    vector<mcast_addr_record_type> r_type {MODE_IS_INCLUDE, MODE_IS_EXCLUDE, CHANGE_TO_INCLUDE_MODE, CHANGE_TO_EXCLUDE_MODE, ALLOW_NEW_SOURCES, BLOCK_OLD_SOURCES};
    uniform_int_distribution<int> d_r_type(0, r_type.size() - 1);

    vector<source> src;
    src.push_back(addr_storage("1.1.1.1"));
    src.push_back(addr_storage("2.2.2.2"));
    src.push_back(addr_storage("3.3.3.3"));
    src.push_back(addr_storage("4.4.4.4"));
    src.push_back(addr_storage("5.5.5.5"));
    src.push_back(addr_storage("6.6.6.6"));
    src.push_back(addr_storage("7.7.7.7"));
    src.push_back(addr_storage("8.8.8.8"));
    src.push_back(addr_storage("9.9.9.9"));
    src.push_back(addr_storage("10.10.10.10"));
    src.push_back(addr_storage("11.11.11.11"));
    src.push_back(addr_storage("12.12.12.12"));
    uniform_int_distribution<int> d_src(0, 8);

    cout << "##-- querier random test --##" << endl;
    print_proxy_instance();

    auto get_src_list = [&]() {
        source_list<source> s;
        int max = d_src(re);
        for (int i = 0; i <= max; i++) {
            s.insert(src[d_src(re)]);
        }
        return s;
    };

    for (int i = 0; i < 50; i++) {
        time = d_src(re);
        cout << "!!--wait: " << time << "sec" << endl;
        cout << endl;

        if (i == 10 || i == 20 || i == 40 ) {
            sleep(2);
            send_record(MODE_IS_EXCLUDE, {}, IGMPv2); 
        }

        if (i == 11 || i == 25 || i == 40 ) {
            sleep(2);
            send_record(CHANGE_TO_EXCLUDE_MODE, {}, IGMPv2); 
        }

        sleep(time);
        send_record(r_type[d_r_type(re)], get_src_list(), IGMPv3);
        print_proxy_instance();
    }
    sleep(1);
    cout << "##-- querier end --##" << endl;
}
#endif /* DEBUG_MODE */
