
#ifndef FIFOBUF_H_
#define FIFOBUF_H_

#include "list/list.h"

/*
 * fifobuf:
 *   \    +------+    +------+        +--+---------+
 *    +-- | data | -> | data | -> ... |  |  unused |
 *        +------+    +------+        +--+---------+
 *        ^                              ^
 *        |                              |
 *     data head                     data tail (last out)
 */
typedef struct fifobuf
{
	unsigned int data_size;
	unsigned int data_used;
	unsigned int data_free;
	struct list_head list_data_fifo; //!< To save allocated data list
	struct list_head list_data_free; //!< To save temporarily free list
} fifobuf_t;


int fifobuf_extend_atomic(fifobuf_t *fb, unsigned int add_len);
int fifobuf_extend_sleep(fifobuf_t *fb, unsigned int add_len);
int fifobuf_extend_vm(fifobuf_t *fb, unsigned int add_len);
int fifobuf_extend_atomic_notrace(fifobuf_t *fb, unsigned int add_len);
int fifobuf_extend_sleep_notrace(fifobuf_t *fb, unsigned int add_len);
int fifobuf_extend_vm_notrace(fifobuf_t *fb, unsigned int add_len);

int fifobuf_enqueue_atomic(fifobuf_t *fb, void *data, unsigned int data_len);
int fifobuf_enqueue_sleep(fifobuf_t *fb, void *data, unsigned int data_len);
int fifobuf_enqueue_vm(fifobuf_t *fb, void *data, unsigned int data_len);
int fifobuf_enqueue_atomic_notrace(fifobuf_t *fb, void *data, unsigned int data_len);
int fifobuf_enqueue_sleep_notrace(fifobuf_t *fb, void *data, unsigned int data_len);
int fifobuf_enqueue_vm_notrace(fifobuf_t *fb, void *data, unsigned int data_len);

int fifobuf_enqueue_zero_padding(fifobuf_t *fb);

void fifobuf_debug(fifobuf_t *fb);
unsigned int fifobuf_dequeue(fifobuf_t *fb, uint8_t *buf, unsigned int buf_len);

#define fifobuf_init(_fb, _data_size) \
	do { \
		INIT_LIST_HEAD(&(_fb)->list_data_fifo); \
		INIT_LIST_HEAD(&(_fb)->list_data_free); \
		(_fb)->data_used = 0; \
		(_fb)->data_free = 0; \
		(_fb)->data_size = (_data_size); \
	} while (0)

#define fifobuf_get_data_size(_fb) ((_fb)->data_size)

void fifobuf_flush(fifobuf_t *fb);
void fifobuf_exit(fifobuf_t *fb);

void fifobuf_free(fifobuf_t *fb);
fifobuf_t *fifobuf_alloc_sleep(unsigned int data_size);
fifobuf_t *fifobuf_alloc_atomic(unsigned int data_size);

unsigned int fifobuf_calibrate_data_size(const unsigned int minimal);

typedef int (* fifobuf_ro_func_t)(void *data, unsigned int data_len, unsigned int offset, unsigned int total, void *priv);
int fifobuf_ro(fifobuf_t *fb, fifobuf_ro_func_t ro_func, void *priv);


#endif /* FIFOBUF_H_ */
