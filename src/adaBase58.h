#ifndef H_CARDANO_APP_ADABASE58
#define H_CARDANO_APP_ADABASE58

unsigned char ada_encode_base58(unsigned char *in, unsigned char length,
                                unsigned char *out, unsigned char maxoutlen);

void run_adaBase58_test();
#endif