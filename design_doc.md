# Cardano Ledger App design document


## Cardano app communication
Cardano app communicates with APDU protocol.
Each "logical" call consists of a series of APDU exchanges where APDU is in the following form

### Command


|field   |CLA|INS|P1 |P2 |Lc |Data|
|--------|---|---|---|---|---|----|
|size (B)| 1 | 1 | 1 | 1 | 1 |var |


Where
- CLA is ????
- INS is the instruction number
- P1 and P2 are instruction parameters
- Lc is length of the data body
- Data is binary data


### Response

???



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
| Le | ? |

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
| Le | ? |

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
