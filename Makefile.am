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

CWD = $(CURDIR)
MKDIR=/bin/mkdir
TAR=/bin/tar

SUBDIRS = \
	stdlib  \
	utillib \
	omslib  \
	applib  \
	servers

ACLOCAL_AMFLAGS = -I m4

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = aclocal.m4 intltool-extract intltool-merge intltool-update iconv-detect.c Makefile *.tar.gz $(CLEAN_IN_FILES)

EXTRA_DIST = docs 

clean-local:
	rm -f state.scm
