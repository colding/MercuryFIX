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

noinst_LTLIBRARIES = libfixmsg.la
libfixmsg_la_LDFLAGS = -static

libfixmsg_la_SOURCES = \
	fix_types.cpp \
	fix_types.h \
	fixmsg_rx.cpp \
	fixmsg_tx.cpp \
	fixmsg.h \
	FIX40.cpp \
	FIX41.cpp \
	FIX42.cpp \
	FIX43.cpp \
	FIX44.cpp \
	FIX50.cpp \
	FIX50SP1.cpp \
	FIX50SP2.cpp \
	FIXT11.cpp \
	fixmsg_utils.cpp

libfixmsg_la_CPPFLAGS = $(MERCURY_CPPFLAGS)
libfixmsg_la_CXXFLAGS = $(MERCURY_CXXFLAGS)

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

