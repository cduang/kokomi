MODULE_NAME := kokomi
obj-m := kokomi.o
KERNEL_SRC := /home/cas/Kernel/cas/out
all:
    make -C $(KERNEL_SRC) M=$(PWD) ARCH=arm64 SUBARCH=arm64 modules
clean:
    rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
