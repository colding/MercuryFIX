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
	db_utils.h \
	db_utils.cpp \
	stack_utils.h \
	fix_pusher.cpp \
	fix_popper.cpp

libfixio_la_CPPFLAGS = $(MERCURY_CPPFLAGS)
libfixio_la_CXXFLAGS = $(MERCURY_CXXFLAGS)

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

