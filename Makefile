obj-m += mymem.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod mymem.ko

unload:
	sudo rmmod mymem

reload: unload load

test:
	cat /dev/mymem

