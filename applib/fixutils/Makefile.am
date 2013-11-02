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

noinst_LTLIBRARIES = libfixutils.la
libfixutils_la_LDFLAGS = -static

libfixutils_la_SOURCES = \
	db_utils.h \
	db_utils.cpp \
	fixmsg_utils.h \
	fixmsg_utils.cpp \
	stack_utils.h

libfixutils_la_CPPFLAGS = $(MERCURY_CPPFLAGS)
libfixutils_la_CXXFLAGS = $(MERCURY_CXXFLAGS)

#########################
# Unit tests using Check
#########################

TESTS = check_fixmsg_utils
noinst_PROGRAMS = check_fixmsg_utils
check_fixmsg_utils_LDFLAGS = -all-static
check_fixmsg_utils_SOURCES = \
	check_fixmsg_utils.cpp \
	fixmsg_utils.h

check_fixmsg_utils_CPPFLAGS = $(CHECK_CFLAGS) $(MERCURY_CXXFLAGS)
check_fixmsg_utils_LDADD = \
	$(CHECK_LIBS) \
	$(MERCURY_top_dir)/applib/fixutils/libfixutils.la \
	$(MERCURY_top_dir)/stdlib/log/liblog.la

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

