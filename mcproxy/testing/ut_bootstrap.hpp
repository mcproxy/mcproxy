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

#ifndef  UT_BOOTSTRAP_HPP
#define  UT_BOOTSTRAP_HPP

#ifdef UNIT_TESTS

#include "include/hamcast_logging.h"

#include "testing/ut_suite.hpp"
#include "testing/test_addr_storage.hpp"
#include "testing/test_membership_aggregation.hpp"
#include "testing/test_parser.hpp"
#include "testing/test_source_list.hpp"

void ut_bootstrap(int, char**){
    hc_set_default_log_fun(HC_LOG_TRACE_LVL);

    ut_suite test_suite;
    
    test_suite.add_test_fun(test_address_storage());
    test_suite.add_test_fun(test_membership_aggregation());
    test_suite.add_test_fun(test_parser_functions());
    test_suite.add_test_fun(test_source_list());

    test_suite.run_test_suite();
}

#endif //UNIT_TESTS

#endif //UT_BOOTSTRAP_HPP
