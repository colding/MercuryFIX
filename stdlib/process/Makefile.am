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

noinst_LTLIBRARIES = libprocess.la
libprocess_la_LDFLAGS = -static

libprocess_la_SOURCES = \
	cpu.h \
	daemon.cpp \
	daemon.h \
	threads.cpp \
	threads.h \
	id.cpp \
	id.h \
	refcnt.h

AM_CPPFLAGS = $(MERCURY_CPPFLAGS)
AM_CXXFLAGS = $(MERCURY_CXXFLAGS)

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = $(BUILT_SOURCES) $(CLEAN_IN_FILES) Makefile
CLEANFILES = *~
