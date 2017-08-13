#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "utility.h"
#include "../lyfs_type.h"

int main(int arg_size, char *argv[]) {
	
	if (arg_size != 2) {
    	printf("You should put only one disk path after `mkfs.lyfs` command!\n");
		return 0;
	}

	char *disk_path = argv[1];

	int fd = open(disk_path, O_RDWR);
	if (fd <= 0) {
    	printf("Can not open %s, errcode=%d, info=%s!\n", disk_path, errno, strerror(errno));
		return 0;
	}

	char buffer[64];
    struct stat buf;
    stat(disk_path, &buf);
    printf("filesize=%lldB\n", buf.st_size);
    printf("filesize:%s\n", gigabyte_suffix(buf.st_size, buffer));

    struct lyfs_super_block sb;
    sb.identifier[0] = 'l';
    sb.identifier[1] = 'y';
    sb.identifier[2] = 'f';
    sb.identifier[3] = 's';
    sb.identifier[4] = '\0';
    sb.partion_size = buf.st_size / LYFS_BLOCK_SIZE * LYFS_BLOCK_SIZE;
    sb.meta_bitmap_index = 1;
    
    sb.meta_node_count = sb.partion_size / LYFS_BLOCK_SIZE / 16;
    sb.meta_area_count = (sb.meta_node_count * sizeof(struct lyfs_meta_node) - 1) / LYFS_BLOCK_SIZE + 1;

    sb.meta_bitmap_block_count = (sb.meta_node_count - 1) / LYFS_BLOCK_SIZE + 1;

    sb.meta_area_index = 1 + sb.meta_bitmap_block_count;

    sb.data_bitmap_index = sb.meta_area_index + sb.meta_area_count;
    long sy_size = sb.partion_size / LYFS_BLOCK_SIZE - sb.data_bitmap_index;

    sb.data_area_count = (sy_size * LYFS_BLOCK_SIZE) / (1 + LYFS_BLOCK_SIZE);
    sb.data_bitmap_block_count = (sb.data_area_count - 1) / LYFS_BLOCK_SIZE + 1;
    sb.data_area_index = sb.data_bitmap_index + sb.data_bitmap_block_count;
	{
	    printf("分区大小           %s\n", gigabyte_suffix(sb.partion_size, buffer));
	    printf("位图起始索引       %d\n", sb.meta_bitmap_index);
	    printf("位图占有块数       %d\n", sb.meta_bitmap_block_count);
	    printf("元数据起始索引     %d\n", sb.meta_area_index);
	    printf("元数据占有块数     %d\n", sb.meta_area_count);
	    printf("元数据的数量       %d\n", sb.meta_node_count);
	    printf("数据位图起始索引   %d\n", sb.data_bitmap_index);
	    printf("数据位图占有块数   %d\n", sb.data_bitmap_block_count);
	    printf("数据起始索引       %d\n", sb.data_area_index);
	    printf("数据占有块数       %d\n", sb.data_area_count);
	}

	struct lyfs_meta_node root_node;
	memset(&root_node, 0, sizeof(root_node));
	root_node.identifier[0] = 'N';
	root_node.identifier[1] = 'M';
	root_node.type = LYFS_TYPE_DENTRY;

	root_node.name[0] = 'r';
	root_node.name[1] = 'o';
	root_node.name[2] = 'o';
	root_node.name[3] = 't';

	size_t maped_size = LYFS_BLOCK_SIZE * (sb.meta_area_index + 1);

	void *mem = mmap(NULL, maped_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (mem == NULL) {
		close(fd);
		printf("mmap error, errcode=%d, info=%s\n", errno, strerror(errno));
		return -1;
	}
	char *data = mem;
	// meta bitmap
	memset(mem, 0, maped_size);

	// super block
	memcpy(mem, &sb, sizeof(sb));
	data[sb.meta_bitmap_index * LYFS_BLOCK_SIZE] = '\1';
	// root dir
	memcpy(data + sb.meta_area_index * LYFS_BLOCK_SIZE, &root_node, sizeof(root_node));

	munmap(mem, maped_size);
	close(fd);
	return 0;
}