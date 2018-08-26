/**
 * @file semlock.c
 * @brief semaphore lock for multi-process concurrency
 *
 * @date Create: Oct 4, 2012
 * @author hac Ping-Jhih Chen
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include <errno.h>
#include <stdint.h>

#include <getopt.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "semlock.h"

#define SEMLOCK_DBG(_fmt, args...) do { } while (0)

/*!
 * \brief Dump sem lock structure.
 * \param sl Input sem lock
 * \internal
 */
extern void semlock_dump(semlock_t *sl)
{
	SEMLOCK_DBG("sem id %d with max reader %d (no = %d)\n", sl->sem_id, sl->sem_maxval, sl->sem_no);
}

/*!
 * \brief Set the max val of sem value which equals to the max concurrent "reader" of this lock.
 * \param sl Input sem lock
 * \param maxval Max sem value.
 */
extern void semlock_set_maxval(semlock_t *sl, const int maxval)
{
	sl->sem_maxval = maxval;
	SEMLOCK_DBG("set sem max val %d\n", sl->sem_maxval);
}

extern short int semlock_get_maxval(semlock_t *sl)
{
	return sl->sem_maxval;
}


/*!
 * \brief Init the semlock_t structure.
 * \note Run this (exactly) one time before attaching the semlock.
 * \param sl Input sem lock to init.
 * \sa semlock_attach
 * \sa semlock_detach
 * \sa SEMLOCK_INITIALIZER
 */
extern int semlock_init(semlock_t *sl)
{
	sl->sem_id = -1;
	sl->sem_no = 1; // * FIXME: harden code: normally, 1 semaphore is enough.
	semlock_set_maxval(sl, SEMLOCK_DEFAULT_MAXVAL);
	return 0;
}

/*!
 * \todo Fixme.
 */
extern int semlock_detach(semlock_t *sl)
{
	// * FIXME: decrease attached sem no by 1. Delete the sem if we reach 0.

	return 0;
}

/*!
 * \brief Remove the sem lock.
 * \param sl Input sem lock to destroy (delete)
 * \details All sem waiting for the resource will be unblocked and get an errno EIDRM.
 */
extern int semlock_destroy(semlock_t *sl)
{
	/*
	 * Immediately  remove  the semaphore set, awakening all processes blocked in semop(2) calls on the set (with an
	 * error return and errno set to EIDRM).  The effective user ID of the calling process must match the creator or
	 * owner of the semaphore set, or the caller must be privileged.  The argument semnum is ignored.
	 */
	if (sl->sem_id >= 0)
	{
		SEMLOCK_DBG("detach sem id %d\n", sl->sem_id);
		semctl(sl->sem_id, 0, IPC_RMID);
		sl->sem_id = -1;
	}

	return 0;
}

/*!
 * \brief Init semaphor lock structure.
 * \param sl Input sem lock
 * \param path An existed path to generate unique sem key.
 * \param key_seed A value to generate unique sem key if there's collision in path
 * \sa semlock_detach
 */
extern int semlock_attach(semlock_t *sl, const char *path, const int key_seed)
{
	key_t sem_key;
	struct semid_ds sem_ds;
	struct sembuf sem_buf;

	sem_key = ftok(path, key_seed);

	if (sem_key < 0)
	{
		return -1; // Cannot get unique sem key
	}

	sl->sem_id = semget(sem_key, sl->sem_no,
		IPC_CREAT | IPC_EXCL | (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

	if (sl->sem_id < 0 && errno == EEXIST)
	{
		/*
		 * In case, the sem is already created by somebody.
		 */
		sl->sem_id = semget(sem_key, sl->sem_no, 0);

		if (sl->sem_id < 0)
		{
			return -1; // Sorry. Check errno.
		}

		SEMLOCK_DBG("attach existed sem with sem id %d\n", sl->sem_id);

#if 1
		/* Wait for the creator to init sem */
		for (;;)
		{
			semctl(sl->sem_id, sl->sem_no - 1, IPC_STAT, &sem_ds);

			if (sem_ds.sem_otime != 0)
			{
				break;
			}
			else
			{
#if 1
				SEMLOCK_DBG("wait sem id %d\n", sl->sem_id);
				// * FIXME: How to do polling for this.
				usleep(1000); // * Warning. It's only safe to sleep in user engine.
#endif
			}
		} // end for
#endif
	}
	else if (sl->sem_id >= 0)
	{
		/*
		 * In case, this process is the creator.
		 */
		SEMLOCK_DBG("create sem id %d with max val %d\n", sl->sem_id, sl->sem_maxval);

		sem_buf.sem_op = sl->sem_maxval;
		sem_buf.sem_flg = 0;

		/* Init every sem in this id */
		for (sem_buf.sem_num = 0; sem_buf.sem_num < sl->sem_no; sem_buf.sem_num++)
		{
			if (semop(sl->sem_id, &sem_buf, 1) < 0)
			{
				semlock_detach(sl);
				return -1;
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

/* Read lock */

/*!
 * \brief Lock a r lock
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 */
extern int semlock_lock_r(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = -1;
	sem_buf.sem_flg = SEM_UNDO;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot lock (r) sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("r lock ok\n");

	return 0;
}

/*!
 * \brief Lock a r lock, but return immediately if the lock is not available (i.e. nonblocked).
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 */
extern int semlock_trylock_r(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = -1;
	sem_buf.sem_flg = SEM_UNDO | IPC_NOWAIT;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot lock (r) sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("try r lock ok\n");

	return 0;
}


/*!
 * \brief Unlcok a r lock.
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 * \sa semlock_trylock_r
 * \sa semlock_lock_r
 */
extern int semlock_unlock_r(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = 1; // add back
	sem_buf.sem_flg = SEM_UNDO;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot unlock sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("r unlock ok\n");

	return 0;
}

/* Write Lock */

/*!
 * \brief Lock a w lock, but return immediately if the lock is not available (i.e. nonblocked).
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 */
extern int semlock_trylock_w(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = (0 - sl->sem_maxval);
	sem_buf.sem_flg = SEM_UNDO | IPC_NOWAIT;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot lock (w) sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("try w lock ok\n");

	return 0;
}

/*!
 * \brief Lock a w lock
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 * \sa semlock_unlock_w
 */
extern int semlock_lock_w(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = (0 - sl->sem_maxval);
	sem_buf.sem_flg = SEM_UNDO;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot lock (w) sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("w lock ok");

	return 0;
}

/*!
 * \brief Unlock a w lock
 * \param sl Input sem lock
 * \return 0 if ok
 * \return < 0, if there's error
 * \sa semlock_lock_w
 */
extern int semlock_unlock_w(semlock_t *sl)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = 0;
	sem_buf.sem_op = (sl->sem_maxval);
	sem_buf.sem_flg = SEM_UNDO;

	if (semop(sl->sem_id, &sem_buf, 1) < 0)
	{
		SEMLOCK_DBG("cannot unlock sem %d %s\n", sl->sem_id, strerror(errno));
		return -1;
	}

	SEMLOCK_DBG("w unlock ok");

	return 0;
}


