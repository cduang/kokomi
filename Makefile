MODULE_NAME := kokomi
obj-m := kokomi.o
KER=/home/cas/Kernel/cas/out

all:
	rm -rf *.ko
	make -C $(KER) M=$(PWD) modules
	rm -rf *.mod* *.sym* *.cmd* *.order *.o
clean:
	rm -rf *.ko
	rm -rf *.mod* *.sym* *.cmd* *.order *.o
