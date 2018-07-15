/*!
 * \file md5sum.c
 * \brief MD5 database search algorithm implementation.
 * \details The source here is aimed to run in both user/kernel space.
 * Therefore, you won't find any specific env-dependend source here, and plz do not add any.
 *
 * \par Reference:
 * \li RFC 1321, "MD5 Message Digest Algorithm"
 *
 * \date Create: Dec 27, 2012
 * \author hac Ping-Jhih Chen
 */

#include "md5sum.h"

#define F1(x, y, z)     (z ^ (x & (y ^ z)))
#define F2(x, y, z)     F1(z, x, y)
#define F3(x, y, z)     (x ^ y ^ z)
#define F4(x, y, z)     (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, in, s) (w += f(x, y, z) + in, w = (w << s | w >> (32 - s)) + x)

/*!
 * \brief Calculate MD5 for incoming 64 Bytes block queue.
 * \param hash Previous calculated hash. This function will use the hash and update after transform.
 * \param bq Input block queue.
 *
 * \details Follow RFC 1321.
 */
static inline void md5sum_hash_calc(md5sum_hash_t *hash, md5sum_hash_t const *bq)
{
	md5sum_hash_t a, b, c, d;

	a = hash[0];
	b = hash[1];
	c = hash[2];
	d = hash[3];

	MD5STEP(F1, a, b, c, d, bq[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, bq[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, bq[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, bq[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, bq[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, bq[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, bq[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, bq[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, bq[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, bq[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, bq[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, bq[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, bq[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, bq[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, bq[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, bq[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, bq[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, bq[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, bq[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, bq[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, bq[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, bq[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, bq[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, bq[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, bq[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, bq[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, bq[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, bq[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, bq[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, bq[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, bq[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, bq[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, bq[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, bq[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, bq[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, bq[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, bq[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, bq[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, bq[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, bq[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, bq[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, bq[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, bq[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, bq[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, bq[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, bq[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, bq[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, bq[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, bq[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, bq[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, bq[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, bq[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, bq[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, bq[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, bq[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, bq[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, bq[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, bq[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, bq[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, bq[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, bq[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, bq[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, bq[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, bq[9] + 0xeb86d391, 21);

	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
}

#if CONFIG_MD5DB_BIG_ENDIAN
/*
 * big-endian, we have to convert byte order in this case.
 *
 * To follow RFC 1321, we have to use little-endian order in calculation.
 */
#define constant_swap32(x) ((md5sum_hash_t) ( \
	(((md5sum_hash_t)(x) & (md5sum_hash_t)0x000000ffUL) << 24) | \
	(((md5sum_hash_t)(x) & (md5sum_hash_t)0x0000ff00UL) <<  8) | \
	(((md5sum_hash_t)(x) & (md5sum_hash_t)0x00ff0000UL) >>  8) | \
	(((md5sum_hash_t)(x) & (md5sum_hash_t)0xff000000UL) >> 24)))

static inline void le32_to_cpu_array(md5sum_hash_t *buf, unsigned int no)
{
	while (no--)
	{
		*buf = constant_swap32(*buf);
		buf++;
	}
}

static inline void cpu_to_le32_array(md5sum_hash_t *buf, unsigned int no)
{
	while (no--)
	{
		*buf = constant_swap32(*buf);
		buf++;
	}
}

#else
/*
 * little-endian
 */
#define constant_swap32(x) (x)
#define le32_to_cpu_array(_buf, _no) do { } while (0)
#define cpu_to_le32_array(_buf, _no) do { } while (0)

#endif

#define md5sum_hash_calc_helper(_hash, _bq, _no) \
	do { \
		/* Input byte stream are always in big endian order, we have to convert them to little endian integers. */ \
		le32_to_cpu_array((md5sum_hash_t *) _bq, _no); \
		/* Then, calculate the MD5 hash. */ \
		md5sum_hash_calc((md5sum_hash_t *) _hash, (md5sum_hash_t *) _bq); \
	} while (0)

/*!
 * \brief Initialize input MD5 calculation structure.
 * \param cksum Input MD5 structure.
 *
 * \note You must use this function to initialize, or you will get invalid MD5 result.
 */
void md5sum_init(md5sum_t *cksum)
{
	cksum->processed_len = 0;
	cksum->bq_len = 0;

    cksum->hash[0] = 0x67452301;
    cksum->hash[1] = 0xefcdab89;
    cksum->hash[2] = 0x98badcfe;
    cksum->hash[3] = 0x10325476;
}

/*!
 * \brief Update MD5 by calculating incoming byte stream.
 * \param cksum Input MD5 structure.
 * \param in Input raw stream.
 * \param in_len Input raw stream length (bytes).
 *
 * \par Example:
 * \code

md5sum_t cksum;
char input[512];
md5sum_octet_t md5[MD5DB_MD5_OCTET_LEN];

int i;

md5sum_init(&cksum);

md5sum_update(&cksum, input, sizeof(input));
md5sum_complete(&cksum, md5, sizeof(md5));

for (i = 0; i < sizeof(md5); i++)
{
	printf("%02X", md5[i]);
}
 * \endcode
 *
 * \par Example (Multiple MD5 update):
 * \code

md5sum_t cksum;
char input[512];
md5sum_octet_t md5[MD5DB_MD5_OCTET_LEN];

int i;

md5sum_init(&cksum);

for (i = 0; i < sizeof(input); i++)
{
	md5sum_update(&cksum, input + i, 1);
}

md5sum_complete(&cksum, md5, sizeof(md5));

for (i = 0; i < sizeof(md5); i++)
{
	printf("%02X", md5[i]);
}

 * \endcode
 */
void md5sum_update(md5sum_t *cksum, const md5sum_octet_t *in, int in_len)
{
	int bq_avail;

	cksum->processed_len += in_len; // Totally processed length, this is used to calculate the MD5 result.

	for (;;)
	{
		if (in_len <= 0)
		{
			break; // No more input
		}

		/*
		 * Enqueue input stream.
		 */
		bq_avail = sizeof(cksum->bq) - cksum->bq_len;

		if (in_len < bq_avail)
		{
			/*
			 * Input byte stream is not enough, so we only have to enqueue the input.
			 */
			memcpy((md5sum_octet_t *) cksum->bq + cksum->bq_len, in, in_len);

			cksum->bq_len += in_len;

			break;
		}
		else
		{
			memcpy((md5sum_octet_t *) cksum->bq + cksum->bq_len, in, bq_avail);

//			cksum->bq_len += bq_avail; // redundant.

			in_len -= bq_avail;
			in += bq_avail;

			md5sum_hash_calc_helper(cksum->hash, cksum->bq, MD5DB_MD5_BQ_NO);

			cksum->bq_len = 0;
		}
	} // end for
}

/*!
 * \brief Get MD5 result after completing the calculation.
 * \param cksum Input md5 structure
 * \param buf[out] Input buffer to save the MD5 result.
 * \param buf_len Input buffer length
 * \return 0 if ok
 * \return < 0, otherwise.
 */
int md5sum_get_md5_octet(const md5sum_t *cksum, md5sum_octet_t *buf, int buf_len)
{
	if (buf_len < sizeof(cksum->hash))
	{
		return -1;
	}

	memcpy(buf, cksum->hash, sizeof(cksum->hash));

	return 0;
}
/*!
 * \brief Complete the MD5 calculation after invoking the last md5sum_update.
 *
 * \param cksum Input md5 structure.
 * \param buf   Input buffer to save the MD5 result. If you don't need the result, set the buffer to NULL.
 * \param buf_len Input buffer length. If you don't need the result, set this to zero.
 *
 * \return 0 if ok
 * \return < 0, otherwise.
 *
 * \sa md5sum_update
 */
int md5sum_complete(md5sum_t *cksum, md5sum_octet_t *buf, int buf_len)
{
	md5sum_octet_t *p;
	int padding;

	/*
	 * Add the last 0x80 to terminate MD5 stream.
	 */
	p = (md5sum_octet_t *) cksum->bq + cksum->bq_len;
	*p = 0x80;
	cksum->bq_len++;

	p++; // shift 1 byte.

	/*
	 * The last 2 block is from total processed length, so we have to take care of block overflow.
	 *
	 * * NOTE: This is following RFC 1321.
	 */
	if (sizeof(cksum->bq) - cksum->bq_len < (sizeof(md5sum_hash_t) * 2))
	{
		// Not enough space for the last 2 hash, flush this buffer queue
		memset((md5sum_octet_t *) cksum->bq + cksum->bq_len, 0x00, sizeof(cksum->bq) - cksum->bq_len);

		md5sum_hash_calc_helper(cksum->hash, cksum->bq, MD5DB_MD5_BQ_NO);

		/*
		 * And reset the buffer queue so that we can add zero padding in next step.
		 */
		cksum->bq_len = 0;
	}

	/* Add padding */
	padding = sizeof(cksum->bq) - cksum->bq_len;
	p = (md5sum_octet_t *) cksum->bq + cksum->bq_len;

	memset(p, 0x00, padding);

	cksum->bq[MD5DB_MD5_BQ_NO - 2] = cksum->processed_len << 3;
	cksum->bq[MD5DB_MD5_BQ_NO - 1] = cksum->processed_len >> 29;

	md5sum_hash_calc_helper(cksum->hash, cksum->bq, MD5DB_MD5_BQ_NO - 2);

	/* Convert hash back to little endian. */
	cpu_to_le32_array(cksum->hash, MD5DB_MD5_HASH_NO);

	md5sum_get_md5_octet(cksum, buf, buf_len);

	return 0; // ok
}


/*!
 * \brief Complete the MD5 calculation after invoking the last md5sum_update.
 *
 * \param cksum Input md5 structure.
 * \param buf   Input buffer to save the MD5 result. If you don't need the result, set the buffer to NULL.
 * \param buf_len Input buffer length. If you don't need the result, set this to zero.
 *
 * \return 0 if ok
 * \return < 0, otherwise.
 *
 * \sa md5sum_update
 */
int md5sum_temporary_complete(md5sum_t *cksum_safe, md5sum_octet_t *buf, int buf_len)
{
	md5sum_octet_t *p;
	int padding;

	md5sum_t cksum;

	memcpy(&cksum, cksum_safe, sizeof(cksum));

	/*
	 * Add the last 0x80 to terminate MD5 stream.
	 */
	p = (md5sum_octet_t *) cksum.bq + cksum.bq_len;
	*p = 0x80;
	cksum.bq_len++;

	p++; // shift 1 byte.

	/*
	 * The last 2 block is from total processed length, so we have to take care of block overflow.
	 *
	 * * NOTE: This is following RFC 1321.
	 */
	if (sizeof(cksum.bq) - cksum.bq_len < (sizeof(md5sum_hash_t) * 2))
	{
		// Not enough space for the last 2 hash, flush this buffer queue
		memset((md5sum_octet_t *) cksum.bq + cksum.bq_len, 0x00, sizeof(cksum.bq) - cksum.bq_len);

		md5sum_hash_calc_helper(cksum.hash, cksum.bq, MD5DB_MD5_BQ_NO);

		/*
		 * And reset the buffer queue so that we can add zero padding in next step.
		 */
		cksum.bq_len = 0;
	}

	/* Add padding */
	padding = sizeof(cksum.bq) - cksum.bq_len;
	p = (md5sum_octet_t *) cksum.bq + cksum.bq_len;

	memset(p, 0x00, padding);

	cksum.bq[MD5DB_MD5_BQ_NO - 2] = cksum.processed_len << 3;
	cksum.bq[MD5DB_MD5_BQ_NO - 1] = cksum.processed_len >> 29;

	md5sum_hash_calc_helper(cksum.hash, cksum.bq, MD5DB_MD5_BQ_NO - 2);

	/* Convert hash back to little endian. */
	cpu_to_le32_array(cksum.hash, MD5DB_MD5_HASH_NO);

	md5sum_get_md5_octet(&cksum, buf, buf_len);

	return 0; // ok
}

