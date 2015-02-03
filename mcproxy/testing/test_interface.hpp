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

#ifndef  TEST_INTERFACE_HPP 
#define  TEST_INTERFACE_HPP

#ifdef UNIT_TESTS

//#include <netinet/in.h>
#include <arpa/inet.h>

#include "testing/ut_bootstrap.hpp"
#include "testing/ut_suite.hpp"

#include "include/parser/interface.hpp"
#include "include/proxy/def.hpp"

test_status test_interface_a();


std::list<std::tuple<ut_test_fun, ut_effort>> test_interface()
{
    return std::list<std::tuple<ut_test_fun, ut_effort>> {
        std::make_tuple(test_interface_a, 1)
    };
}

test_status test_interface_a()
{
    UT_INITIALISATION;

    //this behaviour is nessesary because in the file simple_membership_aggregation.hpp
    //mc_filter and rb_filer are represents as an union
    UT_CHECK(FT_BLACKLIST != static_cast<rb_filter_type>(INCLUDE_MODE));
    UT_CHECK(FT_BLACKLIST != static_cast<rb_filter_type>(EXCLUDE_MODE));

    UT_CHECK(FT_WHITELIST != static_cast<rb_filter_type>(INCLUDE_MODE));
    UT_CHECK(FT_WHITELIST != static_cast<rb_filter_type>(EXCLUDE_MODE));

    UT_CHECK(FT_UNDEFINED != static_cast<rb_filter_type>(INCLUDE_MODE));
    UT_CHECK(FT_UNDEFINED != static_cast<rb_filter_type>(EXCLUDE_MODE));

    UT_SUMMARY;
}

#endif //UNIT_TESTS

#endif //TEST_INTERFACE_HPP
