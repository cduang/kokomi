obj-m += kokomi.o

KERNEL_SRC := /home/runner/work/xiaomi-cas-online/xiaomi-cas-online/source/out

all:
            make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
            make -C $(KERNEL_SRC) M=$(PWD) clean
