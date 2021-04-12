ifneq  ($(KERNELRELEASE),)
obj-m:=tsf.o
tsf-objs := sch_tsf.o sch_kfifo.o
else
KDIR := /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.symvers *.cmd *.cmd.o
endif

