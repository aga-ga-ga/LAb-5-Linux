MODULENAME=calc

obj-m += $(MODULENAME).o

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod $(MODULENAME).ko

uninstall:
	sudo rmmod $(MODULENAME)
