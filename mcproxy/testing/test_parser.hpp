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

#ifndef  TEST_PARSER_HPP
#define  TEST_PARSER_HPP

#ifdef UNIT_TESTS

#include "testing/ut_bootstrap.hpp"
#include "testing/ut_suite.hpp"

#include "include/parser/configuration.hpp"
#include "include/parser/interface.hpp"
#include "include/proxy/def.hpp"

struct test_parser {

    test_status test_rb_filter_mc_filter() {
        HC_LOG_TRACE("");
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

    test_status test_single_addr() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        single_addr s0(addr_storage("0.0.0.0"));
        UT_CHECK(s0.is_addr_contained(addr_storage("0.0.0.0")));
        UT_CHECK(s0.is_addr_contained(addr_storage("1.1.1.1")));
        UT_CHECK(!s0.is_addr_contained(addr_storage("2::2")));
        UT_CHECK(!s0.is_addr_contained(addr_storage("0::0")));

        single_addr s1(addr_storage("1.1.1.1"));
        UT_CHECK(s1.is_addr_contained(addr_storage("0.0.0.0")));
        UT_CHECK(s1.is_addr_contained(addr_storage("1.1.1.1")));
        UT_CHECK(!s1.is_addr_contained(addr_storage("2::2")));
        UT_CHECK(!s1.is_addr_contained(addr_storage("0::0")));

        UT_SUMMARY;
    }

    test_status test_addr_range() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        addr_range a0(addr_storage("1.1.1.1"), addr_storage("1.1.1.10"));
        UT_CHECK(a0.get_addr_set().size() == 10);
        UT_CHECK(a0.is_addr_contained(addr_storage("1.1.1.1")));
        UT_CHECK(a0.is_addr_contained(addr_storage("1.1.1.10")));
        UT_CHECK(a0.is_addr_contained(addr_storage("0.0.0.0")));
        UT_CHECK(!a0.is_addr_contained(addr_storage("2.2.2.2")));

        addr_range a1(addr_storage("0.0.0.0"), addr_storage("0.0.0.9"));
        UT_CHECK(a1.get_addr_set().size() == 10);
        UT_CHECK(a1.is_addr_contained(addr_storage("0.0.0.0")));
        UT_CHECK(a1.is_addr_contained(addr_storage("0.0.0.1")));
        UT_CHECK(a1.is_addr_contained(addr_storage("0.0.0.4")));
        UT_CHECK(a1.is_addr_contained(addr_storage("0.0.0.9")));
        UT_CHECK(!a1.is_addr_contained(addr_storage("2.2.2.2")));

        addr_range a2(addr_storage("1.1.1.1"), addr_storage("1.1.1.1"));
        UT_CHECK(a2.get_addr_set().size() == 1);
        UT_CHECK(a2.is_addr_contained(addr_storage("1.1.1.1")));
        UT_CHECK(!a2.is_addr_contained(addr_storage("2.2.2.2")));

        addr_range a3(addr_storage("255.255.255.250"), addr_storage("0.0.0.4"));
        UT_CHECK(a3.get_addr_set().size() == 0);
        UT_CHECK(!a3.is_addr_contained(addr_storage("255.255.255.250")));
        UT_CHECK(a3.is_addr_contained(addr_storage("0.0.0.0")));
        UT_CHECK(!a3.is_addr_contained(addr_storage("2::2")));

        UT_SUMMARY;
    }

    test_status test_rule_addr() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        const addr_storage x("0.0.0.0");

        const addr_storage m0("239.99.99.1");
        const addr_storage m1("239.99.99.99");
        const addr_storage m2("239.99.99.100");
        const addr_storage m3("239.99.99.101");

        const addr_storage s0("1.1.1.1");
        const addr_storage s1("1.1.1.99");
        const addr_storage s2("1.1.1.100");
        const addr_storage s3("1.1.1.101");

        using addr_set = std::set<addr_storage>;

        auto get_single_addr = [](const addr_storage & a) {
            return std::unique_ptr<addr_box>(new single_addr(a));
        };

        auto get_addr_range = [](const addr_storage & from, const addr_storage & to) {
            return std::unique_ptr<addr_box>(new addr_range(from, to));
        };

        auto get_addr_set = [](const addr_storage & from, const addr_storage & to) {
            addr_range ar(from, to);
            return ar.get_addr_set();
        };

        //test interface
        //test match sources

        rule_addr ra1("", get_single_addr(m1), get_single_addr(s0));
        UT_CHECK(ra1.get_addr_set("", m1, false) == addr_set {s0});
        UT_CHECK(ra1.get_addr_set("*", m1, false) == addr_set {s0});
        UT_CHECK(ra1.get_addr_set("eth0", m1, false) == addr_set {s0});
        UT_CHECK(ra1.get_addr_set("eth0", m2, false) == addr_set {});
        UT_CHECK(ra1.get_addr_set("", m1, true) == addr_set {s0});
        UT_CHECK(ra1.get_addr_set("*", m1, true) == addr_set {});
        UT_CHECK(ra1.get_addr_set("eth0", m1, true) == addr_set {});
        UT_CHECK(ra1.get_addr_set("eth0", m2, true) == addr_set {});

        rule_addr ra2("", get_addr_range(m0, m2), get_addr_range(s0, s1));
        UT_CHECK(ra2.get_addr_set("", m1, false) == get_addr_set(s0, s1));
        UT_CHECK(ra2.get_addr_set("", m3, false) == addr_set {});
        UT_CHECK(ra2.get_addr_set("eth1", m3, false) == addr_set {});
        UT_CHECK(ra2.get_addr_set("*", m3, false) == addr_set {});
        UT_CHECK(ra2.get_addr_set("*", m2, false) == get_addr_set(s0, s1));
        UT_CHECK(ra2.get_addr_set("xxx", m2, false) == get_addr_set(s0, s1));
        UT_CHECK(ra2.get_addr_set("", m1, true) == get_addr_set(s0, s1));
        UT_CHECK(ra2.get_addr_set("", m3, true) == addr_set {});
        UT_CHECK(ra2.get_addr_set("eth1", m3, true) == addr_set {});
        UT_CHECK(ra2.get_addr_set("*", m3, true) == addr_set {});
        UT_CHECK(ra2.get_addr_set("*", m2, true) == addr_set {});
        UT_CHECK(ra2.get_addr_set("xxx", m2, true) == addr_set {});

        rule_addr ra3("eth0", get_addr_range(m0, m2), get_addr_range(s0, s1));
        UT_CHECK(ra3.get_addr_set("eth0", m1, false) == get_addr_set(s0, s1));
        UT_CHECK(ra3.get_addr_set("eth0", m3, false) == addr_set {});
        UT_CHECK(ra3.get_addr_set("asdf", m1, false) == addr_set {});
        UT_CHECK(ra3.get_addr_set("*", m1, false) == get_addr_set(s0, s1));
        UT_CHECK(ra3.get_addr_set("", m2, false) == addr_set {});
        UT_CHECK(ra3.get_addr_set("", m3, false) == addr_set {});
        UT_CHECK(ra3.get_addr_set("eth0", m1, true) == get_addr_set(s0, s1));
        UT_CHECK(ra3.get_addr_set("eth0", m3, true) == addr_set {});
        UT_CHECK(ra3.get_addr_set("asdf", m1, true) == addr_set {});
        UT_CHECK(ra3.get_addr_set("*", m1, true) == addr_set {});
        UT_CHECK(ra3.get_addr_set("", m2, true) == addr_set {});
        UT_CHECK(ra3.get_addr_set("", m3, true) == addr_set {});

        rule_addr ra4("eth0", get_single_addr(x), get_single_addr(x));
        UT_CHECK(ra4.get_addr_set("eth0", m1, false) == addr_set {x});
        UT_CHECK(ra4.get_addr_set("eth1", m1, false) == addr_set {});
        UT_CHECK(ra4.get_addr_set("", m1, false) == addr_set {});
        UT_CHECK(ra4.get_addr_set("", x, false) == addr_set {});
        UT_CHECK(ra4.get_addr_set("*", m1, false) == addr_set {x});
        UT_CHECK(ra4.get_addr_set("*", x, false) == addr_set {x});
        UT_CHECK(ra4.get_addr_set("eth0", m1, true) == addr_set {x});
        UT_CHECK(ra4.get_addr_set("eth1", m1, true) == addr_set {});
        UT_CHECK(ra4.get_addr_set("", m1, true) == addr_set {});
        UT_CHECK(ra4.get_addr_set("", x, true) == addr_set {});
        UT_CHECK(ra4.get_addr_set("*", m1, true) == addr_set {});
        UT_CHECK(ra4.get_addr_set("*", x, true) == addr_set {});
        UT_SUMMARY;
    }

    test_status test_table() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        const addr_storage x("0.0.0.0");
        const addr_storage m1_0("239.99.99.1");
        const addr_storage m1_1("239.99.99.50");
        const addr_storage m1_2("239.99.99.100");
        const addr_storage m2_0("224.99.99.1");
        const addr_storage m2_1("224.99.99.50");
        const addr_storage m2_2("224.99.99.100");
        const addr_storage m3("233.0.0.1");
        const addr_storage m4("244.44.44.44");

        const addr_storage s1_0("1.1.1.1");
        const addr_storage s1_1("1.1.1.50");
        const addr_storage s1_2("1.1.1.100");
        const addr_storage s2("2.2.2.2");
        const addr_storage s3("3.3.3.3");
        const addr_storage s4("4.4.4.4");


        using addr_set = std::set<addr_storage>;

        //single_addr
        auto single = [](const addr_storage & a) {
            return std::unique_ptr<addr_box>(new single_addr(a));
        };

        //addr_range
        auto range = [](const addr_storage & from, const addr_storage & to) {
            return std::unique_ptr<addr_box>(new addr_range(from, to));
        };

        //rule_addr
        auto rule = [](const std::string & if_name, std::unique_ptr<addr_box> gaddr, std::unique_ptr<addr_box> saddr) {
            return std::unique_ptr<rule_box>(new rule_addr(if_name, std::move(gaddr), std::move(saddr)));
        };

        std::list<std::unique_ptr<rule_box>> rule_box_list;
        rule_box_list.push_back(rule("eth0", range(m1_0, m1_2), range(s1_0, s1_2)));
        rule_box_list.push_back(rule("eth0", single(m1_1), single(s1_1)));
        rule_box_list.push_back(rule("eth1", range(m2_0, m2_2), single(s2)));
        rule_box_list.push_back(rule("eth0", single(m3), single(s3)));
        rule_box_list.push_back(rule("", single(m4), single(s4)));
        table t("table_a", std::move(rule_box_list));

//table table_a{
//  eth0(239.99.99.1 - 239.99.99.100 | 1.1.1.1 - 1.1.1.100)
//  eth0(239.99.99.50 | 1.1.1.50)
//  eth1(224.99.99.1 - 244.99.99.100 | 2.2.2.2.)
//  eth0(233.0.0.1| 3.3.3.3)
//  (244.44.44.44 | 4.4.4.4)
//};

        UT_CHECK(t.get_name().compare("table_a") == 0);

        UT_CHECK(t.get_addr_set("", m1_1, false) == addr_set {});
        UT_CHECK(t.get_addr_set("", m2_1, false) == addr_set {});
        UT_CHECK(t.get_addr_set("", m3, false) == addr_set {});

        UT_CHECK(t.get_addr_set("eth0", m1_1, false) == range(s1_0, s1_2)->get_addr_set());
        UT_CHECK(t.get_addr_set("xxx", m1_1, false) == addr_set {});

        UT_CHECK(t.get_addr_set("xxx", m2_1, false) == addr_set {});

        UT_CHECK(t.get_addr_set("eth0", m3, false) == single(s3)->get_addr_set());
        UT_CHECK(t.get_addr_set("xxx", m3, false) == addr_set {});
        UT_CHECK(t.get_addr_set("eth0", m3, true) == single(s3)->get_addr_set());
        UT_CHECK(t.get_addr_set("xxx", m3, true) == addr_set {});

        UT_CHECK(t.get_addr_set("asdf", m4, false) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("xxx", m4, false) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("", m4, false) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("*", m4, false) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("*", m3, false) == single(s3)->get_addr_set());

        UT_CHECK(t.get_addr_set("asdf", m4, true) == addr_set {});
        UT_CHECK(t.get_addr_set("xxx", m4, true) == addr_set {});
        UT_CHECK(t.get_addr_set("", m4, true) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("*", m4, true) == addr_set {});
        UT_CHECK(t.get_addr_set("*", m3, true) == addr_set {});

        addr_set result;
        auto tmp_r = range(s1_0, s1_2)->get_addr_set();
        result.insert(tmp_r.begin(), tmp_r.end());
        result.insert(s3);
        result.insert(s4);
        UT_CHECK(t.get_addr_set("eth0", x, false) == result);

        result.insert(s2);
        UT_CHECK(t.get_addr_set("*", x, false) == result);
        UT_CHECK(t.get_addr_set("", m4, false) == single(s4)->get_addr_set());
        UT_CHECK(t.get_addr_set("", x, false) == single(s4)->get_addr_set());

        UT_SUMMARY;
    }

    test_status test_configuration_delete_comments() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        configuration c;
        UT_CHECK(c.delete_comments("#1234\n1234").compare("\n1234") == 0);
        UT_CHECK(c.delete_comments("1234\n#1234").compare("1234\n") == 0);
        UT_CHECK(c.delete_comments("#\n1234\n#1234").compare("\n1234\n") == 0);
        UT_CHECK(c.delete_comments("1234#1234\n").compare("1234\n") == 0);
        UT_CHECK(c.delete_comments("").compare("") == 0);
        UT_CHECK(c.delete_comments("##\n1234").compare("\n1234") == 0);
        UT_CHECK(c.delete_comments("\n1234#").compare("\n1234") == 0);
        UT_CHECK(c.delete_comments("#12#34\n#56#78#910\n\n\n1234#").compare("\n\n\n\n1234") == 0);
        UT_CHECK(c.delete_comments("#12#34\n#56#78#910\n1\n2\n3\n4#").compare("\n\n1\n2\n3\n4") == 0);
        UT_CHECK(c.delete_comments("1234#\n").compare("1234\n") == 0);

        UT_SUMMARY;
    }

    test_status test_configuration_separate_comments() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        configuration c;
        auto tmp = c.separate_commands(";a; as; asd;;;\nasdf; asdfg\n");
        UT_CHECK(tmp[0].first == 1);
        UT_CHECK(tmp[0].second == "a");

        UT_CHECK(tmp[1].first == 1);
        UT_CHECK(tmp[1].second == "as");

        UT_CHECK(tmp[2].first == 1);
        UT_CHECK(tmp[2].second == "asd");

        UT_CHECK(tmp[3].first == 2);
        UT_CHECK(tmp[3].second == "asdf");

        UT_CHECK(tmp[4].first == 2);
        UT_CHECK(tmp[4].second == "asdfg");

        UT_SUMMARY;
    }

    test_status test_configuration() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        using addr_set = std::set<addr_storage>;

        std::string test_conf = {" \
protocol IGMPv3; \n \
pinstance myProxy(33): eth0 \"!§$%&/()=?_-<>||@{}[]:,\" ==> wlan0; \n \
pinstance pinst: ifa ifb ==> ifa ifc; \n \
\n \
table allx { \n \
    (*|*) \n \
}; \n \
\n \
table random_stuff { \n \
   yyy(239.1.1.1| 1.1.1.1) \n \
   zzz(244.1.1.1 - 244.2.1.1| 2.2.2.2) \n \
   ( * | 66.66.66.66) \n \
   xxx(239.99.99.99| *) \n \
}; \n \
\n \
pinstance pinst upstream * in rulematching mutex 12345; \n \
\n \
pinstance pinst upstream ifa in blacklist table allx; \n \
pinstance pinst upstream ifa out whitelist table {(table random_stuff) (table allx)}; \n \
\n \
pinstance pinst upstream ifb in blacklist table random_stuff; \n \
pinstance pinst upstream ifb out whitelist table random_stuff; \n \
\n \
pinstance pinst downstream ifa in blacklist table {asdf(*| 5.5.5.5)}; \n \
        "
                                };

        configuration c;
        c.m_cmds = c.separate_commands(c.delete_comments(std::move(test_conf)));
        c.run_parser();
        //std::cout << std::endl <<c.to_string() << std::endl;
        UT_CHECK(c.get_group_mem_protocol() == IGMPv3);

        auto inst_def_set = c.get_inst_def_set();
        UT_CHECK(inst_def_set.size() == 2);

        auto pinstance = *(inst_def_set.begin());
        UT_CHECK(pinstance->get_instance_name().compare("myProxy") == 0);
        UT_CHECK(pinstance->get_upstreams().size() == 2);
        UT_CHECK(pinstance->get_downstreams().size() == 1);
        UT_CHECK(pinstance->get_global_settings().size() == 0);
        UT_CHECK(pinstance->get_table_number() == 33);
        UT_CHECK(pinstance->get_user_selected_table_number());

        auto upstreams = pinstance->get_upstreams();
        auto downstreams = pinstance->get_downstreams();

        auto uif_1 = *(upstreams.begin());
        auto uif_2 = *(++upstreams.begin());
        auto dif_1 = *(downstreams.begin());
        UT_CHECK(uif_1->get_if_name().compare("eth0") == 0);
        UT_CHECK(uif_2->get_if_name().compare("!§$%&/()=?_-<>||@{}[]:,") == 0);
        UT_CHECK(dif_1->get_if_name().compare("wlan0") == 0);

        UT_CHECK(uif_1->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(uif_1->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(uif_2->get_filter_type(ID_IN) == FT_BLACKLIST);

        UT_CHECK(dif_1->get_filter_type(ID_OUT) == FT_BLACKLIST);
        UT_CHECK(uif_2->get_filter_type(ID_OUT) == FT_BLACKLIST);
        UT_CHECK(dif_1->get_filter_type(ID_OUT) == FT_BLACKLIST);

        UT_CHECK(uif_1->get_saddr_set(ID_IN, "", addr_storage("239.1.1.1")) == addr_set {});
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "asdf", addr_storage("239.1.1.1")) == addr_set {});
        UT_CHECK(dif_1->get_saddr_set(ID_OUT, "asdf", addr_storage("0.0.0.0")) == addr_set {});

        UT_CHECK(uif_1->is_source_allowed(ID_IN, "xxx", addr_storage("1.2.3.4"), addr_storage("2.3.4.5")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("1.2.3.4"), addr_storage("2.3.4.5")));
        UT_CHECK(uif_2->is_source_allowed(ID_IN, "", addr_storage("1.2.3.99"), addr_storage("2.3.4.5")));

        pinstance = *(++inst_def_set.begin());
        UT_CHECK(pinstance->get_instance_name().compare("pinst") == 0);
        UT_CHECK(pinstance->get_upstreams().size() == 2);
        UT_CHECK(pinstance->get_downstreams().size() == 2);
        UT_CHECK(pinstance->get_global_settings().size() == 1);
        UT_CHECK(!pinstance->get_user_selected_table_number());

        auto global_settings = pinstance->get_global_settings();
        UT_CHECK(global_settings.size() == 1);
        auto rule_1 = *(global_settings.begin());
        UT_CHECK(rule_1->get_rule_binding_type() == RBT_RULE_MATCHING);
        UT_CHECK(rule_1->get_instance_name().compare("pinst") == 0);
        UT_CHECK(rule_1->get_interface_type() == IT_UPSTREAM);
        UT_CHECK(rule_1->get_if_name().compare("*") == 0);
        UT_CHECK(rule_1->get_interface_direction() == ID_IN);
        UT_CHECK(rule_1->get_rule_matching_type() == RMT_MUTEX);
        UT_CHECK(rule_1->get_timeout().count() == 12345);

        upstreams = pinstance->get_upstreams();
        downstreams = pinstance->get_downstreams();

        uif_1 = *(upstreams.begin());
        uif_2 = *(++upstreams.begin());
        dif_1 = *(downstreams.begin());
        auto dif_2 = *(++downstreams.begin());
        UT_CHECK(uif_1->get_if_name().compare("ifa") == 0);
        UT_CHECK(uif_2->get_if_name().compare("ifb") == 0);
        UT_CHECK(dif_1->get_if_name().compare("ifa") == 0);
        UT_CHECK(dif_2->get_if_name().compare("ifc") == 0);

        UT_CHECK(uif_1->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(uif_1->get_saddr_set(ID_IN, "", addr_storage("0.0.0.0")) == addr_set {addr_storage("0.0.0.0")});
        UT_CHECK(!uif_1->is_source_allowed(ID_IN, "ifa", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(!uif_1->is_source_allowed(ID_IN, "", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(!uif_1->is_source_allowed(ID_IN, "", addr_storage("239.1.1.1"), addr_storage("0.0.0.0")));

        UT_CHECK(uif_1->get_filter_type(ID_OUT) == FT_WHITELIST);
        UT_CHECK(uif_1->get_saddr_set(ID_OUT, "", addr_storage("0.0.0.0")) == addr_set {addr_storage("0.0.0.0")});
        UT_CHECK(uif_1->get_saddr_set(ID_OUT, "asdf", addr_storage("0.0.0.0")) == addr_set {addr_storage("0.0.0.0")});
        UT_CHECK(uif_1->get_saddr_set(ID_OUT, "asdf", addr_storage("224.0.0.0")) == addr_set {addr_storage("0.0.0.0")});
        UT_CHECK(uif_1->is_source_allowed(ID_OUT, "asdf", addr_storage("224.0.0.0"), addr_storage("1.1.1.1")));
        UT_CHECK(uif_1->is_source_allowed(ID_OUT, "asdf", addr_storage("224.0.0.0"), addr_storage("0.0.0.0")));

        UT_CHECK(uif_2->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "", addr_storage("239.99.99.99")) == addr_set {addr_storage("66.66.66.66")});
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "xxx", addr_storage("239.99.99.99")) == addr_set {addr_storage("0.0.0.0")});
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "zzz", addr_storage("239.99.99.99")) == addr_set {addr_storage("66.66.66.66")});
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "yyy", addr_storage("239.1.1.1")) == (addr_set {addr_storage("1.1.1.1"), addr_storage("66.66.66.66")}));
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "zzz", addr_storage("0.0.0.0")) == (addr_set {addr_storage("2.2.2.2"), addr_storage("66.66.66.66")}));
        UT_CHECK(uif_2->get_saddr_set(ID_IN, "zzz", addr_storage("244.2.0.55")) == (addr_set {addr_storage("2.2.2.2"), addr_storage("66.66.66.66")}));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "asdf", addr_storage("224.0.0.0"), addr_storage("66.66.66.66")));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "xxx", addr_storage("239.1.1.1"), addr_storage("66.66.66.66")));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "yyy", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(uif_2->is_source_allowed(ID_IN, "asdf", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "xxx", addr_storage("239.99.99.99"), addr_storage("1.1.1.1")));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "xxx", addr_storage("239.99.99.99"), addr_storage("0.0.0.0")));
        UT_CHECK(!uif_2->is_source_allowed(ID_IN, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.2")));
        UT_CHECK(uif_2->is_source_allowed(ID_IN, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.1")));

        UT_CHECK(uif_2->get_filter_type(ID_OUT) == FT_WHITELIST);
        UT_CHECK(uif_2->get_saddr_set(ID_OUT, "yyy", addr_storage("239.1.1.1")) == (addr_set {addr_storage("1.1.1.1"), addr_storage("66.66.66.66")}));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "asdf", addr_storage("224.0.0.0"), addr_storage("66.66.66.66")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.1.1.1"), addr_storage("66.66.66.66")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "yyy", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "asdf", addr_storage("239.1.1.1"), addr_storage("1.1.1.1")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.99.99.99"), addr_storage("1.1.1.1")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.99.99.99"), addr_storage("0.0.0.0")));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.2")));
        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.1")));

        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "asdf", addr_storage("224.0.0.0"), addr_storage("66.66.66.66"), true));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "", addr_storage("224.0.0.0"), addr_storage("66.66.66.66"), true));
        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.1.1.1"), addr_storage("66.66.66.66"), true));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "yyy", addr_storage("239.1.1.1"), addr_storage("1.1.1.1"), true));
        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "asdf", addr_storage("239.1.1.1"), addr_storage("1.1.1.1"), true));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.99.99.99"), addr_storage("1.1.1.1"), true));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "xxx", addr_storage("239.99.99.99"), addr_storage("0.0.0.0"), true));
        UT_CHECK(uif_2->is_source_allowed(ID_OUT, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.2"), true));
        UT_CHECK(!uif_2->is_source_allowed(ID_OUT, "zzz", addr_storage("244.1.3.3"), addr_storage("2.2.2.1"), true));


        UT_CHECK(dif_1->is_source_allowed(ID_IN, "asdf", addr_storage("239.99.99.99"), addr_storage("2.2.2.2")));
        UT_CHECK(!dif_1->is_source_allowed(ID_IN, "asdf", addr_storage("239.99.99.99"), addr_storage("5.5.5.5")));
        UT_CHECK(dif_1->is_source_allowed(ID_IN, "", addr_storage("239.99.99.1"), addr_storage("5.5.5.5")));
        UT_CHECK(!dif_1->is_source_allowed(ID_IN, "*", addr_storage("239.99.99.1"), addr_storage("5.5.5.5")));

        UT_SUMMARY;
    }

};

std::list<std::tuple<ut_test_fun, ut_effort>> test_parser_functions()
{
    HC_LOG_TRACE("");
    test_parser p;

    return std::list<std::tuple<ut_test_fun, ut_effort>> {
        std::make_tuple([&p]() {
            return p.test_rb_filter_mc_filter();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_single_addr();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_addr_range();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_rule_addr();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_table();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_configuration_delete_comments();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_configuration_separate_comments();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_configuration();
        }, 1)
    };
}





#endif //UNIT_TESTS

#endif //TEST_PARSER_HPP
