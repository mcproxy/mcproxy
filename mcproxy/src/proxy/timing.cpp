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
#include "include/proxy/timing.hpp"
#include "include/proxy/proxy_instance.hpp"

#include <iostream>
#include <unistd.h>

timing::timing():
    m_running(false), m_thread(nullptr)
{
    HC_LOG_TRACE("");
    start();
}

timing::~timing()
{
    HC_LOG_TRACE("");
    stop();
    join();
}

void timing::worker_thread()
{
    HC_LOG_TRACE("");

    while (m_running) {

        std::mutex lokal_lock;
        std::unique_lock<std::mutex> ull(lokal_lock);

        if (m_db.empty()) {
            sleep(TIMING_IDLE_POLLING_INTERVAL);
        } else {
            m_con_var.wait_until(ull, m_db.begin()->first);
        }

        std::lock_guard<std::mutex> lock(m_global_lock);

        timing_db_key now = std::chrono::steady_clock::now();

        for (auto i = begin(m_db); i != end(m_db); ++i) {
            if (i->first <= now) {
                timing_db_value& db_value = i->second;
                (*std::get<1>(db_value).get())();
                if (std::get<0>(db_value) != nullptr) {
                    std::get<0>(db_value)->add_msg(std::move(std::get<1>(db_value)));
                }

                i = m_db.erase(i);
                if (i == end(m_db)) {
                    break;
                }

            } else {
                break;
            }
        }
    }
}

void timing::add_time(std::chrono::milliseconds delay, proxy_instance* pr_inst, const std::shared_ptr<proxy_msg> pr_msg)
{
    HC_LOG_TRACE("");
    timing_db_key until = std::chrono::steady_clock::now() + delay;

    std::lock_guard<std::mutex> lock(m_global_lock);

    m_db.insert(timing_db_pair(until, std::make_tuple(pr_inst, pr_msg)));
    m_con_var.notify_one();
}

void timing::stop_all_time(const proxy_instance* pr_inst)
{
    HC_LOG_TRACE("");

    std::lock_guard<std::mutex> lock(m_global_lock);

    for (auto it = begin(m_db); it != end(m_db); ++it ) {
        if (std::get<0>(it->second) == pr_inst) {
            it = m_db.erase(it);
        }
    }

}

void timing::start()
{
    HC_LOG_TRACE("");

    if (m_thread.get() == nullptr) {
        m_running =  true;
        m_thread.reset(new std::thread(&timing::worker_thread, this));
    } else {
        HC_LOG_WARN("timing is already running");
    }
}

void timing::stop()
{
    HC_LOG_TRACE("");

    m_running = false;
}

void timing::join() const
{
    HC_LOG_TRACE("");

    if (m_thread) {
        m_thread->join();
    }
}

void timing::test_timing()
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "##-- test timing --##" << endl;
    timing t;

    cout << "add test message 1 (5sec) " << endl;
    t.add_time(std::chrono::seconds(5), nullptr, std::make_shared<test_msg>(test_msg(1, proxy_msg::SYSTEMIC)));
    cout << "add test message 2 (7sec) " << endl;
    t.add_time(std::chrono::seconds(7), nullptr, std::make_shared<test_msg>(test_msg(2, proxy_msg::SYSTEMIC)));
    cout << "add test message 3 (1sec) " << endl;
    t.add_time(std::chrono::seconds(1), nullptr, std::make_shared<test_msg>(test_msg(3, proxy_msg::SYSTEMIC)));
    cout << "add test message 4 (1msec) " << endl;
    t.add_time(std::chrono::milliseconds(1), nullptr, std::make_shared<test_msg>(test_msg(4, proxy_msg::SYSTEMIC)));
    cout << "add test message 5 (1msec) " << endl;
    t.add_time(std::chrono::milliseconds(1), nullptr, std::make_shared<test_msg>(test_msg(5, proxy_msg::SYSTEMIC)));

    sleep(10);
    cout << "finished" << endl;
}



