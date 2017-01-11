# Required packages
# 	- g++-$VERSION
# 	- gcc-$VERSION
# 	- gcc-$VERSION-source
# 	- gcc-$VERSION-plugin-dev

GCCPLUGINS_DIR:= $(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -gdwarf-2 -Wall -Wextra
SOURCES=$(shell echo *.cc)
ALLPRODUCTS=multiverse.so

all: multiverse.so

include ../common.mk

multiverse.so: $(OBJECTS)
	$(CXX) -shared $(CXXFLAGS) $^ -o $@

.PHONY = all clean
