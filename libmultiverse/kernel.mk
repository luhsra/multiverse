LINUX_BUILD_DIR ?= /lib/modules/$(shell uname -r)/build

SOURCES += platform-linux-kernel-mod.c

obj-m := libmultiverse.o
libmultiverse-objs := $(patsubst %,%.o,$(basename $(SOURCES)))

ccflags-y := -D MULTIVERSE_KERNELSPACE


all:
	make -C $(LINUX_BUILD_DIR) M=$(PWD) modules

clean:
	make -C $(LINUX_BUILD_DIR) M=$(PWD) clean
