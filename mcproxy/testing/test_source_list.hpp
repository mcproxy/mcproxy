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

#ifndef  TEST_SOURCE_LIST
#define  TEST_SOURCE_LIST

#ifdef UNIT_TESTS

#include "testing/ut_bootstrap.hpp"
#include "testing/ut_suite.hpp"

#include "include/proxy/def.hpp"

test_status test_source_list_a();

std::list<std::tuple<ut_test_fun, ut_effort>> test_source_list()
{
    HC_LOG_TRACE("");
    return std::list<std::tuple<ut_test_fun, ut_effort>> {
        std::make_tuple(test_source_list_a, 1)
    };
}

test_status test_source_list_a()
{
    HC_LOG_TRACE("");
    UT_INITIALISATION;
    using sl = source_list<int>;
    sl a;
    sl b;
    sl r;

    const sl sl_a {0, 1, 2, 55};
    const sl sl_b {1, 2, 3, 55};
   
    UT_CHECK(sl_a == sl_a);
    UT_CHECK(!(sl_a != sl_a));
    UT_CHECK(sl_b == sl_b);
    UT_CHECK(!(sl_b != sl_b));
    UT_CHECK(sl_a != sl_b);
    UT_CHECK(!(sl_a == sl_b));

    a = sl_a;
    a += sl_b;
    r = sl{0,1,2,3,55};
    UT_CHECK(a == r);
    UT_CHECK((sl_a + sl_b) == r);

    b = sl_b;
    b += sl_a;
    UT_CHECK(b == r);
    UT_CHECK((sl_b + sl_a) == r);

    a = sl_a;
    a *= sl_b;
    r = sl{1,2,55};
    UT_CHECK(a == r);
    UT_CHECK((sl_a * sl_b) == r);

    b = sl_b;
    b *= sl_a;
    UT_CHECK(b == r);
    UT_CHECK((sl_b * sl_a) == r);

    a = sl_a;
    a -= sl_b;
    r = sl{0};
    UT_CHECK(a == r);
    UT_CHECK((sl_a - sl_b) == r);

    b = sl_b;
    b -= sl_a;
    r = sl{3};
    UT_CHECK(b == r);
    UT_CHECK((sl_b - sl_a) == r);

    //random checks
    UT_CHECK((sl{1, 5, 2} - sl{2} - sl{5, 2}) == sl{1});
    UT_CHECK((sl{5, 2} - sl{1} - sl{5, 2}) == sl{});
    UT_CHECK(((sl{1} + sl{2} + sl{3})* sl{1}) == sl{1});

    UT_SUMMARY;
}

#endif //UNIT_TESTS

#endif //TEST_SOURCE_LIST
