#include "securityPolicy.h"

security_policy_t policyForGetExtendedPublicKey(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidAccount(pathSpec)) return POLICY_DENY;

	return POLICY_PROMPT_WARN_UNUSUAL; //BEFORE_RESPONSE;
}

security_policy_t policyForReturnDeriveAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidAccount(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidChainType(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAddress(pathSpec)) return POLICY_DENY;

	return POLICY_PROMPT_BEFORE_RESPONSE;
}

security_policy_t policyForShowDeriveAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidAccount(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidChainType(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAddress(pathSpec)) return POLICY_DENY;

	return POLICY_ALLOW;
}

security_policy_t policyForAttestUtxo()
{
	return POLICY_ALLOW;
}
