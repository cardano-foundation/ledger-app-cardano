**TODO:❓IOHK needs to decide which way we should go.**

The main issue we are trying to address with the respect of fee calculation is that Ledger cannot assume fee sent over the wire from the client is correct. As such, we are adding a new call which, given `(transaction, output number)` returns "attested UTxO" `(tx hash, output number, amount, per-session signature)`. With these verified inputs, Ledger can now securely compute transaction fee. Unfortunately, the implication is that transaction signing now needs to be supplied with complementary data which cannot be encoded in the raw transaction data. Here are our possible options

 a) "Encode" supplementary data within transaction's CBOR. I list this just for reference only as 1) it is a bad idea, upgrade of protocol could collide with the ad-hoc encoding and could lead to security vulnerabilities 2) There is no point in this anyway as both client and Ledger would need to compute hash of original transaction, not this supplementary data

 b) At this point, we can just use some custom data encoding between ledgerjs and Nano S which is geared towards easier parsing on the Nano S side. The Nano S would be responsible for transaction serialization and will return assembled transaction back to the ledgerjs. The main advantage of this method is that serialization is less error-prone than parsing (bad serialization code usually leads to rejected transactions, bad parsing code might lead to security vulnerabilities)

 c) Taking b) to ad extremum, Nano S does not need to return serialized transaction. Instead, Sending app would be responsible for both 1) giving inputs/outputs 2) for-own-use serialization; Nano S would serialize transaction *internally* and sign correct hash. The main advantage of this method is that it provides additional layer of security -- if the implementations do not agree on the serialization, the tx will be rejected by nodes (ledger signatures would not match txHash of client). Disadvantage is that the implementations would need to be kept on sync which could get tricky with transaction encoding upgrades.

 d) An alternative would be to send both "attested UTxOs" and normally encoded transaction to the Nano S for signing. Given the small memory, this would limit the number of transaction inputs that can be processed by the ledger.

 e) We can extend the attest UTxO digest to contain sum + session signature over multiple UTXos. This would add implementation complexity but would remove the memory constraint of d)

I was trying to think this out multiple times and I think b) is the best choice for now. e) would work but it is needlessly complicated and we are still left with a similar problem for the outputs (In order to get decent UX, NanoS should skip the change output from confirmations. This however means that Nano S must be able to somehow verify this change output is indeed its change. Easiest way is to either 1) supply also the derivation path and verify they match 2) extending 1) just don't bother to send the address, just a derivation path) (edited) 


# TODO(VL): rest of the document needs to be updated after we get feedback on the previous section

# Sign Transaction

**Description**

Given transaction inputs and transaction outputs (addresses + amounts), construct and sign a transaction.

**TODO:❓IOHK needs to review if the following assumptions make sense.**

Unlike Bitcoin or Ethereum, Cardano Ledger app does not support signing already pre-constructed transactions. Instead, the app performs the transaction serialization and returns it to the client.
Rationale behind this is that:
1) App needs to support [attested transaction inputs](ins_attest_utxo.md) which are not part of the standard Cardano transaction message format.
2) App needs to support BIP32 change address outputs (Ledger should not display own change addresses to the user as this degrades UX)
3) (TBD:❓ do we need to support this? What should be longest address Ledger can handle? Note that a resonable threshold might be 255 bytes (i.e., max 1 APDU)) App needs to support long output addresses (Cardano addresses can be >10kb long. As a consequence, Ledger cannot assume that it can safely store even a single full output address in the memory.)

**Non-goals:** The communication protocol is designed to *ease* the Ledger App implementation (and simplify potential edge conditions). As such, the protocol might need more APDU exchanges than strictly necessary. We deem this as a good tradeoff between implementation and performance (after all, the bottleneck are user UI confirmations).


Given these requirements in mind, here is a how transaction signing works:

## Preparation

As part of the transaction signing preparation, client should ask Ledger app to [attest inputs (UTxOs)](ins_attest_utxo.md). Note that attestation is a separate call for each UTxO and can be done irrelavant of signing, assuming it happens during the same *session*

## Signing

Transaction signing consists of an exchange of several APDUs. During this exchange, Ledger 

**General command**

|Field|Value|
|-----|-----|
| CLA | `0xD7` |
| INS | `0x21` |
|  P1 | signing phase |
|  P2 | (specific for each subcall) |

### 1 - Initialize signing

Initializes signing request.

|Field|Value|
|-----|-----|
|  P1 | `0x01` |
|  P2 | unused? |

**Response**
Returns transaction preamble

### 2 - Set UTxO inputs

**Command**
|Field|Value|
|-----|-----|
|  P1 | `0x02` |
|  P2 | input_number |
| data | n-th input |

Where P2 goes from 0 to the number of inputs.
Where format of `data` is TBD (note: presumably should be byte-copy of Attest UTxO output)
TBD: ❓Is `P2` the right place to keep the counter?

**Ledger responsibilities**

- Check that `P1` is valid
 - previous call *must* had `P1 == 0` or `P1 == 1`
- Check that `P2` is valid
 - If previous call had `P1==0` -> check `P2 == 0`
 - If previous call had `P1==1` -> check `P2 == P2_prev + 1`
- Check that `attested_utxo` is valid (contains valid HMAC)
- Sum `attested_utxo.amount` into total transaction amount
- Return CBOR-encoded UTxO part back to client

### 3 - Set outputs & amounts

**Command**
|Field|Value|
|-----|-----|
|  P1 | `0x03` |
|  P2 | output_number |
| data | output |

TBD: How do we handle outputs? There are multiple output types
1) 3-rd party address. This needs to be input as raw address data. What should the encoding be?
2) Ledger address belonging to the external chain. 
 - TBD: Do we need to do something specific here? (Potentialy show user BIP32 path instead of the address?) If yes, this needs specific encoding, otherwise it could be simply same as external address
3) Change address (Ledger address belonging to the internal chain).
 - TBD: Ledger needs to be able to either a) verify or b) construct this address. Option b) makes more sense. 
 - Ledger should probably not require any confirmation from the user about this address if this address is in reasonable range (e.g. belongs to the first 10000000 change addresses a-la Trezor logic). TBD: what should we do if we receive >1000000 index?
 
### 4 - Final confirmation
Ledger needs to calculate fee (and verify it is positive. TBD? Does ledger need to verify fee is big enough?).
User needs to confirm fee & approve transaction.
Return (raw) transaction finalization bytes.

### 5 - Compute witnesses
Given BIP32 path, sign TxHash by Ledger. Return the witness


TODO: ❓design streaming protocol for this call
