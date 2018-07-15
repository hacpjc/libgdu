/*
 * aes.h
 *
 *  Created on: 2012/1/6
 *      Author: hac Ping-Jhih Chen
 */


/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2006-2007  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  The AES block cipher was designed by Vincent Rijmen and Joan Daemen.
 *
 *  http://csrc.nist.gov/encryption/aes/rijndael/Rijndael.pdf
 *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */

/**
 * \file aes.h
 */
#ifndef XYSSL_AES_H
#define XYSSL_AES_H

// config .h //


#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/*
 * Uncomment if native integers are 8-bit wide.
 *
 #define XYSSL_HAVE_INT8
 */

/*
 * Uncomment if native integers are 16-bit wide.
 *
 #define XYSSL_HAVE_INT16
 */

/*
 * Uncomment if the compiler supports long long.
 *
 #define XYSSL_HAVE_LONGLONG
 */

/*
 * Uncomment to enable the use of assembly code.
 */
#define XYSSL_HAVE_ASM

/*
 * Uncomment if the CPU supports SSE2 (IA-32 specific).
 *
 #define XYSSL_HAVE_SSE2
 */

/*
 * Enable all SSL/TLS debugging messages.
 */
#define XYSSL_DEBUG_MSG

/*
 * Enable the checkup functions (*_self_test).
 */
#define XYSSL_SELF_TEST

/*
 * Enable the prime-number generation code.
 */
#define XYSSL_GENPRIME

/*
 * Uncomment this macro to store the AES tables in ROM.
 *
 #define XYSSL_AES_ROM_TABLES
 */

/*
 * Module:  library/aes.c
 * Caller:  library/ssl_tls.c
 *
 * This module enables the following ciphersuites:
 *      SSL_RSA_AES_128_SHA
 *      SSL_RSA_AES_256_SHA
 *      SSL_EDH_RSA_AES_256_SHA
 */
#define XYSSL_AES_C

/*
 * Module:  library/arc4.c
 * Caller:  library/ssl_tls.c
 *
 * This module enables the following ciphersuites:
 *      SSL_RSA_RC4_128_MD5
 *      SSL_RSA_RC4_128_SHA
 */
#define XYSSL_ARC4_C

/*
 * Module:  library/base64.c
 * Caller:  library/x509parse.c
 *
 * This module is required for X.509 support.
 */
#define XYSSL_BASE64_C

/*
 * Module:  library/bignum.c
 * Caller:  library/dhm.c
 *          library/rsa.c
 *          library/ssl_tls.c
 *          library/x509parse.c
 *
 * This module is required for RSA and DHM support.
 */
#define XYSSL_BIGNUM_C

/*
 * Module:  library/certs.c
 * Caller:
 *
 * This module is used for testing (ssl_client/server).
 */
#define XYSSL_CERTS_C

/*
 * Module:  library/debug.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *          library/ssl_tls.c
 *
 * This module provides debugging functions.
 */
#define XYSSL_DEBUG_C

/*
 * Module:  library/des.c
 * Caller:  library/ssl_tls.c
 *
 * This module enables the following ciphersuites:
 *      SSL_RSA_DES_168_SHA
 *      SSL_EDH_RSA_DES_168_SHA
 */
#define XYSSL_DES_C

/*
 * Module:  library/dhm.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *
 * This module enables the following ciphersuites:
 *      SSL_EDH_RSA_DES_168_SHA
 *      SSL_EDH_RSA_AES_256_SHA
 */
#define XYSSL_DHM_C

/*
 * Module:  library/havege.c
 * Caller:
 *
 * This module enables the HAVEGE random number generator.
 */
#define XYSSL_HAVEGE_C

/*
 * Module:  library/md2.c
 * Caller:  library/x509parse.c
 *
 * Uncomment to enable support for (rare) MD2-signed X.509 certs.
 *
 #define XYSSL_MD2_C
 */

/*
 * Module:  library/md4.c
 * Caller:  library/x509parse.c
 *
 * Uncomment to enable support for (rare) MD4-signed X.509 certs.
 *
 #define XYSSL_MD4_C
 */

/*
 * Module:  library/md5.c
 * Caller:  library/ssl_tls.c
 *          library/x509parse.c
 *
 * This module is required for SSL/TLS and X.509.
 */
#define XYSSL_MD5_C

/*
 * Module:  library/net.c
 * Caller:
 *
 * This module provides TCP/IP networking routines.
 */
#define XYSSL_NET_C

/*
 * Module:  library/padlock.c
 * Caller:  library/aes.c
 *
 * This modules adds support for the VIA PadLock on x86.
 */
#define XYSSL_PADLOCK_C

/*
 * Module:  library/rsa.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *          library/ssl_tls.c
 *          library/x509.c
 *
 * This module is required for SSL/TLS and MD5-signed certificates.
 */
#define XYSSL_RSA_C

/*
 * Module:  library/sha1.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *          library/ssl_tls.c
 *          library/x509parse.c
 *
 * This module is required for SSL/TLS and SHA1-signed certificates.
 */
#define XYSSL_SHA1_C

/*
 * Module:  library/sha2.c
 * Caller:
 *
 * This module adds support for SHA-224 and SHA-256.
 */
#define XYSSL_SHA2_C

/*
 * Module:  library/sha4.c
 * Caller:
 *
 * This module adds support for SHA-384 and SHA-512.
 */
#define XYSSL_SHA4_C

/*
 * Module:  library/ssl_cli.c
 * Caller:
 *
 * This module is required for SSL/TLS client support.
 */
#define XYSSL_SSL_CLI_C

/*
 * Module:  library/ssl_srv.c
 * Caller:
 *
 * This module is required for SSL/TLS server support.
 */
#define XYSSL_SSL_SRV_C

/*
 * Module:  library/ssl_tls.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *
 * This module is required for SSL/TLS.
 */
#define XYSSL_SSL_TLS_C

/*
 * Module:  library/timing.c
 * Caller:  library/havege.c
 *
 * This module is used by the HAVEGE random number generator.
 */
#define XYSSL_TIMING_C

/*
 * Module:  library/x509parse.c
 * Caller:  library/ssl_cli.c
 *          library/ssl_srv.c
 *          library/ssl_tls.c
 *
 * This module is required for X.509 certificate parsing.
 */
#define XYSSL_X509_PARSE_C

/*
 * Module:  library/x509_write.c
 * Caller:
 *
 * This module is required for X.509 certificate writing.
 */
#define XYSSL_X509_WRITE_C

//////////////////


#define AES_ENCRYPT     1
#define AES_DECRYPT     0

/**
 * \brief          AES context structure
 */
typedef struct {
	int nr; /*!<  number of rounds  */
	unsigned long *rk; /*!<  AES round keys    */
	unsigned long buf[68]; /*!<  unaligned data    */
} aes_context;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          AES key schedule (encryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      encryption key
 * \param keysize  must be 128, 192 or 256
 */
void aes_setkey_enc(aes_context *ctx, unsigned char *key, int keysize);

/**
 * \brief          AES key schedule (decryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      decryption key
 * \param keysize  must be 128, 192 or 256
 */
void aes_setkey_dec(aes_context *ctx, unsigned char *key, int keysize);

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 */
void aes_crypt_ecb(aes_context *ctx, int mode, unsigned char input[16], unsigned char output[16]);

/**
 * \brief          AES-CBC buffer encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
void aes_crypt_cbc(aes_context *ctx, int mode, int length, unsigned char iv[16], unsigned char *input, unsigned char *output);

/**
 * \brief          AES-CFB buffer encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
void aes_crypt_cfb(aes_context *ctx, int mode, int length, int *iv_off, unsigned char iv[16], unsigned char *input, unsigned char *output);

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int aes_self_test(int verbose);

#ifdef __cplusplus
}
#endif

#endif /* aes.h */
