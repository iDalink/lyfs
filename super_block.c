#include "prefix.h"

struct lyfs_super_block sb_on_drive;

MODULE_AUTHOR("LY");
MODULE_LICENSE("GPL");

static int nbr = 10;
module_param(nbr, int, S_IRUGO);

struct file *fd= NULL;

static int lyfs_super_operations_show_path(struct seq_file *seq, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);

	seq_printf(seq, "lyfs show_path, dentry=%s\n", dentry->d_iname);
	printk("lyfs show_path, dentry=%s\n", dentry->d_iname);
	return 0;
}

static void super_operations_destroy_inode(struct inode *inode)
{
	printk("lyfs destroy_inode, inode_number=%d\n", inode->i_ino);
	kfree(inode);
}

static const struct super_operations lyfs_super_operations = {
	.destroy_inode = super_operations_destroy_inode,
	.show_path = lyfs_super_operations_show_path,
};

static void lyfs_kill_sb(struct super_block *sb)
{
	printk("lyfs kill sb!\n");
}

static int lyfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct dentry *root_dentry = kzalloc(sizeof(struct dentry), GFP_KERNEL);
	struct inode *root_inode = kzalloc(sizeof(struct inode), GFP_KERNEL);
	printk("lyfs root_inode=0x%x\n", root_inode);
	{
		lyfs_file_read(fd, (void *)&sb_on_drive, 0, sizeof(sb_on_drive));

		char buffer[100];
		printk("lyfs partion identifier %s!\n", sb_on_drive.identifier);
		gigabyte_suffix(buffer, sb_on_drive.partion_size);
		printk("lyfs partion size is %s\n", buffer);
		printk("lyfs meta index is %d\n", sb_on_drive.meta_area_index);
	}
	

	if (0 == inode_init_always(sb, root_inode)) {
		printk("lyfs success lyfs_fill_super\n");
	} else {
		printk("lyfs fail lyfs_fill_super\n");

	}

	INIT_HLIST_BL_NODE(&root_dentry->d_hash);
	INIT_LIST_HEAD(&root_dentry->d_lru);
	INIT_LIST_HEAD(&root_dentry->d_subdirs);
	INIT_HLIST_NODE(&root_dentry->d_u.d_alias);
	INIT_LIST_HEAD(&root_dentry->d_child);
	root_dentry->d_lockref.count = 1;
	spin_lock_init(&root_dentry->d_lockref.lock);
	root_dentry->d_iname[0] = 'r';
	root_dentry->d_iname[1] = 'o';
	root_dentry->d_iname[2] = 'o';
	root_dentry->d_iname[3] = 't';
	root_dentry->d_iname[4] = '\0';
	root_dentry->d_parent = root_dentry;
	root_dentry->d_sb = sb;
	root_dentry->d_flags = DCACHE_ENTRY_TYPE & DCACHE_DIRECTORY_TYPE;

	root_inode->i_mode = S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO;
	//root_inode->i_mode = S_IFDIR;
	root_inode->i_uid  = current_fsuid();
	root_inode->i_gid  = current_fsgid();
	root_inode->i_fop = 2;
	root_inode->i_sb = sb;
	root_inode->i_op = &lyfs_inode_operations;
	root_inode->i_fop = &lyfs_file_operations;
	mutex_init(&root_inode->i_mutex);
	INIT_LIST_HEAD(&root_inode->i_lru);
	INIT_LIST_HEAD(&root_inode->i_io_list);
	INIT_LIST_HEAD(&root_inode->i_sb_list);
	INIT_LIST_HEAD(&root_inode->i_data.private_list);
	spin_lock_init(&root_dentry->d_lock);

	root_dentry->d_inode = root_inode;
	
	root_dentry->d_sb = sb;
	sb->s_root = root_dentry;
	return 0;
}

static struct dentry *lyfs_filesystem_type_mount(struct file_system_type *fs_type, int flags,
		       const char *dev_name, void *data)
{
	fd = filp_open(dev_name, flags|O_RDWR, S_IRWXUGO);
	if (fd == NULL) {
		printk("lyfs can not open file!\n");
		return NULL;
	}

	return mount_nodev(fs_type, flags, data, lyfs_fill_super);
}

struct file_system_type lyfs_type = {
	.name = "lyfs",
	.owner = THIS_MODULE,
	.mount = lyfs_filesystem_type_mount,
	.kill_sb = lyfs_kill_sb
};

static int __init lyfs_init(void)
{
	struct vfsmount *mnt;
	int result = register_filesystem(&lyfs_type);
	printk("lyfs regist filesystem, result=%d!\n", result);
    return 0;
}

static void __exit lyfs_exit(void)  
{
	int result = unregister_filesystem(&lyfs_type);
	printk("lyfs unregist filesystem, result = %d!\n", result);

	if (fd)
		filp_close(fd, NULL);
}

module_init(lyfs_init)
module_exit(lyfs_exit)