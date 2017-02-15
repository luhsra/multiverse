CC ?= gcc

PLUGIN_DIR=../../gcc-plugin
PLUGIN=$(PLUGIN_DIR)/multiverse.so
LIBRARY_DIR=../../libmultiverse
LIBRARY=$(LIBRARY_DIR)/libmultiverse.a
EXTRA_DEPS=$(LIBRARY) $(PLUGIN)

CFLAGS  = -fplugin=$(PLUGIN) -I$(LIBRARY_DIR) -O2 -Wextra -I.. -g
LDFLAGS = -L$(LIBRARY_DIR)
LDLIBS  = -lmultiverse -lpthread

SOURCES=$(shell echo *.c)
TESTS=$(foreach x,${SOURCES},$(patsubst %.c,%,$x))

all: main

main: module
	$(CC) -c $@.c $(CFLAGS)
	$(CC) $(LDFLAGS) -o $@ $@.o $^.o $(LDLIBS)

module: module.h
	$(CC) -c $@.c $(CFLAGS)

clean:
	rm -f *.o main

.PHONY: always
