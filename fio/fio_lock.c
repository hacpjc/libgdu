
#include "fio_lock.h"

extern int fio_lock(const char *path)
{
	int fd;

	fd = open(path, O_RDONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
	if (fd < 0) {
		return -1;
	}

	if (flock(fd, LOCK_EX) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

extern int fio_unlock(const int fd)
{
	if (fd < 0) {
		return -1;
	}

	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}
