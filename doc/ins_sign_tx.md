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
