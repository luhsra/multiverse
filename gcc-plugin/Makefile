# Required packages
#	- g++-$VERSION
#	- gcc-$VERSION
#	- gcc-$VERSION-source
#	- gcc-$VERSION-plugin-dev

GCCPLUGINS_DIR:= $(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -gdwarf-2 -Wall -Wextra -std=c++11
SOURCES=$(shell echo *.cc)
ALLPRODUCTS=multiverse.so

# Multiverse debug output is disabled by default. Build with `make DEBUG=1 ...`
# to get useful messages during the usage of the compiler plugin.
ifeq ($(DEBUG), 1)
CXXFLAGS += -DCONFIG_DEBUG_OUT=1
endif

all: multiverse.so

include common.mk

multiverse.so: $(OBJECTS)
	$(CXX) -shared $(CXXFLAGS) $^ -o $@

.PHONY = all clean
