#
#
#

CFLAGS_LIBJANSSON:=$(shell pkg-config --cflags openssl jansson libcurl)
LDFLAGS_LIBJANSSON:=$(shell pkg-config --libs openssl jansson libcurl)

CFLAGS += $(CFLAGS_LIBJANSSON) -Wall -g -O2
LDFLAGS += $(LDFLAGS_LIBJANSSON)

.PHONY:all

all:sha1base32 syncstorage

sha1base32:base32.o sha1base32.o #sha1.o

syncstorage:syncstorage.o

clean:
	${RM} syncstorage.o syncstorage sha1base32 sha1.o base32.o sha1base32.o