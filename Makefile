obj-m += kokomi.o
KERNEL_SRC := /home/runner/work/xiaomi-cas-online/xiaomi-cas-online/source/out
all:
    make -C $(KERNEL_SRC) M=$(PWD) ARCH=arm64 SUBARCH=arm64 modules
clean:
    rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
