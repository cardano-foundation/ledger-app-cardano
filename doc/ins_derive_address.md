# Derive Address

Derive `v2` address for a given BIP32 path. (TBD: Optionally?) return and show it to the user for confirmation. 

Note: Unlike BTC app which returns both public key and the corresponding address in the same instruction call, we decided to split these two functionalities as they serve different purposes. Notably:
- DeriveAddress is weaker than GetPublicKey (Public key enables derivining non-hardened child keys, Address does not). As such, (in the future) the app might apply more restrictions/user confirmations to get the public key.
- GetAddress is usually called only for the purpose of address verification. As such, it should belong to a valid address BIP32 path.

**Command**

|Field|Value|
|-----|-----|
| CLA | ❓ |
| INS | ❓ |
| P1 | unused❓ (see TBD above) |
| P2 | unused |
| Lc | variable |

**Response**

|Field| Length |
|-----|--------|
|address| variable |

Where `address` is encoded in ❓ (TBD: our choices are either raw bytes (byte-efficient) or base58-encoded (1:1 corresponsence with what the user sees. We need to decide which one is better for the protocol).

**Ledger responsibilities**

- TBD❓: specify if/when the address needs to be verified by the user
- For checking the input, Ledger has the same responsibilities as for GetPublicKey.
- On top of that, the address *cannot* be that of an account, or of external/internal address chain root, i.e. it needs to have 
  - `path_len >= 5`,
  - `path[2] == 0` (account), and
  - `path[3] in [0,1]` (internal/external chain) (TODO(ppeshing): which one is which?)
  - (TBD❓: we do not think there is any point in returning *account* address but this needs to be checked with IOHK)
- If the address needs to be verified, Ledger should show it *after* it responds to the host. Note that until user confirms the address, Ledger should not process any subsequent instruction call.

