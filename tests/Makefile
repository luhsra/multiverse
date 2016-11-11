CC=gcc
PLUGIN_DIR=../gcc-plugin
PLUGIN=$(PLUGIN_DIR)/multiverse.so

LIBRARY_DIR=../libmultiverse
LIBRARY=$(LIBRARY_DIR)/libmultiverse.a

CFLAGS = -fplugin=$(PLUGIN) -I$(LIBRARY_DIR) -fdump-tree-all
LDFLAGS = -L$(LIBRARY_DIR) -lmultiverse

TEST_SRCS=$(shell echo *.c)
TEST_OBJS=$(foreach x,${TEST_SRCS},$(patsubst %.c,%.o,$x))
TEST_ASM=$(foreach x,${TEST_SRCS},$(patsubst %.c,%.S,$x))
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
	$(CC)  -c -o $@ $(CFLAGS)-gdwarf-2  $<

%.S: %.c $(PLUGIN) testsuite.h
	$(CC)  -S -o $@ $(CFLAGS) $<

clean:
	rm -f $(TEST_OBJS) $(TEST_ASM) $(TESTS) core
	find -regex ".*\\.c\\.[0-9]*t\\..*" | xargs rm -f

test: $(foreach x,${TEST_SRCS},$(patsubst %.c,test-%,$x))

test-%: %
	./$<

.PHONY: always
