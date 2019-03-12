# Derive Address

**IOHK nicolas/vincent**: just as a note, the deterministic address recovery mechanism is describe in the BIP32, the path model used in IOHK and recommended to use is BIP44.

Derive `v2` address for a given BIP44 path, return, and show it to the user for confirmation.
We expect this call to be used for the address verification purposes (i.e., matching address on Ledger with the one on the screen).


Note: Unlike BTC Ledger app which returns both public key and the corresponding address in the same instruction call, we split these two functionalities as they serve different purposes. Notably:
- `DeriveAddress` is weaker than `GetExtendedPublicKey` (Extended public key enables derivining non-hardened child keys, address does not). As such, (in the future) the app might apply more restrictions/user confirmations to get the public key.
- `GetAddress` is typically called only for the purpose of address verification. As such, it should belong to a valid address BIP32 path.
- Note that implementations would typically call `GetAddress` with `P1_DISPLAY` to display the address to the user and `P1_RETURN` is usually not needed because the wallet anyway requested account's extended public key which enables it to derive all addresses. `P1_RETURN` can be used by paranoid users that do not want to expose account public key to the host yet they still want to be able to export individual addresses.

**Command**

| Field | Value                   |
| ----- | ----------------------- |
| CLA   | `0xD7`                  |
| INS   | `0x11`                  |
| P1    | request type: `P1_RETURN=0x01` for returning address to host, `P1_DISPLAY=0x02` for displaying address on the screen |
| P2    | unused                  |
| Lc    | variable                |

**Response**

| Field   | Length   |
| ------- | -------- |
| address | variable |

Where `address` is encoded in raw bytes (e.g., fully CBOR-encoded but without base58 conversion)

**Ledger responsibilities**

- For checking the input, Ledger has the same responsibilities as in `GetPublicKey`.
- On top of that, the address *cannot* be that of an account, or of external/internal address chain root, i.e. it needs to have 
  - `path_len >= 5`,
  - `path[2] == 0'` (account), and
  - `path[3] in [0,1]` (internal/external chain)
- Ledger might impose more restrictions, see implementation of `policyForReturnDeriveAddress`/`policyForShowDeriveAddress` in [src/securityPolicy.c](../src/securityPolicy.c) for details
- If the request is to show the address Ledger should wait with the response before sending response. Note that until user confirms the address, Ledger should not process any subsequent instruction call.
