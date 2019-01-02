# Cardano Ledger App design document


## Cardano app communication
Cardano app communicates with APDU protocol.
Each "logical" call consists of a series of APDU exchanges where APDU is in the following form

### Command

|field   |CLA|INS|P1 |P2 |Lc |Data| Le |
|--------|---|---|---|---|---|----|----|
|**size (B)**| 1 | 1 | 1 | 1 | 1 |variable |  0 |


Where
- `CLA` is ❓
- `INS` is the instruction number
- `P1` and `P2` are instruction parameters
- `Lc` is length of the data body. Note: unlike standard APDU, `ledger.js` produces `Lc` of exactly 1 byte (even for empty data). Data of length >= 256 are not supported
- Data is binary data
- `Le` is max length of response. This APDU field is **not** present in ledger.js protocol

**Ledger responsibilities**

Upon receiving general APDU, Ledger should check
- `rx` size >= 5 (have all required APDU fields)
- `CLA` is CLA of the Cardano app
- `INS` is known instruction
- `Lc` is consistent with `rx`, i.e. `Lc + 5 == rx`
- `INS` is not changed in the middle of multi-APDU exchange

### Response

Generally the response looks like this:

|field| response data| SW1 | SW2 |
|-----|---|----|----|
|**size (B)**| variable | 1 | 1 |

where `SW1 SW2` represents return code.
In general
- 0x9000 = OK
- ❓ other codes


**Notes:**
In order to ensure safe forward compatibility, sender **must** set any *unused* field to zero. When upgrading protocol, any unused field that is no longer unused **must** have values != 0. This will ensure that clients using old protocol will receive errors instead of an unexpected behavior.


## GetAppVersion call

**Description**

Gets the version of the app running on Ledger. 
Could be called at any time

**Command**

|Field|Value|
|-----|-----|
| CLA | ❓ |
| INS | ❓ |
| P1 | unused |
| P2 | unused |
| Lc | 0 |

**Response**

|Field|Length|
|-----|-----|
|major| 1 |
|minor| 1 |
|patch| 1 |

Tuple [`major`, `minor`, `patch`] represents app version.


**Error codes**
- 0x9000 OK
- ❓ other codes


**Ledger responsibilities**

- Check:
  - Check `P1 == 0`
  - Check `P2 == 0`
  - Check `Lc == 0`
- Respond with app version


## GetExtendedPublicKey call

**Description**

Get public key for a BIP path.

Could be called at any time. 

**Command**

|Field|Value|
|-----|-----|
| CLA | ❓ |
| INS | ❓ |
| P1 | unused |
| P2 | unused |
| Lc | variable |

**Data**

|Field| Length | Comments |
|-----|--------|----------|
| BIP32 path len| 1 | min 2, max 10 |
| First derivation index | 4 | Big endian. Must be 44' |
| Second derivation index | 4 | Big endian. Must be 1815' |
| (optional) Third derivation index | 4 | Big endian |
| ... | ... | ... |
| (optional) Last derivation index | 4 | Big endian |

**Response**

|Field| Length |
|-----|--------|
|pub_key| 32 |
|chain_code| 32 |

Concatenation of `pub_key` and `chain_code` represents extended public key.

**Errors (SW codes)**

- 0x9000 OK
- ❓ other codes

**Ledger responsibilities**

- Check:
  - check P1 is valid
    - `P1 == 0`
  - check P2 is valid
    - `P2 == 0`
  - check data is valid:
    - `Lc >= 1` (we have path_len)
    - `path_len * 4 == Lc`
  - check derivatoin path is valid
    - `path_len >= 3`
    - `path[0] == 44'` (' means hardened)
    - `path[1] == 1815'`
    - `path[2] is hardened` (`path[2]` is account number)
- calculate public key
- respond with public key
- TBD: ❓Should we also support token validation?
- TBD: ❓Should we support permanent app setting where Ledger forces user to acknowledge public key retrieval before sending it to host?
- TBD: ❓Should there be an parameter to show the public key on display? Is it useful in any way?

## DeriveAddress

Derive v2 address for a given BIP32 path, return and show it to the user for confirmation. TODO: ❓spec


## AttestTransactionInput

**Description**

The purpose of this call is to return and attest amount for a given UTxO.

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


## SignTransaction

**Description**

Given transaction inputs and transaction outputs (addresses + amounts), construct and sign transaction.

Note: Unlike Bitcoin or Ethereum, we do not plan to support signing already pre-constructed transactions. Instead, Ledger performs the transaction serialization and returns it to the client. Rationale behind this is that
1) we need to support attested transaction inputs which are not part of standardized in Cardano
2) we need to support BIP32 change address outputs (Ledger should not display change addresses)
3) we need to support long output addresses (Cardano addresses can be >10kb long. As a consequence, Ledger cannot assume that it can safely store even a single full output address)


TODO: design streaming protocol for this call
