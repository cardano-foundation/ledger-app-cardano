#ifndef H_CARDANO_APP_BLAKE2B
#define H_CARDANO_APP_BLAKE2B

#include <os.h>
#include "assert.h"

// This file provides convenience functions for using firmware hashing api

// Note: We would like to make this static const but
// it does not play well with inline functions
enum {
	BLAKE2B224_SIZE = 28,
	BLAKE2B256_SIZE = 32,
	BLAKE2B512_SIZE = 64,
};

#define __BLAKE_DECLARE(bits) \
	typedef struct { \
		cx_blake2b_t cx_ctx; \
	} blake2b##bits##_context_t;\
	\
	inline void blake2b##bits##_init( \
		blake2b##bits##_context_t* ctx \
	) \
	{ \
		STATIC_ASSERT( bits == BLAKE2B##bits##_SIZE * 8, __inconsistent_size); \
		cx_blake2b_init( \
			& ctx->cx_ctx, \
		BLAKE2B##bits##_SIZE * 8 \
		);\
	} \
	\
	inline void blake2b##bits##_append( \
		blake2b##bits##_context_t* ctx, \
		const uint8_t* data, size_t dataLen \
	) { \
		cx_hash( \
			& ctx->cx_ctx.header, \
			0, /* Do not output the hash, yet */ \
			data, \
			dataLen, \
			NULL, 0 \
		); \
	} \
	\
	inline void blake2b##bits##_finalize( \
		blake2b##bits##_context_t* ctx, \
		uint8_t* output, size_t outputLen \
	) { \
		ASSERT(outputLen == BLAKE2B##bits##_SIZE); \
		cx_hash( \
			& ctx->cx_ctx.header, \
			CX_LAST, /* Output the hash */ \
			NULL, \
			0, \
			output, \
			BLAKE2B##bits##_SIZE \
		); \
	} \
	/* Convenience function to make all in one step */ \
	inline void blake2b##bits##_hash( \
		const uint8_t* input, size_t inputLen, \
		uint8_t* output, size_t outputLen \
	) { \
	    ASSERT(outputLen == BLAKE2B##bits##_SIZE); \
	    blake2b##bits##_context_t ctx; \
	    blake2b##bits##_init(&ctx); \
	    /* Note: This could be done by single cx_hash call */ \
	    /* But we don't really care */ \
	    blake2b##bits##_append(&ctx, input, inputLen); \
	    blake2b##bits##_finalize(&ctx, output, outputLen); \
	}

__BLAKE_DECLARE(224)
__BLAKE_DECLARE(256)
__BLAKE_DECLARE(512)

void run_hash_test();
#endif
