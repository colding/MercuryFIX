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

#########################
# Unit tests using Check
#########################

TESTS = check_fixio
noinst_PROGRAMS = check_fixio
check_fixio_LDFLAGS = -all-static
check_fixio_SOURCES = \
	check_fixio.cpp \
	../fixio/fixio.h \
	../fixio/db_utils.h \
	../fixio/db_utils.cpp

check_fixio_CPPFLAGS = $(CHECK_CFLAGS)
check_fixio_LDADD = \
	$(CHECK_LIBS) \
	$(MERCURY_top_dir)/applib/fixio/libfixio.la \
	$(MERCURY_top_dir)/applib/fixmsg/libfixmsg.la \
	$(MERCURY_top_dir)/stdlib/log/liblog.la \
	$(MERCURY_top_dir)/stdlib/process/libprocess.la \
	$(MERCURY_top_dir)/stdlib/network/libnetwork.la \
	$(MERCURY_top_dir)/stdlib/local_db/liblocaldb.la

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

