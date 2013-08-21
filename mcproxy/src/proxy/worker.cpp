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
#include "include/proxy/worker.hpp"

worker::worker(int max_msg):
    m_worker_thread(0), m_running(false), m_job_queue(max_msg)

{
    HC_LOG_TRACE("");
}

worker::~worker()
{
    close();
}


void worker::start()
{
    HC_LOG_TRACE("");

    m_running =  true;
    m_worker_thread =  new boost::thread(worker::worker_thread_starter, this);
}

void worker::worker_thread_starter(worker* w)
{
    HC_LOG_TRACE("");

    w->worker_thread();
}

void worker::close()
{
    HC_LOG_TRACE("");
    join();
    delete m_worker_thread;
}

void worker::add_msg(proxy_msg& msg)
{
    HC_LOG_TRACE("");

    HC_LOG_DEBUG("message type:" << msg.msg_type_to_string());
    m_job_queue.enqueue(msg);
}

bool worker::is_running()
{
    HC_LOG_TRACE("");
    return m_running;
}

void worker::join()
{
    HC_LOG_TRACE("");

    if (m_worker_thread) {
        m_worker_thread->join();
    }
}
