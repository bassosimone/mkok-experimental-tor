dnl Part of measurement-kit <https://measurement-kit.github.io/>.
dnl Measurement-kit is free software. See AUTHORS and LICENSE for more
dnl information on the copying conditions.

dnl Common macros for mkok projects.

dnl Macro that defines on the --disable-example flag
AC_DEFUN([MKPM_ARG_DISABLE_EXAMPLES], [
  AC_ARG_ENABLE([examples],
    AS_HELP_STRING([--disable-examples, skip building of examples programs]),
      [], [enable_examples=yes])
  AM_CONDITIONAL([BUILD_EXAMPLES], [test "$enable_examples" = "yes"])
])

dnl Macro that defines on the --enable-tests flag
AC_DEFUN([MKPM_ARG_ENABLE_TESTS], [
  AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--disable-tests, skip building and running test programs]),
      [], [enable_tests=yes])
  AM_CONDITIONAL([BUILD_TESTS], [test "$enable_tests" = "yes"])
])

dnl Macro that defines the --enable-coverage flag
AC_DEFUN([MKPM_ARG_ENABLE_COVERAGE], [
  AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage, build for coverage]),
      [enable_coverage=yes], [])
])

dnl Require libevent and add --with-libevent flag
AC_DEFUN([MKPM_REQUIRE_LIBEVENT], [
  AC_ARG_WITH([libevent],
    AS_HELP_STRING([--with-libevent, event I/O library]), [
      CPPFLAGS="$CPPFLAGS -I$withval/include"
      LDFLAGS="$LDFLAGS -L$withval/lib"], [])
  AC_CHECK_HEADERS(event2/event.h, [], [AC_MSG_ERROR([libevent not found])])
  AC_CHECK_LIB(event, event_new, [], [AC_MSG_ERROR([libevent not found])])
  AC_CHECK_LIB(event_openssl, bufferevent_openssl_filter_new, [],
    [AC_MSG_ERROR([libevent not found])])
])

dnl Require OpenSSL (or LibReSSL) and add --with-openssl flag
AC_DEFUN([MKPM_REQUIRE_OPENSSL], [
  AC_ARG_WITH([openssl],
    AS_HELP_STRING([--with-openssl, OpenSSL-compatible library]), [
      CPPFLAGS="$CPPFLAGS -I$withval/include"
      LDFLAGS="$LDFLAGS -L$withval/lib"], [])
  AC_CHECK_HEADERS(openssl/ssl.h, [], [AC_MSG_ERROR([OpenSSL not found])])
  AC_CHECK_LIB(crypto, RSA_new, [], [AC_MSG_ERROR([OpenSSL not found])])
  AC_CHECK_LIB(ssl, SSL_new, [], [AC_MSG_ERROR([OpenSSL not found])])
])

dnl Make sure that the compiler supports C++11
AC_DEFUN([MKPM_REQUIRE_CXX11], [
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
])

dnl Add as much warnings as possible to CXXFLAGS
AC_DEFUN([MKPM_ADD_WARNINGS_TO_CXXFLAGS], [
  BASE_CXXFLAGS="-Wall"
  AC_MSG_CHECKING([whether the C++ compiler is clang++])
  if test echo | $CXX -dM -E - | grep __clang__ > /dev/null; then
    AC_MSG_RESULT([yes])
    CXXFLAGS="$CXXFLAGS $BASE_CXXFLAGS -Wmissing-prototypes -Wextra"
  else
    CXXFLAGS="$CXXFLAGS $BASE_CXXFLAGS"
    AC_MSG_RESULT([yes])
  fi
])

dnl Add $(top_srcdir)/include to CPPFLAGS
AC_DEFUN([MKPM_ADD_INCLUDE_TO_CPPFLAGS], [
  CPPFLAGS="$CPPFLAGS -I \$(top_srcdir)/include"
])

dnl If needed, add flags required to enable coverage
AC_DEFUN([MKPM_ADD_COVERAGE_FLAGS_IF_NEEDED], [
  if test "$enable_coverage" = "yes"; then
    CFLAGS="$CFLAGS --coverage"
    CXXFLAGS="$CXXFLAGS --coverage"
    LDFLAGS="$LDFLAGS --coverage"
  fi
])

dnl Print what has been configured by ./configure
AC_DEFUN([MKPM_PRINT_SUMMARY], [
  echo "==== configured flags ===="
  echo "CPPFLAGS      : $CPPFLAGS"
  echo "CXX           : $CXX"
  echo "CXXFLAGS      : $CXXFLAGS"
  echo "LDFLAGS       : $LDFLAGS"
  echo "LIBS          : $LIBS"
  echo "coverage      : $enable_coverage"
  echo "examples      : $enable_examples"
  echo "network_tests : $enable_network_tests"
  echo "tests         : $enable_tests"
])
