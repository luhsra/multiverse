# Required packages
#	- g++-$VERSION
#	- gcc-$VERSION
#	- gcc-$VERSION-source
#	- gcc-$VERSION-plugin-dev

PREFIX ?= /usr/local
GCCPLUGINS_DIR:= $(shell $(CXX) -print-file-name=plugin)
GCCPLUGINS_INSTALL_DIR:= $(subst /usr,$(PREFIX),$(GCCPLUGINS_DIR))
CXXFLAGS += -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -gdwarf-2 -Wall -Wextra -std=c++11
SOURCES:= $(shell echo *.cc)
ALLPRODUCTS=multiverse.so

ifneq ($(DESTDIR),)
	DESTDIR := ../$(DESTDIR)
endif

# Multiverse debug output is disabled by default. Build with `make DEBUG=1 ...`
# to get useful messages during the usage of the compiler plugin.
ifeq ($(DEBUG), 1)
CXXFLAGS += -DCONFIG_DEBUG_OUT=1
endif

all: multiverse.so

include common.mk

multiverse.so: $(OBJECTS)
	$(CXX) -shared $(CXXFLAGS) $^ -o $@

install: multiverse.so 
	install -d $(DESTDIR)$(GCCPLUGINS_INSTALL_DIR)
	install multiverse.so $(DESTDIR)$(GCCPLUGINS_INSTALL_DIR)

uninstall:
	rm -f $(DESTDIR)$(GCCPLUGINS_DIR)/multiverse.so

.PHONY = all clean install uninstall
