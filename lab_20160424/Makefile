obj-m := cdata.o cdata_plat_dev.o

KDIR := /usr/src/linux-headers-3.13.0-74-generic
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c test
