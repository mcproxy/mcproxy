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

#ifndef WORKER_HPP
#define WORKER_HPP

#include "include/proxy/message_queue.hpp"
#include "include/proxy/message_format.hpp"

#include <thread>
#include <memory>

#define WORKER_MESSAGE_QUEUE_DEFAULT_SIZE 150



struct comp_proxy_msg {
    bool operator()(const std::shared_ptr<proxy_msg>& l, const std::shared_ptr<proxy_msg>& r) const {
        return *l < *r;
    }
};

/**
 * @brief Wraps the job queue to a basic worker like an simple actor pattern.
 */
class worker
{
private:
protected:

    std::unique_ptr<std::thread> m_thread;
    /**
     * @brief Worker thread to process the jobs.
     */
    virtual void worker_thread() = 0;

    /**
     * @brief The threads runs as long as m_running is true.
     */
    bool m_running;

    /**
     * @brief Job queue to process proxy_msg.
     */
    mutable message_queue<std::shared_ptr<proxy_msg>,comp_proxy_msg> m_job_queue;
    void join() const;
    void start();
    void stop();
public:

    /**
     * @brief Create a worker with a maximum job queue size.
     * @param max_msg maximum size of the job queue
     */
    worker();
    worker(int queue_size);

    virtual ~worker();


    bool is_running() const;

    /**
     * @brief Add a message to the job queue.
     */
    void add_msg(const std::shared_ptr<proxy_msg>& msg) const;

    static void test_worker();
};

#endif // WORKER_HPP
/** @} */
