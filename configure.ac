# Part of measurement-kit <https://measurement-kit.github.io/>.
# Measurement-kit is free software. See AUTHORS and LICENSE for more
# information on the copying conditions.

# Autoconf requirements
AC_INIT(mkok-libevent-ng, 0.1.0, bassosimone@gmail.com)

echo "==== environment variables ==="
echo "CXX           : $CXX"
echo "CPPFLAGS      : $CPPFLAGS"
echo "CXXFLAGS      : $CXXFLAGS"
echo "LDFLAGS       : $LDFLAGS"
echo "LIBS          : $LIBS"
echo ""

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
LT_INIT
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_HOST

# checks for programs
AC_PROG_INSTALL
AC_PROG_CXX()

AC_ARG_ENABLE([examples],
    AS_HELP_STRING([--disable-examples, skip building of examples programs]),
        [], [enable_examples=yes])
AM_CONDITIONAL([BUILD_EXAMPLES], [test "$enable_examples" = "yes"])

AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--disable-tests, skip building and running test programs]),
        [], [enable_tests=yes])
AM_CONDITIONAL([BUILD_TESTS], [test "$enable_tests" = "yes"])

AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage, build for coverage]),
        [enable_coverage=yes], [])

AC_ARG_ENABLE([network-tests],
    AS_HELP_STRING(
      [--disable-network-tests, do not build tests requiring network]),
        [], [enable_network_tests=yes])
AM_CONDITIONAL([BUILD_NETWORK_TESTS], [test "$enable_network_tests" = "yes"])

# checks for libraries

AC_ARG_WITH([libevent],
    AS_HELP_STRING([--with-libevent, event I/O library]), [
        CPPFLAGS="$CPPFLAGS -I$withval/include"
        LDFLAGS="$LDFLAGS -L$withval/lib"], [])
AC_CHECK_HEADERS(event2/event.h, [], [AC_MSG_ERROR([libevent not found])])
AC_CHECK_LIB(event, event_new, [], [AC_MSG_ERROR([libevent not found])])
AC_CHECK_LIB(event_openssl, bufferevent_openssl_filter_new, [],
    [AC_MSG_ERROR([libevent not found])])

AC_ARG_WITH([openssl],
    AS_HELP_STRING([--with-openssl, OpenSSL-compatible library]), [
        CPPFLAGS="$CPPFLAGS -I$withval/include"
        LDFLAGS="$LDFLAGS -L$withval/lib"], [])
AC_CHECK_HEADERS(openssl/ssl.h, [], [AC_MSG_ERROR([OpenSSL not found])])
AC_CHECK_LIB(ssl, SSL_new, [], [AC_MSG_ERROR([OpenSSL not found])])
AC_CHECK_LIB(crypto, RSA_new, [], [AC_MSG_ERROR([OpenSSL not found])])

# checks for header files
# checks for types
# checks for structures

# checks for compiler characteristics

saved_cxxflags="$CXXFLAGS"
CXXFLAGS=-std=c++11
AC_MSG_CHECKING([whether CXX supports -std=c++11])
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([])], [AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])
     AC_MSG_ERROR([a C++11 compiler is required])])
CXXFLAGS="$saved_cxxflags -std=c++11"
AC_LANG_POP([C++])

BASE_CXXFLAGS="-Wall"
AC_MSG_CHECKING([whether the C++ compiler is clang++])
if test echo | $CXX -dM -E - | grep __clang__ > /dev/null; then
    AC_MSG_RESULT([yes])
    CXXFLAGS="$CXXFLAGS $BASE_CXXFLAGS -Wmissing-prototypes -Wextra"
else
    CXXFLAGS="$CXXFLAGS $BASE_CXXFLAGS"
    AC_MSG_RESULT([yes])
fi

CPPFLAGS="$CPPFLAGS -I \$(top_srcdir)/include"

# checks for library functions

# checks for system services

# done

if test "$enable_coverage" = "yes"; then
    CFLAGS="$CFLAGS --coverage"
    CXXFLAGS="$CXXFLAGS --coverage"
    LDFLAGS="$LDFLAGS --coverage"
fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

echo "==== enabled flags ==="
echo "coverage      : $enable_coverage"
echo "examples      : $enable_examples"
echo "network_tests : $enable_network_tests"
echo "tests         : $enable_tests"
echo ""

echo "==== configured variables ==="
echo "CXX           : $CXX"
echo "CPPFLAGS      : $CPPFLAGS"
echo "CXXFLAGS      : $CXXFLAGS"
echo "LDFLAGS       : $LDFLAGS"
echo "LIBS          : $LIBS"
echo ""
