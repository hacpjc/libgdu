/*!
 * \file ringbuf.c
 * \brief Implementation of ring buffer data structure.
 *
 * \date create: 2011/8/25
 * \author hac <Ping-Jhih Chen>
 * \sa ringbuf.h
 */


/* generic */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>

/* misc */
#include <getopt.h>

/* UNIX system call */
#include <sys/types.h>
#include <sys/stat.h>

#include "ringbuf.h"


/*!
 * \brief Reset/clean the ring content
 * \param ring The ring buffer to clean
 */
extern void ringbuf_reset(ringbuf_t *ring)
{
	if (ring == NULL) return;

	if (ring->container != NULL) {
		memset(ring->container, 0x00, ring->size);
	}

	ring->needle = 0;
}

static void __buffer_init(ringbuf_t *ring, const int size)
{
	if (ring == NULL) return;

	/* release old resource */
	if (ring->container_buf != NULL) free(ring->container_buf);
	if (ring->container != NULL) free(ring->container);

	/* allocate new ring */
	ring->container_buf = malloc(size + 1); // add one more byte for '\0' padding
	assert(ring->container_buf != NULL);
	memset(ring->container_buf, 0x00, size + 1);

	ring->container = malloc(size + 1);
	assert(ring->container != NULL);
	memset(ring->container, 0x00, size + 1);

	/* reset offset counter */
	ring->size = size;
	ring->needle = 0;
}

/*!
 * \brief Create a new ring buffer and initialize it.
 * \param size The max size of created ring buffer
 * \return A pointer to created ring buffer
 * \return NULL if error
 */
extern ringbuf_t *ringbuf_create(const int size)
{
	ringbuf_t *ring;

	ring = (ringbuf_t *) malloc(sizeof(*ring));
	assert(ring != NULL);

	memset(ring, 0x00, sizeof(*ring));

	__buffer_init(ring, size);

	return ring;
}

/*!
 * \brief Destroy a ring
 * \param ring The ring buffer to destroy
 * \sa ringbuf_create
 */
extern void ringbuf_destroy(ringbuf_t *ring)
{
	if (ring == NULL) return;

	if (ring->container_buf != NULL) free(ring->container_buf);
	if (ring->container != NULL) free(ring->container);

	return;
}

/*!
 * \brief Save input into the ring.
 * \param ring Input ring buffer
 * \param input New content to saved in ring buffer
 * \param input_len New content length (bytes)
 * \return >= 0 if ok
 * \return < 0 if error
 * \note Automatically rotate the ring iff. buffer overflow.
 */
extern int ringbuf_input(ringbuf_t *ring, const char *input, int input_len)
{
	int delete_len = 0;

	assert(ring != NULL);

	if (input_len >= ring->size) {
		/*
		 * input overflow, copy newer input to container
		 * (* WARNING: older input is lost)
		 */
		memcpy(ring->container, input + (input_len - ring->size), ring->size);

		ring->needle = ring->size;
	} else if (ring->needle + input_len > ring->size) {
		/*
		 * ring overflow, delete older container
		 */
		delete_len = (ring->needle + input_len) - ring->size;
		assert(delete_len > 0);

		/* copy newer content to buffer */
		memset(ring->container_buf, 0x00, ring->size);
		memcpy(ring->container_buf, ring->container + delete_len, ring->needle - delete_len);

		/* clean ring */
		memset(ring->container, 0x00, ring->size);

		/* copy buffer content to ring container */
		memcpy(ring->container, ring->container_buf, ring->needle - delete_len);
		memcpy(ring->container + (ring->needle - delete_len), input, input_len);

		ring->needle = (ring->needle + input_len) - delete_len;
	} else {
		/*
		 * append new content to ring container directly
		 */
		memcpy(ring->container + ring->needle, input, input_len);

		ring->needle += input_len;
	}

	return input_len;
}

/*!
 * \brief Rotate the ring buffer, i.e. delete older ring container & re-arrange the ring
 * \param ring Input ring buffer
 * \param rotate_len Rotate length (bytes)
 * \return Current used ring buffer size (i.e. needle offset)
 */
extern int ringbuf_rotate(ringbuf_t *ring, int rotate_len)
{
	assert(ring != NULL);

	if (rotate_len >= ring->needle) {
		/*
		 * delete all container
		 */
		memset(ring->container, 0x00, ring->size);
		ring->needle = 0;
	} else {
		/* move reserved container to buffer */
		memcpy(ring->container_buf, ring->container + rotate_len, ring->needle - rotate_len);

		/* clean container */
		memset(ring->container, 0x00, ring->size);

		/* copy buffer to container */
		memcpy(ring->container, ring->container_buf, ring->needle - rotate_len);

		ring->needle -= rotate_len;
	}

	return ring->needle;
}

/*!
 * \brief Get current ring needle position i.e. current content size
 * \param ring Input ring buffer
 * \return The offset of ring needle
 * \note The offset is equal to current saved content size (bytes) 
 */
extern int ringbuf_get_needle(ringbuf_t *ring)
{
	assert(ring != NULL);

	return ring->needle;
}

/*!
 * \brief Get max size (bytes) of input ring buffer
 * \param ring Input ring buffer
 * \return Max ring buffer size (bytes)
 * \note Max size != currently used size
 */
extern int ringbuf_get_size(ringbuf_t *ring)
{
	assert(ring != NULL);

	return ring->size;
}

/*!
 * \brief Get the ring buffer
 * \param ring Ring buffer to fetch its container
 * \return A pointer to the container
 * \return NULL, otherwise
 */
extern void *ringbuf_get_container(ringbuf_t *ring)
{
	assert(ring != NULL);

	return (void *) ring->container;
}
