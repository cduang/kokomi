obj-m += netlink_virt_to_phys.o
KDIR := ~/Kernel/10-Ultra/out
all:
	make -C $(KDIR) M=$(PWD) ARCH=arm64 SUBARCH=arm64 modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
