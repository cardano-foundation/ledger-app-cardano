#ifndef H_CARDANO_APP_BLAKE2B
#define H_CARDANO_APP_BLAKE2B

#include "common.h"

// This file provides convenience functions for using firmware hashing api

// Note: We would like to make this static const but
// it does not play well with inline functions
enum {
	BLAKE2B_224_SIZE = 28,
	BLAKE2B_256_SIZE = 32,
	BLAKE2B_512_SIZE = 64,

	SHA3_256_SIZE = 32,
};


enum {
	HASH_CONTEXT_INITIALIZED_MAGIC = 12345,
};

#define __CIPHER_DECLARE(CIPHER, cipher, bits) \
	typedef struct { \
		uint16_t initialized_magic; \
		cx_##cipher##_t cx_ctx; \
	} cipher##_##bits##_context_t;\
	\
	inline void cipher##_##bits##_init( \
	                                    cipher##_##bits##_context_t* ctx \
	                                  ) \
	{ \
		STATIC_ASSERT( bits == CIPHER##_##bits##_SIZE * 8, "bad cipher size"); \
		cx_##cipher##_init( \
		                    & ctx->cx_ctx, \
		                    CIPHER##_##bits##_SIZE * 8 \
		                  );\
		ctx->initialized_magic = HASH_CONTEXT_INITIALIZED_MAGIC; \
	} \
	\
	inline void cipher##_##bits##_append( \
	                                      cipher##_##bits##_context_t* ctx, \
	                                      const uint8_t* inBuffer, size_t inSize \
	                                    ) { \
		ASSERT(ctx->initialized_magic == HASH_CONTEXT_INITIALIZED_MAGIC); \
		cx_hash( \
		         & ctx->cx_ctx.header, \
		         0, /* Do not output the hash, yet */ \
		         inBuffer, \
		         inSize, \
		         NULL, 0 \
		       ); \
	} \
	\
	inline void cipher##_##bits##_finalize( \
	                                        cipher##_##bits##_context_t* ctx, \
	                                        uint8_t* outBuffer, size_t outSize \
	                                      ) { \
		ASSERT(ctx->initialized_magic == HASH_CONTEXT_INITIALIZED_MAGIC); \
		ASSERT(outSize == CIPHER##_##bits##_SIZE); \
		cx_hash( \
		         & ctx->cx_ctx.header, \
		         CX_LAST, /* Output the hash */ \
		         NULL, \
		         0, \
		         outBuffer, \
		         CIPHER##_##bits##_SIZE \
		       ); \
	} \
	/* Convenience function to make all in one step */ \
	inline void cipher##_##bits##_hash( \
	                                    const uint8_t* inBuffer, size_t inSize, \
	                                    uint8_t* outBuffer, size_t outSize \
	                                  ) { \
		ASSERT(inSize < BUFFER_SIZE_PARANOIA); \
		ASSERT(outSize == CIPHER##_##bits##_SIZE); \
		cipher##_##bits##_context_t ctx; \
		cipher##_##bits##_init(&ctx); \
		/* Note: This could be done by single cx_hash call */ \
		/* But we don't really care */ \
		cipher##_##bits##_append(&ctx, inBuffer, inSize); \
		cipher##_##bits##_finalize(&ctx, outBuffer, outSize); \
	}

__CIPHER_DECLARE(BLAKE2B, blake2b, 224)
__CIPHER_DECLARE(BLAKE2B, blake2b, 256)
__CIPHER_DECLARE(BLAKE2B, blake2b, 512)

__CIPHER_DECLARE(SHA3, sha3, 256)


void run_hash_test();
#endif
