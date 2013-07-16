#
#
#

CFLAGS_LIBJANSSON:=$(shell pkg-config --cflags jansson libcurl)
LDFLAGS_LIBJANSSON:=$(shell pkg-config --libs jansson libcurl)

.PHONY:all

all:sha1base32 syncstorage

sha1base32:sha1.o base32.o sha1base32.o


CFLAGS += $(CFLAGS_LIBJANSSON)
LDFLAGS += $(LDFLAGS_LIBJANSSON)

syncstorage:syncstorage.o

clean:
	${RM} syncstorage.o syncstorage sha1base32 sha1.o base32.o sha1base32.o
