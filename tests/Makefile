CC=gcc
PLUGIN_DIR=../gcc-plugin
PLUGIN=$(PLUGIN_DIR)/multiverse.so

LIBRARY_DIR=../libmultiverse
LIBRARY=$(LIBRARY_DIR)/libmultiverse.a

CFLAGS = -fplugin=$(PLUGIN) -I$(LIBRARY_DIR) -gdwarf-2 -O2
LDFLAGS = -L$(LIBRARY_DIR) -lmultiverse

TEST_SRCS=$(shell echo *.c)
TEST_OBJS=$(foreach x,${TEST_SRCS},$(patsubst %.c,%.o,$x))
TESTS=$(foreach x,${TEST_SRCS},$(patsubst %.c,%,$x))

all: $(TEST_OBJS) $(TESTS)

$(LIBRARY): always
	make -C $(LIBRARY_DIR)

$(PLUGIN): always
	make -C $(PLUGIN_DIR)

# Rule for building the binaries
%: %.o $(LIBRARY)
	$(CC)  -o $@ $< $(LDFLAGS)

%.o: %.c $(PLUGIN) testsuite.h
	$(CC)  -c -o $@ $(CFLAGS) $<

clean:
	rm -f $(TEST_OBJS) $(TESTS)

test: $(foreach x,${TEST_SRCS},$(patsubst %.c,test-%,$x))

test-%: %
	./$<

.PHONY: always
