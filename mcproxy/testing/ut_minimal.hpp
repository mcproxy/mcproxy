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

#ifndef  UT_MINIMAL_HPP
#define  UT_MINIMAL_HPP

#ifdef UNIT_TESTS

#include <iostream>
#include <string>
#include <sstream>

inline void report_error(std::string&& fun, std::string&& msg, std::string&& file, int line, bool stop_testing);

class test_status
{
private:
    unsigned int m_error_counter = 0;
    unsigned int m_test_counter = 0;
    std::string m_test_name;
public:
    test_status(std::string&& test_name)
        : m_test_name(std::move(test_name)) {
    }

    void inrement_error_counter() {
        ++m_error_counter;
    }

    void inrement_test_counter() {
        ++m_test_counter;
    }

    void reset_error_counter() {
        m_error_counter = 0;
    }

    unsigned int get_error_counter() const {
        return m_error_counter;
    }

    unsigned int get_test_counter() const {
        return m_test_counter;
    }

    const std::string& get_test_name() const {
        return m_test_name;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << get_test_name() << ": " << get_error_counter() << " of " << get_test_counter() << " tests failed.";
        return ss.str();
    }

    friend std::ostream& operator<< (std::ostream& out, const test_status& t) {
        return out << t.to_string();
    }
};

class unit_test_error
{
private:
    test_status m_test_status;
public:
    unit_test_error(const test_status& ts)
        : m_test_status(ts) {
    }

    const test_status& get_test_status() const{
        return m_test_status; 
    }
};

#define REPORT_ERROR(ts, fun, msg, file, line, stop_testing)                                                       \
    {                                                                                                              \
        ts.inrement_error_counter();                                                                               \
        ts.inrement_test_counter();                                                                                \
                                                                                                                   \
        char vt100_clearline[] = { 27, '[' , '2', 'K', '\0'};                                                      \
        std::cout << vt100_clearline << "\r" << fun << " " << file << "(" << line << "): " << msg << std::endl;    \
                                                                                                                   \
        if (stop_testing) {                                                                                        \
            throw unit_test_error(ts);                                                                             \
        }                                                                                                          \
    } 


#define UT_CHECK(expression) if (expression) { \
    ut_test_status__.inrement_test_counter();  \
    } else REPORT_ERROR(ut_test_status__, "CHECK FAILED", #expression, __FILE__, __LINE__, false)

#define UT_REQUIRE(expression) if (expression) { \
    ut_test_status__.inrement_test_counter();    \
    } else REPORT_ERROR(ut_test_status__, "CHECK FAILED", #expression, __FILE__, __LINE__, true)

#define UT_ERROR(message) \
    REPORT_ERROR(ut_test_status__, "ERROR", message, __FILE__, __LINE__, false)

#define UT_FAIL(message) \
    REPORT_ERROR(ut_test_status__, "FAILED", message, __FILE__, __LINE__, true)


#define UT_INITIALISATION test_status ut_test_status__(std::string(__FUNCTION__));
#define UT_SUMMARY return ut_test_status__ ;

#endif //UNIT_TESTS

#endif //UT_MINIMAL_HPP
