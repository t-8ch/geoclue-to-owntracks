AC_INIT(geoclue-to-owntracks, 1.0)
AM_INIT_AUTOMAKE([1.11 silent-rules foreign])
LT_INIT

AC_PROG_CC

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_CHECK_HEADERS([mosquitto.h])
AC_SEARCH_LIBS([mosquitto_lib_init],[mosquitto])

PKG_CHECK_MODULES([JSON_C], [json-c])
PKG_CHECK_MODULES([GEOCLUE2], [libgeoclue-2.0])

AC_OUTPUT
