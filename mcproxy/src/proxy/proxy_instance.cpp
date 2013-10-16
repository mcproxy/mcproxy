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

#include <sstream>
#include <iostream>
#include <functional>

#include <unistd.h>
#include <net/if.h>

proxy_instance::proxy_instance(group_mem_protocol group_mem_protocol, int table_number, const std::shared_ptr<const interfaces>& interfaces, const std::shared_ptr<timing>& shared_timing)
    : m_group_mem_protocol(group_mem_protocol)
    , m_table_number(table_number)
    , m_interfaces(interfaces)
    , m_timing(shared_timing)
    , m_mrt_sock(nullptr)
    , m_sender(nullptr)
    , m_receiver(nullptr)
    , m_routing(nullptr)
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
        m_receiver.reset(new igmp_receiver(this, m_mrt_sock, m_interfaces));
    } else if (is_IPv6(m_group_mem_protocol)) {
        m_receiver.reset(new mld_receiver(this, m_mrt_sock, m_interfaces));
    } else {
        HC_LOG_ERROR("unknown ip version");
        return false;
    }

    return true;
}

bool proxy_instance::init_routing()
{
    HC_LOG_TRACE("");
    m_routing.reset(new routing(get_addr_family(m_group_mem_protocol), m_mrt_sock, m_table_number));
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
        case proxy_msg::RET_FILTER_TIMER_MSG:
        case proxy_msg::RET_SOURCE_TIMER_MSG: {
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
            auto it = m_querier.find(r->get_if_index());
            if (it != std::end(m_querier)) {
                it->second->receive_record(msg);
            } else {
                HC_LOG_DEBUG("failed to find querier of interface: " << interfaces::get_if_name(std::static_pointer_cast<timer_msg>(msg)->get_if_index()));
            }
        }
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
    s << "@@##-- proxy instance: " << m_table_number << " --##@@";
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
        std::unique_ptr<querier> q(new querier(this, m_group_mem_protocol, msg->get_if_index(), m_sender, m_timing));
        m_querier.insert(std::pair<int, std::unique_ptr<querier>>(msg->get_if_index(), move(q)));
    }
    break;
    case config_msg::DEL_DOWNSTREAM: {
        auto it = m_querier.find(msg->get_if_index());
        if (it != std::end(m_querier)) {
            m_querier.erase(it);
        } else {
            HC_LOG_ERROR("failed to delete downstream interface: " << interfaces::get_if_name(msg->get_if_index()));

        }
    }
    break;
    case config_msg::ADD_UPSTREAM:
        break;
    case config_msg::DEL_UPSTREAM:
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
    source s1(addr_storage("1.1.1.1"));
    source s2(addr_storage("2.2.2.2"));
    source s3(addr_storage("3.3.3.3"));
    source s4(addr_storage("4.4.4.4"));
    source s5(addr_storage("5.5.5.5"));
    addr_storage gaddr("224.1.1.1");

    group_mem_protocol memproto = IGMPv3; 
    //create a proxy_instance
    proxy_instance pr_i(memproto, 0,  make_shared<interfaces>(get_addr_family(memproto), false), make_shared<timing>());
    //add a downstream
    pr_i.add_msg(make_shared<config_msg>(config_msg::ADD_DOWNSTREAM, interfaces::get_if_index(if_name)));

    {
        //set mali to 10 seconds
        sleep(1);
        querier* q = pr_i.m_querier.find(interfaces::get_if_index(if_name))->second.get();
        q->get_timers_values().set_query_interval(chrono::seconds(4));
        q->get_timers_values().set_query_response_interval(chrono::seconds(2));
    }


    auto print_proxy_instance = bind(&proxy_instance::add_msg, &pr_i, make_shared<debug_msg>());
    
    
    auto __tmp =[&, if_name](mcast_addr_record_type t, source_list<source>&& slist){return make_shared<group_record_msg>(interfaces::get_if_index(if_name), t, gaddr, move(slist), 0);};

    auto send_record = bind(&proxy_instance::send_test_record, &pr_i, bind(__tmp,placeholders::_1, placeholders::_2));

    //-----------------------------------------------------------------
    cout << "##-- querier test on interface " << if_name << " --##" << endl;
    print_proxy_instance();

    send_record(MODE_IS_INCLUDE, source_list<source> {s1, s2});
    print_proxy_instance();

    sleep(2);
    send_record(ALLOW_NEW_SOURCES, source_list<source>{s1});
    print_proxy_instance();

    //sleep(2);
    //send_record(MODE_IS_EXCLUDE, source_list<source>{s2,s3,s4});
    //print_proxy_instance();

    //sleep(2);
    //send_record(MODE_IS_INCLUDE, source_list<source>{s4});
    //print_proxy_instance();

    for (int i = 0; i < 6; ++i) {
        sleep(2);
        print_proxy_instance();
    }
    sleep(1);
    cout << "##-- querier end --##" << endl;
}

void proxy_instance::send_test_record(proxy_instance* const pr_i, std::shared_ptr<group_record_msg> m)
{
    std::cout << "!!--ACTION: receive record" << std::endl;
    std::cout << *m << std::endl;
    std::cout << std::endl;
    pr_i->add_msg(m);
}
