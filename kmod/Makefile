LINUX_BUILD_DIR ?= /lib/modules/$(shell uname -r)/build

obj-m := multiverse_test.o
ccflags-y := -fplugin=$(PWD)/../gcc-plugin/multiverse.so

all:
	make -C $(LINUX_BUILD_DIR) M=$(PWD) modules

clean:
	make -C $(LINUX_BUILD_DIR) M=$(PWD) clean
