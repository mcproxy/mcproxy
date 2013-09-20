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

struct timers_values_tank {
    unsigned int robustness_variable = 2;
    unsigned int query_interval_sec = 125;
    unsigned int query_response_interval_msec = 10000; //Max Response Time/Delay
    unsigned int startup_query_interval_sec = query_interval_sec / 4;
    unsigned int startup_query_count = robustness_variable;
    unsigned int last_member_query_interval_msec = 1000;
    unsigned int last_member_query_count = robustness_variable;
    unsigned int unsolicited_report_interval_msec = 1000;

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
    void delete_new_tank();
    //--------------------------------------
    //return in seconds
    uint16_t calc_qqic_to_sec(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in seconds
    uint16_t calc_qqic_to_sec(uint8_t qqic) const;

    //retrun qqic
    uint8_t calc_sec_to_qqic(uint16_t sec) const;

    //--------------------------------------
    //return in milli seconds
    uint32_t calc_max_resp_code_igmpv3_to_msec(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in milli seconds
    uint32_t calc_max_resp_code_igmpv3_to_msec(uint8_t max_resp_code) const;

    //retrun qqic
    uint8_t calc_msec_to_max_resp_code_igmpv3(uint32_t msec) const;

    //--------------------------------------
    //return in milli seconds
    uint32_t calc_max_resp_code_mldv2_to_msec(bool first_bit, unsigned int exp, unsigned int mant) const;

    //return in milli seconds
    uint32_t calc_max_resp_code_mldv2_to_msec(uint16_t max_resp_code) const;

    //retrun qqic
    uint16_t calc_msec_to_max_resp_code_mldv2(uint32_t msec) const;

public:
    static void test_timers_values();

    unsigned int get_robustness_variable() const;
    unsigned int get_query_interval_sec() const;
    unsigned int get_query_response_interval_msec() const;
    unsigned int get_group_membership_interval() const; //
    unsigned int get_other_querier_present_interval() const; //
    unsigned int get_startup_query_interval_sec() const;
    unsigned int get_startup_query_count() const;
    unsigned int get_last_member_query_interval_msec() const;
    unsigned int get_last_member_query_count() const;
    unsigned int get_last_member_query_time() const; //
    unsigned int get_unsolicited_report_interval_msec() const;
    unsigned int get_older_host_present_interval() const; //

    void set_robustness_variable(unsigned int robustness_variable);
    void set_query_interval_sec(unsigned int query_interval_sec);
    void set_query_response_interval_msec(unsigned int query_response_interval_msec);
    void set_startup_query_interval_sec(unsigned int startup_query_interval_sec);
    void set_startup_query_count(unsigned int startup_query_count);
    void set_last_member_query_interval_msec(unsigned int last_member_query_interval_msec);
    void set_last_member_query_count(unsigned int last_member_query_count);
    void set_unsolicited_report_interval_msec(unsigned int unsolicited_report_interval_msec);

    virtual ~timers_values();

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const timers_values& tv);
};


#endif // TIMERS_VALUES_HPP
/** @} */
