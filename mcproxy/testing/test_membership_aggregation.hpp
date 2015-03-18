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

#ifndef  TEST_MEMBERSHIP_AGGREGATION_HPP
#define  TEST_MEMBERSHIP_AGGREGATION_HPP

#ifdef UNIT_TESTS

#include "testing/ut_bootstrap.hpp"
#include "testing/ut_suite.hpp"
#include "include/proxy/simple_membership_aggregation.hpp"
#include "include/proxy/message_format.hpp"

#include "include/proxy/simple_routing_data.hpp"
#include "include/parser/configuration.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/timing.hpp"


struct test_membership_aggregation {

    test_status test_merge_reminder_disjoint_fun() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        using mss = mem_source_state;
        using fss = filter_source_state;
        using sl = source_list<source>;

        const addr_storage s0("0.0.0.0");
        const addr_storage s1("1.1.1.1");
        const addr_storage s2("2.2.2.2");
        const addr_storage s3("3.3.3.3");
        const mss in_a(INCLUDE_MODE, sl {
            s1, s2
        });
        const mss ex_a(EXCLUDE_MODE, sl {
            s1, s2
        });
        const mss in_b(INCLUDE_MODE, sl {
            s1, s3
        });
        const mss ex_b(EXCLUDE_MODE, sl {
            s1, s3
        });

        const fss wl_b(FT_WHITELIST, sl {
            s1, s3
        });
        const fss bl_b(FT_BLACKLIST, sl {
            s1, s3
        });
        const fss wl_wc(FT_WHITELIST, sl {
            s0
        });
        const fss bl_wc(FT_BLACKLIST, sl {
            s0
        });

        simple_membership_aggregation s_mem_agg(IGMPv3);

        auto check_merge_convert_wildcard_filter = [&](const fss & convert, const fss & result) {
            fss to_tmp = convert;
            s_mem_agg.convert_wildcard_filter(to_tmp);

            if (to_tmp == result) {
                return true;
            } else {
                return false;
            }
        };

        UT_CHECK(check_merge_convert_wildcard_filter(wl_b, wl_b));
        UT_CHECK(check_merge_convert_wildcard_filter(bl_b, bl_b));
        UT_CHECK(check_merge_convert_wildcard_filter(wl_wc, fss(FT_BLACKLIST, sl {})));

        UT_CHECK(check_merge_convert_wildcard_filter(bl_wc, fss(FT_WHITELIST, sl {})));
        UT_CHECK(check_merge_convert_wildcard_filter(fss(FT_WHITELIST, sl {s1, s2, s0, s3}), fss(FT_BLACKLIST, sl {})));
        UT_CHECK(check_merge_convert_wildcard_filter(fss(FT_BLACKLIST, sl {s1, s2, s0, s3}), fss(FT_WHITELIST, sl {})));


        auto check_merge_group_memberships = [&](const mss & to, const mss & from, const mss & result) {
            mss to_tmp = to;
            s_mem_agg.merge_group_memberships(to_tmp, from);

            if (to_tmp == result) {
                return true;
            } else {
                return false;
            }
        };

        //IN{s1,s2} merge with IN{s1,s3} = IN{s1,s2,s3}
        UT_CHECK(check_merge_group_memberships(in_a, in_b, mss(INCLUDE_MODE, sl {s1, s2, s3})));

        //IN{s1,s2} merge with EX{s1,s3} = EX{s3}
        UT_CHECK(check_merge_group_memberships(in_a, ex_b, mss(EXCLUDE_MODE, sl {s3})));

        //EX{s1,s2} merge with IN{s1,s3} = EX{s2}
        UT_CHECK(check_merge_group_memberships(ex_a, in_b, mss(EXCLUDE_MODE, sl {s2})));

        //EX{s1,s2} merge with EX{s1,s3} = EX{s1}
        UT_CHECK(check_merge_group_memberships(ex_a, ex_b, mss(EXCLUDE_MODE, sl {s1})));

        auto check_merge_memberships_filter = [&](const mss & to, const fss & from, const mss & expected_result) {
            mss to_result_tmp = to;
            s_mem_agg.merge_memberships_filter(to_result_tmp, from);

            if (to_result_tmp == expected_result) {
                return true;
            } else {
                return false;
            }
        };

        //IN{s1,s2} merge with WL{s1,s3} = IN{s1} R(IN{s2})
        UT_CHECK(check_merge_memberships_filter(in_a, wl_b, mss(INCLUDE_MODE, sl {s1})));
        //IN{s1,s2} merge with BL{s1,s3} = IN{s2} R(IN{s1})
        UT_CHECK(check_merge_memberships_filter(in_a, bl_b, mss(INCLUDE_MODE, sl {s2})));
        //EX{s1,s2} merge with WL{s1,s3} = IN{s3} R(EX{s1,s2,s3})
        UT_CHECK(check_merge_memberships_filter(ex_a, wl_b, mss(INCLUDE_MODE, sl {s3})));
        //EX{s1,s2} merge with BL{s1,s3} = EX{s1,s2,s3} R(IN{s3})
        UT_CHECK(check_merge_memberships_filter(ex_a, bl_b, mss(EXCLUDE_MODE, sl {s1, s2, s3})));


        //IN{s1,s2} merge with WL{*} = IN{s1,s2} R(IN{})
        UT_CHECK(check_merge_memberships_filter(in_a, wl_wc, mss(INCLUDE_MODE, sl {s1, s2})));
        //IN{s1,s2} merge with BL{*} = IN{} R(IN{s1,s2})
        UT_CHECK(check_merge_memberships_filter(in_a, bl_wc, mss(INCLUDE_MODE, sl {})));
        //EX{s1,s2} merge with WL{*} = EX{s1,s2} R(IN{})
        UT_CHECK(check_merge_memberships_filter(ex_a, wl_wc, mss(EXCLUDE_MODE, sl {s1, s2})));
        //EX{s1,s2} merge with BL{*} = IN{} R(EX{s1,s2})
        UT_CHECK(check_merge_memberships_filter(ex_a, bl_wc, mss(INCLUDE_MODE, sl {})));


        auto check_merge_memberships_filter_reminder = [&](const mss & to, const fss & from, const mss & expected_result, const mss & expected_reminder) {
            mss to_result_tmp = to;
            s_mem_agg.merge_memberships_filter(to_result_tmp, from);

            if (to_result_tmp != expected_result) {
                return false;
            }

            mss to_reminder_tmp = to;
            s_mem_agg.merge_memberships_filter_reminder(to_reminder_tmp, to_result_tmp, from);

            if (to_reminder_tmp == expected_reminder) {
                return true;
            } else {
                return false;
            }
        };

        //IN{s1,s2} merge with WL{s1,s3} = IN{s1} R(IN{s2})
        UT_CHECK(check_merge_memberships_filter_reminder(in_a, wl_b, mss(INCLUDE_MODE, sl {s1}), mss(INCLUDE_MODE, sl {s2})));
        //IN{s1,s2} merge with BL{s1,s3} = IN{s2} R(IN{s1})
        UT_CHECK(check_merge_memberships_filter_reminder(in_a, bl_b, mss(INCLUDE_MODE, sl {s2}), mss(INCLUDE_MODE, sl {s1})));
        //EX{s1,s2} merge with WL{s1,s3} = IN{s3} R(EX{s1,s2,s3})
        UT_CHECK(check_merge_memberships_filter_reminder(ex_a, wl_b, mss(INCLUDE_MODE, sl {s3}), mss(EXCLUDE_MODE, sl {s1, s2, s3})));
        //EX{s1,s2} merge with BL{s1,s3} = EX{s1,s2,s3} R(IN{s3})
        UT_CHECK(check_merge_memberships_filter_reminder(ex_a, bl_b, mss(EXCLUDE_MODE, sl {s1, s2, s3}), mss(INCLUDE_MODE, sl {s3})));


        //IN{s1,s2} merge with WL{*} = IN{s1,s2} R(IN{})
        UT_CHECK(check_merge_memberships_filter_reminder(in_a, wl_wc, mss(INCLUDE_MODE, sl {s1, s2}), mss(INCLUDE_MODE, sl {})));
        //IN{s1,s2} merge with BL{*} = IN{} R(IN{s1,s2})
        UT_CHECK(check_merge_memberships_filter_reminder(in_a, bl_wc, mss(INCLUDE_MODE, sl {}), mss(INCLUDE_MODE, sl {s1, s2})));
        //EX{s1,s2} merge with WL{*} = EX{s1,s2} R(IN{})
        UT_CHECK(check_merge_memberships_filter_reminder(ex_a, wl_wc, mss(EXCLUDE_MODE, sl {s1, s2}), mss(INCLUDE_MODE, sl {})));
        //EX{s1,s2} merge with BL{*} = IN{} R(EX{s1,s2})
        UT_CHECK(check_merge_memberships_filter_reminder(ex_a, bl_wc, mss(INCLUDE_MODE, sl {}), mss(EXCLUDE_MODE, sl {s1, s2})));


        auto check_disjoin_group_memberships = [&](const mss & to, const mss & from, const mss & expected_result) {
            mss to_result_tmp = to;
            s_mem_agg.disjoin_group_memberships(to_result_tmp, from);

            if (to_result_tmp == expected_result) {
                return true;
            } else {
                return false;
            }
        };

        UT_CHECK(check_disjoin_group_memberships(in_a, in_b, mss(INCLUDE_MODE, sl {s2})));
        UT_CHECK(check_disjoin_group_memberships(in_a, ex_b, mss(INCLUDE_MODE, sl {s1})));
        UT_CHECK(check_disjoin_group_memberships(ex_a, in_b, mss(EXCLUDE_MODE, sl {s1, s2, s3})));
        UT_CHECK(check_disjoin_group_memberships(ex_a, ex_b, mss(INCLUDE_MODE, sl {s3})));

        UT_SUMMARY;
    }

    simple_membership_aggregation get_simple_membership_aggregation_mockup(std::string&& test_conf, std::shared_ptr<group_record_msg> grecord_for_qd4, std::shared_ptr<group_record_msg> grecord_for_qd5, std::shared_ptr<group_record_msg> grecord_for_qd6, const addr_storage& gaddr, group_mem_protocol gmp, std::list <std::pair<unsigned int, addr_storage>> routing_data) {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        std::string test_conf_basic = {" \
protocol IGMPv3; \n \
pinstance pinst: u1 u2 u3 ==> d4 d5 d6; \n \
#interface index: 1  2  3 ==>  4  5  6  \n \
#interface prio:  1  2  3 ==> ...   \n \
"
                                      };
        test_conf_basic += test_conf;
        std::string tmp_config = test_conf_basic;
        std::unique_ptr<configuration> c;
        try {
            c = std::unique_ptr<configuration>(new configuration(std::move(test_conf_basic)));
        } catch (const char* e) {
            UT_FAIL("Incorrect configuration(see logfile for detailed info): " << std::endl << tmp_config);
        }

        auto inst_def_set = c->get_inst_def_set();
        auto pinstance = *(inst_def_set.begin());

        auto upstreams = pinstance->get_upstreams();
        auto downstreams = pinstance->get_downstreams();
        auto global_settings = pinstance->get_global_settings();

        std::shared_ptr<simple_routing_data> srd = std::make_shared<simple_routing_data>(gmp, nullptr);

        for (auto & e : routing_data) {
            srd->set_source(e.first, gaddr, e.second);
        }

        std::shared_ptr<interface_infos> ii = std::make_shared<interface_infos>(std::string("test_membership_aggregation"));
        ii->m_upstreams.insert(upstream_infos(1, *upstreams.begin(), 1));
        ii->m_upstreams.insert(upstream_infos(2, *(++upstreams.begin()), 2));
        ii->m_upstreams.insert(upstream_infos(3, *(++(++upstreams.begin())), 3));


        class dummy_timing: public timing
        {
        public:
            dummy_timing()
                : timing(true) {
            }

            void add_time(std::chrono::milliseconds, const worker*, const std::shared_ptr<proxy_msg>&) {
            }
        };
        auto dtiming = std::make_shared<dummy_timing>();

        std::unique_ptr<querier> qd4(new querier(IGMPv3, 4, dtiming, [](unsigned int, const addr_storage&) {}));
        qd4->receive_record(grecord_for_qd4);
        ii->m_downstreams.insert(std::make_pair(4, downstream_infos(std::move(qd4), *downstreams.begin())));
        std::unique_ptr<querier> qd5(new querier(IGMPv3, 5, dtiming, [](unsigned int, const addr_storage&) {}));
        qd5->receive_record(grecord_for_qd5);
        ii->m_downstreams.insert(std::make_pair(5, downstream_infos(std::move(qd5), *(++downstreams.begin()))));
        std::unique_ptr<querier> qd6(new querier(IGMPv3, 6, dtiming, [](unsigned int, const addr_storage&) {}));
        qd6->receive_record(grecord_for_qd6);
        ii->m_downstreams.insert(std::make_pair(6, downstream_infos(std::move(qd6), *(++(++downstreams.begin())))));

        if (global_settings.size() >= 1) {
            auto gs_it1 = global_settings.begin();
            if ((*gs_it1)->get_interface_type() == IT_UPSTREAM) {
                if ((*gs_it1)->get_interface_direction() == ID_IN) {
                    ii->m_upstream_input_rule = *gs_it1;
                } else if ((*gs_it1)->get_interface_direction() == ID_OUT) {
                    ii->m_upstream_output_rule = *gs_it1;
                } else {
                    HC_LOG_ERROR("should not be reachable");
                }
            }

            if (global_settings.size() == 2) {
                auto gs_it2 = ++global_settings.begin();
                if ((*gs_it2)->get_interface_type() == IT_UPSTREAM ) {
                    if ((*gs_it2)->get_interface_direction() == ID_IN) {
                        ii->m_upstream_input_rule = *gs_it2;
                    } else if ((*gs_it2)->get_interface_direction() == ID_OUT) {
                        ii->m_upstream_output_rule = *gs_it2;
                    } else {
                        HC_LOG_ERROR("should not be reachable");
                    }
                }
            }
        }

        return simple_membership_aggregation(RMT_FIRST, gaddr, srd, gmp, ii);
    }

    test_status test_simple_membership_aggregation_mockup() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        addr_storage gaddr("239.99.99.99");
        group_mem_protocol gmp = IGMPv3;

        std::string test_conf = {" \
#protocol IGMPv3; DO NOT USE!!! \n \
#pinstance pinst: u1 u2 u3 ==> d4 d5 d6; DO NOT USE!!! \n \
\n \
pinstance pinst upstream * in rulematching mutex 10; \n \
pinstance pinst upstream * out rulematching first; \n \
\n \
pinstance pinst upstream u1 in blacklist table {(*|*)}; \n \
pinstance pinst upstream u1 out whitelist table {(*|*)}; \n \
\n \
pinstance pinst upstream u2 in whitelist table {(*|*)}; \n \
pinstance pinst upstream u2 out blacklist table {(*|*)}; \n \
\n \
pinstance pinst upstream u3 in whitelist table {(*|*)}; \n \
pinstance pinst upstream u3 out blacklist table {(*|*)}; \n \
\n \
pinstance pinst downstream d4 in whitelist table {(*|*)}; \n \
pinstance pinst downstream d4 out blacklist table {(*|*)}; \n \
\n \
pinstance pinst downstream d5 in blacklist table {(*|*)}; \n \
pinstance pinst downstream d5 out whitelist table {(*|*)}; \n \
        "
                                };

        source_list<source> slist_d4 {addr_storage("1.1.1.1")};
        auto grecord_for_qd4 = std::make_shared<group_record_msg>(4, MODE_IS_INCLUDE, gaddr, std::move(slist_d4), gmp);

        source_list<source> slist_d5 {addr_storage("2.2.2.2")};
        auto grecord_for_qd5 = std::make_shared<group_record_msg>(5, MODE_IS_EXCLUDE, gaddr, std::move(slist_d5), gmp);

        source_list<source> slist_d6 {addr_storage("3.3.3.3")};
        auto grecord_for_qd6 = std::make_shared<group_record_msg>(6, MODE_IS_EXCLUDE, gaddr, std::move(slist_d6), gmp);

        std::list < std::pair<unsigned int, addr_storage>> routing_data {
            std::make_pair<unsigned int, addr_storage>(1, addr_storage("3.3.3.3")),
            std::make_pair<unsigned int, addr_storage>(2, addr_storage("4.4.4.4")),
            std::make_pair<unsigned int, addr_storage>(2, addr_storage("5.5.5.5"))
        };

        auto sma = get_simple_membership_aggregation_mockup(std::move(test_conf), grecord_for_qd4, grecord_for_qd5, grecord_for_qd6, gaddr, gmp, routing_data);

        UT_CHECK(sma.m_routing_data->get_available_sources(gaddr) == (source_list<source> {addr_storage("3.3.3.3"), addr_storage("4.4.4.4"), addr_storage("5.5.5.5")}));

        UT_CHECK(sma.m_group_mem_protocol == IGMPv3);

        auto& if_infos = sma.m_ii;
        UT_CHECK(if_infos->m_upstreams.size() == 3);
        UT_CHECK(if_infos->m_downstreams.size() == 3);

        UT_CHECK(if_infos->m_upstream_input_rule->get_rule_binding_type() == RBT_RULE_MATCHING);
        UT_CHECK(if_infos->m_upstream_input_rule->get_interface_direction() == ID_IN);
        UT_CHECK(if_infos->m_upstream_input_rule->get_rule_matching_type() == RMT_MUTEX);

        UT_CHECK(if_infos->m_upstream_output_rule->get_rule_binding_type() == RBT_RULE_MATCHING);
        UT_CHECK(if_infos->m_upstream_output_rule->get_interface_direction() == ID_OUT);
        UT_CHECK(if_infos->m_upstream_output_rule->get_rule_matching_type() == RMT_FIRST);

        auto& u1 = *(if_infos->m_upstreams.begin());
        auto& u2 = *(++if_infos->m_upstreams.begin());
        auto& u3 = *(++(++if_infos->m_upstreams.begin()));

        UT_CHECK(u1.m_if_index == 1);
        UT_CHECK(u2.m_if_index == 2);
        UT_CHECK(u3.m_if_index == 3);

        UT_CHECK(u1.m_interface->get_if_name().compare("u1") == 0);
        UT_CHECK(u2.m_interface->get_if_name().compare("u2") == 0);
        UT_CHECK(u3.m_interface->get_if_name().compare("u3") == 0);

        UT_CHECK(u1.m_interface->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(u1.m_interface->get_filter_type(ID_OUT) == FT_WHITELIST);

        UT_CHECK(u2.m_interface->get_filter_type(ID_IN) == FT_WHITELIST);
        UT_CHECK(u2.m_interface->get_filter_type(ID_OUT) == FT_BLACKLIST);

        UT_CHECK(u3.m_interface->get_filter_type(ID_IN) == FT_WHITELIST);
        UT_CHECK(u3.m_interface->get_filter_type(ID_OUT) == FT_BLACKLIST);

        auto& d4 = *(if_infos->m_downstreams.begin());
        auto& d5 = *(++if_infos->m_downstreams.begin());
        auto& d6 = *(++(++if_infos->m_downstreams.begin()));

        UT_CHECK(d4.second.m_interface->get_if_name().compare("d4") == 0);
        UT_CHECK(d5.second.m_interface->get_if_name().compare("d5") == 0);
        UT_CHECK(d6.second.m_interface->get_if_name().compare("d6") == 0);

        UT_CHECK(d4.second.m_interface->get_filter_type(ID_IN) == FT_WHITELIST);
        UT_CHECK(d4.second.m_interface->get_filter_type(ID_OUT) == FT_BLACKLIST);

        UT_CHECK(d5.second.m_interface->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(d5.second.m_interface->get_filter_type(ID_OUT) == FT_WHITELIST);

        UT_CHECK(d6.second.m_interface->get_filter_type(ID_IN) == FT_BLACKLIST);
        UT_CHECK(d6.second.m_interface->get_filter_type(ID_OUT) == FT_BLACKLIST);

        UT_CHECK(d4.second.m_querier->get_group_membership_infos(gaddr).first == INCLUDE_MODE);
        UT_CHECK(d4.second.m_querier->get_group_membership_infos(gaddr).second == source_list<source> {addr_storage("1.1.1.1")});

        UT_CHECK(d5.second.m_querier->get_group_membership_infos(gaddr).first == EXCLUDE_MODE);
        UT_CHECK(d5.second.m_querier->get_group_membership_infos(gaddr).second == source_list<source> {addr_storage("2.2.2.2")});

        UT_CHECK(d6.second.m_querier->get_group_membership_infos(gaddr).first == EXCLUDE_MODE);
        UT_CHECK(d6.second.m_querier->get_group_membership_infos(gaddr).second == source_list<source> {addr_storage("3.3.3.3")});

        UT_SUMMARY;
    }


    test_status test_simple_membership_aggregation_first_whitelist() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        addr_storage gaddr("239.99.99.99");
        group_mem_protocol gmp = IGMPv3;

        using slist = source_list<source>;
        using addr = addr_storage;

        std::string test_conf = {" \
#protocol IGMPv3; DO NOT USE!!! \n \
#pinstance pinst: u1 u2 u3 ==> d4 d5 d6; DO NOT USE!!! \n \
#interface index: 1  2  3 ==>  4  5  6 \n \
#interface prio:  1  2  3 ==> ...      \n \
 \n \
table tu1 { \n \
    (239.99.99.99 | 4.4.4.4) #(a) \n \
    (239.99.99.99 | 16.16.16.16) #(b) \n \
}; \n \
 \n \
table tu2 { \n \
    u2 (239.99.99.99 | 5.5.5.5) #(a) \n \
    (239.99.99.99 | 14.14.14.14) #(b) \n \
    (239.99.99.99 | 15.15.15.15) #(b) \n \
}; \n \
 \n \
table tu3 { \n \
#   (239.99.99.99 | 6.6.6.6) #a) block 6.6.6.6 \n \
    (239.99.99.99 | 17.17.17.17) #(b) \n \
    (239.99.99.99 | 24.24.24.24) #(c) \n \
}; \n \
 \n \
table td4 { \n \
    u1 (239.99.99.99 | 4.4.4.4) #(a) \n \
    (239.99.99.99 | 14.14.14.14) #(b) \n \
    (239.99.99.99 | 24.24.24.24) #(c) \n \
    (239.99.99.99 | 25.25.25.25) #(c) \n \
}; \n \
 \n \
table td5 { \n \
    u2 (239.99.99.99 | 5.5.5.5) #(a) \n \
    (239.99.99.99 | 15.15.15.15) #(b) \n \
}; \n \
 \n \
table td6 { \n \
   u3 (239.99.99.99 | 6.6.6.6) #(a) \n \
    (239.99.99.99 | 16.16.16.16) #(b) \n \
    (239.99.99.99 | 17.17.17.17) #(b) \n \
}; \n \
 \n \
 \n \
pinstance pinst upstream * in rulematching first; \n \
 \n \
pinstance pinst upstream u1 in whitelist table tu1; \n \
pinstance pinst upstream u2 in whitelist table tu2; \n \
pinstance pinst upstream u3 in whitelist table tu3; \n \
 \n \
pinstance pinst downstream d4 out whitelist table td4; \n \
pinstance pinst downstream d5 out whitelist table td5; \n \
pinstance pinst downstream d6 out whitelist table td6; \n \
 \n \
#Scenario: \n \
#       (a)                    (b)                     (c) \n \
#       +-----------------+    +-----------------+     +-----------------+ \n \
#       | 1      2      3 |    | 1      2      3 |     | 1      2      3 | \n \
#       | ^      ^      ^ |    | ^    ^ ^      ^ |     |              ^  | \n \
#       | |      |      | |    | |   /  |      | |     |             / x | \n \
#       | |      |      | |    | +--+---+----+ | |     |   ---------- /  | \n \
#       | |      |      | |    |   /    |    | | |     |  / ----------   | \n \
#       | 4      5      6 |    | 4      5      6 |     | 4      5      6 | \n \
#       +-----------------+    +-----------------+     +-----------------+ \n \
#Addr:    4      5      5       14     15    16/17     24/25 \n \
#        in     in     in       in     in     in        in  \n \
        "
                                };

        slist slist_d4 {addr("4.4.4.4"), addr("14.14.14.14"), addr("24.24.24.24"), addr("25.25.25.25")};
        auto grecord_for_qd4 = std::make_shared<group_record_msg>(4, MODE_IS_INCLUDE, gaddr, std::move(slist_d4), gmp);

        slist slist_d5 {addr("5.5.5.5"), addr("15.15.15.15")};
        auto grecord_for_qd5 = std::make_shared<group_record_msg>(5, MODE_IS_INCLUDE, gaddr, std::move(slist_d5), gmp);

        slist slist_d6 {addr("6.6.6.6"), addr("16.16.16.16"), addr("17.17.17.17")};
        auto grecord_for_qd6 = std::make_shared<group_record_msg>(6, MODE_IS_INCLUDE, gaddr, std::move(slist_d6), gmp);

        std::list < std::pair<unsigned int, addr_storage>> routing_data;

        auto sma = get_simple_membership_aggregation_mockup(std::move(test_conf), grecord_for_qd4, grecord_for_qd5, grecord_for_qd6, gaddr, gmp, routing_data);

        auto gmem_u1 = sma.get_group_memberships(1);
        auto gmem_u2 = sma.get_group_memberships(2);
        auto gmem_u3 = sma.get_group_memberships(3);
  
        UT_CHECK(gmem_u1.first == INCLUDE_MODE); 
        UT_CHECK(gmem_u1.second == (slist{addr("4.4.4.4"), addr("16.16.16.16")})); 

        UT_CHECK(gmem_u2.first == INCLUDE_MODE); 
        UT_CHECK(gmem_u2.second == (slist{addr("5.5.5.5"), addr("14.14.14.14"), addr("15.15.15.15")})); 

        UT_CHECK(gmem_u3.first == INCLUDE_MODE); 
        UT_CHECK(gmem_u3.second == (slist{addr("17.17.17.17"), addr("24.24.24.24")})); 

        UT_SUMMARY;
    }

    test_status test_simple_membership_aggregation_first_blacklist() {
        HC_LOG_TRACE("");
        UT_INITIALISATION;

        addr_storage gaddr("239.99.99.99");
        group_mem_protocol gmp = IGMPv3;

        using slist = source_list<source>;
        using addr = addr_storage;

        std::string test_conf = {" \
#protocol IGMPv3; DO NOT USE!!! \n \
#pinstance pinst: u1 u2 u3 ==> d4 d5 d6; DO NOT USE!!! \n \
#interface index: 1  2  3 ==>  4  5  6 \n \
#interface prio:  1  2  3 ==> ...      \n \
 \n \
table tu1 { \n \
    (239.99.99.70 - 239.99.99.110 | 5.5.5.5)  #(b) \n \
    (239.99.99.99 | 6.6.6.6) #(b) \n \
    (239.99.99.99 | 7.7.7.7) #(c) \n \
    (239.99.99.99 | 8.8.8.8) #(d) \n \
    (239.99.99.99 | 9.9.9.9) #(d) \n \
    (239.99.99.99 | 10.10.10.10) #(d) \n \
}; \n \
 \n \
table tu2 { \n \
    (239.99.99.99 | 6.6.6.6) #(b) \n \
#    (239.99.99.99 | 7.7.7.7) #(c) allready blocked in (a) \n \
    (239.99.99.99 | 9.9.9.9) #(d) \n \
}; \n \
 \n \
table tu3 { \n \
#    (239.99.99.99 | 9.9.9.9) #(d) allready blocked in (a) \n \
}; \n \
 \n \
 \n \
table td4 { \n \
    u2 ( * | * ) #(a) \n \
}; \n \
 \n \
table td5 { \n \
    (239.99.99.99 | 10.10.10.10) #(d) \n \
     \n \
}; \n \
 \n \
table td6 { \n \
    u3 ( 239.99.99.0/24 | * ) #(a) \n \
}; \n \
 \n \
pinstance pinst upstream * in rulematching first; \n \
 \n \
pinstance pinst upstream u1 in blacklist table tu1; \n \
pinstance pinst upstream u2 in blacklist table tu2; \n \
pinstance pinst upstream u3 in blacklist table tu3; \n \
 \n \
pinstance pinst downstream d4 out blacklist table td4; \n \
pinstance pinst downstream d5 out blacklist table td5; \n \
pinstance pinst downstream d6 out blacklist table td6; \n \
        "
                                };

        slist slist_d4 {addr("4.4.4.4"), addr("7.7.7.7")};
        auto grecord_for_qd4 = std::make_shared<group_record_msg>(4, MODE_IS_EXCLUDE, gaddr, std::move(slist_d4), gmp);

        slist slist_d5 {addr("5.5.5.5"), addr("7.7.7.7"), addr("8.8.8.8"), addr("10.10.10.10")};
        auto grecord_for_qd5 = std::make_shared<group_record_msg>(5, MODE_IS_INCLUDE, gaddr, std::move(slist_d5), gmp);

        slist slist_d6 {addr("6.6.6.6"), addr("9.9.9.9")};
        auto grecord_for_qd6 = std::make_shared<group_record_msg>(6, MODE_IS_EXCLUDE, gaddr, std::move(slist_d6), gmp);

        std::list < std::pair<unsigned int, addr_storage>> routing_data;

        std::cout << std::endl << "##########Start################"  << std::endl;
        auto sma = get_simple_membership_aggregation_mockup(std::move(test_conf), grecord_for_qd4, grecord_for_qd5, grecord_for_qd6, gaddr, gmp, routing_data);

        auto gmem_u1 = sma.get_group_memberships(1);
        auto gmem_u2 = sma.get_group_memberships(2);
        auto gmem_u3 = sma.get_group_memberships(3);
  
        UT_CHECK(gmem_u1.first == EXCLUDE_MODE); 
        UT_CHECK(gmem_u1.second == (slist{addr("4.4.4.4"), addr("5.5.5.5"), addr("6.6.6.6"), addr("7.7.7.7"), addr("8.8.8.8"), addr("9.9.9.9"), addr("10.10.10.10")})); 
        std::cout << "gmem_u1: " << gmem_u1.second << std::endl;
        

        UT_CHECK(gmem_u2.first == INCLUDE_MODE); 
        UT_CHECK(gmem_u2.second == (slist{addr("5.5.5.5"), addr("8.8.8.8")})); 
        std::cout << "gmem_u2: " << gmem_u2.second << std::endl;


        UT_CHECK(gmem_u3.first == INCLUDE_MODE); 
        UT_CHECK(gmem_u3.second == (slist{addr("7.7.7.7")})); 
        std::cout << "gmem_u3:" << gmem_u3.second << std::endl;

        UT_SUMMARY;
    }


};

std::list<std::tuple<ut_test_fun, ut_effort>> test_membership_aggregation_functions()
{
    HC_LOG_TRACE("");
    test_membership_aggregation p;

    return std::list<std::tuple<ut_test_fun, ut_effort>> {
        std::make_tuple([&p]() {
            return p.test_merge_reminder_disjoint_fun();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_simple_membership_aggregation_mockup();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_simple_membership_aggregation_first_whitelist();
        }, 1),
        std::make_tuple([&p]() {
            return p.test_simple_membership_aggregation_first_blacklist();
        }, 1)
    };
}


#endif //UNIT_TESTS

#endif //TEST_MEMBERSHIP_AGGREGATION_HPP
