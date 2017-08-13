.PHONY = run test clean

DISK_PATH=../output/disk.img

BUILD_FLAG=-std=c99 -g

test:test_example
	./test_example $(DISK_PATH)
	
test_example:test_example.c utility.c ../lyfs_type.h
	gcc $(BUILD_FLAG) -o $@ $^

run:mkfs.lyfs
	./mkfs.lyfs $(DISK_PATH)

mkfs.lyfs:mkfs.lyfs.c utility.c ../lyfs_type.h
	gcc $(BUILD_FLAG) -o $@ $^

clean:
	-@rm mkfs.lyfs
	-@rm test_example