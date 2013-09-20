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

#include "include/proxy/timers_values.hpp"
#include "include/hamcast_logging.h"
#include "include/proxy/membership_db.hpp"

#include <iomanip>
#include <iostream>

template<typename T>
string to_hex_str(T n)
{
    ostringstream s;
    s << "0x" << std::uppercase << std::hex << n;
    return s.str();
}

uint16_t timers_values::calc_qqic_to_sec(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");

    exp = (exp & 0x7) ;
    mant = (mant & 0xF);

    if (!first_bit) {
        return (exp << 0x4) | mant;
    } else {
        return (mant | 0x10)  << (exp + 3);
    }
}

uint16_t timers_values::calc_qqic_to_sec(uint8_t qqic) const
{
    HC_LOG_TRACE("");
    if (qqic & 0x80) {
        int exp = (qqic & 0x70) >> 0x4;
        int mant = qqic & 0xF;
        return (mant | 0x10)  << (exp + 3);
    } else {
        return qqic;
    }

}

uint8_t timers_values::calc_sec_to_qqic(uint16_t sec) const
{
    HC_LOG_TRACE("");
    if (sec < 128) {
        return sec;
    } else {
        int exp;
        int mant;
        for (int pos = 15 ; pos >= 7; pos--) {
            if (sec & (1 << pos)) {
                exp = pos - 5 + 1;
                mant = (sec & (0xF << exp)) >> exp;
                exp -= 3;
                return (1 << 7) | (exp << 4) | mant;
            }
        }

        HC_LOG_ERROR("unknown qqic sec: " << sec);
        return 0;
    }
}


uint32_t timers_values::calc_max_resp_code_igmpv3_to_msec(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");
    return calc_qqic_to_sec(first_bit, exp, mant) * 100;
}

uint32_t timers_values::calc_max_resp_code_igmpv3_to_msec(uint8_t max_resp_code) const
{
    HC_LOG_TRACE("");
    return calc_qqic_to_sec(max_resp_code) * 100;
}

uint8_t timers_values::calc_msec_to_max_resp_code_igmpv3(uint32_t msec) const
{
    HC_LOG_TRACE("");
    return calc_sec_to_qqic(msec / 100);
}

uint32_t timers_values::calc_max_resp_code_mldv2_to_msec(bool first_bit, unsigned int exp, unsigned int mant) const
{
    HC_LOG_TRACE("");

    exp = (exp & 0x7) ;
    mant = (mant & 0xFFF);

    if (!first_bit) {
        return (exp << 12) | mant;
    } else {
        return (mant | 0x1000)  << (exp + 3);
    }
}

uint32_t timers_values::calc_max_resp_code_mldv2_to_msec(uint16_t max_resp_code) const
{
    HC_LOG_TRACE("");

    if (max_resp_code & 0x8000) {
        int exp = (max_resp_code & 0x7000) >> 12;
        int mant = max_resp_code & 0xFFF;
        return (mant | 0x1000)  << (exp + 3);
    } else {
        return max_resp_code;
    }
}

uint16_t timers_values::calc_msec_to_max_resp_code_mldv2(uint32_t msec) const
{
    HC_LOG_TRACE("");

    if (msec < 32767) {
        return msec;
    } else {
        int exp;
        int mant;
        for (int pos = 31; pos >= 15; pos--) {
            if (msec & (1 << pos)) {
                exp = pos - 12;
                mant = (msec & (0xFFF << exp)) >> exp;
                exp -= 3;

                return (1 << 15) | (exp << 12) | mant;
            }
        }
        std::cout << "hier falsch und so" << endl;
        HC_LOG_ERROR("unknown max response code: " << msec);
        return 0;
    }
}

void timers_values::test_timers_values()
{
    HC_LOG_TRACE("");
    timers_values tv;
    std::cout << "##-- test timers and values --##" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "calc_qqic_to_sec(1): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(1)) << std::endl;
    std::cout << "calc_qqic_to_sec(20): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(20)) << std::endl;
    std::cout << "calc_qqic_to_sec(100): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(100)) << std::endl;
    std::cout << "calc_qqic_to_sec(127): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(127)) << std::endl;
    std::cout << "calc_qqic_to_sec(128): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(128)) << std::endl;
    std::cout << "calc_qqic_to_sec(129): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(129)) << std::endl;
    std::cout << "calc_qqic_to_sec(0xFF): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(0xFF)) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "calc_qqic_to_sec(false,0,1): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 0, 1)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,0,7): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 0, 7)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,0,15): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 0, 15)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,0,16): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 0, 16)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,1,0): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 1, 0)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,7,0): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 7, 0)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,8,0): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 8, 0)) << std::endl;
    std::cout << "calc_qqic_to_sec(false,7,15): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(false, 7, 15)) << std::endl;
    std::cout << "calc_qqic_to_sec(true,0,0): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(true, 0, 0)) << std::endl;
    std::cout << "calc_qqic_to_sec(true,0,1): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(true, 0, 1)) << std::endl;
    std::cout << "calc_qqic_to_sec(true,1,0): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(true, 1, 0)) << std::endl;
    std::cout << "calc_qqic_to_sec(true,1,1): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(true, 1, 1)) << std::endl;
    std::cout << "calc_qqic_to_sec(true,7,15): " << static_cast<unsigned int>(tv.calc_qqic_to_sec(true, 7, 15)) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "calc_sec_to_qqic(1): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(1)) << std::endl;
    std::cout << "calc_sec_to_qqic(7): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(7)) << std::endl;
    std::cout << "calc_sec_to_qqic(127): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(127)) << std::endl;
    std::cout << "calc_sec_to_qqic(128): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(128)) << std::endl;
    std::cout << "calc_sec_to_qqic(31744): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(31744)) << std::endl;
    std::cout << "calc_sec_to_qqic(32745): " << static_cast<unsigned int>(tv.calc_sec_to_qqic(32745)) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    for (int i = 100; i <= 0xFF; i += 7) {
        std::cout << "sec_to_qqic(qqic_to_sec(" << i << ") ";
        if (tv.calc_sec_to_qqic(tv.calc_qqic_to_sec(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "calc_max_resp_code_igmpv3_to_msec(true,1,1): " << static_cast<unsigned int>(tv.calc_max_resp_code_igmpv3_to_msec(true, 1, 1)) << std::endl;
    std::cout << "calc_max_resp_code_igmpv3_to_msec(0xFF): " << static_cast<unsigned int>(tv.calc_max_resp_code_igmpv3_to_msec(0xFF)) << std::endl;

    std::cout << "calc_msec_to_max_resp_code_igmpv3(3174400): " << static_cast<unsigned int>(tv.calc_msec_to_max_resp_code_igmpv3(3174400)) << std::endl;

    std::cout << "--------------------------------------" << std::endl;
    for (int i = 100; i <= 0xFF; i += 7) {
        std::cout << "calc_msec_to_max_resp_code_igmpv3(calc_max_resp_code_igmpv3_to_msec(" << i << ") ";
        if (tv.calc_msec_to_max_resp_code_igmpv3(tv.calc_max_resp_code_igmpv3_to_msec(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(false,0,0): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(false, 0, 0)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(false,0,0): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(0)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(true,0,0): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(true, 0, 0)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(0x8000): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(0x8000)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(true,0,1): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(true, 0, 1)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(0x8001): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(0x8001)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(true,1,1): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(true, 1, 1)) << std::endl;
    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(0x9001): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(0x9001)) << std::endl;

    std::cout << "tv.calc_max_resp_code_mldv2_to_msec(5274==" << to_hex_str(5274)<< "): " << static_cast<unsigned int>(tv.calc_max_resp_code_mldv2_to_msec(5274)) << std::endl;
    std::cout << "tv.calc_msec_to_max_resp_code_mldv2(5274==" << to_hex_str(5274)<< "): " << static_cast<unsigned int>(tv.calc_msec_to_max_resp_code_mldv2(5274)) << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    for (int i = 1000; i <= 0xFFFF; i += 2137) {
        std::cout << "calc_msec_to_max_resp_code_mldv2(calc_max_resp_code_mldv2_to_msec(" << i << ") ";
        if (tv.calc_msec_to_max_resp_code_mldv2(tv.calc_max_resp_code_mldv2_to_msec(i)) == i) {
            std::cout << "OK!" << std::endl;
        } else {
            std::cout << "FAILED!" << std::endl;
        }
    }

}
