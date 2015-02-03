/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * * This program is distributed in the hope that it will be useful,
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

#ifndef  UT_SUITE_HPP
#define  UT_SUITE_HPP

#ifdef UNIT_TESTS

#include <list>
#include <functional>
#include <tuple>
#include <sstream>

#include "testing/ut_minimal.hpp"

using ut_test_fun = std::function<test_status()>;
using ut_effort = unsigned int;

class ut_suite
{
private:

    std::list<std::tuple<ut_test_fun, ut_effort>> m_fun_list;
    std::list<test_status> m_test_status_list;
    unsigned int m_abort_counter = 0;

    unsigned int get_entire_effort() {
        unsigned int result = 0;
        for (auto & e : m_fun_list) {
            result += std::get<1>(e);
        }
        return result;
    }
public:

    void add_test_fun(ut_test_fun tf) {
        add_test_fun(tf, 1);
    }

    void add_test_fun(ut_test_fun tf, ut_effort x) {
        m_fun_list.push_back(std::make_tuple(tf, x));
    }

    void add_test_fun(std::list<std::tuple<ut_test_fun, ut_effort>>&& tfl) {
        m_fun_list.splice(m_fun_list.end(), std::move(tfl));
    }

    void run_test_suite() {
        unsigned int full_bar = get_entire_effort();
        unsigned int current_bar = 0;

        for (auto & e : m_fun_list) {
            try {
                auto ts = std::get<0>(e)();
                m_test_status_list.push_back(std::move(ts));
            } catch (unit_test_error& e) {
                ++m_abort_counter;
                m_test_status_list.push_back(e.get_test_status());
            } catch (...) {
                std::cout << std::endl;
                std::cout << "CORE DUMPED occurred in processed test unit!" << std::endl;
                if (m_test_status_list.size() == 0) {
                    std::cout << "\tHint: First test unit." << std::endl;
                } else {
                    auto fun_name = (--(m_test_status_list.end()))->get_test_name();
                    std::cout << "\tHint: Last successed fininshed test function was: " << fun_name << std::endl;
                }

                exit(1);
            }

            current_bar += std::get<1>(e);
            print_process_bar(full_bar, current_bar);
        }
        std::cout << std::endl;
        std::cout << *this << std::endl;
    }

    void print_process_bar(unsigned int full_bar, unsigned int current_bar) {
        unsigned int real_max_bar_length = 70;
        unsigned int real_bar_length = static_cast<unsigned int>((static_cast<double>(current_bar) / full_bar) * real_max_bar_length);
        unsigned int percent = (real_bar_length * 100) / real_max_bar_length;
        if (real_bar_length > real_max_bar_length) {
            real_bar_length = real_max_bar_length;
        }

        std::cout << "\r<";
        for (unsigned int i = 0; i < real_max_bar_length; ++i) {
            if (i < real_bar_length) {
                std::cout << "=";
            } else {
                std::cout << " ";
            }
        }
        std::cout << "> " << percent << "%" << std::flush;
    }

    std::string to_string() const {
        std::stringstream ss;
        unsigned int test_units = m_test_status_list.size();
        unsigned int test_counter = 0;
        unsigned int error_counter = 0;

        for (auto & e : m_test_status_list) {
            test_counter += e.get_test_counter();
            error_counter += e.get_error_counter();
        }

        ss << m_abort_counter << " of " << test_units << " test units were canceled." << std::endl;
        ss << error_counter << " of " << test_counter << " tests failed.";

        return ss.str();
    }

    friend std::ostream& operator<< (std::ostream& out, const ut_suite& t) {
        return out << t.to_string();
    }

};

#endif //UNIT_TESTS

#endif //UT_SUITE_HPP
