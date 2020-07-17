#include "addressUtilsShelley.h"
#include "addressUtilsByron.h"
#include "securityPolicy.h"
#include "bip44.h"

// Warning: following helper macros assume "pathSpec" in the context

// Helper macros

static inline bool spending_path_is_consistent_with_address_type(address_type_t addressType, const bip44_path_t* spendingPath)
{
#define CHECK(cond) if (!(cond)) return false
	// Byron derivation path is only valid for a Byron address
	// the rest should be Shelley derivation scheme
	if (addressType == BYRON) {
		CHECK(bip44_hasByronPrefix(spendingPath));
	} else {
		CHECK(bip44_hasShelleyPrefix(spendingPath));
	}

	if (addressType == REWARD) {
		CHECK(bip44_isValidStakingKeyPath(spendingPath));
	} else {
		// for all non-reward addresses, we check chain type and length
		CHECK(bip44_hasValidChainTypeForAddress(spendingPath));
		CHECK(bip44_containsAddress(spendingPath));
	}

	return true;
#undef CHECK
}

static inline bool staking_info_is_valid(const addressParams_t* addressParams)
{
#define CHECK(cond) if (!(cond)) return false
	CHECK(isStakingInfoConsistentWithAddressType(addressParams));
	if (addressParams->stakingChoice == STAKING_KEY_PATH) {
		CHECK(bip44_isValidStakingKeyPath(&addressParams->stakingKeyPath));
	}
	return true;
#undef CHECK
}

static inline bool has_cardano_prefix_and_any_account(const bip44_path_t* pathSpec)
{
	return bip44_hasValidCardanoPrefix(pathSpec) &&
	       bip44_containsAccount(pathSpec);
}

static inline bool has_valid_change_and_any_address(const bip44_path_t* pathSpec)
{
	return bip44_hasValidChainTypeForAddress(pathSpec) &&
	       bip44_containsAddress(pathSpec);
}

// Both account and address are from small brute-forcable range
static inline bool has_reasonable_account_and_address(const bip44_path_t* pathSpec)
{
	return bip44_hasReasonableAccount(pathSpec) &&
	       bip44_hasReasonableAddress(pathSpec);
}

static inline bool is_too_deep(const bip44_path_t* pathSpec)
{
	return bip44_containsMoreThanAddress(pathSpec);
}

#define DENY_IF(expr)      if (expr)    return POLICY_DENY;
#define DENY_UNLESS(expr)  if (!(expr)) return POLICY_DENY;
#define WARN_IF(expr)      if (expr)    return POLICY_PROMPT_WARN_UNUSUAL;
#define PROMPT_IF(expr)    if (expr)    return POLICY_PROMPT_BEFORE_RESPONSE;
#define ALLOW_IF(expr)     if (expr)    return POLICY_ALLOW_WITHOUT_PROMPT;
#define SHOW_IF(expr)      if (expr)    return POLICY_SHOW_BEFORE_RESPONSE;

// Get extended public key and return it to the host
security_policy_t policyForGetExtendedPublicKey(const bip44_path_t* pathSpec)
{
	DENY_UNLESS(has_cardano_prefix_and_any_account(pathSpec));

	WARN_IF(!bip44_hasReasonableAccount(pathSpec));
	// Normally extPubKey is asked only for an account
	WARN_IF(bip44_containsChainType(pathSpec));

	PROMPT_IF(true);
}

// Derive address and return it to the host
security_policy_t policyForReturnDeriveAddress(addressParams_t* addressParams)
{
	DENY_UNLESS(has_cardano_prefix_and_any_account(&addressParams->spendingKeyPath));
	DENY_UNLESS(spending_path_is_consistent_with_address_type(addressParams->type, &addressParams->spendingKeyPath));
	DENY_UNLESS(staking_info_is_valid(addressParams));

	WARN_IF(!has_reasonable_account_and_address(&addressParams->spendingKeyPath));
	WARN_IF(is_too_deep(&addressParams->spendingKeyPath)); // TODO change to DENY?

	PROMPT_IF(true);
}

// Derive address and show it to the user
security_policy_t policyForShowDeriveAddress(addressParams_t* addressParams)
{
	DENY_UNLESS(has_cardano_prefix_and_any_account(&addressParams->spendingKeyPath));
	DENY_UNLESS(spending_path_is_consistent_with_address_type(addressParams->type, &addressParams->spendingKeyPath));
	DENY_UNLESS(staking_info_is_valid(addressParams));

	WARN_IF(!has_reasonable_account_and_address(&addressParams->spendingKeyPath));
	WARN_IF(is_too_deep(&addressParams->spendingKeyPath)); // TODO change to DENY?

	SHOW_IF(true);
}


// Initiate transaction signing
security_policy_t policyForSignTxInit()
{
	// Could be switched to POLICY_ALLOW_WITHOUT_PROMPT to skip initial "new transaction" question
	PROMPT_IF(true);
}

// For each transaction UTxO input
security_policy_t policyForSignTxInput()
{
	// No need to check tx inputs
	ALLOW_IF(true);
}

// For each transaction (third-party) address output
security_policy_t policyForSignTxOutputAddress(
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        const uint8_t networkId, const uint32_t protocolMagic
)
{
	ASSERT(rawAddressSize >= 1);
	address_type_t addressType = getAddressType(rawAddressBuffer[0]);
	if (addressType == BYRON) {
		uint32_t addressProtocolMagic = extractProtocolMagic(rawAddressBuffer, rawAddressSize);
		DENY_IF(addressProtocolMagic != protocolMagic);
	} else { // shelley
		uint8_t addressNetworkId = getNetworkId(rawAddressBuffer[0]);
		DENY_IF(addressNetworkId != networkId);
		DENY_IF(addressType == REWARD);
	}

	// We always show third-party output addresses
	SHOW_IF(true);
}

// For each output given by derivation path
security_policy_t policyForSignTxOutputAddressParams(
        const addressParams_t* params,
        const uint8_t networkId, const uint32_t protocolMagic
)
{
	DENY_UNLESS(has_cardano_prefix_and_any_account(&params->spendingKeyPath));
	DENY_UNLESS(has_valid_change_and_any_address(&params->spendingKeyPath));

	if (params->type == BYRON) {
		// TODO deny if protocol magic is inconsistent with the given one
	} else { // shelley
		DENY_IF(params->networkId != networkId);
	}

	// TODO should we allow these at all?
	SHOW_IF(!has_reasonable_account_and_address(&params->spendingKeyPath))
	SHOW_IF(is_too_deep(&params->spendingKeyPath));

	// TODO checks for staking info?
	// TODO determine what is a change output (not to be shown)?

	ALLOW_IF(true);
}

// For transaction fee
security_policy_t policyForSignTxFee(uint64_t fee)
{
	// always show the fee
	SHOW_IF(true);
}

// For transaction TTL
security_policy_t policyForSignTxTtl(uint32_t ttl)
{
	// ttl == 0 will not be accepted by a node
	// and indicates a likely bug somewhere
	DENY_IF(ttl == 0);

	// might be changed to POLICY_ALLOW_WITHOUT_PROMPT
	// to avoid bothering the user with TTL
	// (Daedalus does not show this)
	SHOW_IF(true);
}

// For each certificate
security_policy_t policyForSignTxCertificate(const uint8_t certificateType, const bip44_path_t* stakingKeyPath)
{
	DENY_UNLESS(bip44_isValidStakingKeyPath(stakingKeyPath));

	PROMPT_IF(true);
}

// For each withdrawal
security_policy_t policyForSignTxWithdrawal()
{
	// No need to check withdrawals
	ALLOW_IF(true);
}

// For each transaction witness
// Note: witnesses reveal public key of an address
// and Ledger *does not* check whether they correspond to previously declared UTxOs
security_policy_t policyForSignTxWitness(const bip44_path_t* pathSpec)
{
	DENY_UNLESS(has_cardano_prefix_and_any_account(pathSpec));
	DENY_UNLESS(has_valid_change_and_any_address(pathSpec));

	// Perhaps we can relax these?
	WARN_IF(!has_reasonable_account_and_address(pathSpec))
	WARN_IF(is_too_deep(pathSpec));

	ALLOW_IF(true);
}

security_policy_t policyForSignTxMetadata()
{
	SHOW_IF(true);
}

security_policy_t policyForSignTxConfirm()
{
	PROMPT_IF(true);
}
