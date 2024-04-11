KERN_DIR = /home/asc3nt/proj02/rk356x_linux_release_v1.3.0b_20221213/kernel

obj-m   += gpio-demo-drv.o

gpio-demo-drv.ko: gpio-demo-drv.c
	make -C $(KERN_DIR) M=`pwd` modules
	rm -rf *.o *.mod.* *.symvers *.order .*o.cmd .tmp_versions

all: gpio-demo-drv.ko

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
	rm -f gpio-demo-drv



