#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(dit, 0.5, hisham@gobolinux.org)
AC_CONFIG_SRCDIR([dit.c])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [ceil], [], [missing_libraries="$missing_libraries libm"])

AC_ARG_ENABLE(ncurses, [AC_HELP_STRING([--enable-ncurses], [use ncurses (switch to low-level hacks if disabled)])],, enable_ncurses="yes")

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC

if test "`uname`" = "OpenBSD"
then
   CPPFLAGS="$CPPFLAGS -I/usr/local/include"
   LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi

AC_ARG_ENABLE(unicode, [AC_HELP_STRING([--enable-unicode], [enable Unicode support])], ,enable_unicode="yes")
if test "x$enable_unicode" = xyes; then
   AC_CHECK_LIB([ncursesw6], [refresh], [
      AC_DEFINE(HAVE_LIBNCURSESW, 1, [])
      LIBS="-lncursesw6 $LIBS "
   ], [
      AC_CHECK_LIB([ncursesw], [refresh], [], [
        AC_CHECK_LIB([ncurses], [addnwstr], [
          AC_DEFINE(HAVE_LIBNCURSESW, 1, [])
          LIBS="-lncurses $LIBS "
      ], [ 
         missing_libraries="$missing_libraries libncursesw"
         AC_MSG_ERROR([You may want to use --disable-unicode or install libncursesw.])
      ])])
   ])
   AC_CHECK_HEADERS([ncursesw/curses.h],[:],
      [AC_CHECK_HEADERS([ncurses/ncurses.h],[:],
         [AC_CHECK_HEADERS([ncurses/curses.h],[:],
            [AC_CHECK_HEADERS([ncurses.h],[:],[missing_headers="$missing_headers $ac_header"])])])])
   AC_CHECK_LIB([iconv], [iconv], [], [
     AC_CHECK_LIB([iconv], [libiconv], [], [
        AC_CHECK_LIB([c], [iconv], [], [
          missing_libraries="$missing_libraries libiconv"
          AC_MSG_ERROR([You may want to use --disable-unicode or install libiconv.])
        ])
     ])
   ])
else
   AC_CHECK_LIB([ncurses], [refresh], [], [missing_libraries="$missing_libraries libncurses"])
   AC_CHECK_HEADERS([curses.h],[:],
      [AC_CHECK_HEADERS([ncurses/curses.h],[:],
         [AC_CHECK_HEADERS([ncurses/ncurses.h],[:],
            [AC_CHECK_HEADERS([ncurses.h],[:],[missing_headers="$missing_headers $ac_header"])])])])
fi

case "$target_os" in
darwin*)
   : # no support for clock_gettime, internal workaround implemented
   ;;
*)
   AC_CHECK_LIB([c], [clock_gettime], [], [
     AC_CHECK_LIB([rt], [clock_gettime], [], [
        missing_libraries="$missing_libraries librt"
     ])
   ])
   ;;
esac

if test ! -z "$missing_libraries"; then
  AC_MSG_ERROR([missing libraries: $missing_libraries])
fi

AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/time.h unistd.h],[:],[
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

DIT_LUA_VERSION=5.3.0

AC_ARG_ENABLE(bundled-lua, [AC_HELP_STRING([--enable-bundled-lua], [compile bundled copy of Lua])],, enable_bundled_lua="default")

if test x"$enable_bundled_lua" = x"no"
then
   AX_LUA_LIBS
   if test x"$LUA_LIB" = x""
   then
      AC_MSG_ERROR([Could not find Lua libraries in the system.])
   else
      AX_LUA_HEADERS
      AX_LUA_TRY_LIB_VERSION([501], [504], [], [
         AC_MSG_ERROR([Could not find a suitable version of Lua.])
      ])
   fi
elif test x"$enable_bundled_lua" = x"default"
then
   AX_LUA_LIBS
   if test x"$LUA_LIB" = x""
   then
      enable_bundled_lua=yes
   else
      AX_LUA_HEADERS
      AX_LUA_TRY_LIB_VERSION([501], [504], [], [enable_bundled_lua="yes"])
   fi
fi

if test x"$enable_bundled_lua" = x"yes"
then
   AC_MSG_RESULT([Using bundled Lua])
   lua_extra_libs=
   AC_CHECK_LIB([m], [exp], [lua_extra_libs="$lua_extra_libs -lm"], [])
   AC_CHECK_LIB([dl], [dlopen], [lua_extra_libs="$lua_extra_libs -ldl"], [])
   LUA_INCLUDE=-Ilua-${DIT_LUA_VERSION}/src
   LUA_LIB="lua-${DIT_LUA_VERSION}/src/liblua.a $lua_extra_libs"
   LUA_DEP="lua-${DIT_LUA_VERSION}/src/liblua.a"
fi

AC_SUBST([LUA_INCLUDE])
AC_SUBST([LUA_LIB])
AC_SUBST([LUA_DEP])
AC_SUBST([DIT_LUA_VERSION])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
