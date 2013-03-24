#  
# Copyright (C) 2013 by Jules Colding <jcolding@gmail.com>.
#
# All Rights Reserved.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
#

SUBDIRS = 

noinst_LTLIBRARIES = libfixio.la
libfixio_la_LDFLAGS = -static

libfixio_la_SOURCES = \
	fixio.h \
	fix_pusher.cpp \
	fix_popper.cpp

AM_CPPFLAGS = $(MERCURY_CPPFLAGS)
AM_CXXFLAGS = $(MERCURY_CXXFLAGS)

#########################
# Unit tests using Check
#########################

TESTS = check_fixio
noinst_PROGRAMS = check_fixio
check_fixio_LDFLAGS = -all-static
check_fixio_SOURCES = check_fixio.cpp
check_fixio_LDADD = $(CHECK_LIBS) libfixio.la
check_fixio_CPPFLAGS = $(MERCURY_CPPFLAGS) $(CHECK_CFLAGS)
check_fixio_CXXFLAGS = $(MERCURY_CXXFLAGS) $(CHECK_CFLAGS)

##################################
# What to clean boiler plate code
##################################

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = $(BUILT_SOURCES) $(CLEAN_IN_FILES) Makefile
CLEANFILES = *~ *.trs

