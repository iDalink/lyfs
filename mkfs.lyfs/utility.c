#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdarg.h>

char *gigabyte_suffix(unsigned long size, char buffer[])
{
	static const char * const suffix[] = {"B", "KB", "MB", "GB"};
	double value = size;
	const char **suf = (const char **)suffix;
	for (int i = 0; i < sizeof(suffix); i++) {
		if (value < 1024) {
			break;
		}
		suf ++;
		value = value / 1024;
	}
	sprintf(buffer, "%.2f%s", value, *suf);
	return buffer;
}