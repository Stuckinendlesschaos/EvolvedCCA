ifneq ($(KERNELRELEASE),)

# kbuild part of makefile
obj-m  := elastic.o
#tcp_elastic-y := Elastic_TCP.o

else
# normal makefile

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
endif
