# Attest UTxO

**Description**

The purpose of this call is to return and attest ADA amount for a given UTxO.

**Background**

This relates to bullet 2 of Motivation section in the following BIP [https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki].

When signing transactions, UTxOs given as transaction inputs *do not* contain ADA amount but this amount is needed by Ledger to correctly calculate and display transaction fee. Ledger therefore must learn these amounts through some other way.

A naive implementation would be to send UTxO amounts to Ledger without any verification. This however leads to a possible class of attacks where attackers lie to Ledger about UTxO value, potentially leading it to assume there is less money on the input and thus forcing user to unknowingly "trash" coins as fee. (Note: instead of an explicit attacker this threat model also includes bad transaction parsing on the client side, especially in JavaScript where `ADA_MAX_SUPPLY` does not fit into native `number` type).

To address such issues, Ledger implementation of Cardano requires each transaction input to be "verified" before signing a transaction.


**Command**

|Field|Value|
|-----|-----|
| CLA | `0xD7` |
| INS | `0x20` |
| P1 | frame |
| P2 | unused |
| Lc | variable |

TODO: design streaming protocol for this call

**Ledger responsibilities**

- Validate that transaction parses correctly
- Check that transaction contains given output number
- Extract amount of the given output
- Sign (using app session key generated at app start) tuple `(TxHash, OutputNumber, Amount)` and return the tuple together with the signature

**Ledger transaction parsing compatibility**

For security reasons, we decided that transaction parsing *should not* be future-compatible. This stems from 2 practical considerations:

1) It is easier to audit code that asserts certain rigid parts of the transaction structure instead of code which would be able to transparently "skip" unknown parts
2) While very unlikely, it might be possible that transaction metadata in the future would change the meaning of parsed amount. An extreme example would be metadata changing amount unit from lovelace to ADA but subtler semantic changes are also possible.

For these two reasons, we believe it is safer for Ledger to reject unknown version of transaction encoding and force user to upgrade the App version to one which also implements the new schema.

**Ledger transaction parsing responsibility**

General description of Cardano Addresses: (https://cardanodocs.com/cardano/addresses/)
Ledger *must* check and correctly recognize following CBOR-encoded transaction structure.

```javascript
Array(3)[
 // 1 - inputs
 Array(*)[
   Array(2)[
     // addrType. Note: Fixed to 0 (public key addresses) for now
     Unsigned(0),
     // encoded address
     Tag(24)(
        // address. WARNING: We do not parse & verify address
        Bytes[??]
     )
 ],
 // 2 - outputs
 Array(*)[
   Array(2)[
    // raw (base58-decoded) address
    Array(2)[
      Tag(24){
        // Warning: We do not parse & verify this
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

# TODO: ❓Following section needs to be reviewed by IOHK!

**Checks that Ledger App *does not* implement:**

There are certain transaction validity checks that Ledger app will not (or cannot) check.
First of all, the App cannot check whether the referred UTxO transaction is actually included in the blockchain.
On top of that, Ledger relaxes following checks:
- Transaction length check. 
 - TBD: ❓maybe we should include this check
- Address checks. The app treats `Tag(24)` encoded addresses or their checksums as opaque and *does not* check their validity.
- Amount checks. The app *does not* check whether supplied amounts (and their sum) are valid or not.
  - TBD: ❓maybe we should include at least MAX_ADA check)
- Fee checks. The app *does not* verify that there is enough transaction fee.

We believe these checks *are not necessary* to attest UTxO as the attestation's sole purpose is to **bind** together UTxO (i.e., transaction hash + output number) and amount. This can be done whenever main transaction CBOR structure parses correctly. If the attacker supplies wrong transaction data, either
1) the Ledger app will show (from the user-perspective) unexpected amounts and the user would not confirm the transaction, or
2) the Ledger app would show (from the user-perspective) correct amounts but the UTxO has been meddled with. UTxO is, however, referred by transaction hash in the signed output transaction and as such, even if Ledger signs such transaction,  *Cardano blockchain nodes* will reject it.

**Signature specification (maybe question to IOHK)**
TBD: ❓. some sort of HMAC. It is unclear what should be used

**TBD:❓ (question to IOHK?)**
If this instruction is *not* limited by the user interaction, we have a signing oracle. Given that Ledger signs this *not* by it's public key but by a random *session* key, there does not seem to be any disadvantage in allowing this without user confirmation.
