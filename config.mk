PLATFORM ?= linux-kernel
ARCH     ?= $(shell uname -m)

# x86 and x86_64 currently have the same implementation
ifeq ($(ARCH),x86_64)
	ARCH = x86
endif
