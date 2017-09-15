#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([yambler], [1.0.0], [hansvm.projects@gmail.com])
AC_CONFIG_SRCDIR([src/yambler/yambler_parser.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

AC_SEARCH_LIBS([iconv_open],[iconv])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/yambler/Makefile
				 src/test/Makefile])
AC_OUTPUT
