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
AC_CHECK_LIB([m], [ceil], [], [missing_libraries="$missing_libraries libm"])

AC_ARG_ENABLE(ncurses, [AC_HELP_STRING([--enable-ncurses], [use ncurses (switch to low-level hacks if disabled)])],, enable_ncurses="yes")
extra_headers=
if test "x$enable_ncurses" = xyes
then
   AC_CHECK_LIB([ncurses], [refresh], [], [missing_libraries="$missing_libraries libncurses"])
   extra_headers=curses.h
fi

if test ! -z "$missing_libraries"; then
  AC_MSG_ERROR([missing libraries: $missing_libraries])
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/time.h unistd.h $extra_headers],[:],[
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

DIT_LUA_VERSION=5.1.5

AC_ARG_ENABLE(bundled-lua, [AC_HELP_STRING([--enable-bundled-lua], [compile bundled copy of Lua])],, enable_bundled_lua="no")

if test x"$enable_bundled_lua" = x"no"
then
   AX_LUA_HEADERS
   AX_LUA_LIBS
   AX_LUA_TRY_LIB_VERSION([501], [502], [], [enable_bundled_lua="yes"])
fi

if test x"$enable_bundled_lua" = x"yes"
then
   AC_MSG_RESULT([Using bundled Lua])
   lua_extra_libs=
   AC_CHECK_LIB([m], [exp], [lua_extra_libs="$lua_extra_libs -lm"], [])
   AC_CHECK_LIB([dl], [dlopen], [lua_extra_libs="$lua_extra_libs -ldl"], [])
   LUA_INCLUDE=-Ilua-${DIT_LUA_VERSION}/src
   LUA_LIB="lua-${DIT_LUA_VERSION}/src/liblua.a $lua_extra_libs"
   LUA_DEP="lua-${DIT_LUA_VERSION}/src/liblua.a $lua_extra_libs"
fi

AC_SUBST([LUA_INCLUDE])
AC_SUBST([LUA_LIB])
AC_SUBST([LUA_DEP])
AC_SUBST([DIT_LUA_VERSION])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
