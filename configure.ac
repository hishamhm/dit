#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(dit, 0.1, loderunner@users.sf.net)
AC_CONFIG_SRCDIR([dit.c])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([ncurses], [refresh], [], [missing_libraries="$missing_libraries libncurses"])
AC_CHECK_LIB([m], [ceil], [], [missing_libraries="$missing_libraries libm"])

if test ! -z "$missing_libraries"; then
  AC_MSG_ERROR([missing libraries: $missing_libraries])
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/time.h unistd.h curses.h],[:],[
  missing_headers="$missing_headers $ac_header"
])

if test ! -z "$missing_headers"; then
  AC_MSG_ERROR([missing headers: $missing_headers])
fi


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([gettimeofday memchr memmove memset mkdir putenv realpath regcomp strchr strdup strncasecmp strrchr strstr])

AX_LUA_HEADERS
AX_LUA_LIBS
DIT_LUA_VERSION=5.1.5
AX_LUA_TRY_LIB_VERSION([501], [502], [], [
   AC_MSG_RESULT([Using bundled Lua])
   LUA_INCLUDE=-Ilua-${DIT_LUA_VERSION}/src
   LUA_LIB=lua-${DIT_LUA_VERSION}/src/liblua.a
   LUA_DEP=lua-${DIT_LUA_VERSION}/src/liblua.a
])
AC_SUBST([LUA_INCLUDE])
AC_SUBST([LUA_LIB])
AC_SUBST([LUA_DEP])
AC_SUBST([DIT_LUA_VERSION])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
