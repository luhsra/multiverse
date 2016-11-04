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

all: $(PLUGIN).so

$(PLUGIN).so: $(PLUGIN).cc
	$(HOST_GCC) -shared $(CXXFLAGS) $(PLUGIN).cc -o $(PLUGIN).so

clean:
	rm -f *.o *.so test
	rm -f test_1 test_1.c.*

# DEBUGGING & TESTING

debug: CXXFLAGS+= -DDEBUG -g
debug: all

test: test_1

test_1: debug libmultiverse
	rm -f test_1 test_1.c.*
	$(TARGET_GCC) -fplugin=$(PWD)/$(PLUGIN).so -o test_1 test_1.c -fdump-tree-optimized $(CFLAGS) \
	   -L../libmultiverse -lmultiverse
	# less test_1.c.211t.optimized

test_1.S: debug libmultiverse
	rm -f test_1 test_1.c.*
	$(TARGET_GCC) -fplugin=$(PWD)/$(PLUGIN).so test_1.c -fdump-tree-optimized $(CFLAGS) \
	   -S -o- 
	# less test_1.c.211t.optimized

libmultiverse:
	$(MAKE) -C ../libmultiverse

.PHONY= all clean debug test
