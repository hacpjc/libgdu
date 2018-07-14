#ifndef FIO_FIO_LOCK_H_
#define FIO_FIO_LOCK_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

extern int fio_lock(const char *path);
extern int fio_unlock(const int fd);

#endif
