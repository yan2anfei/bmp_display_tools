#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "bmp.h"
#include "debug.h"

struct bmp_handle *load_bmp(char *path)
{
	int fd;
	off_t off;
	struct bmp_handle *handle = NULL;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		E("load bmp failed,errno=%d(%s)\n", errno, strerror(errno));
		goto err;
	}

	handle = malloc(sizeof(*handle));
	if (!handle) {
		E("malloc bmp handle failed.\n");
		goto err_malloc;
	}
	memset(handle, 0, sizeof(*handle));

	handle->fd = fd;
	off = lseek(handle->fd, 0, SEEK_END);
	if (off <= 0) {
		E("seek bmp failed,errno=%d(%s)\n", errno, strerror(errno));
		goto err_seek;
	}
	handle->size = off;
	lseek(handle->fd, 0, SEEK_SET);

	handle->hdr = mmap(0, off, PROT_READ, MAP_PRIVATE, handle->fd, 0);
	if (!handle->hdr) {
		E("mmap bmp failed,errno=%d(%s)\n", errno, strerror(errno));
		goto err_mmap;
	}

	return handle;
err_mmap:
err_seek:
	free(handle);
	handle = NULL;
err_malloc:
	close(fd);
	fd = -1;
err:
	return NULL;
}

void free_bmp(struct bmp_handle *handle)
{
	munmap(handle->hdr, handle->size);
	handle->hdr = NULL;
	close(handle->fd);
	handle->fd = -1;
	free(handle);
}
