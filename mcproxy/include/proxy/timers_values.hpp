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

class timers_values
{
private:
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
};


#endif // TIMERS_VALUES_HPP
/** @} */
