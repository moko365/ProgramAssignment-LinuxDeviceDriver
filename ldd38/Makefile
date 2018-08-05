obj-m := cdata.o cdata_plat_dev.o cdata_dev_class.o

CONFIG_MODULE_SIG=n
KDIR := /usr/src/linux-headers-3.13.0-74-generic
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c test
