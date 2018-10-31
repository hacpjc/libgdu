#ifndef CTRIE_H_
#define CTRIE_H_

#include <stdint.h>

/*
 * ctrie state id with different size
 */
typedef uint32_t ctrie_state24_id_t;
typedef uint32_t ctrie_state_id_t; // * NOTE: Always set to maximum state width

#define CTRIE_STATE24_ID_MAX ((uint32_t) 0x00ffffff)
#define CTRIE_STATE_ID_MAX   (CTRIE_STATE24_ID_MAX)

/*
 * ctrie desc
 */
typedef struct ctrie_desc
{
	unsigned int id; /* An optional id for users to know which desc is matched. */
	const uint8_t *val;
	unsigned int val_len;

	/*
	 * Private data. Plz ignore if you are ctrie caller.
	 */
	ctrie_state_id_t current_state;
	unsigned int val_offset;
} ctrie_desc_t;

#define CTRIE_DESC_INITIALIZER(_id, _val, _val_len) { .id = (_id), .val = (_val), .val_len = (_val_len) }

static inline __attribute__((unused))
void ctrie_desc_init(ctrie_desc_t *desc, const unsigned int id, const uint8_t *val, const unsigned int val_len)
{
	desc->id = id;
	desc->val = val;
	desc->val_len = val_len;
}

/*
 * ctrie ctx - to save current state
 */
typedef struct ctrie_ctx
{
	unsigned int state;
	unsigned int desc_id; //!< Matched desc id
} ctrie_ctx_t;

#define ctrie_ctx_init(_ctx) do { (_ctx)->state = 0; (_ctx)->desc_id = 0; } while (0)
#define ctrie_ctx_exit(_ctx) do { } while (0)

#define CTRIE_CTX_INITIALIZER { .state = 0, .desc_id = 0 }

#define ctrie_ctx_get_desc_id(_ctx) ((_ctx)->desc_id)

/*
 * ctrie result
 */
typedef enum
{
	CTRIE_RES_INVAL = -2,
	CTRIE_RES_CONT,
	CTRIE_RES_FINISH,
} ctrie_res_t;

#define ctrie_res_is_continue(_res) ((_res) == CTRIE_RES_CONT)
#define ctrie_res_is_invalid(_res)  ((_res) == CTRIE_RES_INVAL)
#define ctrie_res_is_finish(_res)   ((_res) == CTRIE_RES_FINISH)

/*
 * ctrie type
 */
typedef enum
{
	CTRIE_TYPE_INVAL = 0,
	CTRIE_TYPE_24BIT,
} ctrie_type_t;


/*
 * trie case-sensitive
 */
enum
{
	CTRIE_CASE_INSEN = 0,
	CTRIE_CASE_SEN
};

/*
 * ctrie state wsp (working space)
 */
typedef struct ctrie_wsp
{
	unsigned int trans_num;
	ctrie_state_id_t trans[256]; // 1KB
} ctrie_wsp_t;

/*
 * ctrie
 */
typedef struct ctrie
{
	ctrie_wsp_t *wsp; // A state working space to build trans of a state.

	void *buf; // Another buf to save state trans instead of a size=256 table.
	unsigned int buf_max;
	unsigned int buf_used;
	unsigned int buf_guess_max;

	void *state; // Save all trie states header.
	unsigned int state_max;
	unsigned int state_used;
	unsigned int state_guess_max;

	ctrie_type_t type;
	unsigned int case_sensitive;

	unsigned int mem;
} ctrie_t;

#define ctrie_is_ready(_ctrie) ((_ctrie)->state_used)

#define ctrie_get_mem(_ctrie) ((_ctrie)->mem)

#define ctrie_get_state_used(_ctrie) ((_ctrie)->state_used)
#define ctrie_get_state_max(_ctrie) ((_ctrie)->state_max)
#define ctrie_get_state_guess_max(_ctrie) ((_ctrie)->state_guess_max)

#define ctrie_get_buf_used(_ctrie) ((_ctrie)->buf_used)
#define ctrie_get_buf_max(_ctrie) ((_ctrie)->buf_max)
#define ctrie_get_buf_guess_max(_ctrie) ((_ctrie)->buf_guess_max)

void ctrie_init(ctrie_t *ctrie, const ctrie_type_t ctrie_type, const unsigned int enable_case_sensitive);
void ctrie_exit(ctrie_t *ctrie);
ctrie_t *ctrie_alloc_sleep(const ctrie_type_t trie_type, const unsigned int enable_case_sensitive);
void ctrie_free(ctrie_t *ctrie);

int ctrie_build_by_desc_tbl(ctrie_t *ctrie, ctrie_desc_t *tbl, const unsigned int tbl_size);

void ctrie_debug(ctrie_t *ctrie);

ctrie_res_t ctrie_trans24(
	ctrie_ctx_t *ctx, const ctrie_t *ctrie,
	const uint8_t *buf, unsigned int buf_len, unsigned int *buf_used_len);
ctrie_res_t ctrie_trans(
	ctrie_ctx_t *ctx, const ctrie_t *ctrie,
	const uint8_t *buf, unsigned int buf_len, unsigned int *buf_used_len);

#endif /* CTRIE_H_ */
