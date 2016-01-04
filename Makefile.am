# Part of measurement-kit <https://measurement-kit.github.io/>.
# Measurement-kit is free software. See AUTHORS and LICENSE for more
# information on the copying conditions.

AUTOMAKE_OPTIONS = foreign 1.9 subdir-objects
ACLOCAL_AMFLAGS = -I m4

noinst_PROGRAMS =  # Empty
check_PROGRAMS =  # Empty
TESTS =  # Empty

VALGRIND = valgrind --trace-children=yes --error-exitcode=1 \
                    --dsymutil=yes --leak-check=yes

run-valgrind: $(check_PROGRAMS)
	for test_name in $(TESTS); do                                          \
	  $(VALGRIND) $$test_name || exit 1;                                   \
	done

example_bufferevent_echo_SOURCES = example/bufferevent-echo.cpp
if BUILD_EXAMPLES
    noinst_PROGRAMS += example/bufferevent-echo
endif
example_bufferevent_get_ssl_SOURCES = example/bufferevent-get-ssl.cpp
if BUILD_EXAMPLES
    noinst_PROGRAMS += example/bufferevent-get-ssl
endif
example_bufferevent_get_SOURCES = example/bufferevent-get.cpp
if BUILD_EXAMPLES
    noinst_PROGRAMS += example/bufferevent-get
endif
example_event_echo_SOURCES = example/event-echo.cpp
if BUILD_EXAMPLES
    noinst_PROGRAMS += example/event-echo
endif

mkokincludedir = $(includedir)/mkok
mkokinclude_HEADERS = # Empty
mkokinclude_HEADERS += include/mkok/libevent.hpp

test_main_SOURCES = test/main.cpp
if BUILD_TESTS
    check_PROGRAMS += test/main
    TESTS += test/main
endif
test_network_SOURCES = test/network.cpp
if BUILD_TESTS
    check_PROGRAMS += test/network
    TESTS += test/network
endif
