/**
 * @file semlock.h
 * @brief semaphore lock for multi-process concurrency
 *
 * @date Create: Oct 4, 2012
 * @author hac Ping-Jhih Chen
 */

#ifndef SEMLOCK_H_
#define SEMLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdint.h>

#include <errno.h>

typedef uint8_t semlock_flag_t;

#define SEMLOCK_FLAG_BLOCKED    0x00
#define SEMLOCK_FLAG_NONBLOCKED 0x01
#define SEMLOCK_FLAG_DEFAULT    SEMLOCK_FLAG_BLOCKED

typedef struct semlock
{
	int sem_id;
	int sem_no;
	short int sem_maxval;
} semlock_t;

#define SEMLOCK_DEFAULT_MAXVAL 4096

#define SEMLOCK_INITIALIZER {.sem_id = -1, .sem_no = 0, .sem_maxval = SEMLOCK_DEFAULT_MAXVAL}

/*
 * function prototypes
 */
extern void semlock_set_maxval(semlock_t *sl, const int maxval);
extern short int semlock_get_maxval(semlock_t *sl);
extern int semlock_init(semlock_t *sl);
extern int semlock_attach(semlock_t *sl, const char *path, const int key_seed);
extern int semlock_detach(semlock_t *sl);
extern int semlock_destroy(semlock_t *sl);

extern int semlock_trylock_r(semlock_t *sl);
extern int semlock_lock_r(semlock_t *sl);
extern int semlock_unlock_r(semlock_t *sl);

extern int semlock_trylock_w(semlock_t *sl);
extern int semlock_lock_w(semlock_t *sl);
extern int semlock_unlock_w(semlock_t *sl);

#endif /* SEMLOCK_H_ */
