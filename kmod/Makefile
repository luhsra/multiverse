# ccflags-y = -fplugin=$(PWD)/../gcc-plugin/multiverse.so -L$(PWD)/../libmultiverse -I$(PWD)/../libmultiverse/ -lmultiverse
# ldflags-y = -L$(PWD)/../libmultiverse -lmultiverse
# ccflags-y = -fplugin=$(PWD)/../gcc-plugin/multiverse.so -I$(PWD)/../libmultiverse/

obj-m := multiverse.o
multiverse-objs := multiverse_test.o std.o libmultiverse_fn.a ../libmultiverse/libmultiverse.a

all: multiverse_fn
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

multiverse_fn:
	gcc -fno-common -mcmodel=kernel -fplugin=../gcc-plugin/multiverse.so -L../libmultiverse -I../libmultiverse/ -lmultiverse -o multiverse_fn.o -c multiverse_fn.c
	ar rcs libmultiverse_fn.a multiverse_fn.o


.PHONY: multiverse_fn
