/*!
 * \file ringbuf.h
 * \brief Implementation of ring buffer data structure.
 *
 * \date create: 2011/8/25
 * \author hac <Ping-Jhih Chen>
 *
 * \details
 * \todo Write sample codes plz.
 */

#ifndef RINGBUF_H_
#define RINGBUF_H_

/*!
 * \brief Ring buffer structure.
 */
typedef struct ringbuf {
	void *container; //!< saved content in ring
	void *container_buf; //!< a container buffer space to co-operate with container

	int size; //!< max available ring size
	int needle; //!< offset of the ring tail
} ringbuf_t;

extern ringbuf_t *ringbuf_create(const int size);
extern void ringbuf_reset(ringbuf_t *ring);
extern void ringbuf_destroy(ringbuf_t *ring);

extern int ringbuf_input(ringbuf_t *ring, const char *input, int input_len);
extern int ringbuf_rotate(ringbuf_t *ring, int rotate_len);
extern int ringbuf_get_needle(ringbuf_t *ring);
extern int ringbuf_get_size(ringbuf_t *ring);
extern void *ringbuf_get_container(ringbuf_t *ring);

#endif /* PMSRING_H_ */
