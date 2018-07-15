#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "dirtrav.h"

#define CONFIG_DEBUG (1) //!< Say 1 to print debug msg.

#define DT_ERR(fmt, args...) fprintf(stderr, " *** ERROR: [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define DT_PRT(fmt, args...) printf(fmt "\n", ##args)

#if CONFIG_DEBUG
#define DT_DBG(fmt, args...) printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##args)
#else
#define DT_DBG(fmt, args...) do { } while (0)
#endif

static inline int stat_get_st_mode(const char *path, mode_t *mode)
{
	struct stat stat_buf;

	if (stat(path, &stat_buf) < 0)
	{
		return -1;
	}

	*mode = stat_buf.st_mode;

	return 0;
}

static int __handle_reg(const char *path, dirtrav_cb_handle_t cb_handle, dirtrav_cb_filter_t cb_filter, void *arg)
{
	if (cb_filter != NULL && cb_filter(path, arg) == 0)
	{
		return 0;
	}

	if (cb_handle)
	{
		return cb_handle(path, arg);
	}

	return 0;
}

/*!
 * \todo Limit directory depth to avoid opening to many files/allocating too much memory.
 */
int dirtrav(const char *path, const dirtrav_flag_t flag,
	const dirtrav_cb_handle_t cb_handle, const dirtrav_cb_filter_t cb_filter, void *arg)
{
	DIR *dir;
	struct dirent *dirent;

	int ret = 0;
	mode_t st_mode;

	if (stat_get_st_mode(path, &st_mode) < 0)
	{
		return -1;
	}

	switch (st_mode & S_IFMT)
	{
	case S_IFDIR: // A directory
		/* Visit files recursively. */
		dir = opendir(path);
		if (dir == NULL)
		{
			DT_ERR("Cannot open dir '%s' %s", path, strerror(errno));
			return -1;
		}

		do
		{
			char *next_path = NULL;

			dirent = readdir(dir);
			if (dirent == NULL)
			{
				break;
			}

			if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
			{
				continue;
			}

			asprintf(&next_path, "%s/%s", path, dirent->d_name);
			if (next_path == NULL)
			{
				DT_ERR("Cannot append path string '%s' with '%s' (%s)",
					dirent->d_name, path, strerror(errno));
				ret = -1;
			}
			else
			{
				ret += dirtrav(next_path, flag, cb_handle, cb_filter, arg);

				free(next_path);
				next_path = NULL;
			}

			if (ret < 0 && (DIRTRAV_FLAG_CHECK(flag, DIRTRAV_FLAG_STOP_AT_ERROR)))
			{
				break;
			}
		} while (1);

		closedir(dir);

		return ret;

		break;
	case S_IFREG: // A regular file
		return __handle_reg(path, cb_handle, cb_filter, arg);
	case S_IFLNK:
		/*! \todo Follow link. readlink(2) */
		DT_DBG("Cannot follow link. (FIXME)");
		break;
	default:
		DT_DBG("Bypass file type with st_mode %d", st_mode & S_IFMT);
		break;
	}

	return 0;
}
