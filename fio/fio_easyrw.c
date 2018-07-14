#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>

#include "fio_easyrw.h"

void fio_easyrw_init(struct fio_easyrw *erw,
	const char *path, const uint64_t limit, const unsigned int nofollow)
{
	erw->path = path;
	erw->limit = limit;
	erw->nofollow = !!nofollow;

	erw->fd = -1;

	erw->out = NULL;
	erw->out_len = 0;
	erw->out_max = 0;
}

void *fio_easyrw_get_out(struct fio_easyrw *erw)
{
	return erw->out;
}

unsigned int fio_easyrw_get_out_len(struct fio_easyrw *erw)
{
	return erw->out_len;
}


static int fio_easyrw_alloc_out(struct fio_easyrw *erw, const unsigned int len)
{
	void *buf;

	assert(erw->out == NULL);

	buf = malloc(len);
	if (!buf)
	{
		return -1;
	}

	erw->out = buf;
	erw->out_len = 0;
	erw->out_max = len;

	return 0;
}

static void fio_easyrw_free_out(struct fio_easyrw *erw)
{
	if (erw->out)
	{
		free(erw->out);
		erw->out = NULL;
		erw->out_len = 0;
		erw->out_max = 0;
	}
}

void fio_easyrw_exit(struct fio_easyrw *erw)
{
	fio_easyrw_free_out(erw);
}

static fio_easyrw_errnum_t __fio_easyrw_read_simple(struct fio_easyrw *erw, const int fd)
{
	assert(erw->out_max > 0 && erw->out != NULL);

	{
		ssize_t res;
		unsigned int accl = 0;
		unsigned int p_unused = erw->out_max;
		uint8_t *p = (uint8_t *) erw->out;

		uint8_t tmp[64];

		while (1)
		{
			res = read(fd, p, p_unused);
			if (res == 0)
			{
				if (p_unused == 0)
				{
					return FIO_EASYRW_ERRNUM_OK;
				}

				break; // EOF
			}

			if (res > 0)
			{
				accl += res;

				p += res;
				p_unused -= res;

				if (accl == erw->out_max && read(fd, tmp, sizeof(tmp)) == 0 /* Make sure all data is read. */)
				{
					erw->out_len = erw->out_max;
				}
			}
			else
			{
				return FIO_EASYRW_ERRNUM_IO; // IO exception.
			}
		} // end loop
	}

	return FIO_EASYRW_ERRNUM_MISC;
}

/**
 * @note Read all data into output.
 */
fio_easyrw_errnum_t fio_easyrw_read_simple(struct fio_easyrw *erw)
{
	fio_easyrw_free_out(erw);

	if (!erw->path)
	{
		return FIO_EASYRW_ERRNUM_INVAL;
	}

	/*
	 * Prepare output
	 */
	{
		struct stat st;

		if (erw->nofollow)
		{
			if (lstat(erw->path, &st))
			{
				return FIO_EASYRW_ERRNUM_OPEN;
			}
		}
		else
		{
			if (stat(erw->path, &st))
			{
				return FIO_EASYRW_ERRNUM_OPEN;
			}
		}

		if (erw->limit && erw->limit < st.st_size)
		{
			return FIO_EASYRW_RRRNUM_OVERSZ;
		}

		if (fio_easyrw_alloc_out(erw, (unsigned int) st.st_size))
		{
			return FIO_EASYRW_ERRNUM_NOMEM;
		}
	}

	/*
	 * Read file to output
	 */
	{
		fio_easyrw_errnum_t errnum;
		int fd;
		int open_flags;

		open_flags = O_RDONLY;
		open_flags |= (erw->nofollow) ? O_NOFOLLOW : 0;

		fd = open(erw->path, open_flags);
		if (fd < 0)
		{
			fio_easyrw_free_out(erw);
			return FIO_EASYRW_ERRNUM_OPEN;
		}

		errnum = __fio_easyrw_read_simple(erw, fd);
		if (errnum != FIO_EASYRW_ERRNUM_OK)
		{
			fio_easyrw_free_out(erw);
			close(fd);
			return errnum;
		}

		close(fd);
	}

	/*
	 * The output buffer is kept for caller to use.
	 */

	return FIO_EASYRW_ERRNUM_OK; // OK
}

static fio_easyrw_errnum_t __fio_easyrw_read(struct fio_easyrw *erw, fio_easyrw_read_func_t func, void *priv, const int fd)
{
	assert(erw->out_max > 0 && erw->out != NULL);
	assert(func != NULL);

	{
		ssize_t res;

		uint64_t accl = 0;

		while (1)
		{
			res = read(fd, erw->out, erw->out_max);
			if (res == 0)
			{
				break; // EOF
			}

			if (res > 0)
			{
				int caller_ret;

				accl += res;
				if (erw->limit && accl > erw->limit)
				{
					return FIO_EASYRW_RRRNUM_OVERSZ;
				}

				erw->out_len = res;
				caller_ret = func(erw, priv);
				if (caller_ret)
				{
					return FIO_EASYRW_ERRNUM_MISC; // Caller stops.
				}
			}
			else
			{
				return FIO_EASYRW_ERRNUM_IO; // IO exception.
			}
		} // end loop
	}

	return FIO_EASYRW_ERRNUM_OK;
}
/**
 * @brief Read data block by block. To read a large file, use this method.
 */
fio_easyrw_errnum_t fio_easyrw_read(struct fio_easyrw *erw, fio_easyrw_read_func_t func, void *priv)
{
	fio_easyrw_free_out(erw);

	if (!erw->path)
	{
		return FIO_EASYRW_ERRNUM_INVAL;
	}

	/*
	 * Prepare output
	 */
	{
		struct stat st;

		if (erw->nofollow)
		{
			if (lstat(erw->path, &st))
			{
				return FIO_EASYRW_ERRNUM_OPEN;
			}
		}
		else
		{
			if (stat(erw->path, &st))
			{
				return FIO_EASYRW_ERRNUM_OPEN;
			}
		}

		if (erw->limit && erw->limit < st.st_size)
		{
			return FIO_EASYRW_RRRNUM_OVERSZ;
		}

		if (fio_easyrw_alloc_out(erw, 8192 /* This is the most effecient buf size in linux */))
		{
			return FIO_EASYRW_ERRNUM_NOMEM;
		}
	}

	/*
	 * Read file to output
	 */
	{
		fio_easyrw_errnum_t errnum;
		int fd;
		int open_flags;

		open_flags = O_RDONLY;
		open_flags |= (erw->nofollow) ? O_NOFOLLOW : 0;

		fd = open(erw->path, open_flags);
		if (fd < 0)
		{
			fio_easyrw_free_out(erw);
			return FIO_EASYRW_ERRNUM_OPEN;
		}

		errnum = __fio_easyrw_read(erw, func, priv, fd);
		if (errnum != FIO_EASYRW_ERRNUM_OK)
		{
			fio_easyrw_free_out(erw);
			close(fd);
			return errnum;
		}

		close(fd);
	}

	fio_easyrw_free_out(erw); // output buffer is now useless.

	return FIO_EASYRW_ERRNUM_OK; // OK
}

#if 0
static int cb_func(struct fio_easyrw *erw, void *unused)
{

	static uint64_t accl = 0;

	accl += fio_easyrw_get_out_len(erw);

	printf("+%u, accl=%lu\n", fio_easyrw_get_out_len(erw), accl);

	return 0;
}

int main(int argc, char **argv)
{
	char *path = argv[1];

	{
		struct fio_easyrw erw;
		fio_easyrw_errnum_t errnum;

		fio_easyrw_init(&erw, path, 0, 1);

		/*
		 * test 1
		 */
		errnum = fio_easyrw_read_simple(&erw);
		if (errnum)
		{
			printf("...errnum %d %s\n", errnum, strerror(errno));
		}
		else
		{
			printf("+%u\n", fio_easyrw_get_out_len(&erw));
		}

		/*
		 * test 2
		 */
		errnum = fio_easyrw_read(&erw, cb_func, NULL);
		if (errnum)
		{
			printf("...errnum %d %s\n", errnum, strerror(errno));
		}

		fio_easyrw_exit(&erw);
	}

	return 0;
}
#endif
