obj-m := think-lmi.o

KERNELRELEASE := $(shell uname -r)
KDIR := /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)
INSTALL	:= install
MODLOADDIR := /etc/modules-load.d
MODLOADCONF := $(MODLOADDIR)/think-lmi.conf

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

modules_install:
	$(INSTALL) -m 644 think-lmi.ko /lib/modules/$(KERNELRELEASE)/extra/

install: modules_install
	$(INSTALL) -m 644 think-lmi.conf $(MODLOADDIR)
	depmod
	modprobe think-lmi
