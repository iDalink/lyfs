#include "prefix.h"



static int lyfs_create_common_file(struct inode *dir, char *file_name, int isdir)
{
    struct lyfs_meta_node parent_inode;

	lyfs_file_read(fd, &parent_inode, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + dir->i_ino * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));

	char *meta_bitmap = kzalloc(sb_on_drive.meta_node_count, GFP_KERNEL);
	lyfs_file_read(fd, meta_bitmap, sb_on_drive.meta_bitmap_index * LYFS_BLOCK_SIZE, sizeof(struct lyfs_meta_node));

	if (meta_bitmap[dir->i_ino] == 0) {

		printk("lyfs %s\n", "没有这个目录");
    	return -1;
	}

    if (parent_inode.type != LYFS_TYPE_DENTRY) {
		printk("lyfs %s\n", "父文件不是目录");
    	return -1;
    }

    
    int parent_enmpty_child = -1;
    int i;
    for (i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {
    	if (parent_inode.child_meta_indexs[i] == 0) {
    		if (parent_enmpty_child == -1) {
    			parent_enmpty_child = i;
    		}
    	} else {

    		struct lyfs_meta_node child_inode;
			lyfs_file_read(fd, &child_inode, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + parent_inode.child_meta_indexs[i] * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));
    		if (strcmp(child_inode.name, file_name) == 0) {
				printk("lyfs %s\n", "File already exists!\n");
    			return -1;
    		}
    	}
    }

    if (parent_enmpty_child < 0) {
		printk("lyfs %s\n", "该目录已满");
    	return -1;
    }

    struct lyfs_meta_node child_inode;
    memset(&child_inode, 0, sizeof(struct lyfs_meta_node));
    
    strcpy(&child_inode.identifier, "NM");
    strcpy(&child_inode.name, file_name);
    child_inode.type = (isdir == 0) ? LYFS_TYPE_FILE : LYFS_TYPE_DENTRY;
    child_inode.parent_dentry_index = parent_ino;

    int inode = 0;
    for (i = 0; i < sb_on_drive.meta_node_count; i++) {
    	if (meta_bitmap[i] == 0) {
    		inode = i;
    		break;
    	}
    }
    if (inode <= 0) {
		printk("lyfs %s\n", "没有空的inode号");
    	return -1;
    }
    child_inode.node_index = inode;
    printk("lyfs %s=%d\n", "找到了空的inode号", inode);

    lyfs_file_write(fd, &child_inode, \
    	sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + \
    	inode * sizeof(struct lyfs_meta_node),
    	 sizeof(struct lyfs_meta_node));

    meta_bitmap[inode] = 1;
    lyfs_file_write(fd, meta_bitmap, \
    	sb_on_drive.meta_bitmap_index * LYFS_BLOCK_SIZE,
    	 sb_on_drive.meta_node_count);

	parent_inode.child_meta_indexs[parent_enmpty_child] = inode;
    lyfs_file_write(fd, &parent_inode, \
    	sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + \
    	parent_inode.node_index * sizeof(struct lyfs_meta_node),
    	 sizeof(struct lyfs_meta_node));

    {
    	// confirm previous operation is succeed
		lyfs_file_read(fd, &child_inode, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + inode * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));
		printk("lyfs common_create, result = %s\n", child_inode.name);
    }

    return 0;
}

static int lyfs_inode_operations_create(struct inode *dir, struct dentry *dentry, umode_t mode,
		bool excl)
{
	printk("lyfs create, name=%s, inode=%d\n", dentry->d_iname, dir->i_ino);
	lyfs_create_common_file(dir, dentry->d_iname, 0);
	return 0;
}

static struct dentry *lyfs_inode_operations_lookup(struct inode * dir, struct dentry *dentry, unsigned int flags)
{
	struct lyfs_meta_node node;
	struct lyfs_meta_node child_node;
	int i = 0;
	int index = 0;
	int find_child = 0;
	printk("lyfs lookup, dir_ino=0x%x, dentry=%s, dentry.ops=%x\n", dir->i_ino, dentry->d_iname, dentry->d_op);
	dentry->d_op = &lyfs_dentry_operations;

	lyfs_file_read(fd, &node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + dir->i_ino * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));

	for ( i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {

		int child_node_number = node.child_meta_indexs[i];
		
		if (child_node_number != 0)  {

			lyfs_file_read(fd, &child_node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + child_node_number * sizeof(struct lyfs_meta_node), sizeof(child_node));
			//printk("lyfs child_node_number = %d, indentifer = %s, name=%s!\n", child_node_number, child_node.identifier, child_node.name);

			if (strcmp(dentry->d_iname, child_node.name) == 0) {
				find_child = 1;
				break;
			}
		}
	}

	if (find_child == 0) {
		return NULL;
	}

	struct dentry *root_dentry = dentry;//kzalloc(sizeof(struct dentry), GFP_KERNEL);
	struct inode *root_inode = kzalloc(sizeof(struct inode), GFP_KERNEL);//kmem_cache_alloc(dentry_cachep, GFP_NOFS);
	root_inode->i_state = I_NEW;

	if (0 == inode_init_always(dir->i_sb, root_inode)) {
		//printk("lyfs lookup init inode = %d\n", root_inode->i_ino);
	} else {
		//printk("lyfs lookup can not init inode\n");
	}

	root_inode->i_ino = child_node.node_index;
	
	root_inode->i_mode = S_IRWXU|S_IRWXG|S_IRWXO;
	root_inode->i_mode = root_inode->i_mode | ((child_node.type == LYFS_TYPE_DENTRY) ? S_IFDIR : S_IFREG);
	//root_inode->i_mode = S_IFDIR;
	root_inode->i_uid  = current_fsuid();
	root_inode->i_gid  = current_fsgid();
	root_inode->i_fop = NULL;
	//root_inode->i_sb = sb;
	root_inode->i_op = &lyfs_inode_operations;
	root_inode->i_fop = &lyfs_file_operations;
	mutex_init(&root_inode->i_mutex);
	INIT_LIST_HEAD(&root_inode->i_lru);
	INIT_LIST_HEAD(&root_inode->i_io_list);
	INIT_LIST_HEAD(&root_inode->i_sb_list);
	INIT_LIST_HEAD(&root_inode->i_data.private_list);

	root_dentry->d_inode = root_inode;
	
	return d_splice_alias(root_inode, dentry);
}

static int lyfs_inode_operations_mkdir(struct inode * dir, struct dentry *dentry, umode_t mode)
{
	printk("lyfs mkdir, name=%s, inum=%d\n", dentry->d_name.name, (int)dir->i_ino);
	lyfs_create_common_file(dir, dentry->d_iname, 1);
	return 1;
}

static int lyfs_inode_operations_rmdir(struct inode * dir, struct dentry *dentry)
{
	printk("lyfs rmdir, name=%s, inum=%d\n", dentry->d_name.name, (int)dir->i_ino);
	return 1;
}

static int lyfs_inode_operations_mknod(struct inode * dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
	printk("lyfs mknod, name=%s, inum=%d\n", dentry->d_name.name, (int)dir->i_ino);
	lyfs_create_common_file(dir, dentry->d_iname, 0);
	return 1;
}

struct inode_operations lyfs_inode_operations = {
	.create		= lyfs_inode_operations_create,
	.lookup		= lyfs_inode_operations_lookup,
	.mkdir		= lyfs_inode_operations_mkdir,
	.rmdir		= lyfs_inode_operations_rmdir,
	.mknod		= lyfs_inode_operations_mknod
};
