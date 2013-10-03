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
#include "include/hamcast_logging.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>

#define MESSAGE_QUEUE_DEFAULT_SIZE 150

/**
 * @brief Fixed sized synchronised job queue.
 */
template< typename T>
class message_queue
{
private:
    std::priority_queue<T> m_q;
    unsigned int m_size;

    std::mutex m_global_lock;
    std::condition_variable cond_empty;
public:
    /**
      * @brief Create a message_queue with a maximum size.
      * @param size size of the message_queue.
      */
    message_queue(int size);
    message_queue();

    /**
      * @brief Return true if the message queue is empty.
      */
    bool is_empty();

    /**
      * @brief Return the current size of the message queue.
      */
    unsigned int size();

    /**
      * @brief Return the set size.
      */
    int max_size();

    /**
     * @brief add an element on tail and wait if full.
     */
    bool enqueue_loseable(const T& t);
    void enqueue(const T& t);
    bool enqueue_loseable(T&& t);
    void enqueue(T&& t);

    /**
     * @brief get and el element on head and wait if empty.
     */
    T dequeue(void);
};

template< typename T>
message_queue<T>::message_queue(int size): m_size(size) {}

template< typename T>
message_queue<T>::message_queue(): message_queue(MESSAGE_QUEUE_DEFAULT_SIZE) {}

template< typename T>
bool message_queue<T>::is_empty()
{
    std::lock_guard<std::mutex> lock(m_global_lock);

    return m_q.empty();
}


template< typename T>
unsigned int message_queue<T>::size()
{
    std::lock_guard<std::mutex> lock(m_global_lock);

    return m_q.size();
}

template< typename T>
int message_queue<T>::max_size()
{
    return m_size;
}

template< typename T>
bool message_queue<T>::enqueue_loseable(const T& t)
{
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        if (m_q.size() < m_size) {
            m_q.push(t);
        } else {
            HC_LOG_WARN("message_queue is full, failed to insert object");
            return false;
        }
    }
    cond_empty.notify_one();
    return true;
}

template< typename T>
void message_queue<T>::enqueue(const T& t)
{
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        m_q.push(t);
    }
    cond_empty.notify_one();
}

template< typename T>
bool message_queue<T>::enqueue_loseable(T&& t)
{
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        if (m_q.size() < m_size) {
            m_q.push(std::move(t));
        } else {
            HC_LOG_WARN("message_queue is full, failed to insert object");
            return false;
        }
    }
    cond_empty.notify_one();
    return true;
}

template< typename T>
void message_queue<T>::enqueue(T&& t)
{
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        m_q.push(std::move(t));
    }
    cond_empty.notify_one();
}

template< typename T>
T message_queue<T>::dequeue(void)
{
    T t;
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        cond_empty.wait(lock, [&]() {
            return m_q.size() != 0;
        });

        t = std::move(m_q.front());
        m_q.pop();
    }
    return t;
}

#endif // MESSAGE_QUEUE_HPP
/** @} */
