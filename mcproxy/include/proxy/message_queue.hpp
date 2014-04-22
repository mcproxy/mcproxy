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
#include <climits>
#include <vector>

/**
 * @brief Fixed sized synchronised priority job queue.
 */
template<typename T, typename Compare = std::less<T>>
class message_queue
{
private:
    std::priority_queue<T, std::vector<T>, Compare> m_q;
    unsigned int m_size;

    std::mutex m_global_lock;
    std::condition_variable cond_empty;

public:
    /**
      * @brief Create a message_queue with a maximum size.
      * @param size size of the message_queue.
      */
    message_queue(int size = UINT_MAX, Compare compare = Compare());

    /**
      * @brief Return true if the message queue is empty.
      */
    bool is_empty() const;

    /**
      * @brief Return the current size of the message queue.
      */
    unsigned int size() const;

    /**
      * @brief Return the maxmum size.
      */
    int max_size() const;

    /**
     * @brief Add an element on tail or delete the element if the queue is full.
     */
    bool enqueue_loseable(const T& t);

    /**
     * @brief Add an element on tail and wait if the queie is full.
     */
    void enqueue(const T& t);

    /**
     * @brief get and el element on head and wait if empty.
     */
    T dequeue(void);
};

template<typename T, typename Compare>
message_queue<T, Compare>::message_queue(int size, Compare compare)
    : m_q(compare)
    , m_size(size)
{
    HC_LOG_TRACE("");
}

template<typename T, typename Compare>
bool message_queue<T, Compare>::is_empty() const
{
    HC_LOG_TRACE("");

    std::lock_guard<std::mutex> lock(m_global_lock);

    return m_q.empty();
}

template<typename T, typename Compare>
unsigned int message_queue<T, Compare>::size() const
{
    HC_LOG_TRACE("");

    std::lock_guard<std::mutex> lock(m_global_lock);

    return m_q.size();
}

template<typename T, typename Compare>
int message_queue<T, Compare>::max_size() const
{
    HC_LOG_TRACE("");

    return m_size;
}

template<typename T, typename Compare>
bool message_queue<T, Compare>::enqueue_loseable(const T& t)
{
    HC_LOG_TRACE("");

    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        if (m_q.size() < m_size) {
            m_q.push(t);
        } else {
            HC_LOG_WARN("message_queue is full, failed to insert message");
            return false;
        }
    }
    cond_empty.notify_one();
    return true;
}

template<typename T, typename Compare>
void message_queue<T, Compare>::enqueue(const T& t)
{
    HC_LOG_TRACE("");

    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        m_q.push(t);
    }
    cond_empty.notify_one();
    HC_LOG_DEBUG("!!!!!test2");
}

template<typename T, typename Compare>
T message_queue<T, Compare>::dequeue(void)
{
    HC_LOG_TRACE("");

    T t;
    {
        std::unique_lock<std::mutex> lock(m_global_lock);
        cond_empty.wait(lock, [&]() {
            return m_q.size() != 0;
        });

        t = m_q.top();
        m_q.pop();
    }
    return t;
}

#endif // MESSAGE_QUEUE_HPP
/** @} */
