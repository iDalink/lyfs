#include "prefix.h"



static ssize_t lyfs_file_operations_read(struct file *filp, char __user *buf, size_t siz, loff_t *ppos)
{
	printk("lyfs read! siz=%d ppos=%x\n f_pos=%d \n", (int)siz, ppos, filp->f_pos);

	struct lyfs_meta_node node;
	lyfs_file_read(fd, &node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + filp->f_inode->i_ino * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));

	if (node.type == LYFS_TYPE_DENTRY) {
		return -EISDIR;
	}

	size_t file_size = 0;
	if (node.data_block_index > 0) {
		struct lyfs_data_block data_block;
		lyfs_file_read(fd, &data_block, (sb_on_drive.data_area_index + node.data_block_index) * LYFS_BLOCK_SIZE, sizeof(struct lyfs_data_block));
		file_size = data_block.file_size;
	} else {
		printk("lyfs the file has no data block!\n");
		// None block yet.
		return 0;
	}

	int read_size = min(file_size - *ppos, siz);

	void *buffer = kmalloc(min(read_size, LYFS_BLOCK_SIZE) + 100, GFP_KERNEL);
	
	int block_index = node.data_block_index;
	size_t readed_size = *ppos;

	printk("lyfs read file_size=%d, block_index=%d, *ppos=%d!\n", file_size, block_index, *ppos);

	while (readed_size < read_size + *ppos) {
		int find_valid_data = 1;
		size_t block_offset;
		if (block_index == node.data_block_index) {
			// Is first block.
			block_offset = sizeof(struct lyfs_data_block);
		} else {
			block_offset = 0;
		}
		size_t block_full_size = LYFS_BLOCK_SIZE - block_offset - 4;

		size_t block_offset_begin = 0;
		size_t block_offset_end = 0;


		if (readed_size + block_full_size < *ppos) {
			// before effect segment, do nothing
			find_valid_data = 0;
		} else {
			if (readed_size + block_full_size <= *ppos + read_size) {
				// right contain
				if (readed_size < *ppos) {
					// left expand
					block_offset_begin = readed_size - *ppos;
				} else {
					// full
				}
			} else {
				block_offset_end = - readed_size + block_full_size - *ppos - read_size ;
				// right expand
				if (readed_size > *ppos + read_size) {
					// tailing effect sesgment, dothing
					find_valid_data = 0;
				} else {
					if (readed_size < *ppos) {
						block_offset_begin = *ppos - readed_size;
					} else {
						// left contain do nothing
					}
				}
			}
		}

		size_t current_read_size = block_full_size - block_offset_begin - block_offset_end;

		printk("lyfs read block_full_size=%d, begin=%d, end=%d\n", block_full_size, block_offset_begin, block_offset_end);
			
		lyfs_file_read(fd, buffer, (sb_on_drive.data_area_index + block_index) * LYFS_BLOCK_SIZE + block_offset + block_offset_begin, current_read_size + sizeof(int));

		if (find_valid_data) {
			copy_to_user(buf + readed_size - *ppos, buffer, current_read_size);
			printk("lyfs read find valid data!\n");
		}

		printk("lyfs read readed_size=%d, current_read_size=%d, buffer=%s\n", readed_size, current_read_size, buffer);
		
		//if (1 == 1) return 0;
		readed_size += block_full_size - block_offset_end;

		block_index = *((int *)(buffer + current_read_size));
	}

	kfree(buffer);

	*ppos = readed_size;

	printk("lyfs read finish ppos=%d, read_size=%d\n", *ppos, read_size);

	return read_size;
}

static ssize_t lyfs_file_operations_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	size_t destination_size = *ppos + count;
	printk("lyfs write! siz=%d ppos=%x f_pos=%d \n", (int)destination_size, ppos);

	struct lyfs_meta_node node;
	lyfs_file_read(fd, &node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + file->f_inode->i_ino * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));

	if (node.type == LYFS_TYPE_DENTRY) {
		return -EISDIR;
	}

	char *block_bitmap = (char *)kzalloc(sb_on_drive.data_area_count, GFP_KERNEL);
	lyfs_file_read(fd, block_bitmap, sb_on_drive.data_bitmap_index * LYFS_BLOCK_SIZE, sb_on_drive.data_area_count);

	size_t file_size = 0;
	struct lyfs_data_block data_block;
	if (node.data_block_index > 0) {
		lyfs_file_read(fd, &data_block, (sb_on_drive.data_area_index + node.data_block_index) * LYFS_BLOCK_SIZE, sizeof(struct lyfs_data_block));
		file_size = data_block.file_size;
	} 

	{
		int current_block_index = node.data_block_index;
		int seeked_size = 0;

		size_t vad_size = max(file_size, destination_size);

		int previous_block_index = 0;

		while (seeked_size < vad_size) {
			if (seeked_size < file_size) {
				// current block already exists
				if (seeked_size < destination_size) {
					// current block should remain keep
					goto jump_to_next_block;
				} else {
					// current block should remove
					goto delete_curr_node;
				}
			} else {
				// shoud insert new data block
				goto insert_new_node;
			}

			insert_new_node:
			{
				int new_block_index = empty_block_index(block_bitmap, sb_on_drive.data_area_count);
				if (new_block_index == 0) {
					return -ENOSPC;
				}
				block_bitmap[new_block_index] = 1;
				printk("lyfs writed new_block_index=%d!\n", new_block_index);

				current_block_index = new_block_index;

				if (previous_block_index == 0){
					// insert first block
					node.data_block_index = current_block_index;
					printk("lyfs writed dentry first has block!\n");

					struct lyfs_data_block data_block = 
					{
						.file_size = destination_size, 
						.link_count = 0
					};

				    lyfs_file_write(fd, &data_block, \
				    	(sb_on_drive.data_area_index  + current_block_index) * LYFS_BLOCK_SIZE,
				    	 sizeof(struct lyfs_data_block));
				} else {
					// insert other block, modify previous block
					int index = current_block_index;

				    lyfs_file_write(fd, &index, \
				    	(sb_on_drive.data_area_index  + previous_block_index + 1) * LYFS_BLOCK_SIZE - 4,
				    	 sizeof(int));
				}

				goto jump_to_next_block;
			}

			delete_curr_node:
			{
				block_bitmap[current_block_index] = 0;
				goto jump_to_next_block;
			}

			jump_to_next_block:
			{
				size_t block_offset;
				if (previous_block_index == 0) {
					// Is first block.
					block_offset = sizeof(struct lyfs_data_block);
				} else {
					block_offset = 0;
				}
				size_t block_full_size = LYFS_BLOCK_SIZE - block_offset - 4;

				seeked_size += block_full_size;

				previous_block_index = current_block_index;
				{
					lyfs_file_read(fd, &current_block_index, (sb_on_drive.data_area_index + current_block_index + 1) * LYFS_BLOCK_SIZE - sizeof(int), sizeof(current_block_index));
				}

				continue;
			}
		}

		lyfs_file_write(fd, block_bitmap, \
			sb_on_drive.data_bitmap_index * LYFS_BLOCK_SIZE,
			sb_on_drive.data_area_count);

		lyfs_file_write(fd, &node, \
			sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + node.node_index * sizeof(struct lyfs_meta_node),
			sizeof(struct lyfs_meta_node));
		printk("lyfs write node = %d, block=%d!\n", node.node_index, node.data_block_index);
	}

	{
		int write_size = count;
		
		int block_index = node.data_block_index;
		size_t writed_size = *ppos;

		printk("lyfs write writed_size=%d, block_index=%d, *ppos=%d!\n", writed_size, block_index, *ppos);

		while (writed_size < write_size + *ppos) {
			int find_valid_data = 1;
			size_t block_offset;
			if (block_index == node.data_block_index) {
				// Is first block.
				block_offset = sizeof(struct lyfs_data_block);
			} else {
				block_offset = 0;
			}
			size_t block_full_size = LYFS_BLOCK_SIZE - block_offset - 4;

			size_t block_offset_begin = 0;
			size_t block_offset_end = 0;


			if (writed_size + block_full_size < *ppos) {
				// before effect segment, do nothing
				find_valid_data = 0;
			} else {
				if (writed_size + block_full_size <= *ppos + write_size) {
					// right contain
					if (writed_size < *ppos) {
						// left expand
						block_offset_begin = writed_size - *ppos;
					} else {
						// full
					}
				} else {
					block_offset_end = - writed_size + block_full_size - *ppos - write_size ;
					// right expand
					if (writed_size > *ppos + write_size) {
						// tailing effect sesgment, dothing
						find_valid_data = 0;
					} else {
						if (writed_size < *ppos) {
							block_offset_begin = *ppos - writed_size;
						} else {
							// left contain do nothing
						}
					}
				}
			}

			size_t current_write_size = block_full_size - block_offset_begin - block_offset_end;

			printk("lyfs write block_full_size=%d, begin=%d, end=%d\n", block_full_size, block_offset_begin, block_offset_end);
				
			int i_node;
			lyfs_file_read(fd, &i_node, (sb_on_drive.data_area_index + block_index + 1) * LYFS_BLOCK_SIZE - 4, sizeof(int));

			if (find_valid_data) {
				lyfs_file_write(fd, buf + writed_size - *ppos, \
					(sb_on_drive.data_area_index + block_index) * LYFS_BLOCK_SIZE + block_offset + block_offset_begin,
					current_write_size);
				printk("lyfs write valid data!\n");
			}

			printk("lyfs write readed_size=%d, current_write_size=%d\n", writed_size, current_write_size);
			
			//if (1 == 1) return 0;
			writed_size += block_full_size - block_offset_end;

			block_index = i_node;
		}

	}

	printk("lyfs writed %d bytes data!\n", count);

	return count;
}

static int lyfs_file_operations_iterate(struct file *file, struct dir_context *ctx)
{
	struct lyfs_meta_node node;
	int i = 0;
	int index = 0;
	printk("lyfs file iterate! ctx=%x, ctx->pos=%ld, inode=%d\n", ctx, ctx->pos, file->f_inode->i_ino);

	lyfs_file_read(fd, &node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + file->f_inode->i_ino * sizeof(struct lyfs_meta_node), sizeof(struct lyfs_meta_node));

	for ( i = 0; i < LYFS_DENTRY_CHILD_LIMIT_COUNT; i ++) {

		struct lyfs_meta_node child_node;

		int child_node_number = node.child_meta_indexs[i];
		
		if (child_node_number != 0)  {

			if (index == ctx->pos) {

				lyfs_file_read(fd, &child_node, sb_on_drive.meta_area_index * LYFS_BLOCK_SIZE + child_node_number * sizeof(struct lyfs_meta_node), sizeof(child_node));
				//printk("lyfs child_node_number = %d, indentifer = %s, name=%s!\n", child_node_number, child_node.identifier, child_node.name);
				
				dir_emit(ctx, child_node.name, strlen(child_node.name), child_node.node_index, (child_node.type == LYFS_TYPE_DENTRY) ? DT_DIR : DT_REG);
			}
			index ++;
			ctx->pos++;
		}
	}
	return 0;
}

struct file_operations lyfs_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= lyfs_file_operations_read,
	.write 		= lyfs_file_operations_write,
	.iterate	= lyfs_file_operations_iterate,
	.fsync		= generic_file_fsync,
};