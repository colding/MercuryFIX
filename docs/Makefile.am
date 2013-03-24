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
#

daemondocdir=$(prefix)/share/doc/$(DAEMOND_NAME)-$(DAEMOND_VERSION)
daemondocdeveldir=$(prefix)/share/doc/$(DAEMOND_NAME)-devel-$(DAEMOND_VERSION)

daemondoc_DATA = \
	../README            \
	../COPYING           \
	../AUTHORS           \
	../ChangeLog

daemondocdevel_DATA = 

if THIS_IS_NOT_A_DISTRIBUTION
CLEAN_IN_FILES = Makefile.in
else
CLEAN_IN_FILES =
endif

DISTCLEANFILES = Makefile $(CLEAN_IN_FILES)

