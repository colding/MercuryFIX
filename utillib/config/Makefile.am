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

noinst_LTLIBRARIES = libconfig.la
libconfig_la_LDFLAGS = -static

libconfig_la_SOURCES = \
	config_item_network.cpp \
	config_item_network.h \
	config_item_simple.cpp \
	config_item_simple.h \
	config_item_string_vector.cpp \
	config_item_string_vector.h \
	config_item_fix_session.cpp \
	config_item_fix_session.h \
	config_item_app_purpose.cpp \
	config_item_app_purpose.h

AM_CPPFLAGS = $(MERCURY_CPPFLAGS)
AM_CXXFLAGS = $(MERCURY_CXXFLAGS)

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = $(BUILT_SOURCES) $(CLEAN_IN_FILES) Makefile
CLEANFILES = *~
