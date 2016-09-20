GCC=gcc-6
GCCPLUGINS_DIR:= $(shell $(GCC) -print-file-name=plugin)
CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2

PLUGIN_SOURCE_FILES= multiverse.c

plugin.so: $(PLUGIN_SOURCE_FILES)
	$(GCC) -shared $(CXXFLAGS) $^ -o $@
