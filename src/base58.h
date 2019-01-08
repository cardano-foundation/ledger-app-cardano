#ifndef H_CARDANO_APP_BASE58
#define H_CARDANO_APP_BASE58

unsigned char encode_base58(unsigned char *in, unsigned char length,
                            unsigned char *out, unsigned char maxoutlen);

void run_base58_test();
#endif