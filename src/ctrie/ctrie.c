
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "ctrie.h"

#define HAVE_DEBUG_MSG (0) //!< Say 1 to debug

#if HAVE_DEBUG_MSG
#define DBG(fmt, args...) printf("[%s#%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args)
#define BUG_ON(_expr) assert(!(_expr))
#else
#define DBG(fmt, args...) do { } while (0)
#define BUG_ON(_expr) do { if (!(_expr)) { } } while (0)
#endif

#define ERR(fmt, args...) printf(" * ERROR: [%s#%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args)


#define VMALLOC malloc
#define KMALLOC_ATOMIC malloc
#define KMALLOC_SLEEP malloc
#define KFREE free
#define KFREE_NULLIFY(_p) do { KFREE(_p); (_p) = NULL; } while (0)
#define VFREE free
#define VFREE_NULLIFY(_p) do { VFREE(_p); (_p) = NULL; } while (0)

#define CTRIE_INIT_STATE (0)
#define CTRIE_STATE(_ctrie, _state_id) ((ctrie_state_t *) (((ctrie_state_t *) (_ctrie)->state) + (_state_id)))

#define CTRIE_STATE_IS_FINISH(_state) ((_state)->finish)

#define __to_upper(c) \
	({ typeof(c) __c = c; ((__c >= 'a') && (__c <= 'z')) ? (__c - 0x20) : __c; })


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


typedef struct ctrie_state
{
	unsigned int desc_id; //!< Output desc id to user
	uint16_t finish; //!< Finish state.
	uint16_t trans_num;
	unsigned int trans; // Offset
} ctrie_state_t;

/*
 * Convert 8 bits val and 24 bits state id -> 32 bit container
 */
static inline ctrie_state24_id_t state24_id2container(const ctrie_state24_id_t trans, const uint8_t ch)
{
#define CONVERT_STATE24_ID(_ch, _trans) ((((uint8_t) (_ch)) << 24) | (_trans))

	BUG_ON((trans & (~0x00ffffff)) != 0); // Invalid transition num
	return CONVERT_STATE24_ID(ch, trans);
}

/*
 * Convert 32 bit container to: 8 bits val and 24 bits state id
 */
static inline void state24_container2id(ctrie_state24_id_t *id, uint8_t *ch, const ctrie_state24_id_t trans)
{
	ctrie_state24_id_t tmp;

	*id = (trans & 0x00ffffff);
	tmp = ((trans & (~0x00ffffff)) >> 24);
	*ch = (uint8_t) tmp;
}

static inline void *ctrie_get_buf(const ctrie_t *ctrie, const unsigned int buf_offset)
{
	BUG_ON(ctrie->buf == NULL);

	return ((uint8_t *) ctrie->buf) + buf_offset;
}

static void ctrie_free_buf(ctrie_t *ctrie)
{
	switch (ctrie->type)
	{
	case CTRIE_TYPE_24BIT:
	default:
		VFREE_NULLIFY(ctrie->buf);
		break;
	}

	ctrie->buf_max = 0;
	ctrie->buf_used = 0;
	ctrie->buf_guess_max = 0;
}

static int ctrie_realloc_buf(ctrie_t *ctrie, const unsigned int new_buf_max)
{
	if (new_buf_max < ctrie->buf_max)
	{
		ERR("Invalid realloc size %u -> %u", new_buf_max, ctrie->buf_max);
		BUG_ON(1);
		return -1;
	}

	{
		void *p, *old;

		/*
		 * Alloc new buf
		 */
		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
		default:
			p = VMALLOC(new_buf_max);
			if (p == NULL)
			{
				ERR("Cannot realloc trans buf %u -> %u bytes", ctrie->buf_max, new_buf_max);
				return -1;
			}

			DBG("Realloc trans buf %u -> %u bytes", ctrie->buf_max, new_buf_max);
			break;
		}

		ctrie->mem += (new_buf_max - ctrie->buf_max);

		/*
		 * Copy old to new
		 */
		old = ctrie->buf;
		ctrie->buf = p;

		memcpy(ctrie->buf, old, ctrie->buf_used);

		ctrie->buf_max = new_buf_max;

		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
		default:
			VFREE(old);
			break;
		}
	}

	return 0; // ok
}

static int ctrie_extend_buf_room(ctrie_t *ctrie, const unsigned int need_bytes)
{
#define KB(_n) ((_n) * 1024)
	unsigned int add_bytes;

	BUG_ON(ctrie->buf_guess_max == 0);

	if ((ctrie->buf_max - ctrie->buf_used) >= need_bytes)
	{
		return 0; /* Still safe to use. Do not extend buf room */
	}

	if (ctrie->buf_max == 0)
	{
		/*
		 * 1st guess
		 */
		if (ctrie->buf_guess_max > KB(8))
		{
			add_bytes = pow2_adjust(ctrie->buf_guess_max / 8);
		}
		else
		{
			/* ctrie is too small. Let it go. */
			add_bytes = ctrie->buf_guess_max;
		}
	}
	else
	{
		/*
		 * 2nd/3rd/4th... guess
		 */
		add_bytes = (ctrie->buf_guess_max / 4);
	}

	if (add_bytes < need_bytes)
	{
		/*
		 * We should not extend the buf < need size.
		 */
		add_bytes = pow2_adjust(need_bytes);
	}

	return ctrie_realloc_buf(ctrie, ctrie->buf_max + (add_bytes));
}

static void ctrie_free_state(ctrie_t *ctrie)
{
	switch (ctrie->type)
	{
	case CTRIE_TYPE_24BIT:
	default:
		VFREE_NULLIFY(ctrie->state);
		break;
	}

	ctrie->state_max = 0;
	ctrie->state_used = 0;
	ctrie->state_guess_max = 0;
}

static int ctrie_realloc_state(ctrie_t *ctrie, unsigned int new_max_state)
{
	if (new_max_state < ctrie->state_max)
	{
		ERR("Invalid realloc state num %u -> %u", new_max_state, ctrie->state_max);
		BUG_ON(1);
		return -1;
	}

	{
		void *p, *old;

		/*
		 * Detect state overflow
		 */
		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
			if (new_max_state >= CTRIE_STATE24_ID_MAX /* Too many state */)
			{
				if (ctrie->state_max < CTRIE_STATE24_ID_MAX)
				{
					new_max_state = CTRIE_STATE24_ID_MAX;
				}
				else
				{
					ERR("Exceed max state num %u", ctrie->state_max);
					return -1;
				}
			}
			break;
		default:
			BUG_ON(1);
			break;
		}

		/*
		 * Alloc new state table
		 */
		p = VMALLOC(sizeof(ctrie_state_t) * new_max_state);
		if (p == NULL)
		{
			ERR("Cannot realloc state num %u -> %u", ctrie->state_max, new_max_state);
			return -1;
		}

		DBG("Realloc state num %u -> %u", ctrie->state_max, new_max_state);

		ctrie->mem += (new_max_state - ctrie->state_max) * sizeof(ctrie_state_t);

		/*
		 * Copy old to new
		 */
		old = ctrie->state;
		ctrie->state = p;

		memcpy(ctrie->state, old, ctrie->state_used * sizeof(ctrie_state_t));

		ctrie->state_max = new_max_state;

		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
		default:
			VFREE(old);
			break;
		}
	}

	return 0; // ok
}

static int ctrie_extend_state_room(ctrie_t *ctrie)
{
	unsigned int add_state;

	BUG_ON(ctrie->state_guess_max == 0);

	if (ctrie->state_used < ctrie->state_max)
	{
		return 0; /* Still safe to use. Do not extend state room */
	}

	if (ctrie->state_max == 0)
	{
		/*
		 * 1st guess
		 */
		if (ctrie->state_guess_max > 1024)
		{
			add_state = pow2_adjust(ctrie->state_guess_max / 8);
		}
		else
		{
			/* ctrie is too small. Let it go. */
			add_state = ctrie->state_guess_max;
		}
	}
	else
	{
		/*
		 * 2nd/3rd/4th... guess
		 */
		add_state = (ctrie->state_guess_max / 4);
	}

	if (add_state == 0)
	{
		add_state = 1; /* A very small ctrie!? =.= */
	}

	return ctrie_realloc_state(ctrie, ctrie->state_max + add_state);
}

void ctrie_init(ctrie_t *ctrie, const ctrie_type_t ctrie_type, const unsigned int enable_case_sensitive)
{
	ctrie->wsp = NULL;

	ctrie->buf = NULL;
	ctrie->buf_max = 0;
	ctrie->buf_used = 0;

	ctrie->state = NULL;
	ctrie->state_max = 0;
	ctrie->state_used = 0;

	ctrie->state_max = 0;
	ctrie->state_used = 0;

	ctrie->type = ctrie_type;

	ctrie->case_sensitive = !!enable_case_sensitive;

	ctrie->mem = 0;
}

void ctrie_exit(ctrie_t *ctrie)
{
	ctrie_free_buf(ctrie);
	ctrie_free_state(ctrie);

	KFREE(ctrie->wsp);

	ctrie_init(ctrie, CTRIE_TYPE_INVAL, 0);
}

ctrie_t *ctrie_alloc_sleep(const ctrie_type_t trie_type, const unsigned int enable_case_sensitive)
{
	ctrie_t *ctrie;

	ctrie = KMALLOC_SLEEP(sizeof(*ctrie));
	if (ctrie == NULL)
	{
		return NULL;
	}

	ctrie_init(ctrie, trie_type, enable_case_sensitive);
	return ctrie;
}

void ctrie_free(ctrie_t *ctrie)
{
	if (ctrie)
	{
		ctrie_exit(ctrie);
		KFREE(ctrie);
	}
}

/*
 * TODO: Improve
 */
static unsigned int guess_ctrie_state_by_desc_tbl(const ctrie_desc_t *tbl, const unsigned int tbl_size)
{
	int i;
	const ctrie_desc_t *desc;

	unsigned int state_num = 0;

	for (i = 0; i < tbl_size; i++)
	{
		desc = &(tbl[i]);

		state_num += desc->val_len + 1; // Add 1 end state for this desc.
	} // end for

	return state_num;
}

static inline void init_ctrie_state(ctrie_state_t *state)
{
	state->trans = 0;
	state->trans_num = 0;

	state->finish = 0;
	state->desc_id = 0;
}

/*
 * Get an unused state from state pool.
 */
static ctrie_state_t *pull_ctrie_state(ctrie_state_id_t *id, ctrie_t *ctrie)
{
	ctrie_state_t *state;

	if (ctrie_extend_state_room(ctrie))
	{
		return NULL;
	}

	DBG("Pull a new state %u", ctrie->state_used);
	*id = ctrie->state_used;
	state = CTRIE_STATE(ctrie, ctrie->state_used);

	ctrie->state_used++;

	init_ctrie_state(state);
	return state;
}

/*
 * Reset working space (wsp) buf.
 */
static inline void ctrie_reset_wsp(ctrie_wsp_t *wsp)
{
	wsp->trans_num = 0;
	memset(wsp->trans, 0x00, sizeof(wsp->trans)); // Set trans to 0! (No trans).
}

/*
 * Init desc private data.
 */
static void init_desc_tbl_current_state(ctrie_desc_t *tbl, const unsigned int tbl_size)
{
	unsigned int i;

	for (i = 0; i < tbl_size; i++)
	{
		ctrie_desc_t *desc = &(tbl[i]);

		/*
		 * e.g. desc = "apple"
		 *
		 * offset=0, state=init state
		 * |
		 * V
		 * a p p l e
		 */
		desc->current_state = CTRIE_INIT_STATE;
		desc->val_offset = 0;
	}
}

static int state24_realloc_trans_from_buf(ctrie_t *ctrie, const unsigned int trans_num)
{
	unsigned int need_bytes = trans_num * sizeof(ctrie_state24_id_t);
	return ctrie_extend_buf_room(ctrie, need_bytes);
}

/*
 * Alloc state trans buf for a new state.
 */
static unsigned int state24_alloc_trans_from_buf(ctrie_t *ctrie, const unsigned int trans_num)
{
	unsigned int offset;
	unsigned int bytes = trans_num * sizeof(ctrie_state24_id_t);

	BUG_ON(bytes > (ctrie->buf_max - ctrie->buf_used));

	offset = ctrie->buf_used;
	ctrie->buf_used += bytes;

	return offset;
}

/*
 * Copy wsp (working space) to state.
 */
static inline int state24_copy_wsp2state(ctrie_state_t *state, ctrie_t *ctrie)
{
	ctrie_wsp_t *wsp = ctrie->wsp;

	DBG("Copy wsp trans num=%u to state %p", wsp->trans_num, state);

	/*
	 * Alloc state trans buf.
	 */
	state->trans_num = wsp->trans_num;
	if (state->trans_num == 0)
	{
		return 0; // No next state.
	}

	/* Make sure we have enough space to save trans and alloc it */
	if (state24_realloc_trans_from_buf(ctrie, state->trans_num))
	{
		return -1;
	}
	else
	{
		state->trans = state24_alloc_trans_from_buf(ctrie, state->trans_num);
	}

	/*
	 * Copy all state trans in wsp to the allocated space.
	 */
	{
		ctrie_state24_id_t *trans;

		unsigned int ch;
		unsigned int trans_used = 0;

		trans = (ctrie_state24_id_t *) ctrie_get_buf(ctrie, state->trans);

		for (ch = 0; ch < 256; ch++)
		{
			if (wsp->trans[ch] != CTRIE_INIT_STATE)
			{
				BUG_ON(trans_used >= state->trans_num);
				trans[trans_used] = state24_id2container(wsp->trans[ch], (uint8_t) ch);
				trans_used++;
			}
		} // end for

		BUG_ON(state->trans_num != trans_used);
	}

	return 0; // ok
}

static int state24_build_by_desc_tbl_bfs(
	ctrie_t *ctrie, ctrie_desc_t *tbl, const unsigned int tbl_size,
	const ctrie_state_id_t state_id)
{
	ctrie_wsp_t *wsp = ctrie->wsp;
	unsigned int idx;

	ctrie_reset_wsp(wsp);

	/*
	 * Save all state trans in wsp first.
	 */
	for (idx = 0; idx < tbl_size; idx++)
	{
		ctrie_desc_t *desc = &(tbl[idx]);

		if (desc->current_state != state_id)
		{
			continue; /* Current desc does not belong to this trie path. Skip this desc. */
		}

		if (desc->val_offset >= desc->val_len)
		{
			continue; /* No more bytes in this desc */
		}

		{
			/*
			 *
			 *          val_offset=2
			 *            |
			 *            V
			 * desc = a p p l e
			 *
			 *               state=2    Alloc state_child and add
			 *                   |         |
			 *                   V         V
			 *                 p        p
			 *        -- 1 +-----+ 2-------+ 3
			 *    a  /
			 * 0 +--+
			 */
			uint8_t ch = desc->val[desc->val_offset];
			ctrie_state_t *state_child;

			if (!ctrie->case_sensitive)
			{
				ch = __to_upper(ch);
			}

			/*
			 * Create new state or use existed path.
			 */
			if (wsp->trans[ch] == CTRIE_INIT_STATE)
			{
				/*
				 * No trans. Add a new state here
				 */
				ctrie_state_id_t id;
				state_child = pull_ctrie_state(&id, ctrie);
				if (state_child == NULL)
				{
					return -1;
				}

				wsp->trans[ch] = id;
				wsp->trans_num++;
			}
			else
			{
				/* Already have a node here */
				state_child = CTRIE_STATE(ctrie, wsp->trans[ch]);
			}

			/*
			 * Save next state of this desc.
			 */
			DBG("Desc [%u]: id=%d, off=%u, '%c' -> %u",
				idx, desc->id, desc->val_offset, isalnum(ch) ? ch : '.', wsp->trans[ch]);

			desc->current_state = wsp->trans[ch];
			desc->val_offset++; /* Next byte of this desc */

			/*
			 * Detect duplicated desc. It's not reasonable to have identical inputs.
			 */
			if (desc->val_offset == desc->val_len)
			{
				if (!CTRIE_STATE_IS_FINISH(state_child))
				{
					DBG("Set finish state at %u", wsp->trans[ch]);
					state_child->desc_id = desc->id;
					state_child->finish = 1;
				}
				else
				{
					ERR("Detect a duplicated ctrie desc idx %u (desc id %u)", idx, state_child->desc_id);
					BUG_ON(1);
				}
			}

		}
	} // end for

	/*
	 * Then, copy the completed wsp to state trans array.
	 */
	return state24_copy_wsp2state(CTRIE_STATE(ctrie, state_id), ctrie);
}

static int __build_by_desc_tbl(ctrie_t *ctrie, ctrie_desc_t *tbl, const unsigned int tbl_size)
{
	ctrie_state_id_t state_id;
	ctrie_state_t *state;

	init_desc_tbl_current_state(tbl, tbl_size);

	/*
	 * Alloc init state 0
	 */
	state = pull_ctrie_state(&state_id, ctrie);
	if (state == NULL)
	{
		return -1;
	}

	BUG_ON(state_id != CTRIE_INIT_STATE);

	/*
	 * Build trie from init state by BFS order.
	 */
	for (state_id = CTRIE_INIT_STATE; state_id < ctrie->state_used; state_id++)
	{
		/*
		 * Assume input trie: "he" "his" "my"
		 *
		 * state 0:
		 *               h  +- 1
		 *                /
		 * 0       ->  0 +--+- 2
		 *                 m
		 *
		 * state 1:
		 *                                  i
		 *                                +--- 3
		 *                               /  e
		 *  h   +- 1             h  +- 1 +---- 4
		 *    /                   /
		 * 0 +--+- 2       ->  0 +--+- 2
		 */
		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
			if (state24_build_by_desc_tbl_bfs(ctrie, tbl, tbl_size, state_id))
			{
				return -1;
			}
			break;
		default:
			return -1;
		}
	}

	return 0; // ok
}

static int validate_desc_tbl(const ctrie_desc_t *tbl, const unsigned int tbl_size)
{
	unsigned int i;

	for (i = 0; i < tbl_size; i++)
	{
		const ctrie_desc_t *desc = &(tbl[i]);

		if (desc->val == NULL || desc->val_len == 0)
		{
			ERR("Invalid ctrie desc value at idx %u", i);
			return -1;
		}
	}

	return 0;
}

int ctrie_build_by_desc_tbl(ctrie_t *ctrie, ctrie_desc_t *tbl, const unsigned int tbl_size)
{
	if (validate_desc_tbl(tbl, tbl_size))
	{
		return -1;
	}

	{
		unsigned int num;

		/*
		 * Guess possible state size
		 */
		num = guess_ctrie_state_by_desc_tbl(tbl, tbl_size);
		if (num == 0)
		{
			ERR("Cannot build ctrie with zero desc");
			return -1;
		}

		ctrie->mem = 0;

		/*
		 * Alloc state buffer
		 */
		switch (ctrie->type)
		{
		case CTRIE_TYPE_24BIT:
			/* Alloc state buf */
			BUG_ON(ctrie->state != NULL);
			ctrie->state_guess_max = num;
			ctrie->state = NULL;
			ctrie->state_max = 0;
			ctrie->state_used = 0;

			/* Alloc state trans buf */
			BUG_ON(ctrie->buf != NULL);
			ctrie->buf_guess_max = (num) * sizeof(ctrie_state24_id_t);
			ctrie->buf = NULL;
			ctrie->buf_used = 0;
			ctrie->buf_max = 0;
			break;
		default:
			ERR("Invalid ctrie type %d", ctrie->type);
			goto ERROR;
		}

		DBG("Successfully alloc ctrie: state num=%u, mem=%u", ctrie->state_max, ctrie->mem);
	}

	/*
	 * Alloc a temporary state working space to save 256 state trans
	 */
	{
		unsigned int mem = sizeof(ctrie->wsp[0]);
		ctrie->wsp = KMALLOC_SLEEP(mem);
		if (ctrie->wsp == NULL)
		{
			ERR("Cannot alloc ctrie state wsp %u bytes", mem);
			goto ERROR;
		}
	}

	/*
	 * Build ctrie by input desc
	 */
	if (__build_by_desc_tbl(ctrie, tbl, tbl_size))
	{
		goto ERROR;
	}

	/*
	 * Free temporary state working space
	 */
	KFREE_NULLIFY(ctrie->wsp);

	return 0;

ERROR:
	ctrie_exit(ctrie);
	return -1;
}

static ctrie_state24_id_t state24_trans(const ctrie_t *ctrie, const ctrie_state_t *state, const uint8_t ch)
{
	ctrie_state24_id_t next_state_id;

	int st, ed, mid;
	int max = state->trans_num;

	if (max == 0)
	{
		return 0; // No next state
	}

	/*
	 * Do binary search to find next state.
	 */
	st = 0;
	ed = max - 1;
	mid = (st + ed) / 2;

	while (st <= ed)
	{
		uint8_t saved_ch;
		ctrie_state24_id_t trans;
		ctrie_state24_id_t *trans_tbl;

		trans_tbl = ctrie_get_buf(ctrie, state->trans);
		trans = trans_tbl[mid];
		state24_container2id(&next_state_id, &saved_ch, trans);

		if (saved_ch < ch)
		{
			st = mid + 1;
		}
		else if (saved_ch == ch)
		{
			return next_state_id;
		}
		else
		{
			ed = mid - 1;
		}

		mid = (st + ed) / 2;
	}

	return 0;
}

ctrie_res_t ctrie_trans24(
	ctrie_ctx_t *ctx, const ctrie_t *ctrie,
	const uint8_t *buf, unsigned int buf_len, unsigned int *buf_used_len)
{
	uint8_t ch;
	ctrie_state_t *state;

	if (buf_used_len)
	{
		*buf_used_len = 0;
	}

	if (ctx->state >= ctrie->state_used)
	{
		DBG("Invalid input ctx");
		return CTRIE_RES_INVAL;
	}

	BUG_ON(ctrie->type != CTRIE_TYPE_24BIT);

	state = CTRIE_STATE(ctrie, ctx->state);
	while (buf_len)
	{
		ctrie_state24_id_t next_state_id;

		ch = *buf;
		if (!ctrie->case_sensitive)
		{
			ch = __to_upper(ch);
		}

		next_state_id = state24_trans(ctrie, state, ch);
		if (next_state_id == 0)
		{
			ctx->state = ctrie->state_used; // Make next transition impossible.
			return CTRIE_RES_INVAL;
		}

		DBG("State trans '%c': %u -> %u",
			isalnum(ch) ? ch : '.', ctx->state, next_state_id);
		ctx->state = next_state_id;

		/*
		 * Next state
		 */
		state = CTRIE_STATE(ctrie, next_state_id);

		buf++;
		buf_len--;
		if (buf_used_len)
		{
			(*buf_used_len)++;
		}

		/*
		 * Output after updating buf_used_len
		 */
		if (CTRIE_STATE_IS_FINISH(state))
		{
			ctx->desc_id = state->desc_id;
			return CTRIE_RES_FINISH;
		}
	} // end while

	return CTRIE_RES_CONT;
}

ctrie_res_t ctrie_trans(
	ctrie_ctx_t *ctx, const ctrie_t *ctrie,
	const uint8_t *buf, unsigned int buf_len, unsigned int *buf_used_len)
{
	switch (ctrie->type)
	{
	case CTRIE_TYPE_24BIT:
		return ctrie_trans24(ctx, ctrie, buf, buf_len, buf_used_len);
	default:
		if (buf_used_len)
		{
			(*buf_used_len) = 0;
		}
		break;
	}

	return CTRIE_RES_INVAL;
}

void ctrie_debug_state(ctrie_t *ctrie)
{
	ctrie_state_id_t state_id;

	for (state_id = CTRIE_INIT_STATE; state_id < ctrie->state_used; state_id++)
	{
		ctrie_state_t *state = CTRIE_STATE(ctrie, state_id);

		printf("State %u (finish=%u, desc_id=%d, trans=%u):\n",
			state_id, state->finish, state->desc_id, state->trans_num);

		/* Print all trans of this state */
		if (state->trans_num)
		{
			unsigned int i;

			for (i = 0; i < state->trans_num; i++)
			{
				ctrie_state24_id_t id24;
				ctrie_state24_id_t *trans_tbl;
				uint8_t ch;

				switch (ctrie->type)
				{
				case CTRIE_TYPE_24BIT:
					trans_tbl = (ctrie_state24_id_t *) ctrie_get_buf(ctrie, state->trans);
					state24_container2id(&id24, &ch, trans_tbl[i]);

					printf("\t['%c'] -> %u\n",
						isalnum(ch) ? ch : '.', id24);
					break;
				default:
					break;
				}
			} // end for
		}
	} // end for
}

void ctrie_debug(ctrie_t *ctrie)
{
	printf("type: %u\n", ctrie->type);
	printf("case-sensitive: %u\n", ctrie->case_sensitive);
	printf("state: %u/%u\n", ctrie->state_used, ctrie->state_max);
	printf("buf: %u/%u\n", ctrie->buf_used, ctrie->buf_max);
	printf("wsp: %p\n", ctrie->wsp); // should be null.
	printf("memory: %u\n", ctrie->mem);

	ctrie_debug_state(ctrie);
}


#if (0)
int main(void)
{
	ctrie_t ctrie;

	ctrie_init(&ctrie, CTRIE_TYPE_24BIT, 1);

	{
		ctrie_desc_t desc_tbl[] =
		{
			CTRIE_DESC_INITIALIZER(0, "abc", 3),
			CTRIE_DESC_INITIALIZER(0, "acd", 3),
		};

		ctrie_build_by_desc_tbl(&ctrie, desc_tbl, 2);
	}

	{
		ctrie_ctx_t ctrie_ctx;
		ctrie_res_t ctrie_res;

		ctrie_ctx_init(&ctrie_ctx);

		ctrie_res = ctrie_trans(&ctrie_ctx, &ctrie, "abc", 3, NULL);
		printf("%d\n", ctrie_res);
		assert(CTRIE_RES_FINISH == ctrie_res);
	}

	ctrie_exit(&ctrie);

	return 0;
}
#endif
