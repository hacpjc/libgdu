#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fifobuf.h"

#define HAVE_DEBUG_MSG (0) //!< Say 1 to debug

#if HAVE_DEBUG_MSG
#define DBG(fmt, args...) printf("[%s#%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#define ERR(fmt, args...) printf(" *** ERROR [%s#%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#else
#define DBG(fmt, args...)
#define ERR(fmt, args...) printf(" *** ERROR [%s#%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#endif

#define MY_KMALLOC_SLEEP malloc
#define MY_KMALLOC_SLEEP_NOTRACE malloc
#define MY_KMALLOC_ATOMIC malloc
#define MY_KMALLOC_ATOMIC_NOTRACE malloc
#define MY_KFREE free
#define MY_KFREE_NOTRACE free

#define MY_VMALLOC malloc
#define MY_VMALLOC_NOTRACE malloc
#define MY_VFREE free
#define MY_VFREE_NOTRACE free

/*
 *   +----- alloc len --------------+
 *  /                                \
 * v                                  v
 * +-----+---------------------+------+
 * | hdr | data                | tail |
 * +-----+---------------------+------+
 *       ^                     ^
 *       |                     |
 *       real data start     real data end
 *
 *
 *       +---- data_size -------+
 *       |                      |
 *       v                      v
 * +-----+----------------------+------+
 * | hdr | data |               | tail |
 * +-----+----------------------+------+
 *              ^\              ^
 *              | \             |
 *              |  +-- data_free+
 *           current
 */
typedef void (*fbd_free_t)(void *);
typedef struct fifobuf_data
{
	struct list_head list;
	unsigned int data_size; //!< Max available data size. Constant. Do not change this value after init.
	unsigned int data_used; //!< Used data size.
	unsigned int data_free; //!< Free data size.
	uint8_t *current; //!< Current data position to append. (Tricky)
	fbd_free_t free_func; //!<Use this function to free memory

	uint8_t data[0];
} fifobuf_data_t;

#define HAVE_FIFOBUF_TAIL (1) //!< Do not change unless you know the risk.

#if HAVE_FIFOBUF_TAIL
typedef struct fifobuf_tail
{
	// Add zero padding so that it's safe to print data as string.
	uint16_t zero_pad;

	// Add magic num to detect weird overwritting.
	uint16_t magic;
} fifobuf_tail_t;

#define fifobuf_tail_init(_tail) \
	do { \
		((fifobuf_tail_t *) (_tail))->zero_pad = 0x0000; \
		((fifobuf_tail_t *) (_tail))->magic = 0x5408; \
	} while (0)
#define fifobuf_tail_size (sizeof(fifobuf_tail_t))

#else
#define fifobuf_tail_size (0)
#endif

#define fifobuf_data_init(_fbd, _max_data_size, _free_func) \
	do { \
		INIT_LIST_HEAD(&((_fbd)->list)); \
		(_fbd)->data_size = _max_data_size; \
		(_fbd)->data_used = 0; \
		(_fbd)->data_free = _max_data_size; \
		(_fbd)->free_func = (fbd_free_t) _free_func; \
		(_fbd)->current = (_fbd)->data; \
	} while (0)

#define fifobuf_data_tail(_fbd) \
	((fifobuf_tail_t *) (((uint8_t *) (_fbd)->data) + (_fbd)->data_size))

#define fifobuf_data_exit(_fbd) \
	do { \
		list_del(&(_fbd)->list); \
		fifobuf_data_init(_fbd, (_fbd)->data_size, (_fbd)->free_func); \
	} while (0)

static void fifobuf_data_free(fifobuf_data_t *fbdata)
{
	DBG("Free a data %p\n", fbdata);

#if 1 /* Optimize */
	list_del(&(fbdata->list));
#else
	fifobuf_data_exit(fbdata);
#endif

#if HAVE_FIFOBUF_TAIL
	{ // Detect overflow bug.
		fifobuf_tail_t *tail = fifobuf_data_tail(fbdata);
		if (tail->magic != 0x5408)
		{
			ERR("Detect invalid magic %04x (BUG)\n", tail->magic);
		}
	}
#endif

	fbdata->free_func(fbdata);
}

#define __calc_data_size(_alloc_len) ((_alloc_len) - sizeof(fifobuf_data_t) - fifobuf_tail_size)

typedef void *(* fifobuf_data_alloc_func_t)(const unsigned int alloc_len);
typedef void (* fifobuf_data_free_func_t)(void *p);
static inline fifobuf_data_t *__fifobuf_data_alloc(const fifobuf_data_alloc_func_t alloc_func, const fifobuf_data_free_func_t free_func, const unsigned int alloc_len)
{
	fifobuf_data_t *fbdata;
	unsigned int real_data_len;

	fbdata = (fifobuf_data_t *) alloc_func(alloc_len);
	if (NULL == fbdata)
	{
		return NULL;
	}

	/*
	 *   +----- alloc len --------------+
	 *  /                                \
	 * v                                  v
	 * +-----+---------------------+------+
	 * | hdr | data                | tail |
	 * +-----+---------------------+------+
	 *       ^                     ^
	 *       |                     |
	 *       real data start     real data end
	 */
	real_data_len = __calc_data_size(alloc_len);
	fifobuf_data_init(fbdata, real_data_len, free_func);

#if HAVE_FIFOBUF_TAIL
	{
		fifobuf_tail_t *tail = fifobuf_data_tail(fbdata);
		fifobuf_tail_init(tail);
	}
#endif

	DBG("Successfully alloc a data %u bytes at %p\n", alloc_len, fbdata);
	return fbdata;
}

static void *fifobuf_kmalloc_atomic(const unsigned int alloc_len)
{
	return MY_KMALLOC_ATOMIC(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_atomic(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_kmalloc_atomic, (void *) MY_KFREE, alloc_len);
}

static void *fifobuf_kmalloc_sleep(const unsigned int alloc_len)
{
	return MY_KMALLOC_SLEEP(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_sleep(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_kmalloc_sleep, (void *) MY_KFREE, alloc_len);
}

static void *fifobuf_vmalloc(const unsigned int alloc_len)
{
	return MY_VMALLOC(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_vm(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_vmalloc, (void *) MY_VFREE, alloc_len);
}

static void *fifobuf_kmalloc_atomic_notrace(const unsigned int alloc_len)
{
	return MY_KMALLOC_ATOMIC_NOTRACE(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_atomic_notrace(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_kmalloc_atomic_notrace, (void *) MY_KFREE_NOTRACE, alloc_len);
}

static void *fifobuf_kmalloc_sleep_notrace(const unsigned int alloc_len)
{
	return MY_KMALLOC_SLEEP_NOTRACE(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_sleep_notrace(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_kmalloc_sleep_notrace, (void *) MY_KFREE_NOTRACE, alloc_len);
}

static void *fifobuf_vmalloc_notrace(const unsigned int alloc_len)
{
	return MY_VMALLOC_NOTRACE(alloc_len);
}

static fifobuf_data_t *fifobuf_data_alloc_vm_notrace(unsigned int alloc_len /* include struct header */)
{
	return __fifobuf_data_alloc(fifobuf_vmalloc_notrace, (void *) MY_VFREE_NOTRACE, alloc_len);
}

static inline unsigned int fifobuf_data_enqueue(fifobuf_data_t *fbdata, void *data, unsigned int data_len)
{
	unsigned int consume;

	if (data_len == 0)
	{
		return 0;
	}

	if (fbdata->data_free >= data_len)
	{
		/*
		 * Consume all input data
		 */
		memcpy(fbdata->current + fbdata->data_used, data, data_len);
		consume = data_len;

		fbdata->data_used += data_len;
		fbdata->data_free -= data_len;
	}
	else if (fbdata->data_free > 0)
	{
		memcpy(fbdata->current + fbdata->data_used, data, fbdata->data_free);
		consume = fbdata->data_free;

		fbdata->data_used += fbdata->data_free;
		fbdata->data_free -= fbdata->data_free;
	}
	else
	{
		consume = 0;
	}

	return consume;
}

static inline unsigned int fifobuf_data_dequeue(fifobuf_data_t *fbdata, uint8_t *data, unsigned int data_len)
{
	unsigned int consume = 0;

	if (fbdata->data_used >= data_len)
	{
		memcpy(data, fbdata->current, data_len);
		consume += data_len;

		fbdata->current += data_len;
		fbdata->data_used -= data_len;
	}
	else
	{
		memcpy(data, fbdata->current, fbdata->data_used);
		consume += fbdata->data_used;

		fbdata->current += fbdata->data_used;
		fbdata->data_used -= fbdata->data_used;
	}

	return consume;
}

static inline int fifobuf_data_is_empty(fifobuf_data_t *fbdata)
{
	return (fbdata->data_used == 0);
}


static int __fifobuf_enqueue(fifobuf_t *fb, uint8_t *data, unsigned int data_len)
{
	fifobuf_data_t *fbdata, *fbdata_save;
	unsigned int consume;

	/*
	 * fifobuf:
	 *   \    +------+    +------+        +--+---------+
	 *    +-- | data | -> | data | -> ... |  |  unused |
	 *        +------+    +------+        +--+---------+
	 *                                       ^
	 *                                       |
	 *                              Append data after here
	 */
	list_for_each_entry_reverse(fbdata, &(fb->list_data_fifo), list)
	{
		consume = fifobuf_data_enqueue(fbdata, data, data_len);

		data_len -= consume;
		data += consume;

		fb->data_used += consume;
		fb->data_free -= consume;
		break;
	}

	/*                                         tail
	 * fifobuf:                                  |
	 *                                           v
	 *   \    +------+    +------+        +------+
	 *    +-- | data | -> | data | -> ... | data |-> NULL
	 *        +------+    +------+        +------+
	 *                                                ^
	 *                                                |
	 *                                 Append data after here
	 */
	if (data_len == 0)
	{
		return 0;
	}

	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_free), list)
	{
		consume = fifobuf_data_enqueue(fbdata, data, data_len);

		data_len -= consume;
		data += consume;

		list_move_tail(&(fbdata->list), &(fb->list_data_fifo));

		fb->data_used += consume;
		fb->data_free -= consume;

		if (data_len == 0)
		{
			break;
		}
	}

	if (data_len)
	{
		ERR("Expect 0, but %u bytes left. Possibly a bug at caller\n", data_len);
	}

	return 0;
}

static inline int fifobuf_extend(fifobuf_t *fb, unsigned int add_len,
	fifobuf_data_t *(* alloc_func)(unsigned int data_size))
{
	fifobuf_data_t *fbdata;

	while (add_len)
	{
		fbdata = alloc_func(fb->data_size);
		if (fbdata == NULL)
		{
			/* Do not need to do garbage collection. Reserve them for next enqueue. */
			return -1;
		}

		/* Append at free list and wait for data enqueue */
		list_add(&(fbdata->list), &(fb->list_data_free));

		/* Calc free size */
		fb->data_free += fbdata->data_free;

		if (add_len > fbdata->data_free)
		{
			add_len -= fbdata->data_free;
		}
		else
		{
			/* Alloc more than expected len. */
			break;
		}
	}

	return 0; // ok
}

int fifobuf_extend_atomic(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_atomic);
}

int fifobuf_extend_sleep(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_sleep);
}

int fifobuf_extend_vm(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_vm);
}

int fifobuf_extend_atomic_notrace(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_atomic_notrace);
}

int fifobuf_extend_sleep_notrace(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_sleep_notrace);
}

int fifobuf_extend_vm_notrace(fifobuf_t *fb, unsigned int add_len)
{
	return fifobuf_extend(fb, add_len, fifobuf_data_alloc_vm_notrace);
}

static inline int fifobuf_enqueue(fifobuf_t *fb, void *data, unsigned int data_len, fifobuf_data_t * (* fifobuf_data_alloc_func)(unsigned int size))
{
	if (fb->data_free < data_len)
	{
		/* Resource not enough. Alloc more data (* NOTE: extend first to make sure we can enqueue.) */
		if (fifobuf_extend(fb, (data_len - fb->data_free), fifobuf_data_alloc_func) < 0)
		{
			DBG("Cannot realloc len %u\n", data_len - fb->data_free);
			return -1;
		}
	}

	DBG("Try to enqueue %u bytes to %p (free=%u/%u)\n",
		data_len, fb, fb->data_free, (fb->data_used + fb->data_free));
	return __fifobuf_enqueue(fb, data, data_len);
}

int fifobuf_enqueue_atomic(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_atomic);
}

int fifobuf_enqueue_sleep(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_sleep);
}

int fifobuf_enqueue_vm(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_vm);
}

int fifobuf_enqueue_atomic_notrace(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_atomic_notrace);
}

int fifobuf_enqueue_sleep_notrace(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_sleep_notrace);
}

int fifobuf_enqueue_vm_notrace(fifobuf_t *fb, void *data, unsigned int data_len)
{
	return fifobuf_enqueue(fb, data, data_len, fifobuf_data_alloc_vm_notrace);
}

int fifobuf_enqueue_zero_padding(fifobuf_t *fb)
{
	fifobuf_data_t *fbdata;
	unsigned int consume;

	uint8_t zero_pad = 0x00;

	/*
	 * fifobuf:
	 *   \    +------+    +------+        +--+---------+
	 *    +-- | data | -> | data | -> ... |  |  unused |
	 *        +------+    +------+        +--+---------+
	 *                                       ^
	 *                                       |
	 *                              Append data after here
	 */
	list_for_each_entry_reverse(fbdata, &(fb->list_data_fifo), list)
	{
		consume = fifobuf_data_enqueue(fbdata, &zero_pad, 1);

		if (consume == 0)
		{
			return -1;
		}

		fb->data_used += consume;
		fb->data_free -= consume;
		break;
	}

	return 0;
}

void fifobuf_debug(fifobuf_t *fb)
{
	fifobuf_data_t *fbdata;

	printf("fifo:\n");
	list_for_each_entry(fbdata, &(fb->list_data_fifo), list)
	{
		printf(" * used=%u free=%u size=%u\n", fbdata->data_used, fbdata->data_free, fbdata->data_size);
	}

	printf("free:\n");
	list_for_each_entry(fbdata, &(fb->list_data_free), list)
	{
		printf(" * used=%u free=%u size=%u\n", fbdata->data_used, fbdata->data_free, fbdata->data_size);
	}
}

/*!
 * \brief Dequeue data to a given buffer.
 *
 * \param fb      fifobuf ctx
 * \param buf     buffer to save output
 * \param buf_len available buffer len (bytes)
 *
 * \return Number of bytes dequeued to buf.
 */
unsigned int fifobuf_dequeue(fifobuf_t *fb, uint8_t *buf, unsigned int buf_len)
{
	fifobuf_data_t *fbdata, *fbdata_save;
	unsigned int consume = 0, consume_total = 0;

	if (fb->data_used == 0)
	{
		DBG("Dequeue %u bytes from %p\n", 0, fb);
		return 0;
	}

	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_fifo), list)
	{
		if (buf_len == 0)
		{
			break;
		}

		consume = fifobuf_data_dequeue(fbdata, buf, buf_len);

		buf_len -= consume;
		buf += consume;
		fb->data_used -= consume;
		consume_total += consume;

		if (fifobuf_data_is_empty(fbdata))
		{
#if (1) /* Free ASAP. */
			fifobuf_data_free(fbdata);
#else   /* Free later */
			/* Send back to free list. */
			fifobuf_data_exit(fbdata);
			list_add(&(fbdata->list), &(fb->list_data_free));
#endif
		}
	} // end for

	DBG("Dequeue %u bytes from %p\n", consume_total, fb);
	return consume_total;
}

void fifobuf_flush(fifobuf_t *fb)
{
	fifobuf_data_t *fbdata, *fbdata_save;

	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_fifo), list)
	{
		fifobuf_data_free(fbdata);
	}

	/* (Optional) Also remove free list?! */
	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_free), list)
	{
		fifobuf_data_free(fbdata);
	}

	fb->data_free = 0;
	fb->data_used = 0;
}

/*!
 * \brief Read and read only. (Won't dequeue read data).
 */
int fifobuf_ro(fifobuf_t *fb, fifobuf_ro_func_t ro_func, void *priv)
{
	fifobuf_data_t *fbdata;
	int ret;

	unsigned int offset, total;

	offset = 0;
	total = fb->data_used;
	list_for_each_entry(fbdata, &(fb->list_data_fifo), list)
	{
		ret = ro_func(fbdata->current, fbdata->data_used, offset, total, priv);
		if (ret)
		{
			DBG("User stops reading data\n");
			return ret;
		}

		offset += fbdata->data_used;
	}

	return 0;
}

/*!
 * \brief Exit a fifobuf ctx.
 *
 * \param fb fifobuf ctx
 *
 * \sa fifobuf_init
 */
void fifobuf_exit(fifobuf_t *fb)
{
	fifobuf_data_t *fbdata, *fbdata_save;

	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_fifo), list)
	{
		fifobuf_data_free(fbdata);
	}

	list_for_each_entry_safe(fbdata, fbdata_save, &(fb->list_data_free), list)
	{
		fifobuf_data_free(fbdata);
	}

#if 0 // Optimize out.
	fifobuf_init(fb);
#endif
}

/*!
 * \brief Free a fifobuf ctx
 *
 * \param fb fifobuf ctx
 */
void fifobuf_free(fifobuf_t *fb)
{
	DBG("Free a fifobuf %p\n", fb);

	fifobuf_exit(fb);
	MY_KFREE(fb);
}

/*!
 * \brief Alloc a fifobuf ctx (sleep)
 *
 * \param data_size A proper size for each data block. Don't be too large (>= 128KB) if you are using enqueue_atomic or enqueue_sleep.
 *
 * \sa fifobuf_free
 */
fifobuf_t *fifobuf_alloc_sleep(unsigned int data_size)
{
	fifobuf_t *fb;

	fb = MY_KMALLOC_SLEEP(sizeof(*fb));
	if (fb == NULL)
	{
		return NULL;
	}

	fifobuf_init(fb, data_size);
	DBG("Successfully alloc a fifobuf %p\n", fb);
	return fb;
}

/*!
 * \brief Alloc a fifobuf ctx (sleep)
 *
 * \param data_size A proper size for each data block. Don't be too large (>= 128KB) if you are using enqueue_atomic or enqueue_sleep.
 *
 * \sa fifobuf_free
 */
fifobuf_t *fifobuf_alloc_atomic(unsigned int data_size)
{
	fifobuf_t *fb;

	fb = MY_KMALLOC_ATOMIC(sizeof(*fb));
	if (fb == NULL)
	{
		return NULL;
	}

	fifobuf_init(fb, data_size);
	DBG("Successfully alloc a fifobuf %p\n", fb);
	return fb;
}

/*!
 * \brief  Round up the input value to a power of 2 (2 ^ n)
 * \detail For example, input 1 -> output 2, input 4 -> output 4, input 5 -> output 8
 *
 * \param x Input unsigned integer must <= (2 ^ 31)
 *
 * \return Round up the input value to a power of 2 (2 ^ n)
 */
static inline unsigned int pow2_adjust(unsigned int x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return (x + 1);
}

/*!
 * \brief Help the caller to calculate a proper data size for alloc function.
 *
 * \param minimal The minimal value of data block size.
 *
 * \sa fifobuf_alloc_atomic
 * \sa fifobuf_alloc_sleep
 *
 * \return A proper size of data block size.
 */
unsigned int fifobuf_calibrate_data_size(unsigned int minimal)
{
	if (minimal <= (sizeof(fifobuf_data_t) + fifobuf_tail_size))
	{
		minimal = ((sizeof(fifobuf_data_t) + fifobuf_tail_size) + 1);
	}

	return pow2_adjust(minimal);
}

#if (1)
int main(void)
{
	fifobuf_t fb;

	fifobuf_init(&fb, 4096);

	fifobuf_enqueue_sleep(&fb, "test ok\n", 8);

	{
		uint8_t output[128] = { 0 };
		fifobuf_dequeue(&fb, output, sizeof(output));
		printf(output);
	}

	fifobuf_exit(&fb);

	return 0;
}
#endif
