#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
#	avarice - The "avarice" program.
#	Copyright (C) 2001 Scott Finneran
#      Copyright (C) 2002, 2003 Intel Corporation
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License Version 2
#       as published by the Free Software Foundation.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
#
# $Id$
#

AC_PREREQ(2.57)
AC_INIT(avarice, 2.0.20031113cvs)
AC_CONFIG_AUX_DIR([config-aux])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([src/autoconf.h:src/autoconf.hin])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_PATH_PROG(pathperl, perl)
if test -z "$pathperl" ; then
  AC_MSG_ERROR([I can't find perl]); 
fi

# Checks for libraries.

## Some systems need "-lsocket -lnsl" when linking.
##
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_CHECK_FUNC(socket, , [
    AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket", [
        AC_CHECK_LIB(nsl, socket, LIBS="$LIBS -lsocket -lnsl", , -lsocket)
    ], "$LIBS")
])

## Some systems need "-lresolv" for inet_aton().
##
AC_SEARCH_LIBS([inet_aton], [resolv])

## If libbfd was configured with nls, the build might need -lintl. This
## seems to be the case with cygwin.
##
AC_SEARCH_LIBS([dcgettext], [intl])

AC_CHECK_LIB([iberty], [xmalloc], ,[ac_found_iberty=no])
AC_CHECK_LIB([bfd], [bfd_init], , [ac_found_bfd=no], [-liberty])

if test "x$ac_found_bfd" = "xno"; then
  AC_MSG_ERROR([You need to install libbfd.a from binutils.])
fi

if test "x$ac_found_iberty" = "xno"; then
  AC_MSG_ERROR([You need to install libiberty.a from binutils.])
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h termios.h unistd.h bfd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/type_socklen_t.html
dnl
AC_DEFUN([TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t], ac_cv_type_socklen_t,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
   #include <sys/socket.h>],
  [socklen_t len = 42; return 0;],
  ac_cv_type_socklen_t=yes,
  ac_cv_type_socklen_t=no)
])
  if test $ac_cv_type_socklen_t != yes; then
    AC_DEFINE([socklen_t], [int], [Substitute for missing socklen_t.])
  fi
])
TYPE_SOCKLEN_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gethostbyname inet_ntoa memmove memset select socket strcasecmp strerror strtol])

AC_CONFIG_FILES([
	scripts/Makefile
	scripts/ice-gdb
	src/Makefile
	doc/Makefile
	avarice.spec
	Makefile])
AC_OUTPUT
