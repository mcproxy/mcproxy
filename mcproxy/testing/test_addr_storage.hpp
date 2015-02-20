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

#ifndef  TEST_ADDR_STORAGE_HPP 
#define  TEST_ADDR_STORAGE_HPP

#ifdef UNIT_TESTS

#include "include/hamcast_logging.h"

//#include <netinet/in.h>
#include <arpa/inet.h>

#include "testing/ut_bootstrap.hpp"
#include "testing/ut_suite.hpp"

#include "include/utils/addr_storage.hpp"

test_status test_addr_storage_a();
test_status test_addr_storage_b();


std::list<std::tuple<ut_test_fun, ut_effort>> test_address_storage()
{
    HC_LOG_TRACE("");
    return std::list<std::tuple<ut_test_fun, ut_effort>> {
        std::make_tuple(test_addr_storage_a, 1)
        , std::make_tuple(test_addr_storage_b, 1)
    };
}

test_status test_addr_storage_a()
{
    HC_LOG_TRACE("");
    UT_INITIALISATION;

    const std::string addr4 = "251.0.0.224";
    const std::string addr6 = "ff02:231:abc::1";

    sockaddr_storage sockaddr4;
    sockaddr_storage sockaddr6;
    in_addr in_addr4;
    in6_addr in_addr6;
    in6_addr in_addr6tmp;

    addr_storage s4;
    addr_storage s6;
    addr_storage s4_tmp;
    addr_storage s6_tmp;
    addr_storage s4_1;
    addr_storage s6_1;

    //string in addr_storage, cout stream, sockaddr_storage to string
    s4 = addr4;
    s6 = addr6;

    UT_CHECK(addr4.compare(s4.to_string()) == 0);
    UT_CHECK(addr6.compare(s6.to_string()) == 0);

    //sockaddr_storage to addr_storage
    sockaddr4 = s4.get_sockaddr_storage();
    sockaddr6 = s6.get_sockaddr_storage();
    s4_1 = sockaddr4;
    s6_1 = sockaddr6;

    UT_CHECK(addr4.compare(s4_1.to_string()) == 0);
    UT_CHECK(addr6.compare(s6_1.to_string()) == 0);

    //equivalent addresses
    s4_tmp = "Test ERROR 1!!";
    s6_tmp = "An other Test ERROR";

    UT_CHECK(s4_tmp != s6_tmp);
    UT_CHECK(s6_1 == s6_1);

    //in_addr and in6_addr
    in_addr4 = s4.get_in_addr();
    in_addr6 = s6.get_in6_addr();

    UT_CHECK(inet_pton(AF_INET6, addr6.c_str(), (void*)&in_addr6tmp) == 1);
    UT_CHECK(in_addr4.s_addr == inet_addr(addr4.c_str()));

    UT_CHECK(IN6_ARE_ADDR_EQUAL(&in_addr6, &in_addr6tmp));
    UT_CHECK((addr_storage(in_addr4).to_string().compare(addr4) == 0));
    UT_CHECK((addr_storage(in_addr6).to_string().compare(addr6) == 0));

    //ipv4 mask
    s6_tmp = "141.22.26.0";
    s4 = "141.22.26.249";
    s6 = "255.255.254.0";
    s4_tmp = s4;
    s4_tmp.mask_ipv4(s6);
    UT_CHECK(s4_tmp == s6_tmp);

    s4 = "141.22.27.155";
    s6 = "255.255.254.0";
    s4_tmp = s4;
    s4_tmp.mask_ipv4(s6);
    UT_CHECK(s4_tmp == s6_tmp);

    s4 = "141.22.27.142";
    s6 = "255.255.254.0";
    s4_tmp = s4;
    s4_tmp.mask_ipv4(s6);
    UT_CHECK(s4_tmp == s6_tmp);

    //less then
    s4 = "141.22.26.249";
    s6 = "255.255.254.0";
    UT_CHECK(s4 < s6);
    UT_CHECK(!(s6 < s4));

    s4 = "fe80::5e26:aff:fe23:8dc0";
    s6 = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";
    UT_CHECK(s4 < s6);
    UT_CHECK(!(s6 < s4));

    s4 = "0:0:0:0:ffff:ffff:ffff:ffff";
    s6 = "ffff:ffff:ffff:ffff::0";
    UT_CHECK(s4 < s6);
    UT_CHECK(!(s6 < s4));

    UT_SUMMARY;
}

test_status test_addr_storage_b()
{
    HC_LOG_TRACE("");
    UT_INITIALISATION;

    const std::string s4("1.2.3.4");
    const std::string s6("1:2:3::4");
    const addr_storage a4(s4);
    const addr_storage a6(s6);
    const std::string sport = "123";
    const int iport = 123;

    UT_CHECK(!(a4 == a6));
    UT_CHECK((a4 != a6));
    UT_CHECK(!(a6 == a4));
    UT_CHECK((a6 != a4));
    UT_CHECK((a4 == a4));
    UT_CHECK(!(a4 != a4));
    UT_CHECK((a6 == a6));
    UT_CHECK(!(a6 != a6));

    UT_CHECK(a4.to_string().compare(s4) == 0);
    UT_CHECK(a6.to_string().compare(s6) == 0);

    UT_CHECK(addr_storage(a4.get_sockaddr_storage()).to_string().compare(s4) == 0);
    UT_CHECK(addr_storage(a6.get_sockaddr_storage()).to_string().compare(s6) == 0);

    UT_CHECK(addr_storage(a4.get_sockaddr()).to_string().compare(s4) == 0);
    UT_CHECK(addr_storage(a4.get_sockaddr()).to_string().compare(s4) == 0);

    UT_CHECK(addr_storage(a4.get_sockaddr_in()).to_string().compare(s4) == 0);
    UT_CHECK(addr_storage(a6.get_sockaddr_in6()).to_string().compare(s6) == 0);

    addr_storage a4a;
    addr_storage a6a;
    a4a = s4;
    a6a = s6;
    UT_CHECK(a4a.set_port(iport).get_port() == iport);
    UT_CHECK(a6a.set_port(iport).get_port() == iport);

    UT_CHECK(a4a.to_string().compare(s4) == 0);
    UT_CHECK(a6a.to_string().compare(s6) == 0);

    UT_CHECK(a4a.set_port(sport).get_port() == iport);
    UT_CHECK(a6a.set_port(sport).get_port() == iport);

    UT_CHECK(a4a.to_string().compare(s4) == 0);
    UT_CHECK(a6a.to_string().compare(s6) == 0);

    UT_CHECK(addr_storage(a4a.set_port(iport).get_sockaddr_in()).get_port() == iport);
    UT_CHECK(addr_storage(a6a.set_port(iport).get_sockaddr_in6()).get_port() == iport);

    UT_CHECK(addr_storage(a4a.set_port(iport).get_in_addr()).get_port() == 0);
    UT_CHECK(a4a.set_port(iport).get_sockaddr_in().sin_port == htons(iport));
    UT_CHECK(a6a.set_port(iport).get_sockaddr_in6().sin6_port == htons(iport));

    UT_CHECK(++addr_storage("239.0.0.44") == addr_storage("239.0.0.45"));
    UT_CHECK(++addr_storage("239.0.255.255") == addr_storage("239.1.0.0"));
    UT_CHECK(++addr_storage("255.255.255.255") == addr_storage("0.0.0.0"));

    UT_CHECK(addr_storage("239.0.0.44") == --addr_storage("239.0.0.45"));
    UT_CHECK(addr_storage("239.0.255.255") == --addr_storage("239.1.0.0"));
    UT_CHECK(addr_storage("255.255.255.255") == --addr_storage("0.0.0.0"));

    UT_CHECK(++addr_storage("1::44") == addr_storage("1::45"));
    UT_CHECK(++addr_storage("1::FFFF:FFFF") == addr_storage("1::1:0:0"));
    UT_CHECK(++addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF") == addr_storage("0::0"));

    UT_CHECK(addr_storage("1::44") == --addr_storage("1::45"));
    UT_CHECK(addr_storage("1::FFFF:FFFF") == --addr_storage("1::1:0:0"));
    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF") == --addr_storage("0::0"));

    UT_CHECK(addr_storage("255.255.255.255") != addr_storage("0.0.0.0"));
    UT_CHECK(addr_storage("255.255.255.255") > addr_storage("0.0.0.0"));
    UT_CHECK(!(addr_storage("255.255.255.255") > addr_storage("255.255.255.255")));
    UT_CHECK(addr_storage("255.255.255.255") >= addr_storage("255.255.255.255"));
    UT_CHECK(addr_storage("255.255.255.255") >= addr_storage("255.255.255.254"));
    UT_CHECK(addr_storage("255.255.255.255") <= addr_storage("255.255.255.255"));
    UT_CHECK(addr_storage("255.255.255.254") <= addr_storage("255.255.255.255"));

    UT_CHECK(addr_storage("123.123.123.123").mask(24) == addr_storage("123.123.123.0"));
    UT_CHECK(addr_storage("123.123.123.123").mask(25) == addr_storage("123.123.123.0"));
    UT_CHECK(addr_storage("123.123.123.223").mask(25) == addr_storage("123.123.123.128"));

    UT_CHECK(addr_storage("123.123.123.123").broadcast_addr(24) == addr_storage("123.123.123.255"));
    UT_CHECK(addr_storage("123.123.123.123").broadcast_addr(25) == addr_storage("123.123.123.127"));
    UT_CHECK(addr_storage("123.123.123.223").broadcast_addr(25) == addr_storage("123.123.123.255"));

    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF").mask(64) == addr_storage("FFFF:FFFF:FFFF:FFFF::"));
    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF").mask(127) == addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFE"));
    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF").mask(95) == addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFE::"));
    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF").mask(1) == addr_storage("8000::"));

    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF::").broadcast_addr(64) == addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"));
    UT_CHECK(addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFF0").broadcast_addr(127) == addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFF1"));
    UT_CHECK(addr_storage("8000::").broadcast_addr(1) == addr_storage("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"));

    UT_CHECK(addr_storage("239.99.99.99").is_multicast_addr());
    UT_CHECK(!addr_storage("192.168.1.2").is_multicast_addr());
    UT_CHECK(addr_storage("FF05::99").is_multicast_addr());
    UT_CHECK(!addr_storage("2001::99").is_multicast_addr());

    UT_SUMMARY;
}

#endif //UNIT_TESTS

#endif //TEST_ADDR_STORAGE_HPP
