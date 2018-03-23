obj-m := cc2500in.o
cc2500in-objs := cc2500.o phy.o driver.o

#KDIR := /home/S5-Driver/lesson7/smart210/linux-smart210
KDIR := /home/S5-Driver/lesson7/smart210/linux-smart210-arm-4.5.1/linux-smart210
all:
	make -C $(KDIR) M=$(PWD) modules CROSS_COMPILE=arm-linux- ARCH=arm

clean:
	rm -f *.ko *.o *.bak
