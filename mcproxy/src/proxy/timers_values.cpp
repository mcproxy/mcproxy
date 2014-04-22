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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */

#include "include/proxy/timers_values.hpp"
#include "include/hamcast_logging.h"
#include "include/proxy/membership_db.hpp"

#include <iomanip>
#include <sstream>
#include <iostream>

template<typename T>
std::string to_hex_string(T n)
{
    std::ostringstream s;
    s << "0x" << std::uppercase << std::hex << static_cast<unsigned int>(n);
    return s.str();
}

timers_values::timers_values(const timers_values& tv)
    : is_default_timers_values_tank(true)
    , tank(&default_timers_values_tank)
{
    HC_LOG_TRACE("");
    *this = tv;
}

timers_values& timers_values::operator=(const timers_values& tv)
{
    HC_LOG_TRACE("");
    reset_to_default_tank();

    if (!tv.is_default_timers_values_tank) {
        is_default_timers_values_tank = false;
        tank = new timers_values_tank;
        *tank = *tv.tank;
    }

    return *this;
}


std::chrono::seconds timers_values::qqic_to_qqi(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");

    exp = (exp & 0x7) ;
    mant = (mant & 0xF);

    if (!first_bit) {
        return std::chrono::seconds((exp << 0x4) | mant);
    } else {
        return std::chrono::seconds((mant | 0x10)  << (exp + 3));
    }
}

std::chrono::seconds timers_values::qqic_to_qqi(uint8_t qqic) const
{
    HC_LOG_TRACE("");
    if (qqic & 0x80) {
        int exp = (qqic & 0x70) >> 0x4;
        int mant = qqic & 0xF;
        return std::chrono::seconds((mant | 0x10)  << (exp + 3));
    } else {
        return std::chrono::seconds(qqic);
    }

}

uint8_t timers_values::qqi_to_qqic(const std::chrono::seconds& sec) const
{
    HC_LOG_TRACE("");
    if (sec.count() < 128) {
        return sec.count();
    } else {
        int exp;
        int mant;
        for (int pos = 15 ; pos >= 7; pos--) {
            if (sec.count() & (1 << pos)) {
                exp = pos - 5 + 1;
                mant = (sec.count() & (0xF << exp)) >> exp;
                exp -= 3;
                return (1 << 7) | (exp << 4) | mant;
            }
        }

        HC_LOG_ERROR("unknown qqic sec: " << time_to_string(sec));
        return 0;
    }
}


std::chrono::milliseconds timers_values::maxrespc_igmpv3_to_maxrespi(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");
    return std::chrono::milliseconds(qqic_to_qqi(first_bit, exp, mant).count() * 100);
}

std::chrono::milliseconds timers_values::maxrespc_igmpv3_to_maxrespi(uint8_t max_resp_code) const
{
    HC_LOG_TRACE("");
    return std::chrono::milliseconds(qqic_to_qqi(max_resp_code).count() * 100);
}

uint8_t timers_values::maxrespi_to_maxrespc_igmpv3(const std::chrono::milliseconds& msec) const
{
    HC_LOG_TRACE("");
    return qqi_to_qqic(std::chrono::seconds(msec.count() / 100));
}

std::chrono::milliseconds timers_values::maxrespc_mldv2_to_maxrespi(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");

    exp = (exp & 0x7) ;
    mant = (mant & 0xFFF);

    if (!first_bit) {
        return std::chrono::milliseconds((exp << 12) | mant);
    } else {
        return std::chrono::milliseconds((mant | 0x1000)  << (exp + 3));
    }
}

std::chrono::milliseconds timers_values::maxrespc_mldv2_to_maxrespi(uint16_t max_resp_code) const
{
    HC_LOG_TRACE("");

    if (max_resp_code & 0x8000) {
        int exp = (max_resp_code & 0x7000) >> 12;
        int mant = max_resp_code & 0xFFF;
        return std::chrono::milliseconds((mant | 0x1000)  << (exp + 3));
    } else {
        return std::chrono::milliseconds(max_resp_code);
    }
}

uint16_t timers_values::maxrespi_to_maxrespc_mldv2(const std::chrono::milliseconds& msec) const
{
    HC_LOG_TRACE("");

    if (msec.count() < 32767) {
        return msec.count();
    } else {
        int exp;
        int mant;
        for (int pos = 31; pos >= 15; pos--) {
            if (msec.count() & (1 << pos)) {
                exp = pos - 12;
                mant = (msec.count() & (0xFFF << exp)) >> exp;
                exp -= 3;

                return (1 << 15) | (exp << 12) | mant;
            }
        }
        HC_LOG_ERROR("unknown max response code: " << time_to_string(msec));
        return 0;
    }
}

//--------------------------------------
unsigned int timers_values::get_robustness_variable() const
{
    HC_LOG_TRACE("");
    return tank->robustness_variable;
}

std::chrono::seconds timers_values::get_query_interval() const
{
    HC_LOG_TRACE("");
    return tank->query_interval;
}

std::chrono::milliseconds timers_values::get_query_response_interval() const
{
    HC_LOG_TRACE("");
    return tank->query_response_interval;
}

std::chrono::milliseconds timers_values::get_multicast_address_listening_interval() const
{
    HC_LOG_TRACE("");
    return (tank->robustness_variable * tank->query_interval) + tank->query_response_interval;
}

std::chrono::milliseconds timers_values::get_other_querier_present_interval() const
{
    HC_LOG_TRACE("");
    return (tank->robustness_variable * tank->query_interval) + (tank->query_response_interval / 2);
}

std::chrono::seconds timers_values::get_startup_query_interval() const
{
    HC_LOG_TRACE("");
    return tank->startup_query_interval;
}

unsigned int timers_values::get_startup_query_count() const
{
    HC_LOG_TRACE("");
    return tank->startup_query_count;
}

std::chrono::milliseconds timers_values::get_last_listener_query_interval() const
{
    HC_LOG_TRACE("");
    return tank->last_listener_query_interval;
}

unsigned int timers_values::get_last_listener_query_count() const
{
    HC_LOG_TRACE("");
    return tank->last_listener_query_count;
}

std::chrono::milliseconds timers_values::get_last_listener_query_time() const
{
    HC_LOG_TRACE("");
    return tank->last_listener_query_interval * tank->last_listener_query_count;
}

std::chrono::milliseconds timers_values::get_unsolicited_report_interval() const
{
    HC_LOG_TRACE("");
    return tank->unsolicited_report_interval;
}

std::chrono::milliseconds timers_values::get_older_host_present_interval() const
{
    HC_LOG_TRACE("");
    return (tank->robustness_variable * tank->query_interval) + tank->query_response_interval;
}


void timers_values::set_new_tank()
{
    HC_LOG_TRACE("");
    if (is_default_timers_values_tank) {
        HC_LOG_DEBUG("set a new tank");
        tank = new timers_values_tank();
        is_default_timers_values_tank = false;
    }
}

void timers_values::reset_to_default_tank()
{
    HC_LOG_TRACE("");
    if (!is_default_timers_values_tank) {
        HC_LOG_DEBUG("delete tank");
        delete tank;
        tank = &default_timers_values_tank;
        is_default_timers_values_tank = true;
    }
}

//--------------------------------------
void timers_values::set_robustness_variable(unsigned int robustness_variable)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->robustness_variable = robustness_variable;
}

void timers_values::set_query_interval(std::chrono::seconds query_interval)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->query_interval = query_interval;
}

void timers_values::set_query_response_interval(std::chrono::milliseconds query_response_interval)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->query_response_interval = query_response_interval;
}

void timers_values::set_startup_query_interval(std::chrono::seconds startup_query_interval)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->startup_query_interval = startup_query_interval;
}

void timers_values::set_startup_query_count(unsigned int startup_query_count)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->startup_query_count = startup_query_count;

}

void timers_values::set_last_listener_query_interval(std::chrono::milliseconds last_listener_query_interval)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->last_listener_query_interval = last_listener_query_interval;

}

void timers_values::set_last_listener_query_count(unsigned int last_listener_query_count)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->last_listener_query_count = last_listener_query_count;

}

void timers_values::set_unsolicited_report_interval(std::chrono::milliseconds unsolicited_report_interval)
{
    HC_LOG_TRACE("");
    set_new_tank();
    tank->unsolicited_report_interval = unsolicited_report_interval;
}


timers_values::~timers_values()
{
    HC_LOG_TRACE("");
    reset_to_default_tank();
}


std::string timers_values::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << "is_default_timers_values_tank: " << (is_default_timers_values_tank ? "true" : "false") << std::endl;
    s << "Robustness Variable: " << get_robustness_variable() << std::endl;
    s << "Query Interval: " << time_to_string(get_query_interval()) << std::endl;
    s << "Query Response Interval: " << time_to_string(get_query_response_interval()) << std::endl;
    s << "Multicast Address Listining Interval: " << time_to_string(get_robustness_variable() * get_query_interval() + get_query_response_interval()) << std::endl;
    s << "Other Querier Present Interval: " << time_to_string(get_robustness_variable() * get_query_interval() + (get_query_response_interval() / 2)) << std::endl;;
    s << "Startup Query Interval: " << time_to_string(get_startup_query_interval()) << std::endl;
    s << "Startup Query Count: " << get_startup_query_count() << std::endl;
    s << "Last Listener Query Interval: " << time_to_string(get_last_listener_query_interval()) << std::endl;
    s << "Last Listener Query Count: " << get_last_listener_query_count() << std::endl;
    s << "Last Listener Query Time: " << time_to_string(get_last_listener_query_interval() * get_last_listener_query_count()) << std::endl;
    s << "Unsolicited Report Interval: " << time_to_string(get_unsolicited_report_interval()) << std::endl;
    s << "Older Host Present Interval: " << time_to_string((get_robustness_variable() * get_query_interval()) + get_query_response_interval()) << std::endl;

    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const timers_values& tv)
{
    HC_LOG_TRACE("");
    return stream << tv.to_string();
}

#ifdef DEBUG_MODE
void timers_values::test_timers_values()
{
    HC_LOG_TRACE("");
    timers_values tv;
    std::cout << "##-- test timers and values --##" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "qqic_to_qqi(1): " << time_to_string((tv.qqic_to_qqi(1))) << std::endl;
    std::cout << "qqic_to_qqi(20): " << time_to_string((tv.qqic_to_qqi(20))) << std::endl;
    std::cout << "qqic_to_qqi(100): " << time_to_string((tv.qqic_to_qqi(100))) << std::endl;
    std::cout << "qqic_to_qqi(127): " << time_to_string((tv.qqic_to_qqi(127))) << std::endl;
    std::cout << "qqic_to_qqi(128): " << time_to_string((tv.qqic_to_qqi(128))) << std::endl;
    std::cout << "qqic_to_qqi(129): " << time_to_string((tv.qqic_to_qqi(129))) << std::endl;
    std::cout << "qqic_to_qqi(0xFF): " << time_to_string((tv.qqic_to_qqi(0xFF))) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "qqic_to_qqi(false,0,1): " << time_to_string((tv.qqic_to_qqi(false, 0, 1))) << std::endl;
    std::cout << "qqic_to_qqi(false,0,7): " << time_to_string((tv.qqic_to_qqi(false, 0, 7))) << std::endl;
    std::cout << "qqic_to_qqi(false,0,15): " << time_to_string((tv.qqic_to_qqi(false, 0, 15))) << std::endl;
    std::cout << "qqic_to_qqi(false,0,16): " << time_to_string((tv.qqic_to_qqi(false, 0, 16))) << std::endl;
    std::cout << "qqic_to_qqi(false,1,0): " << time_to_string((tv.qqic_to_qqi(false, 1, 0))) << std::endl;
    std::cout << "qqic_to_qqi(false,7,0): " << time_to_string((tv.qqic_to_qqi(false, 7, 0))) << std::endl;
    std::cout << "qqic_to_qqi(false,8,0): " << time_to_string((tv.qqic_to_qqi(false, 8, 0))) << std::endl;
    std::cout << "qqic_to_qqi(false,7,15): " << time_to_string((tv.qqic_to_qqi(false, 7, 15))) << std::endl;
    std::cout << "qqic_to_qqi(true,0,0): " << time_to_string((tv.qqic_to_qqi(true, 0, 0))) << std::endl;
    std::cout << "qqic_to_qqi(true,0,1): " << time_to_string((tv.qqic_to_qqi(true, 0, 1))) << std::endl;
    std::cout << "qqic_to_qqi(true,1,0): " << time_to_string((tv.qqic_to_qqi(true, 1, 0))) << std::endl;
    std::cout << "qqic_to_qqi(true,1,1): " << time_to_string((tv.qqic_to_qqi(true, 1, 1))) << std::endl;
    std::cout << "qqic_to_qqi(true,7,15): " << time_to_string((tv.qqic_to_qqi(true, 7, 15))) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "qqi_to_qqic(1 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(1)))) << std::endl;
    std::cout << "qqi_to_qqic(7 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(7)))) << std::endl;
    std::cout << "qqi_to_qqic(127 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(127)))) << std::endl;
    std::cout << "qqi_to_qqic(128 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(128)))) << std::endl;
    std::cout << "qqi_to_qqic(31744 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(31744)))) << std::endl;
    std::cout << "qqi_to_qqic(32745 sec): " << to_hex_string((tv.qqi_to_qqic(std::chrono::seconds(32745)))) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    for (int i = 100; i <= 0xFF; i += 7) {
        std::cout << "qqi_to_qqic(qqic_to_qqi(" << i << "))";
        if (tv.qqi_to_qqic(tv.qqic_to_qqi(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "maxrespc_igmpv3_to_maxrespi(true,1,1): " << time_to_string(tv.maxrespc_igmpv3_to_maxrespi(true, 1, 1)) << std::endl;
    std::cout << "maxrespc_igmpv3_to_maxrespi(0xFF): " << time_to_string(tv.maxrespc_igmpv3_to_maxrespi(0xFF)) << std::endl;

    std::cout << "maxrespi_to_maxrespc_igmpv3(3174400 msec): " << to_hex_string(tv.maxrespi_to_maxrespc_igmpv3(std::chrono::milliseconds(3174400))) << std::endl;

    std::cout << "--------------------------------------" << std::endl;
    for (int i = 100; i <= 0xFF; i += 7) {
        std::cout << "maxrespi_to_maxrespc_igmpv3(maxrespc_igmpv3_to_maxrespi(" << i << ")) ";
        if (tv.maxrespi_to_maxrespc_igmpv3(tv.maxrespc_igmpv3_to_maxrespi(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(false,0,0): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(false, 0, 0)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(false,0,0): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(0)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(true,0,0): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(true, 0, 0)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(0x8000): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(0x8000)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(true,0,1): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(true, 0, 1)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(0x8001): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(0x8001)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(true,1,1): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(true, 1, 1)) << std::endl;
    std::cout << "tv.maxrespc_mldv2_to_maxrespi(0x9001): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(0x9001)) << std::endl;

    std::cout << "tv.maxrespc_mldv2_to_maxrespi(5274==" << to_hex_string(5274) << "): " << time_to_string(tv.maxrespc_mldv2_to_maxrespi(5274)) << std::endl;
    std::cout << "tv.calc_msec_to_max_resp_code_mldv2(5274==" << to_hex_string(5274) << "): " << to_hex_string(tv.maxrespi_to_maxrespc_mldv2(std::chrono::milliseconds(5274))) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    for (int i = 1000; i <= 0xFFFF; i += 2137) {
        std::cout << "calc_msec_to_max_resp_code_mldv2(maxrespc_mldv2_to_maxrespi(" << i << ") ";
        if (tv.maxrespi_to_maxrespc_mldv2(tv.maxrespc_mldv2_to_maxrespi(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

}
void timers_values::test_timers_values_copy()
{
    using namespace std;
    using namespace chrono;
    timers_values tv;
    tv.set_robustness_variable(99);
    tv.set_query_interval(seconds(100));
    tv.set_query_response_interval(milliseconds(101));
    tv.set_startup_query_interval(seconds(102));
    tv.set_startup_query_count(103);
    tv.set_last_listener_query_interval(milliseconds(104));
    tv.set_last_listener_query_count(105);
    tv.set_unsolicited_report_interval(milliseconds(106));

    std::cout << "##-- test timers and values copy function --##" << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    cout << "my timers_values:" << endl;
    cout << tv << endl;

    {
        timers_values tv_copy;
        tv_copy = tv;

        cout << "timers_values copy:" << endl;
        cout << tv_copy << endl;
    }

    cout << "my timers_values:" << endl;
    cout << tv << endl;




}
#endif /* DEBUG_MODE */
