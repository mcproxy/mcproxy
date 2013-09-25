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
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <tuple>


#define TIMING_IDLE_POLLING_INTERVAL 1 //sec
class proxy_instance;

using timing_db_value = std::tuple<proxy_instance*, proxy_msg>;
using timing_db_key = std::chrono::time_point<std::chrono::steady_clock>;
using timing_db = map<timing_db_key, timing_db_value>; 
using timing_db_pair = pair<timing_db_key, timing_db_value>;
/**
 * @brief Organizes reminder.
 */
class timing
{
private:

    timing_db m_db;

    bool m_running;
    std::thread* m_worker_thread;
    static void worker_thread(timing* t);

    std::mutex m_global_lock;
    std::condition_variable m_con_var;

    //GOF singleton
    timing();
    timing(const timing&);
    timing& operator=(const timing&);
    virtual ~timing();
public:
    /**
     * @brief Get an instance of the Routing module (GOF singleton).
     */
    static timing* getInstance();

    /**
     * @brief Add a new reminder with an predefined time.
     * @param msec predefined time in millisecond
     * @param proxy_instance* pointer to the owner of the reminder
     * @param pr_msg message of the reminder
     *
     */
    void add_time(std::chrono::milliseconds delay, proxy_instance* pr_inst, proxy_msg& pr_msg);

    void add_time(std::chrono::milliseconds delay, proxy_instance* pr_inst, proxy_msg&& pr_msg);
    /**
     * @brief Delete all reminder from a specific proxy instance.
     * @param proxy_instance* pointer to the specific proxy instance
     */
    void stop_all_time(const proxy_instance* pr_inst);

    /**
     * @brief Start the module Timer.
     */
    void start();

    /**
     * @brief Stop the module Timer, but dont wait for stopped.
     */
    void stop();

    /**
     * @brief Blocked until module Timer stopped.
     */
    void join() const;

        /**
     * @brief Test the functionality of the module Timer.
     */
    static void test_timing();


};



#endif // TIME_HPP
/** @} */
