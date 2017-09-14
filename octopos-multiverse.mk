$(shell echo Multiverse support was loaded >&2)

MULT_DIR=${SRC_DIR}/multiverse
MULT_PLUGIN_DIR=${MULT_DIR}/gcc-plugin
MULT_PLUGIN=${MULT_PLUGIN_DIR}/multiverse.so
MULT_PLUGIN_SRC=$(shell find ${MULT_PLUGIN_DIR} -name "*.cc"  2> /dev/null)

MULT_LIB_DIR=${MULT_DIR}/libmultiverse


MODULES_ALL += multiverse
MODULES_CLEAN += multiverse

KERNEL_FLAGS += -I${MULT_LIB_DIR} -fplugin=${MULT_PLUGIN}

MULT_C_SRC   = $(shell find $(MULT_LIB_DIR) -name "*.c"  2> /dev/null)
MULT_CC_SRC   = $(shell find $(MULT_LIB_DIR) -name "*.cc"  2> /dev/null)
MULT_C_OBJ := $(MULT_C_SRC:%=%.o)
MULT_CC_OBJ := $(MULT_CC_SRC:%=%.o)
MULT_PREF_OBJ := $(addprefix $(OBJDIR)/,$(MULT_C_OBJ) $(MULT_CC_OBJ))
# Include libmultiverse into the kernel
OCTOPOS_EXTRA_PREF_OBJ += ${MULT_PREF_OBJ}

MULT_DEP := $(patsubst %.o,$(DEPDIR)/%.d,$(MULT_C_OBJ) $(MULT_CC_OBJ))
DEP_FILES += ${MULT_DEP}

# Multiverse GCC Plugin is build before anything else
${OBJDIR}/puma.config: ${MULT_PLUGIN}
${MULT_PLUGIN}: ${MULT_PLUGIN_SRC}
	@echo "Build Multiverse GCC Plugin: ${MULT_PLUGIN}"
	@${MAKE} -C ${MULT_PLUGIN_DIR}

clean_multiverse:
	$(RM) ${MULT_PREF_OBJ}

# This module is not part of the OS kernel
${OBJDIR}/${SRC_DIR}/multiverse/libmultiverse/%.c.o: ${SRC_DIR}/multiverse/libmultiverse/%.c
	@echo "(NON-KERNEL) CC   $@"
	@if test \( ! \( -d ${@D} \) \); then mkdir -p ${@D}; fi
	${VERBOSE} ${CC} -c ${CFLAGS} ${EXTERNAL_FLAGS} -o $@ $<

${OBJDIR}/${SRC_DIR}/multiverse/libmultiverse/%.cc.o: ${SRC_DIR}/multiverse/libmultiverse/%.cc build/puma.config
	@echo "(NON-KERNEL) CXX $@"
	@if test \( ! \( -d ${@D} \) \); then mkdir -p ${@D}; fi
	${VERBOSE} ${CXX} ${CXXFLAGS} ${EXTERNAL_FLAGS} -c -o $@ $<
