obj-m := lyfs.o
#要生成的模块名     
lyfs-objs:= super_block.o inode.o dentry.o file.o utility/utility.o
#生成这个模块名所需要的目标文件

KDIR := ~/compile/linux-4.4
PWD := $(shell pwd)

.PHONY = lyresult lybuild lyrun lyclearmsg lymount

result:run
	@dmesg

run:build clearmsg
	@sudo insmod lyfs.ko

clearmsg:
	@sudo dmesg --clear

unload:
	-@sudo umount output/point
	@sudo rmmod -f lyfs

output/lyfs.ko:build
	@cp lyfs.ko output/lyfs.ko

build:Makefile
	-@make -C $(KDIR) M=$(PWD) modules

mount:run
	-sudo mount -t lyfs output/disk.img output/point
	dmesg

clean:
	-@rm -rf *.o  *.ko *.order *.symvers *.mod.c .*.cmd utility/.*.*.cmd .tmp_versions
