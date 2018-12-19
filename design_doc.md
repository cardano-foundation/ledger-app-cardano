# Cardano Ledger App design document


## Cardano app communication
Cardano app communicates with APDU protocol.
Each "logical" call consists of a series of APDU exchanges where APDU is in the following form

### Command


|field   |CLA|INS|P1 |P2 |Lc |Data| Le |
|--------|---|---|---|---|---|----|----|
|**size (B)**| 1 | 1 | 1 | 1 | 1 |variable |  0 |


Where
- `CLA` is ????
- `INS` is the instruction number
- `P1` and `P2` are instruction parameters
- `Lc` is length of the data body. Note: unlike standard APDU, `ledger.js` produces `Lc` of exactly 1 byte (even for empty data). Data of length >= 256 are not supported
- Data is binary data
- `Le` is max length of response. This APDU field is **not** present in ledger.js protocol

### Response

Generally the response looks like this:

|field| response data| SW1 | SW2 |
|-----|---|----|----|
|**size (B)**| variable | 1 | 1 |

where `SW1 SW2` represents return code.
In general
- 0x90 00 = OK
- TBD



## GetAppVersion call

**Description**

Gets the version of the app running on Ledger. 
Could be called at any time

**Command**

|Field|Value|
|-----|-----|
| CLA | TBD |
| INS | TBD |
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
- TBD


**Ledger responsibilities**

- Check:
  - Check `P1 == 0`
  - Check `P2 == 0`
  - Check `Lc == 0`
- Respond with app version


## GetExtendedPublicKey call

**Description**
Get public key for a BIP path
Could be called at any time. (TBD: potentially we could require user consent for this operation. This would result in reduced UX though)

**Command**

|Field|Value|
|-----|-----|
| CLA | TBD |
| INS | TBD |
| P1 | Display address after response. 0 - do not display, 1 - display|
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
- TBD

**Ledger responsibilities**

- Check:
  - check P1 is valid
    - `P1 == 0 || P1 == 1`
  - check P2 is valid
    - `P2 == 0`
  - check data is valid:
    - `Lc >= 1` (we have path_len)
    - `path_len * 4 == Lc`
  - check derivatoin path is valid
    - `path_len >= 2`
    - `path[0] == 44'` (' means hardened)
    - `path[1] == 1815'`
- calculate public key
- respond with public key
- if `P1 == 1`
  - display public key to the user
  - do not perform APDU processing until user closes the public key screen

## DeriveAddress

Derive v2 address for a given BIP32 path, return and show it to the user for confirmation

## GetTrustedInput

**Description**

The purpose of this call is to return and attest amount for a given UTxO.

**Background**

When signing transactions, UTxOs given as transaction inputs are not to be trusted. This is because the current Cardano signing protocol does not check the UTxO amount. This results in a possible class of attacks where attackers lie to Ledger about UTxO value, potentially leading it to assume there is less money on the input and thus forcing user to unknowingly "trash" coins as fee. (Note: instead of an explicit attacker this threat model also includes bad transaction parsing on the client side, especially in JavaScript where `ADA_MAX_SUPPLY` does not fit into native `number` type). To avoid such issues, Ledger implementation of Cardano requires each transaction input to be verified before signing a transaction.

**Command**

|Field|Value|
|-----|-----|
| CLA | TBD |
| INS | TBD |
| P1 | frame |
| P2 | unused |
| Lc | variable |

TODO: design streaming protocol for this call

**Ledger responsibilities**

**Ledger transaction parsing (pseudocode)**

- token refers to CBOR token under the cursor. Note that token is variable-length in CBOR and therefore the application should wait for more data if it cannot fully determine the current token
- consume token moves cursor to the next token. Note that a special care must be done with consuming long byte-streams as we might need to consume several data frames
- parse functions parse data structure and move cursor to the next token after the data structure

```Python
parseTransaction():
  assert token == array(3)
  consume token
  {# 1
    parseInputs()
  }
  {# 2
    parseOutputs()
  }
  {# 3
    parseMetadata()
  }
  assert EOF
  
parseInputs():
  assert token == array(*)
  consume token
  {
    while token == array(2)
      parseInput()
  }
  assert token == end array(*)
  consume token
  

parseInput():
  assert token == array(2)
  consume token
  {# type
    assert token == unsigned(0)
    consume token
  }
  { # encoded address
    assert token == tag(24)
    consume token
    { # address. WARNING: We do not parse & verify address
      assert token = bytes (len)
      consume token
      consume len
    }
  }
  
parseOutputs()
  assert token == array(*)
  consume token
  {
    while token == array(2):
      parseOutput()
  }
  assert token == end array(*)
  consume token
  

parseRawAddress()
  assert token == array(2)
  consume token
  { # address
    assert token == tag(24)
    consume token
    { # address. WARNING: We do not parse & verify address
      assert token == bytes (len)
      consume token
      consume len
    }
  }
  { # checksum
    assert token = unsigned(var-len)
    consume token-var-len
  }
  
parseOutput()
  assert token == array(2)
  consume token
  { #base58-decoded (i.e. raw) address
    parseRawAddress()
  }
  { # coins
    assert token == unsigned(var-len)
    consume token-var-len
  }
```  


## SignTransaction

Given trusted transaction inputs and transaction outputs (addresses + amounts), construct and sign transaction.
TODO: design streaming protocol for this call
