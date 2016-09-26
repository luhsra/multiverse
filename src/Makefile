# Required packages
# 	- gcc-$VERSION-source
# 	- gcc-$VERSION-plugin-dev

HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN= multiverse
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O0

all: $(PLUGIN).c
	$(HOST_GCC) -shared $(CXXFLAGS) $(PLUGIN).c -o $(PLUGIN).so

clean:
	rm -f *.o *.so test
	rm -f test_1 test_1.c.*

# DEBUGGING & TESTING

debug: CXXFLAGS+= -DDEBUG
debug: all

test: test_1

test_1: debug
	rm -f test_1 test_1.c.*
	$(TARGET_GCC) -fplugin=$(PWD)/$(PLUGIN).so -o test_1 test_1.c -fdump-tree-optimized



.PHONY= all clean debug test
