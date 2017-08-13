#include "prefix.h"


//struct dentry lyfs_get_common_dentry

static int lyfs_dentry_operations_hash(const struct dentry *dentry, struct qstr *str)
{
	printk("lyfs hash, dentry=%s, str=%x\n", dentry->d_iname, str->name);
	return 0;
}


static int lyfs_dentry_operations_delete(const struct dentry *dentry)
{
	printk("lyfs delete, dentry=%s, inode_number=%d\n", dentry->d_iname, dentry->d_inode->i_ino);
	return -1;
}

struct dentry_operations lyfs_dentry_operations = {
	.d_hash = lyfs_dentry_operations_hash,
	.d_delete = lyfs_dentry_operations_delete
};