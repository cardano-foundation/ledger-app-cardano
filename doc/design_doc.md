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

## [Get app version](ins_get_app_version.md)

## [Get extended public key](ins_get_extended_public_key.md)

## [Derive address](ins_derive_address.md)

## [Attest UTxO](ins_attest_utxo.md)

## [Sign Transaction](ins_sign_tx.md)

