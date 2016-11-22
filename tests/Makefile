CC=gcc
PLUGIN_DIR=../gcc-plugin
PLUGIN=$(PLUGIN_DIR)/multiverse.so

LIBRARY_DIR=../libmultiverse
LIBRARY=$(LIBRARY_DIR)/libmultiverse.a

CFLAGS = -fplugin=$(PLUGIN) -I$(LIBRARY_DIR) -O2 -fdump-tree-optimized -fdump-ipa-all -fdump-tree-all -fdump-rtl-final -Wextra
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
%: %.o $(LIBRARY) $(PLUGIN)
	$(CC)  -o $@ $< $(LDFLAGS)

%.o: %.c $(PLUGIN) $(LIBRARY) testsuite.h
	$(CC)  -c -o $@ $(CFLAGS) -gdwarf-2  $<

%.S: %.c $(PLUGIN) $(LIBRARY) testsuite.h
	$(CC)  -S -o $@ $(CFLAGS) $<

clean:
	rm -f $(TEST_OBJS) $(TEST_ASM) $(TESTS) core
	find -regex ".*\\.c\\.[0-9]*[tri]\\..*" | xargs rm -f
	$(MAKE) -C $(LIBRARY_DIR) clean
	$(MAKE) -C $(PLUGIN_DIR) clean


test: $(foreach x,${TEST_SRCS},$(patsubst %.c,test-%,$x))

test-%: %
	./$<

.PHONY: always
