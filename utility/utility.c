
#include <linux/fs.h>
#include <linux/types.h>

#include <linux/uaccess.h>

void gigabyte_suffix(char buffer[], unsigned long size)
{
	static const char * const suffix[] = {"B", "KB", "MB", "GB"};
	unsigned long value = size;
	int i;
	const char **suf = (const char **)suffix;
	for (i = 0; i < sizeof(suffix); i++) {
		if (value < 1024) {
			break;
		}
		suf ++;
		value = value / 1024;
	}
	sprintf(buffer, "%ld%s", value, *suf);
}


int lyfs_file_read(struct file *file, void *buf, loff_t offset, size_t count)
{
	mm_segment_t fs = get_fs();
	size_t readed_size = 0;
	loff_t curr_pos = offset;
	set_fs(KERNEL_DS);
	while (readed_size < count)
		readed_size += vfs_read(file, buf, count - readed_size, &curr_pos);
    set_fs(fs);
	return 0;
}


int lyfs_file_write(struct file *file, void *buf, loff_t offset, size_t count)
{
	mm_segment_t fs = get_fs();
	size_t writted_size = 0;
	loff_t curr_pos = offset;
	set_fs(KERNEL_DS);
	while (writted_size < count)
		writted_size += vfs_write(file, buf, count - writted_size, &curr_pos);
    set_fs(fs);
	return 0;
}

int empty_block_index(char *bitmap, size_t size)
{
	int i;
	for (i = 1; i < size; i ++) {
		if (bitmap[i] == 0) {
			return i;
		}
	}
	return 0;
}