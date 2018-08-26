/*!
 * \copydoc md5sum.c
 */

#ifndef MD5DB_H_
#define MD5DB_H_

#if 0
#define CONFIG_MD5DB_BIG_ENDIAN 1
#else
#define CONFIG_MD5DB_BIG_ENDIAN 0
#endif

#define CONFIG_MD5DB_KERNEL_SPACE 0

#if !CONFIG_MD5DB_KERNEL_SPACE
#define CONFIG_MD5DB_USER_SPACE 1
#endif

#if CONFIG_MD5DB_KERNEL_SPACE

/*
 * Kernel space
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>

typedef u8 md5sum_octet_t;
typedef u16 md5sum_word_t;
typedef u32 md5sum_hash_t; // unsigned plz

#else
/*
 * User space
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t md5sum_octet_t;
typedef uint16_t md5sum_word_t;
typedef uint32_t md5sum_hash_t; // unsigned plz

#endif


////////////////////////////////////////////////////////////////////////////////

#define MD5DB_MD5_OCTET_LEN 16 //!< MD5 length (bytes)
#define MD5DB_MD5_OCTET_STR_LEN (MD5DB_MD5_OCTET_LEN * 2) //!< MD5 string length (bytes), i.e. 0x0A 1 byte octet -> "0A" 2 bytes string

#define MD5DB_MD5_OCTET_NO (MD5DB_MD5_OCTET_LEN / 1) //!< Hmm, this's possibly redundant.
#define MD5DB_MD5_WORD_NO (MD5DB_MD5_OCTET_LEN / sizeof(md5sum_word_t)) //!< 1 word = 2 MD5 octet (Use the term word: 1 word is usually 2 bytes)
#define MD5DB_MD5_HASH_NO (MD5DB_MD5_OCTET_LEN / sizeof(md5sum_hash_t)) //!< 1 hash value in MD5 checksum is 4 bytes

#define MD5DB_MD5_BQ_NO (16) //!< Number of block/buffer queue. Do not change this.

////////////////////////////////////////////////////////////////////////////////

/*!
 * \brief MD5 digest/cksum calculation structure.
 */
typedef struct md5_cksum
{
	/*
	 * A buffer queue to save input binary stream to calculate MD5.
	 */
	md5sum_hash_t bq[MD5DB_MD5_BQ_NO]; //!< Keep 64 bytes buffer for the caller. DO NOT change this. We must follow RFC 1321.
	unsigned short bq_len; //!< * FIXME: Use 64 bits here. The byte count for data_queue

	unsigned long processed_len; //!< Totally processed length of input stream.

	/*
	 * Currently calculated MD5 checksum.
	 */
	md5sum_hash_t hash[MD5DB_MD5_HASH_NO]; //!< Save temporary MD5 result here.
} md5sum_t;

////////////////////////////////////////////////////////////////////////////////

/*!
 * \defgroup GRP_MD5SUM MD5 Hash Algorithm.
 * \{
 */
void md5sum_init(md5sum_t *cksum);
void md5sum_update(md5sum_t *cksum, const md5sum_octet_t *in, int len);
int md5sum_get_md5_octet(const md5sum_t *cksum, md5sum_octet_t *buf, int buf_len);
int md5sum_complete(md5sum_t *cksum, md5sum_octet_t *buf, int buf_len);
/*! \} */

////////////////////////////////////////////////////////////////////////////////


#endif /* MD5DB_H_ */
