#ifndef H_CARDANO_APP_SECURITY_POLICY
#define H_CARDANO_APP_SECURITY_POLICY

#include "bip44.h"

typedef enum {
	POLICY_ALLOW = 1,
	POLICY_DENY = 2,
	POLICY_PROMPT_BEFORE_RESPONSE = 3,
	POLICY_PROMPT_WARN_UNUSUAL = 4,
} security_policy_t;

security_policy_t policyForGetExtendedPublicKey(const bip44_path_t* pathSpec);

security_policy_t policyForShowDeriveAddress(const bip44_path_t* pathSpec);
security_policy_t policyForReturnDeriveAddress(const bip44_path_t* pathSpec);

security_policy_t policyForAttestUtxo();

#endif
