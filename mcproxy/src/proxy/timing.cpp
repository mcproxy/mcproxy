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

timing::timing():
    m_running(false), m_worker_thread(0)
{
    HC_LOG_TRACE("");

}

timing::~timing()
{
    HC_LOG_TRACE("");
    delete m_worker_thread;
}

void timing::worker_thread(timing* t)
{
    HC_LOG_TRACE("");

    while (t->m_running) {

        //std::lock_guard<mutex> lock(t->m_global_lock);

        timing_db_key now = std::chrono::steady_clock::now();

        for(auto e: t->m_db){
            if(e.first <= now){

            }
        }

        //list<struct timehandling>::iterator iter;
        //for (iter = t->m_time_list.begin(); iter != t->m_time_list.end(); iter++) {
            //if (current_timeval.tv_sec > iter->m_time.tv_sec) {
                //iter->m_pr_i->add_msg(iter->m_pr_msg);
                //iter = t->m_time_list.erase(iter);
            //} else if (current_timeval.tv_sec == iter->m_time.tv_sec) {
                //if (current_timeval.tv_usec >= iter->m_time.tv_usec) {
                    //iter->m_pr_i->add_msg(iter->m_pr_msg);
                    //iter = t->m_time_list.erase(iter);
                //}
            //}
        //}

    }
}

timing* timing::getInstance()
{
    HC_LOG_TRACE("");

    static timing instance;
    return &instance;
}

void timing::add_time(std::chrono::milliseconds delay, proxy_instance* pr_inst, proxy_msg& pr_msg)
{
    HC_LOG_TRACE("");
    timing_db_key until = std::chrono::steady_clock::now() + delay;

    std::lock_guard<mutex> lock(m_global_lock);

    m_db.insert(timing_db_pair(until, std::make_tuple(pr_inst,pr_msg)));

}

void timing::stop_all_time(const proxy_instance* pr_inst)
{
    HC_LOG_TRACE("");

    std::lock_guard<mutex> lock(m_global_lock);

    for(auto it = begin(m_db); it != end(m_db); ++it ){
        if(std::get<0>(it->second) == pr_inst) {
            it = m_db.erase(it);
        }
    
    }

}

void timing::start()
{
    HC_LOG_TRACE("");

    m_running =  true;
    m_worker_thread =  new std::thread(timing::worker_thread, this);
}

void timing::stop()
{
    HC_LOG_TRACE("");

    m_running = false;
}

void timing::join() const
{
    HC_LOG_TRACE("");

    if (m_worker_thread) {
        m_worker_thread->join();
    }
}

void timing::test_timing()
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "##-- test timing --##" << endl;
    timing* t = timing::getInstance();

    proxy_msg p_msg;

    p_msg.msg =  new struct test_msg(4);
    
    cout << "start timing" << endl;
    t->start();

    cout << "add test message" << endl;
    t->add_time(std::chrono::seconds(10), nullptr, p_msg);
    /*t->add_time(2000,NULL,p_msg);
    t->add_time(3000,NULL,p_msg);
    t->add_time(4000,NULL,p_msg);
    t->add_time(10000,NULL,p_msg);*/
    
    sleep(20);
    cout << "stop and join timing" << endl;
    t->stop();
    t->join();
    cout << "finished" << endl;
}
