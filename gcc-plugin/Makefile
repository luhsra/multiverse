# Required packages
# 	- g++-$VERSION
# 	- gcc-$VERSION
# 	- gcc-$VERSION-source
# 	- gcc-$VERSION-plugin-dev

HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN= multiverse
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -DDEBUG=1

all: $(PLUGIN).so

$(PLUGIN).so: multiverse.o mv-info.o
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o *.so

.PHONY= all clean debug test
