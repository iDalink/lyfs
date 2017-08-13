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


char buffer[64];

// read all over file system info
int read_file_system_info(void *mem)
{
    struct lyfs_super_block *_sb = (struct lyfs_super_block *)mem;
    {
    	struct lyfs_super_block sb = *_sb;
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
	return 0;
}

//　read one file info by inode number
int read_file_info(void *mem, int inode, struct lyfs_meta_node *node)
{
    struct lyfs_super_block *_sb = (struct lyfs_super_block *)mem;
    
    char *bitmap_pointer = mem + _sb->meta_bitmap_index * LYFS_BLOCK_SIZE;
    struct lyfs_meta_node *meta_pointer = mem + _sb->meta_area_index * LYFS_BLOCK_SIZE;

    if (*(bitmap_pointer + inode) == 0) {
    	return -1;
    }

    *node = *(meta_pointer + inode);

	return 0;
}

// read one dir`s file
int read_file_info_byname(void *mem, int parent_inode_number, char *file_name, struct lyfs_meta_node *node)
{
    struct lyfs_super_block *_sb = (struct lyfs_super_block *)mem;
    
    char *bitmap_pointer = mem + _sb->meta_bitmap_index * LYFS_BLOCK_SIZE;
    struct lyfs_meta_node *meta_pointer = mem + _sb->meta_area_index * LYFS_BLOCK_SIZE;

    struct lyfs_meta_node parent_inode;

    if (read_file_info(mem, parent_inode_number, &parent_inode) < 0) {
    	return -1;
    }
    if (parent_inode.type != LYFS_TYPE_DENTRY) {
		printf("%s\n", "父文件不是目录");
    	return -1;
    }

    for (int i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {
    	if (parent_inode.child_meta_indexs[i]) {
		    struct lyfs_meta_node *child_node = meta_pointer + parent_inode.child_meta_indexs[i];
			if (strcmp(file_name, child_node->name) == 0) {
				
				*node = *child_node;
				
				return 0;
			}
    	}
    }

	return -1;
}

// create file in dir
int create_file(void *mem, int parent_ino, char *file_name, int isdir)
{
    struct lyfs_super_block *_sb = (struct lyfs_super_block *)mem;
    struct lyfs_meta_node *meta_pointer = mem + _sb->meta_area_index * LYFS_BLOCK_SIZE;

    struct lyfs_meta_node parent_inode;

    if (read_file_info(mem, parent_ino, &parent_inode) < 0) {

		printf("%s\n", "没有这个目录");
    	return -1;
    }
    if (parent_inode.type != LYFS_TYPE_DENTRY) {

		printf("%s\n", "父文件不是目录");
    	return -1;
    }
    
    int parent_enmpty_child = -1;
    for (int i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {
    	if (parent_inode.child_meta_indexs[i] == 0) {
    		if (parent_enmpty_child == -1) {
    			parent_enmpty_child = i;
    		}
    	} else {

    		if (strcmp((meta_pointer + parent_inode.child_meta_indexs[i])->name, file_name) == 0) {

				printf("%s\n", "File already exists!\n");
    			return -1;
    		}
    	}
    }

    if (parent_enmpty_child < 0) {

		printf("%s\n", "该目录已满");
    	return -1;
    }

    struct lyfs_meta_node child_inode;
    memset(&child_inode, 0, sizeof(struct lyfs_meta_node));
    
    strcpy((char *)&child_inode.identifier, "NM");
    strcpy((char *)&child_inode.name, file_name);
    child_inode.type = (isdir == 0) ? LYFS_TYPE_FILE : LYFS_TYPE_DENTRY;
    child_inode.parent_dentry_index = parent_ino;

    int inode = 0;
    char *bitmap_pointer = mem + _sb->meta_bitmap_index * LYFS_BLOCK_SIZE;
    for (int i = 0; i < _sb->meta_node_count; i++) {
    	if (*bitmap_pointer == 0) {
    		inode = i;
    		*bitmap_pointer = 1;
    		break;
    	}
    	bitmap_pointer++;
    }

    if (inode <= 0) {

		printf("%s\n", "没有空的inode号");
    	return -1;
    }
    child_inode.node_index = inode;
    printf("%s=%d\n", "找到了空的inode号", inode);

    *(meta_pointer + inode) = child_inode;

	parent_inode.child_meta_indexs[parent_enmpty_child] = inode;
    *(meta_pointer + parent_ino) = parent_inode;

	return 0;
}

// list all file by tree graphic
void find(void *mem, struct lyfs_meta_node node, int depth)
{
	struct lyfs_meta_node child_node;

    int parent_enmpty_child = -1;
    for (int i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {
    	if (node.child_meta_indexs[i] != 0) {
    	
			read_file_info(mem, node.child_meta_indexs[i], &child_node);

			int _depth = depth;
			while(_depth -- ) printf(" ");

			if (child_node.type == LYFS_TYPE_DENTRY)
				printf("*");
			printf("%s\n", child_node.name);

			if (child_node.type == LYFS_TYPE_DENTRY)
				find(mem, child_node, depth + 1);
    	}
    }
}

int main(int arg_size, char *argv[])
{

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

    struct stat buf;
    stat(disk_path, &buf);
    printf("filesize=%lldB\n", buf.st_size);
    printf("filesize:%s\n", gigabyte_suffix(buf.st_size, buffer));

	size_t maped_size = buf.st_size;
	void *mem = mmap(NULL, maped_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);


	read_file_system_info(mem);

	struct lyfs_meta_node node;
	if (read_file_info(mem, 0, &node) >= 0) {
    	printf("查找到文件, is %s, %s\n", node.name, (node.type == LYFS_TYPE_DENTRY) ? "dir" : "file");
	} else {
    	printf("没有查找到文件\n");
	}

	if (read_file_info_byname(mem, 0, "root", &node) >= 0) {
    	printf("查找到文件, is%s, %s\n", node.name, (node.type == LYFS_TYPE_DENTRY) ? "dir" : "file");
	} else {
    	printf("没有查找到文件\n");
	}
	if (create_file(mem, 0, "nihao", 1) >= 0) {
    	printf("创建文件\n");
	} else {		
    	printf("无法创建文件\n");
	}
	if (create_file(mem, 0, "file", 0) >= 0) {
    	printf("创建文件\n");
	} else {		
    	printf("无法创建文件\n");
	}

	if (read_file_info_byname(mem, 0, "nihao", &node) >= 0) {
    	printf("查找到文件, is %s, %s\n", node.name, (node.type == LYFS_TYPE_DENTRY) ? "dir" : "file");

		if (create_file(mem, node.node_index, "sec", 1) >= 0) {
    		printf("创建文件\n");
		} else {		
    		printf("无法创建文件\n");
		}
		if (create_file(mem, node.node_index, "sfil", 0) >= 0) {
    		printf("创建文件\n");
		} else {		
    		printf("无法创建文件\n");
		}
	} else {
    	printf("没有查找到文件\n");
	}

	munmap(mem, maped_size);
	close(fd);
	
	{

		int fd = open(disk_path, O_RDWR);
		if (fd <= 0) {
			return -1;
		}

	    struct stat buf;
	    stat(disk_path, &buf);

		size_t maped_size = buf.st_size;
		void *mem = mmap(NULL, maped_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

		struct lyfs_meta_node root_node;
		read_file_info(mem, 0, &root_node);
		
		printf("*%s\n", root_node.name);
		
		find(mem, root_node, 1);

		munmap(mem, maped_size);
		close(fd);
	}

	return 0;
}