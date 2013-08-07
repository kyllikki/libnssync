#
# Makefile for libcss
#
# Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
#
# Released under the MIT License (see COPYING file)

# Sources
DIR_SOURCES := base32.c base64.c hex16.c request.c storage.c auth.c sync.c crypto.c

include $(NSBUILD)/Makefile.subdir

#
#
#

#CFLAGS_LIBJANSSON:=$(shell pkg-config --cflags openssl jansson libcurl)
#LDFLAGS_LIBJANSSON:=$(shell pkg-config --libs openssl jansson libcurl)

#CFLAGS += $(CFLAGS_LIBJANSSON) -Wall -g -O2
#LDFLAGS += $(LDFLAGS_LIBJANSSON)

#LIBNSSYNC_OBJ=base32.o base64.o hex.o request.o storage.o auth.o sync.o crypto.o

#.PHONY:all

#all:sha1base32 syncstorage synckey

#sha1base32:base32.o sha1base32.o #sha1.o

#synckey:synckey.o $(LIBNSSYNC_OBJ)

#syncstorage:syncstorage.o $(LIBNSSYNC_OBJ)

#clean:
#	${RM} syncstorage.o syncstorage sha1base32 sha1.o synckey synckey.o 
#	${RM} $(LIBNSSYNC_OBJ)
