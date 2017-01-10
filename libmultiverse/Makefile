CC ?= gcc
CFLAGS = -gdwarf-2 -Wall -Wextra
SOURCES=$(shell echo *.c)

ALLPRODUCTS+=libmultiverse.a
all: libmultiverse.a

include ../common.mk

libmultiverse.a: $(OBJECTS)
	ar rcs $@ $^
