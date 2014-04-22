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
 * @defgroup mod_timer Timer
 * @brief The module Timer organizes the temporal behavior of the Proxy-Instances.
 * @{
 */

#ifndef TIME_HPP
#define TIME_HPP

#include "include/proxy/message_format.hpp"

#include <list>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <tuple>
#include <map>

#define TIMING_IDLE_POLLING_INTERVAL 1 //sec

class worker;

typedef std::tuple<const worker*, std::shared_ptr<proxy_msg>> timing_db_value;
typedef std::chrono::time_point<std::chrono::monotonic_clock> timing_db_key;
typedef std::map<timing_db_key, timing_db_value> timing_db;
typedef std::pair<timing_db_key, timing_db_value> timing_db_pair;

/**
 * @brief Organizes timer events.
 */
class timing
{
private:
    timing_db m_db;

    bool m_running;
    std::unique_ptr<std::thread> m_thread;
    void worker_thread();

    std::mutex m_global_lock;
    std::condition_variable m_con_var;

    void start();
    void stop();
    void join() const;

    timing(const timing&) = delete;
    timing(const timing&&) = delete;
    timing& operator=(const timing&) = delete;
    timing& operator=(const timing&&) = delete;

public:
    timing();

    /**
     * @brief Add a new reminder with an predefined time.
     * @param msec predefined time in millisecond
     * @param proxy_instance* pointer to the owner of the reminder
     * @param pr_msg message of the reminder
     */
    void add_time(std::chrono::milliseconds delay, const worker* msg_worker, const std::shared_ptr<proxy_msg>& pr_msg);

    /**
     * @brief Delete all reminder from a specific proxy instance.
     * @param proxy_instance* pointer to the specific proxy instance
     */
    void stop_all_time(const worker* msg_worker);

    virtual ~timing();
    
        /**
     * @brief Test the functionality of the module Timer.
     */
    static void test_timing();
};

#endif // TIME_HPP
/** @} */
