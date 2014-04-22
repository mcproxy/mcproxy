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

#include "unistd.h"

worker::worker()
    : m_thread(nullptr)
    , m_running(false)
    , m_job_queue(WORKER_MESSAGE_QUEUE_DEFAULT_SIZE)
{
    HC_LOG_TRACE("");
}

worker::worker(int queue_size)
    : m_thread(nullptr)
    , m_running(false)
    , m_job_queue(queue_size)
{
    HC_LOG_TRACE("");
}

worker::~worker()
{
    HC_LOG_TRACE("");
    stop();
    join();
}


void worker::start()
{
    HC_LOG_TRACE("");

    if (m_thread.get() == nullptr) {
        m_running =  true;
        m_thread.reset(new std::thread(&worker::worker_thread, this));
    } else {
        HC_LOG_WARN("timing is already running");
    }
}

void worker::add_msg(const std::shared_ptr<proxy_msg>& msg) const
{
    HC_LOG_TRACE("");

    HC_LOG_DEBUG("message type: " << proxy_msg::get_message_type_name(msg->get_type()));
    HC_LOG_DEBUG("message priority: " << proxy_msg::get_message_priority_name(msg->get_priority()));
    if (msg->get_priority() == proxy_msg::LOSEABLE) {
        m_job_queue.enqueue_loseable(msg);
    } else {
        m_job_queue.enqueue(msg);
    }
}

bool worker::is_running() const
{
    HC_LOG_TRACE("");
    return m_running;
}

void worker::stop()
{
    HC_LOG_TRACE("");
    m_running = false;
}

void worker::join() const
{
    HC_LOG_TRACE("");

    if (m_thread.get() != nullptr) {
        m_thread->join();
    }
}

#ifdef DEBUG_MODE
void worker::test_worker()
{
    std::cout << "##-- test worker --##" << std::endl;

    class my_worker: public worker
    {
    public:
        my_worker(int queue_size): worker(queue_size) {
            HC_LOG_TRACE("");
            start();
        }
    private:
        void worker_thread() {
            HC_LOG_TRACE("");
            sleep(1);
            while (m_running) {
                HC_LOG_DEBUG("in while(m_running)");
                auto m = m_job_queue.dequeue();
                switch (m->get_type()) {
                case proxy_msg::TEST_MSG:
                    HC_LOG_DEBUG("dequeued a test_msg");
                    (*m)();
                    break;
                case proxy_msg::EXIT_MSG:
                    HC_LOG_DEBUG("dequeued a exit_msg");
                    std::cout << "exit msg" << std::endl;
                    stop();
                    break;
                default:
                    HC_LOG_DEBUG("dequeued an other message");
                    std::cout << "an other unknown message" << std::endl;
                    break;
                }
            }
            HC_LOG_DEBUG("after while(m_running)");
        };
    };


    //};

    std::unique_ptr<worker> m(new my_worker(4));
    //[4 6] 5  [1 2 3 ] without 7

    m->add_msg(std::make_shared<test_msg>(test_msg(1, proxy_msg::LOSEABLE)));
    m->add_msg(std::make_shared<test_msg>(test_msg(2, proxy_msg::LOSEABLE)));
    m->add_msg(std::make_shared<test_msg>(test_msg(3, proxy_msg::LOSEABLE)));
    m->add_msg(std::make_shared<test_msg>(test_msg(4, proxy_msg::USER_INPUT)));
    m->add_msg(std::make_shared<test_msg>(test_msg(5, proxy_msg::SYSTEMIC)));
    m->add_msg(std::make_shared<test_msg>(test_msg(6, proxy_msg::USER_INPUT)));
    m->add_msg(std::make_shared<test_msg>(test_msg(7, proxy_msg::LOSEABLE)));
    sleep(3);
    m->add_msg(std::make_shared<exit_cmd>(exit_cmd()));

    std::cout << "##-- end of test worker --##" << std::endl;
    sleep(4);
}
#endif /* DEBUG_MODE */
