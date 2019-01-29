#include "securityPolicy.h"

security_policy_t policyForGetExtendedPublicKey(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidCardanoPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAccount(pathSpec)) return POLICY_DENY;

	if (!bip44_hasReasonableAccount(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	// Normally extPubKey is asked only for an account
	if (bip44_containsChainType(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	return POLICY_PROMPT_BEFORE_RESPONSE;
}


security_policy_t policyForReturnDeriveAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidCardanoPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAccount(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidChainType(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAddress(pathSpec)) return POLICY_DENY;

	if (!bip44_hasReasonableAccount(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (!bip44_hasReasonableAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (bip44_containsMoreThanAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	return POLICY_PROMPT_BEFORE_RESPONSE;
}

security_policy_t policyForShowDeriveAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidCardanoPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAccount(pathSpec)) return POLICY_DENY;
	if (!bip44_hasValidChainType(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAddress(pathSpec)) return POLICY_DENY;

	if (!bip44_hasReasonableAccount(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (!bip44_hasReasonableAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (bip44_containsMoreThanAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	return POLICY_SHOW_BEFORE_RESPONSE;
}


security_policy_t policyForAttestUtxo()
{
	return POLICY_ALLOW;
}



security_policy_t policyForSignTxInit()
{
	return POLICY_PROMPT_BEFORE_RESPONSE;
}

security_policy_t policyForSignTxInput()
{
	return POLICY_ALLOW;
}

security_policy_t policyForSignTxOutputAddress(
        const uint8_t* rawAddressBuffer MARK_UNUSED, size_t rawAddressSize MARK_UNUSED
)
{
	return POLICY_SHOW_BEFORE_RESPONSE;
}

security_policy_t policyForSignTxOutputPath(const bip44_path_t* pathSpec)
{
	if (!bip44_hasValidCardanoPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAccount(pathSpec)) return POLICY_DENY;

	if (!bip44_hasValidChainType(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAddress(pathSpec)) return POLICY_DENY;

	// Note: we don't need warning as these will be displayed as addresses, not paths
	if (!bip44_hasReasonableAccount(pathSpec)) return POLICY_SHOW_BEFORE_RESPONSE;
	if (!bip44_hasReasonableAddress(pathSpec)) return POLICY_SHOW_BEFORE_RESPONSE;
	if (bip44_containsMoreThanAddress(pathSpec)) return POLICY_SHOW_BEFORE_RESPONSE;

	return POLICY_ALLOW;
}

security_policy_t policyForSignTxFee(uint64_t fee MARK_UNUSED)
{
	return POLICY_SHOW_BEFORE_RESPONSE;
}

security_policy_t policyForSignTxWitness(const bip44_path_t* pathSpec)
{
	PRINTF("policy for sign tx witness\n");
	if (!bip44_hasValidCardanoPrefix(pathSpec)) return POLICY_DENY;
	if (!bip44_containsAccount(pathSpec)) return POLICY_DENY;

	// Perhaps these could be relaxed?
	if (!bip44_hasReasonableAccount(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (!bip44_hasValidChainType(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;
	if (!bip44_containsAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	if (bip44_containsMoreThanAddress(pathSpec)) return POLICY_PROMPT_WARN_UNUSUAL;

	return POLICY_ALLOW;
}
