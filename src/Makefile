# Required packages
# 	- g++-$VERSION
# 	- gcc-$VERSION
# 	- gcc-$VERSION-source
# 	- gcc-$VERSION-plugin-dev

HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN= multiverse
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti

all: $(PLUGIN).cc
	$(HOST_GCC) -shared $(CXXFLAGS) $(PLUGIN).cc -o $(PLUGIN).so

clean:
	rm -f *.o *.so test
	rm -f test_1 test_1.c.*

# DEBUGGING & TESTING

debug: CXXFLAGS+= -DDEBUG -g
debug: all

test: test_1

test_1: debug
	rm -f test_1 test_1.c.*
	$(TARGET_GCC) -fplugin=$(PWD)/$(PLUGIN).so -o test_1 test_1.c -fdump-tree-optimized -O2
	less test_1.c.211t.optimized

.PHONY= all clean debug test
