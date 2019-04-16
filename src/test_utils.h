#ifndef H_CARDANO_APP_TEST_UTILS
#define H_CARDANO_APP_TEST_UTILS

#include <stdbool.h>
#include "assert.h"

// Assert that expression throws a specific error
#define EXPECT_THROWS(expr, error_code) \
	{ \
		/* __label__ is gcc extension that defines a local label. */ \
		/* It allows us to have multiple EXPECT_THROWS in one function */ \
		__label__ __FINALLYEX; \
		BEGIN_TRY { \
			bool has_thrown = false; \
			TRY { \
				expr; \
			} CATCH(error_code) { \
				has_thrown = true; \
			} CATCH_OTHER(e) { \
				/* pass */ \
			} FINALLY { \
				ASSERT(has_thrown); \
			} \
		} END_TRY;\
	}

// Assert that expression returns correct result
#define EXPECT_EQ(expr, result) ASSERT(expr == result)

// Assert that first len bytes are same
#define EXPECT_EQ_BYTES(ptr1, ptr2, size) ASSERT(os_memcmp(ptr1, ptr2, size) == 0);

// Assert that streams have have available content
#define EXPECT_EQ_STREAM(s1_ptr, s2_ptr) \
	{ \
		size_t l1 = stream_availableBytes(s1_ptr); \
		size_t l2 = stream_availableBytes(s2_ptr); \
		EXPECT_EQ(l1, l2); \
		EXPECT_EQ_BYTES(stream_head(s1_ptr), stream_head(s2_ptr), l1); \
	}


// Assert that expression does not throw (except for another assert)
#define BEGIN_ASSERT_NOEXCEPT \
	{ \
		__label__ __FINALLYEX; \
		BEGIN_TRY { \
			TRY

#define END_ASSERT_NOEXCEPT \
	CATCH (ERR_ASSERT) { \
		/* pass thorugh assertions */ \
		THROW(ERR_ASSERT); \
	} CATCH_OTHER (e) { \
		char msg[50]; \
		snprintf(msg, sizeof(msg), "%s:%d x%x", __FILE__, __LINE__, e); \
		ASSERT_WITH_MSG(false, msg); \
	} FINALLY { \
	} \
	} END_TRY; \
	}


#endif
