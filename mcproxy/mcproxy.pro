#QMAKE_CXX = clang
#QMAKE_CC = clang

CONFIG+=mcproxy #default mode

tester {
    CONFIG-=mcproxy #removes default mode
    message("target tester")
    TARGET = tester
    DEFINES += TESTER

    SOURCES += src/tester/config_map.cpp \
           src/tester/tester.cpp

    HEADERS += include/tester/config_map.hpp \
           include/tester/tester.hpp

    LIBS += -L/usr/lib -lboost_regex
}

mcproxy { #default mode
    message("target mcproxy")
    TARGET = mcproxy
}

CONFIG(debug, debug|release) {
    message("debug mode")
    DEFINES += DEBUG_MODE
}

CONFIG(release, debug|release) { #default mode
    message("release mode")
}

CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11

QMAKE_CXXFLAGS_DEBUG -= -g 
QMAKE_CXXFLAGS_DEBUG += -ggdb 
QMAKE_CXXFLAGS_DEBUG += -Wpedantic 
#QMAKE_CXXFLAGS_DEBUG += -Weverything #clang only


target.path = /usr/local/bin
INSTALLS += target

doc.commands = doxygen ../doxygen/Doxyfile
QMAKE_EXTRA_TARGETS += doc

SOURCES += src/main.cpp \
           src/hamcast_logging.cpp \
               #utils
           src/utils/mc_socket.cpp \
           src/utils/addr_storage.cpp \
           src/utils/mroute_socket.cpp \
           src/utils/if_prop.cpp \
           src/utils/reverse_path_filter.cpp \
               #proxy
           src/proxy/proxy.cpp \
           src/proxy/sender.cpp \
           src/proxy/receiver.cpp \
           src/proxy/mld_receiver.cpp \
           src/proxy/igmp_receiver.cpp \
           src/proxy/mld_sender.cpp \
           src/proxy/igmp_sender.cpp \
           src/proxy/proxy_instance.cpp \
           src/proxy/routing.cpp \
           src/proxy/worker.cpp \
           src/proxy/timing.cpp \
           src/proxy/check_if.cpp \
           src/proxy/check_kernel.cpp \
           src/proxy/membership_db.cpp \
           src/proxy/querier.cpp \
           src/proxy/timers_values.cpp \
           src/proxy/interfaces.cpp \
           src/proxy/def.cpp \
           src/proxy/simple_routing_management.cpp \
           src/proxy/simple_routing_data.cpp \
           src/proxy/simple_membership_aggregation.cpp \
               #parser
           src/parser/scanner.cpp \
           src/parser/token.cpp \
           src/parser/configuration.cpp \
           src/parser/parser.cpp \
           src/parser/interface.cpp

HEADERS += include/hamcast_logging.h \
                #utils
           include/utils/mc_socket.hpp \
           include/utils/addr_storage.hpp \
           include/utils/reverse_path_filter.hpp \
           include/utils/mroute_socket.hpp \
           include/utils/if_prop.hpp \
           include/utils/extended_mld_defines.hpp \
           include/utils/extended_igmp_defines.hpp \
               #proxy
           include/proxy/proxy.hpp \
           include/proxy/sender.hpp \
           include/proxy/receiver.hpp \
           include/proxy/mld_receiver.hpp \
           include/proxy/igmp_receiver.hpp \
           include/proxy/mld_sender.hpp \
           include/proxy/igmp_sender.hpp \
           include/proxy/proxy_instance.hpp \
           include/proxy/message_queue.hpp \
           include/proxy/message_format.hpp \
           include/proxy/routing.hpp \
           include/proxy/worker.hpp \
           include/proxy/timing.hpp \
           include/proxy/check_if.hpp \
           include/proxy/check_kernel.hpp \
           include/proxy/membership_db.hpp \
           include/proxy/def.hpp \
           include/proxy/querier.hpp \
           include/proxy/timers_values.hpp \
           include/proxy/interfaces.hpp \
           include/proxy/routing_management.hpp \
           include/proxy/simple_routing_management.hpp \
           include/proxy/simple_routing_data.hpp \
           include/proxy/simple_membership_aggregation.hpp \
               #parser
           include/parser/scanner.hpp \
           include/parser/token.hpp \
           include/parser/configuration.hpp \
           include/parser/parser.hpp \
           include/parser/interface.hpp

LIBS += -L/usr/lib -lpthread 

QMAKE_CLEAN += thread* 
  
