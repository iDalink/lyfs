
#define LYFS_BLOCK_SIZE (4 * 1024)

#define LYFS_FILE_NAME_LENGTH 15

#define LYFS_DENTRY_CHILD_LIMIT_COUNT 16

#define LYFS_TYPE_FILE 0
#define LYFS_TYPE_DENTRY 1

struct lyfs_data_block
{
	unsigned int file_size;
	// must greater than zero
	unsigned int link_count;
	/*
	char data[LYFS_BLOCK_SIZE - sizeof(unsigned int)];
	unsigned int next_block_index;
	*/
};

struct lyfs_meta_node
{
	char identifier[3];
	// last charactor must be '\0'
	char name[LYFS_FILE_NAME_LENGTH + 1];
	
	unsigned int node_index;

	// file or dentry
	unsigned int type;

	unsigned int data_block_index;

	// it point to meta_inode index
	unsigned int parent_dentry_index;

	unsigned int child_meta_indexs[LYFS_DENTRY_CHILD_LIMIT_COUNT];
};

struct lyfs_super_block
{
	char identifier[8];
	// partion size, written when mkfs.
	unsigned long partion_size;

	// current equal 1
	unsigned int meta_bitmap_index;

	//bitmap area has blocks
	unsigned int meta_bitmap_block_count;

	// index of meta_area first block.
	// meta_area_index = meta_bitmap_index + meta_bitmap_block_count
	unsigned int meta_area_index;
	
	// How much meta_node count.wirtten when mkfs.
	// meta_node_count = file_size / LYFS_BLOCK_SIZE / 16
	unsigned int meta_node_count;

	// How much block count.wirtten when mkfs.
	// meta_area_count = meta_node_count * sizeof(struct lyfs_meta_node) / LYFS_BLOCK_SIZE.
	unsigned int meta_area_count;

	// meta_bitmap_index = meta_area_index + meta_area_count
	unsigned int data_bitmap_index;

	//bitmap area has blocks
	unsigned int data_bitmap_block_count;

	unsigned int data_area_index;

	unsigned int data_area_count;

};
