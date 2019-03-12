# Attest UTxO

**Description**

The purpose of this call is to return and attest ADA amount for a given UTxO.

**Background**

This relates to bullet 2 of Motivation section in the following BIP [https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki].

When signing transactions, UTxOs given as transaction inputs *do not* contain ADA amount but this amount is needed by Ledger to correctly calculate and display transaction fee. Ledger therefore must learn these amounts through some other way.

A naive implementation would be to send UTxO amounts to Ledger without any verification. This however leads to a possible class of attacks where attackers lie to Ledger about UTxO value, potentially leading it to assume there is less money on the input and thus forcing user to unknowingly "trash" coins as fee. (Note: instead of an explicit attacker this threat model also includes bad transaction parsing on the client side, especially in JavaScript where `ADA_MAX_SUPPLY` does not fit into native `number` type).

To address such issues, Ledger implementation of Cardano requires each transaction input to be "verified" before signing a transaction.

**General command fields**

|Field|Value|
|-----|-----|
| CLA | `0xD7` |
| INS | `0x20` |
| P1 | is first/subsequent frame |
| P2 | unused |
| Lc | variable |
| Data | variable |

**Command structure**

`P1=0x01` (first frame)

|Data field|Width (B)|Comment|
|----------|---------|-------|
| outNum   |  4 (Big-endian)     | UTxO output number, indexed by 0|

`P1=0x02` (subsequent frames)

|Data Field|Width (B)|
|----------|---------|
| txChunk | variable |

**Response**

While Tx is not finished, the response is empty.

Upon receiving last txChunk (as determined by the transaction parsing) the response is

|Response data|Width (B)|Comment|
|----------|---------|-------|
| txHash   | 32 | |
| outNum | 4 (Big-endian) | same as in initial command|
| amount | 8 (Big-endian) | Output amount, in Lovelace|
| attestation | 16 | HMAC(txHash, outNum, amount, session key)|


**Ledger responsibilities**

- Validate that transaction parses correctly (for details see below)
- Check that transaction contains given output number
- Extract amount of the given output
- Sign (using app session key generated at app start) tuple `(TxHash, OutputNumber, Amount)` and return the tuple together with the signature. This binds together UTxO and amount so that Ledger won't need to read the whole UTxO later in order to confirm the amount.

Note on HMAC: We use `HMAC_SHA256(key=32 byte session key, message=(txHash,outNum,amount))` to compute the signature digest and return first 16 bytes of it. This should be resilient enough to birthday paradox and other attacks as the key is just per session and we Ledger's signing speed is slow.

**Ledger transaction parsing compatibility**

For security reasons, we decided that transaction parsing *should not* be future-compatible. This stems from 2 practical considerations:

1) It is easier to audit code that asserts certain rigid parts of the transaction structure instead of code which would be able to transparently "skip" unknown parts
2) While very unlikely, it might be possible that transaction metadata in the future would change the meaning of parsed amount. An extreme example would be metadata changing amount unit from lovelace to ADA but subtler semantic changes are also possible.

For these two reasons, we believe it is safer for Ledger to reject unknown version of transaction encoding and force user to upgrade the App version to one which also implements the new schema.

**Ledger transaction parsing responsibility**

**Note: Ledger currently supports pre-Shelley transactions only!**

General description of Cardano Addresses: (https://cardanodocs.com/cardano/addresses/)
Ledger *must* check and correctly recognize following CBOR-encoded transaction structure.

```javascript
Array(3)[
 // 1 - inputs
 Array(*)[ 
   // Next array represents single input
   Array(2)[
     // addrType. Note: We support only 0 (public key addresses) for now
     Unsigned(0),
     // encoded address
     Tag(24)(
        // address. WARNING: We do not parse & verify address
        Bytes[??]
     )
 ],
 // 2 - outputs
 Array(*)[
   // Next array represents single output
   Array(2)[
    // raw (base58-decoded) address
    Array(2)[
      Tag(24){
        // address. WARNING: We do not parse & verify address
        Bytes[??],
      },
      // checksum. WARNING: We do not verify checksum
      Unsigned(??),
    ],
    // amount (in lovelace)
    Unsigned(??)   
   ]
 ],
 //3 - metadata
 Map(0){} // Note: Fixed to be empty
]
```

Where 
- `Array(x)` means array of length `x` or `*` for CBOR indefinite array.
- `Map(x)` means map of length `x`
- `Unsigned(x)` means unsigned iteger of value `x`
- `Tag(x)` means tagged value with tag `x`
- `Bytes[x]` means byte sequence containing `x`
- `??` means we do not constrain this value

**Checks that Ledger App *does not* implement:**

There are certain transaction validity checks that Ledger app will not (or cannot) check.
First of all, the App cannot check whether the referred UTxO transaction is actually included in the blockchain.
On top of that, Ledger relaxes following checks:
- We skip transaction max-length check (i.e., that the transaction is shorter than 64KB).
- Address checks. The app treats `Tag(24)` encoded addresses or their checksums as opaque and *does not* check their validity.
- Amount checks. The app *does not* check whether supplied amounts (and their sums) are valid or not.
- Fee checks. The app *does not* verify that there is enough transaction fee not that sum(inputs) > sum(outputs)

These skipped checks *are not necessary* in order to attest UTxO as the attestation's sole purpose is to **bind** together UTxO (i.e., transaction hash + output number) and amount. This can be done whenever main transaction CBOR structure parses correctly. If the attacker supplies wrong transaction data, then (during SignTx) either
1) the Ledger app will show (from the user-perspective) unexpected amounts and the user would not confirm the transaction, or
2) the Ledger app would show (from the user-perspective) correct amounts but the UTxO has been meddled with. UTxO is, however, referred to by the hash in the transaction under signing and as such, even if Ledger signs such transaction,  *Cardano blockchain nodes* will reject it.

**Signing oracle**
❓(IOHK): Check if this isn't an issue.
This instruction is *not* limited by the user interaction => we have a (relatively fast) signing oracle. Given that Ledger signs this *not* by it's public key but by a random ephemeral *session* key, there does not seem to be any disadvantage in allowing this call without user confirmation.
