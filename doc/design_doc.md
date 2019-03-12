# Cardano Ledger App communication protocol

## Cardano app communication
Cardano app communicates with APDU protocol (a decent overview of APDU protocol can be found [here](http://cardwerk.com/smart-card-standard-iso7816-4-section-5-basic-organizations/#chap5_4)).
Each "logical" call consists of a series of APDU exchanges where APDU is in the following form

### Command

|field   |CLA|INS|P1 |P2 |Lc |Data| Le |
|--------|---|---|---|---|---|----|----|
|**size (B)**| 1 | 1 | 1 | 1 | 1 |variable |  0 |


Where
- `CLA=0xD7` is APDU class number. As we do not adhere to the strict APDU protocol, we have somehow arbitrarily chosen a value beloning to "proprietary structure and coding of command/response" CLA range
- `INS` is the instruction number
- `P1` and `P2` are instruction parameters
- `Lc` is length of the data body encoded as `uint8`. Note: unlike standard APDU, `ledger.js` produces `Lc` of exactly 1 byte (even for empty data). Data of length >= 256 are not supported
- Data is binary data
- `Le` is max length of response. This APDU field is **not** present in `ledger.js` protocol

**Ledger responsibilities**

Upon receiving general APDU, Ledger *must* check
- `rx` size >= 5 (i.e., the request has all required APDU fields)
- `CLA` is valid CLA of the Ledger Cardano App
- `INS` is known and enabled instruction. (Note: development version of the Ledger App might provide some testing/debugging instructions. Such version however *must* visibily display "devel" status to the user.)
- `Lc` is consistent with `rx`, i.e. `Lc + 5 == rx`
- `INS` is not changed in the middle of multi-APDU exchange. (Note: This is a security measure. Ledger Apps need to conserve RAM memory and thus might reuse the same memory regions for different INS calls. We must prevent attacks vectors where changing calls might lead to state confusion.)

## Response

Generally the response from the app looks like this:

|field| response data| SW1 | SW2 |
|-----|---|----|----|
|**size (B)**| variable | 1 | 1 |

where `SW1 SW2` represents the return code.
Known error codes are:
- 0x9000 = OK
- see [src/errors.h](../src/errors.h) for full listing of other errors


## Instructions

Instructions are split into several groups depending on their purpose. See [src/handlers.c](../src/handlers.c) for full listing

### `INS=0x0*` group

Instructions related to general app status
- `0x01`: [Get app version](ins_get_app_version.md)

### `INS=0x1*` group

Instructions related to public keys/addresses

- `0x10` [Get extended public key](ins_get_extended_public_key.md)
- `0x11` [Derive address](ins_derive_address.md)

### `INS=0x2*` group

Instructions related to transaction signing

- `0x20` [Attest UTxO](ins_attest_utxo.md)
- `0x21` [Sign Transaction](ins_sign_tx.md)

### `INS=0xF*` group

Instructions related to debug mode of the app. These instructions *must not* be available on the production build of the app

- `0xF0` Run unit tests
- `0xF2` Attest get session secret (return key used by AttestUTxO HMAC)
- `0xF3` Attest set sessoin secret (set key used by AttestUTxO HMAC)
## Protocol upgrade considerations:

In order to ensure safe forward compatibility, sender *must* set any *unused* field to zero. When upgrading protocol, any unused field that is no longer unused *must* define only values != 0. This will ensure that clients using old protocol will receive errors instead of an unexpected behavior.

❓(VL,IOHK): Do we want to force clients to check app version (mis)match, e.g. by having an explicit handshake? A custom (non `ledgerjs`-based) client app might omit version checks which might lead to potential problems
