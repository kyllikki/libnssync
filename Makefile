#
# Makefile for libnssync
#
# Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
#
# This file is part of libnssync
# Released under the MIT License (see COPYING file)

# Component settings
COMPONENT := nssync
COMPONENT_VERSION := 0.0.1
# Default to a static library
COMPONENT_TYPE ?= lib-static

# Setup the tooling
PREFIX ?= /opt/netsurf
NSSHARED ?= $(PREFIX)/share/netsurf-buildsystem
include $(NSSHARED)/makefiles/Makefile.tools

TESTRUNNER := $(PERL) $(NSTESTTOOLS)/testrunner.pl

# Toolchain flags
WARNFLAGS := -Wall -Wextra -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs -Werror -Wno-unused-parameter

CFLAGS := -g -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L \
	-I$(CURDIR)/include/ -I$(CURDIR)/src $(WARNFLAGS) $(CFLAGS) -Wno-error

# openssl jansson libcurl
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
  ifneq ($(PKGCONFIG),)
    CFLAGS := $(CFLAGS) $(shell $(PKGCONFIG) openssl jansson libcurl --cflags)
    LDFLAGS := $(LDFLAGS) $(shell $(PKGCONFIG) openssl jansson libcurl --libs)
  else
    CFLAGS := $(CFLAGS) -I$(PREFIX)/include
    LDFLAGS := $(LDFLAGS) -lopenssl -ljansson -lcurl
  endif
endif

include $(NSBUILD)/Makefile.top

# Extra installation rules
I := /include
INSTALL_ITEMS := $(INSTALL_ITEMS) $(I):include/libnsfb.h
INSTALL_ITEMS := $(INSTALL_ITEMS) /lib/pkgconfig:lib$(COMPONENT).pc.in
INSTALL_ITEMS := $(INSTALL_ITEMS) /lib:$(OUTPUT)
