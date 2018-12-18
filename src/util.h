#ifndef H_CARDANO_UTILS

#define STATIC_ASSERT(COND,MSG) typedef char static_assert__##MSG[(COND)?1:-1] __attribute__((unused))

#endif
