
void gigabyte_suffix(char buffer[], unsigned long size);

int lyfs_file_read(struct file *file, void *buf, loff_t offset, size_t count);
int lyfs_file_write(struct file *file, void *buf, loff_t offset, size_t count);

int empty_block_index(char *bitmap, size_t size);