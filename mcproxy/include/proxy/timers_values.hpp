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
 * @addtogroup mod_proxy Proxy
 * @{
 */

#ifndef TIMERS_VALUES_HPP
#define TIMERS_VALUES_HPP

#include <cstdint>
#include <string>
#include <chrono>


struct timers_values_tank {
    unsigned int robustness_variable = 2;
    std::chrono::seconds query_interval = std::chrono::seconds(125);
    std::chrono::milliseconds query_response_interval = std::chrono::milliseconds(10000); //Max Response Time/Delay
    std::chrono::seconds startup_query_interval = query_interval / 4;
    unsigned int startup_query_count = robustness_variable;
    std::chrono::milliseconds last_member_query_interval = std::chrono::milliseconds(1000);
    unsigned int last_member_query_count = robustness_variable;
    std::chrono::milliseconds unsolicited_report_interval = std::chrono::milliseconds(1000);
    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const timers_values_tank& tvt);
};


static timers_values_tank default_timers_values_tank = timers_values_tank();

class timers_values
{
private:
    bool is_default_timers_values_tank = true;
    timers_values_tank* tank = &default_timers_values_tank;

    void set_new_tank();
    //--------------------------------------
    //return in seconds
    std::chrono::seconds qqic_to_qqi(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in seconds
    std::chrono::seconds qqic_to_qqi(uint8_t qqic) const;

    //retrun qqic
    uint8_t qqi_to_qqic(const std::chrono::seconds& sec) const;

    //--------------------------------------
    //return in milli seconds
    std::chrono::milliseconds maxrespc_igmpv3_to_maxrespi(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in milli seconds
    std::chrono::milliseconds maxrespc_igmpv3_to_maxrespi(uint8_t max_resp_code) const;

    //retrun qqic
    uint8_t maxrespi_to_maxrespc_igmpv3(const std::chrono::milliseconds& msec) const;

    //--------------------------------------
    //return in milli seconds
    std::chrono::milliseconds maxrespc_mldv2_to_maxrespi(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in milli seconds
    std::chrono::milliseconds maxrespc_mldv2_to_maxrespi(uint16_t max_resp_code) const;

    //retrun qqic
    uint16_t maxrespi_to_maxrespc_mldv2(std::chrono::milliseconds msec) const;

public:
    static void test_timers_values();

    unsigned int get_robustness_variable() const;
    std::chrono::seconds get_query_interval() const;
    std::chrono::milliseconds get_query_response_interval() const;
    std::chrono::milliseconds get_group_membership_interval() const; //
    std::chrono::milliseconds get_other_querier_present_interval() const; //
    std::chrono::seconds get_startup_query_interval() const;
    unsigned int get_startup_query_count() const;
    std::chrono::milliseconds get_last_member_query_interval() const;
    unsigned int get_last_member_query_count() const;
    std::chrono::milliseconds get_last_member_query_time() const; //
    std::chrono::milliseconds get_unsolicited_report_interval() const;
    std::chrono::milliseconds get_older_host_present_interval() const; //

    void set_robustness_variable(unsigned int robustness_variable);
    void set_query_interval_sec(std::chrono::seconds query_interval);
    void set_query_response_interval(std::chrono::milliseconds query_response_interval);
    void set_startup_query_interval(std::chrono::seconds startup_query_interval);
    void set_startup_query_count(unsigned int startup_query_count);
    void set_last_member_query_interval(std::chrono::milliseconds last_member_query_interval);
    void set_last_member_query_count(unsigned int last_member_query_count);
    void set_unsolicited_report_interval(std::chrono::milliseconds unsolicited_report_interval);

    void reset_to_default_tank();

    virtual ~timers_values();

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const timers_values& tv);
};


#endif // TIMERS_VALUES_HPP
/** @} */
