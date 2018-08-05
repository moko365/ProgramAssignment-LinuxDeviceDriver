obj-m := cdata.o cdata_plat_dev.o cdata_dev_class.o omap34xx_sht7x.o

CONFIG_MODULE_SIG=n
#KDIR := /usr/src/linux-headers-4.4.0-1062-aws
KDIR := /usr/src/linux-headers-4.8.0-58-generic
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c test
