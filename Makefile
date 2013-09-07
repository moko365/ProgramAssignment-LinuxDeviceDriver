#
# Author: jollen
# 
obj-m :=  omap34xx_sht7x.o

KDIR := /lib/modules/2.6.24-26-generic/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c
