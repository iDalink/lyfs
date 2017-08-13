#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

const char exits_magic[] = "4.4.0-3-deepin-amd64 SMP mod_unload modversions ";
const char goal_magic[] = "4.10.0-19-generic SMP mod_unload ";

const size_t readed_size = 2 * 1024;

int main() {
	int fd = open("lyfs.ko", O_RDWR);
	if (fd <= 0) {
		return -1;
	}
	void *mem = mmap(NULL, readed_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (mem == NULL) {
		close(fd);
		return -1;
	}
	char *data = (char *)mem;

	size_t actual_size = 0;
	if (data[readed_size - 1] || 1)
		actual_size = readed_size;
	else
		actual_size = strlen(data);

	size_t magic_index = 0;

	for (int i = 0; i < actual_size - sizeof(exits_magic); i ++) {
		for (int j = 0; j <= sizeof(exits_magic); j++) {
			if (data[i + j] != exits_magic[j]) {
				break;
			}
			if (j == (sizeof(exits_magic) - 1)) {
				// find magic string
				magic_index = i;
			}
		}
	}
	if (magic_index) {
		printf("%s\n", data + magic_index);
	}
	for (int i = 0; i < sizeof(exits_magic); i++) {
		if (i < sizeof(goal_magic))
			data[magic_index + i] = goal_magic[i];
		else
			data[magic_index + i] = 0;
	}
	munmap(mem, readed_size);
	close(fd);
	return (magic_index == 0) ? -1 : 0;
}