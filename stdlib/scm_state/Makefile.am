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

noinst_LTLIBRARIES = libscmstate.la

if OS_DARWIN

libscmstate_la_LDFLAGS = -static

else
if OS_LINUX

libscmstate_la_LDFLAGS = -static -r -b binary -o $(MERCURY_SCM_STATE_FILE).o $(MERCURY_SCM_STATE_FILE) 
libscmstate_la_LIBADD = $(MERCURY_SCM_STATE_FILE).o

scm_state.o: $(MERCURY_SCM_STATE_FILE).o

.PHONY: $(MERCURY_SCM_STATE_FILE).o

$(MERCURY_SCM_STATE_FILE).o:
	git status    > $(MERCURY_SCM_STATE_FILE)
	echo ""      >> $(MERCURY_SCM_STATE_FILE)
	git log -n 1 >> $(MERCURY_SCM_STATE_FILE)
	echo ""      >> $(MERCURY_SCM_STATE_FILE)
	git diff -p  >> $(MERCURY_SCM_STATE_FILE)
	$(LD) -r -b binary -o $(MERCURY_SCM_STATE_FILE).o $(MERCURY_SCM_STATE_FILE) 
endif
endif

libscmstate_la_SOURCES = \
	scm_state.cpp \
	scm_state.h

AM_CPPFLAGS = $(MERCURY_CPPFLAGS)
AM_CXXFLAGS = $(MERCURY_CXXFLAGS)

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = $(BUILT_SOURCES) $(CLEAN_IN_FILES) Makefile
CLEANFILES = *~ $(MERCURY_SCM_STATE_FILE).o
