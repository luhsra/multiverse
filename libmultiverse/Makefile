PLATFORM ?= linux-user
ARCH ?= $(shell uname -m)

ALL_ARCHS = x86 x86_64
ALL_PLATFORMS = linux-user linux-kernel

# x86 and x86_64 currently have the same implementation
ifeq ($(ARCH),x86_64)
  ARCH = x86
endif

SOURCES := mv_commit.c mv_info.c arch-$(ARCH).c platform-$(PLATFORM).c

ifeq ($(PLATFORM),linux-kernel)
  include $(PWD)/kernel.mk
else
  include user.mk
endif
