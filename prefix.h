#include <linux/module.h>    /* 引入与模块相关的宏 */  
#include <linux/init.h>        /* 引入module_init() module_exit()函数 */  
#include <linux/moduleparam.h>/* 引入module_param() */  
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/stat.h>
#include <linux/statfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/list_bl.h>
#include <linux/rwsem.h>
#include <linux/dcache.h>
#include <linux/cred.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <linux/security.h>
#include "utility/utility.h"
#include "lyfs_type.h"

extern struct lyfs_super_block sb_on_drive;

extern struct file_system_type lyfs_type;
extern struct inode_operations lyfs_inode_operations;
extern struct dentry_operations lyfs_dentry_operations;
extern struct file_operations lyfs_file_operations;

extern struct file *fd;