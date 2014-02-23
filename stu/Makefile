obj-m := cdata-fb.o

KDIR := /lib/modules/2.6.24-26-generic/build
#KDIR := /home/android/android-kernel/common

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c
