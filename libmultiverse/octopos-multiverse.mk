$(shell echo Multiverse support was loaded >&2)

MULT_DIR=${SRC_DIR}/multiverse-gcc-plugin
MULT_PLUGIN=${MULT_DIR}/multiverse.so
MULT_PLUGIN_SRC=$(shell find ${MULT_PLUGIN_DIR} -name "*.cc"  2> /dev/null)

MODULES_ALL += multiverse
MODULES_CLEAN += multiverse

KERNEL_FLAGS += -I$(SRC_DIR)/libmultiverse -fplugin=${MULT_PLUGIN}

MULT_C_SRC   = $(shell find $(SRC_DIR)/libmultiverse -name "*.c"  2> /dev/null)
MULT_C_OBJ := $(MULT_C_SRC:%=%.o)
MULT_PREF_OBJ := $(addprefix $(OBJDIR)/,$(MULT_C_OBJ))
# Include libmultiverse into the kernel
OCTOPOS_EXTRA_PREF_OBJ += ${MULT_PREF_OBJ}

MULT_DEP := $(patsubst %.o,$(DEPDIR)/%.d,$(MULT_C_OBJ))
DEP_FILES += ${MULT_DEP}

# Multiverse GCC Plugin is build before anything else
${OBJDIR}/puma.config: ${MULT_PLUGIN}
${MULT_PLUGIN}: ${MULT_PLUGIN_SRC}
	@echo "Build Multiverse GCC Plugin: ${MULT_PLUGIN}"
	@${MAKE} -C ${MULT_DIR}

clean_multiverse:
	$(RM) ${MULT_PREF_OBJ}


# This module is not part of the OS kernel
$(eval $(call EXTERNAL_MODULE,libmultiverse))
