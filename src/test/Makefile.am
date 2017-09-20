#
# Test makefile
#

noinst_PROGRAMS=yambler_test

yambler_test_CFLAGS=-I$(top_srcdir)/src/libyambler
yambler_test_LDADD=../libyambler/libyambler.a
yambler_test_SOURCES=test.c main.c

SUBDIRS=yambler_test.tests

AUTOMAKE_OPTIONS=dejagnu

EXTRA_DIST=global-conf.exp

DEJATOOL=yambler_test
