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
 * @defgroup mod_communication Communication
 * @brief The module Communication provide a communication with a synchronised
 * job queue, messaging formats and wrapper to hide the details.
 * @{
 */

#ifndef MESSAGE_QUEUE_HPP
#define MESSAGE_QUEUE_HPP

#include <boost/thread/pthread/mutex.hpp>
#include <boost/thread/pthread/condition_variable.hpp>
#include <queue>
using namespace std;

/**
 * @brief Fixed sized synchronised job queue.
 */
template< typename T>
class message_queue{
private:
    message_queue();

    queue<T> m_q;
    unsigned int m_size;

    boost::mutex m_global_lock;
    boost::condition_variable cond_full;
    boost::condition_variable cond_empty;
public:
    /**
      * @brief Create a message_queue with a fixed size.
      * @param size size of the message_queue.
      */
    message_queue(int size);

    /**
      * @brief Return true if the message queue is empty.
      */
    bool is_empty();

    /**
      * @brief Return the current size of the message queue.
      */
    unsigned int current_size();

    /**
      * @brief Return the set size.
      */
    int max_size();

    /**
     * @brief add an element on tail and wait if full.
     */
    void enqueue(T t);

    /**
     * @brief get and el element on head and wait if empty.
     */
    T dequeue(void);
};

template< typename T>
message_queue<T>::message_queue(int size){
    m_size = size;
}

template< typename T>
bool message_queue<T>::is_empty(){
    boost::lock_guard<boost::mutex> lock(m_global_lock);

    return m_q.empty();
}


template< typename T>
unsigned int message_queue<T>::current_size(){
    boost::lock_guard<boost::mutex> lock(m_global_lock);

    return m_q.size();
}

template< typename T>
int message_queue<T>::max_size(){
    return m_size;
}

template< typename T>
void message_queue<T>::enqueue(T t){
    {
        boost::unique_lock<boost::mutex> lock(m_global_lock);
        while(m_q.size() >= m_size){
            cond_full.wait(lock);
        }

        m_q.push(t);
    }
    cond_empty.notify_one();
}

template< typename T>
T message_queue<T>::dequeue(void){
    T t;
    {
        boost::unique_lock<boost::mutex> lock(m_global_lock);
        while(m_q.size() == 0){
            cond_empty.wait(lock);
        }

        t= m_q.front();
        m_q.pop();
    }
    cond_full.notify_one();
    return t;
}

#endif // MESSAGE_QUEUE_HPP
/** @} */
