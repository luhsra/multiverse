# Required packages
# 	- gcc-$VERSION-source
# 	- gcc-$VERSION-plugin-dev

HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN_SOURCE_FILES= multiverse.c
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2

plugin.so: $(PLUGIN_SOURCE_FILES)
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

test:
	$(TARGET_GCC) -c -fplugin=$(PWD)/plugin.so test.c

.PHONY= test
