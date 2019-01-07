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

#define EXPECT_EQ_BYTES(ptr1, ptr2, len) ASSERT(os_memcmp(ptr1, ptr2, len) == 0);

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
