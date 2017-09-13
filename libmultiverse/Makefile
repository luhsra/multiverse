include ../config.mk

SOURCES := mv_commit.c mv_info.c arch-$(ARCH).c platform-$(PLATFORM).c

ifeq ($(PLATFORM),linux-kernel)
  obj-y := libmultiverse.o
  libmultiverse-objs := $(patsubst %,%.o,$(basename $(SOURCES)))
  ccflags-y := -D MULTIVERSE_KERNELSPACE
else
  include user.mk
endif
