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


#include "include/hamcast_logging.h"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/mld_receiver.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/mld_sender.hpp"

#include <net/if.h>
#include <sstream>

proxy_instance::proxy_instance(int addr_family, int table_number, const std::shared_ptr<const interfaces> interfaces)
    : m_addr_family(addr_family)
    , m_table_number(table_number)
    , m_interfaces(interfaces)
    , m_mrt_sock(nullptr)
    , m_timing(nullptr)
    , m_sender(nullptr)
    , m_receiver(nullptr)
    , m_routing(nullptr)
{
    HC_LOG_TRACE("");

    if (!init_mrt_socket()) {
        throw "failed to initialize mroute socket";
    }

    m_timing = make_shared<timing>();

    if (!init_sender()) {
       throw "failed to initialize sender"; 
    }
    
    if (!init_receiver()) {
       throw "failed to initialise receiver";
    }

    if (!init_routing()) {
       throw "failed to initialise routing";
    }
}

bool proxy_instance::init_mrt_socket()
{
    HC_LOG_TRACE("");
    m_mrt_sock = std::make_shared<mroute_socket>();
    if (m_addr_family == AF_INET) {
        m_mrt_sock->create_raw_ipv4_socket();
    } else if (m_addr_family == AF_INET6) {
        m_mrt_sock->create_raw_ipv6_socket();
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
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
    if (m_addr_family == AF_INET) {
        m_sender = make_shared<igmp_sender>();
    } else if (m_addr_family == AF_INET6) {
        m_sender = make_shared<mld_sender>();
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    return true;
}

bool proxy_instance::init_receiver()
{
    HC_LOG_TRACE("");

    if (m_addr_family == AF_INET) {
        m_receiver.reset(new igmp_receiver(m_mrt_sock, m_interfaces));
    } else if (m_addr_family == AF_INET6) {
        m_receiver.reset(new mld_receiver(m_mrt_sock,m_interfaces));
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    return true;
}

bool proxy_instance::init_routing()
{
    HC_LOG_TRACE("");
    m_routing.reset(new routing(m_addr_family, m_mrt_sock, m_table_number));
    return true;
}

proxy_instance::~proxy_instance()
{
    HC_LOG_TRACE("");
}







void proxy_instance::worker_thread()
{
    HC_LOG_TRACE("");

    //proxy_msg m;

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

//void proxy_instance::handle_config(struct config_msg* c)
//{
//HC_LOG_TRACE("");

//switch (c->type) {
//case config_msg::ADD_DOWNSTREAM:
//break;
//case config_msg::DEL_DOWNSTREAM:
//break;
//case config_msg::SET_UPSTREAM:
//break;
//default:
//HC_LOG_ERROR("unknown config message format");
//}
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
