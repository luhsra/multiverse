CC=gcc

PLUGIN_DIR=../gcc-plugin
PLUGIN=$(PLUGIN_DIR)/multiverse.so
LIBRARY_DIR=../libmultiverse
LIBRARY=$(LIBRARY_DIR)/libmultiverse.a
EXTRA_DEPS=$(LIBRARY) $(PLUGIN)

CFLAGS = -fplugin=$(PLUGIN) -I$(LIBRARY_DIR) -O2 -Wextra
LDFLAGS = -L$(LIBRARY_DIR) -lmultiverse

SOURCES=$(shell echo *.c)
TESTS=$(foreach x,${SOURCES},$(patsubst %.c,%,$x))

all: $(TESTS)

# common MK processes the SOURCES variable
include ../common.mk


$(LIBRARY): always
	$(MAKE) -C $(LIBRARY_DIR)

$(PLUGIN): always
	$(MAKE) -C $(PLUGIN_DIR)

$(foreach test, $(TESTS), $(eval $(call BINARY_template,$(test))))

clean: defaultclean
	find -regex ".*\\.c\\.[0-9]*[tri]\\..*" | xargs rm -f

test: $(foreach x,${TESTS},$(patsubst %,test-%,$x))
test-%: %
	./$<

.PHONY: always
