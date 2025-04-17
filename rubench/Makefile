MODULE_NAME := rubench

KERNEL_SRC := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

obj-m += $(MODULE_NAME).o

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean

load:
	sudo insmod $(MODULE_NAME).ko
	sudo chmod 666 /dev/$(MODULE_NAME)

unload:
	sudo rmmod $(MODULE_NAME)