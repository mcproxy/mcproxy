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

#include <unistd.h>
#include <net/if.h>

proxy_instance::proxy_instance(group_mem_protocol group_mem_protocol, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing, bool in_debug_testing_mode)
    : m_group_mem_protocol(group_mem_protocol)
    , m_table_number(table_number)
    , m_in_debug_testing_mode(in_debug_testing_mode)
    , m_interfaces(interfaces)
    , m_timing(shared_timing)
    , m_mrt_sock(nullptr)
    , m_sender(nullptr)
    , m_receiver(nullptr)
    , m_routing(nullptr)
    , m_upstream(0)
{
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
        m_sender = std::make_shared<igmp_sender>();
    } else if (is_IPv6(m_group_mem_protocol)) {
        m_sender = std::make_shared<mld_sender>();
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
        case proxy_msg::GENERAL_QUERY_MSG: {
            auto it = m_querier.find(std::static_pointer_cast<timer_msg>(msg)->get_if_index());
            if (it != std::end(m_querier)) {
                it->second->timer_triggerd(msg);
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

            auto it = m_querier.find(r->get_if_index());
            if (it != std::end(m_querier)) {
                it->second->receive_record(msg);
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

    ////DEBUG output
    //HC_LOG_DEBUG("initiate GQ timer; pointer: " << m.msg);

    ////##-- thread working loop --##
    //while (m_running) {
    //m = m_job_queue.dequeue();
    //HC_LOG_DEBUG("received new job. type: " << m.msg_type_to_string());
    //switch (m.type) {
    //case proxy_msg::TEST_MSG: {
    //m();
    //break;
    //}
    //case proxy_msg::RECEIVER_MSG: {
    //struct receiver_msg* t = (struct receiver_msg*) m.msg.get();
    //handle_igmp(t);
    //break;
    //}
    //case proxy_msg::CLOCK_MSG: {
    //struct clock_msg* t = (struct clock_msg*) m.msg.get();
    //handle_clock(t);
    //break;
    //}
    //case proxy_msg::CONFIG_MSG: {
    //struct config_msg* t = (struct config_msg*) m.msg.get();
    //handle_config(t);
    //break;
    //}
    //case proxy_msg::DEBUG_MSG: {
    //struct debug_msg* t = (struct debug_msg*) m.msg.get();
    //handle_debug_msg(t);
    //break;
    //}

    //case proxy_msg::EXIT_CMD:
    //m_running = false;
    //break;
    //default:
    //HC_LOG_ERROR("unknown message format");
    //}
    //}

    ////##-- timing --##
    ////remove all running times
    //m_timing->stop_all_time(this);

    HC_LOG_DEBUG("worker thread proxy_instance end");
}

std::string proxy_instance::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << "@@##-- proxy instance: " << m_table_number << " --##@@" << std::endl;;
    s << *m_routing_management << std::endl;
    for (auto it = std::begin(m_querier); it != std::end(m_querier); ++it) {
        s << std::endl << *it->second;
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

        if (m_querier.find(msg->get_if_index()) == std::end(m_querier) ) {
            //register interface
            m_routing->add_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
            m_receiver->registrate_interface(msg->get_if_index());
            HC_LOG_DEBUG("register interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            //create a querier
            std::function<void(unsigned int, const addr_storage&, const source_list<source>&)> cb_state_change = std::bind(&routing_management::event_querier_state_change, m_routing_management.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            std::unique_ptr<querier> q(new querier(this, m_group_mem_protocol, msg->get_if_index(), m_sender, m_timing, msg->get_timers_values(), cb_state_change));
            m_querier.insert(std::pair<int, std::unique_ptr<querier>>(msg->get_if_index(), move(q)));
        } else {
            HC_LOG_WARN("querier for interface: " << interfaces::get_if_name(msg->get_if_index()) << " allready exists");
        }
    }
    break;
    case config_msg::DEL_DOWNSTREAM: {
        auto it = m_querier.find(msg->get_if_index());
        if (it != std::end(m_querier)) {
            //delete querier
            m_querier.erase(it);

            //unregister interface
            m_routing->del_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
            m_receiver->del_interface(msg->get_if_index());
        } else {
            HC_LOG_WARN("failed to delete downstream interface: " << interfaces::get_if_name(msg->get_if_index()));
        }
    }
    break;
    case config_msg::ADD_UPSTREAM:
        if (m_upstream == 0) {
            //register interface
            m_routing->add_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
            m_receiver->registrate_interface(msg->get_if_index());
            HC_LOG_DEBUG("register interface: " << interfaces::get_if_name(msg->get_if_index()) << " with virtual interface index: " << m_interfaces->get_virtual_if_index(msg->get_if_index()));

            m_upstream = msg->get_if_index();
        }
        break;
    case config_msg::DEL_UPSTREAM:
        if (m_upstream != 0) {
            //unregister interface
            m_routing->del_vif(msg->get_if_index(), m_interfaces->get_virtual_if_index(msg->get_if_index()));
            m_receiver->del_interface(msg->get_if_index());

            m_upstream = 0;
        }
        break;
    default:
        HC_LOG_ERROR("unknown config message format");
    }
}

//void proxy_instance::handle_igmp(struct receiver_msg* r)
//{
//HC_LOG_TRACE("");

//switch (r->type) {
//case receiver_msg::JOIN:
//break;
//case receiver_msg::LEAVE:
//break;
//case receiver_msg::CACHE_MISS:
//break;
//default:
//HC_LOG_ERROR("unknown receiver messge format");
//}
//}

//void proxy_instance::handle_clock(struct clock_msg* c)
//{
//HC_LOG_TRACE("");

////switch (c->type) {
////case clock_msg::SEND_GQ_TO_ALL:
////break;
////case clock_msg::SEND_GSQ:
////break;
////case clock_msg::DEL_GROUP:
////break;
////case clock_msg::SEND_GQ:
////break; //start up Query Interval vor new interfaces
////default:
////HC_LOG_ERROR("unknown clock message foramt");
////}
//}


//void proxy_instance::handle_debug_msg(struct debug_msg* db)
//{
//HC_LOG_TRACE("");
//db->add_debug_msg(string());
//}

//bool proxy_instance::init(, int upstream_index, int upstream_vif, int downstream_index, int downstram_vif, )
//{
//HC_LOG_TRACE("");

////m_upstream = upstream_index;
////m_table_number = upstream_index;
////m_vif_map.insert(vif_pair(upstream_index, upstream_vif));

////m_vif_map.insert(vif_pair(downstream_index, downstram_vif));


////m_check_source.init(m_addr_family, &m_mrt_sock);


//if(!init_routing()){
//return false;
//}


//return false;
//}


void proxy_instance::test_querier(std::string if_name)
{
    using namespace std;
    addr_storage gaddr("239.99.99.99");

    group_mem_protocol memproto = IGMPv3;
    //create a proxy_instance
    proxy_instance pr_i(memproto, 0,  make_shared<interfaces>(get_addr_family(memproto), false), make_shared<timing>(), true);

    //add a downstream
    timers_values tv;

    //set mali to 10 seconds
    tv.set_query_interval(chrono::seconds(10));
    tv.set_startup_query_interval(chrono::seconds(10));
    tv.set_startup_query_count(3);
    tv.set_query_response_interval(chrono::seconds(4));

    cout << tv << endl;
    cout << endl;

    pr_i.add_msg(make_shared<config_msg>(config_msg::ADD_DOWNSTREAM, interfaces::get_if_index(if_name), tv));

    auto print_proxy_instance = bind(&proxy_instance::add_msg, &pr_i, make_shared<debug_msg>());


    auto __tmp = [&, if_name](mcast_addr_record_type t, source_list<source> && slist) {
        return make_shared<group_record_msg>(interfaces::get_if_index(if_name), t, gaddr, move(slist), 0);
    };

    auto send_record = bind(&proxy_instance::add_msg, &pr_i, bind(__tmp, placeholders::_1, placeholders::_2));

    //-----------------------------------------------------------------
    quick_test(send_record, print_proxy_instance);
    //rand_test(send_record, print_proxy_instance);
    //test_a(send_record,print_proxy_instance);
    //test_b(send_record, print_proxy_instance);
    //test_c(send_record, print_proxy_instance);
    //test_d(send_record, print_proxy_instance);
}

void proxy_instance::quick_test(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
{
    using namespace std;
    cout << "##-- querier quick test --##" << endl;
    print_proxy_instance();

    send_record(MODE_IS_INCLUDE, source_list<source> {});
    send_record(MODE_IS_INCLUDE, source_list<source> {});

    //send_record(MODE_IS_INCLUDE, source_list<source> {});

    for (int i = 0; i < 120; ++i) {
        sleep(1);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_a(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
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

    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s2, s3, s4, s5});
    print_proxy_instance();

    sleep(4);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s3, s5});
    print_proxy_instance();

    sleep(5);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s3, s5});
    print_proxy_instance();

    sleep(2);
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s1, s2, s3});
    print_proxy_instance();

    sleep(5);
    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s2});
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s6, s7});
    print_proxy_instance();

    sleep(1);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s6, s7});
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_b(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
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
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2, s3});
    print_proxy_instance();

    sleep(1);
    send_record(BLOCK_OLD_SOURCES, source_list<source> {s2});
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_c(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
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
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2, s3, s4});
    print_proxy_instance();

    sleep(1);
    send_record(CHANGE_TO_EXCLUDE_MODE, source_list<source> {s3, s4, s5, s6});
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s7});
    print_proxy_instance();

    sleep(1);
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s6});
    print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::test_d(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
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
    send_record(ALLOW_NEW_SOURCES, source_list<source> {s1, s2});
    print_proxy_instance();

    sleep(2);
    send_record(MODE_IS_EXCLUDE, source_list<source> {s2, s3});
    print_proxy_instance();

    sleep(2);
    send_record(BLOCK_OLD_SOURCES, source_list<source> {s2, s7});
    print_proxy_instance();

    sleep(2);
    send_record(CHANGE_TO_INCLUDE_MODE, source_list<source> {s1, s2});
    print_proxy_instance();

    sleep(4);
    send_record(CHANGE_TO_EXCLUDE_MODE, source_list<source> {s1, s7});
    print_proxy_instance();
    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }

    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::rand_test(std::function < void(mcast_addr_record_type, source_list<source>&&) > send_record, std::function<void()> print_proxy_instance)
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

        sleep(time);
        send_record(r_type[d_r_type(re)], get_src_list());
        print_proxy_instance();
    }
    sleep(1);
    cout << "##-- querier end --##" << endl;
}
