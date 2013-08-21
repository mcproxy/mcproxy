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
#include "include/proxy/receiver.hpp"

#include <memory>
#include <iostream>
using namespace std;

receiver::receiver():
    m_running(false), m_worker_thread(0)
{
    HC_LOG_TRACE("");
}

receiver::~receiver()
{
    HC_LOG_TRACE("");
    close();
}

void receiver::close()
{
    HC_LOG_TRACE("");
    delete m_worker_thread;
}

bool receiver::init_if_prop()
{
    HC_LOG_TRACE("");

    //if(!m_if_prop.init_IfInfo()) return false;
    //if(!m_if_prop.refresh_network_interfaces()) return false;
    m_if_property.refresh_network_interfaces();

    return true;
}

bool receiver::init(int addr_family, int version, mroute_socket* mrt_sock)
{
    HC_LOG_TRACE("");

    m_addr_family = addr_family;
    m_version =  version;
    m_mrt_sock = mrt_sock;

    if (!init_if_prop()) {
        return false;
    }
    if (!m_mrt_sock->set_receive_timeout(RECEIVER_RECV_TIMEOUT)) {
        return false;
    }
    //if(!m_mrt_sock->setLoopBack(true)) return false;

    return true;
}

proxy_instance* receiver::get_proxy_instance(int if_index)
{
    HC_LOG_TRACE("");
    if_poxy_instance_map::iterator it =  m_if_proxy_map.find(if_index);
    if (it != m_if_proxy_map.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

void receiver::registrate_interface(int if_index, int vif, proxy_instance* p)
{
    HC_LOG_TRACE("");

    boost::lock_guard<boost::mutex> lock(m_data_lock);
    m_if_proxy_map.insert(if_proxy_instance_pair(if_index, p));

    m_vif_map.insert(vif_pair(vif, if_index));
}

void receiver::del_interface(int if_index, int vif)
{
    HC_LOG_TRACE("");

    boost::lock_guard<boost::mutex> lock(m_data_lock);
    m_if_proxy_map.erase(if_index);
    m_vif_map.erase(vif);
}

int receiver::get_if_index(int vif)
{
    HC_LOG_TRACE("");

    vif_map::iterator it =  m_vif_map.find(vif);
    if (it != m_vif_map.end()) {
        return it->second;
    } else {
        return 0;
    }
}

void receiver::worker_thread(void* arg)
{
    HC_LOG_TRACE("");

    receiver* r = (receiver*) arg;
    int info_size = 0;

    //########################
    //create msg
    //iov

    unique_ptr<unsigned char[]> iov_buf { new unsigned char[r->get_iov_min_size()] };
    //unsigned char iov_buf[r->get_iov_min_size()];
    struct iovec iov;
    iov.iov_base = iov_buf.get();
    iov.iov_len = r->get_iov_min_size(); //sizeof(iov_buf);

    //control
    unique_ptr<unsigned char[]> ctrl { new unsigned char[r->get_ctrl_min_size()] };
    //unsigned char ctrl[r->get_ctrl_min_size()];

    //create msghdr
    struct msghdr msg;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = ctrl.get();
    msg.msg_controllen = r->get_iov_min_size(); //sizeof(ctrl);

    msg.msg_flags = 0;
    //########################

    while (r->m_running) {
        if (!r->m_mrt_sock->receive_msg(&msg, info_size)) {
            HC_LOG_ERROR("received failed");
            sleep(1);
            continue;
        }
        if (info_size == 0) {
            continue; //on timeout
        }
        r->m_data_lock.lock();
        r->analyse_packet(&msg, info_size);
        r->m_data_lock.unlock();
    }
}

bool receiver::is_running()
{
    HC_LOG_TRACE("");
    return m_running;
}

void receiver::start()
{
    HC_LOG_TRACE("");

    m_running =  true;
    m_worker_thread =  new boost::thread(receiver::worker_thread, this);
}

void receiver::stop()
{
    HC_LOG_TRACE("");

    m_running = false;
}

void receiver::join()
{
    HC_LOG_TRACE("");

    if (m_worker_thread) {
        m_worker_thread->join();
    }
}
