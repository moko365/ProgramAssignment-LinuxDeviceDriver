obj-m := cdata.o

#
# See: http://stackoverflow.com/questions/24975377/kvm-module-verification-failed-signature-and-or-required-key-missing-taintin
#
CONFIG_MODULE_SIG=n

KDIR := /usr/src/linux-headers-3.13.0-74-generic 
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c test
