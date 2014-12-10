#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
#	avarice - The "avarice" program.
#	Copyright (C) 2001 Scott Finneran
#	Copyright (C) 2002, 2003, 2004 Intel Corporation
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
AC_INIT(avarice, 2.13svn20141210)
AC_CONFIG_AUX_DIR([config-aux])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([src/autoconf.h:src/autoconf.hin])

dnl We don't want the gzip distribution tarball anymore.
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

m4_include([m4_ax_pthread.m4])
AX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS"
	   CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
           CC="$PTHREAD_CC"])

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
## seems to be the case with cygwin. Also, it seems that on cygwin, libintl
## needs libiconv. Plus, on some systems libbfd needs -liberty.
##
AC_CHECK_LIB([iconv], [iconv_open], , [ac_found_iconf=no])
if test "x$ac_found_iconf" = "xno"; then
  AC_CHECK_LIB([iconv], [libiconv_open])
fi
AC_CHECK_LIB([intl], [dcgettext])
AC_CHECK_LIB([iberty], [xmalloc])
AC_CHECK_LIB([bfd], [bfd_init], , [ac_found_bfd=no])
AC_CHECK_LIB([usb], [usb_get_string_simple])
AC_CHECK_LIB([usb], [libusb20_dev_open], [have_libusb_2_0=yes])
if test x$have_libusb_2_0 = xyes; then
   AC_DEFINE([HAVE_LIBUSB_2_0], [], [Define if USB support is enabled via FreeBSD's libusb20 API])
   AC_CHECK_HEADERS([libusb20.h])
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h termios.h unistd.h])

AC_CHECK_HEADERS([bfd.h], , [ac_found_bfd_h=no])

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
  AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
  [#include <sys/types.h>
   #include <sys/socket.h>],
  [socklen_t len = 42; return 0;])],
  ac_cv_type_socklen_t=yes,
  ac_cv_type_socklen_t=no)
])
  if test $ac_cv_type_socklen_t != yes; then
    AC_DEFINE([socklen_t], [int], [Substitute for missing socklen_t.])
  fi
])
TYPE_SOCKLEN_T

AC_MSG_CHECKING([whether libbfd requires libz])
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([#include <bfd.h>
  int main(void) {
    bfd_init();
    bfd_openr("foo", 0);
    return 42;
  }])], [AC_MSG_RESULT([no])],
  [AC_MSG_RESULT([yes])
   AC_CHECK_LIB([z], [inflate], [LIBS="$LIBS -lz"])])

AC_MSG_CHECKING([whether libbfd requires libdl])
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([#include <bfd.h>
  bfd *file;
  int main(void) {
    bfd_init();
    file = bfd_openr("foo", 0);
    bfd_get_section_name(file, file->sections);
    return 42;
  }])], [AC_MSG_RESULT([no])],
  [AC_MSG_RESULT([yes])
   AC_CHECK_LIB([dl], [dlopen], [LIBS="$LIBS -ldl"])])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gethostbyname inet_ntoa memmove memset select socket strcasecmp strerror strtol])

# --enable-warnings / --disable-warnings
AC_ARG_ENABLE(
	[warnings],
	AC_HELP_STRING(
		[--disable-warnings],
		[Disable -Wall -Wextra options]),
	[case "${enableval}" in
	      yes) ENABLE_WARNINGS="-Wall -Wextra" ;;
	      no)  ENABLE_WARNINGS="" ;;
	      *) AC_MSG_ERROR(bad value ${enableval} for disable-warnings option) ;;
	      esac],
	[ENABLE_WARNINGS="-Wall -Wextra"])

AC_SUBST([AM_CPPFLAGS], [$ENABLE_WARNINGS])

# --enable-target-programming / --disable-target-programming
AC_ARG_ENABLE(
	[target-programming],
	AC_HELP_STRING(
		[--enable-target-programming],
		[Enable programming (downloading) the target from ELF file]),
	[case "${enableval}" in
	      yes) ENABLE_TARGET_PROGRAMMING="-DENABLE_TARGET_PROGRAMMING=1" ;;
	      no)  ENABLE_TARGET_PROGRAMMING="-DENABLE_TARGET_PROGRAMMING=0" ;;
	      *) AC_MSG_ERROR(bad value ${enableval} for ensable-target-programming option) ;;
	      esac],
	[ENABLE_TARGET_PROGRAMMING="-DENABLE_TARGET_PROGRAMMING=0"])

AC_SUBST([AM_CPPFLAGS], [$ENABLE_TARGET_PROGRAMMING])

if test "x$enable_target_programming" = "xyes"; then
   if test "x$ac_found_bfd" = "xno"; then
      AC_MSG_ERROR([You need to install libbfd.a from binutils, or configure with --disable-target-programming.])
   fi

   if test "x$ac_found_bfd_h" = "xno"; then
      AC_MSG_ERROR([Your libbfd.a needs an accompanying bfd.h file, or configure with --disable-target-programming.])
   fi
fi

AC_CONFIG_FILES([
	scripts/Makefile
	scripts/ice-gdb
	src/Makefile
	doc/Makefile
	avarice.spec
	Makefile])
AC_OUTPUT
