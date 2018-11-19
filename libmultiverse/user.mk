PREFIX ?= /usr/local
MY_CC ?= gcc
CC = $(MY_CC)
CFLAGS = -gdwarf-2 -Wall -Wextra -O2 -std=c99 -fno-common
ALLPRODUCTS+=libmultiverse.a

ifneq ($(DESTDIR),)
	DESTDIR := ../$(DESTDIR)
endif

all: libmultiverse.a

include ../common.mk

libmultiverse.a: $(OBJECTS)
	ar rcs $@ $^

.PHONY: install
install: libmultiverse.a multiverse.h
	install -d $(DESTDIR)$(PREFIX)/lib
	install libmultiverse.a $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	install multiverse.h $(DESTDIR)$(PREFIX)/include

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libmultiverse.a
	rm -f $(DESTDIR)$(PREFIX)/include/multiverse.h
