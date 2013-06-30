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
#include <sys/time.h>
#include <iostream>

timing::timing():
     m_running(false), m_worker_thread(0)
{
     HC_LOG_TRACE("");

}

timing::~timing(){
     HC_LOG_TRACE("");
     delete m_worker_thread;
}

timing::timehandling::timehandling(struct timeval time, proxy_instance* pr_i, proxy_msg pr_msg){
     HC_LOG_TRACE("");
     m_time = time;
     m_pr_i = pr_i;
     m_pr_msg = pr_msg;
}

void timing::worker_thread(timing* t){
     HC_LOG_TRACE("");

     while(t->m_running){
          usleep(TIME_POLL_INTERVAL);

          struct timeval current_timeval;
          gettimeofday(&current_timeval, NULL);

          t->m_global_lock.lock();

          list<struct timehandling>::iterator iter;
          for(iter= t->m_time_list.begin(); iter != t->m_time_list.end(); iter++){
               if(current_timeval.tv_sec > iter->m_time.tv_sec){
                    iter->m_pr_i->add_msg(iter->m_pr_msg);
                    iter = t->m_time_list.erase(iter);
               }else if(current_timeval.tv_sec == iter->m_time.tv_sec){
                    if(current_timeval.tv_usec >= iter->m_time.tv_usec){
                         iter->m_pr_i->add_msg(iter->m_pr_msg);
                         iter = t->m_time_list.erase(iter);
                    }
               }
          }

          t->m_global_lock.unlock();

     }
}

timing* timing::getInstance(){
     HC_LOG_TRACE("");

     static timing instance;
     return &instance;
}

void timing::add_time(int msec, proxy_instance* m_pr_i, proxy_msg& pr_msg){
     HC_LOG_TRACE("");

     struct timeval t;
     gettimeofday(&t, NULL);
     t.tv_sec += msec/1000;
     t.tv_usec += 1000 * (msec % 1000);;

     struct timehandling th(t, m_pr_i, pr_msg);

     m_global_lock.lock();
     m_time_list.push_back(th);
     m_global_lock.unlock();

}

void timing::stop_all_time(proxy_instance* pr_i){
     HC_LOG_TRACE("");

     m_global_lock.lock();

     list<struct timehandling>::iterator iter;
     for(iter= m_time_list.begin(); iter != m_time_list.end(); iter++){
          if(iter->m_pr_i == pr_i){
               iter = m_time_list.erase(iter);
          }
     }

     m_global_lock.unlock();
}

void timing::start(){
     HC_LOG_TRACE("");

     m_running =  true;
     m_worker_thread =  new boost::thread(timing::worker_thread, this);
}

void timing::stop(){
     HC_LOG_TRACE("");

     m_running= false;
}

void timing::join(){
     HC_LOG_TRACE("");

     if(m_worker_thread){
          m_worker_thread->join();
     }
}

void timing::test_timing(){
     HC_LOG_TRACE("");

     timing* t = timing::getInstance();
     proxy_msg p_msg;
     p_msg.msg =  new struct test_msg(4);

     t->start();
     t->add_time(10000,NULL,p_msg);
     /*t->add_time(2000,NULL,p_msg);
     t->add_time(3000,NULL,p_msg);
     t->add_time(4000,NULL,p_msg);
     t->add_time(10000,NULL,p_msg);*/
     sleep(2);
     t->stop();
     t->join();
}
