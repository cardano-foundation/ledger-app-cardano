# Sign Transaction

**Description**

Given transaction inputs and transaction outputs (addresses + amounts), construct and sign a transaction.

Due to Ledger constraints and potential security implications (parsing errors), Cardano ledger app uses a custom, simplified, format for streaming the transaction to be signed. The main rationale behind not streaming directly the (CBOR-encoded) cardano raw transaction to Ledger is the following:  
1) App needs to support [attested transaction inputs](ins_attest_utxo.md) which are not part of the standard Cardano transaction message format.
2) App needs to support BIP44 change address outputs (Ledger should not display own change addresses to the user as this degrades UX)
3) Serializing is easier than parsing. This is true especially if transaction chunks would not be aligned with processing (e.g., inputs/outputs arbitrarily split to multiple APDUs). This also implies potentially smaller memory footprint on the device
4) SignTx communication protocol is more extensible in the future
5) Potential security improvement -- because SignTx does not output serialized transaction, only witnesses, the host app is responsible for serializing the transaction itself. Any serialization mismatch between host and Ledger would result in an transaction which is rejected by nodes. 

**SignTx Limitations**
- Output address size is limited to ~200 bytes (single APDU). (Note: IOHK is fine with address size limit of 100 bytes)
- Change address is limited to `m/44'/1815'/account'/1/changeIndex` where is recognized account. (TBD(VL)❓: for now `0 <= account <= 10` but we need to unify this across instructions) and `0 <= changeIndex < 1 000 000`. This makes it feasible to brute-force all change addresses in case an attacker manages to modify change address(es) (As user does not confirm change addresses, it is relatively easy to perform MITM attack).
- TBD: Input count? Output count?

**Communication protocol non-goals:**
The communication protocol is designed to *ease* the Ledger App implementation (and simplify potential edge conditions). As such, the protocol might need more APDU exchanges than strictly necessary. We deem this as a good tradeoff between implementation and performance (after all, the bottleneck are user UI confirmations).

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

**Command**

|Field|Value|
|-----|-----|
|  P1 | `0x01` |
|  P2 | unused |

**Data**

|Field| Length | Comments|
|------|-----|-----|
|Num of Tx inputs inputs| 4 | Big endian|
|Num of Tx outputs| 4 | Big endian|

### 2 - Set UTxO inputs

**Command**

|Field|Value|
|-----|-----|
|  P1 | `0x02` |
|  P2 | unused |
| data | Tx input, data depending on type |

**Data for SIGN_TX_INPUT_TYPE_UTXO**

|Field| Length | Comments|
|-----|--------|--------|
|Input type| 1 | `SIGN_TX_INPUT_TYPE_UTXO==0x01` |
|Attested input| 56 | Output of [attestUTxO call](ins_attest_utxo.md) |

Note that ledger should check that the tx contains exactly the announced number of inputs before proceeding to outputs.

**Ledger responsibilities**

- Check that `P1` is valid
 - previous call *must* had `P1 == 0x01` or `P1 == 0x02`
- Check that `P2` is unused
- Check that we are within advertised number of inputs
- Check that `attested_utxo` is valid (contains valid HMAC)
- Sum `attested_utxo.amount` into total transaction amount

### 3 - Set outputs & amounts

**Command**

|Field|Value|
|-----|-----|
|  P1 | `0x03` |
|  P2 | unused |
| data | Tx output, data depending on type |

**Data for SIGN_TX_OUTPUT_TYPE_ADDRESS**

This output type is used for regular destination addresses

|Field| Length | Comments|
|-----|--------|--------|
|Amount| 8| Big endian. Amount in Lovelace|
|Output type| 1 | `SIGN_TX_OUTPUT_TYPE_ADDRESS=0x01`|
|Address| variable | CBOR-encoded raw address (before base58-encoding)|

**Data for SIGN_TX_OUTPUT_TYPE_PATH**

This output type is used for change addresses. These *are not* shown to the user during validation (Note: some restrictions apply. See [src/securityPolicy.c](../src/securityPolicy.c) for details.)

|Field| Length | Comments|
|-----|--------|--------|
|Amount| 8| Big endian. Amount in Lovelace|
|Output type| 1 | `SIGN_TX_OUTPUT_TYPE_PATH=0x02`|
|BIP44 path| 1+4 * len | See [GetExtPubKey call](ins_get_extended_public_key.md) for a format example|

 
### 4 - Final confirmation

Ledger needs to calculate and display transaction fee.
User needs to confirm fee & approve transaction.

|Field|Value|
|-----|-----|
|  P1 | `0x05` |
|  P2 | (unused) |
| data | (none) |

### 5 - Compute witnesses

Given BIP44 path, sign TxHash by Ledger. Return the witness

**Command**

|Field|Value|
|-----|-----|
|  P1 | `0x05` |
|  P2 | (unused) |
| data | BIP44 path. See [GetExtPubKey call](ins_get_extended_public_key.md) for a format example |

**Response**

|Field|Length| Comments|
|-----|-----|-----|
|Witness extended public key| - | Not included in the response. Implementations either need to derive this or ask ledger explicitly. Note that this is a design decision to avoid leaking xpub to adversary|
|Signature|32| Witness signature. Implementations need to construct full witness by prepending xpub and serializing into CBOR|
