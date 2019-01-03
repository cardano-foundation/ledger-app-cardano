# Attest UTxO

**Description**

The purpose of this call is to return and attest ADA amount for a given UTxO.

**Background**

This relates to bullet 2 of Motivation section in the following BIP [https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki].

When signing transactions, UTxOs given as transaction inputs *do not* contain UTxO amount which is needed by Ledger to correctly calculate and display transaction fee. Ledger therefore must learn these amounts through some other way.

A naive implementation would be to send UTxO amounts to Ledger without any verification. This however leads to a possible class of attacks where attackers lie to Ledger about UTxO value, potentially leading it to assume there is less money on the input and thus forcing user to unknowingly "trash" coins as fee. (Note: instead of an explicit attacker this threat model also includes bad transaction parsing on the client side, especially in JavaScript where `ADA_MAX_SUPPLY` does not fit into native `number` type).

To address such issues, Ledger implementation of Cardano requires each transaction input to be "verified" before signing a transaction.


**Command**

|Field|Value|
|-----|-----|
| CLA | ❓ |
| INS | ❓ |
| P1 | frame |
| P2 | unused |
| Lc | variable |

TODO: design streaming protocol for this call

**Ledger responsibilities**

- Validate transaction while parsing it
- Check that transaction contains given input number
- Extract amount of given input
- Sign (using app session key generated at app start) tuple `(TxHash, InputNumber, Amount)` and return the tuple together with the signature

**Ledger transaction parsing compatibility**

For security reasons, we decided that transaction parsing should *not* be future-compatible. This stems from 2 practical considerations:

1) It is easier to audit code that asserts certain rigid parts of the transaction structure instead of code which would be able to transparently "skip" unknown parts
2) While very unlikely, it might be possible that transaction metadata in the future would change the meaning of parsed amount. An extreme example would be metadata changing amount unit from lovelace to ADA but subtler semantic changes are also possible.

For these two reasons, we believe it is safer for Ledger to reject unknown version of transaction encoding and force user to upgrade the App version to one which also implements the new schema.

**Ledger transaction parsing responsibility**


```javascript
Array(3)[
 // 1 - inputs
 Array(*)[
   Array(2)[
     // type. Note: Fixed to 0
     Unsigned(0),
     // encoded address
     Tag(24)(
        // address. WARNING: We do not parse & verify address
        Bytes(??)
     )
 ],
 // 2 - outputs
 Array(*)[
   Array(2)[
    // raw (base58-decoded) address
    Array(2)[
      Tag(24){
        // Warning: We do not parse & verify this
        Bytes(??),
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
- `Bytes(x)` means byte sequence containing `x`
- `??` means we do not constrain this value


