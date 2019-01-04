#ifndef H_CARDANO_APP_TEST_UTILS
#define H_CARDANO_APP_TEST_UTILS

#include <stdbool.h>
#include "assert.h"

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
      EXPECT_EQ(has_thrown, true); \
    } \
  } END_TRY;\
}


// TODO(ppershing): should we move this to assert.h?

// from https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros
#define _TO_STR1_(x) #x
#define _TO_STR2_(x) _TO_STR1_(x)

#define EXPECT_EQ(expr, result) ASSERT(expr == result, __FILE__ ":" _TO_STR2_(__LINE__))



// Assertion that expr does not throw (except for another assert)

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
          snprintf(msg, sizeof(msg), "%s:%d 0x%x", __FILE__, __LINE__, e); \
          ASSERT(false, msg); \
      } FINALLY { \
      } \
    } END_TRY; \
  }
#endif
