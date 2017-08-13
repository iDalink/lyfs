

.PHONY = show_output load unload lyfs.ko

show_output:load
	dmesg | grep lyfs

load:lyfs.ko
	@sudo insmod lyfs.ko

unload:
	@sudo rmmod lyfs