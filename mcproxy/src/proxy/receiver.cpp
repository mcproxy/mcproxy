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

#include <unistd.h>

receiver::receiver(proxy_instance* pr_i, int addr_family, const std::shared_ptr<const mroute_socket> mrt_sock, const std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode)
    : m_running(false)
    , m_in_debug_testing_mode(in_debug_testing_mode)
    , m_thread(nullptr)
    , m_proxy_instance(pr_i)
    , m_addr_family(addr_family)
    , m_mrt_sock(mrt_sock)
    , m_interfaces(interfaces)
{
    HC_LOG_TRACE("");

    if (!m_mrt_sock->set_receive_timeout(RECEIVER_RECV_TIMEOUT)) {
        throw std::string("failed to set receive timeout");
    }

}

receiver::~receiver()
{
    HC_LOG_TRACE("");
    stop();
    join();
}

bool receiver::is_if_index_relevant(unsigned int if_index) const
{
    HC_LOG_TRACE("");
    return m_relevant_if_index.find(if_index) != std::end(m_relevant_if_index);
}

void receiver::registrate_interface(unsigned int if_index)
{
    HC_LOG_TRACE("interface: " << interfaces::get_if_name(if_index));

    std::lock_guard<std::mutex> lock(m_data_lock);

    m_relevant_if_index.insert(if_index);
}

void receiver::del_interface(unsigned int if_index)
{
    HC_LOG_TRACE("interface: " << interfaces::get_if_name(if_index));

    std::lock_guard<std::mutex> lock(m_data_lock);

    m_relevant_if_index.erase(if_index);
}

void receiver::worker_thread()
{
    HC_LOG_TRACE("");

    int info_size = 0;

    //########################
    //create msg
    //iov

    std::unique_ptr<unsigned char[]> iov_buf { new unsigned char[get_iov_min_size()] };
    //unsigned char iov_buf[r->get_iov_min_size()];
    struct iovec iov;
    iov.iov_base = iov_buf.get();
    iov.iov_len = get_iov_min_size(); //sizeof(iov_buf);

    //control
    std::unique_ptr<unsigned char[]> ctrl { new unsigned char[get_ctrl_min_size()] };
    //unsigned char ctrl[r->get_ctrl_min_size()];

    //create msghdr
    struct msghdr msg;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = ctrl.get();
    msg.msg_controllen = get_ctrl_min_size(); //sizeof(ctrl);

    msg.msg_flags = 0;
    //########################

    while (m_running) {
        if (!m_mrt_sock->receive_msg(&msg, info_size)) {
            HC_LOG_ERROR("received failed");
            sleep(1);
            continue;
        }
        if (info_size == 0) {
            continue; //on timeout
        }

        m_data_lock.lock();
        analyse_packet(&msg, info_size);
        m_data_lock.unlock();
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
    if (!m_in_debug_testing_mode) {
        m_running =  true;
        m_thread.reset(new std::thread(&receiver::worker_thread, this));
    }
}

void receiver::stop()
{
    HC_LOG_TRACE("");

    m_running = false;
}

void receiver::join()
{
    HC_LOG_TRACE("");

    if (m_thread.get() != nullptr) {
        m_thread->join();
    }
}
